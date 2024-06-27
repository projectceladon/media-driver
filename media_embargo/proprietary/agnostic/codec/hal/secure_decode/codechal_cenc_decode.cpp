/*
// Copyright (C) 2017-2020 Intel Corporation
//
// Licensed under the Apache License,Version 2.0(the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
//!
//! \file      codechal_cenc_decode.cpp
//! \brief     Impelements the public interface for CodecHal CENC Decode
//!
#include <algorithm>
#include <thread>

#include "codechal_cenc_decode.h"
#include "mos_solo_generic.h"
#include "codechal_mmc.h"
#include "codechal_debug.h"
#include "mos_os_virtualengine.h"
#include "codechal_hw.h"

#ifdef _AVC_DECODE_SUPPORTED
#include "codechal_cenc_decode_avc.h"
#endif

#ifdef _HEVC_DECODE_SUPPORTED
#include "codechal_cenc_decode_hevc.h"
#endif

#include "mhw_vdbox_g12_X.h"
#include "codechal_user_settings_mgr_ext.h"

#define VDBOX_HUC_DECODE_DECRYPT_KERNEL_DESCRIPTOR 2
#define INIT_SECOND_LEVEL_BB_HEAP_SIZE_PAGES 512

CodechalCencDecode::CodechalCencDecode()
{
    MOS_ZeroMemory(&m_platform, sizeof(m_platform));
    m_shareBuf.resTracker = &m_resTracker;
    m_shareBuf.resStatus  = &m_resEncryptedStatusReportNum;
    MOS_ZeroMemory(&m_decryptStatusBuf, sizeof(m_decryptStatusBuf));
    MOS_ZeroMemory(&m_resSlcLevelBatchBuf, CODECHAL_NUM_DECRYPT_BUFFERS * sizeof(MHW_BATCH_BUFFER));
}

CodechalCencDecode::~CodechalCencDecode()
{
    MEDIA_FEATURE_TABLE *skuTable;

    CENC_DECODE_FUNCTION_ENTER;

    CENC_DECODE_CHK_NULL_NO_STATUS(m_osInterface);

    // free status report buffer
    if (m_decryptStatusBuf.pData)
    {
        m_osInterface->pfnUnlockResource(m_osInterface, &m_decryptStatusBuf.resStatusBuffer);
        m_decryptStatusBuf.pData = nullptr;
    }
    m_osInterface->pfnFreeResource(m_osInterface, &m_decryptStatusBuf.resStatusBuffer);

    // free shared buffer

    if (m_hucSharedBuf)
    {
        m_osInterface->pfnUnlockResource(m_osInterface, &m_resHucSharedBuf);
        m_hucSharedBuf = nullptr;
    }
    m_osInterface->pfnFreeResource(m_osInterface, &m_resHucSharedBuf);

    if (m_pr30Enabled)
    {
        m_osInterface->pfnFreeResource(m_osInterface, &m_resEncryptedStatusReportNum);
    }

    MOS_Delete(m_secondLvlBbHeap);
    MOS_Delete(m_bitstreamHeap);
    m_osInterface->pfnUnlockResource(m_osInterface, &m_resTracker);
    m_osInterface->pfnFreeResource(m_osInterface, &m_resTracker);

    for (int i = 0; i < CODECHAL_NUM_DECRYPT_BUFFERS; i++)
    {
        if (!MEDIA_IS_SKU(m_skuTable, FtrCencDecodeRemove2ndLevelBatchBufferCopy))
        {
            // for not dg2+, free 2nd level batch buffer
            Mhw_FreeBb(m_osInterface, &m_resSlcLevelBatchBuf[i], nullptr);
        }
        m_osInterface->pfnFreeResource(m_osInterface, &m_resHucDmem[i]);
        m_osInterface->pfnFreeResource(m_osInterface, &m_resHucIndCryptoState[i]);
        m_osInterface->pfnFreeResource(m_osInterface, &m_resHucSegmentInfoBuf[i]);
        m_osInterface->pfnFreeResource(m_osInterface, &m_resHucSegmentInfoBufClear[i]);
    }
    for (int i = 0; i < CENC_NUM_DECRYPT_SHARED_BUFFERS; i++)
    {
        m_osInterface->pfnFreeResource(m_osInterface, &m_resHcpIndCryptoState[i]);
    }
#if USE_CODECHAL_DEBUG_TOOL
    if (m_debugInterface)
    {
        MOS_Delete(m_debugInterface);
        m_debugInterface = nullptr;
    }
#endif
    if (m_hwInterface)
    {
        MOS_Delete(m_hwInterface);
        m_hwInterface = nullptr;
    }

    m_osInterface->pfnFreeResource(m_osInterface, &m_dummyStreamIn);
    m_osInterface->pfnFreeResource(m_osInterface, &m_dummyStreamOut);
    m_osInterface->pfnFreeResource(m_osInterface, &m_hucDmemDummy);

    if (!Mos_ResourceIsNull(&m_resDataBuffer))
    {
        m_osInterface->pfnFreeResource(m_osInterface, &m_resDataBuffer);
    }

    skuTable = m_osInterface->pfnGetSkuTable(m_osInterface);

    if (skuTable && MEDIA_IS_SKU(skuTable, FtrVcs2))
    {
        m_osInterface->pfnDestroyVideoNodeAssociation(m_osInterface, m_videoGpuNode);
    }
    m_osInterface->pfnDestroy(m_osInterface, false);
    MOS_FreeMemory(m_osInterface);

    return;
}

MOS_STATUS CodechalCencDecode::Create(
    CODECHAL_STANDARD    standard,
    CodechalCencDecode **cencDecoder)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    CENC_DECODE_FUNCTION_ENTER;

    CENC_DECODE_CHK_NULL(cencDecoder);

    switch (standard)
    {
    case CODECHAL_AVC:
        *cencDecoder = MOS_New(CodechalCencDecodeAvc);
        break;
    case CODECHAL_HEVC:
        *cencDecoder = MOS_New(CodechalCencDecodeHevc);
        break;
    default:
        CENC_DECODE_ASSERTMESSAGE("Decrypt not supported by Decode Standard.");
        eStatus = MOS_STATUS_INVALID_PARAMETER;
        break;
    }

    return eStatus;
}

MOS_STATUS CodechalCencDecode::VerifySpaceAvailable()
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;
    uint32_t   requestedSize, additionalSizeNeeded;
    uint32_t   requestedPatchListSize;

    CENC_DECODE_FUNCTION_ENTER;

    CENC_DECODE_CHK_NULL(m_osInterface);

    requestedSize          = m_hucDecryptSizeNeeded;
    requestedPatchListSize = 0;
    additionalSizeNeeded   = COMMAND_BUFFER_RESERVED_SPACE;

    // Try a maximum of 3 attempts to request the required sizes from OS
    // OS could reset the sizes if necessary, therefore, requires to re-verify
    for (int i = 0; i < 3; i++)
    {
        eStatus = (MOS_STATUS)m_osInterface->pfnVerifyCommandBufferSize(
            m_osInterface,
            requestedSize,
            0);

        if (eStatus == MOS_STATUS_SUCCESS)
        {
            return eStatus;
        }
        else
        {
            if (m_osInterface->bUsesCmdBufHeaderInResize)
            {
                MOS_COMMAND_BUFFER cmdBuffer;
                CENC_DECODE_CHK_STATUS(m_osInterface->pfnGetCommandBuffer(m_osInterface, &cmdBuffer, 0));
                CENC_DECODE_CHK_STATUS(m_miInterface->AddMiNoop(&cmdBuffer, nullptr));
                m_osInterface->pfnReturnCommandBuffer(m_osInterface, &cmdBuffer, 0);
            }

            CENC_DECODE_CHK_STATUS(m_osInterface->pfnResizeCommandBufferAndPatchList(m_osInterface, requestedSize + additionalSizeNeeded, 0, 0));
        }
    }

    return eStatus;
}

MOS_STATUS CodechalCencDecode::SendPrologWithFrameTracking(
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    MHW_GENERIC_PROLOG_PARAMS genericPrologParams;
    MOS_STATUS                eStatus = MOS_STATUS_SUCCESS;

    CENC_DECODE_FUNCTION_ENTER;

    CENC_DECODE_CHK_NULL(cmdBuffer);

    // initialize command buffer attributes
    cmdBuffer->Attributes.bTurboMode                   = false;
    cmdBuffer->Attributes.bEnableMediaFrameTracking    = true;
    cmdBuffer->Attributes.resMediaFrameTrackingSurface = &m_decryptStatusBuf.resStatusBuffer;
    cmdBuffer->Attributes.dwMediaFrameTrackingTag      = m_decryptStatusBuf.dwSWStoreData;

    // Set media frame tracking address offset(the offset from the decoder status buffer page, refer to CodecHalDecode_Initialize)
    cmdBuffer->Attributes.dwMediaFrameTrackingAddrOffset = 0;

    MOS_ZeroMemory(&genericPrologParams, sizeof(genericPrologParams));
    genericPrologParams.pOsInterface  = m_osInterface;
    genericPrologParams.pvMiInterface = m_miInterface;
    genericPrologParams.bMmcEnabled   = m_mmcEnabled;

    CENC_DECODE_CHK_STATUS(Mhw_SendGenericPrologCmd(cmdBuffer, &genericPrologParams));

    return eStatus;
}

MOS_STATUS CodechalCencDecode::AllocateResources()
{
    MOS_STATUS                   eStatus = MOS_STATUS_SUCCESS;
    MOS_ALLOC_GFXRES_PARAMS      allocParamsForBufferLinear;
    MOS_LOCK_PARAMS              lockFlags;
    PCODECHAL_CENC_STATUS_BUFFER statusBuf;
    uint8_t *                    data;
    uint32_t                     i;
    uint32_t *                   lockedTracker = nullptr;

    CENC_DECODE_FUNCTION_ENTER;
    CENC_DECODE_CHK_NULL(m_osInterface);

    statusBuf = &m_decryptStatusBuf;

    // diff slice number, shared buffer size and Pic Params for diff standards
    CENC_DECODE_CHK_STATUS(InitializeAllocation());

    // DMEM structure is common for all standards
    m_dmemBufSize         = sizeof(CODECHAL_CENC_DMEM);
    m_incompleteReportNum = 0;

    // initiate allocation paramters and lock flags
    MOS_ZeroMemory(&allocParamsForBufferLinear, sizeof(MOS_ALLOC_GFXRES_PARAMS));
    allocParamsForBufferLinear.Type     = MOS_GFXRES_BUFFER;
    allocParamsForBufferLinear.TileType = MOS_TILE_LINEAR;
    allocParamsForBufferLinear.Format   = Format_Buffer;

    MOS_ZeroMemory(&lockFlags, sizeof(MOS_LOCK_PARAMS));
    lockFlags.ReadOnly = 1;

    // shared buffer (updated by Huc kernel)
    MOS_ZeroMemory(&m_resHucSharedBuf, sizeof(MOS_RESOURCE));
    // set size to 4kB aligned
    allocParamsForBufferLinear.dwBytes  = MOS_ALIGN_CEIL(m_sharedBufSize, CODECHAL_PAGE_SIZE);
    allocParamsForBufferLinear.pBufName = "HucSharedBuffer";
    CENC_DECODE_CHK_STATUS(m_osInterface->pfnAllocateResource(
        m_osInterface,
        &allocParamsForBufferLinear,
        &m_resHucSharedBuf));
    m_hucSharedBuf = m_osInterface->pfnLockResource(m_osInterface, &m_resHucSharedBuf, &lockFlags);

    // initialize the shared buffer
    InitializeSharedBuf(false);
    CENC_DECODE_VERBOSEMESSAGE("shared buf allocate size = %d.", allocParamsForBufferLinear.dwBytes);

    Mos_ResetResource(&m_resTracker);
    allocParamsForBufferLinear.dwBytes  = MHW_CACHELINE_SIZE;
    allocParamsForBufferLinear.pBufName = "TrackerResource";
    CENC_DECODE_CHK_STATUS(m_osInterface->pfnAllocateResource(
        m_osInterface,
        &allocParamsForBufferLinear,
        &m_resTracker));
    MOS_ZeroMemory(&lockFlags, sizeof(MOS_LOCK_PARAMS));
    lockFlags.WriteOnly = 1;
    lockedTracker =
        (uint32_t *)m_osInterface->pfnLockResource(m_osInterface, &m_resTracker, &lockFlags);
    CENC_DECODE_CHK_NULL(lockedTracker);
    *lockedTracker   = MemoryBlock::m_invalidTrackerId;
    m_lockedTracker = lockedTracker;
    m_currTrackerId  = 1;

    m_secondLvlBbHeap->SetDefaultBehavior(HeapManager::Behavior::destructiveExtend);
    if (MEDIA_IS_SKU(m_skuTable, FtrCencDecodeRemove2ndLevelBatchBufferCopy))
    {
        // for dg2+ set initial size to 2048K=32*64K=512*CODECHAL_PAGE_SIZE to store at least 32 frame 64K batch buffer in typical netflix avc usage to avoid heap resize
        CENC_DECODE_CHK_STATUS(m_secondLvlBbHeap->SetInitialHeapSize(
            CODECHAL_PAGE_SIZE * INIT_SECOND_LEVEL_BB_HEAP_SIZE_PAGES));
    }
    else
    {
        // smallest size is 1 page per decrypt buffer
        CENC_DECODE_CHK_STATUS(m_secondLvlBbHeap->SetInitialHeapSize(
            CODECHAL_PAGE_SIZE * CENC_NUM_DECRYPT_SHARED_BUFFERS >> 1));
    }
    CENC_DECODE_CHK_STATUS(m_secondLvlBbHeap->RegisterTrackerResource(
        lockedTracker));
    m_bitstreamHeap->SetDefaultBehavior(HeapManager::Behavior::destructiveExtend);
    // first heap requested for 4k resolution
    CENC_DECODE_CHK_STATUS(m_bitstreamHeap->SetInitialHeapSize(
        CODEC_MAX_DECYRPT_WIDTH * (CODEC_MAX_DECRYPT_HEIGHT * 3) >> 1));
    CENC_DECODE_CHK_STATUS(m_bitstreamHeap->RegisterTrackerResource(
        lockedTracker));

    m_hucSegmentInfoBufSize        = 0;
    m_hucIndCryptoStateNumSegments = CODECHAL_NUM_IND_CRYPTO_BUFFERS;

    if (m_pr30Enabled)
    {
        MOS_ALLOC_GFXRES_PARAMS      allocParamsForBuffer;
        MOS_ZeroMemory(&allocParamsForBuffer, sizeof(MOS_ALLOC_GFXRES_PARAMS));
        allocParamsForBuffer.Type = allocParamsForBufferLinear.Type;
        allocParamsForBuffer.TileType = allocParamsForBufferLinear.TileType;
        allocParamsForBuffer.Format = allocParamsForBufferLinear.Format;

        Mos_ResetResource(&m_resEncryptedStatusReportNum);
        allocParamsForBuffer.dwBytes =
            MHW_CACHELINE_SIZE * 3 * CENC_NUM_DECRYPT_SHARED_BUFFERS;
        allocParamsForBuffer.pBufName = "PR3_0EncryptedStatusReportNum";
        if (MEDIA_IS_SKU(m_osInterface->pfnGetSkuTable(m_osInterface), FtrLimitedLMemBar) || MEDIA_IS_SKU(m_osInterface->pfnGetSkuTable(m_osInterface), FtrPAVPLMemOnly))
        {
            allocParamsForBuffer.dwMemType = MOS_MEMPOOL_DEVICEMEMORY;
            allocParamsForBuffer.Flags.bNotLockable = true;
        }
        CENC_DECODE_CHK_STATUS(m_osInterface->pfnAllocateResource(
            m_osInterface,
            &allocParamsForBuffer,
            &m_resEncryptedStatusReportNum));

        // skip resource sync
        CENC_DECODE_CHK_STATUS(m_osInterface->pfnSkipResourceSync(&m_resEncryptedStatusReportNum));

    }
    m_sliceLevelBatchBufSize =
        (m_standardSlcSizeNeeded * m_numSlices) + sizeof(uint32_t);
    m_sliceLevelBatchBufSize = MOS_ALIGN_CEIL(m_sliceLevelBatchBufSize, CODECHAL_PAGE_SIZE);

    if (!MEDIA_IS_SKU(m_skuTable, FtrCencDecodeRemove2ndLevelBatchBufferCopy))
    {
        // for not dg2+, need clean m_resSlcLevelBatchBuf before use
        MOS_ZeroMemory(
            &m_resSlcLevelBatchBuf,
            sizeof(m_resSlcLevelBatchBuf));
    }

    for (i = 0; i < CODECHAL_NUM_DECRYPT_BUFFERS; i++)
    {
        if (!MEDIA_IS_SKU(m_skuTable, FtrCencDecodeRemove2ndLevelBatchBufferCopy))
        {
            // for not dg2+, allocate 2nd level batch buffer, assuming 4k resolution
            CENC_DECODE_CHK_STATUS(Mhw_AllocateBb(
                m_osInterface,
                &m_resSlcLevelBatchBuf[i],
                nullptr,
                m_sliceLevelBatchBufSize));
            m_resSlcLevelBatchBuf[i].bSecondLevel = true;
#if (_DEBUG || _RELEASE_INTERNAL)
            m_resSlcLevelBatchBuf[i].iLastCurrent = m_resSlcLevelBatchBuf[i].iSize;
#endif  // (_DEBUG || _RELEASE_INTERNAL)
        }
        // allocate the resource for DMEM command
        MOS_ZeroMemory(&m_resHucDmem[i], sizeof(MOS_RESOURCE));
        allocParamsForBufferLinear.dwBytes  = MOS_ALIGN_CEIL(m_dmemBufSize, MHW_CACHELINE_SIZE);
        allocParamsForBufferLinear.pBufName = "HucDmemBuffer";
        CENC_DECODE_CHK_STATUS((MOS_STATUS)m_osInterface->pfnAllocateResource(
            m_osInterface,
            &allocParamsForBufferLinear,
            &m_resHucDmem[i]));

        Mos_ResetResource(&m_resHucIndCryptoState[i]);
        allocParamsForBufferLinear.dwBytes  = m_cpInterface->GetSizeOfCmdIndirectCryptoState() * CODECHAL_NUM_IND_CRYPTO_BUFFERS;
        allocParamsForBufferLinear.pBufName = "HucIndirectCryptoStateBuffer";
        CENC_DECODE_CHK_STATUS(m_osInterface->pfnAllocateResource(
            m_osInterface,
            &allocParamsForBufferLinear,
            &m_resHucIndCryptoState[i]));

        // allocate the resource for segmentinfo if support
        if (m_enableAesNativeSupport)
        {
            Mos_ResetResource(&m_resHucSegmentInfoBuf[i]);
            m_hucSegmentInfoBufSize             = sizeof(CODECHAL_HUC_SEGMENT_INFO) * CODECHAL_NUM_IND_CRYPTO_BUFFERS;
            m_hucSegmentInfoBufSize             = MOS_ALIGN_CEIL(m_hucSegmentInfoBufSize, CODECHAL_PAGE_SIZE);
            allocParamsForBufferLinear.dwBytes  = m_hucSegmentInfoBufSize;
            allocParamsForBufferLinear.pBufName = "HucSegmentInfoBuffer";
            CENC_DECODE_CHK_STATUS(m_osInterface->pfnAllocateResource(
                m_osInterface,
                &allocParamsForBufferLinear,
                &m_resHucSegmentInfoBuf[i]));

            m_hucSegmentInfoNumSegments[i] = CODECHAL_NUM_IND_CRYPTO_BUFFERS;

            Mos_ResetResource(&m_resHucSegmentInfoBufClear[i]);
            allocParamsForBufferLinear.dwBytes  = m_hucSegmentInfoBufSize;
            allocParamsForBufferLinear.pBufName = "HucSegmentInfoBuffer";
            CENC_DECODE_CHK_STATUS(m_osInterface->pfnAllocateResource(
                m_osInterface,
                &allocParamsForBufferLinear,
                &m_resHucSegmentInfoBufClear[i]));
        }

        m_sizeOfLength[i] = 0;
    }
    for (i = 0; i < CENC_NUM_DECRYPT_SHARED_BUFFERS; i++)
    {
        if (m_enableAesNativeSupport)
        {
            Mos_ResetResource(&m_resHcpIndCryptoState[i]);
            allocParamsForBufferLinear.dwBytes  = m_cpInterface->GetSizeOfCmdIndirectCryptoState() * CODECHAL_NUM_IND_CRYPTO_BUFFERS;
            allocParamsForBufferLinear.pBufName = "HcpIndirectCryptoStateBuffer";
            CENC_DECODE_CHK_STATUS(m_osInterface->pfnAllocateResource(
                m_osInterface,
                &allocParamsForBufferLinear,
                &m_resHcpIndCryptoState[i]));

            m_hcpIndCryptoStateSize = allocParamsForBufferLinear.dwBytes;
        }
    }
    if (MEDIA_IS_WA(m_waTable, WaHucStreamoutEnable))
    {
        m_dummyDmemBufSize = MOS_ALIGN_CEIL(m_dmemBufSize, MHW_CACHELINE_SIZE);

        allocParamsForBufferLinear.dwBytes  = m_dummyDmemBufSize;
        allocParamsForBufferLinear.pBufName = "HucDmemBufferDummy";
        CENC_DECODE_CHK_STATUS((MOS_STATUS)m_osInterface->pfnAllocateResource(
            m_osInterface,
            &allocParamsForBufferLinear,
            &m_hucDmemDummy));
        // set lock flag to WRITE_ONLY
        MOS_ZeroMemory(&lockFlags, sizeof(MOS_LOCK_PARAMS));
        lockFlags.WriteOnly = 1;
        data =
            (uint8_t *)m_osInterface->pfnLockResource(m_osInterface, &m_hucDmemDummy, &lockFlags);
        CENC_DECODE_CHK_NULL(data);
        MOS_ZeroMemory(data, m_dummyDmemBufSize);
        m_osInterface->pfnUnlockResource(m_osInterface, &m_hucDmemDummy);

        allocParamsForBufferLinear.dwBytes = MHW_CACHELINE_SIZE;

        allocParamsForBufferLinear.pBufName = "HucDummyStreamInBuffer";
        CENC_DECODE_CHK_STATUS(m_osInterface->pfnAllocateResource(
            m_osInterface,
            &allocParamsForBufferLinear,
            &m_dummyStreamIn));
        allocParamsForBufferLinear.pBufName = "HucDummyStreamOutBuffer";
        CENC_DECODE_CHK_STATUS(m_osInterface->pfnAllocateResource(
            m_osInterface,
            &allocParamsForBufferLinear,
            &m_dummyStreamOut));
        }
    statusBuf->dwReportSize = MOS_ALIGN_CEIL(sizeof(CODECHAL_CENC_STATUS), (2 * sizeof(uint32_t)));

    // Decrypt status reporting
    allocParamsForBufferLinear.dwBytes  = (statusBuf->dwReportSize) * CENC_NUM_DECRYPT_SHARED_BUFFERS + sizeof(uint32_t) * 2;
    allocParamsForBufferLinear.pBufName = "DecryptStatusQueryBuffer";
    CENC_DECODE_CHK_STATUS(m_osInterface->pfnAllocateResource(
        m_osInterface,
        &allocParamsForBufferLinear,
        &statusBuf->resStatusBuffer));

    // lock flag is still WRITE_ONLY
    data = (uint8_t *)m_osInterface->pfnLockResource(m_osInterface, &statusBuf->resStatusBuffer, &lockFlags);

    MOS_ZeroMemory(data, allocParamsForBufferLinear.dwBytes);
    statusBuf->pData                      = nullptr;
    statusBuf->ucGlobalStoreDataLength    = sizeof(uint32_t) * 2;  // 2 uint32_ts in front of CODECHAL_CENC_STATUS * 8
    statusBuf->wCurrIndex                 = 0;
    statusBuf->wFirstIndex                = 0;
    statusBuf->dwSWStoreData              = 1;
    statusBuf->ucErrorStatus2MaskOffset   = CODECHAL_OFFSETOF(CODECHAL_CENC_STATUS, dwErrorStatus2Mask);
    statusBuf->ucErrorStatus2Offset       = CODECHAL_OFFSETOF(CODECHAL_CENC_STATUS, dwErrorStatus2Reg);
    statusBuf->ucKernelHeaderInfoOffset   = CODECHAL_OFFSETOF(CODECHAL_CENC_STATUS, dwKernelHeaderInfoReg);
    statusBuf->ucDecryptErrorStatusOffset = CODECHAL_OFFSETOF(CODECHAL_CENC_STATUS, dwErrorStatusReg);

    m_osInterface->pfnUnlockResource(m_osInterface, &statusBuf->resStatusBuffer);

    return eStatus;
}

MOS_STATUS CodechalCencDecode::SetHucDmemParams()
{
    MOS_LOCK_PARAMS     lockFlags;
    PCODECHAL_CENC_DMEM decryptDmem;
#if (_DEBUG || _RELEASE_INTERNAL)
    MOS_USER_FEATURE_VALUE_DATA userFeatureData;
#endif
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    CENC_DECODE_CHK_NULL(m_osInterface);

    // set lock flag to WRITE_ONLY
    MOS_ZeroMemory(&lockFlags, sizeof(MOS_LOCK_PARAMS));
    lockFlags.WriteOnly = 1;
    decryptDmem        = (PCODECHAL_CENC_DMEM)m_osInterface->pfnLockResource(
        m_osInterface,
        &m_resHucDmem[m_urrDecryptBufIdx],
        &lockFlags);
    CENC_DECODE_CHK_NULL(decryptDmem);

    MOS_ZeroMemory(decryptDmem, sizeof(CODECHAL_CENC_DMEM));

    // set region0 size and report index
    decryptDmem->dwRegion0Size     = m_sliceLevelBatchBufSize;
    decryptDmem->dwRegion1Size     = m_hucSegmentInfoBufSize;
    decryptDmem->dwRegion2Size     = m_hcpIndCryptoStateSize;
    decryptDmem->ui8SizeofLength   = m_sizeOfLength[m_urrDecryptBufIdx];
    decryptDmem->ucStatusReportIdx = m_urrDecryptBufIdx % CODECHAL_NUM_DECRYPT_STATUS_BUFFERS;
    decryptDmem->ui16StatusReportFeedbackNumber =
        m_statusReportNumbers[m_currSharedBufIdx];
    decryptDmem->uiDashMode         = m_dashMode;
    decryptDmem->uiSliceHeaderParse = m_pr30Enabled;
    decryptDmem->StandardInUse = m_standard;
    decryptDmem->ProductFamily      = m_HuCFamily;
    decryptDmem->RevId              = m_HuCRevId;

    decryptDmem->uiFulsimInUse = 0;
#if (_DEBUG || _RELEASE_INTERNAL)
    MOS_ZeroMemory(&userFeatureData, sizeof(userFeatureData));
    MOS_UserFeature_ReadValue_ID(
        nullptr,
        __MEDIA_USER_FEATURE_VALUE_FULSIM_IN_USE_ID,
        &userFeatureData,
        m_osInterface->pOsContext);
    decryptDmem->uiFulsimInUse = (userFeatureData.u32Data) ? true : false;
#endif

    if (m_aesFromHuC)
    {
        decryptDmem->uiAesFromHuC   = true;
        decryptDmem->dwNumOfSegment = m_hucSegmentInfoNumSegments[m_urrDecryptBufIdx];
    }
    else
    {
        decryptDmem->uiAesFromHuC   = false;
        decryptDmem->dwNumOfSegment = 0;
    }

    decryptDmem->uiDummyRefIdxState = 
        MEDIA_IS_WA(m_waTable, WaDummyReference) && !m_osInterface->bSimIsActive;

#ifdef CODECHAL_HUC_KERNEL_DEBUG
    pDecryptDmem->hucKernelDebugLevel = ucHucKernelDebugLevel;
#endif
    CENC_DECODE_VERBOSEMESSAGE("pDecryptDmem->ucStatusReportIdx = 0x%x", decryptDmem->ucStatusReportIdx);
    MOS_TraceDumpExt("HucDmem", 0, decryptDmem, sizeof(*decryptDmem));

    m_osInterface->pfnUnlockResource(
        m_osInterface,
        &m_resHucDmem[m_urrDecryptBufIdx]);

    return eStatus;
}

MOS_STATUS CodechalCencDecode::SetHucDashAesSegmentInfo(
    PMHW_PAVP_AES_IND_PARAMS params,
    PMOS_COMMAND_BUFFER      cmdBuffer)
{
    MOS_STATUS                         eStatus = MOS_STATUS_SUCCESS;
    MOS_LOCK_PARAMS                    lockFlags;
    uint32_t                           i;
    PCODECHAL_HUC_SEGMENT_INFO         segmentInfo;
    PMOS_RESOURCE                      resSegmentInfo;
    MHW_VDBOX_PIPE_MODE_SELECT_PARAMS  pipeModeSelectParams;
    MHW_VDBOX_IND_OBJ_BASE_ADDR_PARAMS indObjParams;
    MHW_VDBOX_HUC_STREAM_OBJ_PARAMS    streamObjParams;

    CENC_DECODE_CHK_NULL(params);
    CENC_DECODE_CHK_NULL(params->pSegmentInfo);

    lockFlags.Value     = 0;
    lockFlags.WriteOnly = 1;

    resSegmentInfo = &m_resHucSegmentInfoBufClear[m_urrDecryptBufIdx];
    segmentInfo =
        (PCODECHAL_HUC_SEGMENT_INFO)m_osInterface->pfnLockResource(m_osInterface, resSegmentInfo, &lockFlags);

    CENC_DECODE_CHK_NULL(segmentInfo);

    for (i = 0; i < params->dwNumSegments; i++)
    {
        MOS_ZeroMemory(&segmentInfo[i], sizeof(CODECHAL_HUC_SEGMENT_INFO));
        segmentInfo[i].dwAesCbcIvOrCtr[3] = params->pSegmentInfo[i].dwAesCbcIvOrCtr[3];
        segmentInfo[i].dwAesCbcIvOrCtr[2] = params->pSegmentInfo[i].dwAesCbcIvOrCtr[2];
        segmentInfo[i].dwAesCbcIvOrCtr[1] = params->pSegmentInfo[i].dwAesCbcIvOrCtr[1];
        segmentInfo[i].dwAesCbcIvOrCtr[0] = params->pSegmentInfo[i].dwAesCbcIvOrCtr[0];

        segmentInfo[i].dwInitByteLength             = params->pSegmentInfo[i].dwInitByteLength;
        segmentInfo[i].dwPartialAesBlockSizeInBytes = (params->pSegmentInfo[i].dwPartialAesBlockSizeInBytes) % 16;
        segmentInfo[i].dwSegmentLength              = params->pSegmentInfo[i].dwSegmentLength;
        segmentInfo[i].dwSegmentStartOffset         = params->pSegmentInfo[i].dwSegmentStartOffset;
    }

    m_osInterface->pfnUnlockResource(m_osInterface, resSegmentInfo);

    pipeModeSelectParams.dwMediaSoftResetCounterValue = 2400;
    pipeModeSelectParams.bStreamOutEnabled            = true;
    pipeModeSelectParams.disableProtectionSetting     = true;

    MOS_ZeroMemory(&indObjParams, sizeof(indObjParams));
    indObjParams.presDataBuffer            = resSegmentInfo;
    indObjParams.dwDataSize                = sizeof(CODECHAL_HUC_SEGMENT_INFO) * params->dwNumSegments;
    indObjParams.presStreamOutObjectBuffer = params->presCryptoBuffer;
    indObjParams.dwStreamOutObjectSize     = sizeof(CODECHAL_HUC_SEGMENT_INFO) * params->dwNumSegments;

    MOS_ZeroMemory(&streamObjParams, sizeof(streamObjParams));
    streamObjParams.dwIndStreamInLength           = sizeof(CODECHAL_HUC_SEGMENT_INFO) * params->dwNumSegments;
    streamObjParams.dwIndStreamInStartAddrOffset  = 0;
    streamObjParams.dwIndStreamOutStartAddrOffset = 0;
    streamObjParams.bStreamOutEnable              = 1;

    CENC_DECODE_CHK_STATUS(m_hucInterface->AddHucPipeModeSelectCmd(cmdBuffer, &pipeModeSelectParams));
    CENC_DECODE_CHK_STATUS(m_hucInterface->AddHucIndObjBaseAddrStateCmd(cmdBuffer, &indObjParams));
    CENC_DECODE_CHK_STATUS(m_hucInterface->AddHucStreamObjectCmd(cmdBuffer, &streamObjParams));

    return eStatus;
}

//!
//! \brief    Update CmdBufIdGlobal
//! \details  Split Mhw_StateHeapInterface_UpdateGlobalCmdBufId into two
//!           functions for decrypt, this one updates the global CmdBufId
//! \param    [in]
//!           Decrypt interface
//! \param    [in] miInterface
//!           Common Mi interface
//! \param    [in] cmdBuffer
//!           The command buffer containing the decode workload
//! \param    [in] pHeapInfo
//!           Heap information for the frame in use
//! \return   MOS_STATUS
//!           MOS_STATUS_SUCCESS if success, else fail reason
//!
MOS_STATUS CodechalCencDecode::UpdateGlobalCmdBufId(
    MhwMiInterface *         miInterface,
    PMOS_COMMAND_BUFFER      cmdBuffer,
    CODECHAL_CENC_HEAP_INFO *heapInfo)
{
    MHW_MI_STORE_DATA_PARAMS miStoreDataParams;
    MOS_STATUS               eStatus = MOS_STATUS_SUCCESS;

    CENC_DECODE_FUNCTION_ENTER;

    CENC_DECODE_CHK_NULL(miInterface);
    CENC_DECODE_CHK_NULL(cmdBuffer);

    MOS_ZeroMemory(&miStoreDataParams, sizeof(miStoreDataParams));
    miStoreDataParams.pOsResource = &m_resTracker;
    miStoreDataParams.dwValue     = heapInfo->u32TrackerId;
    CENC_DECODE_VERBOSEMESSAGE("dwCmdBufId = %d", miStoreDataParams.dwValue);
    CENC_DECODE_CHK_STATUS(miInterface->AddMiStoreDataImmCmd(
        cmdBuffer,
        &miStoreDataParams));

    return eStatus;
}

//!
//! \brief    Update dwCurrCmdBufId
//! \details  Split Mhw_StateHeapInterface_UpdateGlobalCmdBufId into two
//!           functions for decrypt, this one updates the curr CmdBufId
//! \param    PCODECHAL_DECRYPT_STATE
//!           [in] Decrypt interface
//! \return   MOS_STATUS
//!           MOS_STATUS_SUCCESS if success, else fail reason
//!
MOS_STATUS CodechalCencDecode::UpdateCurrCmdBufId()
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    CENC_DECODE_FUNCTION_ENTER;

    ++m_currTrackerId;

    return eStatus;
}

//!
//! \brief    Assign Sync Tag
//! \details  Assign Sync Tag for state heap
//! \param    PCODECHAL_DECRYPT_STATE
//!           [in] Decrypt interface
//! \return   MOS_STATUS
//!           MOS_STATUS_SUCCESS if success, else fail reason
//!
MOS_STATUS CodechalCencDecode::AssignSyncTag()
{
    MOS_STATUS eStatus          = MOS_STATUS_SUCCESS;

    CENC_DECODE_FUNCTION_ENTER;

    m_heapInfo[m_currSharedBufIdx].u32TrackerId = m_currTrackerId;

    return eStatus;
}

//!
//! \brief    Update heap extend size
//! \details  Check if current heap size is sufficient and increase
//!           the initial heap size the heap if it is not enough
//! \param    [in]
//!           Decrypt interface
//! \param    [in,out] heapManager
//!           Heap manager whose size to update
//! \param    [in] requiredSize
//!           Expected workload size for the heap
//! \return   MOS_STATUS
//!           MOS_STATUS_SUCCESS if success, else fail reason
//!
MOS_STATUS CodechalCencDecode::UpdateHeapExtendSize(
    HeapManager *heapManager,
    uint32_t     requiredSize)
{
    uint32_t   numWorkloads;
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    CENC_DECODE_FUNCTION_ENTER;

    CENC_DECODE_CHK_NULL(heapManager);

    requiredSize = MOS_ALIGN_CEIL(requiredSize, MOS_PAGE_SIZE);

    if (heapManager->GetTotalSize() == 0)
    {
        CENC_DECODE_ASSERTMESSAGE("No memory is managed by the heap manager");
        return MOS_STATUS_INVALID_PARAMETER;
    }

    numWorkloads = heapManager->GetTotalSize() / requiredSize;

    if (numWorkloads < CENC_NUM_DECRYPT_SHARED_BUFFERS)
    {
        CENC_DECODE_VERBOSEMESSAGE("dwNumWorkloads = %d", numWorkloads);

        uint32_t u32MinNumWorkload = CENC_NUM_DECRYPT_SHARED_BUFFERS >> 2;
        numWorkloads =
            (numWorkloads < u32MinNumWorkload) ? u32MinNumWorkload : numWorkloads * 2;

        if (numWorkloads > CENC_NUM_DECRYPT_SHARED_BUFFERS)
        {
            numWorkloads = CENC_NUM_DECRYPT_SHARED_BUFFERS;
        }
        uint32_t u32ExtendSize     = numWorkloads * requiredSize;
        uint32_t u32CurrExtendSize = heapManager->GetExtendSize();

        if (u32ExtendSize > u32CurrExtendSize)
        {
            CENC_DECODE_CHK_STATUS(heapManager->SetInitialHeapSize(u32ExtendSize));
        }
    }

    return eStatus;
}

//!
//! \brief    Assign space in state heap
//! \details  Assign space in state heap
//! \param    [in]
//!           Decrypt interface
//! \param    [in] heapInfo
//!           Heap information for the frame in use
//! \param    [in] heapManager
//!           Manager for the heap space is being assigned for
//! \param    [in,out] block
//!           Memory block in a heapInfo representing assigned space
//! \param    [in] size
//!           Required size to assign
//! \return   MOS_STATUS
//!           MOS_STATUS_SUCCESS if success, else fail reason
//!
MOS_STATUS CodechalCencDecode::AssignSpaceInStateHeap(
    CODECHAL_CENC_HEAP_INFO *heapInfo,
    HeapManager *            heapManager,
    MemoryBlock *            block,
    uint32_t                 size)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    CENC_DECODE_FUNCTION_ENTER;

    MemoryBlockManager::AcquireParams acquireParams =
        MemoryBlockManager::AcquireParams(
            heapInfo->u32TrackerId,
            m_blockSizes);

    CENC_DECODE_CHK_NULL(heapManager);
    CENC_DECODE_CHK_NULL(block);

    CENC_DECODE_CHK_NULL(heapInfo);

    if (m_blockSizes.empty())
    {
        m_blockSizes.emplace_back(size);
    }
    else
    {
        m_blockSizes[0] = size;
    }

    CENC_DECODE_VERBOSEMESSAGE("size = %x", size);

    uint32_t u32SpaceNeeded = 0;

    CENC_DECODE_CHK_STATUS(heapManager->AcquireSpace(
        acquireParams,
        m_blocks,
        u32SpaceNeeded));

    if (m_blocks.empty())
    {
        CENC_DECODE_ASSERTMESSAGE("No blocks were acquired");
        return MOS_STATUS_UNKNOWN;
    }
    if (!m_blocks[0].IsValid())
    {
        CENC_DECODE_ASSERTMESSAGE("No blocks were acquired");
        return MOS_STATUS_UNKNOWN;
    }

    *block = m_blocks[0];

    CENC_DECODE_VERBOSEMESSAGE("dwBlockSize = %x, dwOffsetInStateHeap = %x,  dwSyncTagId = %d",
        block->GetSize(),
        block->GetOffset(),
        block->GetTrackerId());

    // since the block is only written to by HW, submit immediately
    CENC_DECODE_CHK_STATUS(heapManager->SubmitBlocks(m_blocks));

    return eStatus;
}

MOS_STATUS CodechalCencDecode::AddHucDummyStreamOut(
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    if (!MEDIA_IS_WA(m_waTable, WaHucStreamoutEnable))
    {
        return eStatus;
    }

    CENC_DECODE_FUNCTION_ENTER;

    CENC_DECODE_CHK_NULL(cmdBuffer);
    CENC_DECODE_CHK_NULL(m_miInterface);

    MHW_MI_FLUSH_DW_PARAMS             flushDwParams;
    MHW_VDBOX_PIPE_MODE_SELECT_PARAMS  pipeModeSelectParams;
    MHW_VDBOX_IND_OBJ_BASE_ADDR_PARAMS indObjParams;
    MHW_VDBOX_HUC_STREAM_OBJ_PARAMS    streamObjParams;

    MOS_ZeroMemory(&flushDwParams, sizeof(flushDwParams));
    CENC_DECODE_CHK_STATUS(m_miInterface->AddMiFlushDwCmd(cmdBuffer, &flushDwParams));

    // pipe mode select
    pipeModeSelectParams.dwMediaSoftResetCounterValue = 2400;

    // pass bit-stream buffer by Ind Obj Addr command. Set size to 1 for dummy stream
    MOS_ZeroMemory(&indObjParams, sizeof(indObjParams));
    indObjParams.presDataBuffer            = &m_dummyStreamIn;
    indObjParams.dwDataSize                = 1;
    indObjParams.presStreamOutObjectBuffer = &m_dummyStreamOut;
    indObjParams.dwStreamOutObjectSize     = 1;

    // set stream object with stream out enabled
    MOS_ZeroMemory(&streamObjParams, sizeof(streamObjParams));
    streamObjParams.dwIndStreamInLength           = 1;
    streamObjParams.dwIndStreamInStartAddrOffset  = 0;
    streamObjParams.bHucProcessing                = true;
    streamObjParams.dwIndStreamOutStartAddrOffset = 0;
    streamObjParams.bStreamOutEnable              = 1;

    CENC_DECODE_CHK_STATUS(m_miInterface->AddMiFlushDwCmd(cmdBuffer, &flushDwParams));

    MHW_VDBOX_HUC_IMEM_STATE_PARAMS   imemParams;
    MHW_VDBOX_HUC_DMEM_STATE_PARAMS   dmemParams;

    MOS_ZeroMemory(&imemParams, sizeof(imemParams));
    imemParams.dwKernelDescriptor = VDBOX_HUC_DECODE_DECRYPT_KERNEL_DESCRIPTOR;

    // set HuC DMEM param
    MOS_ZeroMemory(&dmemParams, sizeof(dmemParams));
    dmemParams.presHucDataSource = &m_hucDmemDummy;
    dmemParams.dwDataLength      = m_dummyDmemBufSize;
    dmemParams.dwDmemOffset      = HUC_DMEM_OFFSET_RTOS_GEMS;

    streamObjParams.bHucProcessing  = true;
    streamObjParams.bStreamInEnable = 1;

    CENC_DECODE_CHK_STATUS(m_hucInterface->AddHucImemStateCmd(cmdBuffer, &imemParams));
    CENC_DECODE_CHK_STATUS(m_hucInterface->AddHucPipeModeSelectCmd(cmdBuffer, &pipeModeSelectParams));
    CENC_DECODE_CHK_STATUS(m_hucInterface->AddHucDmemStateCmd(cmdBuffer, &dmemParams));

    // gen9 huc decrypt workload require region setup, otherwise will have page fault for gen9 huc fw
    // access region at startup
    MHW_VDBOX_HUC_VIRTUAL_ADDR_PARAMS virtualAddrParams;
    MOS_ZeroMemory(&virtualAddrParams, sizeof(virtualAddrParams));
    virtualAddrParams.regionParams[0].presRegion  = &m_dummyStreamIn;
    virtualAddrParams.regionParams[0].isWritable  = true;
    virtualAddrParams.regionParams[15].presRegion = &m_dummyStreamOut;
    virtualAddrParams.regionParams[15].isWritable = false;
    CENC_DECODE_CHK_STATUS(m_hucInterface->AddHucVirtualAddrStateCmd(cmdBuffer, &virtualAddrParams));

    CENC_DECODE_CHK_STATUS(m_hucInterface->AddHucIndObjBaseAddrStateCmd(cmdBuffer, &indObjParams));
    CENC_DECODE_CHK_STATUS(m_hucInterface->AddHucStreamObjectCmd(cmdBuffer, &streamObjParams));
    CENC_DECODE_CHK_STATUS(m_hucInterface->AddHucStartCmd(cmdBuffer, true));

    CENC_DECODE_CHK_STATUS(m_miInterface->AddMiFlushDwCmd(cmdBuffer, &flushDwParams));

    return eStatus;
}

MOS_STATUS CodechalCencDecode::PerformHucStreamOut(
    CencHucStreamoutParams *hucStreamOutParams,
    PMOS_COMMAND_BUFFER     cmdBuffer)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    CENC_DECODE_FUNCTION_ENTER;
    CENC_DECODE_CHK_NULL(cmdBuffer);

    if (MEDIA_IS_SKU(m_skuTable, FtrEnableMediaKernels) && MEDIA_IS_WA(m_waTable, WaHucStreamoutEnable))
    {
        CENC_DECODE_CHK_STATUS(AddHucDummyStreamOut(cmdBuffer));
    }

    // pipe mode select
    MHW_VDBOX_PIPE_MODE_SELECT_PARAMS pipeModeSelectParams;
    pipeModeSelectParams.Mode                         = hucStreamOutParams->mode;
    pipeModeSelectParams.dwMediaSoftResetCounterValue = 2400;
    pipeModeSelectParams.bStreamObjectUsed            = true;
    pipeModeSelectParams.bStreamOutEnabled            = true;
    if (hucStreamOutParams->segmentInfo == nullptr && m_osInterface->osCpInterface->IsCpEnabled())
    {
        // Disable protection control setting in huc drm
        pipeModeSelectParams.disableProtectionSetting = true;
    }

    // Enlarge the stream in/out data size to avoid upper bound hit assert in HuC
    hucStreamOutParams->dataSize += hucStreamOutParams->inputRelativeOffset;
    hucStreamOutParams->streamOutObjectSize += hucStreamOutParams->outputRelativeOffset;

    // pass bit-stream buffer by Ind Obj Addr command
    MHW_VDBOX_IND_OBJ_BASE_ADDR_PARAMS indObjParams;
    MOS_ZeroMemory(&indObjParams, sizeof(indObjParams));
    indObjParams.presDataBuffer            = hucStreamOutParams->dataBuffer;
    indObjParams.dwDataSize                = MOS_ALIGN_CEIL(hucStreamOutParams->dataSize, MHW_PAGE_SIZE);
    indObjParams.dwDataOffset              = hucStreamOutParams->dataOffset;
    indObjParams.presStreamOutObjectBuffer = hucStreamOutParams->streamOutObjectBuffer;
    indObjParams.dwStreamOutObjectSize     = MOS_ALIGN_CEIL(hucStreamOutParams->streamOutObjectSize, MHW_PAGE_SIZE);
    indObjParams.dwStreamOutObjectOffset   = hucStreamOutParams->streamOutObjectOffset;

    // set stream object with stream out enabled
    MHW_VDBOX_HUC_STREAM_OBJ_PARAMS streamObjParams;
    MOS_ZeroMemory(&streamObjParams, sizeof(streamObjParams));
    streamObjParams.dwIndStreamInLength           = hucStreamOutParams->indStreamInLength;
    streamObjParams.dwIndStreamInStartAddrOffset  = hucStreamOutParams->inputRelativeOffset;
    streamObjParams.dwIndStreamOutStartAddrOffset = hucStreamOutParams->outputRelativeOffset;
    streamObjParams.bHucProcessing                = true;
    streamObjParams.bStreamInEnable               = true;
    streamObjParams.bStreamOutEnable              = true;

    CENC_DECODE_CHK_STATUS(m_hucInterface->AddHucPipeModeSelectCmd(cmdBuffer, &pipeModeSelectParams));
    CENC_DECODE_CHK_STATUS(m_hucInterface->AddHucIndObjBaseAddrStateCmd(cmdBuffer, &indObjParams));
    CENC_DECODE_CHK_STATUS(m_hucInterface->AddHucStreamObjectCmd(cmdBuffer, &streamObjParams));

    // This flag is always false if huc fw is not loaded.
    if (MEDIA_IS_SKU(m_skuTable, FtrEnableMediaKernels) &&
        MEDIA_IS_WA(m_waTable, WaHucStreamoutOnlyDisable))
    {
        CENC_DECODE_CHK_STATUS(AddHucDummyStreamOut(cmdBuffer));
    }

    return eStatus;
}

//!
//! \brief    Assign and copy to 2nd level batch buffer
//! \details  Assign the space for 2nd level batch fuffer and
//!           copy the required size for decoder
//! \param    PCODECHAL_DECRYPT_STATE
//!           [in] Decrypt interface
//! \param    char bufIdx
//!           [in] The index for the kernel states
//! \return   MOS_STATUS
//!           MOS_STATUS_SUCCESS if success, else fail reason
MOS_STATUS CodechalCencDecode::AssignAndCopyToSliceLevelBb(
    uint8_t bufIdx)
{
    uint32_t                     sliceLevelBBSizePageAlign;
    MOS_STATUS                   eStatus = MOS_STATUS_SUCCESS;
    MOS_RESOURCE *               dstRes  = nullptr;
    uint8_t *                    srcBuf  = nullptr;
    uint8_t *                    dstBuf  = nullptr;
    uint32_t                     copySize;

    CENC_DECODE_FUNCTION_ENTER;

    CENC_DECODE_CHK_NULL(m_osInterface);

    CODECHAL_CENC_HEAP_INFO &heapInfo = m_heapInfo[bufIdx];
    MOS_RESOURCE &           srcRes   = m_resSlcLevelBatchBuf[bufIdx % CODECHAL_NUM_DECRYPT_BUFFERS].OsResource;

    CENC_DECODE_CHK_NULL(srcRes.pGmmResInfo);

    sliceLevelBBSizePageAlign = MOS_ALIGN_CEIL(m_sliceLevelBBSize, MHW_PAGE_SIZE);
    CENC_DECODE_CHK_STATUS(AssignSpaceInStateHeap(
        &heapInfo,
        m_secondLvlBbHeap,
        &heapInfo.SecondLvlBbBlock,
        sliceLevelBBSizePageAlign));
    dstRes = heapInfo.SecondLvlBbBlock.GetResource();
    CENC_DECODE_CHK_NULL(dstRes);
    CENC_DECODE_CHK_NULL(dstRes->pGmmResInfo);

    CodechalResLock srcLock(m_osInterface, &srcRes);
    srcBuf = (uint8_t *)srcLock.Lock(CodechalResLock::readOnly);
    CENC_DECODE_CHK_NULL(srcBuf);
    CodechalResLock dstLock(m_osInterface, dstRes);
    dstBuf = (uint8_t *)dstLock.Lock(CodechalResLock::writeOnly);
    CENC_DECODE_CHK_NULL(dstBuf);
    dstBuf += heapInfo.SecondLvlBbBlock.GetOffset();

    // cpu copy 2nd level batch buffer and surface tag
    copySize = MOS_ALIGN_CEIL(m_sliceLevelBBSize, MHW_CACHELINE_SIZE);
    MOS_SecureMemcpy(dstBuf, copySize, srcBuf, copySize);
    dstRes->pGmmResInfo->GetSetCpSurfTag(true, srcRes.pGmmResInfo->GetSetCpSurfTag(false, 0));

    return eStatus;
}

MOS_STATUS CodechalCencDecode::Execute(void *params)
{
    MOS_STATUS                   eStatus = MOS_STATUS_SUCCESS;
    PCODECHAL_CENC_PARAMS        decryptionParam;
    MOS_SYNC_PARAMS              syncParams;
    MOS_COMMAND_BUFFER           cmdBuffer;
    PCODECHAL_CENC_STATUS_BUFFER decryptStatusBuffer;
    uint32_t                     statusBufferCurrOffset, streamOutBufferSize;
    uint32_t                     sliceLevelBBSizePageAlign;
#if (_DEBUG || _RELEASE_INTERNAL)
    MOS_USER_FEATURE_VALUE_WRITE_DATA userFeatureWriteData;
#endif  // (_DEBUG || _RELEASE_INTERNAL)

    MHW_GENERIC_PROLOG_PARAMS                  genericPrologParams;
    MHW_MI_FLUSH_DW_PARAMS                     flushDwParams;
    MHW_VDBOX_PIPE_MODE_SELECT_PARAMS          pipeModeSelectParams;
    MHW_VDBOX_HUC_IMEM_STATE_PARAMS            imemParams;
    MHW_VDBOX_HUC_DMEM_STATE_PARAMS            dmemParams;
    MHW_VDBOX_HUC_VIRTUAL_ADDR_PARAMS          virtualAddrParams;
    MHW_VDBOX_IND_OBJ_BASE_ADDR_PARAMS         indObjParams;
    MHW_PAVP_AES_PARAMS                        aesParams;
    MHW_PAVP_AES_IND_PARAMS                    indAesParams;
    MHW_VDBOX_HUC_STREAM_OBJ_PARAMS            streamObjParams;
    MHW_MI_STORE_DATA_PARAMS                   storeDataParams;
    MHW_MI_STORE_REGISTER_MEM_PARAMS           storeRegMemParams;
    MmioRegistersHuc *                         mmioRegisters;
    CODECHAL_CENC_HEAP_INFO *                  heapInfo = nullptr;
    MOS_RESOURCE *                             resHeap   = nullptr;

    CENC_DECODE_FUNCTION_ENTER;
    MOS_TraceEventExt(EVENT_CODEC_CENC, EVENT_TYPE_START, &m_urrDecryptBufIdx, sizeof(uint8_t), &m_currSharedBufIdx, sizeof(uint8_t));

    decryptionParam = (PCODECHAL_CENC_PARAMS)params;

    m_serialMode = decryptionParam->bSerialMode;

    CODECHAL_DEBUG_TOOL(
        CodechalDecodeRestoreData<CODECHAL_FUNCTION> codecFunctionRestore(&m_debugInterface->m_codecFunction);
        m_debugInterface->m_codecFunction = CODECHAL_FUNCTION_CENC_DECODE;)

    CENC_DECODE_CHK_NULL(decryptionParam);

    CENC_DECODE_CHK_NULL(m_osInterface);
    CENC_DECODE_CHK_NULL(m_osInterface->osCpInterface);

    decryptStatusBuffer = &m_decryptStatusBuf;

    CENC_DECODE_CHK_COND((m_vdboxIndex > m_mfxInterface->GetMaxVdboxIndex()), "ERROR - vdbox index exceed the maximum");
    mmioRegisters = m_hucInterface->GetMmioRegisters(m_vdboxIndex);

    CODECHAL_DEBUG_TOOL(
        m_debugInterface->m_bufferDumpFrameNum = dwFrameNum;
        m_debugInterface->m_frameType = 0; // frame type is not known at this time
    )

    CENC_DECODE_NORMALMESSAGE("FrameNum = %d, ucCurrDecryptBufIdx = %d, ucCurrSharedBufIdx = %d",
        dwFrameNum,
        m_urrDecryptBufIdx,
        m_currSharedBufIdx);

#if (_DEBUG || _RELEASE_INTERNAL)
    userFeatureWriteData               = __NULL_USER_FEATURE_VALUE_WRITE_DATA__;
    userFeatureWriteData.Value.i32Data = dwFrameNum;
    userFeatureWriteData.ValueID       = __MEDIA_USER_FEATURE_VALUE_DECRYPT_DECODE_IN_ID;
    MOS_UserFeature_WriteValues_ID(nullptr, &userFeatureWriteData, 1, m_osInterface->pOsContext);
#endif  // (_DEBUG || _RELEASE_INTERNAL)

    m_osInterface->pfnSetPerfTag(
        m_osInterface,
        (uint16_t)(((m_mode << 4) & 0xF0) | ILDB_TYPE));
    m_osInterface->pfnResetPerfBufferID(m_osInterface);

    CENC_DECODE_CHK_STATUS(m_cpInterface->UpdateParams(true));

    m_statusReportNumbers[m_currSharedBufIdx] =
        decryptionParam->ui16StatusReportFeedbackNumber;
    m_savedEncryptionParams[m_currSharedBufIdx] =
        *m_cpInterface->GetEncryptionParams();

    // For native dash support, set correct pavp mode for PipeModeSelectParams. It might change to fairplay mode for 0 0 0 0 case
    if (m_aesFromHuC)
    {
        m_savedEncryptionParams[m_currSharedBufIdx].HostEncryptMode    = CP_TYPE_BASIC;
        m_savedEncryptionParams[m_currSharedBufIdx].PavpEncryptionType = CP_SECURITY_AES128_CTR;
    }

    heapInfo = &m_heapInfo[m_currSharedBufIdx];

    streamOutBufferSize = MOS_ALIGN_CEIL(decryptionParam->dwDataBufferSize, CODECHAL_PAGE_SIZE);
    CENC_DECODE_CHK_STATUS(AssignSyncTag());
    CENC_DECODE_CHK_STATUS(AssignSpaceInStateHeap(
        heapInfo,
        m_bitstreamHeap,
        &heapInfo->BitstreamBlock,
        streamOutBufferSize));

    if (MEDIA_IS_SKU(m_skuTable, FtrCencDecodeRemove2ndLevelBatchBufferCopy))
    {
        // for dg2 small memory bar config, remove m_resSlcLevelBatchBuf which is temp beffer between HuC and Decoder
        // so for HuC directly write 2nd level batch buffer, need to assign space in heap here.  
        // set 2nd level batch buffer size according to number of slices
        m_sliceLevelBBSize = m_standardSlcSizeNeeded * m_numSlices;
        sliceLevelBBSizePageAlign = MOS_ALIGN_CEIL(m_sliceLevelBBSize, MHW_PAGE_SIZE);
        CENC_DECODE_CHK_STATUS(AssignSpaceInStateHeap(
            heapInfo,
            m_secondLvlBbHeap,
            &heapInfo->SecondLvlBbBlock,
            sliceLevelBBSizePageAlign));
        CENC_DECODE_VERBOSEMESSAGE("AssignSpaceInStateHeap for 2ndLevelBatchbuf, sliceLevelBBSizePageAlign = %d, Heap Total Size = %d, Heap Extend Size = %d",
            sliceLevelBBSizePageAlign,
            m_secondLvlBbHeap->GetTotalSize(),
            m_secondLvlBbHeap->GetExtendSize());
        CENC_DECODE_VERBOSEMESSAGE("SecondLvlBbBlock Offset = %d, Size = %d",
            heapInfo->SecondLvlBbBlock.GetOffset(),
            heapInfo->SecondLvlBbBlock.GetSize());
    }

    // if sizeOfLength == 0, assume start code mode
    m_dashMode =
        (m_cpInterface->GetHostEncryptMode() == CENC_TYPE_CTR_LENGTH) &&
        (decryptionParam->ui8SizeOfLength != 0);
    CENC_DECODE_VERBOSEMESSAGE("HostEncryptMode = %d, ui8SizeOfLength = %d", m_cpInterface->GetHostEncryptMode(), decryptionParam->ui8SizeOfLength);

    CENC_DECODE_CHK_STATUS(InitializeSharedBuf(true));

    if (m_dashMode)
    {
        m_sizeOfLength[m_urrDecryptBufIdx] = decryptionParam->ui8SizeOfLength;
    }

    if (m_enableAesNativeSupport)
    {
        CENC_DECODE_CHK_STATUS(UpdateSecureOptMode(m_cpInterface->GetEncryptionParams()));
    }

    // force to disable encryption mode if no segments
    if (!decryptionParam->dwNumSegments)
    {
        m_cpInterface->SetHostEncryptMode(CP_TYPE_NONE);
    }

    // set GPU context
    CENC_DECODE_CHK_STATUS(m_osInterface->pfnSetGpuContext(m_osInterface, m_videoContextForDecrypt));

    // verify command buffer space available
    CENC_DECODE_CHK_STATUS(VerifySpaceAvailable());

    // Get and send command buffer header at the beginning (OS dependent)
    CENC_DECODE_CHK_STATUS(m_osInterface->pfnGetCommandBuffer(m_osInterface, &cmdBuffer, 0));

    // Send command buffer header at the beginning (OS dependent)
    CENC_DECODE_CHK_STATUS(SendPrologWithFrameTracking(&cmdBuffer));

    // insert start status report after MI_FLUSH
    CENC_DECODE_CHK_STATUS(StartStatusReport(&cmdBuffer));

    // Dummy stream WA for HW StreamOut issue
    if (MEDIA_IS_WA(m_waTable, WaHucStreamoutEnable))
    {
        CENC_DECODE_CHK_STATUS(AddHucDummyStreamOut(&cmdBuffer));
    }

    if (m_aesFromHuC)
    {
        MOS_ZeroMemory(&indAesParams, sizeof(indAesParams));
        indAesParams.presCryptoBuffer = &m_resHucSegmentInfoBuf[m_urrDecryptBufIdx];
        indAesParams.dwNumSegments    = decryptionParam->dwNumSegments;
        indAesParams.pSegmentInfo     = (PMHW_SEGMENT_INFO)decryptionParam->pSegmentInfo;

        CENC_DECODE_CHK_STATUS(SetHucDashAesSegmentInfo(
            &indAesParams,
            &cmdBuffer));
        // Dummy stream WA for HW StreamOut issue
        if (MEDIA_IS_WA(m_waTable, WaHucStreamoutEnable))
        {
            CENC_DECODE_CHK_STATUS(AddHucDummyStreamOut(&cmdBuffer));
        }
    }

    if (m_pr30Enabled)
    {
        CENC_DECODE_CHK_STATUS(m_cpInterface->UpdateStatusReportNum(
            m_currSharedBufIdx,
            m_statusReportNumbers[m_currSharedBufIdx],
            &m_resEncryptedStatusReportNum,
            &cmdBuffer));
    }

    // load kernel from WOPCM into L2 storage RAM
    MOS_ZeroMemory(&imemParams, sizeof(imemParams));
    imemParams.dwKernelDescriptor = VDBOX_HUC_DECODE_DECRYPT_KERNEL_DESCRIPTOR;
    CENC_DECODE_CHK_STATUS(m_hucInterface->AddHucImemStateCmd(&cmdBuffer, &imemParams));
    CENC_DECODE_VERBOSEMESSAGE("After IMEM command.");

    // pipe mode select
    pipeModeSelectParams.bStreamObjectUsed = true;
    pipeModeSelectParams.Mode = m_mode;
    pipeModeSelectParams.dwMediaSoftResetCounterValue = 2400;
    CENC_DECODE_CHK_STATUS(m_hucInterface->AddHucPipeModeSelectCmd(&cmdBuffer, &pipeModeSelectParams));

    // set HuC DMEM param
    if (m_aesFromHuC)
    {
        m_hucSegmentInfoNumSegments[m_urrDecryptBufIdx] = decryptionParam->dwNumSegments;
    }
    CENC_DECODE_CHK_STATUS(SetHucDmemParams());
    MOS_ZeroMemory(&dmemParams, sizeof(dmemParams));
    dmemParams.presHucDataSource = &m_resHucDmem[m_urrDecryptBufIdx];
    dmemParams.dwDataLength      = MOS_ALIGN_CEIL(m_dmemBufSize, MHW_CACHELINE_SIZE);
    dmemParams.dwDmemOffset      = HUC_DMEM_OFFSET_RTOS_GEMS;
    CENC_DECODE_CHK_STATUS(m_hucInterface->AddHucDmemStateCmd(&cmdBuffer, &dmemParams));

    // pass 2nd level BB and shared buffer by Virtual Addr command
    MOS_ZeroMemory(&virtualAddrParams, sizeof(virtualAddrParams));
    if (MEDIA_IS_SKU(m_skuTable, FtrCencDecodeRemove2ndLevelBatchBufferCopy))
    {
        // for dg2+, HuC directly write 2nd level batch buffer, need to assign heap resource and offset here.  
        virtualAddrParams.regionParams[0].presRegion = heapInfo->SecondLvlBbBlock.GetResource();
        virtualAddrParams.regionParams[0].dwOffset   = heapInfo->SecondLvlBbBlock.GetOffset();
    }
    else
    {
        // for not dg2+, HuC write 2nd level batch buffer to m_resSlcLevelBatchBuf as temp
        virtualAddrParams.regionParams[0].presRegion = &m_resSlcLevelBatchBuf[m_urrDecryptBufIdx].OsResource;
        virtualAddrParams.regionParams[0].dwOffset   = 0;
    }
    virtualAddrParams.regionParams[0].isWritable = true;
    if (m_aesFromHuC)
    {
        virtualAddrParams.regionParams[1].presRegion = &m_resHucSegmentInfoBuf[m_urrDecryptBufIdx];
        virtualAddrParams.regionParams[1].isWritable = true;

        virtualAddrParams.regionParams[2].presRegion = &m_resHcpIndCryptoState[m_currSharedBufIdx];
        virtualAddrParams.regionParams[2].isWritable = true;
    }
    virtualAddrParams.regionParams[15].presRegion = &m_resHucSharedBuf;
    virtualAddrParams.regionParams[15].isWritable = false;

    CENC_DECODE_CHK_STATUS(m_hucInterface->AddHucVirtualAddrStateCmd(&cmdBuffer, &virtualAddrParams));
    CENC_DECODE_VERBOSEMESSAGE("After Virtual addr command.");

    // pass bit-stream buffer by Ind Obj Addr command
    MOS_ZeroMemory(&indObjParams, sizeof(indObjParams));
    indObjParams.presDataBuffer = decryptionParam->presDataBuffer;

    if (decryptionParam->dwDataBufferSize < 4)
    {
        decryptionParam->dwDataBufferSize = 4;
    }

    indObjParams.dwDataSize            = decryptionParam->dwDataBufferSize;
    heapInfo->resOriginalBitstream     = *decryptionParam->presDataBuffer;
    heapInfo->u32OriginalBitstreamSize = decryptionParam->dwDataBufferSize;

    CENC_DECODE_CHK_NULL(resHeap = heapInfo->BitstreamBlock.GetResource());
    indObjParams.presStreamOutObjectBuffer = resHeap;
    indObjParams.dwStreamOutObjectSize     = heapInfo->BitstreamBlock.GetSize();
    indObjParams.dwStreamOutObjectOffset   = heapInfo->BitstreamBlock.GetOffset();

    CENC_DECODE_CHK_STATUS(m_hucInterface->AddHucIndObjBaseAddrStateCmd(&cmdBuffer, &indObjParams));
    CENC_DECODE_VERBOSEMESSAGE("Ind obj from MSDK, size = %d.", indObjParams.dwDataSize);

    if (decryptionParam->dwNumSegments)
    {
        CENC_DECODE_ASSERT(decryptionParam->dwNumSegments < m_hucIndCryptoStateNumSegments);
        CENC_DECODE_VERBOSEMESSAGE("dwNumSegments %d", decryptionParam->dwNumSegments);

        MOS_ZeroMemory(&indAesParams, sizeof(indAesParams));
        indAesParams.presCryptoBuffer = &m_resHucIndCryptoState[m_urrDecryptBufIdx];
        indAesParams.dwNumSegments    = decryptionParam->dwNumSegments;
        indAesParams.pSegmentInfo     = (PMHW_SEGMENT_INFO)decryptionParam->pSegmentInfo;
        CENC_DECODE_CHK_STATUS(m_cpInterface->SetHucIndirectCryptoState(&indAesParams));

        MOS_ZeroMemory(&aesParams, sizeof(aesParams));
        aesParams.presDataBuffer = &m_resHucIndCryptoState[m_urrDecryptBufIdx];
        aesParams.dwSliceIndex   = decryptionParam->dwNumSegments;
        CENC_DECODE_CHK_STATUS(m_cpInterface->AddHucAesIndState(&cmdBuffer, &aesParams));
    }

    // WVN: byte address of bit-stream buffer under 4K
    MOS_ZeroMemory(&streamObjParams, sizeof(streamObjParams));
    streamObjParams.dwIndStreamInLength           = decryptionParam->dwDataBufferSize;
    streamObjParams.dwIndStreamInStartAddrOffset  = 0;
    streamObjParams.bHucProcessing                = true;
    streamObjParams.dwIndStreamOutStartAddrOffset = 0;
    streamObjParams.bStreamInEnable               = 1;
    //For Native Dash support, no stream out is needed
    if (m_noStreamOutOpt)
    {
        streamObjParams.bStreamOutEnable = 0;
    }
    else
    {
        streamObjParams.bStreamOutEnable = 1;
    }
    streamObjParams.bEmulPreventionByteRemoval = 1;
    if (m_lengthModeDisabledForDash)
    {
        streamObjParams.bEmulPreventionByteRemoval = 0;
        streamObjParams.bLengthModeEnabled         = 0;
    }
    else if (m_dashMode)
    {
        streamObjParams.bLengthModeEnabled = 1;
    }
    else  // Widevine Classic
    {
        streamObjParams.bStartCodeSearchEngine = 1;
        streamObjParams.ucStartCodeByte0       = 0;
        streamObjParams.ucStartCodeByte1       = 0;
        streamObjParams.ucStartCodeByte2       = 1;
    }
    CENC_DECODE_CHK_STATUS(m_hucInterface->AddHucStreamObjectCmd(&cmdBuffer, &streamObjParams));

    statusBufferCurrOffset =
        decryptStatusBuffer->ucGlobalStoreDataLength +
        (decryptStatusBuffer->wCurrIndex * decryptStatusBuffer->dwReportSize);

    // Write HUC_STATUS2 header info mask
    MOS_ZeroMemory(&storeDataParams, sizeof(storeDataParams));
    storeDataParams.pOsResource      = &decryptStatusBuffer->resStatusBuffer;
    storeDataParams.dwResourceOffset = statusBufferCurrOffset + decryptStatusBuffer->ucErrorStatus2MaskOffset;
    storeDataParams.dwValue          = m_hucInterface->GetHucStatus2ImemLoadedMask();
    CENC_DECODE_CHK_STATUS(m_miInterface->AddMiStoreDataImmCmd(&cmdBuffer, &storeDataParams));

    // store HUC_STATUS2 register
    MOS_ZeroMemory(&storeRegMemParams, sizeof(storeRegMemParams));
    storeRegMemParams.presStoreBuffer = &decryptStatusBuffer->resStatusBuffer;
    storeRegMemParams.dwOffset        = statusBufferCurrOffset + decryptStatusBuffer->ucErrorStatus2Offset;
    storeRegMemParams.dwRegister      = mmioRegisters->hucStatus2RegOffset;
    CENC_DECODE_CHK_STATUS(m_miInterface->AddMiStoreRegisterMemCmd(&cmdBuffer, &storeRegMemParams));

    // store HUC_uKERNEL_HDR_INFO register
    MOS_ZeroMemory(&storeRegMemParams, sizeof(storeRegMemParams));
    storeRegMemParams.presStoreBuffer = &decryptStatusBuffer->resStatusBuffer;
    storeRegMemParams.dwOffset        = statusBufferCurrOffset + decryptStatusBuffer->ucKernelHeaderInfoOffset;
    storeRegMemParams.dwRegister      = mmioRegisters->hucUKernelHdrInfoRegOffset;
    CENC_DECODE_CHK_STATUS(m_miInterface->AddMiStoreRegisterMemCmd(&cmdBuffer, &storeRegMemParams));

    // for now always set LastStreamObj = true since Huc is accepting entire frame
    CENC_DECODE_CHK_STATUS(m_hucInterface->AddHucStartCmd(&cmdBuffer, true));

    MOS_ZeroMemory(&flushDwParams, sizeof(flushDwParams));
    flushDwParams.bVideoPipelineCacheInvalidate = false;
    CENC_DECODE_CHK_STATUS(m_miInterface->AddMiFlushDwCmd(&cmdBuffer, &flushDwParams));
    CENC_DECODE_CHK_STATUS(m_miInterface->AddMiFlushDwCmd(&cmdBuffer, &flushDwParams));

    // insert end status report before batch buffer end
    CENC_DECODE_CHK_STATUS(EndStatusReport(&cmdBuffer));

    CENC_DECODE_CHK_STATUS(m_miInterface->AddMiBatchBufferEnd(&cmdBuffer, nullptr));

    CODECHAL_DEBUG_TOOL(
        CENC_DECODE_CHK_STATUS(m_debugInterface->DumpCmdBuffer(
            &cmdBuffer,
            CODECHAL_NUM_MEDIA_STATES,
            "_DEC"));
        CENC_DECODE_CHK_STATUS(m_debugInterface->DumpHucRegion(
            &m_resHucSharedBuf,
            0,
            m_sharedBufSize,
            15,
            "",
            true,
            0,
            CodechalHucRegionDumpType::hucRegionDumpRegionLocked));

        CENC_DECODE_CHK_STATUS(m_debugInterface->DumpHucRegion(
            decryptionParam->presDataBuffer,
            0,
            decryptionParam->dwDataBufferSize,
            3,
            "",
            true,
            0,
            CodechalHucRegionDumpType::hucRegionDumpDefault));

        if (m_aesFromHuC) {
            CENC_DECODE_CHK_STATUS(m_debugInterface->DumpHucRegion(
                &m_resHucSegmentInfoBufClear[m_urrDecryptBufIdx],
                0,
                m_hucSegmentInfoBufSize,
                1,
                "",
                true,
                0,
                CodechalHucRegionDumpType::hucRegionDumpDefault));
        }

        CENC_DECODE_CHK_STATUS(m_debugInterface->DumpHucDmem(
            &m_resHucDmem[m_urrDecryptBufIdx],
            m_dmemBufSize,
            0,
            CodechalHucRegionDumpType::hucRegionDumpDefault));

        CENC_DECODE_VERBOSEMESSAGE("after DMEM Dump.");
        if (m_osInterface->osCpInterface->IsCpEnabled()) {
            CENC_DECODE_CHK_STATUS(m_debugInterface->DumpHucRegion(
                &m_resHucIndCryptoState[m_urrDecryptBufIdx],
                0,
                m_hucIndCryptoStateNumSegments * m_cpInterface->GetSizeOfCmdIndirectCryptoState(),
                4,
                "",
                true,
                0,
                CodechalHucRegionDumpType::hucRegionDumpDefault));
        })

    m_osInterface->pfnReturnCommandBuffer(m_osInterface, &cmdBuffer, 0);

    if (MOS_VE_SUPPORTED(m_osInterface) && cmdBuffer.Attributes.pAttriVe)
    {
        auto pAttriVe = (PMOS_CMD_BUF_ATTRI_VE)(cmdBuffer.Attributes.pAttriVe);
        pAttriVe->bUseVirtualEngineHint = false;
    }

    // submit Huc command buffer
    CENC_DECODE_CHK_STATUS(m_osInterface->pfnSubmitCommandBuffer(
        m_osInterface,
        &cmdBuffer,
        m_videoContextUsesNullHw));
    CENC_DECODE_VERBOSEMESSAGE("after Submit Command Buffer.");

    // reset status report after submitting HuC command buffer
    CENC_DECODE_CHK_STATUS(ResetStatusReport(m_videoContextUsesNullHw));
    CENC_DECODE_VERBOSEMESSAGE("after ResetStatusReport.");

    // increment the buffer index
    m_urrDecryptBufIdx = (m_urrDecryptBufIdx + 1) % CODECHAL_NUM_DECRYPT_BUFFERS;
    m_currSharedBufIdx  = (m_currSharedBufIdx + 1) % CENC_NUM_DECRYPT_SHARED_BUFFERS;
    dwFrameNum++;
    CENC_DECODE_CHK_STATUS(UpdateCurrCmdBufId());
    //After huc decrypt set HostEncryptMode to BASIC to avoid streamout.
    m_cpInterface->SetHostEncryptMode(CP_TYPE_BASIC);

#if (_DEBUG || _RELEASE_INTERNAL)
    userFeatureWriteData               = __NULL_USER_FEATURE_VALUE_WRITE_DATA__;
    userFeatureWriteData.Value.i32Data = dwFrameNum;
    userFeatureWriteData.ValueID       = __MEDIA_USER_FEATURE_VALUE_DECRYPT_DECODE_OUT_ID;
    MOS_UserFeature_WriteValues_ID(nullptr, &userFeatureWriteData, 1, m_osInterface->pOsContext);
#endif  // (_DEBUG || _RELEASE_INTERNAL)
    MOS_TraceEventExt(EVENT_CODEC_CENC, EVENT_TYPE_END, &decryptionParam->dwNumSegments, sizeof(uint32_t), decryptionParam->pSegmentInfo, sizeof(MHW_SEGMENT_INFO) * decryptionParam->dwNumSegments);

    return eStatus;
}

MOS_STATUS CodechalCencDecode::AllocateDecodeResource(
    uint8_t bufIdx)
{
    MOS_STATUS              eStatus = MOS_STATUS_SUCCESS;
    MOS_ALLOC_GFXRES_PARAMS allocParamsForBuffer2D;

    CENC_DECODE_FUNCTION_ENTER;

    CENC_DECODE_CHK_NULL(m_osInterface);

    if (m_bitstreamSize > m_bitstreamSizeMax)
    {
        CENC_DECODE_VERBOSEMESSAGE("dwBitstreamSize = %x, dwBitstreamSizeMax = %x",
            m_bitstreamSize,
            m_bitstreamSizeMax);
        m_bitstreamSizeMax = m_bitstreamSize;
        CENC_DECODE_CHK_STATUS(UpdateHeapExtendSize(
            m_bitstreamHeap,
            m_bitstreamSize));
    }

    if (m_sliceLevelBBSize > m_sliceLevelBBSizeMax)
    {
        CENC_DECODE_VERBOSEMESSAGE("dwSliceLevelBBSize = %x, dwSliceLevelBBSizeMax = %x",
            m_sliceLevelBBSize,
            m_sliceLevelBBSizeMax);
        m_sliceLevelBBSizeMax = m_sliceLevelBBSize;
        CENC_DECODE_CHK_STATUS(UpdateHeapExtendSize(
            m_secondLvlBbHeap,
            m_sliceLevelBBSize));
    }

    if (!MEDIA_IS_SKU(m_skuTable, FtrCencDecodeRemove2ndLevelBatchBufferCopy))
    {
        // for not dg2+, HuC write 2nd level batch buffer to m_resSlcLevelBatchBuf as temp
        // need to copy to heap here
        eStatus = AssignAndCopyToSliceLevelBb(bufIdx);
    }
    return eStatus;
}

MOS_STATUS CodechalCencDecode::StartStatusReport(
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    MOS_STATUS                   eStatus = MOS_STATUS_SUCCESS;
    PCODECHAL_CENC_STATUS_BUFFER decryptStatusBuf;
    MHW_MI_STORE_DATA_PARAMS     storeDataParams;
    uint32_t                     offset;

    CENC_DECODE_FUNCTION_ENTER;

    decryptStatusBuf = &m_decryptStatusBuf;

    offset =
        decryptStatusBuf->ucGlobalStoreDataLength + decryptStatusBuf->wCurrIndex * decryptStatusBuf->dwReportSize;
    storeDataParams.pOsResource      = &decryptStatusBuf->resStatusBuffer;
    storeDataParams.dwResourceOffset = offset;
    storeDataParams.dwValue          = CODECHAL_STATUS_QUERY_START_FLAG;

    CENC_DECODE_CHK_STATUS(m_miInterface->AddMiStoreDataImmCmd(cmdBuffer, &storeDataParams));
    CENC_DECODE_VERBOSEMESSAGE("store START_FLAG.");

    return eStatus;
}

MOS_STATUS CodechalCencDecode::EndStatusReport(
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    MOS_STATUS                       eStatus = MOS_STATUS_SUCCESS;
    PCODECHAL_CENC_STATUS_BUFFER     decryptStatusBuf;
    PCODECHAL_CENC_STATUS            decryptStatus;
    MHW_MI_STORE_DATA_PARAMS         storeDataParams;
    MHW_MI_STORE_REGISTER_MEM_PARAMS storeRegMemParams;
    MmioRegistersHuc *               mmioRegisters;
    uint32_t                         offset;

    CENC_DECODE_FUNCTION_ENTER;

    CENC_DECODE_CHK_COND((m_vdboxIndex > m_mfxInterface->GetMaxVdboxIndex()), "ERROR - vdbox index exceed the maximum");
    mmioRegisters = m_hucInterface->GetMmioRegisters(m_vdboxIndex);

    decryptStatusBuf = (PCODECHAL_CENC_STATUS_BUFFER)&m_decryptStatusBuf;

    CodechalResLock statusBuffer(m_osInterface, &m_decryptStatusBuf.resStatusBuffer);
    decryptStatusBuf->pData = (uint8_t *)statusBuffer.Lock(CodechalResLock::writeOnly);

    offset       = decryptStatusBuf->ucGlobalStoreDataLength + decryptStatusBuf->wCurrIndex * decryptStatusBuf->dwReportSize;
    CENC_DECODE_CHK_NULL(decryptStatusBuf->pData);
    decryptStatus = (PCODECHAL_CENC_STATUS)(decryptStatusBuf->pData + offset);

    // write CB end flag
    storeDataParams.pOsResource      = &decryptStatusBuf->resStatusBuffer;
    storeDataParams.dwResourceOffset = offset;
    storeDataParams.dwValue          = CODECHAL_STATUS_QUERY_END_FLAG;
    CENC_DECODE_CHK_STATUS(m_miInterface->AddMiStoreDataImmCmd(cmdBuffer, &storeDataParams));

    // store HuC status register
    MOS_ZeroMemory(&storeRegMemParams, sizeof(storeRegMemParams));
    storeRegMemParams.presStoreBuffer = &decryptStatusBuf->resStatusBuffer;
    storeRegMemParams.dwOffset        = offset + decryptStatusBuf->ucDecryptErrorStatusOffset;
    storeRegMemParams.dwRegister      = mmioRegisters->hucStatusRegOffset;
    CENC_DECODE_CHK_STATUS(m_miInterface->AddMiStoreRegisterMemCmd(cmdBuffer, &storeRegMemParams));

    // record current CB's number/position in all the CBs submitted to HW
    decryptStatus->dwSWStoredData = decryptStatusBuf->dwSWStoreData;
    decryptStatus->dwFrameNum     = dwFrameNum;
    decryptStatusBuf->wCurrIndex  = (decryptStatusBuf->wCurrIndex + 1) % CENC_NUM_DECRYPT_SHARED_BUFFERS;

    // zero the next report buffer to be written
    offset       = decryptStatusBuf->ucGlobalStoreDataLength + decryptStatusBuf->wCurrIndex * decryptStatusBuf->dwReportSize;
    decryptStatus = (PCODECHAL_CENC_STATUS)(decryptStatusBuf->pData + offset);
    MOS_ZeroMemory((uint8_t *)decryptStatus, sizeof(CODECHAL_CENC_STATUS));
    CENC_DECODE_VERBOSEMESSAGE("store END_FLAG, dwSWStoreData = %d, curr index = %d.",
        decryptStatusBuf->dwSWStoreData,
        decryptStatusBuf->wCurrIndex);

    return eStatus;
}

MOS_STATUS CodechalCencDecode::ResetStatusReport(
    bool nullHwInUse)
{
    MOS_STATUS                   eStatus = MOS_STATUS_SUCCESS;
    PCODECHAL_CENC_STATUS_BUFFER decryptStatusBuf;
    MOS_COMMAND_BUFFER           cmdBuffer;
    MHW_MI_FLUSH_DW_PARAMS       flushDwParams;
    MHW_GENERIC_PROLOG_PARAMS    genericPrologParams;

    CENC_DECODE_FUNCTION_ENTER;
    CENC_DECODE_CHK_NULL(m_osInterface);

    decryptStatusBuf = (PCODECHAL_CENC_STATUS_BUFFER)&m_decryptStatusBuf;

    if (!m_osInterface->bEnableKmdMediaFrameTracking)
    {
        CENC_DECODE_CHK_STATUS(m_osInterface->pfnSetGpuContext(m_osInterface, m_videoContextForDecrypt));
        CENC_DECODE_CHK_STATUS(m_osInterface->pfnGetCommandBuffer(m_osInterface, &cmdBuffer, 0));

        // Send command buffer header at the beginning (OS dependent)
        MOS_ZeroMemory(&genericPrologParams, sizeof(genericPrologParams));
        genericPrologParams.pOsInterface     = m_osInterface;
        genericPrologParams.pvMiInterface    = m_miInterface;
        genericPrologParams.bMmcEnabled      = m_mmcEnabled;
        genericPrologParams.presStoreData    = &decryptStatusBuf->resStatusBuffer;
        genericPrologParams.dwStoreDataValue = decryptStatusBuf->dwSWStoreData;
        CENC_DECODE_CHK_STATUS(Mhw_SendGenericPrologCmd(&cmdBuffer, &genericPrologParams));

        CENC_DECODE_CHK_STATUS(m_miInterface->AddMiBatchBufferEnd(&cmdBuffer, nullptr));

        m_osInterface->pfnReturnCommandBuffer(m_osInterface, &cmdBuffer, 0);

        CENC_DECODE_CHK_STATUS(m_osInterface->pfnSubmitCommandBuffer(m_osInterface, &cmdBuffer, nullHwInUse));
    }

    CENC_DECODE_VERBOSEMESSAGE("store dwSWStoreData = %d.", decryptStatusBuf->dwSWStoreData);

    // total number of CBs submitted to HW
    decryptStatusBuf->dwSWStoreData++;
    CENC_DECODE_VERBOSEMESSAGE("now dwSWStoreData = %d.", decryptStatusBuf->dwSWStoreData);

    return eStatus;
}

MOS_STATUS CodechalCencDecode::GetStatusReport(
    void *   status,
    uint16_t numStatus, 
    HUC_STATUS *hucStatus)
{
    MOS_STATUS                          eStatus = MOS_STATUS_SUCCESS;
    PCODECHAL_CENC_STATUS               decryptStatus;
    PCODECHAL_CENC_STATUS_BUFFER        decryptStatusBuf;
    uint32_t                            numReportsAvailable, globalHWStoredData;
    uint16_t                            reportsGenerated = 0;
    uint32_t                            globalCount, localCount, offset;
    uint8_t*                            statusBuf;
    uint16_t                            i, j;
    bool                                isRetry;

    CENC_DECODE_FUNCTION_ENTER;

    CODECHAL_DEBUG_TOOL(
        CodechalDecodeRestoreData<CODECHAL_FUNCTION> codecFunctionRestore(&m_debugInterface->m_codecFunction);
        m_debugInterface->m_codecFunction = CODECHAL_FUNCTION_CENC_DECODE;)

    CENC_DECODE_CHK_NULL(m_osInterface);

    decryptStatusBuf = &m_decryptStatusBuf;
    numReportsAvailable =
        ((decryptStatusBuf->wCurrIndex - decryptStatusBuf->wFirstIndex) &
            (CENC_NUM_DECRYPT_SHARED_BUFFERS - 1));

    CENC_DECODE_VERBOSEMESSAGE("wNumStatus = %d, statusReportSize = 0x%x", numStatus, m_standardReportSizeApp);
    CENC_DECODE_VERBOSEMESSAGE("dwNumReportsAvailable = %d.", numReportsAvailable);

    // number of CBs driver submitted to HW since last time query status
    if (numReportsAvailable == 0)
    {
        CENC_DECODE_ASSERTMESSAGE("No reports available, wCurrIndex = %d, wFirstIndex = %d", decryptStatusBuf->wCurrIndex, decryptStatusBuf->wFirstIndex);
        return eStatus;
    }

    statusBuf = (uint8_t *)status;
    if (m_videoContextUsesNullHw)
    {
        for (i = 0; i < numReportsAvailable; i++)
        {
            *(uint32_t *)statusBuf = CODECHAL_STATUS_SUCCESSFUL;
            statusBuf += m_standardReportSizeApp;
            reportsGenerated++;
        }

        decryptStatusBuf->wFirstIndex =
            (decryptStatusBuf->wFirstIndex + reportsGenerated) % CENC_NUM_DECRYPT_SHARED_BUFFERS;

        return eStatus;
    }

    if (numReportsAvailable < numStatus)
    {
        statusBuf += (m_standardReportSizeApp * numReportsAvailable);
        for (i = (uint16_t)numReportsAvailable; i < numStatus; i++)
        {
            // These buffers are not yet received by driver. Just don't report anything back.
            *(uint32_t *)statusBuf = CODECHAL_STATUS_INCOMPLETE;
            statusBuf += m_standardReportSizeApp;
        }
        numStatus = (uint16_t)numReportsAvailable;
    }

    // Reset m_incompleteReportNum
    m_incompleteReportNum = 0;

    static std::chrono::duration<int> const MAX_DELAY_HUC_STATUS(1); // 1 second 
    auto startTime = std::chrono::high_resolution_clock::now();

    do
    {
        CodechalResLock statusBuffer(m_osInterface, &m_decryptStatusBuf.resStatusBuffer);
        decryptStatusBuf->pData = (uint8_t*)statusBuffer.Lock(CodechalResLock::readOnly);

        // total number of CBs that finished execution on HW
        CENC_DECODE_CHK_NULL(decryptStatusBuf->pData);
        globalHWStoredData = *((uint32_t *)decryptStatusBuf->pData);
        CENC_DECODE_VERBOSEMESSAGE("HWStoreData = %d.", globalHWStoredData);
        // number of CBs that are currently queued (submitted, but not yet executed)
        globalCount = decryptStatusBuf->dwSWStoreData - globalHWStoredData;
        CENC_DECODE_VERBOSEMESSAGE("dwGlobalCount = %d.", globalCount);

        // Reset reportsGenerated & isRetry
        reportsGenerated = 0;
        isRetry = false;

        if (m_incompleteReportNum >= CODECHAL_INCOMPLETE_REPORTS_TH2)
        {
            offset          = decryptStatusBuf->ucGlobalStoreDataLength + decryptStatusBuf->wFirstIndex * decryptStatusBuf->dwReportSize;
            decryptStatus   = (PCODECHAL_CENC_STATUS)(decryptStatusBuf->pData + offset);

            *((uint32_t *)decryptStatusBuf->pData) = decryptStatus->dwSWStoredData;

            CENC_DECODE_ASSERTMESSAGE("Exceeds the max incomplete reports, manually store data = %d.", decryptStatus->dwSWStoredData);
        }

        // Report status in reverse temporal order
        for (j = 0; j < numStatus; j++)
        {
            i = (decryptStatusBuf->wCurrIndex - j - 1) & (CENC_NUM_DECRYPT_SHARED_BUFFERS - 1);

            statusBuf       = (uint8_t *)((uint8_t *)status + (numStatus - j - 1) * m_standardReportSizeApp);
            offset          = decryptStatusBuf->ucGlobalStoreDataLength + i * decryptStatusBuf->dwReportSize;
            decryptStatus   = (PCODECHAL_CENC_STATUS)(decryptStatusBuf->pData + offset);

            localCount = decryptStatus->dwSWStoredData - globalHWStoredData;

            if (hucStatus != nullptr)
            {
                hucStatus->Value = decryptStatus->dwErrorStatusReg;
            }
            CENC_DECODE_VERBOSEMESSAGE("check buffer %d, dwLocalCount = %d.", i, localCount);

            if (localCount == 0 || localCount > globalCount)
            {
                // Check if HW execution is complete.
                if (decryptStatus->dwHWStoredData == CODECHAL_STATUS_QUERY_END_FLAG)
                {
                    // Check to see if IMEM load succeed and any decoding error occurs
                    if (!(decryptStatus->dwErrorStatus2Reg & decryptStatus->dwErrorStatus2Mask) ||
                         (decryptStatus->dwErrorStatusReg & m_hucInterface->GetHucErrorFlagsMask()))
                    {
                        CENC_DECODE_VERBOSEMESSAGE("HuC status indicates error");
                        *(uint32_t *)statusBuf = CODECHAL_STATUS_ERROR;
                    }
                    else
                    {
                        // HuC execution finished w/o error
                        *(uint32_t *)statusBuf = CODECHAL_STATUS_SUCCESSFUL;

                        eStatus = ParseSharedBufParams(
                                    i,
                                    j,
                                    statusBuf);
                        if (eStatus != MOS_STATUS_SUCCESS)
                        {
                            // If there is no space available in 2nd level BB state heap
                            // attempt to wait and retry :: HSD 10017935
                            if (eStatus == MOS_STATUS_CLIENT_AR_NO_SPACE)
                            {
                                *(uint32_t *)statusBuf = CODECHAL_STATUS_INCOMPLETE;
                                if (j == (numStatus - 1))
                                {
                                    isRetry = true;
                                }
                                else
                                {
                                    isRetry = false;
                                }
                                continue;
                            }
                            else
                            {
                                return eStatus;
                            }
                        }
                    }
                }
                else
                {
                    CENC_DECODE_VERBOSEMESSAGE("Media reset may have occured, pDecryptStatus->dwHwStoredData = 0x%x", decryptStatus->dwHWStoredData);
                    // BB_END data not written. Media reset might have occurred.
                    *(uint32_t *)statusBuf = CODECHAL_STATUS_ERROR;
                }

                // log u32 status + u16 status report num into trace
                MOS_TraceEventExt(EVENT_CODEC_CENC, EVENT_TYPE_INFO, statusBuf, 6, decryptStatus, sizeof(*decryptStatus));

                CENC_DECODE_NORMALMESSAGE("FrameNum = %d", decryptStatus->dwFrameNum);
                CENC_DECODE_VERBOSEMESSAGE("HUC_STATUS register = 0x%x", decryptStatus->dwErrorStatusReg);
                CENC_DECODE_VERBOSEMESSAGE("HUC_STATUS2 register = 0x%x", decryptStatus->dwErrorStatus2Reg);
                CENC_DECODE_VERBOSEMESSAGE("HUC_uKERNEL_HDR_INFO register = 0x%x", decryptStatus->dwKernelHeaderInfoReg);

                CODECHAL_DEBUG_TOOL(
                    m_debugInterface->m_bufferDumpFrameNum = decryptStatus->dwFrameNum;
                m_debugInterface->m_frameType = 0; // frame type is not known at this time

                if (MEDIA_IS_SKU(m_skuTable, FtrCencDecodeRemove2ndLevelBatchBufferCopy))
                {
                    // for dg2+, need dump slice level batch buffer from heap
                    CODECHAL_CENC_HEAP_INFO * heapInfo = &m_heapInfo[i];
                    CENC_DECODE_CHK_STATUS(m_debugInterface->DumpHucRegion(
                        heapInfo->SecondLvlBbBlock.GetResource(),
                        heapInfo->SecondLvlBbBlock.GetOffset(),
                        heapInfo->SecondLvlBbBlock.GetSize(),
                        0,
                        "",
                        false,
                        0,
                        CodechalHucRegionDumpType::hucRegionDumpDefault));
                }
                else
                {
                    // for not dg2+, need dump 2nd level batch buffer from temp buffer
                    CENC_DECODE_CHK_STATUS(m_debugInterface->DumpHucRegion(
                    &m_resSlcLevelBatchBuf[i % CODECHAL_NUM_DECRYPT_BUFFERS].OsResource,
                    0,
                    m_resSlcLevelBatchBuf[i % CODECHAL_NUM_DECRYPT_BUFFERS].iSize,
                    0,
                    "",
                    false,
                    0,
                    CodechalHucRegionDumpType::hucRegionDumpDefault));
                }
                CENC_DECODE_CHK_STATUS(m_debugInterface->DumpHucRegion(
                    &m_resHucSharedBuf,
                    0,
                    m_sharedBufSize,
                    15,
                    "",
                    true,
                    0,
                    CodechalHucRegionDumpType::hucRegionDumpRegionLocked));
                MOS_RESOURCE *resHeap = nullptr;
                CENC_DECODE_CHK_NULL(resHeap = m_heapInfo[i].BitstreamBlock.GetResource());
                CENC_DECODE_CHK_STATUS(m_debugInterface->DumpBuffer(
                    resHeap,
                    CodechalDbgAttr::attrDecodeBitstream,
                    "_DEC",
                    m_heapInfo[i].BitstreamBlock.GetSize(),
                    m_heapInfo[i].BitstreamBlock.GetOffset(),
                    CODECHAL_NUM_MEDIA_STATES));

                if (m_aesFromHuC)
                {
                    CENC_DECODE_CHK_STATUS(m_debugInterface->DumpHucRegion(
                        &m_resHcpIndCryptoState[i],
                        0,
                        m_hcpIndCryptoStateSize,
                        2,
                        "",
                        false,
                        0,
                        CodechalHucRegionDumpType::hucRegionDumpDefault));
                    CENC_DECODE_CHK_STATUS(m_debugInterface->DumpHucRegion(
                        &m_resHucSegmentInfoBuf[i],
                        0,
                        m_hcpIndCryptoStateSize,
                        1,
                        "",
                        false,
                        0,
                        CodechalHucRegionDumpType::hucRegionDumpDefault));
                })

                if (*(uint32_t *)statusBuf == CODECHAL_STATUS_ERROR)
                {
                    CENC_DECODE_ASSERTMESSAGE("Kernel failed to execute correctly!!!!");
                    eStatus = MOS_STATUS_HUC_KERNEL_FAILED;
                }
            }
            else
            {
                // Considering about the parallelism between decrypt and decode,
                // block this call status by status. Pay attention to the reverse order.
                *(uint32_t *)statusBuf = CODECHAL_STATUS_INCOMPLETE;
                CENC_DECODE_VERBOSEMESSAGE("status buffer %d is INCOMPLETE.", j);
                if (j == (numStatus - 1))
                {
                    isRetry = true;
                    m_incompleteReportNum++;
                }
                else
                {
                    isRetry = false;
                }
            }

            if (*(uint32_t *)statusBuf == CODECHAL_STATUS_SUCCESSFUL ||
                *(uint32_t *)statusBuf == CODECHAL_STATUS_ERROR)
            {
                reportsGenerated++;
                CENC_DECODE_VERBOSEMESSAGE("Incrementing reports generated to %d.", reportsGenerated);
                isRetry = false;
            }
        }
    } while (isRetry && ((std::chrono::high_resolution_clock::now() - startTime) < MAX_DELAY_HUC_STATUS));

    decryptStatusBuf->wFirstIndex =
        (decryptStatusBuf->wFirstIndex + reportsGenerated) % CENC_NUM_DECRYPT_SHARED_BUFFERS;
    CENC_DECODE_VERBOSEMESSAGE("wFirstIndex now becomes %d.", decryptStatusBuf->wFirstIndex);

    return eStatus;
}

MOS_STATUS CodechalCencDecode::Initialize(
    PMOS_CONTEXT                osContext,
    CodechalSetting             *settings)
{
    uint32_t        patchListSize;
    uint8_t         idx; 
    bool            setVideoNode;
#if (_DEBUG || _RELEASE_INTERNAL)
    MOS_USER_FEATURE_VALUE_DATA       userFeatureData;
    MOS_USER_FEATURE_VALUE_WRITE_DATA userFeatureWriteData;
#endif  // (_DEBUG || _RELEASE_INTERNAL)
    MhwInterfaces *     mhwInterfaces = nullptr;
    MOS_STATUS          eStatus       = MOS_STATUS_SUCCESS;
    MediaUserSettingSharedPtr   userSettingPtr = nullptr;

    CENC_DECODE_FUNCTION_ENTER;

    CENC_DECODE_CHK_NULL(osContext);
    CENC_DECODE_CHK_NULL(settings);

    m_osInterface =
        (PMOS_INTERFACE)MOS_AllocAndZeroMemory(sizeof(MOS_INTERFACE));
    CENC_DECODE_CHK_NULL(m_osInterface);
    CENC_DECODE_CHK_STATUS(Mos_InitInterface(
        m_osInterface,
        osContext,
        COMPONENT_Decode));
    
    CENC_DECODE_CHK_STATUS(Mos_Solo_DisableAubcaptureOptimizations(
        m_osInterface,
        true));
    
    m_osInterface->pfnGetPlatform(m_osInterface, &m_platform);
    m_skuTable = m_osInterface->pfnGetSkuTable(m_osInterface);
    CENC_DECODE_CHK_NULL(m_skuTable);
    m_waTable = m_osInterface->pfnGetWaTable(m_osInterface);
    CENC_DECODE_CHK_NULL(m_waTable);

    userSettingPtr = m_osInterface->pfnGetUserSettingInstance(m_osInterface);

    m_videoGpuNode = MOS_GPU_NODE_VIDEO;
    if (MEDIA_IS_SKU(m_skuTable, FtrVcs2))
    {
        // For now force VDBOX0 for HuC-based DRM
        setVideoNode = true;
        m_videoGpuNode = MOS_GPU_NODE_VIDEO;

        CENC_DECODE_CHK_STATUS(m_osInterface->pfnCreateVideoNodeAssociation(
            m_osInterface,
            setVideoNode,
            &m_videoGpuNode));

        CODECHAL_UPDATE_VDBOX_USER_FEATURE(m_videoGpuNode, m_osInterface->pOsContext);
    }

    if (MOS_VE_SUPPORTED(m_osInterface))
    {
        // When virtual engine enabled, decrypt GPU context is different to decode
        m_videoGpuNode           = MOS_GPU_NODE_VIDEO;
        m_videoContextForDecrypt = MOS_GPU_CONTEXT_VIDEO2;
    }
    else
    {
        // Use WA context to differentiate cenc decode from decode
        m_videoContextForDecrypt = ((m_videoGpuNode == MOS_GPU_NODE_VIDEO2) ? MOS_GPU_CONTEXT_VDBOX2_VIDEO2 : MOS_GPU_CONTEXT_VIDEO2);
    }

    if (m_osInterface->osCpInterface->IsCencCtxBasedSubmissionEnabled())
    {
        m_osInterface->pfnVirtualEngineSupported(m_osInterface, true, true);
        Mos_SetVirtualEngineSupported(m_osInterface, true);
    }
    MOS_GPUCTX_CREATOPTIONS_ENHANCED createOption;
    CENC_DECODE_CHK_STATUS(m_osInterface->pfnCreateGpuContext(
        m_osInterface,
        m_videoContextForDecrypt,
        m_videoGpuNode,
        &createOption));

    CENC_DECODE_VERBOSEMESSAGE("GpuContext = %d", m_videoContextForDecrypt);
    m_vdboxIndex = (m_videoGpuNode == MOS_GPU_NODE_VIDEO2) ? MHW_VDBOX_NODE_2 : MHW_VDBOX_NODE_1;

    // Setup Hw dependent functions
    MhwInterfaces::CreateParams params;
    params.Flags.m_huc                 = true;
    params.Flags.m_mfx                 = true;
    params.Flags.m_hcp                 = true;
    params.m_isDecode                  = true;
    mhwInterfaces                      = MhwInterfaces::CreateFactory(params, m_osInterface);
    CENC_DECODE_CHK_NULL(mhwInterfaces);
    m_miInterface = mhwInterfaces->m_miInterface;
    CENC_DECODE_CHK_NULL(m_miInterface);
    CENC_DECODE_CHK_NULL(mhwInterfaces->m_cpInterface);
    m_cpInterface = dynamic_cast<MhwCpBase*>(mhwInterfaces->m_cpInterface);
    CENC_DECODE_CHK_NULL(m_cpInterface);
    m_hucInterface = mhwInterfaces->m_hucInterface;
    CENC_DECODE_CHK_NULL(m_hucInterface);
    m_mfxInterface = mhwInterfaces->m_mfxInterface;
    CENC_DECODE_CHK_NULL(m_mfxInterface);
    m_hcpInterface = mhwInterfaces->m_hcpInterface;
    CENC_DECODE_CHK_NULL(m_hcpInterface);
    m_hwInterface = CodechalHwInterface::Create(m_osInterface, CODECHAL_FUNCTION_CENC_DECODE, mhwInterfaces, false);
    CENC_DECODE_CHK_NULL(m_hwInterface);

    if (MEDIA_IS_SKU(m_hwInterface->GetSkuTable(), FtrMemoryCompression))
    {
        MOS_USER_FEATURE_VALUE_DATA userFeatureData;
        MOS_ZeroMemory(&userFeatureData, sizeof(userFeatureData));

        // read reg key of Codec MMC enabling. MMC default on.
        userFeatureData.i32Data = true;
        userFeatureData.i32DataFlag = MOS_USER_FEATURE_VALUE_DATA_FLAG_CUSTOM_DEFAULT_VALUE_TYPE;

        MOS_USER_FEATURE_VALUE_ID valueId = __MEDIA_USER_FEATURE_VALUE_CODEC_MMC_ENABLE_ID;
        MOS_UserFeature_ReadValue_ID(
            nullptr,
            valueId,
            &userFeatureData,
            m_osInterface->pOsContext);
        m_mmcEnabled = (userFeatureData.i32Data) ? true : false;
    }

#if USE_CODECHAL_DEBUG_TOOL
    m_debugInterface = CodechalDebugInterface::Create();
    CENC_DECODE_CHK_NULL(m_debugInterface);
    m_debugInterface->Initialize(m_hwInterface, CODECHAL_FUNCTION_CENC_DECODE);
#endif
    MOS_Delete(mhwInterfaces);

    m_cpInterface->RegisterParams(settings->GetCpParams());

    m_decryptDelayStart = CODECHAL_DECRYPT_DELAY_START_DEFAULT;
    m_decryptDelayStep  = CODECHAL_DECRYPT_DELAY_STEP_DEFAULT;
    MOS_NULL_RENDERING_FLAGS nullHWAccelerationEnable;
    nullHWAccelerationEnable.Value = 0;
    
#if (_DEBUG || _RELEASE_INTERNAL)
    MOS_ZeroMemory(&userFeatureData, sizeof(userFeatureData));
    userFeatureData.u32Data = CODECHAL_DECRYPT_DELAY_START_DEFAULT;
    eStatus                 = MOS_UserFeature_ReadValue_ID(
        nullptr,
        __MEDIA_USER_FEATURE_VALUE_DECRYPT_DELAY_START_ID,
        &userFeatureData,
        m_osInterface->pOsContext);
    m_decryptDelayStart = userFeatureData.u32Data;
    MOS_ZeroMemory(&userFeatureData, sizeof(userFeatureData));
    userFeatureData.u32Data = CODECHAL_DECRYPT_DELAY_STEP_DEFAULT;
    eStatus                 = MOS_UserFeature_ReadValue_ID(
        nullptr,
        __MEDIA_USER_FEATURE_VALUE_DECRYPT_DELAY_STEP_ID,
        &userFeatureData,
        m_osInterface->pOsContext);
    m_decryptDelayStep = userFeatureData.u32Data;

    ReadUserSettingForDebug(
        userSettingPtr,
        nullHWAccelerationEnable.Value,
        __MEDIA_USER_FEATURE_VALUE_NULL_HW_ACCELERATION_ENABLE,
        MediaUserSetting::Group::Device);
#endif  // (_DEBUG || _RELEASE_INTERNAL)

    m_videoContextUsesNullHw = nullHWAccelerationEnable.CodecGlobal || 
        nullHWAccelerationEnable.CtxVideo;

    // StateHeap initialize
    m_secondLvlBbHeap = MOS_New(HeapManager);
    CENC_DECODE_CHK_NULL(m_secondLvlBbHeap);
    CENC_DECODE_CHK_STATUS(m_secondLvlBbHeap->RegisterOsInterface(
        m_osInterface));
    
    if (MEDIA_IS_SKU(m_skuTable, FtrLimitedLMemBar))
    {
        m_secondLvlBbHeap->SetHwWriteOnlyHeap(true);
    }

    m_bitstreamHeap = MOS_New(HeapManager);
    CENC_DECODE_CHK_NULL(m_bitstreamHeap);
    CENC_DECODE_CHK_STATUS(m_bitstreamHeap->RegisterOsInterface(
        m_osInterface));

    if (MEDIA_IS_SKU(m_skuTable, FtrLimitedLMemBar) || MEDIA_IS_SKU(m_skuTable, FtrPAVPLMemOnly))
    {
        m_bitstreamHeap->SetHwWriteOnlyHeap(true);
    }

    m_standard = settings->standard;
    m_mode   = settings->mode;
    m_urrDecryptBufIdx = 0;
    m_currSharedBufIdx  = 0;
    m_bitstreamSize = m_bitstreamSizeMax = 0;
    m_sliceLevelBBSize = m_sliceLevelBBSizeMax = 0;
    m_HuCFamily = m_hucInterface->GetHucProductFamily();
    m_HuCRevId  = m_platform.usRevId;

    m_pr30Enabled = settings->CheckCencAdvance();

    for (idx = 0; idx < CENC_NUM_DECRYPT_SHARED_BUFFERS; idx++)
    {
        m_statusReportNumbers[idx] = 0xFFFF;
    }

#if (_DEBUG || _RELEASE_INTERNAL) && defined(CODECHAL_HUC_KERNEL_DEBUG)
    MOS_ZeroMemory(&UserFeatureData, sizeof(UserFeatureData));
    MOS_UserFeature_ReadValue_ID(
        nullptr,
        __MEDIA_USER_FEATURE_VALUE_HUC_KERNEL_DEBUG_LEVEL_ID,
        &UserFeatureData,
        m_osInterface->pOsContext);
    ucHucKernelDebugLevel = (uint8_t)UserFeatureData.i32Data;
#endif  // (_DEBUG || _RELEASE_INTERNAL)

    // calculate Huc command buffer size
    uint32_t cpCommandsSize  = 0;
    uint32_t cpPatchListSize = 0;
    m_cpInterface->GetCpStateLevelCmdSize(cpCommandsSize, cpPatchListSize);
    uint32_t                       hucCommandsSize  = 0;
    uint32_t                       hucPatchListSize = 0;
    MHW_VDBOX_STATE_CMDSIZE_PARAMS stateCmdSizeParams;
    stateCmdSizeParams.bShortFormat    = false;
    stateCmdSizeParams.bHucDummyStream = MEDIA_IS_WA(m_waTable, WaHucStreamoutEnable);

    CENC_DECODE_CHK_STATUS(m_hucInterface->GetHucStateCommandSize(
        CODECHAL_DECODE_MODE_CENC, (uint32_t *)&hucCommandsSize, (uint32_t *)&hucPatchListSize, &stateCmdSizeParams));

    m_hucDecryptSizeNeeded = hucCommandsSize + cpCommandsSize;
    patchListSize        = hucPatchListSize + cpPatchListSize;

#if (_DEBUG || _RELEASE_INTERNAL)
    MOS_ZeroMemory(&userFeatureData, sizeof(userFeatureData));
    eStatus = MOS_UserFeature_ReadValue_ID(
        nullptr,
        __MEDIA_USER_FEATURE_VALUE_AES_NATIVE_ENABLE_ID,
        &userFeatureData,
        m_osInterface->pOsContext);
    m_enableAesNativeSupport = (userFeatureData.u32Data) ? true : false;
#endif  // (_DEBUG || _RELEASE_INTERNAL)

    CENC_DECODE_VERBOSEMESSAGE("standard  = %d.", m_standard);
    CENC_DECODE_VERBOSEMESSAGE("dwHucDecryptSizeNeeded = %d.", m_hucDecryptSizeNeeded);

    if (MOS_VE_SUPPORTED(m_osInterface))
    {
        //single pipe VE initialize for decrypt
        MOS_VIRTUALENGINE_INIT_PARAMS  VEInitParams;
        MOS_ZeroMemory(&VEInitParams, sizeof(VEInitParams));
        VEInitParams.bScalabilitySupported = false;
        CENC_DECODE_CHK_STATUS(m_osInterface->pfnVirtualEngineInterfaceInitialize(m_osInterface, &VEInitParams))
    }

    return eStatus;
}

MOS_STATUS CodechalCencDecode::UpdateDataBuffer(void* buffer, uint32_t size)
{
    MOS_STATUS  eStatus = MOS_STATUS_SUCCESS;
    MOS_SURFACE dataBufDetails;
    bool        bAllocDataBuf = false;

    if (Mos_ResourceIsNull(&m_resDataBuffer))
    {
        bAllocDataBuf = true;
    }
    else
    {
        MOS_ZeroMemory(&dataBufDetails, sizeof(MOS_SURFACE));
        dataBufDetails.Format = Format_Invalid;
        m_osInterface->pfnGetResourceInfo(m_osInterface, &m_resDataBuffer, &dataBufDetails);
        if (dataBufDetails.dwSize < size)
        {
            m_osInterface->pfnFreeResource(m_osInterface, &m_resDataBuffer);
            bAllocDataBuf = true;
        }
    }

    if (bAllocDataBuf)
    {
        MOS_ALLOC_GFXRES_PARAMS allocParams;
        MOS_ZeroMemory(&allocParams, sizeof(MOS_ALLOC_GFXRES_PARAMS));
        allocParams.Type        = MOS_GFXRES_BUFFER;
        allocParams.TileType    = MOS_TILE_LINEAR;
        allocParams.Format      = Format_Buffer;
        allocParams.dwBytes     = size;
        allocParams.pBufName    = "encrypt bitstream buffer";

        m_osInterface->pfnAllocateResource(
            m_osInterface,
            &allocParams,
            &m_resDataBuffer);

        if (Mos_ResourceIsNull(&m_resDataBuffer))
        {
            CENC_DECODE_ASSERTMESSAGE("failed to allocate encrypt bitstream buffer!");
            return MOS_STATUS_UNKNOWN;
        }
    }

    CodechalResLock ResourceLock(m_osInterface, &m_resDataBuffer);
    auto data = (uint8_t*)ResourceLock.Lock(CodechalResLock::writeOnly);
    if (data == nullptr)
    {
        CENC_DECODE_ASSERTMESSAGE("failed to lock encrypt bitstream buffer!");
        return MOS_STATUS_UNKNOWN;
    }

    MOS_SecureMemcpy(data, size, buffer, size);

    return eStatus;
}

void CodechalCencDecode::SetReportSizeApp(uint32_t size)
{

    m_standardReportSizeApp = size;
}

void CodechalCencDecode::FixEmulationPrevention(uint8_t* & outBitStreamBuf, uint32_t* byteWritten)
{
    if (byteWritten == nullptr || outBitStreamBuf == nullptr)
    {
        CENC_DECODE_ASSERTMESSAGE("Invalid input parameters");
        return;
    }

    uint8_t* pPrevThreeBytes = outBitStreamBuf - 3;
    // If last 3 bytes written are "00 00 00" or "00 00 01" or "00 00 02" or "00 00 03",
    // insert an emulation byte, 3, as required by H.264
    if (*byteWritten >= 3 && pPrevThreeBytes[0] == 0 && pPrevThreeBytes[1] == 0
        && (pPrevThreeBytes[2] == 0 || pPrevThreeBytes[2] == 1 || pPrevThreeBytes[2] == 2 || pPrevThreeBytes[2] == 3))
    {
        *outBitStreamBuf = pPrevThreeBytes[2];
        pPrevThreeBytes[2] = 3;
        outBitStreamBuf++;
        (*byteWritten)++;
    }
}

void CodechalCencDecode::WriteBits(
    uint32_t uValue, uint8_t valueValidBits,
    uint8_t* accumulator, uint8_t* accumulatorValidBits,
    uint8_t* & outBitStreamBuf,
    uint32_t* byteWritten,
    bool fixEmulation)
{
    if (accumulator          == nullptr || 
        accumulatorValidBits == nullptr || 
        outBitStreamBuf      == nullptr || 
        byteWritten          == nullptr)
    {
        CENC_DECODE_ASSERTMESSAGE("Invalid input parameters");
        return;
    }

    // If uValue has valid bits up to the 32 bits, and previously unwritten number of bits
    // are less than a byte, write at least one byte to the output
    if (valueValidBits > 0 && valueValidBits <= SizeOfInBits(uValue) && *accumulatorValidBits < SizeOfInBits(*accumulator))
    {
        // Make sure that all the non-valid bits are masked off
        uValue = uValue & static_cast<uint32_t>((1ULL << valueValidBits) - 1);
        // If total valid bits, old and new, is less then a byte, just update the accumulator.
        if ((valueValidBits + *accumulatorValidBits) < SizeOfInBits(*outBitStreamBuf))
        {
            uint8_t uTmp1 = static_cast<uint8_t>(*accumulator << valueValidBits);
            uint8_t uTmp2 = static_cast<uint8_t>(uValue & ((1 << valueValidBits) - 1));
            *accumulator = static_cast<uint8_t>(uTmp1 | uTmp2);
            *accumulatorValidBits += valueValidBits;
        }
        else
        {
            // If have old bits, together with the new bits, at least a byte combined
            if (*accumulatorValidBits != 0)
            {
                uint8_t cBitsNeededFromValue = SizeOfInBits(*outBitStreamBuf) - *accumulatorValidBits;
                *outBitStreamBuf = (uint8_t)((*accumulator << cBitsNeededFromValue) & 0xff);
                *outBitStreamBuf |= static_cast<uint8_t>((uValue >> (valueValidBits - cBitsNeededFromValue)) & 0xff);
                outBitStreamBuf++;
                (*byteWritten)++;
                valueValidBits -= cBitsNeededFromValue;
                if (fixEmulation)
                {
                    FixEmulationPrevention(outBitStreamBuf, byteWritten);
                }
            }

            // If still have at least a byte in uValue, write it to the output
            while (valueValidBits >= SizeOfInBits(*outBitStreamBuf))
            {
                uint8_t cBitsToShift = valueValidBits - SizeOfInBits(*outBitStreamBuf);
                *outBitStreamBuf = static_cast<uint8_t>((uValue >> cBitsToShift) & 0xff);
                outBitStreamBuf++;
                (*byteWritten)++;
                valueValidBits = cBitsToShift;
                if (fixEmulation)
                {
                    FixEmulationPrevention(outBitStreamBuf, byteWritten);
                }
            }

            // At this point number of valid bits left unwritten to the stream is less than a byte
            *accumulator = static_cast<uint8_t>(uValue & ((1 << valueValidBits) - 1));
            *accumulatorValidBits = valueValidBits;
        }
    }
    else
    {
        CENC_DECODE_ASSERT(valueValidBits <= SizeOfInBits(uValue));
        CENC_DECODE_VERBOSEMESSAGE("Verify caller specified number of bits are less than 32");
        CENC_DECODE_ASSERT(*accumulatorValidBits < SizeOfInBits(*accumulator));
        CENC_DECODE_VERBOSEMESSAGE("Verify previously written number of bits are less than a byte");
    }
}

MOS_STATUS CodechalCencDecode::EncodeExpGolomb(
    uint32_t uValue, uint8_t valueValidBits,
    uint32_t bufSize, uint8_t* accumulator,
    uint8_t* accumulatorValidBits,
    uint8_t* & outBitStreamBuf,
    uint32_t* byteWritten)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    if (accumulator          == nullptr || 
        accumulatorValidBits == nullptr || 
        outBitStreamBuf      == nullptr || 
        byteWritten          == nullptr)
    {
        CENC_DECODE_ASSERTMESSAGE("Invalid input parameters");
        return MOS_STATUS_NULL_POINTER;
    }

    // The largest value to encode is 32 bit, which is at most 32 valid bits
    uint8_t uZeroBits = valueValidBits;
    // This lower limit of the range that can be encoded with uZeroBits number of bits
    uint32_t uLowerLimit = static_cast<uint32_t>(((1ULL << valueValidBits) - 1) & 0xffffffff);
    while (uZeroBits != 0 && uValue < uLowerLimit)
    {
        uZeroBits--;
        uLowerLimit = uLowerLimit >> 1;
    }

    WRITE_BITS(0, uZeroBits, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
    WRITE_BITS(1, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
    WRITE_BITS(uValue - uLowerLimit, uZeroBits, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);

    return eStatus;
}

