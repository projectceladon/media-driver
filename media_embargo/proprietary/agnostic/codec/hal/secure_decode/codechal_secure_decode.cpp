/*
// Copyright (C) 2017-2021 Intel Corporation
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
//! \file     codechal_secure_decode.cpp
//! \brief    Impelements the public interface for CodecHal Secure Decode 
//!

#include "codechal_secure_decode.h"
#include "codechal_secure_decode_avc.h"
#include "codechal_secure_decode_hevc.h"
#include "codechal_secure_decode_vp9.h"
#ifdef _AV1_SECURE_DECODE_SUPPORTED
#include "codechal_secure_decode_av1.h"
#include "codec_def_common_av1.h"
#endif
#include "codechal_setting_ext.h"
#include "mos_gfxinfo_id.h"

CodechalSecureDecode::CodechalSecureDecode(CodechalHwInterface *hwInterfaceInput)
{
    hwInterface = hwInterfaceInput;
    osInterface = hwInterfaceInput->GetOsInterface();
    waTable     = hwInterfaceInput->GetWaTable();

    for (int i = 0; i < INDIRECT_CRYPTO_STATE_BUFFER_NUM; i++)
    {
        MOS_ZeroMemory(&hucIndCryptoStateBuf[i], sizeof(MOS_RESOURCE));
        hucIndCryptoStateBufSize[i] = 0; 
    }

    for (uint32_t i = 0; i < STREAM_OUT_BUFFER_NUM; i++)
    {
        MOS_ZeroMemory(&hucStreamOutBuf[i], sizeof(MOS_RESOURCE));
        hucStreamOutBufSize[i] = 0;
    }

    MOS_ZeroMemory(&auxBuf, sizeof(MOS_RESOURCE));
}

CodechalSecureDecode::~CodechalSecureDecode()
{
    if (!Mos_ResourceIsNull(&auxBuf))
    {
        osInterface->pfnFreeResource(osInterface, &auxBuf);
    }
    FreeStreamOutResource();
}

CodechalSecureDecodeInterface* Create_SecureDecode(
    CodechalSetting *          codecHalSettings,
    CodechalHwInterface         *hwInterfaceInput)
{
    bool                   hucBasedDrmInUse     = false;
    bool                   shortFormatInUse     = false;
    MOS_STATUS             eStatus              = MOS_STATUS_SUCCESS;
    MhwCpBase               *mhwCp = nullptr;
    CodechalSecureDecode*    pSecureDecode = nullptr;
    CodechalSettingExt      *pcodecHalSettingExt = nullptr;

    CODECHAL_DECODE_FUNCTION_ENTER;

    CODECHAL_DECODE_CHK_NULL(codecHalSettings);
    CODECHAL_DECODE_CHK_NULL(hwInterfaceInput);

    mhwCp =  dynamic_cast<MhwCpBase*>(hwInterfaceInput->GetCpInterface());
    CODECHAL_DECODE_CHK_NULL(mhwCp);


    switch (codecHalSettings->standard)
    {
    case CODECHAL_AVC:
        pSecureDecode= MOS_New(CodechalSecureDecodeAvc,hwInterfaceInput);
        CODECHAL_DECODE_CHK_NULL(pSecureDecode);
        break;
    case CODECHAL_HEVC:
        pSecureDecode= MOS_New(CodechalSecureDecodeHevc,hwInterfaceInput);
        CODECHAL_DECODE_CHK_NULL(pSecureDecode);
        break;
    case CODECHAL_VP9:
        pSecureDecode = MOS_New(CodechalSecureDecodeVp9,hwInterfaceInput);
        CODECHAL_DECODE_CHK_NULL(pSecureDecode);
        break;
#ifdef _AV1_SECURE_DECODE_SUPPORTED
    case CODECHAL_AV1:
        pSecureDecode = MOS_New(CodechalSecureDecodeAv1, hwInterfaceInput);
        CODECHAL_DECODE_CHK_NULL(pSecureDecode);
        break;
#endif
    default:
        pSecureDecode = MOS_New(CodechalSecureDecode,hwInterfaceInput);
        CODECHAL_DECODE_CHK_NULL(pSecureDecode);
        break;
    }

    // for telemetry events
    pcodecHalSettingExt = dynamic_cast<CodechalSettingExt*>(codecHalSettings);

    if (pcodecHalSettingExt && pSecureDecode)
    {
        uint32_t codecFunction = codecHalSettings->codecFunction;
        uint32_t shortFormatInUse = codecHalSettings->shortFormatInUse ? 1 : 0;
        MosUtilities::MosGfxInfo(
            1,
            GFXINFO_COMPID_UMD_MEDIA_CP,
            GFXINFO_ID_CP_HUC_HEADER_PARSING,
            3,
            "data1", GFXINFO_PTYPE_UINT32, &codecFunction,
            "data2", GFXINFO_PTYPE_UINT32, &shortFormatInUse,
            "data3", GFXINFO_PTYPE_UINT32, &(codecHalSettings->standard)
        );
    }

finish:
    return pSecureDecode;
}

void Delete_SecureDecode(CodechalSecureDecodeInterface* pCodechalSecureDecoeInterface)
{
    MOS_Delete(pCodechalSecureDecoeInterface);
}

MOS_STATUS CodechalSecureDecode::AllocateStreamOutResource(uint32_t streamOutBufRequiredSize)
{
    MOS_ALLOC_GFXRES_PARAMS     AllocParamsForBufferLinear;
    MOS_ALLOC_GFXRES_PARAMS     AllocParamsForBuffer2D;

    CODECHAL_DECODE_FUNCTION_ENTER;

    CODECHAL_DECODE_CHK_NULL_RETURN(hwInterface);
    CODECHAL_DECODE_CHK_NULL_RETURN(hwInterface->GetCpInterface());
    MhwCpBase *mhwCp =  dynamic_cast<MhwCpBase*>(hwInterface->GetCpInterface());
    CODECHAL_DECODE_CHK_NULL_RETURN(mhwCp);
    CODECHAL_DECODE_CHK_NULL_RETURN(osInterface);

    PMHW_PAVP_ENCRYPTION_PARAMS pEncryptionParams = mhwCp->GetEncryptionParams();
    if ((mhwCp->GetHostEncryptMode() == CP_TYPE_BASIC) || pEncryptionParams->dwSubSampleCount == 0)
    {
        return MOS_STATUS_SUCCESS;
    }

    //There is three senarios used in secure decoder,1 is huc streamout,2 is huc S2L,3 is fullsample. isStreamoutNeeded = true represent 1st senario.
    isStreamoutNeeded = true;
    CODECHAL_DECODE_CHK_COND_RETURN((pEncryptionParams->pSubSampleMappingBlock == nullptr),
        "Error - Null sample mapping block!");

    // Initiate allocation paramters
    MOS_ZeroMemory(&AllocParamsForBufferLinear, sizeof(MOS_ALLOC_GFXRES_PARAMS));
    AllocParamsForBufferLinear.Type     = MOS_GFXRES_BUFFER;
    AllocParamsForBufferLinear.TileType = MOS_TILE_LINEAR;
    AllocParamsForBufferLinear.Format   = Format_Buffer;
    MOS_ZeroMemory(&AllocParamsForBuffer2D, sizeof(MOS_ALLOC_GFXRES_PARAMS));
    AllocParamsForBuffer2D.Type         = MOS_GFXRES_2D;
    AllocParamsForBuffer2D.TileType     = MOS_TILE_LINEAR;
    AllocParamsForBuffer2D.Format       = Format_Buffer_2D;

    if (m_enableSampleGroupConstantIV)
    {
        if (m_pSubsampleSegmentNum)
        {
            MOS_FreeMemory(m_pSubsampleSegmentNum);
            m_pSubsampleSegmentNum = nullptr;
        }

        m_pSubsampleSegmentNum =
            (uint32_t *)MOS_AllocAndZeroMemory(sizeof(uint32_t) * pEncryptionParams->dwSubSampleCount);
        CODECHAL_DECODE_CHK_COND_RETURN((m_pSubsampleSegmentNum == nullptr),
            "Failed to allocate subsample segment number buffer.");
    }

    curSegmentNum = 0;
    if (pEncryptionParams->dwBlocksStripeEncCount > 0)
    {
        uint32_t *pSubSampleMappingBlock = (uint32_t *)pEncryptionParams->pSubSampleMappingBlock;
        for (uint32_t i = 0; i < pEncryptionParams->dwSubSampleCount; i++)
        {
            uint32_t encryptedSize = *(pSubSampleMappingBlock + 2 * i + 1);
            curSegmentNum += ComputeSubSegmentNumStripe(
                encryptedSize,
                pEncryptionParams->dwBlocksStripeEncCount,
                pEncryptionParams->dwBlocksStripeClearCount);

            if (m_enableSampleGroupConstantIV)
            {
                // Save Subsample ending location
                m_pSubsampleSegmentNum[i] = curSegmentNum;
            }
        }

        if ((curSegmentNum > sampleRemappingBlockNum) ||
            sampleRemappingBlock == nullptr)
        {
            if (sampleRemappingBlock != nullptr)
            {
                MOS_FreeMemory(sampleRemappingBlock);
            }

            sampleRemappingBlock =
                (uint32_t *)MOS_AllocAndZeroMemory(sizeof(uint32_t) * 2 * curSegmentNum);
            CODECHAL_DECODE_CHK_COND_RETURN((sampleRemappingBlock == nullptr),
                "Failed to allocate remapped sample block buffer.");
        }

         sampleRemappingBlockNum = curSegmentNum;
    }
    else
    {
        curSegmentNum = pEncryptionParams->dwSubSampleCount;
    }

    // IndCryptoState buffer
    uint32_t curHucIndCryptoStateBufRequiredSize =
        mhwCp->GetSizeOfCmdIndirectCryptoState() * curSegmentNum;

    if ((curHucIndCryptoStateBufRequiredSize > hucIndCryptoStateBufSize[hucIndCryptoStateBufIndex]) ||
        Mos_ResourceIsNull(&hucIndCryptoStateBuf[hucIndCryptoStateBufIndex]))
    {
        if (!Mos_ResourceIsNull(&hucIndCryptoStateBuf[hucIndCryptoStateBufIndex]))
        {
            osInterface->pfnFreeResource(
                osInterface,
                &hucIndCryptoStateBuf[hucIndCryptoStateBufIndex]);
        }

        AllocParamsForBufferLinear.dwBytes = curHucIndCryptoStateBufRequiredSize;
        AllocParamsForBufferLinear.pBufName = "HucIndirectCryptoStateBuffer";

        CODECHAL_DECODE_CHK_STATUS_MESSAGE_RETURN(osInterface->pfnAllocateResource(
            osInterface,
            &AllocParamsForBufferLinear,
            &hucIndCryptoStateBuf[hucIndCryptoStateBufIndex]),
            "Failed to allocate HuC indirect crypto state buffer.");

        hucIndCryptoStateBufSize[hucIndCryptoStateBufIndex] = curHucIndCryptoStateBufRequiredSize;
    }

    // Segment Info buffer
    if ((curSegmentNum > segmentNum) ||
        segmentInfo == nullptr)
    {
        if (segmentInfo != nullptr)
        {
            MOS_FreeMemory(segmentInfo);
        }

        segmentInfo =
            (PMHW_SEGMENT_INFO)MOS_AllocAndZeroMemory(sizeof(MHW_SEGMENT_INFO) * curSegmentNum);
        CODECHAL_DECODE_CHK_COND_RETURN((segmentInfo == nullptr),
            "Failed to allocate segment info buffer.");

        segmentNum = curSegmentNum;
    }

    // Stream Out buffer
    if ((streamOutBufRequiredSize > hucStreamOutBufSize[hucStreamOutBufIndex]) ||
        (Mos_ResourceIsNull(&hucStreamOutBuf[hucStreamOutBufIndex]) && streamOutBufRequiredSize > 0))
    {
        if (!Mos_ResourceIsNull(&hucStreamOutBuf[hucStreamOutBufIndex]))
        {
            // Lock/unlock the buffer to ensure HW is not using it
            {
                CodechalResLock streamoutBuf(osInterface, &hucStreamOutBuf[hucStreamOutBufIndex]);
                auto bufData = streamoutBuf.Lock(CodechalResLock::readOnly);
                CODECHAL_DECODE_CHK_NULL_RETURN(bufData);
            }
            osInterface->pfnFreeResource(
                osInterface,
                &hucStreamOutBuf[hucStreamOutBufIndex]);
        }

        AllocParamsForBufferLinear.dwBytes = MOS_ALIGN_CEIL(streamOutBufRequiredSize, CODECHAL_PAGE_SIZE);
        AllocParamsForBufferLinear.pBufName = "HucStreamOutBuffer";

        CODECHAL_DECODE_CHK_STATUS_MESSAGE_RETURN(osInterface->pfnAllocateResource(
            osInterface,
            &AllocParamsForBufferLinear,
            &hucStreamOutBuf[hucStreamOutBufIndex]),
            "Failed to allocate HuC stream out buffer.");

        hucStreamOutBufSize[hucStreamOutBufIndex] = AllocParamsForBufferLinear.dwBytes;
    }

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS CodechalSecureDecode::FreeStreamOutResource()
{
    CODECHAL_DECODE_FUNCTION_ENTER;

    for (uint32_t i = 0; i < STREAM_OUT_BUFFER_NUM; i++)
    {
        if (!Mos_ResourceIsNull(&hucStreamOutBuf[i]))
        {
            osInterface->pfnFreeResource(osInterface, &hucStreamOutBuf[i]);
        }
        hucStreamOutBufSize[i] = 0;
    }
    hucStreamOutBufIndex = 0;

    for (int i = 0; i < INDIRECT_CRYPTO_STATE_BUFFER_NUM; i++)
    {
        if (!Mos_ResourceIsNull(&hucIndCryptoStateBuf[i]))
        {
            osInterface->pfnFreeResource(osInterface, &hucIndCryptoStateBuf[i]);
        }

        hucIndCryptoStateBufSize[i] = 0;
    }

    MOS_FreeMemory(segmentInfo);
    segmentInfo = nullptr;
    MOS_FreeMemory(sampleRemappingBlock);
    sampleRemappingBlock = nullptr;

    if (m_pSubsampleSegmentNum)
    {
        MOS_FreeMemory(m_pSubsampleSegmentNum);
        m_pSubsampleSegmentNum = nullptr;
    }

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS CodechalSecureDecode::PerformHucStreamOut(
    CodechalHucStreamoutParams          *hucStreamOutParams,
    PMOS_COMMAND_BUFFER                 cmdBuffer)
{
    MhwVdboxHucInterface                *hucInterface = nullptr;
    MHW_PAVP_AES_PARAMS                 AESParams;
    MHW_PAVP_AES_IND_PARAMS             IndAesParams;
    MHW_VDBOX_PIPE_MODE_SELECT_PARAMS   PipeModeSelectParams;
    MHW_VDBOX_IND_OBJ_BASE_ADDR_PARAMS  IndObjParams;
    MHW_VDBOX_HUC_STREAM_OBJ_PARAMS     StreamObjParams;
    MOS_STATUS                          eStatus = MOS_STATUS_SUCCESS;
    MhwCpBase                            *mhwCp = nullptr;

    CODECHAL_DECODE_FUNCTION_ENTER;

    CODECHAL_DECODE_CHK_NULL(hwInterface);
    CODECHAL_DECODE_CHK_NULL(hwInterface->GetCpInterface());
    mhwCp = dynamic_cast<MhwCpBase*>(hwInterface->GetCpInterface());
    CODECHAL_DECODE_CHK_NULL(mhwCp);
    CODECHAL_DECODE_CHK_NULL(osInterface);
    CODECHAL_DECODE_CHK_NULL(osInterface->osCpInterface);
    CODECHAL_DECODE_CHK_NULL(hucStreamOutParams);
    CODECHAL_DECODE_CHK_NULL(cmdBuffer);

    hucInterface = hwInterface->GetHucInterface();

    if (MEDIA_IS_SKU(hwInterface->GetSkuTable(), FtrEnableMediaKernels) && MEDIA_IS_WA(hwInterface->GetWaTable(), WaHucStreamoutEnable))
    {
        CODECHAL_DECODE_CHK_STATUS(hwInterface->AddHucDummyStreamOut(cmdBuffer));
    }

    // Pipe mode select
    PipeModeSelectParams.Mode = hucStreamOutParams->mode;
    PipeModeSelectParams.dwMediaSoftResetCounterValue = 2400;
    PipeModeSelectParams.bStreamObjectUsed = true;
    PipeModeSelectParams.bStreamOutEnabled = true;
    if (hucStreamOutParams->segmentInfo == nullptr && osInterface->osCpInterface->IsCpEnabled())
    {
        // Disable AES control setting in huc drm pavp heavy mode
        PipeModeSelectParams.disableProtectionSetting = true;
    }

    // Enlarge the stream in/out data size to avoid upper bound hit assert in HuC
    hucStreamOutParams->dataSize += hucStreamOutParams->inputRelativeOffset;
    hucStreamOutParams->streamOutObjectSize += hucStreamOutParams->outputRelativeOffset;

    // Pass bit-stream buffer by Ind Obj Addr command
    MOS_ZeroMemory(&IndObjParams, sizeof(IndObjParams));
    IndObjParams.presDataBuffer = hucStreamOutParams->dataBuffer;
    IndObjParams.dwDataSize = MOS_ALIGN_CEIL(hucStreamOutParams->dataSize, MHW_PAGE_SIZE);
    IndObjParams.dwDataOffset = hucStreamOutParams->dataOffset;
    IndObjParams.presStreamOutObjectBuffer = hucStreamOutParams->streamOutObjectBuffer;
    IndObjParams.dwStreamOutObjectSize = MOS_ALIGN_CEIL(hucStreamOutParams->streamOutObjectSize, MHW_PAGE_SIZE);
    IndObjParams.dwStreamOutObjectOffset = hucStreamOutParams->streamOutObjectOffset;

    // Set stream object with stream out enabled
    MOS_ZeroMemory(&StreamObjParams, sizeof(StreamObjParams));
    StreamObjParams.dwIndStreamInLength = hucStreamOutParams->indStreamInLength;
    StreamObjParams.dwIndStreamInStartAddrOffset = hucStreamOutParams->inputRelativeOffset;
    StreamObjParams.dwIndStreamOutStartAddrOffset = hucStreamOutParams->outputRelativeOffset;
    StreamObjParams.bHucProcessing = true;
    StreamObjParams.bStreamInEnable = true;
    StreamObjParams.bStreamOutEnable = true;

    CODECHAL_DECODE_CHK_STATUS(hucInterface->AddHucPipeModeSelectCmd(cmdBuffer, &PipeModeSelectParams));
    CODECHAL_DECODE_CHK_STATUS(hucInterface->AddHucIndObjBaseAddrStateCmd(cmdBuffer, &IndObjParams));

    if (hucStreamOutParams->segmentInfo)
    {
        MOS_ZeroMemory(&IndAesParams, sizeof(IndAesParams));
        IndAesParams.presCryptoBuffer = &hucStreamOutParams->hucIndState;
        IndAesParams.dwOffset = hucStreamOutParams->curIndEntriesNum;
        IndAesParams.dwNumSegments = hucStreamOutParams->curNumSegments;
        IndAesParams.pSegmentInfo = (PMHW_SEGMENT_INFO)hucStreamOutParams->segmentInfo;
        CODECHAL_DECODE_CHK_STATUS(mhwCp->SetHucIndirectCryptoState(&IndAesParams));

        MOS_ZeroMemory(&AESParams, sizeof(AESParams));
        AESParams.presDataBuffer = &hucStreamOutParams->hucIndState;
        AESParams.dwOffset = hucStreamOutParams->curIndEntriesNum * mhwCp->GetSizeOfCmdIndirectCryptoState();
        AESParams.dwSliceIndex = hucStreamOutParams->curNumSegments;
        CODECHAL_DECODE_CHK_STATUS(mhwCp->AddHucAesIndState(cmdBuffer, &AESParams));
    }

    CODECHAL_DECODE_CHK_STATUS(hucInterface->AddHucStreamObjectCmd(cmdBuffer, &StreamObjParams));

    // This flag is always false if huc fw is not loaded.
    if (MEDIA_IS_WA(hwInterface->GetWaTable(), WaHucStreamoutOnlyDisable))
    {
        CODECHAL_DECODE_CHK_STATUS(hwInterface->AddHucDummyStreamOut(cmdBuffer));
    }

finish:
    return eStatus;
}

uint32_t CodechalSecureDecode::ComputeSubSegmentNumStripe(
    uint32_t encryptedSize,
    uint32_t stripeEncCount,
    uint32_t stripeClearCount)
{
    CODECHAL_DECODE_FUNCTION_ENTER;

    // Compute stripe pair number
    uint32_t subSegmentNum = encryptedSize /
        ((stripeEncCount + stripeClearCount) * stripeBlockSize);

    // Comptue residual size to check if there is remaining enc block
    uint32_t residualSize = encryptedSize -
        subSegmentNum * (stripeEncCount + stripeClearCount) * stripeBlockSize;
    if (residualSize > (stripeBlockSize * stripeEncCount))
    {
        // If residual size bigger than stripe enc size that means there are enc part with clear part.
        subSegmentNum += 2;
    }
    else if (residualSize > stripeBlockSize &&
             ((residualSize % stripeBlockSize) != 0))
    {
        // else if residual size bigger than 16 and don't align to 16, then there are enc part with clear part.
        subSegmentNum += 2;
    }
    else if (residualSize != 0 ||
             (residualSize == 0 && encryptedSize == 0) ||
             (residualSize == 0 && stripeClearCount != 0))
    {
        // Remainging four cases:
        //           1. residual size bigger than 16 and align to 16, then there will be enc part only.
        //           2. residual size equal to 16, then there will be enc part only.
        //           3. residual size smaller than 16, then there will be clear part only.
        //           4. residual size is zero, then there is no block for clear header if 1) encryptedsize not equal to 0 and 2) clear stripe equal to 0.
            subSegmentNum++;
    }

    return subSegmentNum;
}

uint32_t CodechalSecureDecode::ComputeSubStripeEncLen(
    uint32_t remappingSize,
    uint32_t stripeEncCount)
{
    if (remappingSize >= (stripeEncCount * stripeBlockSize))
    {
        return (stripeEncCount * stripeBlockSize);
    }
    else if (remappingSize >= stripeBlockSize)
    {
        return MOS_ALIGN_FLOOR(remappingSize, stripeBlockSize);
    }
    else
    {
        return 0;
    }
}

uint32_t CodechalSecureDecode::ComputeSubStripeClearLen(
    uint32_t remappingSize,
    uint32_t stripeClearCount)
{
    if (remappingSize >= (stripeClearCount * stripeBlockSize))
    {
        return (stripeClearCount * stripeBlockSize);
    }
    else
    {
        return remappingSize;
    }
}

MOS_STATUS CodechalSecureDecode::ComputeRemappingBlockStripe(
    uint32_t subSampleCount,
    uint32_t *subSampleMappingBlock,
    uint32_t stripeEncCount,
    uint32_t stripeClearCount)
{
    CODECHAL_DECODE_FUNCTION_ENTER;

    CODECHAL_DECODE_CHK_NULL_RETURN(subSampleMappingBlock);
    CODECHAL_DECODE_CHK_COND_RETURN((subSampleCount == 0), "Error - Segment count is zero!");
    CODECHAL_DECODE_CHK_COND_RETURN((stripeEncCount == 0), "Error - wrong stripe parameter!");

    uint32_t remappingIdx = 0;
    for (uint32_t i = 0; i < subSampleCount; i++)
    {
        uint32_t uiClearSize = *(subSampleMappingBlock + 2 * i);
        uint32_t uiEncryptedSize = *(subSampleMappingBlock + 2 * i + 1);

        uint32_t *curRemappingBlock = sampleRemappingBlock + 2 * remappingIdx;
        *curRemappingBlock = uiClearSize;
        uint32_t subStripeEncLen = ComputeSubStripeEncLen(uiEncryptedSize, stripeEncCount);
        *(curRemappingBlock + 1) = subStripeEncLen;

        // Not enough even for first encryption block, then current block is clear at all.
        if (subStripeEncLen == 0)
        {
            *curRemappingBlock += uiEncryptedSize;
            uiEncryptedSize = 0;
        }

        remappingIdx++;

        uint32_t remappingSize = uiEncryptedSize - subStripeEncLen;
        while (remappingSize > 0)
        {
            CODECHAL_DECODE_CHK_COND_RETURN((remappingIdx >= curSegmentNum), "Error - Remapping block exceeds maximum number!\n");

            curRemappingBlock = sampleRemappingBlock + 2 * remappingIdx;

            uiClearSize = ComputeSubStripeClearLen(remappingSize, stripeClearCount);
            *curRemappingBlock = uiClearSize;
            remappingSize -= uiClearSize;
            subStripeEncLen = ComputeSubStripeEncLen(remappingSize, stripeEncCount);
            *(curRemappingBlock + 1) = subStripeEncLen;
            remappingSize -= subStripeEncLen;

            // If current block and next block are clear only, then next block is the tail of current block
            if (subStripeEncLen == 0 &&
                remappingSize > 0 &&
                remappingSize <= (stripeClearCount * stripeBlockSize))
            {
                *curRemappingBlock += remappingSize;
                remappingSize = 0;
            }

            remappingIdx++;
        }
    }

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS CodechalSecureDecode::GetSegmentInfoLegacy(
    MhwCpInterface *cpInterface,
    PMOS_RESOURCE dataBufferRes,
    uint32_t subSampleCount,
    uint32_t *subSampleMappingBlock)
{
    CODECHAL_DECODE_FUNCTION_ENTER;
    MhwCpBase *mhwCp =  dynamic_cast<MhwCpBase*>(cpInterface);
    CODECHAL_DECODE_CHK_NULL_RETURN(mhwCp);

    CODECHAL_DECODE_CHK_COND_RETURN((subSampleCount == 0),
        "Error - Segment count is zero!");
    CODECHAL_DECODE_CHK_COND_RETURN((subSampleMappingBlock == nullptr),
        "Error - Null sample mapping block!");

    CODECHAL_DECODE_CHK_NULL_RETURN(mhwCp);
    CODECHAL_DECODE_CHK_NULL_RETURN(mhwCp->GetEncryptionParams());
    PMHW_PAVP_ENCRYPTION_PARAMS encryptionParams = mhwCp->GetEncryptionParams();

    // CBC related
    uint8_t *dataBuffer = nullptr;
    uint32_t lastEncryptedOword[CODEC_DWORDS_PER_AES_BLOCK];
    CodechalResLock ResourceLock(osInterface, dataBufferRes);
    if (encryptionParams->PavpEncryptionType == CP_SECURITY_AES128_CBC)
    {
        dataBuffer = (uint8_t *)ResourceLock.Lock(CodechalResLock::readOnly);
        CODECHAL_DECODE_CHK_NULL_RETURN(dataBuffer);
        memset(lastEncryptedOword, 0, sizeof(lastEncryptedOword));
        memcpy_s(lastEncryptedOword, sizeof(lastEncryptedOword),
            encryptionParams->dwPavpAesCounter, sizeof(encryptionParams->dwPavpAesCounter));
    }

    uint32_t uiPreEncryptedSize = 0;
    uint8_t ucPreAesResidual    = 0;
    uint32_t subsampleIndex     = 0;

    for (uint32_t i = 0; i < subSampleCount; i++)
    {
        uint32_t uiClearSize = *(subSampleMappingBlock + 2 * i);
        uint32_t uiEncryptedSize = *(subSampleMappingBlock + 2 * i + 1);

        uint8_t ucCurAesResidualBegin = (ucPreAesResidual == 0) ? 0 : (16 - ucPreAesResidual);

        if (i == 0)
        {
            // Current segment is the first segment of this slice
            segmentInfo[i].dwSegmentStartOffset = 0;
            segmentInfo[i].dwInitByteLength     = uiClearSize;
        }
        else
        {
            // The remaing segment of current slice, this slice already has one or more segments
            segmentInfo[i].dwSegmentStartOffset = segmentInfo[i - 1].dwSegmentStartOffset + segmentInfo[i - 1].dwSegmentLength;
            segmentInfo[i].dwInitByteLength     = uiClearSize;
        }

        segmentInfo[i].dwSegmentLength              = uiClearSize + uiEncryptedSize;
        segmentInfo[i].dwPartialAesBlockSizeInBytes = ucCurAesResidualBegin;

        if (encryptionParams->PavpEncryptionType == CP_SECURITY_AES128_CBC)
        {
            if ((encryptionParams->dwBlocksStripeEncCount > 0) && m_enableSampleGroupConstantIV && (m_pSubsampleSegmentNum[subsampleIndex] == i))
            {
                // Reset IV for next Subsample
                MOS_SecureMemcpy(lastEncryptedOword, sizeof(lastEncryptedOword), encryptionParams->dwPavpAesCounter, sizeof(encryptionParams->dwPavpAesCounter));
                subsampleIndex++;
            }
            memcpy_s(segmentInfo[i].dwAesCbcIvOrCtr, sizeof(segmentInfo[i].dwAesCbcIvOrCtr),
                lastEncryptedOword, sizeof(lastEncryptedOword));

            if (segmentInfo[i].dwSegmentLength > segmentInfo[i].dwInitByteLength)
            {
                CODECHAL_DECODE_CHK_COND_RETURN(
                    ((segmentInfo[i].dwSegmentStartOffset + segmentInfo[i].dwSegmentLength) < sizeof(lastEncryptedOword)),
                    "Error - Wrong sample block parameters!");
                uint8_t *lastEncryptedEnd = dataBuffer + segmentInfo[i].dwSegmentStartOffset + segmentInfo[i].dwSegmentLength;
                memcpy_s(lastEncryptedOword, sizeof(lastEncryptedOword),
                    lastEncryptedEnd - sizeof(lastEncryptedOword), sizeof(lastEncryptedOword));
            }
        }
        else
        {
            memcpy_s(segmentInfo[i].dwAesCbcIvOrCtr, sizeof(segmentInfo[i].dwAesCbcIvOrCtr),
                encryptionParams->dwPavpAesCounter, sizeof(encryptionParams->dwPavpAesCounter));

            mhwCp->IncrementCounter(
                segmentInfo[i].dwAesCbcIvOrCtr,
                uiPreEncryptedSize >> 4,
                encryptionParams->dwIVSize);
        }

        uiPreEncryptedSize += uiEncryptedSize;
        if (uiEncryptedSize >= ucCurAesResidualBegin)
        {
            ucPreAesResidual = (uiEncryptedSize - ucCurAesResidualBegin) & 0x0F;
        }
        else
        {
            ucPreAesResidual = ucPreAesResidual + uiEncryptedSize;
        }
    }

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS CodechalSecureDecode::GetSegmentInfoStripe(
    MhwCpInterface *cpInterface,
    PMOS_RESOURCE  dataBufferRes)
{
    CODECHAL_DECODE_FUNCTION_ENTER;

    MhwCpBase *mhwCp =  dynamic_cast<MhwCpBase*>(cpInterface);
    CODECHAL_DECODE_CHK_NULL_RETURN(mhwCp);
    CODECHAL_DECODE_CHK_NULL_RETURN(mhwCp->GetEncryptionParams());
    PMHW_PAVP_ENCRYPTION_PARAMS pEncryptionParams = mhwCp->GetEncryptionParams();

    CODECHAL_DECODE_CHK_STATUS_RETURN(ComputeRemappingBlockStripe(
        pEncryptionParams->dwSubSampleCount,
        (uint32_t *)pEncryptionParams->pSubSampleMappingBlock,
        pEncryptionParams->dwBlocksStripeEncCount,
        pEncryptionParams->dwBlocksStripeClearCount));

    CODECHAL_DECODE_CHK_STATUS_RETURN(GetSegmentInfoLegacy(
        cpInterface,
        dataBufferRes,
        curSegmentNum,
        sampleRemappingBlock));

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS CodechalSecureDecode::GetSegmentInfo(
    MhwCpInterface *cpInterface,
    PMOS_RESOURCE  dataBufferRes)
{
    CODECHAL_DECODE_FUNCTION_ENTER;

    CODECHAL_DECODE_CHK_NULL_RETURN(cpInterface);
    MhwCpBase *mhwCp =  dynamic_cast<MhwCpBase*>(cpInterface);
    CODECHAL_DECODE_CHK_NULL_RETURN(mhwCp);
    CODECHAL_DECODE_CHK_NULL_RETURN(dataBufferRes);

    PMHW_PAVP_ENCRYPTION_PARAMS pEncryptionParams = mhwCp->GetEncryptionParams();
    if (pEncryptionParams->dwBlocksStripeEncCount == 0)
    {
        CODECHAL_DECODE_CHK_STATUS_RETURN(GetSegmentInfoLegacy(
            cpInterface,
            dataBufferRes,
            pEncryptionParams->dwSubSampleCount,
            (uint32_t *)pEncryptionParams->pSubSampleMappingBlock));
    }
    else
    {
        CODECHAL_DECODE_CHK_STATUS_RETURN(GetSegmentInfoStripe(
            cpInterface,
            dataBufferRes));
    }

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS CodechalSecureDecode::UpdateHuCStreamoutBufferIndex()
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    CODECHAL_DECODE_FUNCTION_ENTER;

    if (isStreamoutNeeded)
    {
        hucStreamOutBufIndex = (hucStreamOutBufIndex + 1) % STREAM_OUT_BUFFER_NUM;
        isStreamoutNeeded = false;
    }

    return eStatus;
}

bool CodechalSecureDecode::IsAuxDataInvalid(
    PMOS_RESOURCE res)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;
    bool hasAuxSurf = false;
    MhwCpBase* mhwCp = nullptr;

    CODECHAL_DECODE_CHK_NULL(res);
    CODECHAL_DECODE_CHK_NULL(res->pGmmResInfo);

    CODECHAL_DECODE_CHK_NULL(hwInterface);
    CODECHAL_DECODE_CHK_NULL(hwInterface->GetCpInterface());
    mhwCp = dynamic_cast<MhwCpBase *>(hwInterface->GetCpInterface());
    CODECHAL_DECODE_CHK_NULL(mhwCp);

    GMM_RESOURCE_FLAG gmmFlags;
    MOS_ZeroMemory(&gmmFlags, sizeof(gmmFlags));
    gmmFlags = res->pGmmResInfo->GetResFlags();
    hasAuxSurf = (gmmFlags.Gpu.MMC || gmmFlags.Info.MediaCompressed) && gmmFlags.Gpu.UnifiedAuxSurface;

    // Need to re-intialize surface aux data with these 2 conditions
    //
    // 1. The first time surface is ued
    //    a. If surface is created with HWProteced flag, it's CpTag session id is 15, will differ with current CpTag session bit (0~14)
    //    b. If surface is created without HWProtected flag, it's CpTag session id is 0, will differ with current session CpTag
    // 2. Teardown happens
    //    When teardown happens, instance id of CpTag of PAVP session will change
    //
    // Both the above conditions can be shorten by surface.CpTag != session.CpTag
    if (!hasAuxSurf || (res->pGmmResInfo->GetSetCpSurfTag(false, 0) == mhwCp->GetEncryptionParams()->CpTag))
    {
        return false;
    }

    CODECHAL_DECODE_VERBOSEMESSAGE("Aux data is invalid for CpSurfTag(%d), CpContext(%d)",
        res->pGmmResInfo->GetSetCpSurfTag(false, 0), mhwCp->GetEncryptionParams()->CpTag);

finish:
    CODECHAL_DECODE_VERBOSEMESSAGE("IsAuxDataInvalid return, Status(%d)", eStatus);
    return true;
}

void CodechalSecureDecode::EnableSampleGroupConstantIV()
{
    m_enableSampleGroupConstantIV = true;
}

MOS_STATUS CodechalSecureDecode::InitAuxSurface(
    PMOS_RESOURCE res,
    bool auxUV,
    bool isMmcEnabled)
{
    MOS_STATUS                          eStatus = MOS_STATUS_SUCCESS;
    bool                                hasAuxSurf = false;
    MhwCpBase*                          mhwCp = nullptr;
    PMOS_CMD_BUF_ATTRI_VE               pAttriVe           = nullptr;
    MhwMiInterface*                     pCommonMiInterface = nullptr;

    CODECHAL_DECODE_FUNCTION_ENTER;

    CODECHAL_DECODE_CHK_NULL_RETURN(res);
    CODECHAL_DECODE_CHK_NULL_RETURN(res->pGmmResInfo);
    CODECHAL_DECODE_CHK_NULL_RETURN(osInterface);

    CODECHAL_DECODE_CHK_NULL_RETURN(hwInterface);
    CODECHAL_DECODE_CHK_NULL_RETURN(hwInterface->GetCpInterface());
    mhwCp = dynamic_cast<MhwCpBase *>(hwInterface->GetCpInterface());
    CODECHAL_DECODE_CHK_NULL_RETURN(mhwCp);
    pCommonMiInterface = hwInterface->GetMiInterface();
    CODECHAL_DECODE_CHK_NULL_RETURN(pCommonMiInterface);
    m_mmcEnabled = isMmcEnabled;

    GMM_RESOURCE_FLAG gmmFlags;
    MOS_ZeroMemory(&gmmFlags, sizeof(gmmFlags));
    gmmFlags = res->pGmmResInfo->GetResFlags();
    hasAuxSurf = (gmmFlags.Gpu.MMC || gmmFlags.Info.MediaCompressed) && gmmFlags.Gpu.UnifiedAuxSurface;

    if (!hasAuxSurf)
    {
        return MOS_STATUS_SUCCESS;
    }

    uint32_t auxSurfSize = (uint32_t)res->pGmmResInfo->GetSizeAuxSurface(GMM_AUX_CCS);
    uint32_t auxSurfOffset = (uint32_t)res->pGmmResInfo->GetPlanarAuxOffset(0, GMM_AUX_Y_CCS);

    if (auxUV)
    {
        uint32_t auxSizeY;
        uint32_t auxSizeUV;

        bool isPlanar = (osInterface->pfnGetGmmClientContext(osInterface)->IsPlanar(res->pGmmResInfo->GetResourceFormat()) != 0);
        if (isPlanar)
        {
            auxSizeY = (uint32_t)(res->pGmmResInfo->GetPlanarAuxOffset(0, GMM_AUX_UV_CCS) -
                res->pGmmResInfo->GetPlanarAuxOffset(0, GMM_AUX_Y_CCS));
        }
        else
        {
            auxSizeY = (uint32_t)(res->pGmmResInfo->GetSizeAuxSurface(GMM_AUX_CCS));
        }

        auxSizeUV = (uint32_t)(res->pGmmResInfo->GetPlanarAuxOffset(0, GMM_AUX_COMP_STATE) -
            res->pGmmResInfo->GetPlanarAuxOffset(0, GMM_AUX_UV_CCS));

        if (auxSizeUV == 0)
        {
            auxSizeUV = (uint32_t)(res->pGmmResInfo->GetSizeAuxSurface(GMM_AUX_SURF)) /
                (res->pGmmResInfo)->GetArraySize() - auxSizeY;
        }

        auxSurfSize = auxSizeUV;
        auxSurfOffset = (uint32_t)res->pGmmResInfo->GetPlanarAuxOffset(0, GMM_AUX_UV_CCS);
    }

    MOS_ALLOC_GFXRES_PARAMS AllocParams;
    MOS_ZeroMemory(&AllocParams, sizeof(MOS_ALLOC_GFXRES_PARAMS));
    AllocParams.Type     = MOS_GFXRES_BUFFER;
    AllocParams.TileType = MOS_TILE_LINEAR;
    AllocParams.Format   = Format_Buffer;
    AllocParams.dwBytes  = auxSurfSize;
    AllocParams.pBufName = "AuxInitBuffer";
    if (Mos_ResourceIsNull(&auxBuf))
    {
        CODECHAL_DECODE_CHK_STATUS_MESSAGE_RETURN(osInterface->pfnAllocateResource(
            osInterface,
            &AllocParams,
            &auxBuf),
            "Failed to allocate aux surface initialization buffer.");
    }
    else if (auxBuf.pGmmResInfo->GetSizeSurface() < auxSurfSize)
    {
        osInterface->pfnFreeResource(osInterface, &auxBuf);
        CODECHAL_DECODE_CHK_STATUS_MESSAGE_RETURN(osInterface->pfnAllocateResource(
            osInterface,
            &AllocParams,
            &auxBuf),
            "Failed to allocate aux surface initialization buffer.");
    }

    MOS_COMMAND_BUFFER cmdBuffer;
    CODECHAL_DECODE_CHK_STATUS_RETURN(osInterface->pfnGetCommandBuffer(osInterface, &cmdBuffer, 0));

    // Send command buffer header at the beginning (OS dependent)
    MHW_GENERIC_PROLOG_PARAMS GenericPrologParams;
    MOS_ZeroMemory(&GenericPrologParams, sizeof(GenericPrologParams));
    GenericPrologParams.pOsInterface  = osInterface;
    GenericPrologParams.pvMiInterface = hwInterface->GetMiInterface();
    GenericPrologParams.bMmcEnabled   = m_mmcEnabled;
    CODECHAL_DECODE_CHK_STATUS_RETURN(Mhw_SendGenericPrologCmd(
        &cmdBuffer,
        &GenericPrologParams));

    MHW_ADD_CP_COPY_PARAMS          cpCopyParams;
    MOS_ZeroMemory(&cpCopyParams, sizeof(cpCopyParams));
    cpCopyParams.size          = auxSurfSize;
    cpCopyParams.presSrc       = &auxBuf;
    cpCopyParams.presDst       = res;
    cpCopyParams.offset        = auxSurfOffset;

    CODECHAL_DECODE_VERBOSEMESSAGE("Apply InitAuxSurface through Crypto Copy: size(%d), offset(%d)", cpCopyParams.size, cpCopyParams.offset);

    CODECHAL_DECODE_CHK_STATUS_RETURN(mhwCp->AddCpCopy(osInterface, &cmdBuffer, &cpCopyParams));

    MHW_MI_FLUSH_DW_PARAMS FlushDwParams;
    MOS_ZeroMemory(&FlushDwParams, sizeof(FlushDwParams));
    CODECHAL_DECODE_CHK_STATUS_RETURN(pCommonMiInterface->AddMiFlushDwCmd(
        &cmdBuffer,
        &FlushDwParams));

    CODECHAL_DECODE_CHK_STATUS_RETURN(pCommonMiInterface->AddMiBatchBufferEnd(&cmdBuffer, nullptr));
    osInterface->pfnReturnCommandBuffer(osInterface, &cmdBuffer, 0);

    if (MOS_VE_SUPPORTED(osInterface) && cmdBuffer.Attributes.pAttriVe)
    {
        pAttriVe                        = (PMOS_CMD_BUF_ATTRI_VE)cmdBuffer.Attributes.pAttriVe;
        pAttriVe->bUseVirtualEngineHint = true;
        MOS_ZeroMemory(&pAttriVe->VEngineHintParams, sizeof(pAttriVe->VEngineHintParams));
        pAttriVe->VEngineHintParams.NeedSyncWithPrevious = true;
    }

    CODECHAL_DECODE_CHK_STATUS_RETURN(osInterface->pfnSubmitCommandBuffer(osInterface, &cmdBuffer, 0));

    return eStatus;
}
