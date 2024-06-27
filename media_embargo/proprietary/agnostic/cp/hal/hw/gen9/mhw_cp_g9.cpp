/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2015-2021
Intel Corporation All Rights Reserved.

The source code contained or described herein and all documents related to the
source code ("Material") are owned by Intel Corporation or its suppliers or
licensors. Title to the Material remains with Intel Corporation or its suppliers
and licensors. The Material contains trade secrets and proprietary and confidential
information of Intel or its suppliers and licensors. The Material is protected by
worldwide copyright and trade secret laws and treaty provisions. No part of the
Material may be used, copied, reproduced, modified, published, uploaded, posted,
transmitted, distributed, or disclosed in any way without Intel's prior express
written permission.

No license under any patent, copyright, trade secret or other intellectual
property right is granted to or conferred upon you by disclosure or delivery
of the Materials, either expressly, by implication, inducement, estoppel
or otherwise. Any license under such intellectual property rights must be
express and approved by Intel in writing.

======================= end_copyright_notice ==================================*/
//!
//! \file     mhw_cp_g9.cpp
//! \brief    MHW interface for content protection
//! \details  Impelements the Gen-9 functionalities for content protection
//!

#include "mhw_cp_g9.h"
#include "mhw_mmio_ext.h"
#include "mhw_cp_g8.h"
#include "mhw_cp_hwcmd_common.h"
#include "mhw_mi_hwcmd_g9_X.h"
#include "mhw_vdbox.h"
#include "mhw_mi.h"
#include "mhw_mmio_g9_ext.h"
#include "igvpkrn_g9.h"
#include "igvpkrn_g9_cml.h"
#include "igvpkrn_g9_cml_tgp.h"
#include "igvpkrn_g9_cmpv.h"
#include "vpkrnheader.h"

MhwCpG9::MhwCpG9(MOS_STATUS& eStatus, MOS_INTERFACE& osInterface) : MhwCpBase(osInterface)
{
    eStatus = MOS_STATUS_SUCCESS;
    do
    {
        m_cpCmdPropsMap = &g_decryptBitOffsets_g9;
        m_counterList.clear();

        this->m_hwUnit = MOS_NewArray(CpHwUnit*, PAVP_HW_UNIT_MAX_COUNT);
        if (this->m_hwUnit == nullptr)
        {
            MOS_ASSERTMESSAGE(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW, "PAVP G9 HW unit pointer array allocation failed.");
            eStatus = MOS_STATUS_NO_SPACE;
            MT_ERR3(MT_CP_MHW_ALLOCATION_FAIL, MT_COMPONENT, MOS_COMPONENT_CP, MT_SUB_COMPONENT, MOS_CP_SUBCOMP_MHW,
                                                                                            MT_ERROR_CODE, eStatus);
            break;
        }

        this->m_hwUnit[PAVP_HW_UNIT_RCS] = MOS_New(CpHwUnitG9Rcs, eStatus, osInterface);
        if (eStatus != MOS_STATUS_SUCCESS)
        {
            break;
        }

        this->m_hwUnit[PAVP_HW_UNIT_VCS] = MOS_New(CpHwUnitG9Vcs, eStatus, osInterface);
        if (eStatus != MOS_STATUS_SUCCESS)
        {
            break;
        }

        this->m_hwUnit[PAVP_HW_UNIT_VECS] = MOS_New(CpHwUnitG9Vecs, eStatus, osInterface);
        if (eStatus != MOS_STATUS_SUCCESS)
        {
            break;
        }

        if ((this->m_hwUnit[PAVP_HW_UNIT_RCS] == nullptr) ||
            (this->m_hwUnit[PAVP_HW_UNIT_VCS] == nullptr) ||
            (this->m_hwUnit[PAVP_HW_UNIT_VECS] == nullptr))
        {
            MOS_ASSERTMESSAGE(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW, "PAVP G9 HW unit allocation failed.");
            eStatus = MOS_STATUS_NO_SPACE;
            MT_ERR3(MT_CP_MHW_ALLOCATION_FAIL, MT_COMPONENT, MOS_COMPONENT_CP, MT_SUB_COMPONENT, MOS_CP_SUBCOMP_MHW,
                                                                                            MT_ERROR_CODE, eStatus);
            break;
        }

        m_sizeOfCmdIndirectCryptoState = sizeof(HUC_INDIRECT_CRYPTO_STATE_G8LP);

    } while (false);
}

MhwCpG9::~MhwCpG9()
{
    m_counterList.clear();
}

CpHwUnitG9Rcs::CpHwUnitG9Rcs(MOS_STATUS& eStatus, MOS_INTERFACE& osInterface) : CpHwUnitRcs(eStatus, osInterface),
                                                                                m_ScratchResource(),
                                                                                m_pScratchBuffer(nullptr)
{
    MOS_STATUS              status                     = MOS_STATUS_UNKNOWN;
    MOS_ALLOC_GFXRES_PARAMS allocParamsForBufferLinear = {};
    MOS_LOCK_PARAMS         lockFlags                  = {};
    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL_NO_STATUS_RETURN(m_osInterface.pfnAllocateResource);
    MHW_CP_CHK_NULL_NO_STATUS_RETURN(m_osInterface.pfnLockResource);

    do
    {
        allocParamsForBufferLinear.Type     = MOS_GFXRES_BUFFER;
        allocParamsForBufferLinear.TileType = MOS_TILE_LINEAR;
        allocParamsForBufferLinear.Format   = Format_Buffer;
        allocParamsForBufferLinear.dwBytes  = MHW_CACHELINE_SIZE;
        allocParamsForBufferLinear.pBufName = "CpHwUnitG9Rcs_ScratchRes";
        status                              = m_osInterface.pfnAllocateResource(
            &m_osInterface,
            &allocParamsForBufferLinear,
            &m_ScratchResource);
        if (status != MOS_STATUS_SUCCESS)
        {
            break;
        }

        lockFlags.WriteOnly = 1;
        m_pScratchBuffer    = reinterpret_cast<uint32_t*>(m_osInterface.pfnLockResource(&m_osInterface, &m_ScratchResource, &lockFlags));
        MHW_CP_CHK_NULL_NO_STATUS_RETURN(m_pScratchBuffer);
#if (_DEBUG || _RELEASE_INTERNAL)
        (reinterpret_cast<uint32_t*>(m_pScratchBuffer))[1] = 0xFFFFFFFF;
        (reinterpret_cast<uint32_t*>(m_pScratchBuffer))[2] = 0xFFFFFFFF;
#endif

    } while (false);

    MHW_FUNCTION_EXIT;
    eStatus = status;
}

CpHwUnitG9Rcs::~CpHwUnitG9Rcs()
{
    MHW_CP_FUNCTION_ENTER;

    if (m_pScratchBuffer)
    {
        MHW_CP_CHK_NULL_NO_STATUS_RETURN(m_osInterface.pfnUnlockResource);
        m_osInterface.pfnUnlockResource(&m_osInterface, &m_ScratchResource);
        MHW_CP_CHK_NULL_NO_STATUS_RETURN(m_osInterface.pfnFreeResource);
        m_osInterface.pfnFreeResource(&m_osInterface, &m_ScratchResource);
    }
}

CpHwUnitG9Vecs::CpHwUnitG9Vecs(MOS_STATUS& eStatus, MOS_INTERFACE& osInterface) : CpHwUnitVecs(eStatus, osInterface),
                                                                                m_ScratchResource()
{
    MOS_STATUS              status                     = MOS_STATUS_UNKNOWN;
    MOS_ALLOC_GFXRES_PARAMS allocParamsForBufferLinear = {};

    MHW_CP_FUNCTION_ENTER;

    do
    {
        MHW_CP_CHK_NULL_NO_STATUS_RETURN(m_osInterface.pfnAllocateResource);
        allocParamsForBufferLinear.Type     = MOS_GFXRES_BUFFER;
        allocParamsForBufferLinear.TileType = MOS_TILE_LINEAR;
        allocParamsForBufferLinear.Format   = Format_Buffer;
        allocParamsForBufferLinear.dwBytes  = MOS_GPU_CONTEXT_MAX * sizeof(uint64_t);
        allocParamsForBufferLinear.pBufName = "CpHwUnitG9Vecs_ScratchRes";
        status                              = m_osInterface.pfnAllocateResource(
            &m_osInterface,
            &allocParamsForBufferLinear,
            &m_ScratchResource);
        if (status != MOS_STATUS_SUCCESS)
        {
            break;
        }
    } while (false);

    MHW_CP_FUNCTION_EXIT(status);
    eStatus = status;
}

CpHwUnitG9Vecs::~CpHwUnitG9Vecs()
{
    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL_NO_STATUS_RETURN(m_osInterface.pfnFreeResource);
    m_osInterface.pfnFreeResource(&m_osInterface, &m_ScratchResource);

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
}

MOS_STATUS CpHwUnitG9Vecs::AddCheckForEarlyExit(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    MHW_MI_CONDITIONAL_BATCH_BUFFER_END_PARAMS ConditionalBatchBufferEndCmd = {};
    MHW_MI_STORE_REGISTER_MEM_PARAMS           storeRegMemParams            = {};
    MHW_MI_FLUSH_DW_PARAMS                     flushDwParams                = {};
    MHW_MI_ATOMIC_PARAMS                       atomicParams                 = {};
    uint32_t                                   sessionId                    = MHW_INVALID_SESSION_ID;

    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(osInterface->pfnGetGpuContext);
    MHW_CP_CHK_NULL(osInterface->osCpInterface);

    MosCp *pMosCp = dynamic_cast<MosCp*>(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(pMosCp);

    MOS_GPU_CONTEXT mosGpuContext = osInterface->pfnGetGpuContext(osInterface);
    switch (mosGpuContext)
    {
    case MOS_GPU_CONTEXT_VEBOX:
        break;
    default:
        MHW_ASSERTMESSAGE("Unsupported GPU context %d", mosGpuContext);
        MT_ERR1(MT_MOS_GPUCXT_GET, MT_MOS_GPUCXT, mosGpuContext);
        break;
    }

    if (!osInterface->osCpInterface->IsCpEnabled() || !osInterface->osCpInterface->IsHMEnabled())
    {
        return MOS_STATUS_SUCCESS;
    }

    sessionId = pMosCp->GetSessionId();
    MHW_CP_CHK_COND(sessionId == MHW_INVALID_SESSION_ID, MOS_STATUS_INVALID_PARAMETER, "Invalid session ID");

    if (TRANSCODE_APP_ID_MASK & sessionId)
    {
        MOS_NORMALMESSAGE(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW, "Skip check early exit for transcode session, sessionId=[%d]", sessionId);
        return MOS_STATUS_SUCCESS;
    }

    MHW_VERBOSEMESSAGE("Using GPU Context id: %d", mosGpuContext);

    // Conditional exit #1:
    // Load VCR Debug Status 2 register into scratch memory
    storeRegMemParams.dwRegister      = MHW_PAVP_VCR_DEBUG_STATUS_2_REGISTER;
    storeRegMemParams.presStoreBuffer = &m_ScratchResource;
    storeRegMemParams.dwOffset        = sizeof(uint64_t) * mosGpuContext;  // m_ScratchResource[mosGpuContext]
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiStoreRegisterMemCmd(cmdBuffer, &storeRegMemParams));

    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiFlushDwCmd(
            cmdBuffer,
            &flushDwParams));

    // AND scratch memory with 0x40000000 back into scratch memory
    // MHW_PAVP_PLANE_ENABLE_MASKED_BIT_G9 = 0x40000000, which indicates whether
    // attacked and waiting for driver cleanup
    atomicParams.Operation         = MHW_MI_ATOMIC_AND;
    atomicParams.pOsResource       = &m_ScratchResource;
    atomicParams.dwDataSize        = sizeof(uint32_t);
    atomicParams.dwResourceOffset  = sizeof(uint64_t) * mosGpuContext;  // m_ScratchResource[mosGpuContext]
    atomicParams.bInlineData       = true;
    atomicParams.dwOperand1Data[0] = MHW_PAVP_PLANE_ENABLE_MASKED_BIT_G9;
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiAtomicCmd(cmdBuffer, &atomicParams));

    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiFlushDwCmd(
            cmdBuffer,
            &flushDwParams));


    // Conditionally exit if scratch memory is equal to 0x00000000
    // Use of conditional batch buffer function (which calls AddProlog) prevented adding
    // this whole function as part of AddProlog
    ConditionalBatchBufferEndCmd.presSemaphoreBuffer = &m_ScratchResource;
    ConditionalBatchBufferEndCmd.dwValue             = 0x00000000;
    ConditionalBatchBufferEndCmd.bDisableCompareMask = true;
    ConditionalBatchBufferEndCmd.dwOffset            = sizeof(uint64_t) * mosGpuContext;
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiConditionalBatchBufferEndCmd(
            cmdBuffer,
            &ConditionalBatchBufferEndCmd));

    // Conditional exit #2:
    // Load lite/heavy SessionInPlay register into scratch memory
    storeRegMemParams.dwRegister      = MHW_PAVP_SESSION_IN_PLAY_REGISTER;
    storeRegMemParams.presStoreBuffer = &m_ScratchResource;
    storeRegMemParams.dwOffset        = sizeof(uint64_t) * mosGpuContext;
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiStoreRegisterMemCmd(
            cmdBuffer,
            &storeRegMemParams));

    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiFlushDwCmd(
            cmdBuffer,
            &flushDwParams));

    // AND scratch memory with session ID bit  back into scratch memory
    atomicParams.Operation         = MHW_MI_ATOMIC_AND;
    atomicParams.pOsResource       = &m_ScratchResource;
    atomicParams.dwDataSize        = sizeof(uint32_t);
    atomicParams.bInlineData       = true;
    atomicParams.dwResourceOffset  = sizeof(uint64_t) * mosGpuContext;  // m_ScratchResource[mosGpuContext]
    atomicParams.dwOperand1Data[0] = 1 << sessionId;
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiAtomicCmd(
            cmdBuffer,
            &atomicParams));

    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiFlushDwCmd(
            cmdBuffer,
            &flushDwParams));

    // Conditionally exit if scratch memory is equal to 0x00000000
    // Use of conditional batch buffer function (which calls AddProlog) prevented adding
    // this whole function as part of AddProlog
    ConditionalBatchBufferEndCmd.presSemaphoreBuffer = &m_ScratchResource;
    ConditionalBatchBufferEndCmd.dwValue             = 0x00000000;
    ConditionalBatchBufferEndCmd.bDisableCompareMask = true;
    ConditionalBatchBufferEndCmd.dwOffset            = sizeof(uint64_t) * mosGpuContext;
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiConditionalBatchBufferEndCmd(
            cmdBuffer,
            &ConditionalBatchBufferEndCmd));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);

    return MOS_STATUS_SUCCESS;
}

CpHwUnitG9Vcs::CpHwUnitG9Vcs(MOS_STATUS &eStatus, MOS_INTERFACE &osInterface) : CpHwUnitVcs(eStatus, osInterface),
                                                                                m_ScratchResource()
{
    MOS_STATUS              status                     = MOS_STATUS_UNKNOWN;
    MOS_ALLOC_GFXRES_PARAMS allocParamsForBufferLinear = {};
    MOS_LOCK_PARAMS         lockFlags                  = {};

    MHW_CP_FUNCTION_ENTER;

    do
    {
        MHW_CP_CHK_NULL_NO_STATUS_RETURN(m_osInterface.pfnAllocateResource);
        allocParamsForBufferLinear.Type     = MOS_GFXRES_BUFFER;
        allocParamsForBufferLinear.TileType = MOS_TILE_LINEAR;
        allocParamsForBufferLinear.Format   = Format_Buffer;
        allocParamsForBufferLinear.dwBytes  = MOS_GPU_CONTEXT_MAX * sizeof(uint64_t);
        allocParamsForBufferLinear.pBufName = "CpHwUnitG9Vcs_ScratchRes";
        status                              = m_osInterface.pfnAllocateResource(
            &m_osInterface,
            &allocParamsForBufferLinear,
            &m_ScratchResource);
        if (status != MOS_STATUS_SUCCESS)
        {
            break;
        }
    } while (false);

    MHW_CP_FUNCTION_EXIT(status);
    eStatus = status;
}

CpHwUnitG9Vcs::~CpHwUnitG9Vcs()
{
    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL_NO_STATUS_RETURN(m_osInterface.pfnFreeResource);
    m_osInterface.pfnFreeResource(&m_osInterface, &m_ScratchResource);

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
}

MOS_STATUS MhwCpG9::AddMfxAesState(
    bool                 isDecodeInUse,
    PMOS_COMMAND_BUFFER  cmdBuffer,
    PMHW_BATCH_BUFFER    batchBuffer,
    PMHW_PAVP_AES_PARAMS params)
{
    MHW_CP_FUNCTION_ENTER;
    // One of the pointers should be valid
    if (cmdBuffer == nullptr && batchBuffer == nullptr)
    {
        MHW_ASSERTMESSAGE("There was no valid buffer to add the HW command to.");
        MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, MOS_STATUS_NULL_POINTER);
        return MOS_STATUS_NULL_POINTER;
    }

    // Get counter value from encryption params before counter increment
    CounterArray counter = {0};
    auto encryptionParams = this->GetEncryptionParams();
    MHW_CP_CHK_STATUS_MESSAGE(
        MOS_SecureMemcpy(
            counter.data(),
            CODEC_DWORDS_PER_AES_BLOCK * sizeof(uint32_t),
            encryptionParams->dwPavpAesCounter,
            CODEC_DWORDS_PER_AES_BLOCK * sizeof(uint32_t)),
        "Failed to copy memory.");

    if(params->bLastPass)
    {
        m_counterList.push_back(counter);
    }

    MHW_CP_CHK_STATUS(AddMfxAesStateG9(this, &this->m_osInterface, isDecodeInUse, cmdBuffer, batchBuffer, params));
    // This wait cmd is needed to make sure pavp command is done before decode starts
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMfxWaitCmd(cmdBuffer, batchBuffer, false));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpG9::AddMfxCryptoCopyBaseAddr(
    PMOS_COMMAND_BUFFER               cmdBuffer,
    PMHW_PAVP_CRYPTO_COPY_ADDR_PARAMS params)
{
    return MhwCpG8::AddMfxCryptoCopyBaseAddrG8(this, &this->m_osInterface, cmdBuffer, params);
}

MOS_STATUS MhwCpG9::AddMfxCryptoKeyExchange(
    PMOS_COMMAND_BUFFER                          cmdBuffer,
    PMHW_PAVP_MFX_CRYPTO_KEY_EXCHANGE_CMD_PARAMS params)
{
    return MhwCpBase::AddMfxCryptoKeyExchange(cmdBuffer, params);
}

MOS_STATUS MhwCpG9::AddMfxCryptoInlineStatusRead(
    PMOS_INTERFACE                                     osInterface,
    PMOS_COMMAND_BUFFER                                cmdBuffer,
    PMHW_PAVP_MFX_CRYPTO_INLINE_STATUS_READ_CMD_PARAMS params)
{
    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(cmdBuffer);
    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(osInterface->osCpInterface);

    auto hwUnit = MhwCpBase::getHwUnit(osInterface);
    MHW_CP_CHK_NULL(hwUnit);
    MHW_CP_CHK_STATUS(hwUnit->AddMfxCryptoInlineStatusRead(osInterface, cmdBuffer, params));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpG9::AddMfxCryptoCopy(
    PMOS_COMMAND_BUFFER          cmdBuffer,
    PMHW_PAVP_CRYPTO_COPY_PARAMS params)
{
    return MhwCpG8::AddMfxCryptoCopyG8(this, cmdBuffer, params);
}

void MhwCpG9::GetCpStateLevelCmdSize(uint32_t& cmdSize, uint32_t& patchListSize)
{
    MHW_CP_FUNCTION_ENTER;

    uint32_t prologCmdSize = mhw_mi_g9_X::MI_FLUSH_DW_CMD::byteSize + sizeof(MI_SET_APP_ID_CMD);
    uint32_t epilogCmdSize = mhw_mi_g9_X::MI_FLUSH_DW_CMD::byteSize;

    cmdSize =
        prologCmdSize +
        MOS_MAX(
        (
            sizeof(HCP_AES_STATE_CMD) +
            mhw_mi_g9_X::MFX_WAIT_CMD::byteSize +
            sizeof(MFX_CRYPTO_COPY_BASE_ADDR_CMD_G8) +
            sizeof(MFX_CRYPTO_COPY_CMD_G8)),
            (sizeof(HUC_AES_IND_STATE_CMD))) +
        epilogCmdSize;

    patchListSize =
        PATCH_LIST_COMMAND(MI_FLUSH_DW_CMD) +
        PATCH_LIST_COMMAND(MI_SET_APP_ID_CMD) +
        PATCH_LIST_COMMAND(HCP_AES_STATE_CMD) +
        PATCH_LIST_COMMAND(MFX_WAIT_CMD) +
        PATCH_LIST_COMMAND(MFX_CRYPTO_COPY_BASE_ADDR_CMD) +
        PATCH_LIST_COMMAND(MFX_CRYPTO_COPY_CMD) +
        PATCH_LIST_COMMAND(HUC_AES_IND_STATE_CMD) +
        PATCH_LIST_COMMAND(MI_FLUSH_DW_CMD);

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
}

void MhwCpG9::GetCpSliceLevelCmdSize(uint32_t& cmdSize, uint32_t& patchListSize)
{
    MHW_CP_FUNCTION_ENTER;

    uint32_t cmdSizeForMfx = sizeof(MFX_AES_STATE_CMD_G8) + mhw_mi_g9_X::MFX_WAIT_CMD::byteSize;
    uint32_t cmdSizeForHcp = sizeof(HCP_AES_STATE_CMD) + mhw_mi_g9_X::MFX_WAIT_CMD::byteSize + sizeof(HCP_AES_IND_STATE_CMD);
    uint32_t cmdSizeForHuc = sizeof(HUC_AES_STATE_CMD) + sizeof(HUC_AES_IND_STATE_CMD);

    uint32_t patchLSizeForMfx = PATCH_LIST_COMMAND(MFX_AES_STATE_CMD) + PATCH_LIST_COMMAND(MFX_WAIT_CMD);
    uint32_t patchLSizeForHcp = PATCH_LIST_COMMAND(HCP_AES_STATE_CMD) + PATCH_LIST_COMMAND(HCP_AES_IND_STATE_CMD);
    uint32_t patchLSizeForHuc = PATCH_LIST_COMMAND(HUC_AES_STATE_CMD) + PATCH_LIST_COMMAND(HUC_AES_IND_STATE_CMD);

    cmdSize = MOS_MAX((cmdSizeForMfx), MOS_MAX((cmdSizeForHcp), (cmdSizeForHuc)));
    patchListSize = MOS_MAX((patchLSizeForMfx), MOS_MAX((patchLSizeForHcp), (patchLSizeForHuc)));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
}

bool MhwCpG9::IsHwCounterIncrement(PMOS_INTERFACE osInterface)
{
    MHW_CP_FUNCTION_ENTER;
    bool bAutoIncSupport = false;
    MHW_CP_ASSERT(osInterface);
    MHW_CP_ASSERT(osInterface->osCpInterface);
    MEDIA_FEATURE_TABLE *pSkuTable = osInterface->pfnGetSkuTable(osInterface);
    MHW_CP_ASSERT(pSkuTable);

    if (osInterface->osCpInterface->IsHMEnabled()
        && !osInterface->osCpInterface->IsTSEnabled()
        && MEDIA_IS_SKU(pSkuTable, FtrHWCounterAutoIncrementSupport))
    {
        bAutoIncSupport = true;
        MOS_NORMALMESSAGE(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW, "HWCounterAutoIncrementEnforced is Enabled.");
    }

    MHW_CP_FUNCTION_EXIT(true);
    return bAutoIncSupport;
}

MOS_STATUS MhwCpG9::GetCounterValue(uint32_t* ctr)
{
    MHW_CP_FUNCTION_ENTER;
    MHW_CP_CHK_NULL(ctr);

    if(!m_counterList.empty())
    {
        MHW_CP_CHK_STATUS_MESSAGE(
            MOS_SecureMemcpy(
                ctr,
                CODEC_DWORDS_PER_AES_BLOCK * sizeof(uint32_t),
                (void*)(m_counterList.front().data()),
                CODEC_DWORDS_PER_AES_BLOCK * sizeof(uint32_t)),
            "Failed to copy memory.");
        m_counterList.pop_front();
    }
    else
    {
        MOS_ASSERTMESSAGE(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW, "MhwCpG9::GetCounterValue failed with empty m_counterList");
        MT_ERR3(MT_CP_STATUS_UNINITIALIZED, MT_COMPONENT, MOS_COMPONENT_CP, MT_SUB_COMPONENT, MOS_CP_SUBCOMP_MHW, 
                                                                        MT_ERROR_CODE, MOS_STATUS_UNINITIALIZED);
        return MOS_STATUS_UNINITIALIZED;
    }
    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpG9::GetTranscryptKernelInfo(
    const uint32_t **transcryptMetaData,
    uint32_t     *transcryptMetaDataSize,
    const uint32_t                  **encryptedKernels,
    uint32_t                        *encryptedKernelsSize,
    MHW_PAVP_KERNEL_ENCRYPTION_TYPE encryptionType)
{
    MHW_CP_CHK_NULL(transcryptMetaData);
    MHW_CP_CHK_NULL(transcryptMetaDataSize);
    MHW_CP_CHK_NULL(encryptedKernels);
    MHW_CP_CHK_NULL(encryptedKernelsSize);

    MOS_STATUS status = MOS_STATUS_PLATFORM_NOT_SUPPORTED;
    PLATFORM platform;
    m_osInterface.pfnGetPlatform(&m_osInterface, &platform);

    switch (encryptionType)
    {

    case MHW_PAVP_KERNEL_ENCRYPTION_PRODUCTION:


        if (platform.ePCHProductFamily == PCH_CMP_LP ||
            platform.ePCHProductFamily == PCH_CMP_H)
        {
            *transcryptMetaData = IGVPKRN_G9_CML + IDR_VP_TOTAL_NUM_KERNELS + 1 + IGVPKRN_G9_CML[IDR_VP_zzz_production] / sizeof(uint32_t);
            *transcryptMetaDataSize = IGVPKRN_G9_CML[IDR_VP_zzz_production + 1] - IGVPKRN_G9_CML[IDR_VP_zzz_production];
            *encryptedKernels = IGVPKRN_G9_CML + IDR_VP_TOTAL_NUM_KERNELS + 1 + IGVPKRN_G9_CML[IDR_VP_zzz_production + 1] / sizeof(uint32_t);
            *encryptedKernelsSize = IGVPKRN_G9_CML[IDR_VP_TOTAL_NUM_KERNELS] - IGVPKRN_G9_CML[IDR_VP_zzz_production + 1];
            status = MOS_STATUS_SUCCESS;
        }
        else if (platform.ePCHProductFamily == PCH_CMP_V)
        {
            *transcryptMetaData = IGVPKRN_G9_CMPV + IDR_VP_TOTAL_NUM_KERNELS + 1 + IGVPKRN_G9_CMPV[IDR_VP_zzz_production] / sizeof(uint32_t);
            *transcryptMetaDataSize = IGVPKRN_G9_CMPV[IDR_VP_zzz_production + 1] - IGVPKRN_G9_CMPV[IDR_VP_zzz_production];
            *encryptedKernels = IGVPKRN_G9_CMPV + IDR_VP_TOTAL_NUM_KERNELS + 1 + IGVPKRN_G9_CMPV[IDR_VP_zzz_production + 1] / sizeof(uint32_t);
            *encryptedKernelsSize = IGVPKRN_G9_CMPV[IDR_VP_TOTAL_NUM_KERNELS] - IGVPKRN_G9_CMPV[IDR_VP_zzz_production + 1];
            status = MOS_STATUS_SUCCESS;
        }
        else if (platform.ePCHProductFamily == PCH_TGL_LP || 
            platform.ePCHProductFamily == PCH_TGL_H)
        {
            *transcryptMetaData     = IGVPKRN_G9_CML_TGP + IDR_VP_TOTAL_NUM_KERNELS + 1 + IGVPKRN_G9_CML_TGP[IDR_VP_zzz_production] / sizeof(uint32_t);
            *transcryptMetaDataSize = IGVPKRN_G9_CML_TGP[IDR_VP_zzz_production + 1] - IGVPKRN_G9_CML_TGP[IDR_VP_zzz_production];
            *encryptedKernels       = IGVPKRN_G9_CML_TGP + IDR_VP_TOTAL_NUM_KERNELS + 1 + IGVPKRN_G9_CML_TGP[IDR_VP_zzz_production + 1] / sizeof(uint32_t);
            *encryptedKernelsSize   = IGVPKRN_G9_CML_TGP[IDR_VP_TOTAL_NUM_KERNELS] - IGVPKRN_G9_CML_TGP[IDR_VP_zzz_production + 1];
            status = MOS_STATUS_SUCCESS;
        }
        else
        {
            *transcryptMetaData = IGVPKRN_G9 + IDR_VP_TOTAL_NUM_KERNELS + 1 + IGVPKRN_G9[IDR_VP_zzz_production] / sizeof(uint32_t);
            *transcryptMetaDataSize = IGVPKRN_G9[IDR_VP_zzz_production + 1] - IGVPKRN_G9[IDR_VP_zzz_production];
            *encryptedKernels = IGVPKRN_G9 + IDR_VP_TOTAL_NUM_KERNELS + 1 + IGVPKRN_G9[IDR_VP_zzz_production + 1] / sizeof(uint32_t);
            *encryptedKernelsSize = IGVPKRN_G9[IDR_VP_TOTAL_NUM_KERNELS] - IGVPKRN_G9[IDR_VP_zzz_production + 1];
            status = MOS_STATUS_SUCCESS;
        }
        break;

    case MHW_PAVP_KERNEL_ENCRYPTION_PREPRODUCTION:
        *transcryptMetaData     = IGVPKRN_G9 + IDR_VP_TOTAL_NUM_KERNELS + 1 + IGVPKRN_G9[IDR_VP_zzz_preproduction] / sizeof(uint32_t);
        *transcryptMetaDataSize = IGVPKRN_G9[IDR_VP_zzz_preproduction + 1] - IGVPKRN_G9[IDR_VP_zzz_preproduction];
        *encryptedKernels       = IGVPKRN_G9 + IDR_VP_TOTAL_NUM_KERNELS + 1 + IGVPKRN_G9[IDR_VP_zzz_preproduction + 1] / sizeof(uint32_t);
        *encryptedKernelsSize   = IGVPKRN_G9[IDR_VP_zzz_production] - IGVPKRN_G9[IDR_VP_zzz_preproduction + 1];
        status = MOS_STATUS_SUCCESS;
        break;

    default:
        *transcryptMetaData     = nullptr;
        *transcryptMetaDataSize = 0;
        *encryptedKernels       = nullptr;
        *encryptedKernelsSize   = 0;
        status = MOS_STATUS_PLATFORM_NOT_SUPPORTED;
        break;
    }

    return status;
}

// ----- static functions shared by several Gens. ----------
MOS_STATUS MhwCpG9::AddMfxAesStateG9(
    MhwCpBase*      mhwCp,
    PMOS_INTERFACE       osInterface,
    bool                 isDecodeInUse,
    PMOS_COMMAND_BUFFER  cmdBuffer,
    PMHW_BATCH_BUFFER    batchBuffer,
    PMHW_PAVP_AES_PARAMS params)
{
    uint32_t        counterIncrement;
    uint32_t        aesCtr[4] = {0};
    MOS_LOCK_PARAMS lockFlags;

    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(mhwCp);
    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(mhwCp->m_mhwMiInterface);
    MHW_CP_CHK_NULL(params);

    auto miInterface      = mhwCp->m_mhwMiInterface;
    auto encryptionParams = mhwCp->GetEncryptionParams();
    auto cmd              = g_cInit_MFX_AES_STATE_CMD_G9;

    MHW_CP_CHK_STATUS_MESSAGE(MOS_SecureMemcpy(
                                  aesCtr,
                                  CODEC_DWORDS_PER_AES_BLOCK * sizeof(uint32_t),
                                  encryptionParams->dwPavpAesCounter,
                                  CODEC_DWORDS_PER_AES_BLOCK * sizeof(uint32_t)),
        "Failed to copy memory.");

    if (encryptionParams->PavpEncryptionType == CP_SECURITY_DECE_AES128_CTR)
    {
        // DECE is relevant for AVC only, the start code is always '0 0 1' .

        if (encryptionParams->pSubSampleMappingBlock != nullptr)
        {
            PMHW_SUB_SAMPLE_MAPPING_BLOCK pSubSampleBlocks = reinterpret_cast<PMHW_SUB_SAMPLE_MAPPING_BLOCK>(encryptionParams->pSubSampleMappingBlock);
            PMHW_SUB_SAMPLE_MAPPING_BLOCK pCurrentBlock = &(pSubSampleBlocks[params->dwSliceIndex]);

            cmd.DW7.InitPacketBytesLength = pCurrentBlock->ClearSize;
            cmd.DW8.EncryptedBytesLength = pCurrentBlock->EncryptedSize;
            cmd.DW9.NumberByteInputBitstream = pCurrentBlock->ClearSize;
            cmd.DW8.LastPartialBlockAdj = 1;

            uint32_t encryptedSize = 0;
            for(uint32_t i = 0; i < params->dwSliceIndex; i++)
            {
                encryptedSize += pSubSampleBlocks[i].EncryptedSize;
            }
            counterIncrement = encryptedSize >> 4;
        }
        else
        {
            // Data offset is coming from the application.
            // For each slice we need to calculate the offset based on the number of Bytes in previous slices.
            uint32_t dwZeroPadding = params->dwDataStartOffset - params->dwTotalBytesConsumed;

            // Clear bytes (which are the slice header and part of the bit stream) contains also the start code (0 0 1).
            // According to the DECE spec: Clear Bytes + zero padding = 128 exactly.
            uint32_t dwNumClearBytes = params->dwDefaultDeceInitPacketLengthInBytes - dwZeroPadding;

            if (params->dwDataLength <= params->dwDefaultDeceInitPacketLengthInBytes)
            {
                //Reset Counter to ZERO
                aesCtr[0] = aesCtr[1] = aesCtr[2] = aesCtr[3] = 0;

                //No increment as well
                counterIncrement              = 0;
                cmd.DW7.InitPacketBytesLength = params->dwDefaultDeceInitPacketLengthInBytes;
                cmd.DW8.EncryptedBytesLength  = 0;
            }
            else
            {
                //CounterIncremnet = Total 16 Byte Blocks consumed before this slice - Totals 16 bytes block of clear data before this slice
                //                 =  (TolalBytesConsumedBeforeThisSlice / 16) - ( 8 * SlicesProcessedSofar); 128 = 16 * 8
                counterIncrement = (params->dwTotalBytesConsumed >> 4) - (params->dwSliceIndex << 3);

                // InitPacketBytesLength is the amount of bytes from the beginning of the bit stream until the first encrypted byte NOT incuding the start code.
                cmd.DW7.InitPacketBytesLength = dwNumClearBytes - MOS_NAL_UNIT_STARTCODE_LENGTH;

                // Number of encrypted bytes.
                cmd.DW8.EncryptedBytesLength = params->dwDataLength - dwNumClearBytes;
            }
        }
    }
    else if (encryptionParams->PavpEncryptionType == CP_SECURITY_AES128_CBC)
    {
        MHW_CP_CHK_NULL(params->presDataBuffer);

        uint32_t *pdwLastEncryptedOword = nullptr;

        MOS_ZeroMemory(&lockFlags, sizeof(MOS_LOCK_PARAMS));
        lockFlags.ReadOnly          = 1;
        auto pdwFirstEncryptedOword = static_cast<uint32_t*>(osInterface->pfnLockResource(
            osInterface,
            params->presDataBuffer,
            &lockFlags));
        MHW_CP_CHK_NULL(pdwFirstEncryptedOword);

        pdwLastEncryptedOword =
            pdwFirstEncryptedOword +
            ((params->dwDataStartOffset >> 4) * sizeof(uint32_t));

        if (pdwLastEncryptedOword != pdwFirstEncryptedOword)
        {
            pdwLastEncryptedOword -= sizeof(uint32_t);
            aesCtr[0] = pdwLastEncryptedOword[0];
            aesCtr[1] = pdwLastEncryptedOword[1];
            aesCtr[2] = pdwLastEncryptedOword[2];
            aesCtr[3] = pdwLastEncryptedOword[3];
        }

        osInterface->pfnUnlockResource(osInterface, params->presDataBuffer);

        counterIncrement = 0;
    }
    else
    {
        counterIncrement = params->dwDataStartOffset >> 4;
    }

    // Decode increments the counter before adding the AES state to the command buffer
    if (isDecodeInUse)
    {
        mhwCp->IncrementCounter(
            aesCtr,
            counterIncrement);
    }

    cmd.DW4.InitializationValue3 = aesCtr[3];
    cmd.DW3.InitializationValue2 = aesCtr[2];
    cmd.DW2.InitializationValue1 = aesCtr[1];
    cmd.DW1.InitializationValue0 = aesCtr[0];

    // Encode increments the counter after adding the AES state to the command buffer
    // and the counter is incremented for last pass only
    if (!isDecodeInUse && params->bLastPass)
    {
        mhwCp->IncrementCounter(
            encryptionParams->dwPavpAesCounter,
            encryptionParams->uiCounterIncrement);
    }

    if (cmdBuffer == nullptr && batchBuffer == nullptr)
    {
        MHW_ASSERTMESSAGE("There was no valid buffer to add the HW command to.");
        MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, MOS_STATUS_NO_SPACE);
        return MOS_STATUS_NO_SPACE;
    }

    MHW_CP_CHK_STATUS(Mhw_AddCommandCmdOrBB(osInterface, cmdBuffer, batchBuffer, &cmd, sizeof(cmd)));

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpG9::ReadEncodeCounterFromHW(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer,
    PMOS_RESOURCE       resource,
    uint16_t            currentIndex)
{
    return MhwCpBase::ReadEncodeCounterFromHW(
        osInterface,
        cmdBuffer,
        resource,
        currentIndex);
}

MOS_STATUS MhwCpG9::AddProlog(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(cmdBuffer);

    MosCp *pMosCp = dynamic_cast<MosCp *>(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(pMosCp);

    MHW_PAVP_PROLOG_TRACE_DATA eventData =
        {
            osInterface->pfnGetGpuContext(osInterface),
            osInterface->osCpInterface->IsCpEnabled(),
            m_encryptionParams.CpTag,
        };
    MOS_TraceEventExt(EVENT_MHW_PROLOG, EVENT_TYPE_INFO, &cmdBuffer->pCmdBase, sizeof(uint32_t), &eventData, sizeof(eventData));

    if (osInterface->osCpInterface->IsCpEnabled() == false)
    {
        // the operation is not needed
        return MOS_STATUS_SUCCESS;
    }

    auto hwUnit = MhwCpBase::getHwUnit(osInterface);
    MHW_CP_CHK_NULL(hwUnit);

    MHW_CP_CHK_STATUS(hwUnit->SaveEncryptionParams(&m_encryptionParams));
    MHW_CP_CHK_STATUS(hwUnit->AddProlog(osInterface, cmdBuffer));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

// ----------  Add Prolog ----------
MOS_STATUS CpHwUnitG9Rcs::AddProlog(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    // Render
    PIPE_CONTROL_PAVP*      pipeControl = nullptr;
    MHW_PIPE_CONTROL_PARAMS pipeControlParams;
    PIPELINE_SELECT_CMD_G9  pipelineSelectCmd;

    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(cmdBuffer);
    MHW_CP_CHK_NULL(this->m_mhwMiInterface);

    MosCp *pMosCp = dynamic_cast<MosCp*>(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(pMosCp);

    // Gen9 and Gen10 have a concept of individual power wells.
    // i.e. Render engine can go to power well while Media is functional and visa versa.
    // Our Crypto Engine is behind Media so when Render is using PAVP,
    // Media has to be awake so that Crypto Engine can do serpent decryption.
    // So need two additional PIPELINE_SELECT to wrap the MI_SET_APPID and PIPE_CONTROL
    // For Gen11, the two PIPELINE_SELECT are not needed.
    pipelineSelectCmd.DW0.ForceMediaAwake = 0x1;
    pipelineSelectCmd.DW0.MaskBits        = 0x20;

    // Add the 1st PIPELINE_SELECT to force Media awake
    MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, &pipelineSelectCmd, sizeof(pipelineSelectCmd)));

    // ---------------- Wrapped by PIPELINE_SELECT ----------------
    MOS_ZeroMemory(&pipeControlParams, sizeof(pipeControlParams));
    pipeControlParams.dwFlushMode = MHW_FLUSH_WRITE_CACHE;

    // get the address of the PIPE_CONTROL going to be added
    pipeControl = reinterpret_cast<PIPE_CONTROL_PAVP*>(cmdBuffer->pCmdPtr + 1);  // DW1

    // actually add the PIPE_CONTROL
    MHW_CP_CHK_STATUS(this->m_mhwMiInterface->AddPipeControl(cmdBuffer, nullptr, &pipeControlParams));

    // patch the ProtectedMemoryEnable bit
    pipeControl->ProtectedMemoryEnable = osInterface->osCpInterface->IsHMEnabled();

    // Send MI_SET_APPID
    MHW_CP_CHK_STATUS(AddMiSetAppId(osInterface, cmdBuffer, pMosCp->GetSessionId()));
    // ---------------- Wrapped by PIPELINE_SELECT ----------------

    // Add the 2nd PIPELINE_SELECT to disable force Media awake
    pipelineSelectCmd.DW0.ForceMediaAwake = 0x0;
    MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, &pipelineSelectCmd, sizeof(pipelineSelectCmd)));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

// ---------- Add Check For Early Exit ----------
MOS_STATUS CpHwUnitG9Rcs::AddCheckForEarlyExit(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    // Render
    uint32_t                         registerGpr0Lo        = MHW_REGISTER_INVALID;
    MHW_MI_STORE_REGISTER_MEM_PARAMS storeRegMemParams     = {};
    MHW_MI_FLUSH_DW_PARAMS           flushDwParams         = {};
    MHW_MI_LOAD_REGISTER_IMM_PARAMS  registerImmParams     = {};
    MHW_MI_ATOMIC_PARAMS             atomicParams          = {};
    MHW_MI_LOAD_REGISTER_REG_PARAMS  loadRegisterRegParams = {};
#if (_DEBUG || _RELEASE_INTERNAL)
    MHW_MI_COPY_MEM_MEM_PARAMS copyMemMemParams = {};
#endif
    MHW_MI_CONDITIONAL_BATCH_BUFFER_END_PARAMS conditionalBatchBufferEndParams = {};
    uint32_t                                   *transcryptedKernel              = nullptr;
    uint32_t                                   transcryptedKernelSize          = 0;
    uint32_t                                   sessionId                       = MHW_INVALID_SESSION_ID;

    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(osInterface->pfnGetGpuContext);
    MHW_CP_CHK_NULL(osInterface->osCpInterface);

    MosCp *pMosCp = dynamic_cast<MosCp*>(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(pMosCp);

    switch (osInterface->pfnGetGpuContext(osInterface))
    {
    case MOS_GPU_CONTEXT_RENDER:
    case MOS_GPU_CONTEXT_RENDER2:
    case MOS_GPU_CONTEXT_RENDER3:
    case MOS_GPU_CONTEXT_RENDER4:
        registerGpr0Lo = MHW_RCS_GENERAL_PURPOSE_REGISTER_LO(0);
        break;
    default:
        MHW_ASSERTMESSAGE("Unsupported GPU context %d",
            osInterface->pfnGetGpuContext(osInterface));
        MT_ERR1(MT_MOS_GPUCXT_GET, MT_MOS_GPUCXT, osInterface->pfnGetGpuContext(osInterface));
        break;
    }

    //Output above logs instead of exit to avoid block issue
    if (registerGpr0Lo == MHW_REGISTER_INVALID)
    {
        return MOS_STATUS_SUCCESS;
    }

    if (!osInterface->osCpInterface->IsCpEnabled() ||
        !osInterface->osCpInterface->IsSMEnabled())
    {
        return MOS_STATUS_SUCCESS;
    }

    MHW_CP_CHK_STATUS(osInterface->osCpInterface->GetTK(
        &transcryptedKernel, &transcryptedKernelSize, nullptr));
    if (!transcryptedKernel)
    {
        return MOS_STATUS_SUCCESS;
    }

    sessionId = pMosCp->GetSessionId();
    MHW_CP_CHK_COND(sessionId == MHW_INVALID_SESSION_ID, MOS_STATUS_INVALID_PARAMETER, "Invalid session ID");

    // Conditional exit #1:
    // Load VCR Debug Status 2 register into scratch memory
    storeRegMemParams.dwRegister      = MHW_PAVP_VCR_DEBUG_STATUS_2_REGISTER;
    storeRegMemParams.presStoreBuffer = &m_ScratchResource;
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiStoreRegisterMemCmd(
            cmdBuffer,
            &storeRegMemParams));

    // AND scratch memory with 0x40000000 (in GPR0Lo) back into scratch memory
    // MHW_PAVP_PLANE_ENABLE_MASKED_BIT_G9 = 0x40000000, which indicates whether
    // attacked and waiting for driver cleanup
    registerImmParams.dwData     = MHW_PAVP_PLANE_ENABLE_MASKED_BIT_G9;
    registerImmParams.dwRegister = registerGpr0Lo;
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiLoadRegisterImmCmd(
            cmdBuffer,
            &registerImmParams));
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiFlushDwCmd(
            cmdBuffer,
            &flushDwParams));
    atomicParams.Operation   = MHW_MI_ATOMIC_AND;
    atomicParams.pOsResource = &m_ScratchResource;
    atomicParams.dwDataSize  = sizeof(uint32_t);
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiAtomicCmd(
            cmdBuffer,
            &atomicParams));

#if (_DEBUG || _RELEASE_INTERNAL)
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiFlushDwCmd(
            cmdBuffer,
            &flushDwParams));

    // Store result in scratch memory plus one
    copyMemMemParams.presSrc     = &m_ScratchResource;
    copyMemMemParams.dwSrcOffset = 0;
    copyMemMemParams.presDst     = &m_ScratchResource;
    copyMemMemParams.dwDstOffset = sizeof(uint32_t);
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiCopyMemMemCmd(
            cmdBuffer,
            &copyMemMemParams));
#endif

    // Conditionally exit if scratch memory is equal to 0x00000000
    // Use of conditional batch buffer function (which calls AddProlog) prevented adding
    // this whole function as part of AddProlog
    conditionalBatchBufferEndParams.presSemaphoreBuffer = &m_ScratchResource;
    conditionalBatchBufferEndParams.dwValue             = 0x00000000;
    conditionalBatchBufferEndParams.bDisableCompareMask = true;
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiConditionalBatchBufferEndCmd(
            cmdBuffer,
            &conditionalBatchBufferEndParams));

#if (_DEBUG || _RELEASE_INTERNAL)
    // Indicate failure if conditionally exited earlier
    // Reporting up error doesn't improve robustness or terminate playback
    if ((reinterpret_cast<uint32_t*>(m_pScratchBuffer))[1] == 0x00000000)
    {
        MHW_ASSERTMESSAGE("Debug status check failed with 0x%08x GPR0 0x%x",
            (reinterpret_cast<uint32_t*>(m_pScratchBuffer))[1],
            registerGpr0Lo);
        MT_ERR2(MT_CP_MHW_EARLY_EXIT_CHECK, MT_CP_MHW_SCRATCH_BUFFER, (reinterpret_cast<uint32_t *>(m_pScratchBuffer))[1],
                                                                                          MT_CP_MHW_GPR0, registerGpr0Lo);
    }
#endif

    // Conditional exit #2:
    // Load Isolated Decode SIP register into scratch memory
    storeRegMemParams.dwRegister      = MHW_PAVP_ISO_DEC_SESSION_IN_PLAY_REGISTER;
    storeRegMemParams.presStoreBuffer = &m_ScratchResource;
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiStoreRegisterMemCmd(
            cmdBuffer,
            &storeRegMemParams));

    // AND scratch memory with session ID bit (in GPR0Lo) back into scratch memory
    registerImmParams.dwData     = 1 << sessionId;
    registerImmParams.dwRegister = registerGpr0Lo;
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiLoadRegisterImmCmd(
            cmdBuffer,
            &registerImmParams));
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiFlushDwCmd(
            cmdBuffer,
            &flushDwParams));
    atomicParams.Operation   = MHW_MI_ATOMIC_AND;
    atomicParams.pOsResource = &m_ScratchResource;
    atomicParams.dwDataSize  = sizeof(uint32_t);
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiAtomicCmd(
            cmdBuffer,
            &atomicParams));

    // OR scratch memory with 0x00400000 (in GPR0Lo) back into scratch memory
    // MHW_PAVP_AUTHENTICATED_KERNEL_SIP_BIT = 0x00400000, which is transcode slot #6
    // used to indicate a valid key to decrypt authenticated kernels
    registerImmParams.dwData     = MHW_PAVP_AUTHENTICATED_KERNEL_SIP_BIT_G9;
    registerImmParams.dwRegister = registerGpr0Lo;
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiLoadRegisterImmCmd(
            cmdBuffer,
            &registerImmParams));
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiFlushDwCmd(
            cmdBuffer,
            &flushDwParams));
    atomicParams.Operation   = MHW_MI_ATOMIC_OR;
    atomicParams.pOsResource = &m_ScratchResource;
    atomicParams.dwDataSize  = sizeof(uint32_t);
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiAtomicCmd(
            cmdBuffer,
            &atomicParams));

    // AND scratch memory with SIP register (in GPR0Lo) back into scratch memory
    loadRegisterRegParams.dwSrcRegister = MHW_PAVP_SESSION_IN_PLAY_REGISTER_G9;
    loadRegisterRegParams.dwDstRegister = registerGpr0Lo;
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiLoadRegisterRegCmd(
            cmdBuffer,
            &loadRegisterRegParams));
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiFlushDwCmd(
            cmdBuffer,
            &flushDwParams));
    atomicParams.Operation   = MHW_MI_ATOMIC_AND;
    atomicParams.pOsResource = &m_ScratchResource;
    atomicParams.dwDataSize  = sizeof(uint32_t);
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiAtomicCmd(
            cmdBuffer,
            &atomicParams));
#if (_DEBUG || _RELEASE_INTERNAL)
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiFlushDwCmd(
            cmdBuffer,
            &flushDwParams));

    // Store result in scratch memory plus two
    copyMemMemParams.presSrc     = &m_ScratchResource;
    copyMemMemParams.dwSrcOffset = 0;
    copyMemMemParams.presDst     = &m_ScratchResource;
    copyMemMemParams.dwDstOffset = 2 * sizeof(uint32_t);
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiCopyMemMemCmd(
            cmdBuffer,
            &copyMemMemParams));
#endif

    // Conditionally exit if scratch memory is less than or equal 0x00400000
    // Use of conditional batch buffer function (which calls AddProlog) prevented adding
    // this whole function as part of AddProlog
    conditionalBatchBufferEndParams.presSemaphoreBuffer = &m_ScratchResource;
    conditionalBatchBufferEndParams.dwValue             = MHW_PAVP_AUTHENTICATED_KERNEL_SIP_BIT_G9;
    conditionalBatchBufferEndParams.bDisableCompareMask = true;
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiConditionalBatchBufferEndCmd(
            cmdBuffer,
            &conditionalBatchBufferEndParams));

#if (_DEBUG || _RELEASE_INTERNAL)
    // Indicate failure if conditionally exited earlier
    // Reporting up error doesn't improve robustness or terminate playback
    if ((reinterpret_cast<uint32_t*>(m_pScratchBuffer))[2] <= MHW_PAVP_AUTHENTICATED_KERNEL_SIP_BIT_G9)
    {
        MHW_ASSERTMESSAGE("Stout mode check failed with SIP 0x%08x GPR0 0x%x",
            (reinterpret_cast<uint32_t*>(m_pScratchBuffer))[2],
            registerGpr0Lo);
        MT_ERR2(MT_CP_MHW_EARLY_EXIT_CHECK, MT_CP_MHW_SCRATCH_BUFFER, (reinterpret_cast<uint32_t *>(m_pScratchBuffer))[2],
                                                                                          MT_CP_MHW_GPR0, registerGpr0Lo);
    }
#endif

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS CpHwUnitG9Vcs::AddCheckForEarlyExit(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    MHW_MI_CONDITIONAL_BATCH_BUFFER_END_PARAMS ConditionalBatchBufferEndCmd = {};
    MHW_MI_STORE_REGISTER_MEM_PARAMS           storeRegMemParams            = {};
    MHW_MI_FLUSH_DW_PARAMS                     flushDwParams                = {};
    MHW_MI_ATOMIC_PARAMS                       atomicParams                 = {};
    uint32_t                                   sessionId                    = MHW_INVALID_SESSION_ID;

    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(osInterface->pfnGetGpuContext);
    MHW_CP_CHK_NULL(osInterface->osCpInterface);

    MosCp *pMosCp = dynamic_cast<MosCp*>(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(pMosCp);

    MOS_GPU_CONTEXT mosGpuContext = osInterface->pfnGetGpuContext(osInterface);
    switch (mosGpuContext)
    {
    case MOS_GPU_CONTEXT_VIDEO:
    case MOS_GPU_CONTEXT_VIDEO2:
    case MOS_GPU_CONTEXT_VIDEO3:
    case MOS_GPU_CONTEXT_VIDEO4:
    case MOS_GPU_CONTEXT_VIDEO5:
    case MOS_GPU_CONTEXT_VIDEO6:
    case MOS_GPU_CONTEXT_VIDEO7:
    case MOS_GPU_CONTEXT_VDBOX2_VIDEO:
    case MOS_GPU_CONTEXT_VDBOX2_VIDEO2:
    case MOS_GPU_CONTEXT_VDBOX2_VIDEO3:
        break;
    default:
        MHW_ASSERTMESSAGE("Unsupported GPU context %d", mosGpuContext);
        MT_ERR1(MT_MOS_GPUCXT_GET, MT_MOS_GPUCXT, mosGpuContext);
        break;
    }

    if (!osInterface->osCpInterface->IsCpEnabled() || !osInterface->osCpInterface->IsHMEnabled())
    {
        return MOS_STATUS_SUCCESS;
    }

    sessionId = pMosCp->GetSessionId();
    MHW_CP_CHK_COND(sessionId == MHW_INVALID_SESSION_ID, MOS_STATUS_INVALID_PARAMETER, "Invalid session ID");

    if (TRANSCODE_APP_ID_MASK & sessionId)
    {
        MOS_NORMALMESSAGE(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW, "Skip check early exit for transcode session, sessionId=[%d]", sessionId);
        return MOS_STATUS_SUCCESS;
    }

    MHW_VERBOSEMESSAGE("Using GPU Context id: %d", mosGpuContext);

    // Conditional exit #1:
    // Load VCR Debug Status 2 register into scratch memory
    storeRegMemParams.dwRegister      = MHW_PAVP_VCR_DEBUG_STATUS_2_REGISTER;
    storeRegMemParams.presStoreBuffer = &m_ScratchResource;
    storeRegMemParams.dwOffset        = sizeof(uint64_t) * mosGpuContext;  // m_ScratchResource[mosGpuContext]
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiStoreRegisterMemCmd(cmdBuffer, &storeRegMemParams));

    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiFlushDwCmd(
            cmdBuffer,
            &flushDwParams));

    // AND scratch memory with 0x40000000 back into scratch memory
    // MHW_PAVP_PLANE_ENABLE_MASKED_BIT_G9 = 0x40000000, which indicates whether
    // attacked and waiting for driver cleanup
    atomicParams.Operation         = MHW_MI_ATOMIC_AND;
    atomicParams.pOsResource       = &m_ScratchResource;
    atomicParams.dwDataSize        = sizeof(uint32_t);
    atomicParams.dwResourceOffset  = sizeof(uint64_t) * mosGpuContext;  // m_ScratchResource[mosGpuContext]
    atomicParams.bInlineData       = true;
    atomicParams.dwOperand1Data[0] = MHW_PAVP_PLANE_ENABLE_MASKED_BIT_G9;
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiAtomicCmd(cmdBuffer, &atomicParams));

    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiFlushDwCmd(
            cmdBuffer,
            &flushDwParams));

    // Conditionally exit if scratch memory is equal to 0x00000000
    // Use of conditional batch buffer function (which calls AddProlog) prevented adding
    // this whole function as part of AddProlog
    ConditionalBatchBufferEndCmd.presSemaphoreBuffer = &m_ScratchResource;
    ConditionalBatchBufferEndCmd.dwValue             = 0x00000000;
    ConditionalBatchBufferEndCmd.bDisableCompareMask = true;
    ConditionalBatchBufferEndCmd.dwOffset            = sizeof(uint64_t) * mosGpuContext;
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiConditionalBatchBufferEndCmd(
            cmdBuffer,
            &ConditionalBatchBufferEndCmd));

    // Conditional exit #2:
    // Load Isolated Decode SIP register into scratch memory
    storeRegMemParams.dwRegister      = MHW_PAVP_SESSION_IN_PLAY_REGISTER;
    storeRegMemParams.presStoreBuffer = &m_ScratchResource;
    storeRegMemParams.dwOffset        = sizeof(uint64_t) * mosGpuContext;
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiStoreRegisterMemCmd(
            cmdBuffer,
            &storeRegMemParams));

    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiFlushDwCmd(
            cmdBuffer,
            &flushDwParams));

    // AND scratch memory with session ID bit  back into scratch memory
    atomicParams.Operation         = MHW_MI_ATOMIC_AND;
    atomicParams.pOsResource       = &m_ScratchResource;
    atomicParams.dwDataSize        = sizeof(uint32_t);
    atomicParams.bInlineData       = true;
    atomicParams.dwResourceOffset  = sizeof(uint64_t) * mosGpuContext;  // m_ScratchResource[mosGpuContext]
    atomicParams.dwOperand1Data[0] = 1 << sessionId;
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiAtomicCmd(
            cmdBuffer,
            &atomicParams));

    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiFlushDwCmd(
            cmdBuffer,
            &flushDwParams));

    // Conditionally exit if scratch memory is equal to 0x00000000
    // Use of conditional batch buffer function (which calls AddProlog) prevented adding
    // this whole function as part of AddProlog
    ConditionalBatchBufferEndCmd.presSemaphoreBuffer = &m_ScratchResource;
    ConditionalBatchBufferEndCmd.dwValue             = 0x00000000;
    ConditionalBatchBufferEndCmd.bDisableCompareMask = true;
    ConditionalBatchBufferEndCmd.dwOffset            = sizeof(uint64_t) * mosGpuContext;
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiConditionalBatchBufferEndCmd(
            cmdBuffer,
            &ConditionalBatchBufferEndCmd));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);

    return MOS_STATUS_SUCCESS;
}

//------------------ PAVP Inline Read Command Type----------------
MOS_STATUS CpHwUnitG9Vcs::AddMfxCryptoInlineStatusRead(
    PMOS_INTERFACE                                     osInterface,
    PMOS_COMMAND_BUFFER                                cmdBuffer,
    PMHW_PAVP_MFX_CRYPTO_INLINE_STATUS_READ_CMD_PARAMS params)
{
    MHW_RESOURCE_PARAMS resourceParams;
    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(params);
    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(params->presReadData);
    MHW_CP_CHK_NULL(osInterface->pfnGetGpuContext);

    auto cmd           = g_cInit_MFX_CRYPTO_INLINE_STATUS_READ_CMD_G8;

    // this is where the status read data requested will be stored
    if (params->presReadData != nullptr)
    {
        MOS_ZeroMemory(&resourceParams, sizeof(resourceParams));
        resourceParams.presResource    = params->presReadData;
        resourceParams.dwOffset        = params->dwReadDataOffset;
        resourceParams.pdwCmd          = (uint32_t*)&(cmd.DW2.Value);
        resourceParams.dwLsbNum        = MHW_PAVP_INLINE_STATUS_READ_DATA_SHIFT;
        resourceParams.dwLocationInCmd = 2;
        resourceParams.dwSize          = params->dwSize;
        resourceParams.bIsWritable     = true;

        // upper bound of the allocated resource will not be set
        resourceParams.dwUpperBoundLocationOffsetFromCmd = 0;

        MHW_CP_CHK_STATUS(pfnAddResourceToCmd(
            osInterface,
            cmdBuffer,
            &resourceParams));
    }

    // this is the pipelined media store dword operation
    // that guarantees the completion of status read request
    if (params->presWriteData != nullptr)
    {
        MOS_ZeroMemory(&resourceParams, sizeof(resourceParams));
        resourceParams.presResource    = params->presWriteData;
        resourceParams.dwOffset        = params->dwWriteDataOffset;
        resourceParams.pdwCmd          = (uint32_t*)&(cmd.DW5.Value);
        resourceParams.dwLsbNum        = MHW_PAVP_INLINE_DWORD_STORE_DATA_SHIFT;
        resourceParams.dwLocationInCmd = 5;
        resourceParams.dwSize          = params->dwSize;
        resourceParams.bIsWritable     = true;

        // upper bound of the allocated resource will not be set
        resourceParams.dwUpperBoundLocationOffsetFromCmd = 0;

        MHW_CP_CHK_STATUS(pfnAddResourceToCmd(
            osInterface,
            cmdBuffer,
            &resourceParams));
    }

    switch (params->StatusReadType)
    {
    case CRYPTO_INLINE_MEMORY_STATUS_READ:
        //Set DW1{2:0] to the required mode (000 in this case)
        cmd.DW1.InlineReadCmdType = CRYPTO_INLINE_MEMORY_STATUS_READ;
        break;
    case CRYPTO_INLINE_GET_FRESHNESS_READ:
        cmd.DW1.InlineReadCmdType         = CRYPTO_INLINE_GET_FRESHNESS_READ;
        cmd.DW1.KeyRefreshRandomValueType = params->Value.KeyRefreshType;
        break;
    case CRYPTO_INLINE_CONNECTION_STATE_READ:
        cmd.DW1.InlineReadCmdType = CRYPTO_INLINE_CONNECTION_STATE_READ;
        cmd.DW4.Nonce             = params->Value.NonceIn;
        break;
    case CRYPTO_INLINE_GET_WIDI_COUNTER_READ:
        cmd.DW1.InlineReadCmdType = CRYPTO_INLINE_GET_WIDI_COUNTER_READ;
        break;
    default:
        return MOS_STATUS_INVALID_PARAMETER;
    }

    cmd.DW7.ImmediateWriteData0 = MHW_PAVP_QUERY_STATUS_STORE_DWORD_DATA;
    cmd.DW8.ImmediateWriteData1 = MHW_PAVP_QUERY_STATUS_STORE_DWORD_DATA;
    MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, &cmd, sizeof(cmd)));

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS CpHwUnitG9Vcs::AddProlog(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(osInterface->osCpInterface);

    MosCp *pMosCp = dynamic_cast<MosCp *>(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(pMosCp);
    MHW_CP_CHK_NULL(cmdBuffer);
    MHW_CP_CHK_NULL(m_mhwMiInterface);

    // VDBox
    MHW_MI_FLUSH_DW_PARAMS miFlushDwParams;

    MHW_CP_FUNCTION_ENTER;

    //Send CMD_MFX_WAIT...
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMfxWaitCmd(cmdBuffer, nullptr, true));
    MOS_ZeroMemory(&miFlushDwParams, sizeof(miFlushDwParams));
    // get the address of the MI_FLUSH_DW going to be added
    auto miFlushDw = reinterpret_cast<MI_FLUSH_DW_PAVP *>(cmdBuffer->pCmdPtr);  // DW0
    MHW_CP_CHK_NULL(miFlushDw);
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiFlushDwCmd(cmdBuffer, &miFlushDwParams));
    // patch the protection bit OFF
    miFlushDw->ProtectedMemoryEnable = 0;
    // Send MI_SET_APPID
    MHW_CP_CHK_STATUS(AddMiSetAppId(osInterface, cmdBuffer, pMosCp->GetSessionId()));
    // Send CMD_MFX_WAIT
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMfxWaitCmd(cmdBuffer, nullptr, true));

    // MI_FLUSH_DW
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiFlushDwCmd(cmdBuffer, &miFlushDwParams));

    // Insert CRYPTO_KEY_EXCHANGE_COMMAND if Kb based Key Ex
    if (m_VcsEncryptionParams.bInsertKeyExinVcsProlog)
    {
        if (m_VcsEncryptionParams.CryptoKeyExParams.KeyExchangeKeyUse == CRYPTO_TERMINATE_KEY)
        {
            // This is a programming error (either GPU HAL is not updating MHW CP or GPU HAL was not updated when a key was programmed)
            MHW_ASSERTMESSAGE("Cached decrypt/encrypt key is not valid!!!");
            MT_ERR1(MT_CP_INVALID_CACHED_KEY, MT_CODE_LINE, __LINE__);
            MHW_CP_CHK_STATUS(MOS_STATUS_INVALID_PARAMETER);
        }

        uint32_t ret = memcmp(reinterpret_cast<uint8_t *>(m_VcsEncryptionParams.CryptoKeyExParams.EncryptedDecryptionKey), 
            m_cacheEncryptDecryptKey, MHW_PAVP_AES128_NUM_BYTES_IN_BLOCK);
        if (ret != 0)
        {
            MOS_SecureMemcpy(
                &m_cacheEncryptDecryptKey,
                MHW_PAVP_AES128_NUM_BYTES_IN_BLOCK,
                reinterpret_cast<uint8_t *>(m_VcsEncryptionParams.CryptoKeyExParams.EncryptedDecryptionKey),
                MHW_PAVP_AES128_NUM_BYTES_IN_BLOCK);
            MHW_CP_CHK_STATUS(AddMfxCryptoKeyExchange(cmdBuffer, &(m_VcsEncryptionParams.CryptoKeyExParams)));
        }
    }

    //Send CMD_MFX_WAIT...
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMfxWaitCmd(cmdBuffer, nullptr, true));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS CpHwUnitG9Vcs::AddEpilog(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    MHW_CP_FUNCTION_ENTER;

    // VDBox
    MHW_MI_FLUSH_DW_PARAMS miFlushDwParams;

    MOS_UNUSED(osInterface);
    MHW_CP_CHK_NULL(m_mhwMiInterface);
    MHW_CP_CHK_NULL(cmdBuffer);

    // get the address of the MI_FLUSH_DW going to be added
    auto miFlushDw = reinterpret_cast<MI_FLUSH_DW_PAVP *>(cmdBuffer->pCmdPtr);
    MOS_ZeroMemory(&miFlushDwParams, sizeof(miFlushDwParams));
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiFlushDwCmd(cmdBuffer, &miFlushDwParams));
    // but patch the protection bit off
    miFlushDw->ProtectedMemoryEnable = 0;

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}
