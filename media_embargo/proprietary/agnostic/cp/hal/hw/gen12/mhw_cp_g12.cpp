/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2019-2021
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
//! \file     mhw_cp_g12.cpp
//! \brief    MHW interface for content protection
//! \details  Impelements the Gen-12 functionalities for content protection
//!
#include "mhw_cp_g12.h"
#include "mhw_mmio_ext.h"
#include "mhw_cp_hwcmd_common.h"
#include "mhw_mi_hwcmd_g12_X.h"
#include "mhw_vdbox.h"
#include "mhw_mi.h"
#include "vpkrnheader.h"
#include "vpkrnheader.h"
#include "mhw_cp_next.h"
#include "mhw_cp_hwcmd_next.h"

MhwCpG12::MhwCpG12(MOS_STATUS& eStatus, MOS_INTERFACE& osInterface) : MhwCpBase(osInterface)
{
    eStatus = MOS_STATUS_SUCCESS;
    do
    {
        m_cpCmdPropsMap = &g_decryptBitOffsets;

        this->m_hwUnit = MOS_NewArray(CpHwUnit*, PAVP_HW_UNIT_MAX_COUNT);
        if (this->m_hwUnit == nullptr)
        {
            MOS_ASSERTMESSAGE(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW, "PAVP G12 HW unit pointer array allocation failed.");
            eStatus = MOS_STATUS_NO_SPACE;
            MT_ERR3(MT_CP_MHW_ALLOCATION_FAIL, MT_COMPONENT, MOS_COMPONENT_CP, MT_SUB_COMPONENT, MOS_CP_SUBCOMP_MHW,
                                                                                            MT_ERROR_CODE, eStatus);
            break;
        }

        this->m_hwUnit[PAVP_HW_UNIT_RCS] = MOS_New(CpHwUnitG12Rcs, eStatus, osInterface);
        if (eStatus != MOS_STATUS_SUCCESS)
        {
            break;
        }

        this->m_hwUnit[PAVP_HW_UNIT_VCS] = MOS_New(CpHwUnitG12Vcs, eStatus, osInterface);
        if (eStatus != MOS_STATUS_SUCCESS)
        {
            break;
        }

        this->m_hwUnit[PAVP_HW_UNIT_VECS] = MOS_New(CpHwUnitG12Vecs, eStatus, osInterface);
        if (eStatus != MOS_STATUS_SUCCESS)
        {
            break;
        }

        if ((this->m_hwUnit[PAVP_HW_UNIT_RCS] == nullptr) ||
            (this->m_hwUnit[PAVP_HW_UNIT_VCS] == nullptr) ||
            (this->m_hwUnit[PAVP_HW_UNIT_VECS] == nullptr))
        {
            MOS_ASSERTMESSAGE(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW, "PAVP G12 HW unit allocation failed.");
            eStatus = MOS_STATUS_NO_SPACE;
            MT_ERR3(MT_CP_MHW_ALLOCATION_FAIL, MT_COMPONENT, MOS_COMPONENT_CP, MT_SUB_COMPONENT, MOS_CP_SUBCOMP_MHW,
                                                                                            MT_ERROR_CODE, eStatus);
            break;
        }

        m_sizeOfCmdIndirectCryptoState = sizeof(HUC_INDIRECT_CRYPTO_STATE_G8LP);
    } while (false);
}

MhwCpG12::~MhwCpG12()
{
    /* Free memory in base class */
}

CpHwUnitG12Rcs::CpHwUnitG12Rcs(MOS_STATUS& eStatus, MOS_INTERFACE& osInterface) : CpHwUnitRcs(eStatus, osInterface),
                                                                                  m_ScratchResource(),
                                                                                  m_pScratchBuffer(nullptr)
{
    MOS_STATUS              status                     = MOS_STATUS_UNKNOWN;
    MOS_ALLOC_GFXRES_PARAMS allocParamsForBufferLinear = {};
    MOS_LOCK_PARAMS         lockFlags                  = {};

    MHW_CP_FUNCTION_ENTER;

    do
    {
        MHW_CP_CHK_NULL_NO_STATUS_RETURN(m_osInterface.pfnAllocateResource);
        MHW_CP_CHK_NULL_NO_STATUS_RETURN(m_osInterface.pfnLockResource);

        allocParamsForBufferLinear.Type     = MOS_GFXRES_BUFFER;
        allocParamsForBufferLinear.TileType = MOS_TILE_LINEAR;
        allocParamsForBufferLinear.Format   = Format_Buffer;
        allocParamsForBufferLinear.dwBytes  = MHW_CACHELINE_SIZE;
        allocParamsForBufferLinear.pBufName = "CpHwUnitG12Rcs_ScratchRes";
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
        status = MOS_STATUS_SUCCESS;
    } while (false);

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    eStatus = status;
}

CpHwUnitG12Rcs::~CpHwUnitG12Rcs()
{
    MHW_CP_FUNCTION_ENTER;

    if (m_pScratchBuffer)
    {
        MHW_CP_CHK_NULL_NO_STATUS_RETURN(m_osInterface.pfnUnlockResource);
        m_osInterface.pfnUnlockResource(&m_osInterface, &m_ScratchResource);
        MHW_CP_CHK_NULL_NO_STATUS_RETURN(m_osInterface.pfnFreeResource);
        m_osInterface.pfnFreeResource(&m_osInterface, &m_ScratchResource);
    }

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
}

CpHwUnitG12Vcs::~CpHwUnitG12Vcs()
{
    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL_NO_STATUS_RETURN(m_osInterface.pfnFreeResource);
    m_osInterface.pfnFreeResource(&m_osInterface, &m_ResForKeyValidRegisterScratch);
    m_osInterface.pfnFreeResource(&m_osInterface, &m_ResForSessionInPlayRegisterScratch);

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
}

CpHwUnitG12Vecs::CpHwUnitG12Vecs(MOS_STATUS& eStatus, MOS_INTERFACE& osInterface) : CpHwUnitVecs(eStatus, osInterface),
                                                                                    m_ResForKeyValidRegisterScratch(),
                                                                                    m_ResForSessionInPlayRegisterScratch()
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
        allocParamsForBufferLinear.pBufName = "m_ResForKeyValidRegisterScratch";
        status                              = m_osInterface.pfnAllocateResource(&m_osInterface, &allocParamsForBufferLinear, &m_ResForKeyValidRegisterScratch);
        if (status != MOS_STATUS_SUCCESS)
        {
            break;
        }

        allocParamsForBufferLinear.pBufName = "m_ResForSessionInPlayRegisterScratch";
        status                              = m_osInterface.pfnAllocateResource(&m_osInterface, &allocParamsForBufferLinear, &m_ResForSessionInPlayRegisterScratch);
        if (status != MOS_STATUS_SUCCESS)
        {
            break;
        }
    } while (false);

    MHW_CP_FUNCTION_EXIT(status);
    eStatus = status;
}

CpHwUnitG12Vecs::~CpHwUnitG12Vecs()
{
    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL_NO_STATUS_RETURN(m_osInterface.pfnFreeResource);
    m_osInterface.pfnFreeResource(&m_osInterface, &m_ResForKeyValidRegisterScratch);
    m_osInterface.pfnFreeResource(&m_osInterface, &m_ResForSessionInPlayRegisterScratch);

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
}

CpHwUnitG12Vcs::CpHwUnitG12Vcs(MOS_STATUS& eStatus, MOS_INTERFACE& osInterface) : CpHwUnitVcs(eStatus, osInterface),
                                                                                  m_ResForKeyValidRegisterScratch(),
                                                                                  m_ResForSessionInPlayRegisterScratch()
{
    MOS_STATUS              status                     = MOS_STATUS_UNKNOWN;
    MOS_ALLOC_GFXRES_PARAMS allocParamsForBufferLinear = {};
    MOS_LOCK_PARAMS         lockFlags                  = {};

    MHW_CP_FUNCTION_ENTER;

    do
    {
        MHW_CP_CHK_NULL_NO_STATUS_RETURN(m_osInterface.pfnAllocateResource);
        MOS_ZeroMemory(&m_VcsEncryptionParams, sizeof(m_VcsEncryptionParams));

        allocParamsForBufferLinear.Type     = MOS_GFXRES_BUFFER;
        allocParamsForBufferLinear.TileType = MOS_TILE_LINEAR;
        allocParamsForBufferLinear.Format   = Format_Buffer;
        allocParamsForBufferLinear.dwBytes = MOS_GPU_CONTEXT_MAX * sizeof(uint64_t);
        allocParamsForBufferLinear.pBufName = "m_ResForKeyValidRegisterScratch";
        status = m_osInterface.pfnAllocateResource(&m_osInterface, &allocParamsForBufferLinear, &m_ResForKeyValidRegisterScratch);
        if (status != MOS_STATUS_SUCCESS)
        {
            break;
        }

        allocParamsForBufferLinear.pBufName = "m_ResForSessionInPlayRegisterScratch";
        status = m_osInterface.pfnAllocateResource(&m_osInterface, &allocParamsForBufferLinear, &m_ResForSessionInPlayRegisterScratch);
        if (status != MOS_STATUS_SUCCESS)
        {
            break;
        }
    } while (false);

    MHW_CP_FUNCTION_EXIT(status);
    eStatus = status;
}

MOS_STATUS MhwCpG12::AddProlog(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(cmdBuffer);

    MosCp *pMosCp = dynamic_cast<MosCp*>(osInterface->osCpInterface);
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

    uint32_t *locationAddress = (uint32_t*)cmdBuffer->pCmdPtr;
    uint32_t offset          = cmdBuffer->iOffset;

    auto hwUnit = MhwCpBase::getHwUnit(osInterface);
    MHW_CP_CHK_NULL(hwUnit);

    MHW_CP_CHK_STATUS(hwUnit->SaveEncryptionParams(&m_encryptionParams));
    MHW_CP_CHK_STATUS(hwUnit->AddProlog(osInterface, cmdBuffer));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpG12::RefreshCounter(PMOS_INTERFACE osInterface, PMOS_COMMAND_BUFFER cmdBuffer)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    if (!osInterface->osCpInterface->IsCpEnabled() || !IsHwCounterIncrement(osInterface))
    {
        return eStatus;
    }
    else
    {
        MHW_PAVP_MFX_CRYPTO_KEY_EXCHANGE_CMD_PARAMS sCryptoKeyExchangeCmdParams;
        MOS_ZeroMemory(&sCryptoKeyExchangeCmdParams, sizeof(sCryptoKeyExchangeCmdParams));
        sCryptoKeyExchangeCmdParams.KeyExchangeKeyUse = CRYPTO_ENCRYPTION_OFF;
        return this->AddMfxCryptoKeyExchange(cmdBuffer, &sCryptoKeyExchangeCmdParams);
    }
}

MOS_STATUS MhwCpG12::ReadEncodeCounterFromHW(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer,
    PMOS_RESOURCE       resource,
    uint16_t            currentIndex)
{
    MHW_MI_STORE_REGISTER_MEM_PARAMS    storeRegMemParams = {};
    uint32_t                            uiSessionId = MHW_INVALID_SESSION_ID;
    uint16_t                            count = 0;
    uint32_t                            offset = currentIndex * sizeof(CP_ENCODE_HW_COUNTER);

    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(cmdBuffer);
    MHW_CP_CHK_NULL(resource);

    MosCp *pMosCp = dynamic_cast<MosCp*>(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(pMosCp);

    // Gen11+ Widi counter should be read from KCR Counter reg using MI_STORE_REG_MEM command.
    // 16 bytes (8 byte Counter + 8 byte Nonce) - 4 Dwords
    // Count is used to increment base address for each DWord address
    storeRegMemParams.presStoreBuffer = resource;
    uiSessionId = pMosCp->GetSessionId();

    MOS_NORMALMESSAGE(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW, "Gen11+ uses MI_STOR_REG_MEM command to read Widi Counter and Nonce, uiSessionId=[%d]", uiSessionId);

    while (count < 4)
    {
        if (uiSessionId & TRANSCODE_APP_ID_MASK)
        {
            uint32_t Temp = (uiSessionId & 0x0F);
            // Transcode session base addr +  (multiple of 0x10 and least significiant 4 bits of session id) + address increments by 4 for next Dword
            storeRegMemParams.dwRegister = MHW_PAVP_TRANSCODE_KCR_COUNTER_NONCE_REGISTER + (0x10 * Temp) + count * 4;
        }
        else
        {
            // Base address incremented by 4 for next Dword read by MI_STORE_REG_MEM command
            storeRegMemParams.dwRegister = MHW_PAVP_WIDI_KCR_COUNTER_NONCE_REGISTER + count * 4;
        }

        storeRegMemParams.dwOffset = offset + count * 4;
        MHW_CP_CHK_STATUS(
            m_mhwMiInterface->AddMiStoreRegisterMemCmd(
                cmdBuffer,
                &storeRegMemParams));
        count++;
    }

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpG12::AddMfxAesState(
    bool                 isDecodeInUse,
    PMOS_COMMAND_BUFFER  cmdBuffer,
    PMHW_BATCH_BUFFER    batchBuffer,
    PMHW_PAVP_AES_PARAMS params)
{
    MHW_CP_FUNCTION_ENTER;

    if (isDecodeInUse == false)
    {
        // MFX_AES_STATE is not supported for encoder from Gen11.
        MOS_NORMALMESSAGE(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW, "Skipping MFX_AES_STATE for encode for Gen11+");
        return MOS_STATUS_SUCCESS;
    }

    MHW_CP_CHK_NULL(m_mhwMiInterface);
    MHW_CP_CHK_NULL(params);

    // One of the pointers should be valid
    if (cmdBuffer == nullptr && batchBuffer == nullptr)
    {
        MHW_ASSERTMESSAGE("There was no valid buffer to add the HW command to.");
        MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, MOS_STATUS_NULL_POINTER);
        return MOS_STATUS_NULL_POINTER;
    }

    // This wait cmd is needed in Gen 11 before AES State is issued...
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMfxWaitCmd(cmdBuffer, batchBuffer, true));

    MHW_CP_CHK_STATUS(MhwCpNextBase::AddMfxAesState(this, &this->m_osInterface, isDecodeInUse, cmdBuffer, batchBuffer, params));

    // This wait cmd is needed in Gen 11 before AES State is issued...
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMfxWaitCmd(cmdBuffer, batchBuffer, true));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpG12::AddDummyCryptoCopyBaseAddr(
    PMOS_COMMAND_BUFFER               cmdBuffer,
    PMHW_PAVP_CRYPTO_COPY_ADDR_PARAMS params)
{
    MHW_CP_FUNCTION_ENTER;

    CpHwUnit*                        hwUnit = nullptr;
    MHW_RESOURCE_PARAMS              resourceParams;
    MFX_CRYPTO_COPY_BASE_ADDR_CMD_G8 cmd;

    MHW_CP_CHK_NULL(params);

    cmd = g_cInit_MFX_CRYPTO_COPY_BASE_ADDR_CMD_G8;

    MOS_ZeroMemory(&resourceParams, sizeof(resourceParams));
    resourceParams.dwLsbNum      = MHW_VDBOX_MFX_UPPER_BOUND_STATE_SHIFT;
    resourceParams.HwCommandType = MOS_HW_COMMAND_NULL;

    resourceParams.presResource    = params->presSrc;
    resourceParams.dwOffset        = 0;
    resourceParams.pdwCmd          = (uint32_t *)&(cmd.DW1.Value);
    resourceParams.dwLocationInCmd = 1;
    resourceParams.dwSize          = params->dwSize;
    resourceParams.bIsWritable     = false;

    // upper bound of the allocated resource will be set at 1 DW apart from address location
    resourceParams.dwUpperBoundLocationOffsetFromCmd = 2;

    hwUnit = getHwUnit(&m_osInterface);
    MHW_CP_CHK_NULL(hwUnit);
    MHW_CP_CHK_STATUS(hwUnit->pfnAddResourceToCmd(
        &m_osInterface,
        cmdBuffer,
        &resourceParams));

    if (params->presDst != nullptr)
    {
        // Address location in the token is different than the location in the command
        resourceParams.presResource    = params->presDst;
        resourceParams.dwOffset        = (uint32_t)params->offset;
        resourceParams.pdwCmd          = (uint32_t *)&(cmd.DW5.Value);
        resourceParams.dwLocationInCmd = 5;
        resourceParams.dwSize          = 0;
        resourceParams.bIsWritable     = WRITE_WA; // CC List's src resource marked as protected after be written by dummy CC may cause R2R issue

        resourceParams.HwCommandType = params->bIsDestRenderTarget ? MOS_MFX_CC_BASE_ADDR_STATE : MOS_HW_COMMAND_NULL;

        // upper bound of the allocated resource will not be set
        resourceParams.dwUpperBoundLocationOffsetFromCmd = 0;

        hwUnit = getHwUnit(&m_osInterface);
        MHW_CP_CHK_NULL(hwUnit);
        MHW_CP_CHK_STATUS(hwUnit->pfnAddResourceToCmd(
            &m_osInterface,
            cmdBuffer,
            &resourceParams));
    }

    MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, &cmd, sizeof(cmd)));

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpG12::AddMfxCryptoCopyBaseAddr(
    PMOS_COMMAND_BUFFER               cmdBuffer,
    PMHW_PAVP_CRYPTO_COPY_ADDR_PARAMS params)
{
    MHW_CP_FUNCTION_ENTER;
    return MhwCpNextBase::AddMfxCryptoCopyBaseAddr(this, &this->m_osInterface, cmdBuffer, params);
}

MOS_STATUS MhwCpG12::AddMfxCryptoKeyExchange(
    PMOS_COMMAND_BUFFER                          cmdBuffer,
    PMHW_PAVP_MFX_CRYPTO_KEY_EXCHANGE_CMD_PARAMS params)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    auto cmd = g_cInit_MFX_CRYPTO_KEY_EXCHANGE_CMD;

    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(params);
    MHW_CP_CHK_NULL(cmdBuffer);

    // Populate the fields as requested by the caller.
    switch (params->KeyExchangeKeyUse)
    {
    case CRYPTO_TERMINATE_KEY:
        cmd.DW1.KeyUseIndicator = MHW_CRYPTO_TERMINATE_KEY;
        MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, &cmd, sizeof(cmd)));
        break;
    case CRYPTO_DECRYPT_AND_USE_NEW_KEY:
        cmd.DW1.KeyUseIndicator = MHW_CRYPTO_DECRYPT_AND_USE_NEW_KEY;
        if (params->KeyExchangeKeySelect & MHW_CRYPTO_DECRYPTION_KEY)
        {
            // DWORDLength is the number of DWORDs taken by the keys
            cmd.DW0.DWordLength            = KEY_DWORDS_PER_AES_BLOCK;
            cmd.DW1.DecryptionOrEncryption = CRYPTO_DECRYPT;
            MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, &cmd, sizeof(cmd)));
            MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, params->EncryptedDecryptionKey, sizeof(params->EncryptedDecryptionKey)));
        }
        if (params->KeyExchangeKeySelect & MHW_CRYPTO_ENCRYPTION_KEY)
        {
            // DWORDLength is the number of DWORDs taken by the keys
            cmd.DW0.DWordLength            = KEY_DWORDS_PER_AES_BLOCK;
            cmd.DW1.DecryptionOrEncryption = CRYPTO_ENCRYPT;
            MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, &cmd, sizeof(cmd)));
            // Populate the key blob.
            MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, params->EncryptedEncryptionKey, sizeof(params->EncryptedEncryptionKey)));
        }
        break;
    case CRYPTO_ENCRYPTION_OFF:
        cmd.DW1.KeyUseIndicator = MHW_CRYPTO_ENCRYPTION_OFF;
        MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, &cmd, sizeof(cmd)));
        break;
    case CRYPTO_USE_NEW_FRESHNESS_VALUE:
        MHW_ASSERTMESSAGE("CRYPTO_USE_NEW_FRESHNESS_VALUE is not valid for gen11");
        eStatus = MOS_STATUS_INVALID_PARAMETER;
        MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, eStatus);
        break;
    default:
        eStatus = MOS_STATUS_INVALID_PARAMETER;
        break;
    }

    MHW_CP_FUNCTION_EXIT(eStatus);
    return eStatus;
}

MOS_STATUS MhwCpG12::AddMfxCryptoInlineStatusRead(
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

MOS_STATUS MhwCpG12::AddMfxCryptoCopy(
    PMOS_COMMAND_BUFFER          cmdBuffer,
    PMHW_PAVP_CRYPTO_COPY_PARAMS params)
{
    MHW_CP_FUNCTION_ENTER;
    return MhwCpNextBase::AddMfxCryptoCopy(this, cmdBuffer, params);
}

MOS_STATUS MhwCpG12::AddHcpAesState(
    bool                 isDecodeInUse,
    PMOS_COMMAND_BUFFER  cmdBuffer,
    PMHW_BATCH_BUFFER    batchBuffer,
    PMHW_PAVP_AES_PARAMS params)
{
    MHW_CP_FUNCTION_ENTER;
    MHW_CP_CHK_NULL(cmdBuffer);
    MHW_CP_CHK_NULL(params);

    MHW_CP_CHK_NULL(m_mhwMiInterface);

    // This wait cmd is needed in Gen 11 before AES State is issued...
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMfxWaitCmd(cmdBuffer, batchBuffer, true));

    MHW_CP_CHK_STATUS(MhwCpBase::AddHcpAesState(isDecodeInUse, cmdBuffer, batchBuffer, params));

    // This wait cmd is needed in Gen 11 after AES State is issued...
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMfxWaitCmd(cmdBuffer, batchBuffer, true));

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpG12::AddHucAesState(
    PMOS_COMMAND_BUFFER  cmdBuffer,
    PMHW_PAVP_AES_PARAMS params)
{
    MHW_CP_FUNCTION_ENTER;
    MHW_CP_CHK_NULL(cmdBuffer);
    MHW_CP_CHK_NULL(params);
    MHW_CP_CHK_NULL(m_mhwMiInterface);

    // This wait cmd is needed in Gen 11 before AES State is issued...
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMfxWaitCmd(cmdBuffer, nullptr, true));

    MHW_CP_CHK_STATUS(MhwCpBase::AddHucAesState(cmdBuffer, params));

    // This wait cmd is needed in Gen 11 after AES State is issued...
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMfxWaitCmd(cmdBuffer, nullptr, true));

    return MOS_STATUS_SUCCESS;
}

void MhwCpG12::GetCpStateLevelCmdSize(uint32_t& cmdSize, uint32_t& patchListSize)
{
    MHW_CP_FUNCTION_ENTER;

    uint32_t prologCmdSize =
        mhw_mi_g12_X::MI_FLUSH_DW_CMD::byteSize +
        mhw_mi_g12_X::MFX_WAIT_CMD::byteSize +
        sizeof(MI_SET_APP_ID_CMD) +
        mhw_mi_g12_X::MFX_WAIT_CMD::byteSize +
        mhw_mi_g12_X::MI_FLUSH_DW_CMD::byteSize +
        sizeof(MFX_CRYPTO_KEY_EXCHANGE_CMD) +
        sizeof(MFX_CRYPTO_KEY_EXCHANGE_CMD) +
        mhw_mi_g12_X::MFX_WAIT_CMD::byteSize;

    uint32_t epilogCmdSize =
        mhw_mi_g12_X::MI_FLUSH_DW_CMD::byteSize +
        sizeof(MFX_CRYPTO_KEY_EXCHANGE_CMD) +
        mhw_mi_g12_X::MFX_WAIT_CMD::byteSize;

    cmdSize =
        prologCmdSize +
        MOS_MAX(
        (
            sizeof(HCP_AES_STATE_CMD) +
            mhw_mi_g12_X::MFX_WAIT_CMD::byteSize +
            sizeof(MFX_CRYPTO_COPY_BASE_ADDR_CMD_G8) +
            mhw_pavp_next::MFX_CRYPTO_COPY_CMD::byteSize),
            (sizeof(HUC_AES_IND_STATE_CMD))) +
        epilogCmdSize;

    patchListSize =
        PATCH_LIST_COMMAND(MFX_WAIT_CMD) * 3 +  // 3 MFX_WAIT in prolog
        PATCH_LIST_COMMAND(MI_SET_APP_ID_CMD) +
        PATCH_LIST_COMMAND(MI_FLUSH_DW_CMD) +
        PATCH_LIST_COMMAND(MFX_CRYPTO_KEY_EXCHANGE_CMD) * 2 + // At least 1, max 2 MFX_CRYPTO_KEY_EXCHANGE_CMD in prolog if transcode session
        PATCH_LIST_COMMAND(HCP_AES_STATE_CMD) +
        PATCH_LIST_COMMAND(MFX_WAIT_CMD) +
        PATCH_LIST_COMMAND(MFX_CRYPTO_COPY_BASE_ADDR_CMD) +
        PATCH_LIST_COMMAND(MFX_CRYPTO_COPY_CMD) +
        PATCH_LIST_COMMAND(HUC_AES_IND_STATE_CMD) +
        PATCH_LIST_COMMAND(MI_FLUSH_DW_CMD) +  // epilog
        PATCH_LIST_COMMAND(MFX_CRYPTO_KEY_EXCHANGE_CMD) +
        PATCH_LIST_COMMAND(MFX_WAIT_CMD);

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
}

void MhwCpG12::GetCpSliceLevelCmdSize(uint32_t& cmdSize, uint32_t& patchListSize)
{
    MHW_CP_FUNCTION_ENTER;

    uint32_t cmdSizeForMfx = sizeof(MFX_AES_STATE_CMD_G8) + 2 * mhw_mi_g12_X::MFX_WAIT_CMD::byteSize;
    uint32_t cmdSizeForHcp = sizeof(HCP_AES_STATE_CMD) + 3 * mhw_mi_g12_X::MFX_WAIT_CMD::byteSize + sizeof(HCP_AES_IND_STATE_CMD);
    uint32_t cmdSizeForHuc = sizeof(HUC_AES_STATE_CMD) + 2 * mhw_mi_g12_X::MFX_WAIT_CMD::byteSize + sizeof(HUC_AES_IND_STATE_CMD);

    uint32_t patchLSizeForMfx = PATCH_LIST_COMMAND(MFX_AES_STATE_CMD) + PATCH_LIST_COMMAND(MFX_WAIT_CMD);
    uint32_t patchLSizeForHcp = PATCH_LIST_COMMAND(HCP_AES_STATE_CMD) + PATCH_LIST_COMMAND(HCP_AES_IND_STATE_CMD);
    uint32_t patchLSizeForHuc = PATCH_LIST_COMMAND(HUC_AES_STATE_CMD) + PATCH_LIST_COMMAND(HUC_AES_IND_STATE_CMD);

    cmdSize = MOS_MAX((cmdSizeForMfx), MOS_MAX((cmdSizeForHcp), (cmdSizeForHuc)));
    patchListSize = MOS_MAX((patchLSizeForMfx), MOS_MAX((patchLSizeForHcp), (patchLSizeForHuc)));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
}

bool MhwCpG12::IsHwCounterIncrement(
    PMOS_INTERFACE osInterface)
{
    MHW_CP_FUNCTION_ENTER;
    MHW_CP_ASSERT(osInterface);
    // From Gen11+, both Transcode and WIDI supports HW Counter Increment
    // Return eStatus SUCCESS always for Gen11+
    bool bAutoIncSupport = true;
    MHW_CP_FUNCTION_EXIT(true);
    return bAutoIncSupport;
}

bool MhwCpG12::IsSplitHucStreamOutNeeded()
{
    return true;
}

MOS_STATUS MhwCpG12::GetTranscryptKernelInfo(
    const uint32_t **transcryptMetaData,
    uint32_t     *transcryptMetaDataSize,
    const uint32_t                  **encryptedKernels,
    uint32_t                        *encryptedKernelsSize,
    MHW_PAVP_KERNEL_ENCRYPTION_TYPE encryptionType)
{
    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(transcryptMetaData);
    MHW_CP_CHK_NULL(transcryptMetaDataSize);
    MHW_CP_CHK_NULL(encryptedKernels);
    MHW_CP_CHK_NULL(encryptedKernelsSize);

    MOS_STATUS status = MOS_STATUS_PLATFORM_NOT_SUPPORTED;
    return status;
}


// ----------  Add Prolog ----------
MOS_STATUS CpHwUnitG12Rcs::AddProlog(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    MHW_CP_FUNCTION_ENTER;

    // Render
    PIPE_CONTROL_PAVP*      pipeControl = nullptr;
    MHW_PIPE_CONTROL_PARAMS pipeControlParams;
    PIPELINE_SELECT_CMD_G11 pipelineSelectCmd;

    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(cmdBuffer);
    MHW_CP_CHK_NULL(this->m_mhwMiInterface);

    MosCp *pMosCp = dynamic_cast<MosCp*>(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(pMosCp);

    // Gen9 and Gen10 have a concept of individual power wells.
    // i.e. Render engine can go to power well while Media is functional and visa�versa.
    // Our Crypto Engine is behind Media so when Render is using PAVP,
    // Media has to be awake so that Crypto Engine can do serpent decryption.
    // So need two additional PIPELINE_SELECT to wrap the MI_SET_APPID and PIPE_CONTROL

    // For Gen11, the two PIPELINE_SELECT are not needed, but they are retained for now since
    // RCS+PAVP codepath has not been validated on presi due to display dependencies.
    // Arch discussions ongoing, codepath will be updated later. TBD

    MHW_VERBOSEMESSAGE("PIPELINE_SELECT is not needed in RCS prolog, this code needs to be revisitied - TBD!");

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

    // Gen11: MI_SET_APPID
    MHW_CP_CHK_STATUS(AddMiSetAppId(osInterface, cmdBuffer, pMosCp->GetSessionId()));

    MOS_ZeroMemory(&pipeControlParams, sizeof(pipeControlParams));
    pipeControlParams.dwFlushMode = MHW_FLUSH_WRITE_CACHE;

    // get the address of the PIPE_CONTROL going to be added
    pipeControl = reinterpret_cast<PIPE_CONTROL_PAVP*>(cmdBuffer->pCmdPtr + 1);  // DW1

    // Add the PIPE_CONTROL to set ProtectedMemoryEnable
    MHW_CP_CHK_STATUS(this->m_mhwMiInterface->AddPipeControl(cmdBuffer, nullptr, &pipeControlParams));
    pipeControl->ProtectedMemoryEnable = osInterface->osCpInterface->IsHMEnabled();

    // ---------------- Wrapped by PIPELINE_SELECT ----------------

    // Add the 2nd PIPELINE_SELECT to disable force Media awake
    pipelineSelectCmd.DW0.ForceMediaAwake = 0x0;
    MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, &pipelineSelectCmd, sizeof(pipelineSelectCmd)));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS CpHwUnitG12Vecs::AddProlog(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    MHW_CP_FUNCTION_ENTER;

    // VEBox
    MHW_MI_FLUSH_DW_PARAMS miFlushDwParams;

    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(cmdBuffer);
    MHW_CP_CHK_NULL(this->m_mhwMiInterface);

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

    // Protected MI_FLUSH after MI_SET_APPID
    MHW_CP_CHK_STATUS(this->m_mhwMiInterface->AddMiFlushDwCmd(cmdBuffer, &miFlushDwParams));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS CpHwUnitG12Vcs::AddProlog(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(cmdBuffer);
    MHW_CP_CHK_NULL(this->m_mhwMiInterface);

    MosCp *pMosCp = dynamic_cast<MosCp*>(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(pMosCp);

    // VDBox
    MHW_MI_FLUSH_DW_PARAMS                       miFlushDwParams;
    MHW_PAVP_MFX_CRYPTO_KEY_EXCHANGE_CMD_PARAMS  cryptoKeyExchangeCmdParams;

    //For gen 11, the programming sequence is:
    //CMD_MFX_WAIT
    //MI_FLUSH_DW -> Protection OFF
    //Video_CryptoKeyExchange with CRYPTO_ENCRYPTION_OFF if it's encode
    //MI_SET_APPID
    //CMD_MFX_WAIT
    //MI_FLUSH_DW -> Protection ON
    //Video_CryptoKeyExchange (if Kb key refresh mode)
    //Video_CryptoKeyExchange for encrypt key, if present (if Kb key refresh mode)
    //CMD_MFX_WAIT

    //Send CMD_MFX_WAIT...
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMfxWaitCmd(cmdBuffer, nullptr, true));
    MOS_ZeroMemory(&miFlushDwParams, sizeof(miFlushDwParams));
    // get the address of the MI_FLUSH_DW going to be added
    auto miFlushDw = reinterpret_cast<MI_FLUSH_DW_PAVP*>(cmdBuffer->pCmdPtr);  // DW0
    MHW_CP_CHK_NULL(miFlushDw);
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiFlushDwCmd(cmdBuffer, &miFlushDwParams));
    // patch the protection bit OFF
    miFlushDw->ProtectedMemoryEnable = 0;
    // add Crypto key exchange command to turnoff encryption for encode workloads only
    if (osInterface->Component == COMPONENT_Encode)
    {
        MOS_NORMALMESSAGE(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW, "Adding crypto key ex command (enc off) for encode workload");
        cryptoKeyExchangeCmdParams.KeyExchangeKeyUse = CRYPTO_ENCRYPTION_OFF;
        MHW_CP_CHK_STATUS(this->AddMfxCryptoKeyExchange(cmdBuffer, &cryptoKeyExchangeCmdParams));
    }
    // Send MI_SET_APPID
    MHW_CP_CHK_STATUS(AddMiSetAppId(osInterface, cmdBuffer, pMosCp->GetSessionId()));
    // Send CMD_MFX_WAIT
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMfxWaitCmd(cmdBuffer, nullptr, true));

    // MI_FLUSH_DW
    MHW_CP_CHK_STATUS(this->m_mhwMiInterface->AddMiFlushDwCmd(cmdBuffer, &miFlushDwParams));

    // Insert CRYPTO_KEY_EXCHANGE_COMMAND if Kb based Key Ex
    if (m_VcsEncryptionParams.bInsertKeyExinVcsProlog)
    {
        if (m_VcsEncryptionParams.CryptoKeyExParams.KeyExchangeKeyUse == CRYPTO_TERMINATE_KEY)
        {
            // This is a programming error (either GPU HAL is not updating MHW CP or GPU HAL was not updated when a key was programmed)
            MHW_ASSERTMESSAGE("Cached decrypt/encrypt key is not valid!!!");
            MT_ERR1(MT_CP_MHW_INVALID_KEY, MT_CODE_LINE, __LINE__);
            MHW_CP_CHK_STATUS(MOS_STATUS_INVALID_PARAMETER);
        }
        MHW_CP_CHK_STATUS(this->AddMfxCryptoKeyExchange(cmdBuffer, &(m_VcsEncryptionParams.CryptoKeyExParams)));
    }

    //Send CMD_MFX_WAIT...
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMfxWaitCmd(cmdBuffer, nullptr, true));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS CpHwUnitG12Vcs::AddMfxCryptoKeyExchange(
    PMOS_COMMAND_BUFFER                          cmdBuffer,
    PMHW_PAVP_MFX_CRYPTO_KEY_EXCHANGE_CMD_PARAMS params)
{
    MHW_CP_FUNCTION_ENTER;

    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    auto cmd = g_cInit_MFX_CRYPTO_KEY_EXCHANGE_CMD;

    // Populate the fields as requested by the caller.
    switch (params->KeyExchangeKeyUse)
    {
    case CRYPTO_TERMINATE_KEY:
        cmd.DW1.KeyUseIndicator = MHW_CRYPTO_TERMINATE_KEY;
        MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, &cmd, sizeof(cmd)));
        break;
    case CRYPTO_DECRYPT_AND_USE_NEW_KEY:
        cmd.DW1.KeyUseIndicator = MHW_CRYPTO_DECRYPT_AND_USE_NEW_KEY;
        if (params->KeyExchangeKeySelect & MHW_CRYPTO_DECRYPTION_KEY)
        {
            // DWORDLength is the number of DWORDs taken by the keys
            cmd.DW0.DWordLength            = 0x4;
            cmd.DW1.DecryptionOrEncryption = CRYPTO_DECRYPT;
            MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, &cmd, sizeof(cmd)));
            MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, params->EncryptedDecryptionKey, sizeof(params->EncryptedDecryptionKey)));
        }
        if (params->KeyExchangeKeySelect & MHW_CRYPTO_ENCRYPTION_KEY)
        {
            // DWORDLength is the number of DWORDs taken by the keys
            cmd.DW0.DWordLength            = 0x4;
            cmd.DW1.DecryptionOrEncryption = CRYPTO_ENCRYPT;
            MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, &cmd, sizeof(cmd)));
            // Populate the key blob.
            MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, params->EncryptedEncryptionKey, sizeof(params->EncryptedEncryptionKey)));
        }
        break;
    case CRYPTO_ENCRYPTION_OFF:
        cmd.DW1.KeyUseIndicator = MHW_CRYPTO_ENCRYPTION_OFF;
        MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, &cmd, sizeof(cmd)));
        break;
    case CRYPTO_USE_NEW_FRESHNESS_VALUE:
        MHW_ASSERTMESSAGE("CRYPTO_USE_NEW_FRESHNESS_VALUE is not valid for gen11");
        eStatus = MOS_STATUS_INVALID_PARAMETER;
        MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, eStatus);
        break;
    default:
        eStatus = MOS_STATUS_INVALID_PARAMETER;
        break;
    }

    return eStatus;
}

MOS_STATUS CpHwUnitG12Vcs::AddEpilog(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    MHW_CP_FUNCTION_ENTER;

    // VDBox
    MHW_MI_FLUSH_DW_PARAMS                      miFlushDwParams;
    MHW_PAVP_MFX_CRYPTO_KEY_EXCHANGE_CMD_PARAMS cryptoKeyExchangeCmdParams;

    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(cmdBuffer);

    // get the address of the MI_FLUSH_DW going to be added
    auto miFlushDw = reinterpret_cast<MI_FLUSH_DW_PAVP*>(cmdBuffer->pCmdPtr);
    MOS_ZeroMemory(&miFlushDwParams, sizeof(miFlushDwParams));
    MHW_CP_CHK_STATUS(this->m_mhwMiInterface->AddMiFlushDwCmd(cmdBuffer, &miFlushDwParams));
    // but patch the protection bit off
    miFlushDw->ProtectedMemoryEnable = 0;

    // add Crypto key exchange command to turnoff encryption for encode workloads only
    if (osInterface->Component == COMPONENT_Encode)
    {
        MOS_NORMALMESSAGE(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW, "Adding crypto key ex command (enc off) for encode workload");
        cryptoKeyExchangeCmdParams.KeyExchangeKeyUse = CRYPTO_ENCRYPTION_OFF;
        MHW_CP_CHK_STATUS(this->AddMfxCryptoKeyExchange(cmdBuffer, &cryptoKeyExchangeCmdParams));
    }

    //add MFX_WAIT
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMfxWaitCmd(cmdBuffer, nullptr, true));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

// ---------- Add Check For Early Exit ----------
MOS_STATUS CpHwUnitG12Rcs::AddCheckForEarlyExit(
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
    case MOS_GPU_CONTEXT_COMPUTE:
    case MOS_GPU_CONTEXT_RENDER_RA:
    case MOS_GPU_CONTEXT_COMPUTE_RA:
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
    // Load KCR Debug Status 2 register into scratch memory
    storeRegMemParams.dwRegister      = MHW_PAVP_VCR_DEBUG_STATUS_2_REGISTER;
    storeRegMemParams.presStoreBuffer = &m_ScratchResource;
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiStoreRegisterMemCmd(
            cmdBuffer,
            &storeRegMemParams));

    // AND scratch memory with 0x80000000 (in GPR0Lo) back into scratch memory
    // MHW_PAVP_ISOLATED_DECODE_KEYS_VALID_MASKED_BIT, which indicates whether the ISD key is still valid. 0=>invalid, 1=>valid
    registerImmParams.dwData     = MHW_PAVP_ISOLATED_DECODE_KEYS_VALID_MASKED_BIT;
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

    // Conditionally exit if scratch memory is equal to 0x00000000, which indicates the ISD key is invalid.
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
        MHW_ASSERTMESSAGE("Debug status check (RCS) failed with 0x%08x GPR0 0x%x",
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
    registerImmParams.dwData     = 1 << sessionId | MHW_PAVP_AUTHENTICATED_KERNEL_SIP_BIT;
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
    conditionalBatchBufferEndParams.dwValue             = MHW_PAVP_AUTHENTICATED_KERNEL_SIP_BIT;
    conditionalBatchBufferEndParams.bDisableCompareMask = true;
    MHW_CP_CHK_STATUS(
        m_mhwMiInterface->AddMiConditionalBatchBufferEndCmd(
            cmdBuffer,
            &conditionalBatchBufferEndParams));

#if (_DEBUG || _RELEASE_INTERNAL)
    // Indicate failure if conditionally exited earlier
    // Reporting up error doesn't improve robustness or terminate playback
    if ((reinterpret_cast<uint32_t*>(m_pScratchBuffer))[2] <= MHW_PAVP_AUTHENTICATED_KERNEL_SIP_BIT)
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

MOS_STATUS CpHwUnitG12Vecs::AddCheckForEarlyExit(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    MHW_MI_STORE_REGISTER_MEM_PARAMS storeRegMemParams = {};
    MHW_MI_FLUSH_DW_PARAMS           flushDwParams     = {};
    MHW_MI_ATOMIC_PARAMS             atomicParams      = {};
    uint32_t                         sessionId         = MHW_INVALID_SESSION_ID;

    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(osInterface->pfnGetGpuContext);
    MHW_CP_CHK_NULL(osInterface->osCpInterface);

    MosCp *pMosCp = dynamic_cast<MosCp *>(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(pMosCp);

    MOS_GPU_CONTEXT mosGpuContext = osInterface->pfnGetGpuContext(osInterface);
    switch (mosGpuContext)
    {
    case MOS_GPU_CONTEXT_VEBOX:
    case MOS_GPU_CONTEXT_VEBOX2:
        break;
    default:
        MHW_ASSERTMESSAGE("Unsupported GPU context %d", mosGpuContext);
        MT_ERR1(MT_MOS_GPUCXT_GET, MT_MOS_GPUCXT, mosGpuContext);
        break;
    }

    if (!osInterface->osCpInterface->IsCpEnabled() || !osInterface->osCpInterface->IsHMEnabled()
        || !osInterface->osCpInterface->CpRegisterAccessible())
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

    // Conditional exit #1, check if Serpent/Heavy or IsolatedDecode keys are valid from "KCR Status Register2" BIT5 => 0:invalid, 1:valid
    // Load the register into scratch memory
    storeRegMemParams.dwRegister      = MHW_PAVP_VCR_DEBUG_STATUS_2_REGISTER;
    storeRegMemParams.presStoreBuffer = &m_ResForKeyValidRegisterScratch;
    storeRegMemParams.dwOffset        = sizeof(uint64_t) * mosGpuContext;  // m_ResForKeyValidRegisterScratch[mosGpuContext]
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiStoreRegisterMemCmd(cmdBuffer, &storeRegMemParams));

    // AND scratch memory with session ID bit back into scratch memory
    atomicParams.Operation         = MHW_MI_ATOMIC_AND;
    atomicParams.pOsResource       = &m_ResForKeyValidRegisterScratch;
    atomicParams.dwDataSize        = sizeof(uint32_t);
    atomicParams.bInlineData       = true;
    atomicParams.dwResourceOffset  = sizeof(uint64_t) * mosGpuContext;                                                                                                             // m_ResForKeyValidRegisterScratch[mosGpuContext]
    atomicParams.dwOperand1Data[0] = (osInterface->osCpInterface->IsSMEnabled() ?
        MHW_PAVP_ISOLATED_DECODE_KEYS_VALID_MASKED_BIT : MHW_PAVP_SERPENT_HEAVY_KEYS_VALID_MASKED_BIT);  // we only need the BIT5 for HM or BIT7 for SM
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiAtomicCmd(cmdBuffer, &atomicParams));

    // patch the protection bit OFF before the conditional batch buffer end
    MOS_ZeroMemory(&flushDwParams, sizeof(flushDwParams));
    auto miFlushDw = reinterpret_cast<MI_FLUSH_DW_PAVP *>(cmdBuffer->pCmdPtr);  // DW0
    MHW_CP_CHK_NULL(miFlushDw);
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiFlushDwCmd(cmdBuffer, &flushDwParams));
    miFlushDw->ProtectedMemoryEnable = 0;

    // write 0 four times as BSpec requires for MI_CONDITIONAL_BATCH_BUFFER_END_CMD
    MHW_MI_STORE_DATA_PARAMS storeDataParams;
    MOS_ZeroMemory(&storeDataParams, sizeof(MHW_MI_STORE_DATA_PARAMS));
    storeDataParams.pOsResource      = &m_ResForKeyValidRegisterScratch;
    storeDataParams.dwResourceOffset = sizeof(uint32_t) * (MOS_GPU_CONTEXT_MAX - 1);
    storeDataParams.dwValue          = 0;
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiStoreDataImmCmd(cmdBuffer, &storeDataParams));
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiStoreDataImmCmd(cmdBuffer, &storeDataParams));
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiStoreDataImmCmd(cmdBuffer, &storeDataParams));
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiStoreDataImmCmd(cmdBuffer, &storeDataParams));

    mhw_mi_g12_X::MI_CONDITIONAL_BATCH_BUFFER_END_CMD ConditionalBatchBufferEndCmd;
    ConditionalBatchBufferEndCmd.DW0.CompareSemaphore = 1;
    ConditionalBatchBufferEndCmd.DW1.CompareDataDword = 0x0;

    MHW_RESOURCE_PARAMS resourceParams;
    MOS_ZeroMemory(&resourceParams, sizeof(resourceParams));
    resourceParams.presResource    = &m_ResForKeyValidRegisterScratch;
    resourceParams.dwOffset        = sizeof(uint64_t) * mosGpuContext;
    resourceParams.pdwCmd          = ConditionalBatchBufferEndCmd.DW2_3.Value;
    resourceParams.dwLocationInCmd = 2;
    resourceParams.dwLsbNum        = MHW_COMMON_MI_CONDITIONAL_BATCH_BUFFER_END_SHIFT;
    resourceParams.HwCommandType   = MOS_MI_CONDITIONAL_BATCH_BUFFER_END;

    Mhw_AddResourceToCmd_GfxAddress(osInterface, cmdBuffer, &resourceParams);
    Mos_AddCommand(cmdBuffer, &ConditionalBatchBufferEndCmd, ConditionalBatchBufferEndCmd.byteSize);

    // Conditional exit #2, check if the session is still alive from SIP => 0:inactive, other:active
    // Load SIP register into scratch memory
    storeRegMemParams.dwRegister      = MHW_PAVP_SESSION_IN_PLAY_REGISTER;
    storeRegMemParams.presStoreBuffer = &m_ResForSessionInPlayRegisterScratch;
    storeRegMemParams.dwOffset        = sizeof(uint64_t) * mosGpuContext;  // m_ResForSessionInPlayRegisterScratch[mosGpuContext]
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiStoreRegisterMemCmd(cmdBuffer, &storeRegMemParams));

    // AND scratch memory with session ID bit back into scratch memory
    atomicParams.Operation        = MHW_MI_ATOMIC_AND;
    atomicParams.pOsResource      = &m_ResForSessionInPlayRegisterScratch;
    atomicParams.dwDataSize       = sizeof(uint32_t);
    atomicParams.bInlineData      = true;
    atomicParams.dwResourceOffset = sizeof(uint64_t) * mosGpuContext;  // m_ResForSessionInPlayRegisterScratch[mosGpuContext]

    if (TRANSCODE_APP_ID_MASK & sessionId)  // transcode (sessionId=0x80~0x85), session_0 starts from SIP[BIT16]
    {
        atomicParams.dwOperand1Data[0] = 1 << ((0x0F & sessionId) + 16);
    }
    else  // display session id
    {
        atomicParams.dwOperand1Data[0] = 1 << sessionId;
    }

    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiAtomicCmd(cmdBuffer, &atomicParams));

    // write 0 four times as BSpec requires for MI_CONDITIONAL_BATCH_BUFFER_END_CMD
    storeDataParams.pOsResource = &m_ResForSessionInPlayRegisterScratch;
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiStoreDataImmCmd(cmdBuffer, &storeDataParams));
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiStoreDataImmCmd(cmdBuffer, &storeDataParams));
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiStoreDataImmCmd(cmdBuffer, &storeDataParams));
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiStoreDataImmCmd(cmdBuffer, &storeDataParams));

    resourceParams.presResource = &m_ResForSessionInPlayRegisterScratch;
    resourceParams.dwOffset     = sizeof(uint64_t) * mosGpuContext;
    Mhw_AddResourceToCmd_GfxAddress(osInterface, cmdBuffer, &resourceParams);
    Mos_AddCommand(cmdBuffer, &ConditionalBatchBufferEndCmd, ConditionalBatchBufferEndCmd.byteSize);

    // turn on the protection bit if not exit due to conditional batch buffer end #1 or #2
    MOS_ZeroMemory(&flushDwParams, sizeof(flushDwParams));
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiFlushDwCmd(cmdBuffer, &flushDwParams));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

//------------------ PAVP Inline Read Command Type----------------
MOS_STATUS CpHwUnitG12Vcs::AddMfxCryptoInlineStatusRead(
    PMOS_INTERFACE                                     osInterface,
    PMOS_COMMAND_BUFFER                                cmdBuffer,
    PMHW_PAVP_MFX_CRYPTO_INLINE_STATUS_READ_CMD_PARAMS params)
{
    MFX_CRYPTO_INLINE_STATUS_READ_CMD_G8 cmd;
    MHW_MI_STORE_REGISTER_MEM_PARAMS     storeRegMemParams = {};
    MHW_RESOURCE_PARAMS                  resourceParams;
    MHW_MI_FLUSH_DW_PARAMS               flushDwParams = {0};
    uint16_t                             count = 0;
    uint32_t                             uiSessionId = MHW_INVALID_SESSION_ID;

    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(params);
    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(params->presReadData);
    MHW_CP_CHK_NULL(osInterface->osCpInterface);

    MosCp *pMosCp = dynamic_cast<MosCp*>(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(pMosCp);

    cmd = g_cInit_MFX_CRYPTO_INLINE_STATUS_READ_CMD_G8;

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
        MOS_ASSERTMESSAGE(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW, "Unexpected!");
        MT_ERR2(MT_CP_MHW_STATUS_READ, MT_COMPONENT, MOS_COMPONENT_CP, MT_SUB_COMPONENT, MOS_CP_SUBCOMP_MHW);
        break;
    default:
        return MOS_STATUS_INVALID_PARAMETER;
    }

    cmd.DW7.ImmediateWriteData0 = MHW_PAVP_QUERY_STATUS_STORE_DWORD_DATA;
    cmd.DW8.ImmediateWriteData1 = MHW_PAVP_QUERY_STATUS_STORE_DWORD_DATA;
    MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, &cmd, sizeof(cmd)));

    return MOS_STATUS_SUCCESS;
}


MOS_STATUS CpHwUnitG12Vcs::AddCheckForEarlyExit(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    MHW_MI_STORE_REGISTER_MEM_PARAMS storeRegMemParams = {};
    MHW_MI_FLUSH_DW_PARAMS flushDwParams = {};
    MHW_MI_ATOMIC_PARAMS atomicParams = {};
    uint32_t sessionId = MHW_INVALID_SESSION_ID;

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

    if (!osInterface->osCpInterface->IsCpEnabled() || !osInterface->osCpInterface->IsHMEnabled()
        || !osInterface->osCpInterface->CpRegisterAccessible())
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

    // Conditional exit #1, check if Serpent/Heavy or IsolatedDecode keys are valid from "KCR Status Register2" BIT5 => 0:invalid, 1:valid
    // Load the register into scratch memory
    storeRegMemParams.dwRegister = MHW_PAVP_VCR_DEBUG_STATUS_2_REGISTER;
    storeRegMemParams.presStoreBuffer = &m_ResForKeyValidRegisterScratch;
    storeRegMemParams.dwOffset = sizeof(uint64_t) * mosGpuContext; // m_ResForKeyValidRegisterScratch[mosGpuContext]
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiStoreRegisterMemCmd(cmdBuffer, &storeRegMemParams));

    // AND scratch memory with session ID bit back into scratch memory
    atomicParams.Operation = MHW_MI_ATOMIC_AND;
    atomicParams.pOsResource = &m_ResForKeyValidRegisterScratch;
    atomicParams.dwDataSize = sizeof(uint32_t);
    atomicParams.bInlineData = true;
    atomicParams.dwResourceOffset = sizeof(uint64_t) * mosGpuContext; // m_ResForKeyValidRegisterScratch[mosGpuContext]
    atomicParams.dwOperand1Data[0] = (osInterface->osCpInterface->IsSMEnabled() ?
        MHW_PAVP_ISOLATED_DECODE_KEYS_VALID_MASKED_BIT : MHW_PAVP_SERPENT_HEAVY_KEYS_VALID_MASKED_BIT); // we only need the BIT5 for HM or BIT7 for SM
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiAtomicCmd(cmdBuffer, &atomicParams));

    // patch the protection bit OFF before the conditional batch buffer end
    MOS_ZeroMemory(&flushDwParams, sizeof(flushDwParams));
    auto miFlushDw = reinterpret_cast<MI_FLUSH_DW_PAVP*>(cmdBuffer->pCmdPtr);  // DW0
    MHW_CP_CHK_NULL(miFlushDw);
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiFlushDwCmd(cmdBuffer, &flushDwParams));
    miFlushDw->ProtectedMemoryEnable = 0;

    // write 0 four times as BSpec requires for MI_CONDITIONAL_BATCH_BUFFER_END_CMD
    MHW_MI_STORE_DATA_PARAMS storeDataParams;
    MOS_ZeroMemory(&storeDataParams, sizeof(MHW_MI_STORE_DATA_PARAMS));
    storeDataParams.pOsResource = &m_ResForKeyValidRegisterScratch;
    storeDataParams.dwResourceOffset = sizeof(uint32_t) * (MOS_GPU_CONTEXT_MAX - 1);
    storeDataParams.dwValue = 0;
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiStoreDataImmCmd(cmdBuffer, &storeDataParams));
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiStoreDataImmCmd(cmdBuffer, &storeDataParams));
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiStoreDataImmCmd(cmdBuffer, &storeDataParams));
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiStoreDataImmCmd(cmdBuffer, &storeDataParams));

    mhw_mi_g12_X::MI_CONDITIONAL_BATCH_BUFFER_END_CMD ConditionalBatchBufferEndCmd;
    ConditionalBatchBufferEndCmd.DW0.CompareSemaphore = 1;
    ConditionalBatchBufferEndCmd.DW1.CompareDataDword = 0x0;

    MHW_RESOURCE_PARAMS resourceParams;
    MOS_ZeroMemory(&resourceParams, sizeof(resourceParams));
    resourceParams.presResource = &m_ResForKeyValidRegisterScratch;
    resourceParams.dwOffset = sizeof(uint64_t) * mosGpuContext;
    resourceParams.pdwCmd = ConditionalBatchBufferEndCmd.DW2_3.Value;
    resourceParams.dwLocationInCmd = 2;
    resourceParams.dwLsbNum = MHW_COMMON_MI_CONDITIONAL_BATCH_BUFFER_END_SHIFT;
    resourceParams.HwCommandType = MOS_MI_CONDITIONAL_BATCH_BUFFER_END;

    Mhw_AddResourceToCmd_GfxAddress(osInterface, cmdBuffer, &resourceParams);
    Mos_AddCommand(cmdBuffer, &ConditionalBatchBufferEndCmd, ConditionalBatchBufferEndCmd.byteSize);

    // Conditional exit #2, check if the session is still alive from SIP => 0:inactive, other:active
    // Load SIP register into scratch memory
    storeRegMemParams.dwRegister = MHW_PAVP_SESSION_IN_PLAY_REGISTER;
    storeRegMemParams.presStoreBuffer = &m_ResForSessionInPlayRegisterScratch;
    storeRegMemParams.dwOffset = sizeof(uint64_t) * mosGpuContext; // m_ResForSessionInPlayRegisterScratch[mosGpuContext]
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiStoreRegisterMemCmd(cmdBuffer, &storeRegMemParams));

    // AND scratch memory with session ID bit back into scratch memory
    atomicParams.Operation = MHW_MI_ATOMIC_AND;
    atomicParams.pOsResource = &m_ResForSessionInPlayRegisterScratch;
    atomicParams.dwDataSize = sizeof(uint32_t);
    atomicParams.bInlineData = true;
    atomicParams.dwResourceOffset = sizeof(uint64_t) * mosGpuContext; // m_ResForSessionInPlayRegisterScratch[mosGpuContext]

    if (TRANSCODE_APP_ID_MASK & sessionId) // transcode (sessionId=0x80~0x85), session_0 starts from SIP[BIT16]
    {
        atomicParams.dwOperand1Data[0] = 1 << ((0x0F & sessionId) + 16);
    }
    else // display session id
    {
        atomicParams.dwOperand1Data[0] = 1 << sessionId;
    }

    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiAtomicCmd(cmdBuffer, &atomicParams));

    // write 0 four times as BSpec requires for MI_CONDITIONAL_BATCH_BUFFER_END_CMD
    storeDataParams.pOsResource = &m_ResForSessionInPlayRegisterScratch;
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiStoreDataImmCmd(cmdBuffer, &storeDataParams));
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiStoreDataImmCmd(cmdBuffer, &storeDataParams));
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiStoreDataImmCmd(cmdBuffer, &storeDataParams));
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiStoreDataImmCmd(cmdBuffer, &storeDataParams));

    resourceParams.presResource = &m_ResForSessionInPlayRegisterScratch;
    resourceParams.dwOffset = sizeof(uint64_t) * mosGpuContext;
    Mhw_AddResourceToCmd_GfxAddress(osInterface, cmdBuffer, &resourceParams);
    Mos_AddCommand(cmdBuffer, &ConditionalBatchBufferEndCmd, ConditionalBatchBufferEndCmd.byteSize);

    // turn on the protection bit if not exit due to conditional batch buffer end #1 or #2
    MOS_ZeroMemory(&flushDwParams, sizeof(flushDwParams));
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiFlushDwCmd(cmdBuffer, &flushDwParams));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

