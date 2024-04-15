/*===================== begin_copyright_notice ==================================
INTEL CONFIDENTIAL
Copyright 2013-2023
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

File Name: cp_hal_gpu_xe_lpm_plus.cpp

Abstract:
Abstract base class for GPU HAL for PAVP functions

Environment:
OS agnostic - should support - Windows 10, Linux, Android
Platform specific - should support - Gen 13

Notes:
None

======================= end_copyright_notice ==================================*/

#include "cp_hal_gpu.h"
#include "cp_hal_gpu_xe_lpm_plus.h"
#include "cp_hal_gpu_defs.h"
#include "mhw_cp.h"
#include "mhw_mi.h"
#include "mos_utilities.h"
#include "cp_os_services.h"
#include <bitset>
#include <queue>

#define PAVP_REGISTER_SIZE 32

// Use these to mask the session in play register to get the session types in play.
#define PAVP_GEN13_DECODE_SESSION_IN_PLAY_MASK       0xFFFF
#define PAVP_GEN13_TRANSCODE_SESSION_IN_PLAY_MASK    0xFF0000
#define PAVP_GEN13_WIDI_SESSION_IN_PLAY_MASK         0x1000

//To determine min/max value for App Id for Decode and Transcode Sessions.
#define MIN_TRANSCODE_APP_ID   0x80
#define MAX_TRANSCODE_APP_ID   0x85
#define MIN_DECODE_APP_ID      0x0
#define MAX_DECODE_APP_ID      0xf

CPavpGpuHal13::CPavpGpuHal13(PLATFORM& sPlatform, MEDIA_WA_TABLE& sWaTable, HRESULT& hr, CPavpOsServices& OsServices)
                            : CPavpGpuHal(sPlatform, sWaTable, hr, OsServices)
{
    MOS_ZeroMemory(&m_CryptoKeyExParams, sizeof(m_CryptoKeyExParams));
    m_bIsKbKeyRefresh = TRUE;
    MhwInterfacesNext::CreateParams params;
    params.m_isCp    = true;
    m_pMhwInterfaces = MhwInterfacesNext::CreateFactory(params, m_pOsServices->m_pOsInterface);

    if (nullptr == m_pMhwInterfaces ||
        nullptr == m_pMhwInterfaces->m_cpInterface ||
        nullptr == m_pMhwInterfaces->m_miItf)
    {
        GPU_HAL_ASSERTMESSAGE("m_miItf or MhwCpInterface returned a NULL instance.");
        hr = E_INVALIDARG;
        MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
        return;
    }
}

HRESULT CPavpGpuHal13::BuildCryptoCopyClearToAesCounter(
    MOS_COMMAND_BUFFER                    *pCmdBuf,
    const PAVP_GPU_HAL_CRYPTO_COPY_PARAMS &sBuildCryptoCopyParams,
    bool                                   bWaitForCompletion)
{
    HRESULT       hr             = S_OK;
    MOS_STATUS    eStatus        = MOS_STATUS_UNKNOWN;
    MosCp        *pMosCp         = nullptr;
    MhwCpBase    *mhwCp          = nullptr;
    PMOS_RESOURCE pStoreResource = nullptr;
    uint32_t     *hwcounter      = nullptr;
    MOS_INTERFACE *pMos          = nullptr;

    GPU_HAL_FUNCTION_ENTER;

    GPU_HAL_CHK_NULL(pCmdBuf);
    GPU_HAL_CHK_NULL(m_pOsServices);
    GPU_HAL_CHK_NULL(m_pOsServices->m_pOsInterface);
    GPU_HAL_CHK_NULL(m_pOsServices->m_pOsInterface->osCpInterface);
    GPU_HAL_CHK_NULL(m_pMhwInterfaces->m_miItf);
    GPU_HAL_CHK_NULL(m_pMhwInterfaces->m_cpInterface);
    GPU_HAL_ASSERT(m_pOsServices->m_pOsInterface->osCpInterface->IsCpEnabled());

    pMosCp = dynamic_cast<MosCp *>(m_pOsServices->m_pOsInterface->osCpInterface);
    mhwCp  = dynamic_cast<MhwCpBase *>(m_pMhwInterfaces->m_cpInterface);
    pMos   = m_pOsServices->m_pOsInterface;

    GPU_HAL_CHK_NULL(pMosCp);
    GPU_HAL_CHK_NULL(mhwCp);

    do
    {
        SetGpuCryptoKeyExParams(mhwCp->GetEncryptionParams());
        GPU_HAL_CHK_STATUS_WITH_HR(m_pMhwInterfaces->m_cpInterface->AddProlog(m_pOsServices->m_pOsInterface, pCmdBuf));

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

        GPU_HAL_CHK_NULL(pMos->pfnGetResourceGfxAddress);
        uint32_t srcPageOffset = (pMos->pfnGetResourceGfxAddress(pMos, sBuildCryptoCopyParams.SrcResource) & (MHW_PAGE_SIZE - 1));
        uint32_t dstPageOffset = (pMos->pfnGetResourceGfxAddress(pMos, sBuildCryptoCopyParams.DstResource) & (MHW_PAGE_SIZE - 1));

        {  // Clear to Serpent
            MHW_PAVP_CRYPTO_COPY_ADDR_PARAMS CryptoCopyAddrParams = {0};
            CryptoCopyAddrParams.dwSize                           = MOS_ALIGN_CEIL(sBuildCryptoCopyParams.dwDataSize, 0x1000);
            CryptoCopyAddrParams.presSrc                          = sBuildCryptoCopyParams.SrcResource;
            CryptoCopyAddrParams.presDst                          = sBuildCryptoCopyParams.DstResource;
            GPU_HAL_CHK_STATUS_WITH_HR(mhwCp->AddMfxCryptoCopyBaseAddr(pCmdBuf, &CryptoCopyAddrParams));

            MHW_PAVP_CRYPTO_COPY_PARAMS sCryptoCopyCmdParams = {0};
            sCryptoCopyCmdParams.dwCopySize                  = sBuildCryptoCopyParams.dwDataSize;
            sCryptoCopyCmdParams.dwSrcBaseOffset             = srcPageOffset;
            sCryptoCopyCmdParams.dwDestBaseOffset            = dstPageOffset;
            sCryptoCopyCmdParams.ui8SelectionForIndData      = CRYPTO_COPY_TYPE_CLEAR_TO_SERPENT_ENCRYPT;
            GPU_HAL_CHK_STATUS_WITH_HR(mhwCp->AddMfxCryptoCopy(pCmdBuf, &sCryptoCopyCmdParams));
        }

        // adding MI_FLUSH_DW to flush any cache to dest resource.
        auto &flushDwParams = m_pMhwInterfaces->m_miItf->MHW_GETPAR_F(MI_FLUSH_DW)();
        flushDwParams       = {};
        GPU_HAL_CHK_STATUS_WITH_HR(m_pMhwInterfaces->m_miItf->MHW_ADDCMD_F(MI_FLUSH_DW)(pCmdBuf));

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
            sCryptoCopyCmdParams.dwSrcBaseOffset             = dstPageOffset;
            sCryptoCopyCmdParams.dwDestBaseOffset            = dstPageOffset;
            sCryptoCopyCmdParams.dwAesCounter[0]             = sBuildCryptoCopyParams.dwAesCtr[0];
            sCryptoCopyCmdParams.dwAesCounter[1]             = sBuildCryptoCopyParams.dwAesCtr[1];
            sCryptoCopyCmdParams.dwAesCounter[2]             = sBuildCryptoCopyParams.dwAesCtr[2];
            sCryptoCopyCmdParams.dwAesCounter[3]             = sBuildCryptoCopyParams.dwAesCtr[3];

            GPU_HAL_CHK_STATUS_WITH_HR(mhwCp->AddMfxCryptoCopy(pCmdBuf, &sCryptoCopyCmdParams));
        }

        auto &mfxWaitParams               = m_pMhwInterfaces->m_miItf->MHW_GETPAR_F(MFX_WAIT)();
        mfxWaitParams                     = {};
        mfxWaitParams.iStallVdboxPipeline = true;
        GPU_HAL_CHK_STATUS_WITH_HR(m_pMhwInterfaces->m_miItf->MHW_ADDCMD_F(MFX_WAIT)(pCmdBuf, nullptr));

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

            auto &flushDwParams             = m_pMhwInterfaces->m_miItf->MHW_GETPAR_F(MI_FLUSH_DW)();
            flushDwParams                   = {};
            flushDwParams.postSyncOperation = MHW_FLUSH_WRITE_IMMEDIATE_DATA;
            flushDwParams.pOsResource       = pStoreResource;
            flushDwParams.dwResourceOffset  = 0;
            flushDwParams.dwDataDW1         = MHW_PAVP_STORE_DATA_DWORD_DATA;
            GPU_HAL_CHK_STATUS_WITH_HR(m_pMhwInterfaces->m_miItf->MHW_ADDCMD_F(MI_FLUSH_DW)(pCmdBuf));
            GPU_HAL_VERBOSEMESSAGE("Successfully added MI_FLUSH_DW to command buffer.");
        }
        auto &batchBufferEndParams = m_pMhwInterfaces->m_miItf->MHW_GETPAR_F(MI_BATCH_BUFFER_END)();
        batchBufferEndParams       = {};
        GPU_HAL_CHK_STATUS_WITH_HR(m_pMhwInterfaces->m_miItf->MHW_ADDCMD_F(MI_BATCH_BUFFER_END)(pCmdBuf, nullptr));

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

HRESULT CPavpGpuHal13::BuildKeyExchange(
    PMOS_COMMAND_BUFFER                     pCmdBuf,
    const PAVP_GPU_HAL_KEY_EXCHANGE_PARAMS& sBuildKeyExchangeParams)
{
    HRESULT                       hr = S_OK;
    INT                           i = 0, iRemaining = 0;
    MI_FLUSH_DW_PAVP              *pMiFlushDw = NULL;
    BOOL                          bCacheCryptoKeyParam = FALSE;

    MOS_STATUS eStatus = MOS_STATUS_UNKNOWN;

    MosCp                         *pMosCp = nullptr;
    MhwCpBase                     *mhwCp = nullptr;

    GPU_HAL_FUNCTION_ENTER;

    if (m_pOsServices                                  == nullptr ||
        m_pOsServices->m_pOsInterface                  == nullptr ||
        m_pOsServices->m_pOsInterface->osCpInterface   == nullptr ||
        m_pMhwInterfaces->m_miItf                      == nullptr ||
        m_pMhwInterfaces->m_cpInterface                == nullptr ||
        pCmdBuf                                        == nullptr)
    {
        hr = E_UNEXPECTED;
        return hr;
    }

    pMosCp = dynamic_cast<MosCp*>(m_pOsServices->m_pOsInterface->osCpInterface);
    mhwCp  = dynamic_cast<MhwCpBase*>(m_pMhwInterfaces->m_cpInterface);

    if(mhwCp == nullptr || pMosCp == nullptr)
    {
        hr = E_UNEXPECTED;
        return hr;
    }

    do
    {
        // add CP prolog
        GPU_HAL_CHK_STATUS_WITH_HR(mhwCp->AddProlog(m_pOsServices->m_pOsInterface, pCmdBuf));

        // Insert CRYPTO_KEY_EXCHANGE_COMMAND.
        MHW_PAVP_MFX_CRYPTO_KEY_EXCHANGE_CMD_PARAMS sCryptoKeyExchangeCmdParams;
        MOS_ZeroMemory(&sCryptoKeyExchangeCmdParams, sizeof(sCryptoKeyExchangeCmdParams));

        switch (sBuildKeyExchangeParams.KeyExchangeType)
        {
        case PAVP_KEY_EXCHANGE_TYPE_TERMINATE:
            // Insert the command to terminate the key.
            sCryptoKeyExchangeCmdParams.KeyExchangeKeyUse = CRYPTO_TERMINATE_KEY;
            GPU_HAL_CHK_STATUS_WITH_HR(mhwCp->AddMfxCryptoKeyExchange(pCmdBuf, &sCryptoKeyExchangeCmdParams));

            // Clear cached key exchange params
            MOS_ZeroMemory(&m_CryptoKeyExParams, sizeof(m_CryptoKeyExParams));
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
                sCryptoKeyExchangeCmdParams.KeyExchangeKeyUse = CRYPTO_DECRYPT_AND_USE_NEW_KEY;
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
            bCacheCryptoKeyParam = TRUE;
            break;

        case PAVP_KEY_EXCHANGE_TYPE_KEY_ROTATION: 
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

            auto &flushDwParams = m_pMhwInterfaces->m_miItf->MHW_GETPAR_F(MI_FLUSH_DW)();
            flushDwParams       = {};
            GPU_HAL_CHK_STATUS_WITH_HR(m_pMhwInterfaces->m_miItf->MHW_ADDCMD_F(MI_FLUSH_DW)(pCmdBuf));

            GPU_HAL_CHK_STATUS_WITH_HR(MOS_SecureMemcpy(
                sCryptoKeyExchangeCmdParams.EncryptedDecryptionKey,
                sizeof(sCryptoKeyExchangeCmdParams.EncryptedDecryptionKey),
                sBuildKeyExchangeParams.EncryptedDecryptionRotationKey,
                sizeof(sBuildKeyExchangeParams.EncryptedDecryptionRotationKey)));

            GPU_HAL_CHK_STATUS_WITH_HR(mhwCp->AddMfxCryptoKeyExchange(pCmdBuf, &sCryptoKeyExchangeCmdParams));
            bCacheCryptoKeyParam = TRUE;
            break;
        }
        default:
            GPU_HAL_ASSERTMESSAGE("Unknown key exchange type requested: %d.", sBuildKeyExchangeParams.KeyExchangeType);
            MT_ERR1(MT_CP_KEY_EXCHANGE, MT_CP_KEY_EXCHANGE_TYPE, sBuildKeyExchangeParams.KeyExchangeType);
            break;
        }

        if (bCacheCryptoKeyParam)
        {
            GPU_HAL_NORMALMESSAGE("Saving crypto key ex params in GPU HAL");
            GPU_HAL_CHK_STATUS_WITH_HR(MOS_SecureMemcpy(
                &m_CryptoKeyExParams,
                sizeof(MHW_PAVP_MFX_CRYPTO_KEY_EXCHANGE_CMD_PARAMS),
                &sCryptoKeyExchangeCmdParams,
                sizeof(MHW_PAVP_MFX_CRYPTO_KEY_EXCHANGE_CMD_PARAMS)));
        }
        //Insert MFX_WAIT command.
        auto &mfxWaitParams               = m_pMhwInterfaces->m_miItf->MHW_GETPAR_F(MFX_WAIT)();
        mfxWaitParams                     = {};
        mfxWaitParams.iStallVdboxPipeline = true;
        GPU_HAL_CHK_STATUS_WITH_HR(m_pMhwInterfaces->m_miItf->MHW_ADDCMD_F(MFX_WAIT)(pCmdBuf, nullptr));

        // Insert MI_Flush_DW command with protection OFF.
        auto &flushDwParams = m_pMhwInterfaces->m_miItf->MHW_GETPAR_F(MI_FLUSH_DW)();
        flushDwParams       = {};
        GPU_HAL_CHK_STATUS_WITH_HR(m_pMhwInterfaces->m_miItf->MHW_ADDCMD_F(MI_FLUSH_DW)(pCmdBuf));

        GPU_HAL_VERBOSEMESSAGE("Parsing assistance from KMD = %d", m_bParsingAssistanceFromKmd);

        if (!m_bParsingAssistanceFromKmd)
        {
            auto &batchBufferEndParams = m_pMhwInterfaces->m_miItf->MHW_GETPAR_F(MI_BATCH_BUFFER_END)();
            batchBufferEndParams       = {};
            GPU_HAL_CHK_STATUS_WITH_HR(m_pMhwInterfaces->m_miItf->MHW_ADDCMD_F(MI_BATCH_BUFFER_END)(pCmdBuf, nullptr));
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
        i = iRemaining - pCmdBuf->iRemaining;
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

int32_t CPavpGpuHal13::BuildMultiCryptoCopy(
    MOS_COMMAND_BUFFER *                   pCmdBuf,
    PAVP_GPU_HAL_MULTI_CRYPTO_COPY_PARAMS *pBuildMultiCryptoCopyParams,
    size_t                                 cryptoCopyNumber)
{
    int32_t                hr             = 0;
    uint8_t *              pStoreResBytes = nullptr;
    PMOS_RESOURCE          pStoreResource = nullptr;
    MOS_STATUS             eStatus         = MOS_STATUS_UNKNOWN;
    int32_t                i               = 0;
    int32_t                iRemaining      = 0;
    volatile uint32_t      dwValueInMemory = 0;
    MosCp *                pMosCp          = nullptr;
    MhwCpBase *            mhwCp           = nullptr;

    GPU_HAL_FUNCTION_ENTER;

    if (pCmdBuf == nullptr || pBuildMultiCryptoCopyParams == nullptr)
    {
        GPU_HAL_ASSERTMESSAGE("Invalid parameters passed in to BuildMultiCryptoCopy.");
        hr = E_UNEXPECTED;
        MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
        return hr;
    }

    if (m_pOsServices                                   == nullptr ||
        m_pOsServices->m_pOsInterface                   == nullptr ||
        m_pOsServices->m_pOsInterface->osCpInterface    == nullptr ||
        m_pMhwInterfaces->m_miItf                       == nullptr ||
        m_pMhwInterfaces->m_cpInterface                 == nullptr)
    {
        GPU_HAL_ASSERTMESSAGE("Os services or HwInterface have not been created.");
        hr = E_UNEXPECTED;
        MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
        return hr;
    }

    pMosCp = dynamic_cast<MosCp *>(m_pOsServices->m_pOsInterface->osCpInterface);
    mhwCp  = dynamic_cast<MhwCpBase*>(m_pMhwInterfaces->m_cpInterface);

    if (pMosCp == nullptr || mhwCp == nullptr)
    {
        GPU_HAL_ASSERTMESSAGE("pMosCp or mhwCp is nullptr.");
        hr = E_UNEXPECTED;
        MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
        return hr;
    }

    do
    {
        // Update MHW with crypto key exchange command(s) if required
        SetGpuCryptoKeyExParams(mhwCp->GetEncryptionParams());

        // add CP prolog
        GPU_HAL_CHK_STATUS_WITH_HR(m_pMhwInterfaces->m_cpInterface->AddProlog(m_pOsServices->m_pOsInterface, pCmdBuf));

        for (size_t i = 0; i < cryptoCopyNumber; i++)
        {
            // The hardware hangs on a zero size crypto copy so submit a no-op instead and break.
            if (pBuildMultiCryptoCopyParams->dwDataSize == 0)
            {
                GPU_HAL_ASSERTMESSAGE("0 size crypto copy requested. Adding NOOP command to buffer.");
                auto &miNoopParams = m_pMhwInterfaces->m_miItf->MHW_GETPAR_F(MI_NOOP)();
                miNoopParams       = {};
                GPU_HAL_CHK_STATUS_WITH_HR(m_pMhwInterfaces->m_miItf->MHW_ADDCMD_F(MI_NOOP)(pCmdBuf, nullptr));
                break;
            }

            // Insert MFX_CRYPTO_COPY_BASE_ADDR command.
            MHW_PAVP_CRYPTO_COPY_ADDR_PARAMS sCryptoCopyAddrParams;
            MOS_ZeroMemory(&sCryptoCopyAddrParams, sizeof(sCryptoCopyAddrParams));
            sCryptoCopyAddrParams.bIsDestRenderTarget = false;
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
            auto &flushDwParams = m_pMhwInterfaces->m_miItf->MHW_GETPAR_F(MI_FLUSH_DW)();
            flushDwParams       = {};
            GPU_HAL_CHK_STATUS_WITH_HR(m_pMhwInterfaces->m_miItf->MHW_ADDCMD_F(MI_FLUSH_DW)(pCmdBuf));

            pBuildMultiCryptoCopyParams++;
        }

        //Insert MFX_WAIT command.
        auto &mfxWaitParams               = m_pMhwInterfaces->m_miItf->MHW_GETPAR_F(MFX_WAIT)();
        mfxWaitParams                     = {};
        mfxWaitParams.iStallVdboxPipeline = true;
        GPU_HAL_CHK_STATUS_WITH_HR(m_pMhwInterfaces->m_miItf->MHW_ADDCMD_F(MFX_WAIT)(pCmdBuf, nullptr));

        // Insert MI_FLUSH_DW command to wait for completion
        {
            // Allocate a resource for the output.
            GPU_HAL_VERBOSEMESSAGE("Allocating resources required to wait for CryptoCopy completion...");
            hr = m_pOsServices->AllocateLinearResource(
                MOS_GFXRES_BUFFER,  // eType
                sizeof(uint32_t),   // dwWidth
                1,                  // dwHeight
                __FUNCTION__,       // pResourceName
                Format_Buffer,      // eFormat
                pStoreResource);

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
                sizeof(uint32_t),
                PAVP_QUERY_STATUS_BUFFER_INITIAL_VALUE);
            if (FAILED(hr))
            {
                GPU_HAL_ASSERTMESSAGE("Failed to fill resource memory. Error = 0x%x.", hr);
                MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }
            GPU_HAL_VERBOSEMESSAGE("Successfully allocated resources required to wait for CryptoCopy completion. Adding MI_FLUSH_DW(PostSync) to command buffer...");

            auto &flushDwParams = m_pMhwInterfaces->m_miItf->MHW_GETPAR_F(MI_FLUSH_DW)();
            flushDwParams       = {};
            flushDwParams.postSyncOperation = MHW_FLUSH_WRITE_IMMEDIATE_DATA;
            flushDwParams.pOsResource       = pStoreResource;
            flushDwParams.dwResourceOffset  = 0;
            flushDwParams.dwDataDW1         = MHW_PAVP_STORE_DATA_DWORD_DATA;
            GPU_HAL_CHK_STATUS_WITH_HR(m_pMhwInterfaces->m_miItf->MHW_ADDCMD_F(MI_FLUSH_DW)(pCmdBuf));
            GPU_HAL_VERBOSEMESSAGE("Successfully added MI_FLUSH_DW to command buffer.");
        }

        if (!m_bParsingAssistanceFromKmd)
        {
            auto &batchBufferEndParams = m_pMhwInterfaces->m_miItf->MHW_GETPAR_F(MI_BATCH_BUFFER_END)();
            batchBufferEndParams       = {};
            GPU_HAL_CHK_STATUS_WITH_HR(m_pMhwInterfaces->m_miItf->MHW_ADDCMD_F(MI_BATCH_BUFFER_END)(pCmdBuf, nullptr));
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

        // Wait for store data command to finish
        {
            pStoreResBytes = static_cast<uint8_t *>(m_pOsServices->LockResource(*pStoreResource, CPavpOsServices::LockType::Read));
            if (pStoreResBytes == nullptr)
            {
                GPU_HAL_ASSERTMESSAGE("Error locking the resource.");
                hr = E_FAIL;
                MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }

            // Wait if necessary for the hardware to finish writing the output.
            uint32_t dwWaitInMs = PAVP_QUERY_STATUS_TIMEOUT;
            GPU_HAL_VERBOSEMESSAGE("Trying to read data. Attempt #%d.", PAVP_QUERY_STATUS_TIMEOUT - dwWaitInMs + 1);
            dwValueInMemory = *(reinterpret_cast<uint32_t *>(pStoreResBytes));

            if (dwValueInMemory != MHW_PAVP_STORE_DATA_DWORD_DATA)
            {
                GPU_HAL_VERBOSEMESSAGE("MEM = 0x%08x, Needed = 0x%08x.", dwValueInMemory, MHW_PAVP_STORE_DATA_DWORD_DATA);

                // Poll every 1 ms until timeout.
                for (dwWaitInMs = PAVP_QUERY_STATUS_TIMEOUT; dwWaitInMs > 0; dwWaitInMs--)
                {
                    GPU_HAL_VERBOSEMESSAGE("Trying to read status. Attempt #%d.", PAVP_QUERY_STATUS_TIMEOUT - dwWaitInMs + 1);

                    dwValueInMemory = *(reinterpret_cast<uint32_t *>(pStoreResBytes));

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

        m_pOsServices->SyncOnSubmit(pCmdBuf, true);

    } while (false);

finish:
    if (pStoreResource)
    {
        m_pOsServices->FreeResource(pStoreResource);
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
            pCmdBuf->pCmdBase + pCmdBuf->iOffset / sizeof(uint32_t);

        // Return unused command buffer space to OS
        m_pOsServices->m_pOsInterface->pfnReturnCommandBuffer(m_pOsServices->m_pOsInterface, pCmdBuf, 0);
    }

    GPU_HAL_FUNCTION_EXIT(hr);
    return hr;
}

bool CPavpGpuHal13::CheckAudioCryptoCopyCounterRollover(
    PAVP_GPU_HAL_MULTI_CRYPTO_COPY_PARAMS sCryptoCopyParam,
    uint32_t                              &numBytesBeforeRollover,
    uint32_t                              &remainingSize)
{
    bool                bSplitCryptoCopy = false;
    std::array<bool, 4> overflowBits     = {false, false, false, false};

    if (MEDIA_IS_WA(&m_sWaTable, WaCheckCryptoCopyRollover) &&
        (sCryptoCopyParam.dwAppId & TRANSCODE_APP_ID_MASK))
    {
        // dwAesCounter[0] maps to CC DW6 (low) and dwAesCounter[1] maps to CC DW7 (high)
        bSplitCryptoCopy = CheckCryptoCopyCounterRollover(
            sCryptoCopyParam.dwDataSize,
            sCryptoCopyParam.dwAesCtr[0],
            sCryptoCopyParam.dwAesCtr[1],
            overflowBits);
    }

    if (bSplitCryptoCopy)
    {
        GPU_HAL_VERBOSEMESSAGE("Crypto copy rollover detected, need to split submissions");

        // According to the BSpec, only two crypto copies need to be submitted

        // Explanation from HW:
        // Basic idea is that HW doesn't rollover in the below bits
        //    bit 59->bit 60
        //    bit 60->bit 61
        //    bit 61->bit 62
        //    bit 62->bit 63
        // So software has to do the rollover. So SW needs to set bit 60 and program HW.

        // Calculate the "midpoint" (rollover value) based on which bits rolled over
        uint64_t counterAfterRollover = 0;

        for (size_t i = 0; i < overflowBits.size(); i++)
        {
            if (overflowBits[i])
            {
                counterAfterRollover |= (1ULL << (i + PAVP_GEN13_FIRST_RESERVED_COUNTER_BIT));
            }
        }

        uint32_t totalSize      = sCryptoCopyParam.dwDataSize;
        uint32_t numBlocks      = totalSize / MHW_PAVP_AES128_NUM_BYTES_IN_BLOCK;
        uint64_t currentCounter = ((static_cast<uint64_t>(sCryptoCopyParam.dwAesCtr[1]) << PAVP_SHIFT_UINT32_TO_UINT64) | sCryptoCopyParam.dwAesCtr[0]);
        uint64_t finalCounter   = currentCounter + numBlocks;

        uint64_t numBlocksBeforeRollover = counterAfterRollover - currentCounter;
        uint64_t numBlocksAfterRollover  = finalCounter - counterAfterRollover;

        numBytesBeforeRollover = static_cast<uint32_t>(numBlocksBeforeRollover * MHW_PAVP_AES128_NUM_BYTES_IN_BLOCK);
        remainingSize          = totalSize - numBytesBeforeRollover;

        GPU_HAL_VERBOSEMESSAGE("Total size = %d bytes, of which %d bytes (%d blocks) are before rollover and %d bytes (%d blocks) are after rollover",
            totalSize,
            numBytesBeforeRollover,
            numBlocksBeforeRollover,
            remainingSize,
            numBlocksAfterRollover);
    }
    else
    {
        GPU_HAL_VERBOSEMESSAGE("No crypto copy rollover detected, no need to split submissions");
    }

    return bSplitCryptoCopy;
}

HRESULT CPavpGpuHal13::BuildCryptoCopy(
    MOS_COMMAND_BUFFER                      *pCmdBuf,
    const PAVP_GPU_HAL_CRYPTO_COPY_PARAMS&  sBuildCryptoCopyParams)
{
    HRESULT                     hr = S_OK;
    PBYTE                       pStoreResBytes = nullptr;
    PMOS_RESOURCE               pStoreResource = nullptr;
    PVOID                       pResBytes = nullptr;
    PMOS_RESOURCE               pResource = nullptr;
    MOS_STATUS                  eStatus = MOS_STATUS_UNKNOWN;
    INT                         i = 0, iRemaining = 0;
    volatile DWORD              dwValueInMemory = 0;
    MI_FLUSH_DW_PAVP            *pMiFlushDw = NULL;
    MosCp                       *pMosCp = nullptr;
    MhwCpBase                   *mhwCp = nullptr;
    BOOL                        bSplitCryptoCopy = FALSE;
    DWORD                       RemainingSize = 0;
    DWORD                       NumBytesBeforeRollover = 0;
    PMOS_RESOURCE               pSecondSrcResource = nullptr;
    PMOS_RESOURCE               pSecondDestResource = nullptr;
    BOOL                        didLockOriginalSrcResource = FALSE;
    BOOL                        didLockSecondSrcResource = FALSE;
    BOOL                        didLockOriginalDestResource = FALSE;
    BOOL                        didLockSecondDestResource = FALSE;

    std::queue<MHW_PAVP_CRYPTO_COPY_PARAMS>         CryptoCopyParams;
    std::queue<MHW_PAVP_CRYPTO_COPY_ADDR_PARAMS>    CryptoCopyAddrParams;
    std::array<bool, 4>                             OverflowBits = { false, false, false, false };

    GPU_HAL_FUNCTION_ENTER;

    if (m_pOsServices                                  == nullptr ||
        m_pOsServices->m_pOsInterface                  == nullptr ||
        m_pOsServices->m_pOsInterface->osCpInterface   == nullptr ||
        m_pMhwInterfaces->m_miItf                      == nullptr ||
        m_pMhwInterfaces->m_cpInterface                == nullptr ||
        pCmdBuf                                        == nullptr)
    {
        hr = E_UNEXPECTED;
        return hr;
    }

    pMosCp = dynamic_cast<MosCp*>(m_pOsServices->m_pOsInterface->osCpInterface);
    mhwCp  = dynamic_cast<MhwCpBase*>(m_pMhwInterfaces->m_cpInterface);

    if(pMosCp == nullptr || mhwCp == nullptr)
    {
        hr = E_UNEXPECTED;
        return hr;
    }

    do
    {
        if (sBuildCryptoCopyParams.eCryptoCopyType == CRYPTO_COPY_TYPE_CLEAR_TO_AES_ENCRYPT)
        {
            hr = BuildCryptoCopyClearToAesCounter(pCmdBuf, sBuildCryptoCopyParams, true);
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
            auto &miNoopParams = m_pMhwInterfaces->m_miItf->MHW_GETPAR_F(MI_NOOP)();
            miNoopParams       = {};
            GPU_HAL_CHK_STATUS_WITH_HR(m_pMhwInterfaces->m_miItf->MHW_ADDCMD_F(MI_NOOP)(pCmdBuf, nullptr));
            break;
        }

        // Update MHW with crypto key exchange command(s) if required
        SetGpuCryptoKeyExParams(mhwCp->GetEncryptionParams());

        // add CP prolog
        GPU_HAL_CHK_STATUS_WITH_HR(mhwCp->AddProlog(m_pOsServices->m_pOsInterface, pCmdBuf));

        // Patching for destination allocation.
        if (sBuildCryptoCopyParams.eCryptoCopyType == CRYPTO_COPY_TYPE_SET_PLANE_ENABLE)
        {
            // Allocate a resource for the ped packet information.
            hr = m_pOsServices->AllocateLinearResource(
                MOS_GFXRES_BUFFER,          // eType
                sBuildCryptoCopyParams.dwDataSize,  // dwWidth
                1,                          // dwHeight
                __FUNCTION__,               // pResourceName
                Format_Buffer,              // eFormat
                pResource);                 // pResource
            if (FAILED(hr))
            {
                GPU_HAL_ASSERTMESSAGE("Error allocating resource. Error = 0x%x.", hr);
                MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }

            if (pResource == nullptr)
            {
                GPU_HAL_ASSERTMESSAGE("Error allocating the resource");
                hr = E_FAIL;
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

        // Populate common attributes for MFX_CRYPTO_COPY_BASE_ADDR command(s).
        MHW_PAVP_CRYPTO_COPY_ADDR_PARAMS sCryptoCopyAddrParams;
        MOS_ZeroMemory(&sCryptoCopyAddrParams, sizeof(sCryptoCopyAddrParams));
        sCryptoCopyAddrParams.bIsDestRenderTarget = FALSE;
        sCryptoCopyAddrParams.dwSize = MOS_ALIGN_CEIL(sBuildCryptoCopyParams.dwDataSize, 0x1000);

        if (sBuildCryptoCopyParams.eCryptoCopyType != CRYPTO_COPY_TYPE_SET_PLANE_ENABLE)
        {
            sCryptoCopyAddrParams.presSrc = sBuildCryptoCopyParams.SrcResource;
            sCryptoCopyAddrParams.presDst = sBuildCryptoCopyParams.DstResource;
        }
        else
        {
            sCryptoCopyAddrParams.presSrc = pResource;
        }

        // Always add first MFX_CRYPTO_COPY_BASE_ADDR command.
        CryptoCopyAddrParams.push(sCryptoCopyAddrParams);

        // Populate common attributes for MFX_CRYPTO_COPY command(s).
        MHW_PAVP_CRYPTO_COPY_PARAMS sCryptoCopyCmdParams;
        MOS_ZeroMemory(&sCryptoCopyCmdParams, sizeof(sCryptoCopyCmdParams));
        sCryptoCopyCmdParams.AesMode = sBuildCryptoCopyParams.eAesMode;
        sCryptoCopyCmdParams.dwCopySize = sBuildCryptoCopyParams.dwDataSize;
        sCryptoCopyCmdParams.ui8SelectionForIndData = sBuildCryptoCopyParams.eCryptoCopyType;
        GPU_HAL_CHK_STATUS_WITH_HR(MOS_SecureMemcpy(
            sCryptoCopyCmdParams.dwAesCounter,
            sizeof(sCryptoCopyCmdParams.dwAesCounter),
            sBuildCryptoCopyParams.dwAesCtr,
            sizeof(sBuildCryptoCopyParams.dwAesCtr)));

        if (sBuildCryptoCopyParams.eCryptoCopyType == CRYPTO_COPY_TYPE_AES_DECRYPT_PARTIAL)
        {
            sCryptoCopyCmdParams.uiEncryptedBytes = sBuildCryptoCopyParams.uiEncryptedBytes;
            sCryptoCopyCmdParams.uiClearBytes = sBuildCryptoCopyParams.uiClearBytes;
        }

        // Always add first MFX_CRYPTO_COPY command.
        CryptoCopyParams.push(sCryptoCopyCmdParams);

        if (MEDIA_IS_WA(&m_sWaTable, WaCheckCryptoCopyRollover) &&
           (sBuildCryptoCopyParams.dwAppId & TRANSCODE_APP_ID_MASK) &&
           (sBuildCryptoCopyParams.eCryptoCopyType == CRYPTO_COPY_TYPE_SERPENT_TO_AES_ENCRYPT))
        {
            GPU_HAL_VERBOSEMESSAGE("Performing crypto copy rollover check for clear/serpent->AES crypto copy");

            // dwAesCounter[0] maps to CC DW6 (low) and dwAesCounter[1] maps to CC DW7 (high)
            bSplitCryptoCopy = CheckCryptoCopyCounterRollover(
                sCryptoCopyCmdParams.dwCopySize,
                sCryptoCopyCmdParams.dwAesCounter[0],
                sCryptoCopyCmdParams.dwAesCounter[1],
                OverflowBits);

            if (bSplitCryptoCopy)
            {
                GPU_HAL_VERBOSEMESSAGE("Crypto copy rollover detected, need to split submissions");

                // According to the BSpec, only two crypto copies need to be submitted

                // Explanation from HW:
                // Basic idea is that HW doesn't rollover in the below bits
                //    bit 59->bit 60
                //    bit 60->bit 61
                //    bit 61->bit 62
                //    bit 62->bit 63
                // So software has to do the rollover. So SW needs to set bit 60 and program HW.

                // Calculate the "midpoint" (rollover value) based on which bits rolled over
                uint64_t CounterAfterRollover = 0;
                for (size_t i = 0; i < OverflowBits.size(); i++)
                {
                    if (OverflowBits[i])
                    {
                        CounterAfterRollover |= (1ULL << (i + PAVP_GEN13_FIRST_RESERVED_COUNTER_BIT));
                    }
                }

                DWORD TotalSize = sCryptoCopyCmdParams.dwCopySize;
                uint32_t NumBlocks = TotalSize / MHW_PAVP_AES128_NUM_BYTES_IN_BLOCK;
                uint64_t CurrentCounter = ((static_cast<uint64_t>(sCryptoCopyCmdParams.dwAesCounter[1]) << PAVP_SHIFT_UINT32_TO_UINT64) | sCryptoCopyCmdParams.dwAesCounter[0]);
                uint64_t FinalCounter = CurrentCounter + NumBlocks;

                uint64_t NumBlocksBeforeRollover = CounterAfterRollover - CurrentCounter;
                uint64_t NumBlocksAfterRollover = FinalCounter - CounterAfterRollover;

                NumBytesBeforeRollover = static_cast<DWORD>(NumBlocksBeforeRollover * MHW_PAVP_AES128_NUM_BYTES_IN_BLOCK);
                RemainingSize = TotalSize - NumBytesBeforeRollover;

                GPU_HAL_VERBOSEMESSAGE("Total size = %d bytes, of which %d bytes (%d blocks) are before rollover and %d bytes (%d blocks) are after rollover",
                    TotalSize, NumBytesBeforeRollover, NumBlocksBeforeRollover, RemainingSize, NumBlocksAfterRollover);

                auto aesCtr = sCryptoCopyCmdParams.dwAesCounter;
                GPU_HAL_VERBOSEMESSAGE("Counter for first crypto copy submission is 0x%08x 0x%08x 0x%08x 0x%08x",
                    aesCtr[0], aesCtr[1], aesCtr[2], aesCtr[3]);
                GPU_HAL_VERBOSEMESSAGE("Encrypting %d bytes in first crypto copy submission", NumBytesBeforeRollover);
                CryptoCopyParams.front().dwSrcBaseOffset = 0;
                CryptoCopyParams.front().dwDestBaseOffset = 0;
                CryptoCopyParams.front().dwCopySize = NumBytesBeforeRollover;

                // Check to avoid submitting 0-byte crypto copy for second submission
                if (RemainingSize > 0)
                {
                    // Create source and destination resources for the second crypto copy submission.
                    hr = m_pOsServices->AllocateLinearResource(
                        MOS_GFXRES_BUFFER,
                        RemainingSize,
                        1,
                        __FUNCTION__,
                        Format_Buffer,
                        pSecondSrcResource);
                    if (FAILED(hr) || pSecondSrcResource == nullptr)
                    {
                        GPU_HAL_ASSERTMESSAGE("Failed to allocate source resource for second crypto copy, hr = 0x%08x", hr);
                        MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                        break;
                    }

                    hr = m_pOsServices->AllocateLinearResource(
                        MOS_GFXRES_BUFFER,
                        RemainingSize,
                        1,
                        __FUNCTION__,
                        Format_Buffer,
                        pSecondDestResource);
                    if (FAILED(hr) || pSecondDestResource == nullptr)
                    {
                        GPU_HAL_ASSERTMESSAGE("Failed to allocate destination resource for second crypto copy, hr = 0x%08x", hr);
                        MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                        break;
                    }

                    // Lock original source and second source resources to copy the appropriate number of bytes over.
                    uint8_t* pOriginalSrc = static_cast<uint8_t*>(m_pOsServices->LockResource(
                        *sBuildCryptoCopyParams.SrcResource,
                        CPavpOsServices::LockType::Read));
                    if (pOriginalSrc == nullptr)
                    {
                        GPU_HAL_ASSERTMESSAGE("Failed to lock original source resource for reading");
                        hr = E_UNEXPECTED;
                        MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                        break;
                    }

                    didLockOriginalSrcResource = TRUE;

                    uint8_t* pSecondSrc = static_cast<uint8_t*>(m_pOsServices->LockResource(
                        *pSecondSrcResource,
                        CPavpOsServices::LockType::Write));
                    if (pSecondSrc == nullptr)
                    {
                        GPU_HAL_ASSERTMESSAGE("Failed to lock second source resource for writing");
                        hr = E_UNEXPECTED;
                        MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                        break;
                    }

                    didLockSecondSrcResource = TRUE;

                    GPU_HAL_CHK_STATUS_WITH_HR(MOS_SecureMemcpy(
                        pSecondSrc,
                        RemainingSize,
                        pOriginalSrc + NumBytesBeforeRollover,
                        RemainingSize));

                    // Unlock original source and second source resources.
                    m_pOsServices->m_pOsInterface->pfnUnlockResource(
                        m_pOsServices->m_pOsInterface,
                        sBuildCryptoCopyParams.SrcResource);

                    didLockOriginalSrcResource = FALSE;

                    m_pOsServices->m_pOsInterface->pfnUnlockResource(
                        m_pOsServices->m_pOsInterface,
                        pSecondSrcResource);

                    didLockSecondSrcResource = FALSE;

                    // Add second MFX_CRYPTO_COPY_BASE_ADDR command.
                    sCryptoCopyAddrParams.presSrc = pSecondSrcResource;
                    sCryptoCopyAddrParams.presDst = pSecondDestResource;
                    sCryptoCopyAddrParams.dwSize = MOS_ALIGN_CEIL(RemainingSize, 0x1000);
                    CryptoCopyAddrParams.push(sCryptoCopyAddrParams);

                    GPU_HAL_VERBOSEMESSAGE("Incrementing starting counter by %d blocks", NumBlocksBeforeRollover);
                    mhwCp->IncrementCounter(sCryptoCopyCmdParams.dwAesCounter, static_cast<uint32_t>(NumBlocksBeforeRollover));

                    auto aesCtrPostIncrement = sCryptoCopyCmdParams.dwAesCounter;
                    GPU_HAL_VERBOSEMESSAGE("Counter for second crypto copy submission is 0x%08x 0x%08x 0x%08x 0x%08x",
                        aesCtrPostIncrement[0], aesCtrPostIncrement[1], aesCtrPostIncrement[2], aesCtrPostIncrement[3]);
                    GPU_HAL_VERBOSEMESSAGE("Encrypting %d bytes in second crypto copy submission", RemainingSize);
                    sCryptoCopyCmdParams.dwSrcBaseOffset = 0;
                    sCryptoCopyCmdParams.dwDestBaseOffset = 0;
                    sCryptoCopyCmdParams.dwCopySize = RemainingSize;
                    CryptoCopyParams.push(sCryptoCopyCmdParams);
                }
            }
            else
            {
                GPU_HAL_VERBOSEMESSAGE("No crypto copy rollover detected, no need to split submissions");
            }
        }
        else
        {
            GPU_HAL_VERBOSEMESSAGE("Crypto copy rollover check not required");
        }

        if (CryptoCopyAddrParams.size() != CryptoCopyParams.size())
        {
            GPU_HAL_ASSERTMESSAGE(
                "Mismatched number of MFX_CRYPTO_COPY_BASE_ADDR commands (%d) and MFX_CRYPTO_COPY commands (%d)!",
                CryptoCopyAddrParams.size(),
                CryptoCopyParams.size());
            hr = E_UNEXPECTED;
            MT_ERR3(MT_CP_CRYPT_COPY_PARAM, MT_CP_CRYPT_COPY_ADDR_CMD, static_cast<int64_t>(CryptoCopyAddrParams.size()),
                                 MT_CP_CRYPT_COPY_CMD, static_cast<int64_t>(CryptoCopyParams.size()), MT_ERROR_CODE, hr);
            break;
        }

        while (!CryptoCopyAddrParams.empty() && !CryptoCopyParams.empty())
        {
            auto sCryptoCopyAddrParams = CryptoCopyAddrParams.front();
            GPU_HAL_CHK_STATUS_WITH_HR(mhwCp->AddMfxCryptoCopyBaseAddr(pCmdBuf, &sCryptoCopyAddrParams));
            CryptoCopyAddrParams.pop();

            MOS_INTERFACE *pMos = m_pOsServices->m_pOsInterface;
            GPU_HAL_CHK_NULL(pMos->pfnGetResourceGfxAddress);
            uint32_t srcPageOffset = (pMos->pfnGetResourceGfxAddress(pMos, sCryptoCopyAddrParams.presSrc) & (MHW_PAGE_SIZE - 1));
            uint32_t dstPageOffset = (pMos->pfnGetResourceGfxAddress(pMos, sCryptoCopyAddrParams.presDst) & (MHW_PAGE_SIZE - 1));

            auto sCryptoCopyCmdParams = CryptoCopyParams.front();
            sCryptoCopyCmdParams.dwSrcBaseOffset = srcPageOffset;
            sCryptoCopyCmdParams.dwDestBaseOffset = dstPageOffset;

            GPU_HAL_CHK_STATUS_WITH_HR(mhwCp->AddMfxCryptoCopy(pCmdBuf, &sCryptoCopyCmdParams));
            CryptoCopyParams.pop();
        }

        // Insert MI_Flush_DW command with protection OFF.
        auto &flushDwParams = m_pMhwInterfaces->m_miItf->MHW_GETPAR_F(MI_FLUSH_DW)();
        flushDwParams       = {};
        GPU_HAL_CHK_STATUS_WITH_HR(m_pMhwInterfaces->m_miItf->MHW_ADDCMD_F(MI_FLUSH_DW)(pCmdBuf));

        //Insert MFX_WAIT command.
        auto &mfxWaitParams               = m_pMhwInterfaces->m_miItf->MHW_GETPAR_F(MFX_WAIT)();
        mfxWaitParams                     = {};
        mfxWaitParams.iStallVdboxPipeline = true;
        GPU_HAL_CHK_STATUS_WITH_HR(m_pMhwInterfaces->m_miItf->MHW_ADDCMD_F(MFX_WAIT)(pCmdBuf, nullptr));
        // Insert MI_FLUSH_DW command to wait for completion
        {
            // Allocate a resource for the output.
            GPU_HAL_VERBOSEMESSAGE("Allocating resources required to wait for CryptoCopy completion...");
            hr = m_pOsServices->AllocateLinearResource(
                MOS_GFXRES_BUFFER,                      // eType
                sizeof(DWORD),                          // dwWidth
                1,                                      // dwHeight
                __FUNCTION__,                           // pResourceName
                Format_Buffer,                          // eFormat
                pStoreResource);

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

            auto &flushDwParams = m_pMhwInterfaces->m_miItf->MHW_GETPAR_F(MI_FLUSH_DW)();
            flushDwParams       = {};
            flushDwParams.postSyncOperation = MHW_FLUSH_WRITE_IMMEDIATE_DATA;
            flushDwParams.pOsResource = pStoreResource;
            flushDwParams.dwResourceOffset = 0;
            flushDwParams.dwDataDW1 = MHW_PAVP_STORE_DATA_DWORD_DATA;
            GPU_HAL_CHK_STATUS_WITH_HR(m_pMhwInterfaces->m_miItf->MHW_ADDCMD_F(MI_FLUSH_DW)(pCmdBuf));
            GPU_HAL_VERBOSEMESSAGE("Successfully added MI_FLUSH_DW to command buffer.");
        }

        if (!m_bParsingAssistanceFromKmd)
        {
            auto &batchBufferEndParams = m_pMhwInterfaces->m_miItf->MHW_GETPAR_F(MI_BATCH_BUFFER_END)();
            batchBufferEndParams       = {};
            GPU_HAL_CHK_STATUS_WITH_HR(m_pMhwInterfaces->m_miItf->MHW_ADDCMD_F(MI_BATCH_BUFFER_END)(pCmdBuf, nullptr));
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

        // Wait for store data command to finish
        {
            pStoreResBytes = static_cast<BYTE *>(m_pOsServices->LockResource(*pStoreResource, CPavpOsServices::LockType::Read));
            if (pStoreResBytes == nullptr)
            {
                GPU_HAL_ASSERTMESSAGE("Error locking the resource.");
                hr = E_FAIL;
                MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }

            // Wait if necessary for the hardware to finish writing the output.
            DWORD dwWaitInMs = PAVP_QUERY_STATUS_TIMEOUT;
            GPU_HAL_VERBOSEMESSAGE("Trying to read data. Attempt #%d.", PAVP_QUERY_STATUS_TIMEOUT - dwWaitInMs + 1);
            dwValueInMemory = *(reinterpret_cast<DWORD *>(pStoreResBytes));

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

        if (bSplitCryptoCopy && RemainingSize > 0)
        {
            GPU_HAL_VERBOSEMESSAGE("Copying %d bytes back to original destination resource", RemainingSize);

            // Lock original destination and second destination resources to copy the appropriate number of bytes back.
            uint8_t* pOriginalDest = static_cast<uint8_t*>(m_pOsServices->LockResource(
                *sBuildCryptoCopyParams.DstResource,
                CPavpOsServices::LockType::Write));
            if (pOriginalDest == nullptr)
            {
                GPU_HAL_ASSERTMESSAGE("Failed to lock original destination resource for writing");
                hr = E_UNEXPECTED;
                MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }

            didLockOriginalDestResource = TRUE;

            uint8_t* pSecondDest = static_cast<uint8_t*>(m_pOsServices->LockResource(
                *pSecondDestResource,
                CPavpOsServices::LockType::Read));
            if (pSecondDest == nullptr)
            {
                GPU_HAL_ASSERTMESSAGE("Failed to lock second destination resource for reading");
                hr = E_UNEXPECTED;
                MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }

            didLockSecondDestResource = TRUE;

            GPU_HAL_CHK_STATUS_WITH_HR(MOS_SecureMemcpy(
                pOriginalDest + NumBytesBeforeRollover,
                RemainingSize,
                pSecondDest,
                RemainingSize));

            // Unlock original destination and second destination resources.
            m_pOsServices->m_pOsInterface->pfnUnlockResource(
                m_pOsServices->m_pOsInterface,
                sBuildCryptoCopyParams.DstResource);

            didLockOriginalDestResource = FALSE;

            m_pOsServices->m_pOsInterface->pfnUnlockResource(
                m_pOsServices->m_pOsInterface,
                pSecondDestResource);

            didLockSecondDestResource = FALSE;
        }

        m_pOsServices->SyncOnSubmit(pCmdBuf, true);

    } while (FALSE);

finish:
    if (pResource)
    {
        m_pOsServices->FreeResource(pResource);
    }

    if (pStoreResource)
    {
        m_pOsServices->FreeResource(pStoreResource);
    }

    if (didLockOriginalSrcResource)
    {
        m_pOsServices->m_pOsInterface->pfnUnlockResource(
            m_pOsServices->m_pOsInterface,
            sBuildCryptoCopyParams.SrcResource);
    }

    if (didLockOriginalDestResource)
    {
        m_pOsServices->m_pOsInterface->pfnUnlockResource(
            m_pOsServices->m_pOsInterface,
            sBuildCryptoCopyParams.DstResource);
    }

    if (didLockSecondSrcResource)
    {
        m_pOsServices->m_pOsInterface->pfnUnlockResource(
            m_pOsServices->m_pOsInterface,
            pSecondSrcResource);
    }

    if (didLockSecondDestResource)
    {
        m_pOsServices->m_pOsInterface->pfnUnlockResource(
            m_pOsServices->m_pOsInterface,
            pSecondDestResource);
    }

    if (pSecondSrcResource)
    {
        m_pOsServices->FreeResource(pSecondSrcResource);
    }

    if (pSecondDestResource)
    {
        m_pOsServices->FreeResource(pSecondDestResource);
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
        i = iRemaining - pCmdBuf->iRemaining;
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

HRESULT CPavpGpuHal13::SetGpuCryptoKeyExParams(PMHW_PAVP_ENCRYPTION_PARAMS pMhwPavpEncParams)
{
    HRESULT hr = S_OK;
    MOS_STATUS  eStatus = MOS_STATUS_UNKNOWN;
    bool bCryptoKeyExParamIsValid;

    // We don't call BuildKeyExchange() in certain scenario (IndirectDisplay) so no need to cache/insert KeyExchange into prolog.
    bCryptoKeyExParamIsValid = (m_CryptoKeyExParams.KeyExchangeKeySelect & (MHW_CRYPTO_ENCRYPTION_KEY | MHW_CRYPTO_DECRYPTION_KEY));

    GPU_HAL_FUNCTION_ENTER;

    do {
        if (pMhwPavpEncParams == nullptr)
        {
            GPU_HAL_ASSERTMESSAGE("decode encryption params are NULL");
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        if (m_bIsKbKeyRefresh && bCryptoKeyExParamIsValid)
        {
            GPU_HAL_VERBOSEMESSAGE("cryptokeyex params copied into decode encryption params");
            GPU_HAL_CHK_STATUS_WITH_HR(MOS_SecureMemcpy(
                &(pMhwPavpEncParams->CryptoKeyExParams),
                sizeof(MHW_PAVP_MFX_CRYPTO_KEY_EXCHANGE_CMD_PARAMS),
                &m_CryptoKeyExParams,
                sizeof(MHW_PAVP_MFX_CRYPTO_KEY_EXCHANGE_CMD_PARAMS)));
            pMhwPavpEncParams->bInsertKeyExinVcsProlog = TRUE;
        }
        else
        {
            GPU_HAL_VERBOSEMESSAGE("No cryptokeyex params copied into decode encryption params. "
                "m_bIsKbKeyRefresh=[%d] bCryptoKeyExParamIsValid=[%d]", m_bIsKbKeyRefresh, bCryptoKeyExParamIsValid);
            pMhwPavpEncParams->bInsertKeyExinVcsProlog = FALSE;
        }

    } while (FALSE);

finish:
    GPU_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpGpuHal13::BuildQueryStatus(
    PMOS_COMMAND_BUFFER                      pCmdBuf,
    PAVP_GPU_HAL_QUERY_STATUS_PARAMS&        sBuildQueryStatusParams)
{
    HRESULT                 hr = S_OK;
    PMOS_RESOURCE           pResource = nullptr;
    MOS_STATUS              eStatus = MOS_STATUS_UNKNOWN;
    PBYTE                   pResBytes = nullptr;
    volatile DWORD          dwValueInMemory = 0;
    INT                     i = 0, iRemaining = 0;
    DWORD                   AllocationSize = 0;
    DWORD                   OutputDataSize = 0;

    MhwCpBase               *mhwCp = nullptr;

    GPU_HAL_FUNCTION_ENTER;

    if (m_pOsServices                                  == nullptr ||
        m_pOsServices->m_pOsInterface                  == nullptr ||
        m_pOsServices->m_pOsInterface->osCpInterface   == nullptr ||
        m_pMhwInterfaces->m_miItf                      == nullptr ||
        m_pMhwInterfaces->m_cpInterface                == nullptr ||
        pCmdBuf                                        == nullptr)
    {
        hr = E_UNEXPECTED;
        return hr;
    }

    mhwCp = dynamic_cast<MhwCpBase*>(m_pMhwInterfaces->m_cpInterface);

    if(mhwCp == nullptr)
    {
        hr = E_UNEXPECTED;
        return hr;
    }

    do
    {
        // Insert SET_APP_ID command.
        PAVP_MI_SET_APP_ID_CMD_PARAMS sSetAppIdCmdParams;
        MOS_ZeroMemory(&sSetAppIdCmdParams, sizeof(sSetAppIdCmdParams));
        sSetAppIdCmdParams.dwAppId = sBuildQueryStatusParams.AppId;
        GPU_HAL_CHK_STATUS_WITH_HR(mhwCp->AddMiSetAppId(m_pOsServices->m_pOsInterface, pCmdBuf, sSetAppIdCmdParams.dwAppId));

        //Insert MFX_WAIT command.
        auto &mfxWaitParams               = m_pMhwInterfaces->m_miItf->MHW_GETPAR_F(MFX_WAIT)();
        mfxWaitParams                     = {};
        mfxWaitParams.iStallVdboxPipeline = true;
        GPU_HAL_CHK_STATUS_WITH_HR(m_pMhwInterfaces->m_miItf->MHW_ADDCMD_F(MFX_WAIT)(pCmdBuf, nullptr));

        if (sBuildQueryStatusParams.QueryOperation == PAVP_QUERY_STATUS_WIDI_COUNTER)
        {
            AllocationSize = sizeof(CP_ENCODE_HW_COUNTER);
            OutputDataSize = sizeof(CP_ENCODE_HW_COUNTER);
        }
        else
        {   // allocation and output sizes are different because allocation also include data for pipelined media store
            AllocationSize = PAVP_QUERY_STATUS_MAX_OUT_DATA_SIZE;
            OutputDataSize = sizeof(sBuildQueryStatusParams.OutData);
        }

        // Allocate a resource for the output.
        hr = m_pOsServices->AllocateLinearResource(
            MOS_GFXRES_BUFFER,                      // eType
            AllocationSize,                        // dwWidth
            1,                                      // dwHeight
            __FUNCTION__,                           // pResourceName
            Format_Buffer,                          // eFormat
            pResource);
        if (FAILED(hr) || pResource == nullptr)
        {
            GPU_HAL_ASSERTMESSAGE("Failed to allocate resource. Error = 0x%x.", hr);
            MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            goto finish;
        }

        if (sBuildQueryStatusParams.QueryOperation == PAVP_QUERY_STATUS_WIDI_COUNTER)
        {
            uint16_t            currentIndex = 0;
            GPU_HAL_CHK_STATUS_WITH_HR(mhwCp->ReadEncodeCounterFromHW(m_pOsServices->m_pOsInterface, pCmdBuf, pResource, currentIndex));
        }
        else
        {
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

            sCryptoInlineStatusReadCmdParams.presReadData = pResource;
            sCryptoInlineStatusReadCmdParams.dwSize = PAVP_QUERY_STATUS_MAX_OUT_DATA_SIZE;
            sCryptoInlineStatusReadCmdParams.dwReadDataOffset = 0;
            sCryptoInlineStatusReadCmdParams.presWriteData = pResource;
            sCryptoInlineStatusReadCmdParams.dwWriteDataOffset = PAVP_QUERY_STATUS_STORE_DWORD_OFFSET;

            switch (sBuildQueryStatusParams.QueryOperation)
            {
            case PAVP_QUERY_STATUS_MEMORY_MODE:
                sCryptoInlineStatusReadCmdParams.StatusReadType = CRYPTO_INLINE_MEMORY_STATUS_READ;
                break;
            case PAVP_QUERY_STATUS_CONNECTION_STATE:
                sCryptoInlineStatusReadCmdParams.StatusReadType = CRYPTO_INLINE_CONNECTION_STATE_READ;
                sCryptoInlineStatusReadCmdParams.Value.NonceIn = sBuildQueryStatusParams.Value.NonceIn;
                break;
            case PAVP_QUERY_STATUS_FRESHNESS:
                sCryptoInlineStatusReadCmdParams.StatusReadType = CRYPTO_INLINE_GET_FRESHNESS_READ;
                sCryptoInlineStatusReadCmdParams.Value.KeyRefreshType = sBuildQueryStatusParams.Value.KeyRefreshType;
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
        }

        if (!m_bParsingAssistanceFromKmd)
        {
            auto &batchBufferEndParams = m_pMhwInterfaces->m_miItf->MHW_GETPAR_F(MI_BATCH_BUFFER_END)();
            batchBufferEndParams       = {};
            GPU_HAL_CHK_STATUS_WITH_HR(m_pMhwInterfaces->m_miItf->MHW_ADDCMD_F(MI_BATCH_BUFFER_END)(pCmdBuf, nullptr));
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

        //reading data stored by pipelined media store DW
        if (sBuildQueryStatusParams.QueryOperation != PAVP_QUERY_STATUS_WIDI_COUNTER)
        {
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
        }

        // Copy the result to the caller provided struct.
        GPU_HAL_CHK_STATUS_WITH_HR(MOS_SecureMemcpy(
            static_cast<void *>(&sBuildQueryStatusParams.OutData),
            OutputDataSize,
            pResBytes,
            OutputDataSize));

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
        i = iRemaining - pCmdBuf->iRemaining;
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

HRESULT CPavpGpuHal13::SetKeyRefreshType()
{
    // bit 23 of KCR App Char reg inidicates Key Refresh mode. 0 = Diasy, 1 = Kb based
    PAVP_GPU_REGISTER_VALUE ValueRead = 0;
    HRESULT                 hr        = S_OK;
    MOS_STATUS              eStatus   = MOS_STATUS_UNKNOWN;

    // Need to update this call to pass in the session ID if we decide we need this check
    GPU_HAL_CHK_STATUS_WITH_HR(m_pOsServices->ReadMMIORegister(PAVP_REGISTER_KCR_APP_CHAR, ValueRead));

    GPU_HAL_NORMALMESSAGE("PAVP_REGISTER_KCR_APP_CHAR = 0x%x",ValueRead);

    if (ValueRead & PAVP_GEN13_KCR_APP_CHAR_KEY_REFRESH_MODE_BIT_MASK)
    {
        GPU_HAL_NORMALMESSAGE("Bit 23 of KCR APP char reg is set");
        m_bIsKbKeyRefresh = TRUE;
    }

    GPU_HAL_NORMALMESSAGE("m_bIsKbKeyRefresh = 0x%x", m_bIsKbKeyRefresh);

finish:
    GPU_HAL_FUNCTION_EXIT(hr);
    return hr;
}

BOOL CPavpGpuHal13::CheckCryptoCopyCounterRollover(
    uint32_t DataLength,
    uint32_t CounterLow,
    uint32_t CounterHigh,
    std::array<bool, 4>& OverflowBits)
{
    // The algorithm used to detect counter rollover is specified in section 6.2 of the ICL PAVP BSpec
    BOOL WillRollover = FALSE;

    uint64_t InitialCounter = ((static_cast<uint64_t>(CounterHigh) << PAVP_SHIFT_UINT32_TO_UINT64) | CounterLow);
    uint64_t FinalCounter = InitialCounter + (DataLength / MHW_PAVP_AES128_NUM_BYTES_IN_BLOCK);

    std::bitset<PAVP_NUMBER_OF_BITS_IN_AES128_CTR> InitialCounterBits(InitialCounter);
    std::bitset<PAVP_NUMBER_OF_BITS_IN_AES128_CTR> FinalCounterBits(FinalCounter);

    for (int i = PAVP_GEN13_FIRST_RESERVED_COUNTER_BIT; i <= PAVP_GEN13_LAST_RESERVED_COUNTER_BIT; i++)
    {
        WillRollover |= (InitialCounterBits[i] == 0 && FinalCounterBits[i] == 1);
        uint32_t index = i - PAVP_GEN13_FIRST_RESERVED_COUNTER_BIT;
        if (WillRollover == TRUE && FinalCounterBits[i] == 1)
        {
            OverflowBits[index] = true;
        }
        else
        {
            OverflowBits[index] = false;
        }
    }

    const BOOL OverflowInBit63 = (InitialCounterBits[PAVP_GEN13_LAST_RESERVED_COUNTER_BIT] == 1 &&
        FinalCounterBits[PAVP_GEN13_LAST_RESERVED_COUNTER_BIT] == 0);
    WillRollover |= OverflowInBit63;
    if (OverflowInBit63 == TRUE)
    {
        OverflowBits[PAVP_GEN13_LAST_RESERVED_COUNTER_BIT - PAVP_GEN13_FIRST_RESERVED_COUNTER_BIT] = true;
    }

    return WillRollover;
}


HRESULT CPavpGpuHal13::ReadSessionInPlayRegister(
    uint32_t                       sessionId,
    PAVP_GPU_REGISTER_VALUE      &value)
{
    HRESULT hr = S_OK;
    PAVP_GPU_REGISTER_TYPE  ullRegType = PAVP_REGISTER_SESSION_IN_PLAY;

    GPU_HAL_FUNCTION_ENTER;

    do
    {
        // Read the 'PAVP Per App Session in Play' register.
        if (sessionId & TRANSCODE_APP_ID_MASK)
        {
            // On DG2+, trancode session in play is queried from two dedicated registers,
            // register PAVP_REGISTER_TRANSCODE_SESSION_IN_PLAY_1 for session 0-31,
            // register PAVP_REGISTER_TRANSCODE_SESSION_IN_PLAY_2 for session 32-63.
            ullRegType = (sessionId & ~TRANSCODE_APP_ID_MASK) < PAVP_REGISTER_SIZE ?
                           PAVP_REGISTER_TRANSCODE_SESSION_IN_PLAY_1 :
                           PAVP_REGISTER_TRANSCODE_SESSION_IN_PLAY_2;
        }

        hr = m_pOsServices->ReadMMIORegister(
                ullRegType,
                value,
                CPavpGpuHal::ConvertRegType2RegOffset(ullRegType),
                0,                          //default Index
                PAVP_HWACCESS_GPU_NOT_IDLE);
        if (FAILED(hr))
        {
            GPU_HAL_ASSERTMESSAGE("Failed to read the 'PAVP Per App Session in Play' register.");
            MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        GPU_HAL_VERBOSEMESSAGE("'PAVP Per App Session in Play' register value: 0x%x.", static_cast<uint32_t>(value));
    } while (FALSE);

    GPU_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpGpuHal13::ReadSessionModeRegister(
    uint32_t                          sessionId,
    PAVP_SESSION_MODE               sessionMode,
    PAVP_GPU_REGISTER_VALUE         &value)
{
    HRESULT hr = S_OK;
    PAVP_GPU_REGISTER_TYPE  ullRegType = PAVP_REGISTER_INVALID;

    GPU_HAL_FUNCTION_ENTER;

    do
    {
        if ((sessionMode == PAVP_SESSION_MODE_STOUT) &&
            (sessionId & TRANSCODE_APP_ID_MASK))
        {
            ullRegType = (sessionId & ~TRANSCODE_APP_ID_MASK) < PAVP_REGISTER_SIZE ?
                       PAVP_REGISTER_TRANSCODE_STOUT_STATUS_1 :
                       PAVP_REGISTER_TRANSCODE_STOUT_STATUS_2;

            hr = m_pOsServices->ReadMMIORegister(ullRegType, value, CPavpGpuHal::ConvertRegType2RegOffset(ullRegType));
            if (FAILED(hr))
            {
                GPU_HAL_ASSERTMESSAGE("Failed to read the 'PAVP sesion mode' register.");
                MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }

            GPU_HAL_VERBOSEMESSAGE("'PAVP Per App Session mode' register value: 0x%x.", static_cast<uint32_t>(value));
        }
    } while (FALSE);

    GPU_HAL_FUNCTION_EXIT(hr);
    return hr;
}

BOOL CPavpGpuHal13::VerifySessionInPlay(
    uint32_t                      sessionId,
    PAVP_GPU_REGISTER_VALUE     value)
{
    PAVP_GPU_REGISTER_VALUE sessionInPlayMask = 0;
    uint32_t                  shiftOffset = 0;

    GPU_HAL_FUNCTION_ENTER;

    shiftOffset = sessionId & 0xF;
    sessionInPlayMask = (PAVP_GPU_REGISTER_VALUE)1 << shiftOffset;
    sessionInPlayMask <<= ((sessionId & TRANSCODE_APP_ID_MASK) ? PAVP_REGISTER_SIZE/2 : 0);

    if (sessionId & TRANSCODE_APP_ID_MASK)
    {
        shiftOffset       = (sessionId & ~TRANSCODE_APP_ID_MASK) % PAVP_REGISTER_SIZE;
        sessionInPlayMask = (PAVP_GPU_REGISTER_VALUE)1 << shiftOffset;
    }

    GPU_HAL_FUNCTION_EXIT(S_OK);
    return ((sessionInPlayMask & value) != 0);
}

BOOL CPavpGpuHal13::VerifySessionMode(
    uint32_t                      sessionId,
    PAVP_SESSION_MODE           sessionMode,
    PAVP_GPU_REGISTER_VALUE     value)
{
    PAVP_GPU_REGISTER_VALUE sessionModeMask = 0;

    GPU_HAL_FUNCTION_ENTER;

    if ((sessionMode == PAVP_SESSION_MODE_STOUT) &&
        (sessionId & TRANSCODE_APP_ID_MASK))
    {
        sessionModeMask = (PAVP_GPU_REGISTER_VALUE)1 << ((sessionId & ~TRANSCODE_APP_ID_MASK) % PAVP_REGISTER_SIZE);

        if ((sessionModeMask & value) == 0)
            return false;
    }

    GPU_HAL_FUNCTION_EXIT(S_OK);
    return true;
}

CPavpGpuHal13::~CPavpGpuHal13()
{
    if (m_pMhwInterfaces)
    {
        if (m_pMhwInterfaces->m_cpInterface != nullptr)
        {
            MOS_Delete(m_pMhwInterfaces->m_cpInterface);
            m_pMhwInterfaces->m_cpInterface = nullptr;
        }

        MOS_Delete(m_pMhwInterfaces);
        m_pMhwInterfaces = nullptr;
    }
}
