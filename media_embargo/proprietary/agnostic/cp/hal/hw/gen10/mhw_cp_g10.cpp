/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2015-2017
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
//! \file     mhw_cp_g10.cpp
//! \brief    MHW interface for content protection
//! \details  Impelements the Gen-10 functionalities for content protection
//!

#include "mhw_cp_g10.h"
#include "mhw_mmio_ext.h"
#include "mhw_cp_g8.h"
#include "mhw_cp_g9.h"
#include "mhw_cp_hwcmd_common.h"
#include "mhw_mi_hwcmd_g9_X.h"
#include "mhw_vdbox.h"
#include "mhw_mi.h"
#include "mhw_mmio_g10_ext.h"
#include "igvpkrn_g10.h"
#include "vpkrnheader.h"

MhwCpG10::MhwCpG10(MOS_STATUS& eStatus, MOS_INTERFACE& osInterface) : MhwCpBase(osInterface)
{
    eStatus = MOS_STATUS_SUCCESS;
    do
    {
        m_cpCmdPropsMap = &g_decryptBitOffsets_g9;

        this->m_hwUnit = MOS_NewArray(CpHwUnit*, PAVP_HW_UNIT_MAX_COUNT);
        if (this->m_hwUnit == nullptr)
        {
            MOS_ASSERTMESSAGE(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW, "PAVP G10 HW unit pointer array allocation failed.");
            eStatus = MOS_STATUS_NO_SPACE;
            MT_ERR3(MT_CP_MHW_ALLOCATION_FAIL, MT_COMPONENT, MOS_COMPONENT_CP, MT_SUB_COMPONENT, MOS_CP_SUBCOMP_MHW,
                                                                                            MT_ERROR_CODE, eStatus);
            break;
        }

        this->m_hwUnit[PAVP_HW_UNIT_RCS] = MOS_New(CpHwUnitG10Rcs, eStatus, osInterface);
        if (eStatus != MOS_STATUS_SUCCESS)
        {
            break;
        }

        this->m_hwUnit[PAVP_HW_UNIT_VCS] = MOS_New(CpHwUnitG10Vcs, eStatus, osInterface);
        if (eStatus != MOS_STATUS_SUCCESS)
        {
            break;
        }

        this->m_hwUnit[PAVP_HW_UNIT_VECS] = MOS_New(CpHwUnitG10Vecs, eStatus, osInterface);
        if (eStatus != MOS_STATUS_SUCCESS)
        {
            break;
        }

        if ((this->m_hwUnit[PAVP_HW_UNIT_RCS] == nullptr) ||
            (this->m_hwUnit[PAVP_HW_UNIT_VCS] == nullptr) ||
            (this->m_hwUnit[PAVP_HW_UNIT_VECS] == nullptr))
        {
            MOS_ASSERTMESSAGE(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW, "PAVP G10 HW unit allocation failed.");
            eStatus = MOS_STATUS_NO_SPACE;
            MT_ERR3(MT_CP_MHW_ALLOCATION_FAIL, MT_COMPONENT, MOS_COMPONENT_CP, MT_SUB_COMPONENT, MOS_CP_SUBCOMP_MHW,
                                                                                            MT_ERROR_CODE, eStatus);
            break;
        }

        m_sizeOfCmdIndirectCryptoState = sizeof(HUC_INDIRECT_CRYPTO_STATE_G8LP);

    } while (false);
}

MhwCpG10::~MhwCpG10()
{
}

MOS_STATUS MhwCpG10::AddProlog(
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

CpHwUnitG10Rcs::CpHwUnitG10Rcs(MOS_STATUS& eStatus, MOS_INTERFACE& osInterface) : CpHwUnitRcs(eStatus, osInterface),
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
        allocParamsForBufferLinear.pBufName = "CpHwUnitG10Rcs_ScratchRes";
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

CpHwUnitG10Rcs::~CpHwUnitG10Rcs()
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

CpHwUnitG10Vecs::CpHwUnitG10Vecs(MOS_STATUS& eStatus, MOS_INTERFACE& osInterface) : CpHwUnitVecs(eStatus, osInterface)
{
}

CpHwUnitG10Vcs::CpHwUnitG10Vcs(MOS_STATUS& eStatus, MOS_INTERFACE& osInterface) : CpHwUnitVcs(eStatus, osInterface)
{
}

MOS_STATUS MhwCpG10::AddMfxAesState(
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

    MHW_CP_CHK_STATUS(MhwCpG9::AddMfxAesStateG9(this, &this->m_osInterface, isDecodeInUse, cmdBuffer, batchBuffer, params));
    // This wait cmd is needed to make sure pavp command is done before decode starts
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMfxWaitCmd(cmdBuffer, batchBuffer, false));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return  MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpG10::AddMfxCryptoCopyBaseAddr(
    PMOS_COMMAND_BUFFER               cmdBuffer,
    PMHW_PAVP_CRYPTO_COPY_ADDR_PARAMS params)
{
    return MhwCpG8::AddMfxCryptoCopyBaseAddrG8(this, &this->m_osInterface, cmdBuffer, params);
}

MOS_STATUS MhwCpG10::AddMfxCryptoKeyExchange(
    PMOS_COMMAND_BUFFER                          cmdBuffer,
    PMHW_PAVP_MFX_CRYPTO_KEY_EXCHANGE_CMD_PARAMS params)
{
    return MhwCpBase::AddMfxCryptoKeyExchange(cmdBuffer, params);
}

MOS_STATUS MhwCpG10::AddMfxCryptoInlineStatusRead(
    PMOS_INTERFACE                                     osInterface,
    PMOS_COMMAND_BUFFER                                cmdBuffer,
    PMHW_PAVP_MFX_CRYPTO_INLINE_STATUS_READ_CMD_PARAMS params)
{
    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(cmdBuffer);
    MHW_CP_CHK_NULL(osInterface);

    auto hwUnit = MhwCpBase::getHwUnit(osInterface);
    MHW_CP_CHK_NULL(hwUnit);
    MHW_CP_CHK_STATUS(hwUnit->AddMfxCryptoInlineStatusRead(osInterface, cmdBuffer, params));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpG10::AddMfxCryptoCopy(
    PMOS_COMMAND_BUFFER          cmdBuffer,
    PMHW_PAVP_CRYPTO_COPY_PARAMS params)
{
    return MhwCpG8::AddMfxCryptoCopyG8(this, cmdBuffer, params);
}

void MhwCpG10::GetCpStateLevelCmdSize(uint32_t& cmdSize, uint32_t& patchListSize)
{
    MHW_CP_FUNCTION_ENTER;

    uint32_t prologCmdSize = 2 * mhw_mi_g9_X::MI_FLUSH_DW_CMD::byteSize + sizeof(MI_SET_APP_ID_CMD);
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

void MhwCpG10::GetCpSliceLevelCmdSize(uint32_t& cmdSize, uint32_t& patchListSize)
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

bool MhwCpG10::IsHwCounterIncrement(PMOS_INTERFACE osInterface)
{
    MHW_CP_FUNCTION_ENTER;
    bool bAutoIncSupport = false;
    MHW_CP_ASSERT(osInterface);
    MHW_CP_ASSERT(osInterface->osCpInterface);
    MEDIA_FEATURE_TABLE *pSkuTable = osInterface->pfnGetSkuTable(osInterface);
    MHW_CP_ASSERT(pSkuTable);

    if (osInterface->osCpInterface->IsHMEnabled() && MEDIA_IS_SKU(pSkuTable, FtrHWCounterAutoIncrementSupport))
    {
        bAutoIncSupport = true;
        MOS_NORMALMESSAGE(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW, "HWCounterAutoIncrementEnforced is Enabled.");
    }

    MHW_CP_FUNCTION_EXIT(true);
    return bAutoIncSupport;
}

MOS_STATUS MhwCpG10::GetTranscryptKernelInfo(
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

    switch (encryptionType)
    {
    case MHW_PAVP_KERNEL_ENCRYPTION_PRODUCTION:
        *transcryptMetaData     = IGVPKRN_G10 + IDR_VP_TOTAL_NUM_KERNELS + 1 + IGVPKRN_G10[IDR_VP_zzz_production] / sizeof(uint32_t);
        *transcryptMetaDataSize = IGVPKRN_G10[IDR_VP_zzz_production + 1] - IGVPKRN_G10[IDR_VP_zzz_production];
        *encryptedKernels       = IGVPKRN_G10 + IDR_VP_TOTAL_NUM_KERNELS + 1 + IGVPKRN_G10[IDR_VP_zzz_production + 1] / sizeof(uint32_t);
        *encryptedKernelsSize   = IGVPKRN_G10[IDR_VP_TOTAL_NUM_KERNELS] - IGVPKRN_G10[IDR_VP_zzz_production + 1];
        break;

    case MHW_PAVP_KERNEL_ENCRYPTION_PREPRODUCTION:
        *transcryptMetaData     = IGVPKRN_G10 + IDR_VP_TOTAL_NUM_KERNELS + 1 + IGVPKRN_G10[IDR_VP_zzz_preproduction] / sizeof(uint32_t);
        *transcryptMetaDataSize = IGVPKRN_G10[IDR_VP_zzz_preproduction + 1] - IGVPKRN_G10[IDR_VP_zzz_preproduction];
        *encryptedKernels       = IGVPKRN_G10 + IDR_VP_TOTAL_NUM_KERNELS + 1 + IGVPKRN_G10[IDR_VP_zzz_preproduction + 1] / sizeof(uint32_t);
        *encryptedKernelsSize   = IGVPKRN_G10[IDR_VP_zzz_production] - IGVPKRN_G10[IDR_VP_zzz_preproduction + 1];
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

// ----------  Add Prolog ----------
MOS_STATUS CpHwUnitG10Rcs::AddProlog(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    // Render
    PIPE_CONTROL_PAVP*      pipeControl = nullptr;
    MHW_PIPE_CONTROL_PARAMS pipeControlParams;
    PIPELINE_SELECT_CMD_G9  pipelineSelectCmd;

    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(cmdBuffer);
    MHW_CP_CHK_NULL(this->m_mhwMiInterface);

    MosCp *pMosCp = dynamic_cast<MosCp*>(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(pMosCp);

    // Gen9 and Gen10 have a concept of individual power wells.
    // i.e. Render engine can go to power well while Media is functional and visa ersa.
    // Our Crypto Engine is behind Media so when Render is using PAVP,
    // Media has to be awake so that Crypto Engine can do serpent decryption.
    // So need two additional PIPELINE_SELECT to wrap the MI_SET_APPID and PIPE_CONTROL
    // For Gen11, the two PIPELINE_SELECT are not needed.
    pipelineSelectCmd.DW0.ForceMediaAwake = 0x1;
    pipelineSelectCmd.DW0.MaskBits        = 0x20;

    // Add the 1st PIPELINE_SELECT to force Media awake
    MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, &pipelineSelectCmd, sizeof(pipelineSelectCmd)));

    MOS_ZeroMemory(&pipeControlParams, sizeof(pipeControlParams));
    pipeControlParams.dwFlushMode = MHW_FLUSH_WRITE_CACHE;

    // get the address of the PIPE_CONTROL going to be added
    pipeControl = reinterpret_cast<PIPE_CONTROL_PAVP*>(cmdBuffer->pCmdPtr + 1);  // DW1

    // Add the PIPE_CONTROL to set ProtectedMemoryDisable
    MHW_CP_CHK_STATUS(this->m_mhwMiInterface->AddPipeControl(cmdBuffer, nullptr, &pipeControlParams));
    pipeControl->ProtectedMemoryDisable = osInterface->osCpInterface->IsHMEnabled();

    // Gen10: MI_SET_APPID
    MHW_CP_CHK_STATUS(AddMiSetAppId(osInterface, cmdBuffer, pMosCp->GetSessionId()));

    MOS_ZeroMemory(&pipeControlParams, sizeof(pipeControlParams));
    pipeControlParams.dwFlushMode = MHW_FLUSH_WRITE_CACHE;

    // get the address of the PIPE_CONTROL going to be added
    pipeControl = reinterpret_cast<PIPE_CONTROL_PAVP*>(cmdBuffer->pCmdPtr + 1);

    // Add the PIPE_CONTROL to set ProtectedMemoryEnable
    MHW_CP_CHK_STATUS(this->m_mhwMiInterface->AddPipeControl(cmdBuffer, nullptr, &pipeControlParams));
    pipeControl->ProtectedMemoryEnable = osInterface->osCpInterface->IsHMEnabled();

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS CpHwUnitG10Rcs::AddEpilog(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    // Render
    PIPE_CONTROL_PAVP*      pipeControl = NULL;
    MHW_PIPE_CONTROL_PARAMS pipeControlParams;
    PIPELINE_SELECT_CMD_G9  pipelineSelectCmd;

    MOS_FUNCTION_ENTER(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW);

    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(cmdBuffer);
    MHW_CP_CHK_NULL(m_mhwMiInterface);

    MOS_ZeroMemory(&pipeControlParams, sizeof(pipeControlParams));
    pipeControlParams.dwFlushMode = MHW_FLUSH_WRITE_CACHE;

    // get the address of the PIPE_CONTROL going to be added
    pipeControl = reinterpret_cast<PIPE_CONTROL_PAVP*>(cmdBuffer->pCmdPtr + 1);  // DW1

    // actually add the PIPE_CONTROL
    MHW_CP_CHK_STATUS(this->m_mhwMiInterface->AddPipeControl(cmdBuffer, NULL, &pipeControlParams));

    // set the ProtectedMemoryDisable bit to ON for Gen8+
    pipeControl->ProtectedMemoryDisable = osInterface->osCpInterface->IsHMEnabled(); 

    // Add the PIPELINE_SELECT to disable force Media awake
    pipelineSelectCmd.DW0.ForceMediaAwake = 0x0;
    pipelineSelectCmd.DW0.MaskBits        = 0x20;
    MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, &pipelineSelectCmd, sizeof(pipelineSelectCmd)));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS CpHwUnitG10Vecs::AddProlog(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    // VEBox
    MHW_MI_FLUSH_DW_PARAMS miFlushDwParams;

    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(osInterface->osCpInterface);

    MosCp *pMosCp = dynamic_cast<MosCp*>(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(pMosCp);

    MOS_ZeroMemory(&miFlushDwParams, sizeof(miFlushDwParams));
    // get the address of the MI_FLUSH_DW going to be added
    auto miFlushDw = reinterpret_cast<MI_FLUSH_DW_PAVP*>(cmdBuffer->pCmdPtr);  // DW0
    MHW_CP_CHK_NULL(miFlushDw);
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiFlushDwCmd(cmdBuffer, &miFlushDwParams));
    // patch the protection bit OFF
    miFlushDw->ProtectedMemoryEnable = 0;

    // Send MI_SET_APPID
    MHW_CP_CHK_STATUS(AddMiSetAppId(osInterface, cmdBuffer, pMosCp->GetSessionId()));

    // Send MI_FLUSH with protection on
    MHW_CP_CHK_STATUS(this->m_mhwMiInterface->AddMiFlushDwCmd(cmdBuffer, &miFlushDwParams));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

// ---------- Add Check For Early Exit ----------
MOS_STATUS CpHwUnitG10Rcs::AddCheckForEarlyExit(
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

    // AND scratch memory with 0x04000000 (in GPR0Lo) back into scratch memory
    // MHW_PAVP_PLANE_ENABLE_MASKED_BIT = 0x04000000, which indicates whether
    // attacked and waiting for driver cleanup
    registerImmParams.dwData     = MHW_PAVP_PLANE_ENABLE_MASKED_BIT;
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

    // XOR scratch memory with 0x04000000 (in GPR0Lo) back into scratch memory
    // Had to flip the bit to conditionally abort
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiFlushDwCmd(
            cmdBuffer,
            &flushDwParams));
    atomicParams.Operation   = MHW_MI_ATOMIC_XOR;
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
    registerImmParams.dwData     = MHW_PAVP_AUTHENTICATED_KERNEL_SIP_BIT_G10;
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
    loadRegisterRegParams.dwSrcRegister = MHW_PAVP_SESSION_IN_PLAY_REGISTER_G10;
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
    conditionalBatchBufferEndParams.dwValue             = MHW_PAVP_AUTHENTICATED_KERNEL_SIP_BIT_G10;
    conditionalBatchBufferEndParams.bDisableCompareMask = true;
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiConditionalBatchBufferEndCmd(
            cmdBuffer,
            &conditionalBatchBufferEndParams));

#if (_DEBUG || _RELEASE_INTERNAL)
    // Indicate failure if conditionally exited earlier
    // Reporting up error doesn't improve robustness or terminate playback
    if ((reinterpret_cast<uint32_t*>(m_pScratchBuffer))[2] <= MHW_PAVP_AUTHENTICATED_KERNEL_SIP_BIT_G10)
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

//------------------ PAVP Inline Read Command Type----------------
MOS_STATUS CpHwUnitG10Vcs::AddMfxCryptoInlineStatusRead(
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

MOS_STATUS CpHwUnitG10Vcs::AddProlog(
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

MOS_STATUS CpHwUnitG10Vcs::AddEpilog(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
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
