/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2023
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

File Name: cp_hal_gpu_legacy.cpp

Abstract:
Abstract base class for GPU HAL for PAVP functions

Environment:
OS agnostic - should support - Windows, Linux
Platform should support - Gen8, Gen9, Gen11, Gen12

Notes:
None

======================= end_copyright_notice ==================================*/

#include "cp_hal_gpu_legacy.h"
#include "mos_utilities.h"
#include "cp_hal_gpu_g12.h"
#include "cp_hal_gpu_defs.h"
#include "mhw_cp.h"
#include "mhw_mi.h"
#include "mos_utilities.h"
#include "cp_os_services.h"
#include "mhw_cp_hwcmd_common.h"

//To determine min/max value for App Id for Decode and Transcode Sessions.
#define MIN_TRANSCODE_APP_ID 0x80
#define MAX_TRANSCODE_APP_ID 0x87
#define TRANSCODE_APP_ID_MASK 0x80
#define MIN_DECODE_APP_ID 0x0
#define MAX_DECODE_APP_ID 0xf

// Use these to mask the session in play register to get the session types in play.
#define PAVP_DECODE_SESSION_IN_PLAY_MASK 0xFFFF  // Includes WiDi.
#define PAVP_TRANSCODE_SESSION_IN_PLAY_MASK 0xFF0000
#define PAVP_WIDI_SESSION_IN_PLAY_MASK 0x1000

CPavpGpuHalLegacy::~CPavpGpuHalLegacy()
{
    GPU_HAL_FUNCTION_ENTER;

    if (m_pResourcehwctr)
    {
        m_pOsServices->FreeResource(m_pResourcehwctr);
    }
    if (m_pMhwInterfacesLegacy)
    {
        if (m_pMhwInterfacesLegacy->m_miInterface != nullptr)
        {
            MOS_Delete(m_pMhwInterfacesLegacy->m_miInterface);
            m_pMhwInterfacesLegacy->m_miInterface = nullptr;
        }

        if (m_pMhwInterfacesLegacy->m_cpInterface != nullptr)
        {
            MOS_Delete(m_pMhwInterfacesLegacy->m_cpInterface);
            m_pMhwInterfacesLegacy->m_cpInterface = nullptr;
        }

        MOS_Delete(m_pMhwInterfacesLegacy);
        m_pMhwInterfacesLegacy = nullptr;
    }
    m_HwInterface.pPavpHwInterface = nullptr;
    m_HwInterface.pMiInterface     = nullptr;
}

CPavpGpuHalLegacy::CPavpGpuHalLegacy(PLATFORM &sPlatform, MEDIA_WA_TABLE &sWaTable, HRESULT &hr, CPavpOsServices &OsServices)
    : CPavpGpuHal(sPlatform, sWaTable, hr, OsServices)
{
    hr = S_OK;
    m_HwInterface.pPavpHwInterface = nullptr;
    m_HwInterface.pMiInterface     = nullptr;

    MhwInterfaces::CreateParams params;
    params.m_isCp    = true;
    m_pMhwInterfacesLegacy = MhwInterfaces::CreateFactory(params, m_pOsServices->m_pOsInterface);

    if (nullptr == m_pMhwInterfacesLegacy ||
        nullptr == m_pMhwInterfacesLegacy->m_cpInterface ||
        nullptr == m_pMhwInterfacesLegacy->m_miInterface)
    {
        GPU_HAL_ASSERTMESSAGE("m_miItf or MhwCpInterface returned a NULL instance.");
        hr = E_INVALIDARG;
        MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
        return;
    }

    m_HwInterface.pPavpHwInterface = m_pMhwInterfacesLegacy->m_cpInterface;
    m_HwInterface.pMiInterface     = m_pMhwInterfacesLegacy->m_miInterface;
}

/*******************************************************************************
*                           Public functions                                   *
********************************************************************************/

HRESULT CPavpGpuHalLegacy::BuildKeyExchange(
    PMOS_COMMAND_BUFFER                     pCmdBuf,
    const PAVP_GPU_HAL_KEY_EXCHANGE_PARAMS &sBuildKeyExchangeParams)
{
    HRESULT                hr = S_OK;
    MHW_MI_FLUSH_DW_PARAMS FlushDwParams;
    INT                    i = 0, iRemaining = 0;
    MhwCpBase             *mhwCp = nullptr;

    MOS_STATUS eStatus = MOS_STATUS_UNKNOWN;

    GPU_HAL_FUNCTION_ENTER;

    if (m_pOsServices == nullptr ||
        m_pOsServices->m_pOsInterface == nullptr ||
        m_pOsServices->m_pOsInterface->osCpInterface == nullptr ||
        m_HwInterface.pMiInterface == nullptr ||
        m_HwInterface.pPavpHwInterface == nullptr ||
        pCmdBuf == nullptr)
    {
        hr = E_UNEXPECTED;
        return hr;
    }

    mhwCp = dynamic_cast<MhwCpBase *>(m_HwInterface.pPavpHwInterface);

    if (mhwCp == nullptr)
    {
        hr = E_UNEXPECTED;
        return hr;
    }

    do
    {
        // add CP prolog
        GPU_HAL_CHK_STATUS_WITH_HR(m_HwInterface.pPavpHwInterface->AddProlog(m_pOsServices->m_pOsInterface, pCmdBuf));

        // Insert CRYPTO_KEY_EXCHANGE_COMMAND.
        MHW_PAVP_MFX_CRYPTO_KEY_EXCHANGE_CMD_PARAMS sCryptoKeyExchangeCmdParams;
        MOS_ZeroMemory(&sCryptoKeyExchangeCmdParams, sizeof(sCryptoKeyExchangeCmdParams));

        switch (sBuildKeyExchangeParams.KeyExchangeType)
        {
        case PAVP_KEY_EXCHANGE_TYPE_TERMINATE:
            // Insert the command to terminate the key.
            sCryptoKeyExchangeCmdParams.KeyExchangeKeyUse = CRYPTO_TERMINATE_KEY;
            GPU_HAL_CHK_STATUS_WITH_HR(mhwCp->AddMfxCryptoKeyExchange(pCmdBuf, &sCryptoKeyExchangeCmdParams));
            break;

        case PAVP_KEY_EXCHANGE_TYPE_DECRYPT_FRESHNESS:
        case PAVP_KEY_EXCHANGE_TYPE_ENCRYPT_FRESHNESS:
            sCryptoKeyExchangeCmdParams.KeyExchangeKeyUse = CRYPTO_USE_NEW_FRESHNESS_VALUE;
            sCryptoKeyExchangeCmdParams.KeyExchangeKeySelect =
                (sBuildKeyExchangeParams.KeyExchangeType == PAVP_KEY_EXCHANGE_TYPE_ENCRYPT_FRESHNESS) ? MHW_CRYPTO_ENCRYPTION_KEY : MHW_CRYPTO_DECRYPTION_KEY;

            GPU_HAL_CHK_STATUS_WITH_HR(mhwCp->AddMfxCryptoKeyExchange(pCmdBuf, &sCryptoKeyExchangeCmdParams));
            break;

        case PAVP_KEY_EXCHANGE_TYPE_DECRYPT_ENCRYPT:
        case PAVP_KEY_EXCHANGE_TYPE_DECRYPT:
        case PAVP_KEY_EXCHANGE_TYPE_ENCRYPT:
            if (PAVP_KEY_EXCHANGE_TYPE_DECRYPT_ENCRYPT == sBuildKeyExchangeParams.KeyExchangeType ||
                PAVP_KEY_EXCHANGE_TYPE_DECRYPT == sBuildKeyExchangeParams.KeyExchangeType)
            {
                // Add new decrypt key.
                sCryptoKeyExchangeCmdParams.KeyExchangeKeyUse    = CRYPTO_DECRYPT_AND_USE_NEW_KEY;
                sCryptoKeyExchangeCmdParams.KeyExchangeKeySelect = MHW_CRYPTO_DECRYPTION_KEY;

                GPU_HAL_CHK_STATUS_WITH_HR(MOS_SecureMemcpy(
                    sCryptoKeyExchangeCmdParams.EncryptedDecryptionKey,
                    sizeof(sCryptoKeyExchangeCmdParams.EncryptedDecryptionKey),
                    sBuildKeyExchangeParams.EncryptedDecryptionKey,
                    sizeof(sBuildKeyExchangeParams.EncryptedDecryptionKey)));

                GPU_HAL_CHK_STATUS_WITH_HR(mhwCp->AddMfxCryptoKeyExchange(pCmdBuf, &sCryptoKeyExchangeCmdParams));
            }

            if (PAVP_KEY_EXCHANGE_TYPE_DECRYPT_ENCRYPT == sBuildKeyExchangeParams.KeyExchangeType ||
                PAVP_KEY_EXCHANGE_TYPE_ENCRYPT == sBuildKeyExchangeParams.KeyExchangeType)
            {
                // Add new encrypt key.
                sCryptoKeyExchangeCmdParams.KeyExchangeKeyUse = CRYPTO_DECRYPT_AND_USE_NEW_KEY;
                sCryptoKeyExchangeCmdParams.KeyExchangeKeySelect |= MHW_CRYPTO_ENCRYPTION_KEY;

                GPU_HAL_CHK_STATUS_WITH_HR(MOS_SecureMemcpy(
                    sCryptoKeyExchangeCmdParams.EncryptedEncryptionKey,
                    sizeof(sCryptoKeyExchangeCmdParams.EncryptedEncryptionKey),
                    sBuildKeyExchangeParams.EncryptedEncryptionKey,
                    sizeof(sBuildKeyExchangeParams.EncryptedEncryptionKey)));

                GPU_HAL_CHK_STATUS_WITH_HR(mhwCp->AddMfxCryptoKeyExchange(pCmdBuf, &sCryptoKeyExchangeCmdParams));
            }
            break;

        case PAVP_KEY_EXCHANGE_TYPE_KEY_ROTATION:
            // Add new decrypt key.
            sCryptoKeyExchangeCmdParams.KeyExchangeKeyUse    = CRYPTO_DECRYPT_AND_USE_NEW_KEY;
            sCryptoKeyExchangeCmdParams.KeyExchangeKeySelect = MHW_CRYPTO_DECRYPTION_KEY;

            GPU_HAL_CHK_STATUS_WITH_HR(MOS_SecureMemcpy(
                sCryptoKeyExchangeCmdParams.EncryptedDecryptionKey,
                sizeof(sCryptoKeyExchangeCmdParams.EncryptedDecryptionKey),
                sBuildKeyExchangeParams.EncryptedDecryptionKey,
                sizeof(sBuildKeyExchangeParams.EncryptedDecryptionKey)));

            GPU_HAL_CHK_STATUS_WITH_HR(mhwCp->AddMfxCryptoKeyExchange(pCmdBuf, &sCryptoKeyExchangeCmdParams));

            MOS_ZeroMemory(&FlushDwParams, sizeof(FlushDwParams));
            GPU_HAL_CHK_STATUS_WITH_HR(m_HwInterface.pMiInterface->AddMiFlushDwCmd(pCmdBuf, &FlushDwParams));

            GPU_HAL_CHK_STATUS_WITH_HR(MOS_SecureMemcpy(
                sCryptoKeyExchangeCmdParams.EncryptedDecryptionKey,
                sizeof(sCryptoKeyExchangeCmdParams.EncryptedDecryptionKey),
                sBuildKeyExchangeParams.EncryptedDecryptionRotationKey,
                sizeof(sBuildKeyExchangeParams.EncryptedDecryptionRotationKey)));

            GPU_HAL_CHK_STATUS_WITH_HR(mhwCp->AddMfxCryptoKeyExchange(pCmdBuf, &sCryptoKeyExchangeCmdParams));
            break;

        default:
            GPU_HAL_ASSERTMESSAGE("Unknown key exchange type requested: %d.", sBuildKeyExchangeParams.KeyExchangeType);
            MT_ERR1(MT_CP_KEY_EXCHANGE, MT_CP_KEY_EXCHANGE_TYPE, sBuildKeyExchangeParams.KeyExchangeType);
            break;
        }

        // Insert MI_Flush_DW command with protection OFF.
        MOS_ZeroMemory(&FlushDwParams, sizeof(FlushDwParams));
        GPU_HAL_CHK_STATUS_WITH_HR(m_HwInterface.pMiInterface->AddMiFlushDwCmd(pCmdBuf, &FlushDwParams));

        //Setting AppId to AppId + 1 as it forces the S2 keys to be pushed
        //"WOPCM" for slave VDBox to pickup those keys, if run in 2VDBox mode.
        //If the AppId requested is 15 or 23, then AppId 0 is set, to force the S2 keys in WOPCM.
        //Also note,Terminate Session does not require the context switch.
        if (MEDIA_IS_WA(&m_sWaTable, WaProgramAppIDFor2VDBox) && PAVP_KEY_EXCHANGE_TYPE_TERMINATE != sBuildKeyExchangeParams.KeyExchangeType)
        {
            PAVP_MI_SET_APP_ID_CMD_PARAMS sSetAppIdCmdParams;
            MOS_ZeroMemory(&sSetAppIdCmdParams, sizeof(sSetAppIdCmdParams));
            GPU_HAL_NORMALMESSAGE("Implementing Workaround to push S2 Keys into WOPCM for 2VDBox Mode");

            sSetAppIdCmdParams.dwAppId = ((((sBuildKeyExchangeParams.AppId + 1) > MIN_DECODE_APP_ID) && ((sBuildKeyExchangeParams.AppId + 1) <= MAX_DECODE_APP_ID)) ||
                                             (((sBuildKeyExchangeParams.AppId + 1) > MIN_TRANSCODE_APP_ID) && ((sBuildKeyExchangeParams.AppId + 1) <= MAX_TRANSCODE_APP_ID)))
                                             ? (sBuildKeyExchangeParams.AppId + 1)
                                             : 0;
            GPU_HAL_CHK_STATUS_WITH_HR(mhwCp->AddMiSetAppId(m_pOsServices->m_pOsInterface, pCmdBuf, sSetAppIdCmdParams.dwAppId));
            GPU_HAL_CHK_STATUS_WITH_HR(m_HwInterface.pMiInterface->AddMfxWaitCmd(pCmdBuf, NULL, FALSE));

            //Resetting AppId to the current AppId because MI_SET_APPID
            //with a non-playing app could cause a hang
            sSetAppIdCmdParams.dwAppId = sBuildKeyExchangeParams.AppId;
            GPU_HAL_CHK_STATUS_WITH_HR(mhwCp->AddMiSetAppId(m_pOsServices->m_pOsInterface, pCmdBuf, sSetAppIdCmdParams.dwAppId));
            GPU_HAL_CHK_STATUS_WITH_HR(m_HwInterface.pMiInterface->AddMfxWaitCmd(pCmdBuf, NULL, FALSE));
        }

        GPU_HAL_VERBOSEMESSAGE("Parsing assistance from KMD = %d", m_bParsingAssistanceFromKmd);

        if (!m_bParsingAssistanceFromKmd)
        {
            GPU_HAL_CHK_STATUS_WITH_HR(m_HwInterface.pMiInterface->AddMiBatchBufferEnd(pCmdBuf, nullptr));
        }

        m_pOsServices->m_pOsInterface->pfnReturnCommandBuffer(m_pOsServices->m_pOsInterface, pCmdBuf, 0);
        hr = m_pOsServices->m_pOsInterface->pfnSubmitCommandBuffer(
            m_pOsServices->m_pOsInterface,
            pCmdBuf,
            /* enable rendering in KMD */
            FALSE);
        if (FAILED(hr))
        {
            GPU_HAL_ASSERTMESSAGE("Failed to submit command buffer. Error = 0x%x.", hr);
            MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            goto finish;
        }

    } while (FALSE);

finish:

    // Failed -> discard all changes in Command Buffer
    if (FAILED(hr))
    {
        // Buffer overflow - display overflow size
        if (pCmdBuf->iRemaining < 0)
        {
            GPU_HAL_ASSERTMESSAGE("Command Buffer overflow by %d bytes", -pCmdBuf->iRemaining);
            MT_ERR2(MT_CP_CMD_BUFFER_OVERFLOW, MT_CP_CMD_BUFFER_REMAIN, pCmdBuf->iRemaining, MT_ERROR_CODE, hr);
        }

        // Move command buffer back to beginning
        i                   = iRemaining - pCmdBuf->iRemaining;
        pCmdBuf->iRemaining = iRemaining;
        pCmdBuf->iOffset -= i;
        pCmdBuf->pCmdPtr =
            pCmdBuf->pCmdBase + pCmdBuf->iOffset / sizeof(DWORD);

        // Return unused command buffer space to OS
        m_pOsServices->m_pOsInterface->pfnReturnCommandBuffer(m_pOsServices->m_pOsInterface, pCmdBuf, 0);
    }

    GPU_HAL_FUNCTION_EXIT(hr);
    return hr;
}

int32_t CPavpGpuHalLegacy::BuildMultiCryptoCopy(
    MOS_COMMAND_BUFFER                    *pCmdBuf,
    PAVP_GPU_HAL_MULTI_CRYPTO_COPY_PARAMS *pBuildMultiCryptoCopyParams,
    size_t                                 cryptoCopyNumber)
{
    int32_t                hr = 0;
    MHW_MI_FLUSH_DW_PARAMS flushDwParams;
    MOS_STATUS             eStatus    = MOS_STATUS_UNKNOWN;
    int32_t                i          = 0;
    int32_t                iRemaining = 0;
    MhwCpBase             *mhwCp      = nullptr;

    GPU_HAL_FUNCTION_ENTER;

    if (pCmdBuf == nullptr || pBuildMultiCryptoCopyParams == nullptr)
    {
        GPU_HAL_ASSERTMESSAGE("Invalid parameters passed in to BuildMultiCryptoCopy.");
        hr = E_UNEXPECTED;
        MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
        return hr;
    }

    if (m_pOsServices == nullptr ||
        m_pOsServices->m_pOsInterface == nullptr ||
        m_pOsServices->m_pOsInterface->osCpInterface == nullptr ||
        m_HwInterface.pMiInterface == nullptr ||
        m_HwInterface.pPavpHwInterface == nullptr)
    {
        GPU_HAL_ASSERTMESSAGE("Os services or HwInterface have not been created.");
        hr = E_UNEXPECTED;
        MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
        return hr;
    }

    mhwCp = dynamic_cast<MhwCpBase *>(m_HwInterface.pPavpHwInterface);

    if (mhwCp == nullptr)
    {
        GPU_HAL_ASSERTMESSAGE("mhwCp is nullptr.");
        hr = E_UNEXPECTED;
        MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
        return hr;
    }

    do
    {
        // add CP prolog
        GPU_HAL_CHK_STATUS_WITH_HR(m_HwInterface.pPavpHwInterface->AddProlog(m_pOsServices->m_pOsInterface, pCmdBuf));

        for (size_t i = 0; i < cryptoCopyNumber; i++)
        {
            // The hardware hangs on a zero size crypto copy so submit a no-op instead and break.
            if (pBuildMultiCryptoCopyParams->dwDataSize == 0)
            {
                GPU_HAL_ASSERTMESSAGE("0 size crypto copy requested. Adding NOOP command to buffer.");
                GPU_HAL_CHK_STATUS_WITH_HR(m_HwInterface.pMiInterface->AddMiNoop(pCmdBuf, nullptr));
                break;
            }

            // Insert MFX_CRYPTO_COPY_BASE_ADDR command.
            MHW_PAVP_CRYPTO_COPY_ADDR_PARAMS sCryptoCopyAddrParams;
            MOS_ZeroMemory(&sCryptoCopyAddrParams, sizeof(sCryptoCopyAddrParams));
            sCryptoCopyAddrParams.bIsDestRenderTarget = true;
            sCryptoCopyAddrParams.dwSize              = MOS_ALIGN_CEIL(pBuildMultiCryptoCopyParams->dwDataSize, 0x1000);

            sCryptoCopyAddrParams.presSrc = pBuildMultiCryptoCopyParams->SrcResource;
            sCryptoCopyAddrParams.presDst = pBuildMultiCryptoCopyParams->DstResource;

            GPU_HAL_CHK_STATUS_WITH_HR(mhwCp->AddMfxCryptoCopyBaseAddr(pCmdBuf, &sCryptoCopyAddrParams));

            // Insert MFX_CRYPTO_COPY command(s).
            MHW_PAVP_CRYPTO_COPY_PARAMS sCryptoCopyCmdParams;
            MOS_ZeroMemory(&sCryptoCopyCmdParams, sizeof(sCryptoCopyCmdParams));

            sCryptoCopyCmdParams.AesMode                = pBuildMultiCryptoCopyParams->eAesMode;
            sCryptoCopyCmdParams.dwCopySize             = pBuildMultiCryptoCopyParams->dwDataSize;
            sCryptoCopyCmdParams.ui8SelectionForIndData = pBuildMultiCryptoCopyParams->eCryptoCopyType;
            GPU_HAL_CHK_STATUS_WITH_HR(MOS_SecureMemcpy(
                sCryptoCopyCmdParams.dwAesCounter,
                sizeof(sCryptoCopyCmdParams.dwAesCounter),
                pBuildMultiCryptoCopyParams->dwAesCtr,
                sizeof(pBuildMultiCryptoCopyParams->dwAesCtr)));

            if (pBuildMultiCryptoCopyParams->eCryptoCopyType == CRYPTO_COPY_TYPE_AES_DECRYPT_PARTIAL)
            {
                sCryptoCopyCmdParams.uiEncryptedBytes = pBuildMultiCryptoCopyParams->uiEncryptedBytes;
                sCryptoCopyCmdParams.uiClearBytes     = pBuildMultiCryptoCopyParams->uiClearBytes;
            }

            sCryptoCopyCmdParams.dwSrcBaseOffset  = pBuildMultiCryptoCopyParams->offset;
            sCryptoCopyCmdParams.dwDestBaseOffset = pBuildMultiCryptoCopyParams->offset;
            GPU_HAL_CHK_STATUS_WITH_HR(mhwCp->AddMfxCryptoCopy(pCmdBuf, &sCryptoCopyCmdParams));

            // Insert MI_Flush_DW command with protection OFF.
            MOS_ZeroMemory(&flushDwParams, sizeof(flushDwParams));
            GPU_HAL_CHK_STATUS_WITH_HR(m_HwInterface.pMiInterface->AddMiFlushDwCmd(pCmdBuf, &flushDwParams));

            pBuildMultiCryptoCopyParams++;
        }

        if (!m_bParsingAssistanceFromKmd)
        {
            GPU_HAL_CHK_STATUS_WITH_HR(m_HwInterface.pMiInterface->AddMiBatchBufferEnd(pCmdBuf, nullptr));
        }

        m_pOsServices->m_pOsInterface->pfnReturnCommandBuffer(m_pOsServices->m_pOsInterface, pCmdBuf, 0);

        hr = m_pOsServices->m_pOsInterface->pfnSubmitCommandBuffer(
            m_pOsServices->m_pOsInterface,
            pCmdBuf,
            /* enable rendering in KMD */
            false);

        if (FAILED(hr))
        {
            GPU_HAL_ASSERTMESSAGE("Failed to submit command buffer. Error = 0x%x.", hr);
            MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        m_pOsServices->SyncOnSubmit(pCmdBuf, true);

    } while (false);

finish:

    // Failed -> discard all changes in Command Buffer
    if (FAILED(hr))
    {
        // Buffer overflow - display overflow size
        if (pCmdBuf->iRemaining < 0)
        {
            GPU_HAL_ASSERTMESSAGE("Command Buffer overflow by %d bytes", -pCmdBuf->iRemaining);
            MT_ERR2(MT_CP_CMD_BUFFER_OVERFLOW, MT_CP_CMD_BUFFER_REMAIN, pCmdBuf->iRemaining, MT_ERROR_CODE, hr);
        }

        // Move command buffer back to beginning
        i                   = iRemaining - pCmdBuf->iRemaining;
        pCmdBuf->iRemaining = iRemaining;
        pCmdBuf->iOffset -= i;
        pCmdBuf->pCmdPtr =
            pCmdBuf->pCmdBase + pCmdBuf->iOffset / sizeof(uint32_t);

        // Return unused command buffer space to OS
        m_pOsServices->m_pOsInterface->pfnReturnCommandBuffer(m_pOsServices->m_pOsInterface, pCmdBuf, 0);
    }

    GPU_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpGpuHalLegacy::BuildCryptoCopyClearToAesCounter(
    MOS_COMMAND_BUFFER                    *pCmdBuf,
    const PAVP_GPU_HAL_CRYPTO_COPY_PARAMS &sBuildCryptoCopyParams,
    bool                                   bWaitForCompletion)
{
    HRESULT                hr             = S_OK;
    MOS_STATUS             eStatus        = MOS_STATUS_UNKNOWN;
    MosCp                 *pMosCp         = nullptr;
    MhwCpBase             *mhwCp          = nullptr;
    PMOS_RESOURCE          pStoreResource = nullptr;
    MHW_MI_FLUSH_DW_PARAMS FlushDwParams;
    uint32_t              *hwcounter = nullptr;

    GPU_HAL_FUNCTION_ENTER;

    GPU_HAL_CHK_NULL(pCmdBuf);
    GPU_HAL_CHK_NULL(m_pOsServices);
    GPU_HAL_CHK_NULL(m_pOsServices->m_pOsInterface);
    GPU_HAL_CHK_NULL(m_pOsServices->m_pOsInterface->osCpInterface);
    GPU_HAL_CHK_NULL(m_HwInterface.pMiInterface);
    GPU_HAL_CHK_NULL(m_HwInterface.pPavpHwInterface);
    GPU_HAL_ASSERT(m_pOsServices->m_pOsInterface->osCpInterface->IsCpEnabled());

    pMosCp = dynamic_cast<MosCp *>(m_pOsServices->m_pOsInterface->osCpInterface);
    mhwCp  = dynamic_cast<MhwCpBase *>(m_HwInterface.pPavpHwInterface);

    GPU_HAL_CHK_NULL(pMosCp);
    GPU_HAL_CHK_NULL(mhwCp);

    do
    {
        SetGpuCryptoKeyExParams(mhwCp->GetEncryptionParams());
        GPU_HAL_CHK_STATUS_WITH_HR(m_HwInterface.pPavpHwInterface->AddProlog(m_pOsServices->m_pOsInterface, pCmdBuf));

        if (sBuildCryptoCopyParams.pHwctr != nullptr)
        {  // read hw counter
            if (m_pResourcehwctr == nullptr)
            {
                DWORD AllocationSize = 0;
                AllocationSize       = sizeof(CP_ENCODE_HW_COUNTER);

                hr = m_pOsServices->AllocateLinearResource(
                    MOS_GFXRES_BUFFER,  // eType
                    AllocationSize,     // dwWidth
                    1,                  // dwHeight
                    __FUNCTION__,       // pResourceName
                    Format_Buffer,      // eFormat
                    m_pResourcehwctr,
                    true);
                if (FAILED(hr) || m_pResourcehwctr == nullptr)
                {
                    GPU_HAL_ASSERTMESSAGE("Failed to allocate resource. Error = 0x%x.", hr);
                    MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                    break;
                }
            }

            MOS_LOCK_PARAMS lockFlags;
            MOS_ZeroMemory(&lockFlags, sizeof(MOS_LOCK_PARAMS));
            lockFlags.WriteOnly   = 1;
            lockFlags.NoOverWrite = 1;
            hwcounter             = (uint32_t *)m_pOsServices->m_pOsInterface->pfnLockResource(m_pOsServices->m_pOsInterface, m_pResourcehwctr, &lockFlags);
            if (hwcounter == nullptr)
            {
                GPU_HAL_ASSERTMESSAGE("Error locking the resource.");
                hr = E_FAIL;
                MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }
            // insert read ctr command
            GPU_HAL_CHK_STATUS_WITH_HR(mhwCp->ReadEncodeCounterFromHW(m_pOsServices->m_pOsInterface, pCmdBuf, m_pResourcehwctr, 0));
        }

        {  // Clear to Serpent
            MHW_PAVP_CRYPTO_COPY_ADDR_PARAMS CryptoCopyAddrParams = {0};
            CryptoCopyAddrParams.dwSize                           = MOS_ALIGN_CEIL(sBuildCryptoCopyParams.dwDataSize, 0x1000);
            CryptoCopyAddrParams.presSrc                          = sBuildCryptoCopyParams.SrcResource;
            CryptoCopyAddrParams.presDst                          = sBuildCryptoCopyParams.DstResource;
            GPU_HAL_CHK_STATUS_WITH_HR(mhwCp->AddMfxCryptoCopyBaseAddr(pCmdBuf, &CryptoCopyAddrParams));

            MHW_PAVP_CRYPTO_COPY_PARAMS sCryptoCopyCmdParams = {0};
            sCryptoCopyCmdParams.dwCopySize                  = sBuildCryptoCopyParams.dwDataSize;
            sCryptoCopyCmdParams.ui8SelectionForIndData      = CRYPTO_COPY_TYPE_CLEAR_TO_SERPENT_ENCRYPT;
            GPU_HAL_CHK_STATUS_WITH_HR(mhwCp->AddMfxCryptoCopy(pCmdBuf, &sCryptoCopyCmdParams));
        }

        // adding MI_FLUSH_DW to flush any cache to dest resource.
        MOS_ZeroMemory(&FlushDwParams, sizeof(FlushDwParams));
        GPU_HAL_CHK_STATUS_WITH_HR(m_HwInterface.pMiInterface->AddMiFlushDwCmd(pCmdBuf, &FlushDwParams));

        {  // Serpent to AES-counter
            MHW_PAVP_CRYPTO_COPY_ADDR_PARAMS CryptoCopyAddrParams = {0};
            CryptoCopyAddrParams.dwSize                           = MOS_ALIGN_CEIL(sBuildCryptoCopyParams.dwDataSize, 0x1000);
            CryptoCopyAddrParams.presSrc                          = sBuildCryptoCopyParams.DstResource;  // in-place serpent to AES-counter
            CryptoCopyAddrParams.presDst                          = sBuildCryptoCopyParams.DstResource;
            GPU_HAL_CHK_STATUS_WITH_HR(mhwCp->AddMfxCryptoCopyBaseAddr(pCmdBuf, &CryptoCopyAddrParams));

            MHW_PAVP_CRYPTO_COPY_PARAMS sCryptoCopyCmdParams = {0};
            sCryptoCopyCmdParams.AesMode                     = CRYPTO_COPY_AES_MODE_CTR;
            sCryptoCopyCmdParams.dwCopySize                  = sBuildCryptoCopyParams.dwDataSize;
            sCryptoCopyCmdParams.ui8SelectionForIndData      = CRYPTO_COPY_TYPE_SERPENT_TO_AES_ENCRYPT;
            sCryptoCopyCmdParams.dwAesCounter[0]             = sBuildCryptoCopyParams.dwAesCtr[0];
            sCryptoCopyCmdParams.dwAesCounter[1]             = sBuildCryptoCopyParams.dwAesCtr[1];
            sCryptoCopyCmdParams.dwAesCounter[2]             = sBuildCryptoCopyParams.dwAesCtr[2];
            sCryptoCopyCmdParams.dwAesCounter[3]             = sBuildCryptoCopyParams.dwAesCtr[3];

            GPU_HAL_CHK_STATUS_WITH_HR(mhwCp->AddMfxCryptoCopy(pCmdBuf, &sCryptoCopyCmdParams));
        }

        GPU_HAL_CHK_STATUS_WITH_HR(m_HwInterface.pMiInterface->AddMfxWaitCmd(pCmdBuf, NULL, TRUE));

        // Insert MI_FLUSH_DW command if bWaitForCompletion is required
        if (bWaitForCompletion)
        {
            // Allocate a resource for the output.
            GPU_HAL_VERBOSEMESSAGE("Allocating resources required to wait for CryptoCopy completion...");
            hr = m_pOsServices->AllocateLinearResource(
                MOS_GFXRES_BUFFER,  // eType
                sizeof(DWORD),      // dwWidth
                1,                  // dwHeight
                __FUNCTION__,       // pResourceName
                Format_Buffer,      // eFormat
                pStoreResource,
                true);

            if (FAILED(hr) ||
                pStoreResource == nullptr)
            {
                GPU_HAL_ASSERTMESSAGE("Failed to allocate resource. Error = 0x%x.", hr);
                MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }

            // Fill the output buffer with 0xF (locking and unlocking of the resource is done inside).
            hr = m_pOsServices->FillResourceMemory(
                *pStoreResource,
                sizeof(DWORD),
                PAVP_QUERY_STATUS_BUFFER_INITIAL_VALUE);
            if (FAILED(hr))
            {
                GPU_HAL_ASSERTMESSAGE("Failed to fill resource memory. Error = 0x%x.", hr);
                MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }
            GPU_HAL_VERBOSEMESSAGE("Successfully allocated resources required to wait for CryptoCopy completion. Adding MI_FLUSH_DW(PostSync) to command buffer...");

            MOS_ZeroMemory(&FlushDwParams, sizeof(FlushDwParams));
            FlushDwParams.postSyncOperation = MHW_FLUSH_WRITE_IMMEDIATE_DATA;
            FlushDwParams.pOsResource       = pStoreResource;
            FlushDwParams.dwResourceOffset  = 0;
            FlushDwParams.dwDataDW1         = MHW_PAVP_STORE_DATA_DWORD_DATA;
            GPU_HAL_CHK_STATUS_WITH_HR(m_HwInterface.pMiInterface->AddMiFlushDwCmd(pCmdBuf, &FlushDwParams));
            GPU_HAL_VERBOSEMESSAGE("Successfully added MI_FLUSH_DW to command buffer.");
        }

        GPU_HAL_CHK_STATUS_WITH_HR(m_HwInterface.pMiInterface->AddMiBatchBufferEnd(pCmdBuf, nullptr));

        m_pOsServices->m_pOsInterface->pfnReturnCommandBuffer(m_pOsServices->m_pOsInterface, pCmdBuf, 0);

        hr = m_pOsServices->m_pOsInterface->pfnSubmitCommandBuffer(m_pOsServices->m_pOsInterface, pCmdBuf, FALSE);
        if (FAILED(hr))
        {
            GPU_HAL_ASSERTMESSAGE("Failed to submit command buffer. Error = 0x%x.", hr);
            MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        // Wait for store data command to finish if bWaitForCompletion is required
        if (bWaitForCompletion)
        {
            PBYTE pStoreResBytes = static_cast<BYTE *>(m_pOsServices->LockResource(*pStoreResource, CPavpOsServices::LockType::Read));
            if (pStoreResBytes == nullptr)
            {
                GPU_HAL_ASSERTMESSAGE("Error locking the resource.");
                hr = E_FAIL;
                MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }

            // Wait if necessary for the hardware to finish writing the output.
            DWORD dwWaitInMs = PAVP_QUERY_STATUS_TIMEOUT;
            GPU_HAL_VERBOSEMESSAGE("Trying to read data. Attempt #%d.", PAVP_QUERY_STATUS_TIMEOUT - dwWaitInMs + 1);
            DWORD dwValueInMemory = *(reinterpret_cast<DWORD *>(pStoreResBytes));

            if (dwValueInMemory != MHW_PAVP_STORE_DATA_DWORD_DATA)
            {
                GPU_HAL_VERBOSEMESSAGE("MEM = 0x%08x, Needed = 0x%08x.", dwValueInMemory, MHW_PAVP_STORE_DATA_DWORD_DATA);

                // Poll every 1 ms until timeout.
                for (dwWaitInMs = PAVP_QUERY_STATUS_TIMEOUT; dwWaitInMs > 0; dwWaitInMs--)
                {
                    GPU_HAL_VERBOSEMESSAGE("Trying to read status. Attempt #%d.", PAVP_QUERY_STATUS_TIMEOUT - dwWaitInMs + 1);

                    dwValueInMemory = *(reinterpret_cast<DWORD *>(pStoreResBytes));

                    MosUtilities::MosSleep(1);

                    GPU_HAL_VERBOSEMESSAGE("MEM = 0x%08x, Needed = 0x%08x.", dwValueInMemory, MHW_PAVP_STORE_DATA_DWORD_DATA);

                    if (dwValueInMemory == MHW_PAVP_STORE_DATA_DWORD_DATA)
                    {
                        break;
                    }
                }

                // Timeout.
                if (dwWaitInMs == 0)
                {
                    GPU_HAL_ASSERTMESSAGE("Error: Failed to read Store Data output.");
                    hr = E_FAIL;
                    MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                    break;
                }
            }

            if (dwValueInMemory == MHW_PAVP_STORE_DATA_DWORD_DATA)
            {
                GPU_HAL_VERBOSEMESSAGE("Wait for CryptoCopy completion successful.");
            }

            hr = m_pOsServices->m_pOsInterface->pfnUnlockResource(m_pOsServices->m_pOsInterface, pStoreResource);
            if (FAILED(hr))
            {
                GPU_HAL_ASSERTMESSAGE("Error unlocking resource hr = 0x%x", hr);
                MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }
        }

        if (sBuildCryptoCopyParams.pHwctr != nullptr)
        {
            if (hwcounter != nullptr)
            {
                GPU_HAL_CHK_STATUS_WITH_HR(MOS_SecureMemcpy(sBuildCryptoCopyParams.pHwctr, 4 * sizeof(uint32_t), hwcounter, 4 * sizeof(uint32_t)));
            }
        }

        m_pOsServices->SyncOnSubmit(pCmdBuf, true);

    } while (FALSE);

finish:
    if (pStoreResource)
    {
        m_pOsServices->FreeResource(pStoreResource);
    }
    if (sBuildCryptoCopyParams.pHwctr != nullptr && m_pResourcehwctr)
    {
        hr = m_pOsServices->m_pOsInterface->pfnUnlockResource(m_pOsServices->m_pOsInterface, m_pResourcehwctr);
        if (FAILED(hr))
        {
            GPU_HAL_ASSERTMESSAGE("Error unlocking resource hr = 0x%x", hr);
            MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
        }
    }
    GPU_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpGpuHalLegacy::BuildCryptoCopy(
    MOS_COMMAND_BUFFER                    *pCmdBuf,
    const PAVP_GPU_HAL_CRYPTO_COPY_PARAMS &sBuildCryptoCopyParams)
{
    HRESULT                  hr        = S_OK;
    PVOID                    pResBytes = nullptr;
    PMOS_RESOURCE            pResource = nullptr;
    MHW_MI_FLUSH_DW_PARAMS   FlushDwParams;
    MHW_MI_STORE_DATA_PARAMS StoreDataParams;
    MOS_STATUS               eStatus = MOS_STATUS_UNKNOWN;
    INT                      i = 0, iRemaining = 0;

    MhwCpBase *mhwCp = nullptr;

    GPU_HAL_FUNCTION_ENTER;

    if (m_pOsServices == nullptr ||
        m_pOsServices->m_pOsInterface == nullptr ||
        m_pOsServices->m_pOsInterface->osCpInterface == nullptr ||
        m_HwInterface.pMiInterface == nullptr ||
        m_HwInterface.pPavpHwInterface == nullptr ||
        pCmdBuf == nullptr)
    {
        hr = E_UNEXPECTED;
        return hr;
    }

    mhwCp = dynamic_cast<MhwCpBase *>(m_HwInterface.pPavpHwInterface);

    if (mhwCp == nullptr)
    {
        hr = E_UNEXPECTED;
        return hr;
    }

    do
    {
        if (sBuildCryptoCopyParams.eCryptoCopyType == CRYPTO_COPY_TYPE_CLEAR_TO_AES_ENCRYPT)
        {
            hr = BuildCryptoCopyClearToAesCounter(pCmdBuf, sBuildCryptoCopyParams, false);
            break;
        }

        if (sBuildCryptoCopyParams.dwDataSize == 0 &&
            sBuildCryptoCopyParams.eCryptoCopyType == CRYPTO_COPY_TYPE_SET_PLANE_ENABLE)
        {
            // Illegal to have empty source for setting plane enables packet.
            GPU_HAL_ASSERTMESSAGE("PED packet source size is 0.");
            hr = E_FAIL;
            MT_ERR2(MT_CP_PED_PACKET_SIZE_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        // The hardware hangs on a zero size crypto copy so submit a no-op instead and break.
        if (sBuildCryptoCopyParams.dwDataSize == 0 &&
            sBuildCryptoCopyParams.eCryptoCopyType != CRYPTO_COPY_TYPE_SET_PLANE_ENABLE)
        {
            GPU_HAL_ASSERTMESSAGE("0 size crypto copy requested. Adding NOOP command to buffer.");
            GPU_HAL_CHK_STATUS_WITH_HR(m_HwInterface.pMiInterface->AddMiNoop(pCmdBuf, nullptr));
            break;
        }

        // add CP prolog
        GPU_HAL_CHK_STATUS_WITH_HR(m_HwInterface.pPavpHwInterface->AddProlog(m_pOsServices->m_pOsInterface, pCmdBuf));

        // Patching for destination allocation.
        if (sBuildCryptoCopyParams.eCryptoCopyType == CRYPTO_COPY_TYPE_SET_PLANE_ENABLE)
        {
            // Allocate a resource for the ped packet information.
            hr = m_pOsServices->AllocateLinearResource(
                MOS_GFXRES_BUFFER,                  // eType
                sBuildCryptoCopyParams.dwDataSize,  // dwWidth
                1,                                  // dwHeight
                __FUNCTION__,                       // pResourceName
                Format_Buffer,                      // eFormat
                pResource,                          // pResource
                true);
            if (FAILED(hr) || (pResource == nullptr))
            {
                GPU_HAL_ASSERTMESSAGE("Error allocating resource. Error = 0x%x.", hr);
                MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }

            // Copy the Ped Packet information to the resource buffer.
            pResBytes = m_pOsServices->LockResource(*pResource, CPavpOsServices::LockType::Write);
            if (pResBytes == nullptr)
            {
                GPU_HAL_ASSERTMESSAGE("Error locking the resource");
                hr = E_FAIL;
                MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }

            GPU_HAL_CHK_STATUS_WITH_HR(MOS_SecureMemcpy(
                pResBytes,
                sBuildCryptoCopyParams.dwDataSize,
                sBuildCryptoCopyParams.pSrcData,
                sBuildCryptoCopyParams.dwDataSize));

            hr = m_pOsServices->m_pOsInterface->pfnUnlockResource(m_pOsServices->m_pOsInterface, pResource);
            if (FAILED(hr))
            {
                GPU_HAL_ASSERTMESSAGE("Error unlocking the resource. Error = 0x%x.", hr);
                MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }
        }

        // Insert MFX_CRYPTO_COPY_BASE_ADDR command.
        MHW_PAVP_CRYPTO_COPY_ADDR_PARAMS CryptoCopyAddrParams;
        MOS_ZeroMemory(&CryptoCopyAddrParams, sizeof(CryptoCopyAddrParams));
        CryptoCopyAddrParams.bIsDestRenderTarget = TRUE;
        CryptoCopyAddrParams.dwSize              = MOS_ALIGN_CEIL(sBuildCryptoCopyParams.dwDataSize, 0x1000);

        if (sBuildCryptoCopyParams.eCryptoCopyType != CRYPTO_COPY_TYPE_SET_PLANE_ENABLE)
        {
            CryptoCopyAddrParams.presSrc = sBuildCryptoCopyParams.SrcResource;
            CryptoCopyAddrParams.presDst = sBuildCryptoCopyParams.DstResource;
        }
        else
        {
            CryptoCopyAddrParams.presSrc = pResource;
        }

        GPU_HAL_CHK_STATUS_WITH_HR(mhwCp->AddMfxCryptoCopyBaseAddr(pCmdBuf, &CryptoCopyAddrParams));

        // Insert MFX_CRYPTO_COPY command(s).
        MHW_PAVP_CRYPTO_COPY_PARAMS sCryptoCopyCmdParams;
        MOS_ZeroMemory(&sCryptoCopyCmdParams, sizeof(sCryptoCopyCmdParams));

        sCryptoCopyCmdParams.AesMode                = sBuildCryptoCopyParams.eAesMode;
        sCryptoCopyCmdParams.dwCopySize             = sBuildCryptoCopyParams.dwDataSize;
        sCryptoCopyCmdParams.ui8SelectionForIndData = sBuildCryptoCopyParams.eCryptoCopyType;
        GPU_HAL_CHK_STATUS_WITH_HR(MOS_SecureMemcpy(
            sCryptoCopyCmdParams.dwAesCounter,
            sizeof(sCryptoCopyCmdParams.dwAesCounter),
            sBuildCryptoCopyParams.dwAesCtr,
            sizeof(sBuildCryptoCopyParams.dwAesCtr)));

        if (sBuildCryptoCopyParams.eCryptoCopyType == CRYPTO_COPY_TYPE_AES_DECRYPT_PARTIAL)
        {
            sCryptoCopyCmdParams.uiEncryptedBytes = sBuildCryptoCopyParams.uiEncryptedBytes;
            sCryptoCopyCmdParams.uiClearBytes     = sBuildCryptoCopyParams.uiClearBytes;
        }

        GPU_HAL_CHK_STATUS_WITH_HR(mhwCp->AddMfxCryptoCopy(pCmdBuf, &sCryptoCopyCmdParams));

        // Insert MI_Flush_DW command with protection OFF.
        MOS_ZeroMemory(&FlushDwParams, sizeof(FlushDwParams));
        GPU_HAL_CHK_STATUS_WITH_HR(m_HwInterface.pMiInterface->AddMiFlushDwCmd(pCmdBuf, &FlushDwParams));

        if (!m_bParsingAssistanceFromKmd)
        {
            GPU_HAL_CHK_STATUS_WITH_HR(m_HwInterface.pMiInterface->AddMiBatchBufferEnd(pCmdBuf, nullptr));
        }

        m_pOsServices->m_pOsInterface->pfnReturnCommandBuffer(m_pOsServices->m_pOsInterface, pCmdBuf, 0);

        hr = m_pOsServices->m_pOsInterface->pfnSubmitCommandBuffer(
            m_pOsServices->m_pOsInterface,
            pCmdBuf,
            /* enable rendering in KMD */
            FALSE);

        if (FAILED(hr))
        {
            GPU_HAL_ASSERTMESSAGE("Failed to submit command buffer. Error = 0x%x.", hr);
            MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        m_pOsServices->SyncOnSubmit(pCmdBuf, true);

    } while (FALSE);

finish:

    if (pResource)
    {
        m_pOsServices->FreeResource(pResource);
    }

    // Failed -> discard all changes in Command Buffer
    if (FAILED(hr))
    {
        // Buffer overflow - display overflow size
        if (pCmdBuf->iRemaining < 0)
        {
            GPU_HAL_ASSERTMESSAGE("Command Buffer overflow by %d bytes", -pCmdBuf->iRemaining);
            MT_ERR2(MT_CP_CMD_BUFFER_OVERFLOW, MT_CP_CMD_BUFFER_REMAIN, pCmdBuf->iRemaining, MT_ERROR_CODE, hr);
        }

        // Move command buffer back to beginning
        i                   = iRemaining - pCmdBuf->iRemaining;
        pCmdBuf->iRemaining = iRemaining;
        pCmdBuf->iOffset -= i;
        pCmdBuf->pCmdPtr =
            pCmdBuf->pCmdBase + pCmdBuf->iOffset / sizeof(DWORD);

        // Return unused command buffer space to OS
        m_pOsServices->m_pOsInterface->pfnReturnCommandBuffer(m_pOsServices->m_pOsInterface, pCmdBuf, 0);
    }

    GPU_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpGpuHalLegacy::BuildQueryStatus(
    PMOS_COMMAND_BUFFER               pCmdBuf,
    PAVP_GPU_HAL_QUERY_STATUS_PARAMS &sBuildQueryStatusParams)
{
    HRESULT        hr              = S_OK;
    PMOS_RESOURCE  pResource       = nullptr;
    MOS_STATUS     eStatus         = MOS_STATUS_UNKNOWN;
    PBYTE          pResBytes       = nullptr;
    volatile DWORD dwValueInMemory = 0;
    INT            i = 0, iRemaining = 0;

    MhwCpBase *mhwCp = nullptr;

    GPU_HAL_FUNCTION_ENTER;

    if (m_pOsServices == nullptr ||
        m_pOsServices->m_pOsInterface == nullptr ||
        m_pOsServices->m_pOsInterface->osCpInterface == nullptr ||
        m_HwInterface.pMiInterface == nullptr ||
        m_HwInterface.pPavpHwInterface == nullptr ||
        pCmdBuf == nullptr)
    {
        hr = E_UNEXPECTED;
        return hr;
    }

    mhwCp = dynamic_cast<MhwCpBase *>(m_HwInterface.pPavpHwInterface);

    if (mhwCp == nullptr)
    {
        hr = E_UNEXPECTED;
        return hr;
    }

    do
    {
        // add CP prolog
        GPU_HAL_CHK_STATUS_WITH_HR(m_HwInterface.pPavpHwInterface->AddProlog(m_pOsServices->m_pOsInterface, pCmdBuf));

        // Allocate a resource for the output.
        hr = m_pOsServices->AllocateLinearResource(
            MOS_GFXRES_BUFFER,                    // eType
            PAVP_QUERY_STATUS_MAX_OUT_DATA_SIZE,  // dwWidth
            1,                                    // dwHeight
            __FUNCTION__,                         // pResourceName
            Format_Buffer,                        // eFormat
            pResource,
            true);
        if (FAILED(hr) ||
            pResource == nullptr)
        {
            GPU_HAL_ASSERTMESSAGE("Failed to allocate resource. Error = 0x%x.", hr);
            MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            goto finish;
        }

        // Fill the output buffer with 0xF (locking and unlocking of the resource is done inside).
        hr = m_pOsServices->FillResourceMemory(
            *pResource,
            PAVP_QUERY_STATUS_MAX_OUT_DATA_SIZE,
            PAVP_QUERY_STATUS_BUFFER_INITIAL_VALUE);
        if (FAILED(hr))
        {
            GPU_HAL_ASSERTMESSAGE("Failed to fill resource memory. Error = 0x%x.", hr);
            MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            goto finish;
        }

        // Insert MFX_CRYPTO_INLINE_STATUS_READ command.
        MHW_PAVP_MFX_CRYPTO_INLINE_STATUS_READ_CMD_PARAMS sCryptoInlineStatusReadCmdParams;
        MOS_ZeroMemory(&sCryptoInlineStatusReadCmdParams, sizeof(sCryptoInlineStatusReadCmdParams));

        sCryptoInlineStatusReadCmdParams.presReadData      = pResource;
        sCryptoInlineStatusReadCmdParams.dwSize            = PAVP_QUERY_STATUS_MAX_OUT_DATA_SIZE;
        sCryptoInlineStatusReadCmdParams.dwReadDataOffset  = 0;
        sCryptoInlineStatusReadCmdParams.presWriteData     = pResource;
        sCryptoInlineStatusReadCmdParams.dwWriteDataOffset = 0x20;

        switch (sBuildQueryStatusParams.QueryOperation)
        {
        case PAVP_QUERY_STATUS_MEMORY_MODE:
            sCryptoInlineStatusReadCmdParams.StatusReadType = CRYPTO_INLINE_MEMORY_STATUS_READ;
            break;
        case PAVP_QUERY_STATUS_CONNECTION_STATE:
            sCryptoInlineStatusReadCmdParams.StatusReadType = CRYPTO_INLINE_CONNECTION_STATE_READ;
            sCryptoInlineStatusReadCmdParams.Value.NonceIn  = sBuildQueryStatusParams.Value.NonceIn;
            break;
        case PAVP_QUERY_STATUS_FRESHNESS:
            sCryptoInlineStatusReadCmdParams.StatusReadType       = CRYPTO_INLINE_GET_FRESHNESS_READ;
            sCryptoInlineStatusReadCmdParams.Value.KeyRefreshType = sBuildQueryStatusParams.Value.KeyRefreshType;
            break;
        case PAVP_QUERY_STATUS_WIDI_COUNTER:
            sCryptoInlineStatusReadCmdParams.StatusReadType = CRYPTO_INLINE_GET_WIDI_COUNTER_READ;
            break;
        default:
            GPU_HAL_ASSERTMESSAGE("Unrecognized query status operation requested: %d.", sBuildQueryStatusParams.QueryOperation);
            hr = E_INVALIDARG;
            MT_ERR2(MT_CP_HAL_QUERY_STATUS, MT_CP_QUERY_OPERATION, sBuildQueryStatusParams.QueryOperation, MT_ERROR_CODE, hr);
            break;
        }

        if (FAILED(hr))
        {
            // In case of an error inside the 'switch' block, we need to also break from the do-while loop.
            goto finish;
        }

        GPU_HAL_CHK_STATUS_WITH_HR(mhwCp->AddMfxCryptoInlineStatusRead(
            m_pOsServices->m_pOsInterface, pCmdBuf, &sCryptoInlineStatusReadCmdParams));

        if (!m_bParsingAssistanceFromKmd)
        {
            GPU_HAL_CHK_STATUS_WITH_HR(m_HwInterface.pMiInterface->AddMiBatchBufferEnd(pCmdBuf, nullptr));
        }

        // Return unused command buffer space to OS.
        m_pOsServices->m_pOsInterface->pfnReturnCommandBuffer(m_pOsServices->m_pOsInterface, pCmdBuf, 0);

        // Submit the command buffer.
        hr = m_pOsServices->m_pOsInterface->pfnSubmitCommandBuffer(m_pOsServices->m_pOsInterface, pCmdBuf, FALSE);
        if (FAILED(hr))
        {
            GPU_HAL_ASSERTMESSAGE("Error submitting command to GPU. hr = 0x%x", hr);
            MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            goto finish;
        }

        pResBytes = static_cast<BYTE *>(m_pOsServices->LockResource(*pResource, CPavpOsServices::LockType::Read));
        if (pResBytes == nullptr)
        {
            GPU_HAL_ASSERTMESSAGE("Error locking the resource.");
            hr = E_FAIL;
            MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            goto finish;
        }

        // Wait if necessary for the hardware to finish writing the output.
        dwValueInMemory = *(reinterpret_cast<DWORD *>(pResBytes + PAVP_QUERY_STATUS_STORE_DWORD_OFFSET));

        if (dwValueInMemory != MHW_PAVP_QUERY_STATUS_STORE_DWORD_DATA)
        {
            DWORD dwWaitInMs = 0;

            // Poll every 1 ms until timeout.
            for (dwWaitInMs = PAVP_QUERY_STATUS_TIMEOUT; dwWaitInMs > 0; dwWaitInMs--)
            {
                GPU_HAL_VERBOSEMESSAGE("Trying to read status. Attempt #%d.", PAVP_QUERY_STATUS_TIMEOUT - dwWaitInMs + 1);

                dwValueInMemory = *(reinterpret_cast<DWORD *>(pResBytes + PAVP_QUERY_STATUS_STORE_DWORD_OFFSET));

                MosUtilities::MosSleep(1);

                GPU_HAL_VERBOSEMESSAGE("MEM = 0x%08x, Needed = 0x%08x.", dwValueInMemory, MHW_PAVP_QUERY_STATUS_STORE_DWORD_DATA);

                if (dwValueInMemory == MHW_PAVP_QUERY_STATUS_STORE_DWORD_DATA)
                {
                    break;
                }
            }

            // Timeout.
            if (dwWaitInMs == 0)
            {
                GPU_HAL_ASSERTMESSAGE("Error: Failed to read Memory Status output.");
                hr = E_FAIL;
                MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                goto finish;
            }
        }

        if (dwValueInMemory == MHW_PAVP_QUERY_STATUS_STORE_DWORD_DATA)
        {
            GPU_HAL_VERBOSEMESSAGE("Successfuly queried status.");
        }

        // Copy the result to the caller provided struct.
        GPU_HAL_CHK_STATUS_WITH_HR(MOS_SecureMemcpy(
            static_cast<void *>(&sBuildQueryStatusParams.OutData),
            sizeof(sBuildQueryStatusParams.OutData),
            pResBytes,
            sizeof(sBuildQueryStatusParams.OutData)));

        hr = m_pOsServices->m_pOsInterface->pfnUnlockResource(m_pOsServices->m_pOsInterface, pResource);
        if (FAILED(hr))
        {
            GPU_HAL_ASSERTMESSAGE("Error unlocking resource hr = 0x%x", hr);
            MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            goto finish;
        }

    } while (FALSE);

finish:

    if (pResource)
    {
        m_pOsServices->FreeResource(pResource);
    }

    // Failed -> discard all changes in Command Buffer
    if (FAILED(hr))
    {
        // Buffer overflow - display overflow size
        if (pCmdBuf->iRemaining < 0)
        {
            GPU_HAL_ASSERTMESSAGE("Command Buffer overflow by %d bytes", -pCmdBuf->iRemaining);
            MT_ERR2(MT_CP_CMD_BUFFER_OVERFLOW, MT_CP_CMD_BUFFER_REMAIN, pCmdBuf->iRemaining, MT_ERROR_CODE, hr);
        }

        // Move command buffer back to beginning
        i                   = iRemaining - pCmdBuf->iRemaining;
        pCmdBuf->iRemaining = iRemaining;
        pCmdBuf->iOffset -= i;
        pCmdBuf->pCmdPtr =
            pCmdBuf->pCmdBase + pCmdBuf->iOffset / sizeof(DWORD);

        // Return unused command buffer space to OS
        m_pOsServices->m_pOsInterface->pfnReturnCommandBuffer(m_pOsServices->m_pOsInterface, pCmdBuf, 0);
    }

    GPU_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpGpuHalLegacy::GetInfoToTranscryptKernels(
    const bool     bPreProduction,
    const uint32_t *&pTranscryptMetaData,
    uint32_t       &uiTranscryptMetaDataSize,
    const uint32_t *&pEncryptedKernels,
    uint32_t       &uiEncryptedKernelsSize)
{
    HRESULT                         hr                   = S_OK;
    MOS_STATUS                      eStatus              = MOS_STATUS_SUCCESS;
    MHW_PAVP_KERNEL_ENCRYPTION_TYPE kernelEncryptionType = MHW_PAVP_KERNEL_ENCRYPTION_TYPE_NUM;

    GPU_HAL_FUNCTION_ENTER;

    do
    {
        kernelEncryptionType = bPreProduction ? MHW_PAVP_KERNEL_ENCRYPTION_PREPRODUCTION : MHW_PAVP_KERNEL_ENCRYPTION_PRODUCTION;
        GPU_HAL_NORMALMESSAGE("Load Kernel type (0: production 1: pre-production) is %d", kernelEncryptionType);

        if (NULL != m_HwInterface.pPavpHwInterface)
        {
            MhwCpBase *pMhwCp = dynamic_cast<MhwCpBase *>(m_HwInterface.pPavpHwInterface);
            if (pMhwCp != NULL)
            {
                eStatus = pMhwCp->GetTranscryptKernelInfo(
                    &pTranscryptMetaData,
                    &uiTranscryptMetaDataSize,
                    &pEncryptedKernels,
                    &uiEncryptedKernelsSize,
                    kernelEncryptionType);

                if (eStatus != MOS_STATUS_SUCCESS)
                {
                    hr = S_FALSE;
                    GPU_HAL_ASSERTMESSAGE("Failed to get transcrypt kernel info:0x%x", eStatus);
                    MT_ERR2(MT_ERR_MOS_STATUS_CHECK, MT_ERROR_CODE, eStatus, MT_ERROR_CODE, hr);
                }
            }
            else
            {
                hr = S_FALSE;
                GPU_HAL_ASSERTMESSAGE("MHW CP Interface to Mhw CP Base cast failed");
                MT_ERR2(MT_CP_CAST_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            }
        }
        else
        {
            hr = S_FALSE;
            GPU_HAL_ASSERTMESSAGE("MHW CP is NULL");
            MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
        }
    } while (FALSE);

    GPU_HAL_FUNCTION_EXIT(hr);
    return hr;
}
