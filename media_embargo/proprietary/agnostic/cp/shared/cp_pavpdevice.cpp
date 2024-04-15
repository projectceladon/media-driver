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

File Name: cp_pavpdevice.cpp

Abstract:
    Implementation of the PAVP Device base class. Handles all low level PAVP
    device details. This class will be inherited and built upon to map
    DDI-specific entry points to PAVP features/calls.

Environment:
    Windows 7, Windows 8, Android

Notes:

======================= end_copyright_notice ==================================*/
#include <thread>
#include <unordered_map>
#include <algorithm>

// use std::min and std::max instead of min/max macros defined in minwindef.h
#define NOMINMAX

#include "cp_pavpdevice.h"
#include "media_interfaces_mhw.h"
#include "cp_os_services.h"
#include "heci_definitions.h"
#include "intel_pavp_pr_api.h"
#include "mos_gfxinfo_id.h"
#include "cp_drm_defs.h"
#include "codechal_cenc_decode.h"
#include "codechal_cenc_decode_next.h"
#include "cp_user_settings_mgr_ext.h"  // For storing Crypto Key Ex params in Encryption Param struct in SetCryptoKeyExParams()
#include "cp_hal_gpu.h"
#ifdef _AV1_SECURE_DECODE_SUPPORTED
#include "codec_def_common_av1.h"
#endif
#if defined(WIN32)
#include "codechal_setting_ext.h"
#include "pavp_encryption.h"
#include "mos_mediacopy.h"
#include "dxvaumd.h"
#endif


#define KEY_REFRESH_TYPE_DECRYPTED 0
#define KEY_REFRESH_TYPE_ENCRYPTED 1

#define PED_PACKET_SIZE 16

// Number of times we query session in play after key terminate command.
#define MAX_SESSION_IN_PLAY_QUERIES 100

// These query KMD for status change
#define PAVP_MAX_POWERUP_IN_PROGRESS_QUERIES    0x10
#define PAVP_MAX_ATTACKED_IN_PROGRESS_QUERIES   0x10
#define PAVP_MAX_ARBITRATOR_INIT_IN_PROGRESS_QUERIES    0x10
// Keep this max query larger than the above, so we can capture
// the cases above. 
#define PAVP_MAX_RESERVE_SESSION_ID_QUERIES     0x20
// It seems that each GPU Gen has its own display mask but there is no
// consolidated mask to represent all Gens.
#define PAVP_DECODE_SESSION_IN_PLAY_MASK        0x0000FFFF // Includes WiDi.




// CPavpDevice Helper Methods.

#if MOS_MESSAGES_ENABLED

// Debug, converts session status to a string for debug logs.
const char* SessionStatus2Str(PAVP_SESSION_STATUS Status)
{
    switch(Status)
    {
        case PAVP_SESSION_GET_ANY_DECODE_SESSION_IN_USE:
            return "PAVP_SESSION_GET_ANY_DECODE_SESSION_IN_USE";
        case PAVP_SESSION_GET_SESSION_STATUS:
            return "PAVP_SESSION_GET_SESSION_STATUS";
        case PAVP_SESSION_UNSUPPORTED:
            return "PAVP_SESSION_UNSUPPORTED";
        case PAVP_SESSION_POWERUP:
            return "PAVP_SESSION_POWERUP";
        case PAVP_SESSION_POWERUP_IN_PROGRESS:
            return "PAVP_SESSION_POWERUP_IN_PROGRESS";
        case PAVP_SESSION_UNINITIALIZED:
            return "PAVP_SESSION_UNINITIALIZED";
        case PAVP_SESSION_IN_INIT:
            return "PAVP_SESSION_IN_INIT";
        case PAVP_SESSION_INITIALIZED:
            return "PAVP_SESSION_INITIALIZED";
        case PAVP_SESSION_READY:
            return "PAVP_SESSION_READY";
        case PAVP_SESSION_TERMINATED:
            return "PAVP_SESSION_TERMINATED";
        case PAVP_SESSION_TERMINATE_WIDI:
            return "PAVP_SESSION_TERMINATE_WIDI";
        case PAVP_SESSION_TERMINATE_IN_PROGRESS:
            return "PAVP_SESSION_TERMINATE_IN_PROGRESS";
        case PAVP_SESSION_IN_USE:
            return "PAVP_SESSION_IN_USE";
        default:
            return "Unknown session status value";
    }
    return "Unknown Session Status value";
}

const char* PavpSessionType2Str(PAVP_SESSION_TYPE Type)
{
    switch(Type)
    {
        case PAVP_SESSION_TYPE_DECODE:
            return "PAVP_SESSION_TYPE_DECODE";
        case PAVP_SESSION_TYPE_TRANSCODE:
            return "PAVP_SESSION_TYPE_TRANSCODE";
        case PAVP_SESSION_TYPE_WIDI:
            return "PAVP_SESSION_TYPE_WIDI";
        default:
            return "Unknown session type value";
    }
    return "Unknown Session Status value";
}

const char* PavpSessionMode2Str(PAVP_SESSION_MODE Mode)
{
    switch (Mode)
    {
        case PAVP_SESSION_MODE_LITE:
            return "PAVP_SESSION_MODE_LITE";
        case PAVP_SESSION_MODE_HEAVY:
            return "PAVP_SESSION_MODE_HEAVY";
        case PAVP_SESSION_MODE_STOUT:
            return "PAVP_SESSION_MODE_STOUT";
        case PAVP_SESSION_MODE_ISO_DEC:
            return "PAVP_SESSION_MODE_ISO_DEC";
        case PAVP_SESSION_MODE_THV_D:
            return "PAVP_SESSION_MODE_THV_D";
        case PAVP_SESSION_MODE_HUC_GUARD:
            return "PAVP_SESSION_MODE_HUC_GUARD";
        default:
            return "Unknown session mode value";
    }
    return "Unknown Session Mode value";
}
#endif // MOS_MESSAGES_ENABLED

// CPavpDevice Public Methods.

BOOL CPavpDevice::IsPavpEnabled(MEDIA_FEATURE_TABLE *pSkuTable)
{
    BOOL bReturn = FALSE;

    PAVP_DEVICE_FUNCTION_ENTER;

    if (pSkuTable)
    {
        bReturn = static_cast<BOOL>(MEDIA_IS_SKU(pSkuTable, FtrPAVP));
    }

    PAVP_DEVICE_FUNCTION_EXIT(bReturn);
    return bReturn;
}

BOOL CPavpDevice::IsDecodeProfileSupported(
    PAVP_ENCRYPTION_TYPE    eEncryptionType,
    uint32_t                codechalMode,
    MEDIA_FEATURE_TABLE     *pSkuTable,
    BOOL                    bIgnoreDecodeMode)
{
    BOOL bReturn = FALSE;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        // TODO: Need ULT to make sure Codec HAL doesn't modify these and break this logic.

        PAVP_DEVICE_VERBOSEMESSAGE("Encryption type = %d, decode mode = %d.", eEncryptionType, codechalMode);

        if ((bIgnoreDecodeMode == FALSE) &&
            (codechalMode >= CODECHAL_DECODE_MODE_END))
        {
            bReturn = FALSE;
            break;
        }

        switch(eEncryptionType)
        {
            case PAVP_ENCRYPTION_AES128_CTR:
                if (bIgnoreDecodeMode)
                {
                    bReturn = TRUE;
                    break;
                }

                // CTR is supported on HEVC, AVC, VC1, MPEG2, VP9, AV1 and MVC.
                switch(codechalMode)
                {
                    case CODECHAL_DECODE_MODE_AVCVLD:
                    case CODECHAL_DECODE_MODE_MVCVLD:
                    case CODECHAL_DECODE_MODE_VC1VLD:
                    case CODECHAL_DECODE_MODE_MPEG2VLD:
                    case CODECHAL_DECODE_MODE_HEVCVLD:
                    case CODECHAL_DECODE_MODE_VP9VLD:
#ifdef _AV1_SECURE_DECODE_SUPPORTED
                    case CODECHAL_DECODE_MODE_AV1VLD:
#endif
                    case CODECHAL_DECODE_MODE_RESERVED0: //VVC
                        bReturn = TRUE;
                        break;
                    default:
                        bReturn = FALSE;
                        PAVP_DEVICE_ASSERTMESSAGE("Unsupported decode mode");
                        break;
                }
                break;
            case PAVP_ENCRYPTION_AES128_CBC:
                if (bIgnoreDecodeMode)
                {
                    bReturn = TRUE;
                    break;
                }

                // CBC is only supported on AVC and HEVC
                switch(codechalMode)
                {
                    case CODECHAL_DECODE_MODE_AVCVLD:
                    case CODECHAL_DECODE_MODE_HEVCVLD:
#ifdef _AV1_DECODE_SUPPORTED
                    case CODECHAL_DECODE_MODE_AV1VLD:
#endif
                    case CODECHAL_DECODE_MODE_RESERVED0:  //VVC
                        bReturn = TRUE;
                        break;
                    default:
                        bReturn = FALSE;
                        PAVP_DEVICE_ASSERTMESSAGE("Unsupported decode mode");
                        break;
                }
                break;
            case PAVP_ENCRYPTION_DECE_AES128_CTR:
                if (bIgnoreDecodeMode)
                {
                    bReturn = TRUE;
                    break;
                }

                // DECE is only supported on AVC and HEVC.
                switch(codechalMode)
                {
                    case CODECHAL_DECODE_MODE_AVCVLD:
                    case CODECHAL_DECODE_MODE_HEVCVLD:
                        bReturn = TRUE;
                        break;
                    default:
                        bReturn = FALSE;
                        PAVP_DEVICE_ASSERTMESSAGE("Unsupported decode mode");
                        break;
                }
                break;
            default:
                bReturn = FALSE;
                PAVP_DEVICE_ASSERTMESSAGE("Invalid crypto mode");
                break;
        }
    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(bReturn);
    return bReturn;
}

BOOL CPavpDevice::IsEncodeProfileSupported(
    PAVP_ENCRYPTION_TYPE    eEncryptionType,
    uint32_t                codechalMode,
    MEDIA_FEATURE_TABLE     *pSkuTable,
    BOOL                    bIgnoreEncodeMode)
{
    BOOL bReturn = FALSE;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        if ((bIgnoreEncodeMode == FALSE) && 
            (codechalMode < CODECHAL_ENCODE_MODE_BEGIN || codechalMode >= CODECHAL_ENCODE_MODE_END))
        {
            bReturn = FALSE;
            break;
        }

        switch(eEncryptionType)
        {
            case PAVP_ENCRYPTION_AES128_CTR:
                // Supported for AVC and MPEG2 only.
                bReturn = TRUE;
                break;
            default:
                bReturn = FALSE;
        }

        if (bReturn == FALSE)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Invalid crypto mode");
            break;
        }

        if (bIgnoreEncodeMode)
        {
            break;
        }

        switch(codechalMode)
        {
            case CODECHAL_ENCODE_MODE_AVC:
            case CODECHAL_ENCODE_MODE_MPEG2:
            case CODECHAL_ENCODE_MODE_HEVC:
                bReturn = TRUE;
                break;
            default:
                bReturn = FALSE;
                break;
        }

        if (bReturn == FALSE)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Invalid encode mode");
            break;
        }

    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(bReturn);
    return bReturn;
}

BOOL CPavpDevice::IsPavpSessionModeSupported(PAVP_SESSION_MODE pavpSessionMode, PAVP_SESSION_TYPE pavpSessionType, CODECHAL_MODE codechalMode, MEDIA_FEATURE_TABLE *pSkuTable)
{
    BOOL bReturn = FALSE;

    PAVP_DEVICE_FUNCTION_ENTER;

    if (pSkuTable)
    {
        bReturn = TRUE; // all session mode is supported on all codec mode now
    }
    else {
        PAVP_DEVICE_ASSERTMESSAGE("Invalid Sku Table."); 
    }

    PAVP_DEVICE_FUNCTION_EXIT(bReturn);
    return bReturn; 
}

PAVP_SESSION_TYPE CPavpDevice::GetSessionType()
{
    PAVP_DEVICE_FUNCTION_ENTER;
    PAVP_DEVICE_VERBOSEMESSAGE("Session type requested. Returning: %d.", m_PavpSessionType);
    PAVP_DEVICE_FUNCTION_EXIT(0);
    return m_PavpSessionType;
}

PAVP_ENCRYPTION_TYPE CPavpDevice::GetEncryptionType()
{
    PAVP_DEVICE_FUNCTION_ENTER;
    PAVP_DEVICE_VERBOSEMESSAGE("Encryption type requested. Returning: %d.", m_eCryptoType);
    PAVP_DEVICE_FUNCTION_EXIT(0);
    return m_eCryptoType;
}

HRESULT CPavpDevice::OpenArbitratorSession()
{
    HRESULT             hr                        = S_OK;
    PAVP_SESSION_STATUS PreviousPavpSessionStatus = PAVP_SESSION_UNSUPPORTED;  // Should never be this.
    PAVP_SESSION_STATUS CurrentPavpSessionStatus  = PAVP_SESSION_UNSUPPORTED;  // Should never be this.

    if (m_pOsServices == nullptr)
    {
        PAVP_DEVICE_ASSERTMESSAGE("GPU HAL or OS Services is NULL.");
        return E_UNEXPECTED;
    }

    m_cpContext.value = CP_SURFACE_TAG;
    // Get current arbitrator session status by request IN_INIT, set cptag to default value to avoid hwaccess
    hr = m_pOsServices->GetSetPavpSessionStatus(
            PAVP_SESSION_TYPE_WIDI,
            PAVP_SESSION_MODE_HEAVY,
            PAVP_SESSION_IN_INIT,        // What we want.
            &CurrentPavpSessionStatus,   // What we got.
            &PreviousPavpSessionStatus,  // What the status was before.
            &m_cpContext.value);         // Our pavp session context.

    //For Stout session, after init hw decode session, session status is INITIALIZED. after beginFrame SetStreamKey is READY.
    //For arbitrator session, the status after init hw decode is set to READY.
    if (FAILED(hr) || !(m_cpContext.stoutMode || CurrentPavpSessionStatus == PAVP_SESSION_READY) ||
       (m_cpContext.stoutMode && !(CurrentPavpSessionStatus == PAVP_SESSION_READY || CurrentPavpSessionStatus == PAVP_SESSION_INITIALIZED)))
    {
        if (SUCCEEDED(hr) && !(m_cpContext.stoutMode) && CurrentPavpSessionStatus == PAVP_SESSION_IN_USE && PreviousPavpSessionStatus == PAVP_SESSION_IN_USE)
        {
            PAVP_DEVICE_NORMALMESSAGE("current session state %s, previous session state %s. Need to init Arbitrator session first.", SessionStatus2Str(CurrentPavpSessionStatus), SessionStatus2Str(PreviousPavpSessionStatus));
        }
        else
        {
            PAVP_DEVICE_ASSERTMESSAGE("Error: GetSetPavpSessionStatus return failure,  current session state %s.", SessionStatus2Str(CurrentPavpSessionStatus));
        }
        hr = E_UNEXPECTED;
    }
    return hr;
}

HRESULT CPavpDevice::QuerySessionInPlay(BOOL &bSessionInPlay, uint32_t sessionId)
{
    HRESULT                 hr              = S_OK;
    PAVP_GPU_REGISTER_VALUE ullRegValue     = 0;
    BOOL sessionModeMatched                 = FALSE;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        // Verify that GPU HAL has been created.
        // It is needed in order to perform the operation.
        if (m_pGpuHal     == nullptr ||
            m_pOsServices == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("GPU HAL or OS Services is NULL.");
            hr = E_UNEXPECTED;
            break;
        }

        hr = m_pGpuHal->ReadSessionInPlayRegister(sessionId, ullRegValue);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to read the 'PAVP Per App Session in Play' register.");
            break;
        }

        // Find out if the session is alive based on the register value we have just read.
        bSessionInPlay = m_pGpuHal->VerifySessionInPlay(sessionId, ullRegValue);
        PAVP_DEVICE_NORMALMESSAGE("Session in play = %d.", bSessionInPlay);

        hr = m_pGpuHal->ReadSessionModeRegister(sessionId, m_PavpSessionMode, ullRegValue);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to read the 'PAVP Per App Session Mode' register.");
            break;
        }

        sessionModeMatched = m_pGpuHal->VerifySessionMode(sessionId, m_PavpSessionMode, ullRegValue);
        if (!sessionModeMatched)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Session mode is not matched.");
        }

    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::SetWiDiSessionParameters(
        UINT64            rIV,
        uint32_t            StreamCtr,
        BOOL              bWiDiEnabled,
        PAVP_SESSION_MODE ePavpSessionMode,
        ULONG             HDCPType,
        BOOL              bDontMoveSession)
{
    HRESULT             hr                          = S_OK;
    PAVP_SESSION_STATUS CurrentPavpSessionStatus    = PAVP_SESSION_UNSUPPORTED;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        // Verify that the device has been properly initialized.
        if (!IsInitialized() ||
            m_pOsServices == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("The device is not properly initialized.");
            hr = E_UNEXPECTED;
            break;
        }

        hr = m_pOsServices->QuerySessionStatus(&CurrentPavpSessionStatus);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to query whether the session is alive. Error = 0x%x.", hr);
        }

        // The session should be in 'initialized' status when setting WiDi parameters.
        if (!bDontMoveSession && CurrentPavpSessionStatus != PAVP_SESSION_INITIALIZED)
        {
            PAVP_DEVICE_ASSERTMESSAGE("The device must be in INITIALIZED status. Current status: %s.", SessionStatus2Str(CurrentPavpSessionStatus));
            hr = E_INVALIDARG;
            break;
        }

        hr = m_pOsServices->SetWiDiSessionParameters(
                    rIV,
                    StreamCtr,
                    bWiDiEnabled,
                    ePavpSessionMode,
                    HDCPType,
                    bDontMoveSession);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to set WiDi parameters. Error = 0x%x.", hr);
        }

    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::IsSessionInitialized(BOOL& bIsSessionInitialized)
{
    HRESULT             hr                          = S_OK;
    PAVP_SESSION_STATUS CurrentPavpSessionStatus    = PAVP_SESSION_UNSUPPORTED;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        // Validate that the device is initialized.
        if (!IsInitialized() ||
            m_pOsServices == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("The device is not properly initialized.");
            hr = E_UNEXPECTED;
            break;
        }

        if (!m_bHwSessionIsAlive)
        {
            bIsSessionInitialized = FALSE;
            break;
        }

        hr = m_pOsServices->QuerySessionStatus(&CurrentPavpSessionStatus);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to query whether the session is alive. Error = 0x%x.", hr);
        }

        bIsSessionInitialized = (CurrentPavpSessionStatus == PAVP_SESSION_READY || CurrentPavpSessionStatus == PAVP_SESSION_INITIALIZED);

        if (!bIsSessionInitialized)
        {
            PAVP_DEVICE_NORMALMESSAGE("The session is not alive! Session status: %s", SessionStatus2Str(CurrentPavpSessionStatus));
        }

    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::IsSessionReady(BOOL& bIsSessionReady)
{
    HRESULT             hr                          = S_OK;
    PAVP_SESSION_STATUS CurrentPavpSessionStatus    = PAVP_SESSION_UNSUPPORTED;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        if(!IsInitialized())
        {
            PAVP_DEVICE_NORMALMESSAGE("The device is not properly initialized.");
            hr = E_UNEXPECTED;
            break;
        }

        // Validate that the device is initialized.

        if (m_pOsServices == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("The device is not properly initialized.");
            hr = E_UNEXPECTED;
            break;
        }

        hr = m_pOsServices->QuerySessionStatus(&CurrentPavpSessionStatus);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to query whether the session is alive. Error = 0x%x.", hr);
        }

        bIsSessionReady = (CurrentPavpSessionStatus == PAVP_SESSION_READY);

        if (!bIsSessionReady)
        {
            PAVP_DEVICE_NORMALMESSAGE("The session is not alive! Session status: %s", SessionStatus2Str(CurrentPavpSessionStatus));
        }

    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

// CPavpDevice Protected Methods.

CPavpDevice::CPavpDevice()
{
    PAVP_DEVICE_FUNCTION_ENTER;

    ResetMemberVariables();

    PAVP_DEVICE_FUNCTION_EXIT(0);
}

CPavpDevice::~CPavpDevice()
{
    PAVP_DEVICE_FUNCTION_ENTER;

    // for telemetry events
    for (size_t i=0; i< m_bitGfxInfoIds.size(); i++)
    {
        if (m_bitGfxInfoIds.test(i) != 0)
        {
            switch (i)
            {
            case GFXINFO_ID_CP_SESSION_TYPE_MODE:
                MosUtilities::MosGfxInfo(
                    1,
                    GFXINFO_COMPID_UMD_MEDIA_CP,
                    GFXINFO_ID_CP_SESSION_TYPE_MODE,
                    2,
                    "data1", GFXINFO_PTYPE_UINT32, &m_PavpSessionType,
                    "data2", GFXINFO_PTYPE_UINT32, &m_PavpSessionMode
                );
                break;
            default:
                MosUtilities::MosGfxInfo(
                    1,
                    GFXINFO_COMPID_UMD_MEDIA_CP,
                    i,
                    0
                );
            }
        }
    }

    Cleanup();

    PAVP_DEVICE_FUNCTION_EXIT(0);
}

BOOL CPavpDevice::IsInitialized() CONST
{
    // Please do not add function enter/exit macros. This function is called too many times.
    // Those pointers below should be populated in CPavpDevice::Init().
    return (m_pOsServices && m_pRootTrustHal && m_pGpuHal);
}


HRESULT CPavpDevice::Init(PAVP_DEVICE_CONTEXT pDeviceContext)
{
    HRESULT hr                       = S_OK;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        if (pDeviceContext == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Invalid device context.");
            hr = E_UNEXPECTED;
            break;
        }

        if (IsInitialized())
        {
            PAVP_DEVICE_ASSERTMESSAGE("This function can only be called once.");
            hr = E_UNEXPECTED;
            break;
        }

        PAVP_DEVICE_NORMALMESSAGE("Setting up a '%s' type session in '%s' mode.", PavpSessionType2Str(m_PavpSessionType), PavpSessionMode2Str(m_PavpSessionMode));

        hr = CreateHalsAndServices(pDeviceContext);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Could not set up HAL/Service layers.");
            break;
        }

        // These ptrs should not be nullptr if the CreateHalsAndServices() call succeeded,
        // but do a check here because the pointers will be dereferenced below.
        if (m_pRootTrustHal == nullptr ||
            m_pOsServices == nullptr ||
            m_pGpuHal == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Unexpected error: HALs have not been initialized.");
            hr = E_UNEXPECTED;
            break;
        }

        // should remove CheckAsmfModeSupport() if driver doesn't support gen9 and gen10 in the future.
        hr = m_pGpuHal->CheckAsmfModeSupport();
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Unexpected error: platform didn't support ASMF mode.");
            break;
        }

        // Notice: It's important that the PAVP Dll is loaded before getting a session ID.
        // This will initiate the GPU <-> RootTrust handshake if not done, and so 'G' is used intead of KF1.
        hr = m_pRootTrustHal->Init();
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to initialize RootTrust Hal. Error = 0x%x", hr);
            break;
        }

        // Check if provisioning needed
        if (m_pOsServices->IsEPIDProvisioningEnabled() &&
            !m_pOsServices->IsSimulationEnabled()
#if (_DEBUG || _RELEASE_INTERNAL)
            && m_PavpCryptoSessionMode != PAVP_CRYPTO_SESSION_TYPE_PAVP_MECOMM_FW_APP
            && m_PavpCryptoSessionMode != PAVP_CRYPTO_SESSION_TYPE_WIDI_MECOMM_FW_APP
            && m_PavpCryptoSessionMode != PAVP_CRYPTO_SESSION_TYPE_MKHI_MECOMM_FW_APP
            && m_PavpCryptoSessionMode != PAVP_CRYPTO_SESSION_TYPE_GSC_PROXY_MECOMM_FW_APP
#endif
        )
        {
            std::string stEpidResourcePath;
            hr = m_pOsServices->GetEpidResourcePath(stEpidResourcePath);
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Getting Epid File path failed, hr = 0x%x", hr);
                break;
            }
            hr = m_pRootTrustHal->PerformProvisioning(stEpidResourcePath);                        
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Epid Provisioning Failed, hr = 0x%x", hr);                            
                break;
            }
        }

        //Associated device context with this PAVP device
        m_pDeviceContext     = (PVOID)pDeviceContext;
        PAVP_DEVICE_NORMALMESSAGE("Device context %x is associated.", m_pDeviceContext);

        m_pOsServices->WriteRegistry(__MEDIA_USER_FEATURE_VALUE_DYNAMIC_MEMORY_PROTECTION_ID, IsMemoryEncryptionEnabled());

    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::ReserveSessionSlot(uint32_t *pAppID)
{
    HRESULT             hr                       = S_OK;
    PAVP_SESSION_STATUS CurrentPavpSessionStatus = PAVP_SESSION_UNSUPPORTED;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        if (pAppID == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Input pAppID is nullptr.");
            hr = E_UNEXPECTED;
            break;
        }

        hr = ReserveSessionId(m_PavpSessionType, m_PavpSessionMode, &CurrentPavpSessionStatus, nullptr);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Could not obtain a hardware PAVP session ID.");
            break;
        }

        *pAppID = m_cpContext.sessionId;

    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

// we should be able to remove FWFallback logic if master driver supports only CFL+ and not SKL/KBL
// or if FW version must be 4.1+
bool CPavpDevice::IsFWFallbackRequired()
{
    if (!m_pGpuHal->DoesPlatformFWRequireFallback() ||
        m_eDrmType == PAVP_DRM_TYPE_PLAYREADY ||
        m_eDrmType == PAVP_DRM_TYPE_WIDEVINE ||
        m_eDrmType == PAVP_DRM_TYPE_MIRACAST_RX ||
        (m_PavpSessionType == PAVP_SESSION_TYPE_TRANSCODE && m_PavpSessionMode == PAVP_SESSION_MODE_HEAVY) ||
        m_PavpSessionMode == PAVP_SESSION_MODE_ISO_DEC ||
        m_PavpSessionMode == PAVP_SESSION_MODE_STOUT ||
        m_pRootTrustHal->HasFwCaps(FIRMWARE_CAPS_PAVP_UNIFIED_INIT))
    {
        // return false, we use latest FW version
        return false;
    }
    else
    {
         return true;
    }
}

void CPavpDevice::WriteRegistry(
    CP_USER_FEATURE_VALUE_ID  UserFeatureValueId,
    DWORD                     dwData)
{
    if (m_pOsServices)
    {
        m_pOsServices->WriteRegistry(UserFeatureValueId, dwData);
    }
}

HRESULT CPavpDevice::InitHwSession()
{
    HRESULT             hr                          = S_OK;
    INT32               iRetry                      = 0;
    PAVP_SESSION_STATUS CurrentPavpSessionStatus    = PAVP_SESSION_UNSUPPORTED;

    PAVP_DEVICE_FUNCTION_ENTER;
    MOS_TraceEventExt(EVENT_CP_CREATE, EVENT_TYPE_START, &m_PavpSessionType, sizeof(uint32_t), &m_PavpSessionMode, sizeof(uint32_t));

    do
    {
        if (!m_pOsServices  ||
            !m_pGpuHal      ||
            !m_pRootTrustHal)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Cannot call hw session setup without PAVP device setup!");
            hr = E_UNEXPECTED;
            break;
        }

        if (!IsInitialized())
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to InitHwSession() due to uninited pavp device.");
            hr = E_UNEXPECTED;
            break;
        }

        if (m_bHwSessionIsAlive)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to InitHwSession() due to existing hw session.");
            hr = E_UNEXPECTED;
            break;
        }

        PAVP_DEVICE_NORMALMESSAGE("Init hw session with m_eDrmType=%d.", m_eDrmType);

        //use of m_bFwFallbackEnabled to develop a new driver path should be avoided
        if (IsFWFallbackRequired())
        {
            PAVP_DEVICE_VERBOSEMESSAGE("Exercising the FWFallback path");
            m_bFwFallbackEnabled = true;
            m_pRootTrustHal->SetFwFallback(m_bFwFallbackEnabled);
        }

        // create actual pavp session
        for (iRetry = 0; iRetry < PAVP_TEARDOWN_RETRY; iRetry++)
        {
            hr = ReserveSessionId(m_PavpSessionType, m_PavpSessionMode, &CurrentPavpSessionStatus, nullptr);
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Could not obtain a hardware PAVP session ID.");
                break;
            }

            PAVP_DEVICE_NORMALMESSAGE("Reserved PAVP session ID 0x%x session mode=%s",
                m_cpContext.sessionId, PavpSessionMode2Str(m_PavpSessionMode));

#ifdef WIN32
            // stout mode is used on windows
            if (m_pRootTrustHal->HasFwCaps(FIRMWARE_CAPS_PAVP_INIT))
            {
                MEDIA_FEATURE_TABLE* pSkuTable = m_pOsServices->GetSkuTable();
                if (m_PavpSessionMode == PAVP_SESSION_MODE_STOUT &&
                    pSkuTable &&
                    MEDIA_IS_SKU(pSkuTable, FtrPAVPStout) &&
                    m_PavpSessionType != PAVP_SESSION_TYPE_WIDI
                    // From Gen12+, no stout POR, IsKernelAuthenticationSupported is always false, but keep flow here for furture.
                    && m_pRootTrustHal->IsKernelAuthenticationSupported())
                {
                    hr = InitAuthenticatedKernels();
                    if (FAILED(hr))
                    {
                        PAVP_DEVICE_ASSERTMESSAGE("Failed to initialize authenticated kernels.");
                        CleanupHwSession();
                        break;
                    }
                }
            }
#endif

            // For heavy transcode, The legacy PAVP3.0 path (PAVP_SET_SLOT + GET_STREAM_KEY_EX) doesn't support
            // heavy transocde session. But still we can use (PAVP_INIT_4 + GET_STREAM_KEY_EX) for this as FW team
            // suggested. FW injects Kb key during GET_STREAM_KEY_EX so we need to call
            // IsAppIDRegisterMgmtRequired()/GetSetPavpSessionStatus(READY) before/after GET_STREAM_KEY_EX in CPavpDevice::AccessME()
            if (PAVP_SESSION_TYPE_TRANSCODE == m_PavpSessionType)
            {
                hr = InitHwTranscodeSession();
            }
            else if (PAVP_SESSION_TYPE_DECODE == m_PavpSessionType ||
                        PAVP_SESSION_TYPE_WIDI == m_PavpSessionType)
            {
                // Previously initialize transcript kernel here, but due to the time of init transcript kernel is long, might be 2 seconds or more  
                // in Cyberlink's hotplug case tear down will locate in the time area, which caused InitHwDecodeSession failed. Moved the code to
                // beginning of the function to reduce the rate fall back to legacy InitHwDecodeSession
                hr = InitHwDecodeSession();
            }
            else
            {
                PAVP_DEVICE_ASSERTMESSAGE("Unexpected pavp session type 0x%x.", m_PavpSessionType);
                hr = E_UNEXPECTED;
                break;
            }

            if (FAILED(hr))
            {
                CleanupHwSession();
                continue;
            }
            break;
        }
        m_bHwSessionIsAlive = true;

    } while (FALSE);

    MOS_TraceEventExt(EVENT_CP_CREATE, EVENT_TYPE_END, &m_eDrmType, sizeof(uint32_t), &m_eCryptoType, sizeof(uint32_t));
    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::InitHwTranscodeSession()
{
    HRESULT                             hr                          = S_OK;
    PAVP_GET_CONNECTION_STATE_PARAMS2   sConnState                  = {0, {0}, {0}};
    PAVP_SET_STREAM_KEY_PARAMS          sSetKeyParams;
    PAVP_PCH_INJECT_KEY_MODE            PchInjectKeyMode            = PAVP_PCH_INJECT_KEY_MODE_S1;
    PAVP_SESSION_STATUS                 CurrentPavpSessionStatus    = PAVP_SESSION_UNSUPPORTED;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        if (m_pRootTrustHal == nullptr ||
            m_pOsServices == nullptr ||
            m_pGpuHal == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Unexpected error: HALs have not been initialized.");
            hr = E_UNEXPECTED;
            break;
        }

#ifdef ANDROID
        // Transcode is not supported in android. This could be removed if the
        // FW version that supports the new PlayReady3.0 audio transcode init
        // flow bumpbed the version to 3.3. Without the version bump, there
        // is not a convenient way to distinguish between FW 3.2 that supports
        // the new transcode flow and FW 3.2 with legacy support for Android.
        if (!(m_pRootTrustHal->HasFwCaps(FIRMWARE_CAPS_PAVP_INIT)))
        {
            PAVP_DEVICE_ASSERTMESSAGE("firmware did not support transcode session on android.");
            hr = E_NOTIMPL;
            break;
        }
#endif
        PAVP_DEVICE_NORMALMESSAGE("Initiating session creation with RootTrust.");

        if (!IsInitialized())
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to InitHwTranscodeSession() due to uninited pavp device.");
            hr = E_UNEXPECTED;
            break;
        }

        // Verify that a valid session ID has been assigned to the device.
        if (!IsSessionIdValid())
        {
            PAVP_DEVICE_ASSERTMESSAGE("Cannot proceed without a valid PAVP session ID.");
            hr = E_UNEXPECTED;
            break;
        }

        if (m_PavpSessionType != PAVP_SESSION_TYPE_TRANSCODE)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Wrong path for initiating non-transcode session.");
            hr = E_INVALIDARG;
            break;
        }

        // Move the session status to 'initialized'.
        hr = m_pOsServices->GetSetPavpSessionStatus(
            m_PavpSessionType,
            m_PavpSessionMode,
            PAVP_SESSION_INITIALIZED,
            &CurrentPavpSessionStatus,
            nullptr,
            nullptr);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Get/Set Session Status call failed. Error = 0x%x.", hr);
            break;
        }

        if (m_pRootTrustHal->HasFwCaps(FIRMWARE_CAPS_MULTI_SESSION))
        {
            if (m_pRootTrustHal->HasFwCaps(FIRMWARE_CAPS_PAVP_INIT))
            {
                // pavp init need inject the KbKf1 key
                PchInjectKeyMode = PAVP_PCH_INJECT_KEY_MODE_S1_FR;
            }
            if (m_pGpuHal->IsAppIDRegisterMgmtRequired())
            {
                // The FW tansfers Kb across the bus in the SetSlot call for
                // for transcode, so we need to prime the GPU to know which
                // session the keys are associated with.
                PAVP_DEVICE_NORMALMESSAGE("Programming the App ID register for key injection.");
                hr = m_pOsServices->ProgramAppIDRegister(PchInjectKeyMode);
                if (FAILED(hr))
                {
                    PAVP_DEVICE_ASSERTMESSAGE("Could not program the App ID register. Error = 0x%x.", hr);
                    break;
                }
            }
        }

        if (m_pRootTrustHal->HasFwCaps(FIRMWARE_CAPS_PAVP_INIT))
        {
            // Let firmware confirm that the key has been updated and check if HDCP is on.
            PAVP_DEVICE_NORMALMESSAGE("Requesting firmware do the pavp init");
            PAVP_FW_PAVP_MODE PavpMode = static_cast<PAVP_FW_PAVP_MODE>(m_PavpSessionMode);

#if (_DEBUG || _RELEASE_INTERNAL)
            if (m_bForceLiteModeForDebug && m_bLiteModeEnableHucStreamOutForDebug)
            {
                hr = m_pRootTrustHal->EnableLiteModeHucStreamOutBit();
                if (FAILED(hr))
                {
                    PAVP_DEVICE_ASSERTMESSAGE("EnableLiteModeHucStreamOutBit call failed. Error = 0x%x.", hr);
                    break;
                }
            }
#endif

            hr = m_pRootTrustHal->PavpInit4(PavpMode, m_cpContext.sessionId);
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("RootTrust 'PavpInit4' call failed. Error = 0x%x.", hr);
                break;
            }
        }
        else
        {
            hr = m_pRootTrustHal->SetSlot(m_cpContext.sessionId);
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("PCH 'SetSlot' call failed. Error = 0x%x.", hr);
                break;
            }
        }

        if (!(m_pRootTrustHal->HasFwCaps(FIRMWARE_CAPS_MULTI_SESSION)))
        {
            // The legacy transcode session init flow has completed (for now).
            // Kb gets injected across the bus during the GetStreamKeyEx call
            // in the legacy path, and only lite mode is supported in legacy.
            hr = S_OK;
            break;
        }

        hr = m_pGpuHal->VerifyTranscodeSessionProperties(m_cpContext);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to verify that transcode session had right characteristics, hr = 0x%08x", hr);
            break;
        }

#if !EMULATE_CP_RELEASE_TIMING_TEST
#if (_DEBUG || _RELEASE_INTERNAL)
        BOOL bTestSessionInPlay;
        PAVP_DEVICE_NORMALMESSAGE("Checking to make sure the session is in play after transcode 'SetSlotEx'.");
        hr = QuerySessionInPlay(bTestSessionInPlay, m_cpContext.sessionId);
        if (SUCCEEDED(hr))
        {
            PAVP_DEVICE_NORMALMESSAGE("PAVP session in play: %d.", bTestSessionInPlay);
            if (!bTestSessionInPlay)
            {
                PAVP_DEVICE_ASSERTMESSAGE("PAVP session id %d is not in play", m_cpContext.sessionId);
            }
        }
        else
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to query session in play. Error = 0x%x.", hr);
        }
#endif
#endif

        PrintDebugRegisters();

        if (!(m_pRootTrustHal->HasFwCaps(FIRMWARE_CAPS_PAVP_INIT) || m_pRootTrustHal->HasFwCaps(FIRMWARE_CAPS_PAVP_INIT43)))
        {
            PAVP_DEVICE_NORMALMESSAGE("Retrieving memory status for ME firmware verification.");
            hr = GetConnectionState(sConnState);
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Failed to get the memory status. Error = 0x%x.", hr);
                break;
            }

            PAVP_DEVICE_NORMALMESSAGE("Requesting ME firmware memory status verification.");
            hr = m_pRootTrustHal->VerifyMemStatus(sizeof(sConnState.ProtectedMemoryStatus), sConnState.ProtectedMemoryStatus);
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Failed to verify memory status. Error = 0x%x", hr);
                break;
            }
        }

        PAVP_DEVICE_NORMALMESSAGE("PAVP session is initialized.");

    } while (FALSE);
    MOS_TraceEventExt(EVENT_CP_CREATE, EVENT_TYPE_INFO, &m_cpContext.value, sizeof(uint32_t), nullptr, 0);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::CreateOsServices(PAVP_DEVICE_CONTEXT pDeviceContext)
{
    HRESULT         hr              = S_OK;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        if (IsInitialized() ||
            m_pOsServices != nullptr)
        {
            // We are incorrectly calling this more than once (coding error somewhere).
            PAVP_DEVICE_ASSERTMESSAGE("Unexpected error: multiple calls to CreateOsServices.");
            hr = E_UNEXPECTED;
            break;
        }

        // Create an OS Services instance.
        m_pOsServices = CPavpOsServices::PavpOsServicesFactory(hr, pDeviceContext);
        if (m_pOsServices == nullptr)
        {
            CP_OS_ASSERTMESSAGE("Error allocating memory for an OS Services object.");
            hr = E_OUTOFMEMORY;
        }
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to allocate OS Services abstraction layer. Error = 0x%x.", hr);
        }
    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::PowerUpInit()
{
    HRESULT             hr                          = S_OK;
    PAVP_SESSION_STATUS CurrentPavpSessionStatus    = PAVP_SESSION_UNSUPPORTED;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        if (m_pRootTrustHal == nullptr ||
            m_pOsServices == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Unexpected error: HALs have not been initialized.");
            hr = E_UNEXPECTED;
            break;
        }

       // KMD sets all sessions to UNINITIALIZED and we are good to go.
        PAVP_DEVICE_NORMALMESSAGE("Power up init finished, setting all power up in progress sessions to uninitialized");
        hr = m_pOsServices->GetSetPavpSessionStatus(
                    m_PavpSessionType,
                    m_PavpSessionMode,
                    PAVP_SESSION_RECOVER,        // Requested.
                    &CurrentPavpSessionStatus);  // What it was set to.
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("GetSetPavpSessionStatus call failed: hr = 0x%x", hr); 
            break;
        }

        if (CurrentPavpSessionStatus != PAVP_SESSION_UNINITIALIZED)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Unable to set all sessions to uninitialized, got: %s", 
                SessionStatus2Str(CurrentPavpSessionStatus));
            hr = E_FAIL;
            break;
        }
   } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::ReserveSessionId(
    PAVP_SESSION_TYPE       eSessionType,
    PAVP_SESSION_MODE       eSessionMode,
    PAVP_SESSION_STATUS*    pPavpSessionStatus,
    PAVP_SESSION_STATUS*    pPreviousPavpSessionStatus)
{
    HRESULT                 hr                          = S_OK;
    HRESULT                 hrError                     = S_OK;
    uint32_t                  uiQuery                     = 0;
    const uint32_t            uiMaxQueries                = PAVP_MAX_RESERVE_SESSION_ID_QUERIES;
    BOOL                    bSessionSetOk               = FALSE;
    PAVP_SESSION_STATUS     PreviousPavpSessionStatus   = PAVP_SESSION_UNSUPPORTED;  // Should never be this.
    MosCp                   *pMosCp                     = nullptr;
    uint32_t                  CpTag                       = 0;
    PAVP_SESSION_STATUS     CurrentPavpSessionStatus    = PAVP_SESSION_UNSUPPORTED;

    CP_OS_FUNCTION_ENTER;

    do
    {
        // Validate input parameters.
        if (pPavpSessionStatus == nullptr ||
            m_pOsServices      == nullptr ||
            m_pOsServices->m_pOsInterface == nullptr ||
            m_pOsServices->m_pOsInterface->osCpInterface == nullptr)
        {
            hr = E_INVALIDARG;
            PAVP_DEVICE_ASSERTMESSAGE("Invalid parameter.");
            break;
        }

        pMosCp = dynamic_cast<MosCp*>(m_pOsServices->m_pOsInterface->osCpInterface);
        if(pMosCp == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("CP OS Services is NULL.");
            hr = E_UNEXPECTED;
            break;
        }

        *pPavpSessionStatus = PAVP_SESSION_UNINITIALIZED;

        // Try to move status to 'in init'.
        // Search session database to see if we can find a slot.
        // Certain PAVP session global states may prevent this, so we return the status we are in in pPavpSessionStatus.
        hr = m_pOsServices->GetSetPavpSessionStatus(
                                eSessionType,
                                eSessionMode,
                                PAVP_SESSION_IN_INIT,       // What we want.
                                pPavpSessionStatus,         // What we got.
                                &PreviousPavpSessionStatus, // What the status was before.
                                &CpTag);                     // Our pavp session context.
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("GetSetPavpSessionStatus failed. hr = 0x%x", hr);
            break;
        }
        m_cpContext.value = CpTag;

        PAVP_DEVICE_NORMALMESSAGE("After GetSet to IN_INIT, Previous session status: %s, Session status: %s",
            SessionStatus2Str(PreviousPavpSessionStatus),
            SessionStatus2Str(*pPavpSessionStatus));

        switch(*pPavpSessionStatus)
        {
            case PAVP_SESSION_UNINITIALIZED:    // Waiting to go in_init.
                MosUtilities::MosSleep(1);
                break;

            case PAVP_SESSION_IN_INIT:          // We set our desired status.
                if (PreviousPavpSessionStatus == PAVP_SESSION_TERMINATED)
                {
                    PAVP_DEVICE_NORMALMESSAGE("A process had crashed, recovering session slot");
                    hr = TerminateSessionWithConfirmation(m_cpContext.sessionId);
                    if (FAILED(hr))
                    {
                        PAVP_DEVICE_ASSERTMESSAGE("TerminateSession call failed: hr = 0x%x", hr); 
                        break;
                    }
                }
                else if ((PreviousPavpSessionStatus == PAVP_SESSION_IN_USE) &&
                    (eSessionType == PAVP_SESSION_TYPE_WIDI))
                {
                    if (uiQuery == (uiMaxQueries - 1))
                    {
                        PAVP_DEVICE_ASSERTMESSAGE("Previous arbitrator session init failed. Forcing re-init.");
                        bSessionSetOk = TRUE;
                        *pPavpSessionStatus = PAVP_SESSION_IN_INIT;
                        PreviousPavpSessionStatus = PAVP_SESSION_TERMINATED;
                        break;
                    }

                    PAVP_DEVICE_NORMALMESSAGE("Arbitrator session init already in progress, polling for completion");
                    MosUtilities::MosSleep(1);
                    break;
                }
                bSessionSetOk = TRUE;
                break;

            case PAVP_SESSION_INITIALIZED:
                if (eSessionType != PAVP_SESSION_TYPE_WIDI)
                {
                    hr = E_INVALIDARG;
                    PAVP_DEVICE_ASSERTMESSAGE("Unexpected Pavp session status: %s", SessionStatus2Str(*pPavpSessionStatus));
                    break;
                }

                if (uiQuery == uiMaxQueries - 1)
                {
                    PAVP_DEVICE_ASSERTMESSAGE("Previous arbitrator session init failed. Forcing re-init.");
                    bSessionSetOk = TRUE;
                    *pPavpSessionStatus = PAVP_SESSION_IN_INIT;
                    PreviousPavpSessionStatus = PAVP_SESSION_TERMINATED;
                    break;
                }

                // Another process is initializing the arbitrator session
                PAVP_DEVICE_NORMALMESSAGE("Arbitrator session init already in progress, polling for completion");
                MosUtilities::MosSleep(1);
                break;

            case PAVP_SESSION_READY:
                if (eSessionType != PAVP_SESSION_TYPE_WIDI)
                {
                    hr = E_INVALIDARG;
                    PAVP_DEVICE_ASSERTMESSAGE("Unexpected Pavp session status: %s", SessionStatus2Str(*pPavpSessionStatus));
                    break;
                }

                // This is the arbitrator session, and another call had
                // previously initialized it.
                bSessionSetOk = TRUE;
                break;

            case PAVP_SESSION_POWERUP:
                // KMD has booted and we need to do powerup init.
                // Session states are set to 'power up in progress' to lock 
                // as this needs to complete before sessions can be used.
                // If this succeeds, session states are set to 'uninitialized'.
                hr = PowerUpInit();
                if (FAILED(hr))
                {
                    PAVP_DEVICE_ASSERTMESSAGE("Power up init failed, returning all sessions to POWERUP. hr = 0x%x", hr);
                    // Return all sessions to POWERUP - This resets HW back to boot state.
                    hrError = m_pOsServices->GetSetPavpSessionStatus(
                                                    m_PavpSessionType,
                                                    m_PavpSessionMode,
                                                    PAVP_SESSION_POWERUP,
                                                    &CurrentPavpSessionStatus); 
                    if (FAILED(hrError))
                    {
                        PAVP_DEVICE_ASSERTMESSAGE("GetSetPavpSessionStatus failed. hr = 0x%x", hrError);
                    }
                    // Fail this session.
                }
                // We retry going to in_init.
                break;

            case PAVP_SESSION_POWERUP_IN_PROGRESS:
                // Another process owns powerup, retry after a delay.
                PAVP_DEVICE_NORMALMESSAGE("Power up in progress, trying again");
                MosUtilities::MosSleep(1);

                // If after several tries we give up and mark all sessions powerup to let another session try.
                if (uiQuery > PAVP_MAX_POWERUP_IN_PROGRESS_QUERIES)
                {
                    PAVP_DEVICE_ASSERTMESSAGE("Power up in progress timed out (other process crashed?)");
                    hr = m_pOsServices->GetSetPavpSessionStatus(
                                                        m_PavpSessionType,
                                                        m_PavpSessionMode,
                                                        PAVP_SESSION_POWERUP,
                                                        &CurrentPavpSessionStatus); 
                    if (FAILED(hr))
                    {
                        PAVP_DEVICE_ASSERTMESSAGE("GetSetPavpSessionStatus failed. hr = 0x%x", hr);
                    }
                }

                break;

            default:
                hr = E_INVALIDARG;
                PAVP_DEVICE_ASSERTMESSAGE("Unexpected Pavp session status: %s", SessionStatus2Str(*pPavpSessionStatus));
                break;
        }

        if (FAILED(hr))
        {
            // The previous 'break' only broke out of the 'switch'.
            break;
        }

       if (++uiQuery >= uiMaxQueries)
        {
            hr = E_FAIL;
            PAVP_DEVICE_ASSERTMESSAGE("Pavp set session IN_INIT timeout");
            break;
        }
        // update cp context in mos cp interface
        pMosCp->UpdateCpContext(m_cpContext.value);

    } while (bSessionSetOk == FALSE);

    if (pPreviousPavpSessionStatus != nullptr)
    {
        *pPreviousPavpSessionStatus = PreviousPavpSessionStatus;
    }

    CP_OS_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::SetSessionType(PAVP_SESSION_TYPE eSessionType)
{
    HRESULT hr = S_OK;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        if (m_bHwSessionIsAlive)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to SetSessionType() due to existing hw session.");
            hr = E_UNEXPECTED;
            break;
        }

        switch (eSessionType)
        {
            case PAVP_SESSION_TYPE_DECODE:
            case PAVP_SESSION_TYPE_TRANSCODE:
            case PAVP_SESSION_TYPE_WIDI:
                m_PavpSessionType = eSessionType;
                hr = S_OK;
                break;
            default:
                PAVP_DEVICE_ASSERTMESSAGE("Invalid session type argument.");
                hr = E_INVALIDARG;
                break;
        }
    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::SetSessionMode(PAVP_SESSION_MODE eSessionMode)
{
    HRESULT hr = S_OK;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        if (m_bHwSessionIsAlive)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to SetSessionMode() due to existing hw session.");
            hr = E_UNEXPECTED;
            break;
        }
        if (eSessionMode == PAVP_SESSION_MODE_INVALID)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Invalid session mode argument.");
            hr = E_INVALIDARG;
            break;
        }
#if (_DEBUG || _RELEASE_INTERNAL)
        {
            DWORD dwRegKeyForceLiteModeForDebug = 0;
            DWORD dwRegKeyLiteModeEnableHucStreamOutForDebug = 0;

            if (m_pOsServices)
            {
                m_pOsServices->ReadRegistry(__MEDIA_USER_FEATURE_VALUE_PAVP_FORCE_LITE_MODE_FOR_DEBUG_ID,
                   &dwRegKeyForceLiteModeForDebug);

                m_bForceLiteModeForDebug = (dwRegKeyForceLiteModeForDebug != 0);

                if (m_bForceLiteModeForDebug)
                {
                    PAVP_DEVICE_ASSERTMESSAGE("regkey 'PAVP Force Lite Mode For Debug' is on, so we force lite mode for debug.");
                    eSessionMode = PAVP_SESSION_MODE_LITE;
                }

                m_pOsServices->ReadRegistry(__MEDIA_USER_FEATURE_VALUE_PAVP_LITE_MODE_ENABLE_HUC_STREAMOUT_FOR_DEBUG_ID,
                    &dwRegKeyLiteModeEnableHucStreamOutForDebug);

                m_bLiteModeEnableHucStreamOutForDebug = (dwRegKeyLiteModeEnableHucStreamOutForDebug != 0);

                if (m_bLiteModeEnableHucStreamOutForDebug)
                {
                    PAVP_DEVICE_ASSERTMESSAGE("regkey 'PAVP Lite Mode Enable HucStreamOut For Debug' is on.");
                }
            }
        }
#endif
        m_PavpSessionMode = eSessionMode;
    } while (FALSE);
    
    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

#if (_DEBUG || _RELEASE_INTERNAL)
void CPavpDevice::SetPavpCryptSessionType(PAVP_CRYPTO_SESSION_TYPE ePavpCryptoSessionType)
{
    m_PavpCryptoSessionMode = ePavpCryptoSessionType;
}
#endif

HRESULT CPavpDevice::SetDrmType(PAVP_DRM_TYPE eDrmType)
{
    HRESULT hr = S_OK;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        if (m_bHwSessionIsAlive)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to SetDrmType() due to existing hw session.");
            hr = E_UNEXPECTED;
            break;
        }

        switch (eDrmType)
        {
        case PAVP_DRM_TYPE_LEGACY:
        case PAVP_DRM_TYPE_BLURAY:
        case PAVP_DRM_TYPE_MEDIA_VAULT:
        case PAVP_DRM_TYPE_WIDEVINE:
        case PAVP_DRM_TYPE_PLAYREADY:
            m_eDrmType = eDrmType;
            hr = S_OK;
            break;
        default:
            PAVP_DEVICE_ASSERTMESSAGE("Invalid session mode argument.");
            hr = E_INVALIDARG;
            break;
        }
    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;

}

HRESULT CPavpDevice::SetEncryptionType(PAVP_ENCRYPTION_TYPE eEncryptionType)
{
    HRESULT hr = E_INVALIDARG;

    PAVP_DEVICE_FUNCTION_ENTER;

    switch(eEncryptionType)
    {
        case PAVP_ENCRYPTION_NONE:
        case PAVP_ENCRYPTION_AES128_CTR:
        case PAVP_ENCRYPTION_AES128_CBC:
        case PAVP_ENCRYPTION_DECE_AES128_CTR:
            m_eCryptoType = eEncryptionType;
            hr = S_OK;
            break;
        default:
            PAVP_DEVICE_ASSERTMESSAGE("Invalid encryption type: %d", eEncryptionType);
            break;
    }

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}


// define this for a dump of surface bytes for debug
#define DEBUGLOGSURFACEBYTES 0
HRESULT CPavpDevice::DoEncryptionBlt(PAVP_ENCRYPT_BLT_PARAMS &sEncryptBlt) CONST
{
    HRESULT                 hr                      = S_OK;
    DWORD                   dwSrcSize               = 0;
    PMOS_RESOURCE           LinearBufSrc            = nullptr;
    PMOS_RESOURCE           LinearBufDst            = nullptr;
    PAVP_ENCRYPT_BLT_PARAMS sEncryptionBltLinearBuf = {0};

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        // Verify input parameters.
        if (m_pOsServices == nullptr ||
            sEncryptBlt.SrcResource == nullptr ||
            sEncryptBlt.DstResource == nullptr ||
            sEncryptBlt.dwDstResourceSize == 0)      
        {
            PAVP_DEVICE_ASSERTMESSAGE("Invalid argument.");
            hr = E_INVALIDARG;
            break;
        }

        if (!IsInitialized())
        {
            PAVP_DEVICE_ASSERTMESSAGE("Init() must be called first.");
            hr = E_UNEXPECTED;
            break;
        }

        m_pOsServices->UpdateResourceUsageType(sEncryptBlt.SrcResource, MOS_HW_RESOURCE_USAGE_CP_EXTERNAL_READ);

        // Notes:
        // Validations that are done in the DDI class:
        //  * Source and destination resources are valid ones.
        //  * Source resource is in video memory.
        //  * Destination resource is in system memory.
        //  * SrcSubResourceIndex and DstSubResourceIndex are 0.
        //  * sEncryptBlt.DstResourceSize is positive.
        //
        // This function assumes only subresource 0 is used in both resources.
        // It is the DDI responsibility to verify it.
        // Using different subresource is not possible from app side because no such interface is exposed to apps.
        // (see IDirect3DCryptoSession9::EncryptionBlt documentation).
        //
        // In the current implementation, the MFX cryptocopy type used here is CRYPTO_COPY_TYPE_CLEAR_TO_SERPENT_ENCRYPT
        // This is for debug of heavy surface encryption only. It reads from a clear destination surface and writes 
        // heavy encrypted data to the destination surface. This happens only if the PAVP session is in heavy mode.
        // If in lite mode it just does a memcopy.
        //
        // SrcResource and DstResource must have the same size, pitch, width & height (some enforced by DX runtime)
        // They are allocated using IDirect3DDevice9Ex::CreateOffscreenPlainSurfaceEx.
        // SrcResource must be video memory. DstResource should be a system memory surface.

        // Make sure that the src has enough size, we only copy up to sEncryptBlt.dwDstResourceSize       
        dwSrcSize = m_pOsServices->GetResourceSize(sEncryptBlt.SrcResource);
        if (dwSrcSize < sEncryptBlt.dwDstResourceSize)
        {
            hr = E_INVALIDARG;
            PAVP_DEVICE_ASSERTMESSAGE("The src size=[%d] is less than the dst=[%d]", dwSrcSize, sEncryptBlt.dwDstResourceSize);
            break;
        }

        // To manage tiling:
        // The src surface via default allocation by the app is tiled, when the app copies the cleartext into the video memory  
        // it is "swizzled" (AKA tiled). Swizzling occurs only via CPU reads/writes and only applies to video memory surfaces
        // The src surface must be converted to linear (non-tiled) before we encrypt it as an encrypted and tiled surface 
        // isn't useful.
        // Create a new linear video memory surface and copy the tiled src to the linear cryptocopy src via the CPU
        hr = m_pOsServices->CloneToLinearVideoResource(
                                sEncryptBlt.SrcResource,         // The tiled resource.
                                &LinearBufSrc,                   // The new linear resource we copy to.
                                sEncryptBlt.SrcResource,         // The source data resource we copy from.
                                sEncryptBlt.dwDstResourceSize);  // The size of the data copied.
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to copy the source resource into a linear resource in the video memory.");
            break;
        }

        // create a new video memory surface as the crypto copy can't copy to system memory. 
        hr = m_pOsServices->CloneToLinearVideoResource(
                                sEncryptBlt.SrcResource,        // Clone format, size etc from this surface
                                &LinearBufDst);                 // New surface crypto copy Destination
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to create a linear resource for destination.");
            break;
        }

        sEncryptionBltLinearBuf             = sEncryptBlt;
        sEncryptionBltLinearBuf.SrcResource = LinearBufSrc;
        sEncryptionBltLinearBuf.DstResource = LinearBufDst;

        hr = DoEncryptionBltLinearBuffer(sEncryptionBltLinearBuf);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to DoEncryptionBltLinearBuffer(). Error = 0x%x.", hr);
            break;
        }

        // do a memcopy of the video mem encrypted surface to the system memory output surface
        hr = m_pOsServices->CopyVideoResourceToSystemResource(
            LinearBufDst,
            sEncryptBlt.DstResource,
            sEncryptBlt.dwDstResourceSize);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to copy the crypto dst resource into the final sys surface dst resource.");
            break;
        }

        // do a memcopy of counter since it's incremented in AES-count mode.
        MOS_SecureMemcpy(sEncryptBlt.pIV, sizeof(sEncryptBlt.pIV), sEncryptionBltLinearBuf.pIV, sizeof(sEncryptionBltLinearBuf.pIV));

#if ((_DEBUG || _RELEASE_INTERNAL) && DEBUGLOGSURFACEBYTES)
        DWORD dwSrcPitch = m_pOsServices->GetResourcePitch(sEncryptBlt.SrcResource);

        PAVP_DEVICE_NORMALMESSAGE("Clear Text src surface bytes: ");
        m_pOsServices->LogSurfaceBytes(sEncryptBlt.SrcResource, dwSrcPitch * 3);

        // if in heavy mode destination will be heavy encrypted
        PAVP_DEVICE_NORMALMESSAGE("Encrypted Text dst surface bytes: ");
        m_pOsServices->LogSurfaceBytes(sEncryptBlt.DstResource, dwSrcPitch * 3);
#endif
    } while(FALSE);

    if (m_pOsServices)
    {
        if (LinearBufSrc)
        {
            m_pOsServices->FreeResource(LinearBufSrc);
        }
        if (LinearBufDst)
        {
            m_pOsServices->FreeResource(LinearBufDst);
        }
    }

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::DoEncryptionBltLinearBuffer(PAVP_ENCRYPT_BLT_PARAMS &sEncryptBlt) CONST
{
    HRESULT                               hr                  = S_OK;
    DWORD                                 dwSrcSize           = 0;
    PAVP_GPU_HAL_CRYPTO_COPY_PARAMS       CryptoCopyParams;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        MOS_ZeroMemory(&CryptoCopyParams, sizeof(CryptoCopyParams));

        // Verify input parameters.
        if (m_pOsServices == nullptr ||
            sEncryptBlt.SrcResource == nullptr ||
            sEncryptBlt.DstResource == nullptr ||
            sEncryptBlt.dwDstResourceSize == 0)      
        {
            PAVP_DEVICE_ASSERTMESSAGE("Invalid argument.");
            hr = E_INVALIDARG;
            break;
        }

        if (!IsInitialized())
        {
            PAVP_DEVICE_ASSERTMESSAGE("Init() must be called first.");
            hr = E_UNEXPECTED;
            break;
        }

         MOS_TraceEventExt(EVENT_BLT_ENC, EVENT_TYPE_START, &sEncryptBlt.dwDstResourceSize, sizeof(uint32_t), nullptr, 0);
        
        // Make sure that the src has enough size, we only copy up to sEncryptBlt.dwDstResourceSize  
        dwSrcSize = m_pOsServices->GetResourceSize(sEncryptBlt.SrcResource);
        if (dwSrcSize < sEncryptBlt.dwDstResourceSize)
        {
            hr = E_INVALIDARG;
            PAVP_DEVICE_ASSERTMESSAGE("The src size=[%d] is less than the dst=[%d]", dwSrcSize, sEncryptBlt.dwDstResourceSize);
            break;
        }

        // Fill in sCryptoCopyParams with the parameters needed for the "Crypto Copy" command.
        CryptoCopyParams.SrcResource     = sEncryptBlt.SrcResource;
        CryptoCopyParams.DstResource     = sEncryptBlt.DstResource;
        CryptoCopyParams.dwDataSize      = sEncryptBlt.dwDstResourceSize;             // The size in bytes of the amount of data to be decrypted.
        CryptoCopyParams.eCryptoCopyType = CRYPTO_COPY_TYPE_CLEAR_TO_SERPENT_ENCRYPT; // Operation type.
        CryptoCopyParams.dwAppId         = m_cpContext.sessionId;                     // App ID of the current session.

        PAVP_DEVICE_NORMALMESSAGE("Executing the 'CRYPTO_COPY_TYPE_CLEAR_TO_SERPENT_ENCRYPT' command, crypto copy size: %d bytes.", CryptoCopyParams.dwDataSize);
        
        if (m_eDrmType == PAVP_DRM_TYPE_INDIRECT_DISPLAY)
        {
            PAVP_DEVICE_ASSERT(m_PavpSessionMode == PAVP_SESSION_MODE_HEAVY);
            PAVP_DEVICE_VERBOSEMESSAGE("SUBMITTING CLEAR to AES");
            CryptoCopyParams.eCryptoCopyType = CRYPTO_COPY_TYPE_CLEAR_TO_AES_ENCRYPT;
            if (m_pGpuHal->IsOneWLRequired())
            {
                CryptoCopyParams.pHwctr = (uint32_t *)sEncryptBlt.pIV;
                hr = BuildAndSubmitCommand(CRYPTO_COPY, &CryptoCopyParams);
                if (FAILED(hr))
                {
                    PAVP_DEVICE_ASSERTMESSAGE("Failed to submit the 'Crypto Copy' command. Error = 0x%x.", hr);
                    break;
                }
                std::reverse(std::begin(sEncryptBlt.pIV), std::end(sEncryptBlt.pIV));

                PAVP_DEVICE_ASSERTMESSAGE("WIDI COUNTER for Crypto copy command: %08x %08x %08x %08x\n",
                    sEncryptBlt.pIV[0],
                    sEncryptBlt.pIV[1],
                    sEncryptBlt.pIV[2],
                    sEncryptBlt.pIV[3]);
            }
            else
            {
                CryptoCopyParams.dwAesCtr[0] = sEncryptBlt.pIV[0];  // RIV xor streamCtr[0]
                CryptoCopyParams.dwAesCtr[1] = sEncryptBlt.pIV[1];  // RIV xor streamCtr[1]
                CryptoCopyParams.dwAesCtr[2] = sEncryptBlt.pIV[2];  // inputCtr[2]
                CryptoCopyParams.dwAesCtr[3] = sEncryptBlt.pIV[3];  // inputCtr[3]

                // The input is BE - we want it to be little endian
                CounterSwapEndian(CryptoCopyParams.dwAesCtr);

                // Read out the widi counter before we submit the crypto copy command
                hr = GetWiDiCounter(sEncryptBlt.pIV);
                if (FAILED(hr))
                {
                    PAVP_DEVICE_ASSERTMESSAGE("Failed to get WiDi counter. Error = 0x%x.", hr);
                    break;
                }
                PAVP_DEVICE_ASSERTMESSAGE("WIDI COUNTER (before Crypto copy command): %08x %08x %08x %08x\n",
                    sEncryptBlt.pIV[0],
                    sEncryptBlt.pIV[1],
                    sEncryptBlt.pIV[2],
                    sEncryptBlt.pIV[3]);

                hr = BuildAndSubmitCommand(CRYPTO_COPY, &CryptoCopyParams);
                if (FAILED(hr))
                {
                    PAVP_DEVICE_ASSERTMESSAGE("Failed to submit the 'Crypto Copy' command. Error = 0x%x.", hr);
                    break;
                }
            }
        }
        else
        {
            hr = BuildAndSubmitCommand(CRYPTO_COPY, &CryptoCopyParams);
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Failed to submit the 'Crypto Copy' command. Error = 0x%x.", hr);
                break;
            }
        }

    } while(FALSE);

    if (FAILED(hr) && hr != E_INVALIDARG)
    {
        // for telemetry events
        uint32_t uiDrmType = m_eDrmType;
        uint32_t uiCryptoType = m_eCryptoType;
        MosUtilities::MosGfxInfoRTErr(
            1,
            GFXINFO_COMPID_UMD_MEDIA_CP,
            GFXINFO_ERRID_CP_DX11_EncryptionBlt,
            hr,
            2,
            "data2", GFXINFO_PTYPE_UINT32, &uiDrmType,
            "data3", GFXINFO_PTYPE_UINT32, &uiCryptoType);
    }

    MOS_TraceEventExt(EVENT_BLT_ENC, EVENT_TYPE_END, &hr, sizeof(uint32_t), nullptr, 0);
    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::GetWiDiCounter(DWORD IV[4]) CONST
{
    HRESULT                             hr = S_OK;              // TODO: Double check SessionIdValue, or hardcode 15
    PAVP_GPU_HAL_QUERY_STATUS_PARAMS    sQueryStatusParams = { m_cpContext.sessionId, 0, PAVP_QUERY_STATUS_WIDI_COUNTER, { 0 }, { 0 } };
    uint32_t                            regHwCounter[4]   = {0, 0, 0, 0};
    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        // Submit a 'query status' command to HW in order to get the widi counter.

        hr = BuildAndSubmitCommand(QUERY_STATUS, &sQueryStatusParams);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to get the widi counter from GPU HAL. Error = 0x%x.", hr);
            break;
        }

        IV[2] = sQueryStatusParams.OutData[1];
        IV[3] = sQueryStatusParams.OutData[0];
        PAVP_DEVICE_NORMALMESSAGE("The WIDI COUNTER FROM THE HW was found to be:");
        PAVP_DEVICE_NORMALMESSAGE("IV: %08x %08x %08x %08x\n", IV[0], IV[1], IV[2], IV[3]);
        PAVP_DEVICE_NORMALMESSAGE("OUTDATA: %08x %08x %08x %08x\n", sQueryStatusParams.OutData[0],
                                                                    sQueryStatusParams.OutData[1],
                                                                    sQueryStatusParams.OutData[2],
                                                                    sQueryStatusParams.OutData[3]);
    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

inline int32_t CPavpDevice::AssembleAudioCryptoCopyParams(
    uint32_t                               *pIV,
    PMOS_RESOURCE                          pInputResource,
    PMOS_RESOURCE                          pOutputResource,
    uint32_t                               bufSize,
    uint32_t                               uiAesMode,
    uint32_t                               uiCryptoCopyMode,
    uint32_t                               offset,
    PAVP_GPU_HAL_MULTI_CRYPTO_COPY_PARAMS  &sCryptoCopyParams)
{
    int32_t hr = 0;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        if (pIV             == nullptr ||
            pInputResource  == nullptr ||
            pOutputResource == nullptr ||
            bufSize         == 0)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Invalid parameters passed in to AssembleAudioCryptoCopyParams.");
            hr = E_INVALIDARG;
            break;
        }

        sCryptoCopyParams.eAesMode        = static_cast<MHW_PAVP_CRYPTO_COPY_AES_MODE>(uiAesMode);
        sCryptoCopyParams.eCryptoCopyType = static_cast<MHW_PAVP_CRYPTO_COPY_TYPE>(uiCryptoCopyMode);

        if (uiAesMode == CRYPTO_COPY_AES_MODE_CBC_SKIP)
        {
            sCryptoCopyParams.uiEncryptedBytes = bufSize - (bufSize % 16);
        }
        else
        {
            sCryptoCopyParams.uiEncryptedBytes = bufSize;
        }

        sCryptoCopyParams.uiClearBytes = 0;
        sCryptoCopyParams.SrcResource  = pInputResource;
        sCryptoCopyParams.DstResource  = pOutputResource;
        sCryptoCopyParams.dwDataSize   = bufSize;
        sCryptoCopyParams.dwAppId      = m_cpContext.sessionId;
        sCryptoCopyParams.offset       = offset;
        sCryptoCopyParams.dwAesCtr[0]  = pIV[0];
        sCryptoCopyParams.dwAesCtr[1]  = pIV[1];
        sCryptoCopyParams.dwAesCtr[2]  = pIV[2];
        sCryptoCopyParams.dwAesCtr[3]  = pIV[3];

    } while (false);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

uint32_t CPavpDevice::FindSubsamplePos(std::vector<size_t> &eachRegionOffset, uint32_t offset)
{
    PAVP_DEVICE_FUNCTION_ENTER;

    uint32_t low  = 0;
    uint32_t high = eachRegionOffset.size();
    while (low <= high)
    {
        uint32_t mid = low + (high - low) / 2;
        if (eachRegionOffset[mid] > offset)
        {
            high = mid - 1;
        }
        else
        {
            low = mid + 1;
        }
    }

    return high;
}

int32_t CPavpDevice::TranscryptAudioLinearBuffer(
    AudioBufferMap      &inputBufferMap,        // [in]
    uint8_t             *inputAllBuffer,        // [in]
    size_t              bufSize,                // [in]
    uint8_t             *outputAllBuffer,       // [out]
    AudioDecryptParams  &m_AudioDecryptParams)  // [in]
{
    int32_t              hr           = 0;
    GMM_GFX_SIZE_T       resourceSize = 0L;
    MOS_RESOURCE         aesResource;
    MOS_RESOURCE         serpentResource;
    bool                 wasResourceAllocated     = false;
    bool                 serpentResourceLocalOnly = false;
    MEDIA_FEATURE_TABLE  *pSkuTable               = nullptr;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        if (inputBufferMap.size() == 0       ||
            inputAllBuffer        == nullptr ||
            bufSize               == 0       ||
            outputAllBuffer       == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Invalid parameters passed in to TranscryptAudioLinearBuffer.");
            hr = E_INVALIDARG;
            break;
        }

        if (!IsInitialized())
        {
            PAVP_DEVICE_ASSERTMESSAGE("Init() must be called first.");
            hr = E_UNEXPECTED;
            break;
        }

        if (m_pOsServices == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("m_pOsServices was NULL.");
            hr = E_UNEXPECTED;
            break;
        }

        if (!IsSessionIdValid())
        {
            PAVP_DEVICE_ASSERTMESSAGE("Invalid PAVP session ID.");
            hr = E_FAIL;
            break;
        }

        if (!m_pDeviceContext)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Invalid Device Context.");
        }

        pSkuTable = m_pOsServices->GetSkuTable();

        if (pSkuTable == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("pSkuTable was NULL.");
            hr = E_UNEXPECTED;
            break;
        }

        if (MEDIA_IS_SKU(pSkuTable, FtrLimitedLMemBar) ||
            MEDIA_IS_SKU(pSkuTable, FtrPAVPLMemOnly))
        {
            serpentResourceLocalOnly = true;
            PAVP_DEVICE_VERBOSEMESSAGE("serpentResourceLocalOnly = true.");
        }

        hr = m_pOsServices->PrepareDecryptionResources(
            inputAllBuffer,
            bufSize,
            &aesResource,
            &serpentResource,
            &resourceSize);

        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to prepare decryption resources. Error = 0x%x.", hr);
            break;
        }

        wasResourceAllocated = true;

        std::vector<PAVP_GPU_HAL_MULTI_CRYPTO_COPY_PARAMS> sBuildMultiCryptoCopyParams;
        PAVP_GPU_HAL_MULTI_CRYPTO_COPY_PARAMS              sBuildCryptoCopyParam = {};

        uint32_t decryptIV[4]     = {};
        uint32_t encryptIV[4]     = {};
        uint32_t uiAesMode        = CRYPTO_COPY_AES_MODE_CTR;
        uint32_t uiCryptoCopyMode = CRYPTO_COPY_TYPE_AES_DECRYPT;

        for (auto it = inputBufferMap.begin(); it != inputBufferMap.end(); ++it)
        {
            auto &&IV = it->first;

            // calculate decryptIV
            MOS_SecureMemcpy(&decryptIV, sizeof(decryptIV), &IV, sizeof(IV));

            // the current buffer may be encrypted(AES-CBC or AES-CTR) or clear

            // 1.if encrypted by AES-CBC
            // its decryptIV is the previous block of the current encrypted buffer or initial value
            // The composition structure of the subsample which the current buffer belongs may be
            // eg:{E1,C1,C2,E2,C3,C4,E4,.....}

            // if current buffer is generated by rollover and belongs to E2,there will two situations
            // a. if (it->second.startAesEncCounter > 0),means the previous block is the last 16 Bytes of E1
            // b. (it->second.startAesEncCounter = 0),means the previous block belongs to E2

            // if current buffer isn't generated by rollover,there will two situations
            // a. if (it->second.startAesEncCounter > 0),means the previous block is the last 16 Bytes of E1
            // b. if (it->second.startAesEncCounter = 0),means current buffer is E1,decryptIV is initial value

            // 2.if encrypted by AES-CTR
            // if current buffer is generated by rollover,its decryptIV need to add startAesEncCounter
            // if current buffer isn't generated by rollover,this buffer will be full encrypted according to MSFT PlayReady official website

            if (it->second.wasEncrypted)
            {
                uiCryptoCopyMode = m_AudioDecryptParams.isCBC ? CRYPTO_COPY_TYPE_AES_DECRYPT_PARTIAL : CRYPTO_COPY_TYPE_AES_DECRYPT;
                uiAesMode        = m_AudioDecryptParams.isCBC ? CRYPTO_COPY_AES_MODE_CBC_SKIP : CRYPTO_COPY_AES_MODE_CTR;

                if (m_AudioDecryptParams.isCBC)
                {
                    uint8_t tempIv[AES_BLOCK_SIZE] = {0};

                    if (it->second.wasGenerateByRollover)
                    {
                        // decryptIV is the previous block of the current encrypted buffer
                        uint32_t  decryptIvLocation = it->second.startAesEncCounter > 0 ?
                            it->second.offsetBeforePadding - (m_AudioDecryptParams.numClearStripeBlocks) * AES_BLOCK_SIZE - AES_BLOCK_SIZE :
                            it->second.offsetBeforePadding - AES_BLOCK_SIZE;
                        MOS_SecureMemcpy(tempIv, AES_BLOCK_SIZE, m_AudioDecryptParams.encryptedBuf + decryptIvLocation, AES_BLOCK_SIZE);
                    }
                    else
                    {
                        if (it->second.startAesEncCounter > 0)
                        {
                            // decryptIV is the previous block of the current encrypted buffer
                            uint32_t decryptIvLocation = it->second.offsetBeforePadding - (m_AudioDecryptParams.numClearStripeBlocks) * AES_BLOCK_SIZE - AES_BLOCK_SIZE;
                            MOS_SecureMemcpy(tempIv, AES_BLOCK_SIZE, m_AudioDecryptParams.encryptedBuf + decryptIvLocation, AES_BLOCK_SIZE);
                        }
                        else
                        {
                            // Endian-swap decrypt IV for CBC decryption
                            // This should not be required according to Fulsim reference dumps or Bspec, pending investigation
                            MOS_SecureMemcpy(tempIv, AES_BLOCK_SIZE / 2, &decryptIV[2], AES_BLOCK_SIZE/2);
                            SwapEndian64(tempIv);
                            MOS_SecureMemcpy(&tempIv[AES_BLOCK_SIZE / 2], AES_BLOCK_SIZE / 2, &decryptIV[0], AES_BLOCK_SIZE / 2);
                            SwapEndian64(&tempIv[AES_BLOCK_SIZE / 2]);
                        }
                    }

                    MOS_SecureMemcpy(decryptIV, AES_BLOCK_SIZE, tempIv, AES_BLOCK_SIZE);
                }
                else if (it->second.wasGenerateByRollover)
                {
                    decryptIV[0] = decryptIV[0] + it->second.startAesEncCounter;
                }
            }
            else
            {
                uiAesMode        = CRYPTO_COPY_AES_MODE_CTR;
                uiCryptoCopyMode = CRYPTO_COPY_TYPE_CLEAR_TO_SERPENT_ENCRYPT;
            }

            PAVP_DEVICE_VERBOSEMESSAGE("Decrypt IV is 0x%08x 0x%08x 0x%08x 0x%08x", decryptIV[0], decryptIV[1], decryptIV[2], decryptIV[3]);

            // Prepare input parameters for clear or encrypted to Serpent operation
            hr = AssembleAudioCryptoCopyParams(
                decryptIV,
                &aesResource,
                &serpentResource,
                it->second.bufferSizeBeforePadding,
                uiAesMode,
                uiCryptoCopyMode,
                it->second.offsetAfterPadding,
                sBuildCryptoCopyParam);

            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Failed to AssembleAudioCryptoCopyParams, hr = 0x%x", hr);
                break;
            }

            sBuildMultiCryptoCopyParams.push_back(sBuildCryptoCopyParam);

            // calculate encryptIV
            MOS_SecureMemcpy(&encryptIV, sizeof(encryptIV), &IV, sizeof(IV));

            // MSFT requires that returned audio samples be encrypted using an 8-byte IV
            encryptIV[0] = it->second.startAesEncCounter;
            encryptIV[1] = 0;
            PAVP_DEVICE_VERBOSEMESSAGE("Encrypt IV is 0x%08x 0x%08x 0x%08x 0x%08x", encryptIV[0], encryptIV[1], encryptIV[2], encryptIV[3]);

            // Prepare input parameters for Serpent to AES operation
            hr = AssembleAudioCryptoCopyParams(
                encryptIV,
                &serpentResource,
                &aesResource,
                it->second.bufferSizeBeforePadding,
                CRYPTO_COPY_AES_MODE_CTR,
                CRYPTO_COPY_TYPE_SERPENT_TO_AES_ENCRYPT,
                it->second.offsetAfterPadding,
                sBuildCryptoCopyParam);

            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Failed to AssembleAudioCryptoCopyParams, hr = 0x%x", hr);
                break;
            }

            sBuildMultiCryptoCopyParams.push_back(sBuildCryptoCopyParam);
        }

        hr = BuildAndSubmitMultiCryptoCopyCommand(&sBuildMultiCryptoCopyParams[0], sBuildMultiCryptoCopyParams.size());
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to submit the 'Crypto Copy' command. Error = 0x%x.", hr);
            break;
        }
    } while (false);

    if (wasResourceAllocated)
    {
        hr = m_pOsServices->ReleaseDecryptionResources(
            &serpentResource,
            &aesResource,
            bufSize,
            outputAllBuffer);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to retrieve destination buffer and release decryption resources. Error = 0x%x.", hr);
        }
    }
    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::TranscryptLinearBuffer(
    PDWORD          pDecryptIV,     // [in]
    PDWORD          pEncryptIV,     // [in]
    PBYTE           pSrcBuffer,     // [in]
    PBYTE           pDstBuffer,     // [out]
    SIZE_T          bufSize,        // [in]
    uint32_t          uiAesMode)      // [in]
{
    HRESULT                         hr = S_OK;
    GMM_GFX_SIZE_T                  resourceSize = 0L;
    MOS_RESOURCE                    aesResource;
    MOS_RESOURCE                    serpentResource;
    BOOL                            didAllocateResources = FALSE;
    PAVP_GPU_HAL_CRYPTO_COPY_PARAMS cryptoCopyParams = {};
    BOOL                            serpentResourceLocalOnly = FALSE;
    MEDIA_FEATURE_TABLE*            pSkuTable = nullptr;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        if (pDecryptIV == nullptr ||
            pEncryptIV == nullptr ||
            pSrcBuffer == nullptr ||
            pDstBuffer == nullptr ||
            bufSize == 0)
        {
            hr = E_INVALIDARG;
            break;
        }

        if (!IsInitialized())
        {
            PAVP_DEVICE_ASSERTMESSAGE("Init() must be called first.");
            hr = E_UNEXPECTED;
            break;
        }

        if (m_pOsServices == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("m_pOsServices was NULL.");
            hr = E_UNEXPECTED;
            break;
        }

        if (!IsSessionIdValid())
        {
            PAVP_DEVICE_ASSERTMESSAGE("Invalid PAVP session ID.");
            hr = E_FAIL;
            break;
        }

        if (!m_pDeviceContext)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Invalid Device Context.");
        }

        pSkuTable = m_pOsServices->GetSkuTable();
        if (pSkuTable == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("pSkuTable was NULL.");
            hr = E_UNEXPECTED;
            break;
        }
        if (MEDIA_IS_SKU(pSkuTable, FtrLimitedLMemBar)||
            MEDIA_IS_SKU(pSkuTable, FtrPAVPLMemOnly))
        {
            serpentResourceLocalOnly = true;
            PAVP_DEVICE_VERBOSEMESSAGE("serpentResourceLocalOnly = true.");
        }

        // Do the AES-to-serprent CryptoCopy
        hr = m_pOsServices->PrepareDecryptionResources(
            pSrcBuffer,
            bufSize,
            &aesResource,
            &serpentResource,
            &resourceSize,
            serpentResourceLocalOnly);

        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to prepare decryption resources. Error = 0x%x.", hr);
            break;
        }
        didAllocateResources = TRUE;

        cryptoCopyParams.eAesMode = static_cast<MHW_PAVP_CRYPTO_COPY_AES_MODE>(uiAesMode);
        cryptoCopyParams.eCryptoCopyType = CRYPTO_COPY_TYPE_AES_DECRYPT;

        if (cryptoCopyParams.eAesMode == CRYPTO_COPY_AES_MODE_CBC_SKIP)
        {
            cryptoCopyParams.eCryptoCopyType = CRYPTO_COPY_TYPE_AES_DECRYPT_PARTIAL;
            cryptoCopyParams.uiClearBytes = 0;
            uint32_t temp = static_cast<uint32_t>(bufSize);
            cryptoCopyParams.uiEncryptedBytes = temp - (temp % 16);
            PAVP_DEVICE_ASSERTMESSAGE("cryptoCopyParams.uiEncryptedBytes = 0x%x", cryptoCopyParams.uiEncryptedBytes);
        }

        cryptoCopyParams.SrcResource = &aesResource;
        cryptoCopyParams.DstResource = &serpentResource;
        cryptoCopyParams.dwDataSize  = static_cast<DWORD>(bufSize);
        cryptoCopyParams.dwAppId     = m_cpContext.sessionId;

        cryptoCopyParams.dwAesCtr[0] = pDecryptIV[0];
        cryptoCopyParams.dwAesCtr[1] = pDecryptIV[1];
        cryptoCopyParams.dwAesCtr[2] = pDecryptIV[2];
        cryptoCopyParams.dwAesCtr[3] = pDecryptIV[3];

        // Execute the "Crypto Copy" command.
        hr = BuildAndSubmitCommand(CRYPTO_COPY, &cryptoCopyParams);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to submit the 'Crypto Copy' command. Error = 0x%x.", hr);
            break;
        }

        // reset mode to CTR
        cryptoCopyParams.eAesMode = CRYPTO_COPY_AES_MODE_CTR;
        cryptoCopyParams.eCryptoCopyType = CRYPTO_COPY_TYPE_SERPENT_TO_AES_ENCRYPT;
        cryptoCopyParams.SrcResource = &serpentResource;
        cryptoCopyParams.DstResource = &aesResource;
        cryptoCopyParams.dwDataSize  = static_cast<DWORD>(bufSize);
        cryptoCopyParams.dwAppId     = m_cpContext.sessionId;

        cryptoCopyParams.dwAesCtr[0] = pEncryptIV[0];
        cryptoCopyParams.dwAesCtr[1] = pEncryptIV[1];
        cryptoCopyParams.dwAesCtr[2] = pEncryptIV[2];
        cryptoCopyParams.dwAesCtr[3] = pEncryptIV[3];

        // Execute the "Crypto Copy" command.
        hr = BuildAndSubmitCommand(CRYPTO_COPY, &cryptoCopyParams);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to submit the 'Crypto Copy' command. Error = 0x%x.", hr);
            break;
        }
    } while (FALSE);

    if (didAllocateResources)
    {
        hr = m_pOsServices->ReleaseDecryptionResources(
                                &serpentResource,
                                &aesResource,
                                bufSize,
                                pDstBuffer);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to retrieve destination buffer and release decryption resources. Error = 0x%x.", hr);
        }
    }

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::TranscryptLinearBuffer(
    PDWORD          pIV,                // [in]
    PBYTE           pSrcBuffer,         // [in]
    PBYTE           pDstBuffer,         // [out]
    SIZE_T          bufSize,            // [in]
    uint32_t          uiAesMode,          // [in]
    uint32_t          uiCryptoCopyMode)    // [in]
{
    HRESULT                         hr                      = S_OK;
    GMM_GFX_SIZE_T                  resourceSize            = 0L;
    MOS_RESOURCE                    inputResource           = {};
    MOS_RESOURCE                    outputResource          = {};
    BOOL                            didAllocateResources = FALSE;
    PAVP_GPU_HAL_CRYPTO_COPY_PARAMS cryptoCopyParams        = {};
    MHW_PAVP_CRYPTO_COPY_TYPE       eCryptoCopyType = CRYPTO_COPY_TYPE_BYPASS;
    MHW_PAVP_CRYPTO_COPY_AES_MODE   eAesMode = CRYPTO_COPY_AES_MODE_CTR;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        if (pIV == nullptr ||
            pSrcBuffer == nullptr ||
            pDstBuffer == nullptr ||
            bufSize == 0)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Invalid parameters passed in to TranscryptLinearBuffer.");
            hr = E_INVALIDARG;
            break;
        }

        eCryptoCopyType = static_cast<MHW_PAVP_CRYPTO_COPY_TYPE>(uiCryptoCopyMode);
        if (eCryptoCopyType != CRYPTO_COPY_TYPE_AES_DECRYPT &&
            eCryptoCopyType != CRYPTO_COPY_TYPE_AES_DECRYPT_PARTIAL &&
            eCryptoCopyType != CRYPTO_COPY_TYPE_CLEAR_TO_SERPENT_ENCRYPT &&
            eCryptoCopyType != CRYPTO_COPY_TYPE_SERPENT_TO_AES_ENCRYPT)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Wrong code path for crypto copy type %d", eCryptoCopyType);
            hr = E_INVALIDARG;
            break;
        }

        eAesMode = static_cast<MHW_PAVP_CRYPTO_COPY_AES_MODE>(uiAesMode);
        if (eAesMode != CRYPTO_COPY_AES_MODE_CBC_SKIP &&
            eAesMode != CRYPTO_COPY_AES_MODE_CTR)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Unexpected AES mode %d", eAesMode);
            hr = E_INVALIDARG;
            break;
        }

        if (eAesMode == CRYPTO_COPY_AES_MODE_CBC_SKIP &&
            eCryptoCopyType != CRYPTO_COPY_TYPE_AES_DECRYPT_PARTIAL)
        {
            // CBC is supported only for Mode 13 (CRYPTO_COPY_TYPE_AES_DECRYPT_PARTIAL)
            PAVP_DEVICE_ASSERTMESSAGE("Invalid crypto copy type specified for AES-CBC decryption!");
            hr = E_INVALIDARG;
            break;
        }

        if (!IsInitialized())
        {
            PAVP_DEVICE_ASSERTMESSAGE("Init() must be called first.");
            hr = E_UNEXPECTED;
            break;
        }

        if (m_pOsServices == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("m_pOsServices was NULL.");
            hr = E_UNEXPECTED;
            break;
        }

        if (!IsSessionIdValid())
        {
            PAVP_DEVICE_ASSERTMESSAGE("Invalid PAVP session ID.");
            hr = E_FAIL;
            break;
        }

        hr = m_pOsServices->PrepareDecryptionResources(
            pSrcBuffer,
            bufSize,
            &inputResource,
            &outputResource,
            &resourceSize);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to prepare decryption resources. Error = 0x%x.", hr);
            break;
        }
        didAllocateResources = TRUE;

        // Prepare input parameters for CryptoCopy operation
        PAVP_DEVICE_VERBOSEMESSAGE("Executing Crypto Copy operation (type = %d, AES mode = %d) for %d bytes", eCryptoCopyType, eAesMode, bufSize);
        cryptoCopyParams.eAesMode = eAesMode;
        cryptoCopyParams.eCryptoCopyType = eCryptoCopyType;

        cryptoCopyParams.uiEncryptedBytes = static_cast<uint32_t>(bufSize);
        cryptoCopyParams.uiClearBytes = 0;

        cryptoCopyParams.SrcResource = &inputResource;
        cryptoCopyParams.DstResource = &outputResource;
        cryptoCopyParams.dwDataSize  = static_cast<DWORD>(bufSize);
        cryptoCopyParams.dwAppId     = m_cpContext.sessionId;

        cryptoCopyParams.dwAesCtr[0] = pIV[0];
        cryptoCopyParams.dwAesCtr[1] = pIV[1];
        cryptoCopyParams.dwAesCtr[2] = pIV[2];
        cryptoCopyParams.dwAesCtr[3] = pIV[3];

        // Execute the "Crypto Copy" command.
        hr = BuildAndSubmitCommand(CRYPTO_COPY, &cryptoCopyParams);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to submit the 'Crypto Copy' command. Error = 0x%x.", hr);
            break;
        }

    } while (FALSE);

    if (didAllocateResources)
    {
        hr = m_pOsServices->ReleaseDecryptionResources(
            &inputResource,
            &outputResource,
            bufSize,
            pDstBuffer);

        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to retrieve destination buffer and release decryption resources. Error = 0x%x.", hr);
        }
    }

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::DoDecryptionBltLinearBuffer(PAVP_DECRYPT_BLT_PARAMS &sDecryptBlt) CONST
{
    HRESULT                          hr                       = S_OK;
    PAVP_GPU_HAL_CRYPTO_COPY_PARAMS  sCryptoCopyParams;
    DWORD                            *pEncryptedDecryptionKey = nullptr;
    PAVP_GPU_HAL_KEY_EXCHANGE_PARAMS sKeyExchangeParams       = {0};

    PAVP_DEVICE_FUNCTION_ENTER;
    MOS_ZeroMemory(&sCryptoCopyParams, sizeof(sCryptoCopyParams));

    do
    {
        if (!IsInitialized())
        {
            PAVP_DEVICE_ASSERTMESSAGE("Init() must be called first.");
            hr = E_UNEXPECTED;
            break;
        }

        if (!sDecryptBlt.SrcResource ||
            !sDecryptBlt.DstResource ||
            sDecryptBlt.dwSrcResourceSize == 0)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Invalid argument.");
            hr = E_INVALIDARG;
            break;
        }

        MOS_TraceEventExt(EVENT_BLT_DEC, EVENT_TYPE_START, &sDecryptBlt.dwSrcResourceSize, sizeof(uint32_t), nullptr, 0);

        sCryptoCopyParams.SrcResource = sDecryptBlt.SrcResource;
        sCryptoCopyParams.DstResource = sDecryptBlt.DstResource;

        if (sDecryptBlt.bPartialDecryption)
        {
            sCryptoCopyParams.eCryptoCopyType = CRYPTO_COPY_TYPE_AES_DECRYPT_PARTIAL;    // Operation type.
            sCryptoCopyParams.uiEncryptedBytes = sDecryptBlt.uiEncryptedBytes;
            sCryptoCopyParams.uiClearBytes = sDecryptBlt.uiClearBytes;
            sCryptoCopyParams.eAesMode = static_cast<MHW_PAVP_CRYPTO_COPY_AES_MODE>(sDecryptBlt.dwAesMode);
        }
        else
        {
            sCryptoCopyParams.eCryptoCopyType = CRYPTO_COPY_TYPE_AES_DECRYPT;    // Operation type.
        }

        sCryptoCopyParams.dwDataSize = sDecryptBlt.dwSrcResourceSize;   // The size in bytes of the amount of data to be decrypted.
        sCryptoCopyParams.dwAppId    = m_cpContext.sessionId;  // App ID of the current session.

        MOS_SecureMemcpy(
            sCryptoCopyParams.dwAesCtr,
            sizeof(sCryptoCopyParams.dwAesCtr),
            sDecryptBlt.pIV,
            sizeof(sCryptoCopyParams.dwAesCtr));

        if (sDecryptBlt.pContentKey &&
            sDecryptBlt.uiContentKeySize == sizeof(sKeyExchangeParams.EncryptedDecryptionKey))
        {
            PAVP_DEVICE_NORMALMESSAGE("Set decryption key (content key) for Widevine audio decryption.");

            pEncryptedDecryptionKey                      = static_cast<PDWORD>(sDecryptBlt.pContentKey);
            sKeyExchangeParams.KeyExchangeType           = PAVP_KEY_EXCHANGE_TYPE_DECRYPT;
            sKeyExchangeParams.AppId                     = m_cpContext.sessionId;
            sKeyExchangeParams.EncryptedDecryptionKey[0] = pEncryptedDecryptionKey[0];
            sKeyExchangeParams.EncryptedDecryptionKey[1] = pEncryptedDecryptionKey[1];
            sKeyExchangeParams.EncryptedDecryptionKey[2] = pEncryptedDecryptionKey[2];
            sKeyExchangeParams.EncryptedDecryptionKey[3] = pEncryptedDecryptionKey[3];

            hr = BuildAndSubmitCommand(KEY_EXCHANGE, &sKeyExchangeParams);
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Failed to send key exchange command to hardware. hr = 0x%x", hr);
                break;
            }
        }

#if (_DEBUG || _RELEASE_INTERNAL)
        // Support for the "No Exchange" function in the PAVP app.
        // It is for internal use only for security reasons.
        if (((uintptr_t)sDecryptBlt.pContentKey == DECRYPTIONBLT_DEBUG_BIT_CRYPTOCOPY_BYPASS) &&
            m_eDrmType != PAVP_DRM_TYPE_WIDEVINE)
        {
            sCryptoCopyParams.eCryptoCopyType = CRYPTO_COPY_TYPE_BYPASS;
        }
#endif

        // Execute the "Crypto Copy" command.
        hr = BuildAndSubmitCommand(CRYPTO_COPY, &sCryptoCopyParams);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to submit the 'Crypto Copy' command. Error = 0x%x.", hr);
            break;
        }
    } while (FALSE);

    if (FAILED(hr) && hr != E_INVALIDARG)
    {
        // for telemetry events
        uint32_t uiDrmType = m_eDrmType;
        uint32_t uiCryptoType = m_eCryptoType;
        MosUtilities::MosGfxInfoRTErr(
            1,
            GFXINFO_COMPID_UMD_MEDIA_CP,
            GFXINFO_ERRID_CP_DX11_DecryptionBlt,
            hr,
            2,
            "data2", GFXINFO_PTYPE_UINT32, &uiDrmType,
            "data3", GFXINFO_PTYPE_UINT32, &uiCryptoType);
    }

    MOS_TraceEventExt(EVENT_BLT_DEC, EVENT_TYPE_END, &hr, sizeof(uint32_t), nullptr, 0);
    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::DoDecryptionBlt(PAVP_DECRYPT_BLT_PARAMS &sDecryptBlt) CONST
{
    HRESULT                 hr                   = S_OK;
    PAVP_DECRYPT_BLT_PARAMS sDecryptBltLinearBuf = {0};
    PMOS_RESOURCE           LinearBufSrc         = nullptr;
    PMOS_RESOURCE           LinearBufDst         = nullptr;
    PMOS_RESOURCE           ShapeResource;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        if (!IsInitialized() ||
            !m_pOsServices ||
            !m_pOsServices->m_pOsInterface ||
            !m_pOsServices->m_pOsInterface->pfnSyncOnResource)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Init() must be called first.");
            hr = E_UNEXPECTED;
            break;
        }

        // Verify input parameters
        if (!sDecryptBlt.SrcResource ||
            !sDecryptBlt.DstResource ||
            sDecryptBlt.dwSrcResourceSize == 0)      // Cannot be negative
        {
            PAVP_DEVICE_ASSERTMESSAGE("Invalid argument.");
            hr = E_INVALIDARG;
            break;
        }

        if (m_eDrmType != PAVP_DRM_TYPE_WIDEVINE &&
#if (!_DEBUG && !_RELEASE_INTERNAL) // For security reasons, this feature can be used for non-production builds only.
            sDecryptBlt.pContentKey
#else
            (sDecryptBlt.pContentKey && (uintptr_t)(sDecryptBlt.pContentKey) != DECRYPTIONBLT_DEBUG_BIT_CRYPTOCOPY_BYPASS)
#endif
           )
        {
            PAVP_DEVICE_ASSERTMESSAGE("Invalid argument.");
            hr = E_INVALIDARG;
            break;
        }

        m_pOsServices->UpdateResourceUsageType(sDecryptBlt.SrcResource, MOS_HW_RESOURCE_USAGE_CP_EXTERNAL_READ);

        // Notes:
        // This function assumes only subresource 0 is used in both resources.
        // It is the DDI's responsibility to verify it.
        // Using different subresource is not possible from app side because no such interface is exposed to apps
        // (see IDirect3DCryptoSession9::DecryptionBlt documentation).
        //
        // SrcResource and DstResource should have the same size (enforced by DX runtime)
        // and are allocated using IDirect3DDevice9Ex::CreateOffscreenPlainSurfaceEx.
        // SrcResource should be linear and its pixels should be in ARGB format (i.e. 32bit).
        // DstResource will come out Y tiled.
        //
        // Validations that should be done in the DDI class:
        //  * Source and destination resources are valid ones.
        //  * Source resource is in system memory.
        //  * Destination resource is in video memory.
        //  * SrcSubResourceIndex and DstSubResourceIndex are 0.
        //  * DecryptionBlt.SrcResourceSize is positive.
        //  * DecryptionBlt.pIV is in little-endian form.

        // We clone the source and the destination resources into linear resources and save them as
        // CryptoCopyParams.srcResource and CryptoCopyParams.dstResource.
        // It is done because a "Crypto Copy" command must receive linear resources.

        // Clone the source resource into a linear resource in the video memory.

        // App should honor source pitch to placing the input image.
        // For DX9, it is required that src resource is allocated with same size / same pitch of Dst resource.
        // While in DX11 app can only get Src Pitch but not Dst Pitch
        // So we allocate input / output of crypto copy command with source shape
        //
        // For DX9, src surface created as system memory won't have GMM resource info,
        // in this case DstResource is used as shape resource
#if (LHDM && !_VULKAN)
        ShapeResource = DXVAUMD_RESOURCE_GMM(sDecryptBlt.SrcResource->pD3dResource, 0) ?
                            sDecryptBlt.SrcResource :
                            sDecryptBlt.DstResource;
#else
        ShapeResource = sDecryptBlt.DstResource;
#endif

        hr = m_pOsServices->CloneToLinearVideoResource(
                                ShapeResource,                  // The shape resource.
                                &LinearBufSrc,                  // The newly created resource.
                                sDecryptBlt.SrcResource,        // The target data resource.
                                sDecryptBlt.dwSrcResourceSize); // The size of the data resource.
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to clone the source resource into a linear resource in the video memory. Error = 0x%x.", hr);
            break;
        }

        hr = m_pOsServices->CloneToLinearVideoResource(
                                ShapeResource,  // The shape resource.
                                &LinearBufDst); // The newly created resource.
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to create a dstination linear resource in the video memory. Error = 0x%x.", hr);
            break;
        }

        sDecryptBltLinearBuf             = sDecryptBlt;
        sDecryptBltLinearBuf.SrcResource = LinearBufSrc;
        sDecryptBltLinearBuf.DstResource = LinearBufDst;

        // Notify MOS that VDBOX is going to write to crypto copy destination
        m_pOsServices->m_pOsInterface->pfnSyncOnResource(
            m_pOsServices->m_pOsInterface,
            LinearBufDst,
            MOS_GPU_CONTEXT_VIDEO,
            TRUE);

        hr = DoDecryptionBltLinearBuffer(sDecryptBltLinearBuf);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to do decryptionBlt for linear buffer.");
            hr = E_UNEXPECTED;
            break;
        }

        if (m_PavpSessionMode == PAVP_SESSION_MODE_STOUT)
        {
            LinearBufDst->pGmmResInfo->GetSetCpSurfTag(true, sDecryptBlt.DstResource->pGmmResInfo->GetSetCpSurfTag(false, 0));
            // Linear to tile convert through VEBOX
            hr = m_pOsServices->m_pOsInterface->pfnDoubleBufferCopyResource(
                m_pOsServices->m_pOsInterface,
                LinearBufDst,
                sDecryptBlt.DstResource,  //save tiled output in here
                false);
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Failed to tile convert. Error = 0x%x.", hr);
                break;
            }
        }
        else
        {
            // Blt from the "Crypto Copy" output to the destination specified by the application.
            hr = m_pOsServices->BltSurface(
                LinearBufDst,
                sDecryptBlt.DstResource,
                IsMemoryEncryptionEnabled());
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Failed to perform blt. Error = 0x%x.", hr);
                break;
            }
        }
    } while (FALSE);

    if (m_pOsServices)
    {
        if (LinearBufSrc != nullptr)
        {
            m_pOsServices->FreeResource(LinearBufSrc);
        }
        if (LinearBufDst != nullptr)
        {
            m_pOsServices->FreeResource(LinearBufDst);
        }
    }

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::GetFreshnessValue(
    PAVP_GET_FRESHNESS_PARAMS&  sGetFreshnessParams, 
    PAVP_FRESHNESS_REQUEST_TYPE eType) CONST
{
    HRESULT                             hr                  = S_OK;
    PAVP_GPU_HAL_QUERY_STATUS_PARAMS    sQueryStatusParams  = {0, 0, PAVP_QUERY_STATUS_FRESHNESS, {0}, {0}};

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        PAVP_DEVICE_NORMALMESSAGE("The application is requesting a random value for key refresh.");

        // Validate that the device is initilaized.
        if (!IsInitialized() ||
            !m_pGpuHal)
        {
            PAVP_DEVICE_ASSERTMESSAGE("The device is not properly initialized.");
            hr = E_UNEXPECTED;
            break;
        }

        // Query the KMD for a new freshness value.
        sQueryStatusParams.QueryOperation       = PAVP_QUERY_STATUS_FRESHNESS;
        sQueryStatusParams.Value.KeyRefreshType = ((eType == PAVP_FRESHNESS_REQUEST_DECRYPT) ? 
                                                        KEY_REFRESH_TYPE_DECRYPTED :
                                                        KEY_REFRESH_TYPE_ENCRYPTED);
        sQueryStatusParams.AllocationIndex      = 1; // Arbitrary index.
        sQueryStatusParams.AppId                = m_cpContext.sessionId;

        hr = BuildAndSubmitCommand(QUERY_STATUS, &sQueryStatusParams);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to send query status command. Error = 0x%x.", hr);
            break;
        }

        // Copy the value received from KMD to the return value.
        MOS_SecureMemcpy(
            &sGetFreshnessParams,
            sizeof(PAVP_GET_FRESHNESS_PARAMS),
            sQueryStatusParams.OutData,
            PAVP_QUERY_STATUS_KF_OUT_DATA_SIZE);

    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::SetFreshnessValue(PAVP_FRESHNESS_REQUEST_TYPE eType) CONST
{
    HRESULT                             hr                  = S_OK;
    PAVP_GPU_HAL_KEY_EXCHANGE_PARAMS    sKeyExchangeParams  = { 0, PAVP_KEY_EXCHANGE_TYPE_DECRYPT_FRESHNESS, { 0 }, { { 0 } } };

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        PAVP_DEVICE_NORMALMESSAGE("The application is requesting to use the refreshed key.");

        if (!IsInitialized())
        {
            PAVP_DEVICE_ASSERTMESSAGE("The device is not properly initialized.");
            hr = E_UNEXPECTED;
            break;
        }

        if (!IsSessionIdValid())
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to SetFreshnessValue() due to invalid session id.");
            hr = E_UNEXPECTED;
            break;
        }

        // Send a key exchange command to the KMD.
        // The KMD ignores Key 1 and Key2 parameters when setting a freshness value.
        sKeyExchangeParams.KeyExchangeType  = ((eType == PAVP_FRESHNESS_REQUEST_DECRYPT) ?
                                                PAVP_KEY_EXCHANGE_TYPE_DECRYPT_FRESHNESS :
                                                PAVP_KEY_EXCHANGE_TYPE_ENCRYPT_FRESHNESS);
        sKeyExchangeParams.AppId            = m_cpContext.sessionId;
        hr = BuildAndSubmitCommand(KEY_EXCHANGE, &sKeyExchangeParams);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to send the command. Error: 0x%x.", hr);
            break;
        }

    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

bool CPavpDevice::NeedsASMFTranslation()
{
    // if the session is not ready, then SetStreamKey does not require translation from (Sn)Sn-1 to (Sn)Kb since the GPU 
    // is either not setup with a key yet or it already has Kb set.
    // if m_bSkipAsmf is set to TRUE, it implies app is using Get key blob API and therefore key translation is not needed
    bool bNeedsAsmfTranslation = false;
    bool bDrmSkipAsmfTranslation;
    BOOL bSessionIsReady = FALSE;
    HRESULT hr = S_OK;

    PAVP_DEVICE_FUNCTION_ENTER;

    // Modern DRMs like PlayReady, Widevine and Miracast don't require translation, but others do.
    bDrmSkipAsmfTranslation = ((m_eDrmType == PAVP_DRM_TYPE_PLAYREADY) ||
                               (m_eDrmType == PAVP_DRM_TYPE_WIDEVINE) ||
                               (m_eDrmType == PAVP_DRM_TYPE_MIRACAST_RX) ||
                               (m_eDrmType == PAVP_DRM_TYPE_SECURE_COMPUTE) ||
                               (m_bSkipAsmf == TRUE));

    if (bDrmSkipAsmfTranslation)
    {
        bNeedsAsmfTranslation = false;
    }
    else
    {
        hr = IsSessionReady(bSessionIsReady);
        bSessionIsReady = (SUCCEEDED(hr) && bSessionIsReady);
        bNeedsAsmfTranslation = (IsInitialized() && bSessionIsReady);
    }

    PAVP_DEVICE_VERBOSEMESSAGE("bNeedsAsmfTranslation=[%d] m_eDrmType=[%d] bSessionIsReady=[%d]", bNeedsAsmfTranslation, m_eDrmType, bSessionIsReady);
    PAVP_DEVICE_FUNCTION_EXIT(hr);

    return bNeedsAsmfTranslation;
}

HRESULT CPavpDevice::SetStreamKey(PAVP_SET_STREAM_KEY_PARAMS& SetKeyParams, bool KeyTranslationRequired /* = true */)
{
    HRESULT                             hr                          = S_OK;
    PAVP_GPU_HAL_KEY_EXCHANGE_PARAMS    SetStreamKeyParams          = { 0, PAVP_KEY_EXCHANGE_TYPE_DECRYPT, { 0 }, { { 0 } } };
    BOOL                                bSetSessionReady            = FALSE;
    BOOL                                bSetPlaneEnable             = FALSE;
    PAVP_SESSION_STATUS                 CurrentPavpSessionStatus    = PAVP_SESSION_UNSUPPORTED;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        PAVP_DEVICE_NORMALMESSAGE("Attempting to set a new key.");

        if (m_pOsServices == nullptr ||
            m_pGpuHal == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Unexpected error: HALs have not been initialized.");
            hr = E_UNEXPECTED;
            break;
        }

        // Prevent setting an encryption key not during initialization.
        if ((SetKeyParams.StreamType == PAVP_SET_KEY_BOTH ||
             SetKeyParams.StreamType == PAVP_SET_KEY_ENCRYPT) &&
             m_PavpSessionType == PAVP_SESSION_TYPE_DECODE &&
             m_bHwSessionIsAlive)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Cannot set an encryption key after we create hw session.");
            hr = E_INVALIDARG;
            break;
        }

        // check if in ASMF mode. if so, get the keys translated from (Sn)Sn-1 to (Sn)Kb...
        if (KeyTranslationRequired && NeedsASMFTranslation())
        {
            hr = TranslateStreamKeyForASMFMode(SetKeyParams);
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Failed to translate Stream Keys for ASMF mode. Error = 0x%x.", hr);
                break;
            }
        }
        
        switch (SetKeyParams.StreamType)
        {
            case PAVP_SET_KEY_DECRYPT: 
                PAVP_DEVICE_NORMALMESSAGE("Updating the 'Decrypt' key.");
                SetStreamKeyParams.KeyExchangeType = PAVP_KEY_EXCHANGE_TYPE_DECRYPT;
                bSetSessionReady = TRUE;
                break;

            case PAVP_SET_KEY_ENCRYPT:
                PAVP_DEVICE_NORMALMESSAGE("Updating the 'Encrypt' key.");
                SetStreamKeyParams.KeyExchangeType = PAVP_KEY_EXCHANGE_TYPE_ENCRYPT;
                break;

            case PAVP_SET_KEY_BOTH:
                PAVP_DEVICE_NORMALMESSAGE("Updating both the 'Decrypt' key and the 'Encrypt' key.");
                SetStreamKeyParams.KeyExchangeType = PAVP_KEY_EXCHANGE_TYPE_DECRYPT_ENCRYPT;
                bSetSessionReady = TRUE;
                break;

            case PAVP_SET_KEY_WYSIWYS: 
                // SetStreamKey() for WYSIWYS should be called only in heavy mode.
                if (IsMemoryEncryptionEnabled())
                {
                    PAVP_DEVICE_NORMALMESSAGE("A WYSIWYS application is updating the 'decrypt' key.");
                    SetStreamKeyParams.KeyExchangeType = PAVP_KEY_EXCHANGE_TYPE_DECRYPT;
                    bSetSessionReady = TRUE;
                    bSetPlaneEnable  = TRUE;
                }
#if (_DEBUG || _RELEASE_INTERNAL)
                else if (m_bForceLiteModeForDebug)
                {
                    PAVP_DEVICE_ASSERTMESSAGE("A WYSIWYS application is updating the 'decrypt' key, with lite mode for debug.");
                    SetStreamKeyParams.KeyExchangeType = PAVP_KEY_EXCHANGE_TYPE_DECRYPT;
                    bSetSessionReady = TRUE;
                }
#endif
                else 
                {
                    PAVP_DEVICE_ASSERTMESSAGE("A WYSIWYS application should update the 'decrypt' key only in heavy mode.");
                    hr = E_INVALIDARG;
                }
                break;

            case PAVP_SET_KEY_ROTATION:
                PAVP_DEVICE_NORMALMESSAGE("Updating the 'Decrypt keys' for rotation. Please note rotation is only valid for decrypt keys.");
                SetStreamKeyParams.KeyExchangeType = PAVP_KEY_EXCHANGE_TYPE_KEY_ROTATION;

                MOS_SecureMemcpy(SetStreamKeyParams.EncryptedDecryptionRotationKey, sizeof(SetStreamKeyParams.EncryptedDecryptionRotationKey), 
                                 SetKeyParams.EncryptedDecryptRotationKey, PAVP_KEY_LENGTH);
                break;

            default: 
                PAVP_DEVICE_ASSERTMESSAGE("Unrecognized key type: %d.", SetKeyParams.StreamType);
                hr = E_INVALIDARG;
        }

        if (FAILED(hr))
        {
            break;
        }

        MOS_SecureMemcpy(SetStreamKeyParams.EncryptedDecryptionKey, sizeof(SetStreamKeyParams.EncryptedDecryptionKey), SetKeyParams.EncryptedDecryptKey, PAVP_KEY_LENGTH);
        if (SetKeyParams.StreamType != PAVP_SET_KEY_ROTATION)
        {
            MOS_SecureMemcpy(SetStreamKeyParams.EncryptedEncryptionKey, sizeof(SetStreamKeyParams.EncryptedEncryptionKey), SetKeyParams.EncryptedEncryptKey, PAVP_KEY_LENGTH);
        }
        SetStreamKeyParams.AppId = m_cpContext.sessionId;
        hr = BuildAndSubmitCommand(KEY_EXCHANGE, &SetStreamKeyParams);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to send key exchange command. Error = 0x%x.", hr);
            break;
        }

        hr = m_pOsServices->QuerySessionStatus(&CurrentPavpSessionStatus);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to query whether the session is alive. Error = 0x%x.", hr);
        }

        // Move the session status to 'Ready' if not done already.
        if (bSetSessionReady && CurrentPavpSessionStatus != PAVP_SESSION_READY)
        {
            hr = m_pOsServices->GetSetPavpSessionStatus(
                m_PavpSessionType,
                m_PavpSessionMode,
                PAVP_SESSION_READY,
                &CurrentPavpSessionStatus);
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Get/Set Session Status call failed. Error = 0x%x.", hr);
                if (hr == E_ABORT)
                {
                    // PAVP teardown happened, return specific error to notify app recreate. 
                    CleanupHwSession();
                    hr = DRM_E_TEE_OUTPUT_PROTECTION_REQUIREMENTS_NOT_MET;
                }
                break;
            }
            PAVP_DEVICE_NORMALMESSAGE("Session status set to ready");
        }
    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::TranslateStreamKeyForASMFMode(PAVP_SET_STREAM_KEY_PARAMS& SetKeyParams)
{
    //incoming keys are wrapped with the previous key [(Sn)Sn-1]. We need to call the ME FW API to get these keys
    //translated into (Sn)Kb before setting it into HW... We can reuse the SetStreamKey function once the keys have
    //been translated.
    HRESULT                     hr = S_OK;
    MOS_STATUS                  ReturnStatus = MOS_STATUS_SUCCESS;
    PAVP_SET_STREAM_KEY_PARAMS  TranslatedKeyParams;
    StreamKey                   pTranslatedKey;
    StreamKey                   pKeyToTranslate;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        if (m_pRootTrustHal == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Unexpected error: HALs have not been initialized.");
            hr = E_UNEXPECTED;
            break;
        }

        MOS_ZeroMemory(pTranslatedKey, sizeof(pTranslatedKey));
        MOS_ZeroMemory(pKeyToTranslate, sizeof(pKeyToTranslate));
        MOS_ZeroMemory(&TranslatedKeyParams, sizeof(TranslatedKeyParams));

        //operate on a local copy of the incoming key structure...
        TranslatedKeyParams.StreamType = SetKeyParams.StreamType;

        switch (SetKeyParams.StreamType)
        {
        case PAVP_SET_KEY_DECRYPT:
        case PAVP_SET_KEY_WYSIWYS:
            PAVP_DEVICE_NORMALMESSAGE("Translating the 'Decrypt' key.");

            //copy the incoming key to a temp variable...
            ReturnStatus = MOS_SecureMemcpy(pKeyToTranslate, sizeof(pKeyToTranslate), SetKeyParams.EncryptedDecryptKey, sizeof(SetKeyParams.EncryptedDecryptKey));
            if (MOS_STATUS_SUCCESS != ReturnStatus)
            {
                PAVP_DEVICE_ASSERTMESSAGE("MOS Secure Copy of EncryptedDecryptKey returned failure: error = %d.", ReturnStatus);
                hr = E_FAIL;
                break;
            }

            //send in the Key in command...
            hr = m_pRootTrustHal->SendDaisyChainCmd(pKeyToTranslate, true, pTranslatedKey);
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Daisy chain command returned failure: hr = 0x%x.", hr);
                break;
            }

            //get the translated key copied over...
            ReturnStatus = MOS_SecureMemcpy(TranslatedKeyParams.EncryptedDecryptKey, sizeof(TranslatedKeyParams.EncryptedDecryptKey), pTranslatedKey, PAVP_EPID_STREAM_KEY_LEN);
            if (MOS_STATUS_SUCCESS != ReturnStatus)
            {
                PAVP_DEVICE_ASSERTMESSAGE("MOS Secure Copy of translated EncryptedDecryptKey returned failure: error = %d.", ReturnStatus);
                hr = E_FAIL;
                break;
            }
            break;

        case PAVP_SET_KEY_ENCRYPT:
            PAVP_DEVICE_NORMALMESSAGE("Translating the 'Encrypt' key.");

            //copy the incoming key to a temp variable...
            ReturnStatus = MOS_SecureMemcpy(pKeyToTranslate, sizeof(pKeyToTranslate), SetKeyParams.EncryptedEncryptKey, sizeof(SetKeyParams.EncryptedEncryptKey));
            if (MOS_STATUS_SUCCESS != ReturnStatus)
            {
                PAVP_DEVICE_ASSERTMESSAGE("MOS Secure Copy of EncryptedEncryptKey returned failure: error = %d.", ReturnStatus);
                hr = E_FAIL;
                break;
            }

            //send in the Key in command...
            hr = m_pRootTrustHal->SendDaisyChainCmd(pKeyToTranslate, false, pTranslatedKey);
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Daisy chain command returned failure: hr = 0x%x.", hr);
                break;
            }

            //get the translated key copied over...
            ReturnStatus = MOS_SecureMemcpy(TranslatedKeyParams.EncryptedEncryptKey, sizeof(TranslatedKeyParams.EncryptedEncryptKey), pTranslatedKey, PAVP_EPID_STREAM_KEY_LEN);
            if (MOS_STATUS_SUCCESS != ReturnStatus)
            {
                PAVP_DEVICE_ASSERTMESSAGE("MOS Secure Copy of translated EncryptedEncryptKey returned failure: error = %d.", ReturnStatus);
                hr = E_FAIL;
                break;
            }
            break;

        case PAVP_SET_KEY_BOTH:
            //first translate the decrypt key...
            PAVP_DEVICE_NORMALMESSAGE("Translating the 'Decrypt' key.");

            //copy the incoming key to a temp variable...
            ReturnStatus = MOS_SecureMemcpy(pKeyToTranslate, sizeof(pKeyToTranslate), SetKeyParams.EncryptedDecryptKey, sizeof(SetKeyParams.EncryptedDecryptKey));
            if (MOS_STATUS_SUCCESS != ReturnStatus)
            {
                PAVP_DEVICE_ASSERTMESSAGE("MOS Secure Copy of EncryptedDecryptKey returned failure: error = %d.", ReturnStatus);
                hr = E_FAIL;
                break;
            }

            //send in the Key in command...
            hr = m_pRootTrustHal->SendDaisyChainCmd(pKeyToTranslate, true, pTranslatedKey);
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Daisy chain command returned failure: hr = 0x%x.", hr);
                break;
            }

            //get the translated key copied over...
            ReturnStatus = MOS_SecureMemcpy(TranslatedKeyParams.EncryptedDecryptKey, sizeof(TranslatedKeyParams.EncryptedDecryptKey), pTranslatedKey, PAVP_EPID_STREAM_KEY_LEN);
            if (MOS_STATUS_SUCCESS != ReturnStatus)
            {
                PAVP_DEVICE_ASSERTMESSAGE("MOS Secure Copy of translated EncryptedDecryptKey returned failure: error = %d.", ReturnStatus);
                hr = E_FAIL;
                break;
            }

            //next translate the encrypt key...
            PAVP_DEVICE_NORMALMESSAGE("Translating the 'Encrypt' key.");

            //copy the incoming key to a temp variable...
            ReturnStatus = MOS_SecureMemcpy(pKeyToTranslate, sizeof(pKeyToTranslate), SetKeyParams.EncryptedEncryptKey, sizeof(SetKeyParams.EncryptedEncryptKey));
            if (MOS_STATUS_SUCCESS != ReturnStatus)
            {
                PAVP_DEVICE_ASSERTMESSAGE("MOS Secure Copy of EncryptedEncryptKey returned failure: error = %d.", ReturnStatus);
                hr = E_FAIL;
                break;
            }

            //send in the Key in command...
            hr = m_pRootTrustHal->SendDaisyChainCmd(pKeyToTranslate, false, pTranslatedKey);
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Daisy chain command returned failure: hr = 0x%x.", hr);
                break;
            }

            //get the translated key copied over...
            ReturnStatus = MOS_SecureMemcpy(TranslatedKeyParams.EncryptedEncryptKey, sizeof(TranslatedKeyParams.EncryptedEncryptKey), pTranslatedKey, PAVP_EPID_STREAM_KEY_LEN);
            if (MOS_STATUS_SUCCESS != ReturnStatus)
            {
                PAVP_DEVICE_ASSERTMESSAGE("MOS Secure Copy of translated EncryptedEncryptKey returned failure: error = %d.", ReturnStatus);
                hr = E_FAIL;
                break;
            }
            break;

        default:
            //no support for Key rotation...
            PAVP_DEVICE_ASSERTMESSAGE("Unrecognized key type: %d.", SetKeyParams.StreamType);
            hr = E_INVALIDARG;
            break;
        }

        if (FAILED(hr))
        {
            break;
        }

        //update the incoming key structure with the local translated copy...
        ReturnStatus = MOS_SecureMemcpy(SetKeyParams.EncryptedEncryptKey, sizeof(SetKeyParams.EncryptedEncryptKey), TranslatedKeyParams.EncryptedEncryptKey, sizeof(TranslatedKeyParams.EncryptedEncryptKey));
        if (MOS_STATUS_SUCCESS != ReturnStatus)
        {
            PAVP_DEVICE_ASSERTMESSAGE("MOS Secure Copy while returning translated EncryptedEncryptKey returned failure: error = %d.", ReturnStatus);
            hr = E_FAIL;
            break;
        }
        
        ReturnStatus = MOS_SecureMemcpy(SetKeyParams.EncryptedDecryptKey, sizeof(SetKeyParams.EncryptedDecryptKey), TranslatedKeyParams.EncryptedDecryptKey, sizeof(TranslatedKeyParams.EncryptedDecryptKey));
        if (MOS_STATUS_SUCCESS != ReturnStatus)
        {
            PAVP_DEVICE_ASSERTMESSAGE("MOS Secure Copy while returning translated EncryptedDecryptKey returned failure: error = %d.", ReturnStatus);
            hr = E_FAIL;
            break;
        }
    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

static bool didRetrieveWrappedTitleKeys(PAVPCmdHdr const * const pHeader)
{
    // We consider a key blob delivered and a session "READY" iff 
    // 1) A ME command "wv9_wrapped_title_keys" is marked as processed  
    //    having been ORd with "MDRM_OUT_MSG_FLAG"
    //    Note: the debug variant "wv9_dbg_pavp_lite_key" also triggers 
    //    this behavior, but only in DEBUG or RELEASE_INTERNAL builds.
    //    This firmware function is only available in pre-production.
    // 2) The response from ME indicates that command was successful
    //    and that the payload (key blob) is in tow
    // To inspect the contents of a ME message in flight is frowned upon,
    // but this is an odd corner case since ME is no longer marking
    // private data for this call.
    if (pHeader == nullptr || pHeader->PavpCmdStatus != MDRM_SUCCESS)
    {
        return false;
    }

    if ((pHeader->CommandId == (wv9_wrapped_title_keys | MDRM_OUT_MSG_FLAG)))
    {
        return true;
    }

#if (_DEBUG || _RELEASE_INTERNAL)
    if ((pHeader->CommandId == (wv9_dbg_pavp_lite_key | MDRM_OUT_MSG_FLAG)))
    {
        return true;
    }
#endif

    // Otherwise, the ME command is not one of the two special key-retrieval 
    // commands and we should not move the session to READY.
    return false;
}

 HRESULT CPavpDevice::StoreKeyBlob(void *pStoreKeyBlobParams)
{
    HRESULT hr = S_OK;

    PAVP_DEVICE_FUNCTION_ENTER;
    do
    {
        // Verify that the device has been initialized.
        if (!IsInitialized())
        {
            PAVP_DEVICE_ASSERTMESSAGE("Init() must be called before sending data to ME.");
            hr = E_UNEXPECTED;
            break;
        }
        if (m_pRootTrustHal == nullptr ||
            m_pOsServices == nullptr ||
            m_pGpuHal == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Unexpected error: HALs have not been initialized.");
            hr = E_UNEXPECTED;
            break;
        }

        if (pStoreKeyBlobParams == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Invalid parameter.");
            hr = E_INVALIDARG;
            break;
        }

        // Stout Transcode only will store key blob to prepare for creating hw session
        if ((m_PavpSessionType == PAVP_SESSION_TYPE_TRANSCODE) && (m_PavpSessionMode == PAVP_SESSION_MODE_STOUT))
        {
            m_pRootTrustHal->SaveKeyBlobForPAVPINIT43(pStoreKeyBlobParams);
        }

    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::AccessME(
    PBYTE   pInput,
    ULONG   ulInSize,
    PBYTE   pOutput,
    ULONG   ulOutSize,
    bool    bIsWidiMsg,
    uint8_t vTag)
{
    HRESULT                     hr                          = S_OK;
    PAVPCmdHdr                  *pHeaderIn                  = nullptr;
    PAVPCmdHdr                  *pHeader                    = nullptr;
    UINT8                       IgdData[FIRMWARE_MAX_IGD_DATA_SIZE]; // Local buffer for IGD-ME private data.
    uint32_t                      IgdDataLen                  = sizeof(IgdData);
    PAVP_SET_STREAM_KEY_PARAMS  SetKeyParams;
    PAVP_SESSION_STATUS         CurrentPavpSessionStatus    = PAVP_SESSION_UNSUPPORTED;

    PAVP_DEVICE_FUNCTION_ENTER;
    MOS_UNUSED(vTag);

    do
    {
        MOS_ZeroMemory(IgdData, IgdDataLen);

        // Verify that the device has been initialized.
        if (!IsInitialized())
        {
            PAVP_DEVICE_ASSERTMESSAGE("Init() must be called before sending data to ME.");
            hr = E_UNEXPECTED;
            break;
        }

        // Check args, but allow a NULL output buffer (let the ME decide if it's needed).
        if (pInput    == nullptr                            ||
            pOutput   == nullptr                            ||
            ulInSize  == 0                                  ||
            ulOutSize == 0)
        {
            hr = E_INVALIDARG;
            PAVP_DEVICE_ASSERTMESSAGE("Invalid parameter.");
            break;
        }

        if (m_pRootTrustHal == nullptr ||
            m_pOsServices == nullptr ||
            m_pGpuHal == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Unexpected error: HALs have not been initialized.");
            hr = E_UNEXPECTED;
            break;
        }

        // For legacy transcode apps, the key is actually injected when the app requests stream keys. 
        // We program the app ID register here so it's locked for the shortest amount of time possible. 
        // This keeps workloads in flight on the video engine from stalling. 
        if (m_PavpSessionType == PAVP_SESSION_TYPE_TRANSCODE)
        {
            pHeader = reinterpret_cast<PAVPCmdHdr*>(pInput);
            if (pHeader->CommandId == FIRMWARE_CMD_GET_STREAM_KEY_EX)
            {
#ifdef ANDROID
                // Android has been using fw version 3.2 to support modular
                // drm, but Windows has historically fallen back to fw
                // version 3.0.
                // Now that Windows Threshold uses PlayReady3.0, it needs to
                // use fw version 3.2 for multiple sessions per heci channel.
                // Additionally, new transcode session creation flows were
                // added, but the fw version remained 3.2.
                // This means in order to differentiate between the new/legacy
                // transcode paths this must check if the fw is running the 3.2
                // version, or if it has fallen back to the 3.0 version for
                // Windows. Differentiating between fw 3.2 with new/legacy
                // transcode on Android would be much more difficult. However,
                // since there is no real use case for transcode on Android
                // this 3.2 check here is fine as long as the Android path
                // returns an error and avoids needing to differentiate.
                PAVP_DEVICE_ASSERTMESSAGE("Received GetStreamKeyEx, but legacy transcode not supported on Android");
                pHeader = reinterpret_cast<PAVPCmdHdr*>(pOutput);
                pHeader->PavpCmdStatus = PAVP_STATUS_INVALID_FUNCTION;
                hr = S_OK;
                break;
#endif
                if (m_pGpuHal->IsAppIDRegisterMgmtRequired())
                {
                    PAVP_DEVICE_NORMALMESSAGE("Programming the App ID register for key injection.");
                    hr = m_pOsServices->ProgramAppIDRegister(PAVP_PCH_INJECT_KEY_MODE_S1);
                    if (FAILED(hr))
                    {
                        PAVP_DEVICE_ASSERTMESSAGE("Could not program the App ID register. Error = 0x%x.", hr);
                        break;
                    }
                }
            }
        }

        hr = m_pRootTrustHal->PassThrough(
                            pInput,
                            ulInSize,
                            pOutput,
                            ulOutSize,
                            IgdData,
                            &IgdDataLen,
                            bIsWidiMsg);
        pHeaderIn = reinterpret_cast<PAVPCmdHdr*>(pInput);
        pHeader = reinterpret_cast<PAVPCmdHdr*>(pOutput);

        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("PCH Pass through call failed. Error = 0x%x.", hr);

            // Clear private data flag if it's there, though it should never be set on fail.
            if (pHeader->PavpCmdStatus == FIRMWARE_PRIVATE_INFO_FLAG)
            {
                pHeader->PavpCmdStatus ^= FIRMWARE_PRIVATE_INFO_FLAG;
            }
            break;
        }

#if (_DEBUG || _RELEASE_INTERNAL)
        // no need to take any action after completion of passthrough call for FW App GUIDs.
        // for example don't call setstream key even if private data is present
        if (m_PavpCryptoSessionMode == PAVP_CRYPTO_SESSION_TYPE_WIDI_MECOMM_FW_APP ||
            m_PavpCryptoSessionMode == PAVP_CRYPTO_SESSION_TYPE_PAVP_MECOMM_FW_APP)
        {
            break;
        }
#endif
        //if GetKeyBlobAPI is used by App then m_bSkipAsmf will be set to TRUE
        //key translation from (Sn)Sn-1 to (Sn)Kb will be avoided if m_bSkipAsmf is set to TRUE
        if (m_PavpSessionType == PAVP_SESSION_TYPE_DECODE)
        {
            if (reinterpret_cast<PAVPCmdHdr*>(pInput)->CommandId == FIRMWARE_CMD_GET_KEY_BLOB ||
                reinterpret_cast<PAVPCmdHdr*>(pInput)->CommandId == FIRMWARE_42_CMD_GET_KEY_BLOB)
            {
                m_bSkipAsmf = TRUE;
            }
        }

        // Check if the ME firmware has private information for the driver.
        if (pHeader->PavpCmdStatus == FIRMWARE_PRIVATE_INFO_FLAG)
        {
            PAVP_DEVICE_NORMALMESSAGE("ME firmware returned private data.");
            pHeader->PavpCmdStatus ^= FIRMWARE_PRIVATE_INFO_FLAG;

            if (IgdDataLen > 0)
            {
                if (IgdDataLen == PAVP_EPID_STREAM_KEY_LEN || 
                    IgdDataLen == (2*PAVP_EPID_STREAM_KEY_LEN))
                {
                    MOS_ZeroMemory(&SetKeyParams, sizeof(PAVP_SET_STREAM_KEY_PARAMS));
                    PAVP_DEVICE_NORMALMESSAGE("Programming a new 'decrypt' key for the ME.");

                    SetKeyParams.StreamType = PAVP_SET_KEY_DECRYPT;
                    MOS_SecureMemcpy(&(SetKeyParams.EncryptedDecryptKey),
                        PAVP_EPID_STREAM_KEY_LEN, IgdData, 
                        PAVP_EPID_STREAM_KEY_LEN);

                    if ((2*PAVP_EPID_STREAM_KEY_LEN) == IgdDataLen)
                    {
                        PAVP_DEVICE_NORMALMESSAGE("Programming a new 'encrypt' key for the ME.");
                        SetKeyParams.StreamType = PAVP_SET_KEY_BOTH;
                        MOS_SecureMemcpy(&(SetKeyParams.EncryptedEncryptKey),
                            PAVP_EPID_STREAM_KEY_LEN, 
                            IgdData+PAVP_EPID_STREAM_KEY_LEN, 
                            PAVP_EPID_STREAM_KEY_LEN);
                    }

                    hr = SetStreamKey(SetKeyParams, false);
                    if (FAILED(hr))
                    {
                        PAVP_DEVICE_ASSERTMESSAGE("Failed to program keys for the ME. Error = 0x%x.", hr);
                        break;
                    } 
                }

                if (pHeader->ApiVersion >= FIRMWARE_API_VERSION_4_3 && IgdDataLen == 2 * PAVP_SIGMA_STREAM_KEY_MAX_LEN  && (m_PavpSessionType == PAVP_SESSION_TYPE_TRANSCODE) && (m_PavpSessionMode == PAVP_SESSION_MODE_STOUT))
                {
                    MOS_ZeroMemory(&SetKeyParams, sizeof(PAVP_SET_STREAM_KEY_PARAMS));
                    PAVP_DEVICE_NORMALMESSAGE("Programming a new 'encrypt' key for the ME  for fw >= 4_3.");

                    SetKeyParams.StreamType = PAVP_SET_KEY_BOTH;
                    MOS_SecureMemcpy(&(SetKeyParams.EncryptedEncryptKey),
                        PAVP_EPID_STREAM_KEY_LEN,
                        IgdData,
                        PAVP_EPID_STREAM_KEY_LEN);

                    MOS_SecureMemcpy(&(SetKeyParams.EncryptedDecryptKey),
                        PAVP_EPID_STREAM_KEY_LEN,
                        IgdData + PAVP_SIGMA_STREAM_KEY_MAX_LEN,
                        PAVP_EPID_STREAM_KEY_LEN);

                    hr = SetStreamKey(SetKeyParams, false);
                    if (FAILED(hr))
                    {
                        PAVP_DEVICE_ASSERTMESSAGE("Failed to program keys for the ME. Error = 0x%x.", hr);
                        break;
                    }

                    //Return kb wrapped decode and encode key to app for fw43 afterwards
                    if (nullptr != pOutput)
                    {
                        PAVPCmdHdr* pOutHdr = reinterpret_cast<PAVPCmdHdr*>(pOutput);
                        if (pOutHdr->BufferLength + sizeof(PAVPCmdHdr) + 2 * PAVP_EPID_STREAM_KEY_LEN <= ulOutSize)
                        {
                            MOS_SecureMemcpy(pOutput + ulOutSize - 2 * PAVP_EPID_STREAM_KEY_LEN,
                                PAVP_EPID_STREAM_KEY_LEN,
                                IgdData,
                                PAVP_EPID_STREAM_KEY_LEN);

                            MOS_SecureMemcpy(pOutput + ulOutSize - PAVP_EPID_STREAM_KEY_LEN,
                                PAVP_EPID_STREAM_KEY_LEN,
                                IgdData + PAVP_SIGMA_STREAM_KEY_MAX_LEN,
                                PAVP_EPID_STREAM_KEY_LEN);
                        }
                    }
                    
                    MOS_ZeroMemory(IgdData, FIRMWARE_MAX_IGD_DATA_SIZE);  // Clear private data

                    PAVP_DEVICE_NORMALMESSAGE("PAVP session is ready for decrypt/decode/display.");
                    PrintDebugRegisters();
                }
                else if (pHeader->ApiVersion >= FIRMWARE_API_VERSION_4_2 && IgdDataLen == 2*PAVP_SIGMA_STREAM_KEY_MAX_LEN)
                {
                    MOS_ZeroMemory(&SetKeyParams, sizeof(PAVP_SET_STREAM_KEY_PARAMS));
                    PAVP_DEVICE_NORMALMESSAGE("Programming a new 'decrypt' key for the ME.");

                    SetKeyParams.StreamType = PAVP_SET_KEY_DECRYPT;
                    MOS_SecureMemcpy(&(SetKeyParams.EncryptedDecryptKey),
                        PAVP_EPID_STREAM_KEY_LEN,
                        IgdData,
                        PAVP_EPID_STREAM_KEY_LEN);

                    hr = SetStreamKey(SetKeyParams, false);
                    if (FAILED(hr))
                    {
                        PAVP_DEVICE_ASSERTMESSAGE("Failed to program keys for the ME. Error = 0x%x.", hr);
                        break;
                    }
                }

                IgdDataLen = (IgdDataLen > FIRMWARE_MAX_IGD_DATA_SIZE) ? FIRMWARE_MAX_IGD_DATA_SIZE : IgdDataLen;
                MOS_ZeroMemory(IgdData, IgdDataLen); // Clear private data

                PAVP_DEVICE_NORMALMESSAGE("PAVP session is ready for decrypt/decode/display.");
                PrintDebugRegisters();
            }
        }
        else if (pHeader->CommandId == FIRMWARE_CMD_GET_STREAM_KEY_EX && pHeader->PavpCmdStatus == PAVP_STATUS_SUCCESS) // if no FIRMWARE_PRIVATE_INFO_FLAG
        {
            ///////////////////////////////////////////////////////////////////////////////
            // Take care of the special case of transcode sessions in which the key is
            // not given to the GPU through the driver and therefore it does not set
            // the private data bit.

            hr = m_pOsServices->GetSetPavpSessionStatus(m_PavpSessionType, m_PavpSessionMode, PAVP_SESSION_READY, &CurrentPavpSessionStatus);
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Could not update PAVP status to 'ready' via CMD_GET_STREAM_KEY_EX. hr = 0x%x", hr);
                break;
            }
            PAVP_DEVICE_NORMALMESSAGE("PAVP session is ready for transcode.");
            PrintDebugRegisters();
        }
        else if (pHeader->PavpCmdStatus != PAVP_STATUS_SUCCESS)
        {
            PAVP_DEVICE_ASSERTMESSAGE("RootTrust passthrough call returned ME status failure: 0x%x.", pHeader->PavpCmdStatus);
            break;
        }

        if (didRetrieveWrappedTitleKeys(pHeader))
        {
            hr = m_pOsServices->QuerySessionStatus(&CurrentPavpSessionStatus);
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Failed to query whether the session is alive. Error = 0x%x.", hr);
            }

            if (CurrentPavpSessionStatus == PAVP_SESSION_INITIALIZED)
            {
                hr = m_pOsServices->GetSetPavpSessionStatus(
                                        m_PavpSessionType,
                                        m_PavpSessionMode,
                                        PAVP_SESSION_READY,
                                        &CurrentPavpSessionStatus);
                if (FAILED(hr))
                {
                    PAVP_DEVICE_ASSERTMESSAGE("Could not update KMD PAVP status to 'ready', was %s.", SessionStatus2Str(CurrentPavpSessionStatus));
                    break;
                }
            }
            else
            {
                PAVP_DEVICE_NORMALMESSAGE("PAVP session was already READY.");
            }

            PAVP_DEVICE_NORMALMESSAGE("PAVP session is ready for wv9 decode.");
        }

        if (pHeaderIn)
        {
            if (pHeaderIn->CommandId == FIRMWARE_CMD_GET_KEY_BLOB)
            {
                // for telemetry events
                m_bitGfxInfoIds.set(GFXINFO_ID_CP_GKB);
            }

            if (pHeaderIn->CommandId == FIRMWARE_CMD_GET_STREAM_KEY_EX ||
                pHeaderIn->CommandId == FIRMWARE_CMD_GET_STREAM_KEY ||
                pHeaderIn->CommandId == FIRMWARE_42_CMD_GET_STREAM_KEY)
            {
                m_bGSK = true;
            }
        }

    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

uint8_t CPavpDevice::GetSessionId()
{
    return m_cpContext.enable ? m_cpContext.sessionId : PAVP_INVALID_SESSION_ID;
}

bool CPavpDevice::IsArbitratorSessionRequired() const
{
    PAVP_DEVICE_FUNCTION_ENTER;

    bool bArbitratorSessionRequired = (m_PavpSessionMode == PAVP_SESSION_MODE_HEAVY);

    PAVP_DEVICE_VERBOSEMESSAGE("Arbitrator required status : 0x%x.", bArbitratorSessionRequired);

    PAVP_DEVICE_FUNCTION_EXIT(0);
    return bArbitratorSessionRequired;
}

bool CPavpDevice::IsMemoryEncryptionEnabled() const
{
    PAVP_DEVICE_FUNCTION_ENTER;

    bool bUseHeavyMode = (m_PavpSessionMode == PAVP_SESSION_MODE_HEAVY ||
                          m_PavpSessionMode == PAVP_SESSION_MODE_ISO_DEC ||
                          m_PavpSessionMode == PAVP_SESSION_MODE_STOUT);

    PAVP_DEVICE_VERBOSEMESSAGE("Heavy Mode session status : 0x%x.", bUseHeavyMode);

    PAVP_DEVICE_FUNCTION_EXIT(0);
    return bUseHeavyMode;
}

// CPavpDevice Private Methods.

void CPavpDevice::ResetMemberVariables()
{
    PAVP_DEVICE_FUNCTION_ENTER;

    // HALs and services.
    m_pOsServices                  = nullptr;
    m_pRootTrustHal                = nullptr;
    m_pGpuHal                      = nullptr;
    m_cencDecoder                  = nullptr;
    m_cencDecoderNext              = nullptr;

    // Device State variables.
    m_PavpSessionType              = PAVP_SESSION_TYPE_DECODE;
    m_PavpSessionMode              = PAVP_SESSION_MODE_LITE;
    m_bHwSessionIsAlive            = FALSE;
    m_bPlaneEnableSet              = FALSE;
    m_eCryptoType                  = PAVP_ENCRYPTION_NONE;
    m_eDrmType                     = PAVP_DRM_TYPE_NONE;
    m_cachedPR3KeyBlob             = INTEL_OEM_KEYINFO_BLOB();
    m_bFwFallbackEnabled           = false;
    m_cpContext.value              = 0;
    m_bSkipAsmf                    = FALSE;
    m_TranscryptKernelSize         = 0;
    m_bGSK                         = FALSE;
    m_bitGfxInfoIds.reset();
    MOS_ZeroMemory(&m_waTable, sizeof(m_waTable));

    m_pDeviceContext               = nullptr;

#if (_DEBUG || _RELEASE_INTERNAL)
    m_bForceLiteModeForDebug              = FALSE;
    m_bLiteModeEnableHucStreamOutForDebug = FALSE;
    m_PavpCryptoSessionMode               = PAVP_CRYPTO_SESSION_TYPE_NONE;
#endif

    PAVP_DEVICE_FUNCTION_EXIT(0);
}

HRESULT CPavpDevice::CreateHalsAndServices(PAVP_DEVICE_CONTEXT pDeviceContext)
{
    HRESULT  hr = S_OK;
    PLATFORM sPlatform;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        // Note: this class does not care what pDeviceContext points to. 
        // It's up to the OS services class to decide what to do with it. 
        if (IsInitialized()                ||
            m_pOsServices       != nullptr ||
            m_pRootTrustHal     != nullptr ||
            m_pGpuHal           != nullptr)
        {
            // We are incorrectly calling this more than once (coding error somewhere).
            PAVP_DEVICE_ASSERTMESSAGE("Unexpected error: multiple calls to create service.");
            hr = E_UNEXPECTED;
            break;
        }

        // Create an OS Services instance.
        hr = CreateOsServices(pDeviceContext);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to allocate OS Services abstraction layer. Error = 0x%x.", hr);
            break;
        }

        MOS_ZeroMemory(&sPlatform, sizeof(sPlatform));
        hr = m_pOsServices->GetPlatform(sPlatform);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to get platform information. Error = 0x%x.", hr);
            break;
        }

        // Create a Root of Trust instance
        CPavpRootTrustHal::RootTrust_HAL_USE root_trust_use;
        if (m_PavpSessionType == PAVP_SESSION_TYPE_WIDI)
        {
            root_trust_use = CPavpRootTrustHal::RootTrust_HAL_WIDI_USE;
        }
        else
        {
            root_trust_use = CPavpRootTrustHal::RootTrust_HAL_PAVP_USE;
        }
#if (_DEBUG || _RELEASE_INTERNAL)
        if (m_PavpCryptoSessionMode == PAVP_CRYPTO_SESSION_TYPE_WIDI_MECOMM_FW_APP)
        {
            root_trust_use = CPavpRootTrustHal::RootTrust_HAL_WIDI_CLIENT_FW_APP_USE;
        }
        else if (m_PavpCryptoSessionMode == PAVP_CRYPTO_SESSION_TYPE_PAVP_MECOMM_FW_APP)
        {
            root_trust_use = CPavpRootTrustHal::RootTrust_HAL_PAVP_CLIENT_FW_APP_USE;
        }
        else if (m_PavpCryptoSessionMode == PAVP_CRYPTO_SESSION_TYPE_MKHI_MECOMM_FW_APP)
        {
            root_trust_use = CPavpRootTrustHal::RootTrust_HAL_MKHI_CLIENT_FW_APP_USE;
        }
        else if (m_PavpCryptoSessionMode == PAVP_CRYPTO_SESSION_TYPE_GSC_PROXY_MECOMM_FW_APP)
        {
            root_trust_use = CPavpRootTrustHal::RootTrust_HAL_GSC_PROXY_CLIENT_FW_APP_USE;
        }
#endif
        m_pRootTrustHal = CPavpRootTrustHal::RootHal_Factory(root_trust_use, m_pOsServices);
        if (m_pRootTrustHal == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to allocate root of trust hardware abstraction layer.");
            hr = E_FAIL;
            break;
        }

        // Creates a GPU HAL instance.
        hr = m_pOsServices->GetWaTable(m_waTable);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to get Work-Around Table information. Error = 0x%x.", hr);
            break;
        }

        m_pGpuHal = CPavpGpuHal::PavpGpuHalFactory(hr, sPlatform, m_waTable, *m_pOsServices);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Error allocating GPU HAL abstraction layer. hr = 0x%x.", hr);
            break;
        }

        if (m_pGpuHal == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("PavpGpuHalFactory returned a NULL instance.");
            hr = E_UNEXPECTED;  // Static analysis (Klocwork) needs to see this check.
            break;
        }

        // Parsing assistance in KMD is different between OSs (non-existent in Linux). This
        // assistance (among other things) adds an MI_BATCH_BUFFER_END command automatically,
        // so in Linux we need to add it manually. Therefore, the GPU HAL must be informed of this.
        m_pGpuHal->SetParsingAssistanceFromKmd(m_pOsServices->IsParsingAssistanceInKmd());

        // Do not call Simulation if previous call fails otherwise simulation will overwrite previous call's status.

    } while(FALSE);

    if (FAILED(hr))
    {
        // If anything failed, clean up everything so we can try again later. 
        // This prevents having some objects instantiated while others are not. 
        Cleanup();
    }

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::CleanupHwSession()
{
    HRESULT hr = S_OK;
    PAVP_SESSION_STATUS CurrentPavpSessionStatus  = PAVP_SESSION_UNSUPPORTED;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        if (m_pOsServices == nullptr ||
            m_pGpuHal == nullptr)
        {
            PAVP_DEVICE_NORMALMESSAGE("GPU HAL or OS Services have not been created.");
            break;
        }

        if (!IsSessionIdValid())
        {
            PAVP_DEVICE_NORMALMESSAGE("Session ID invalid.");
            break;
        }
        MOS_TraceEventExt(EVENT_CP_DESTROY, EVENT_TYPE_START, &m_cpContext.value, sizeof(uint32_t), nullptr, 0);
        // This notifies the display that this session is about to be destroyed and by that
        // makes sure no frame are displayed after the session keys are terminated.
        hr = m_pOsServices->GetSetPavpSessionStatus(
            m_PavpSessionType,
            m_PavpSessionMode,
            PAVP_SESSION_TERMINATE_IN_PROGRESS,
            &CurrentPavpSessionStatus);
        if (FAILED(hr) || (CurrentPavpSessionStatus != PAVP_SESSION_TERMINATE_IN_PROGRESS))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to move status to 'terminate in progress'. Error = 0x%x", hr);
            MOS_TraceEventExt(EVENT_CP_DESTROY, EVENT_TYPE_END, nullptr, 0, nullptr, 0);
            break;
        }

        // While we have a valid session ID, session establishment may not have completed key exchange.
        // Thats OK as we can terminate anyways.
        hr = TerminateSessionWithConfirmation(m_cpContext.sessionId);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("HW key termination failed. Error = 0x%x.", hr);
        }

        // Tell KMD we're done with this session and it's been terminated gracefully.
        hr = m_pOsServices->GetSetPavpSessionStatus(
                          m_PavpSessionType,
                          m_PavpSessionMode,
                          PAVP_SESSION_UNINITIALIZED,
                          &CurrentPavpSessionStatus);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to move status to 'uninitialized'. hr = 0x%x", hr);
        }
        // clean up context
        m_cpContext.value = 0;
        MOS_TraceEventExt(EVENT_CP_DESTROY, EVENT_TYPE_END, nullptr, 0, nullptr, 0);
    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::Cleanup()
{

    if (m_pOsServices == nullptr ||
        m_pOsServices->m_pOsInterface == nullptr ||
        m_pOsServices->m_pOsInterface->osCpInterface== nullptr)
    {
        if (m_pOsServices != nullptr && m_pOsServices->m_pOsInterface == nullptr)
        {
            m_pOsServices = nullptr;
        }

        PAVP_DEVICE_ASSERTMESSAGE("Cannot proceed without having OS-supporting modules.");
        return E_UNEXPECTED;
    }

    MosCp *pMosCp = dynamic_cast<MosCp*>(m_pOsServices->m_pOsInterface->osCpInterface);

    if (pMosCp == nullptr)
    {
        PAVP_DEVICE_ASSERTMESSAGE("Cannot proceed without having OS-supporting modules.");
        return E_UNEXPECTED;
    }

    HRESULT hr = S_OK;

    PAVP_DEVICE_FUNCTION_ENTER;

    CleanupHwSession();

    if (m_cencDecoder)
    {
        MOS_Delete(m_cencDecoder);
        m_cencDecoder = nullptr;
    }

    if (m_cencDecoderNext)
    {
        MOS_Delete(m_cencDecoderNext);
        m_cencDecoderNext = nullptr;
    }

    if (m_pGpuHal)
    {
        MOS_Delete(m_pGpuHal);
        m_pGpuHal = nullptr;
    }

    if (m_pRootTrustHal)
    {
        MOS_Delete(m_pRootTrustHal);
        m_pRootTrustHal = nullptr;
    }

    if (m_pOsServices)
    {

        // Clear out authenticated kernels if PAVP stout mode.
        if (m_pOsServices->m_pOsInterface)
        {
            if (m_pOsServices->m_pOsInterface->osCpInterface && 
                m_PavpSessionMode == PAVP_SESSION_MODE_STOUT)
            {
                pMosCp->SetTK(nullptr, 0);
            }
        }
    }

    ResetMemberVariables();

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

BOOL CPavpDevice::IsSessionIdValid() CONST
{
    return m_cpContext.enable;
}

HRESULT CPavpDevice::InitAuthenticatedKernels()
{
    HRESULT                   hr                          = S_OK;
    const uint32_t*             pTranscryptMetaData         = nullptr;
    const uint32_t*             pEncryptedKernels           = nullptr;
    uint32_t                    uiTranscryptMetaDataSize    = 0;
    uint32_t                    uiKernelsSize               = 0;
    BYTE*                     pCopyOfTranscryptedKernels  = nullptr;
    bool                      bPreProduction              = false;
    PAVPCmdHdr                checkPreProductionCmd       = { 0 };
    GetGroupIDOutBuf          checkPreProductionResponse  = { 0 };
    uint32_t                    IgdDataLen = 0;
    MosCp                     *pMosCp = nullptr;

    std::vector<BYTE>   transcryptedKernels;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        if (!IsInitialized())
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to InitAuthenticatedKernels() due to uninited pavp device.");
            hr = E_UNEXPECTED;
            break;
        }

        if (m_pGpuHal == nullptr || m_pRootTrustHal == nullptr || m_pDeviceContext == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Cannot proceed without having supporting modules.");
            hr = E_UNEXPECTED;
            break;
        }

        if (m_pOsServices == nullptr ||
            m_pOsServices->m_pOsInterface == nullptr ||
            m_pOsServices->m_pOsInterface->osCpInterface== nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Cannot proceed without having OS-supporting modules.");
            hr = E_UNEXPECTED;
            break;
        }

        PAVP_DEVICE_NORMALMESSAGE("Invalidating transcrypted and authenticated kernels for use by video processor.");
        pMosCp = dynamic_cast<MosCp*>(m_pOsServices->m_pOsInterface->osCpInterface);

        if (pMosCp == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Cannot proceed without having OS-supporting modules.");
            hr = E_UNEXPECTED;
            break;
        }

        pMosCp->SetTK(nullptr, 0);

        //Check if platform is Pre-Production from CSE FW
        checkPreProductionCmd.ApiVersion = FIRMWARE_API_VERSION_1_5;
        checkPreProductionCmd.CommandId = FIRMWARE_CMD_GET_HDCP_STATE;
        checkPreProductionCmd.PavpCmdStatus = 0x0;
        checkPreProductionCmd.BufferLength = 0x0;

        hr = m_pRootTrustHal->PassThrough(
            reinterpret_cast<PBYTE>(&checkPreProductionCmd),
            sizeof(checkPreProductionCmd),
            reinterpret_cast<PBYTE>(&checkPreProductionResponse),
            sizeof(checkPreProductionResponse),
            nullptr,
            &IgdDataLen,
            FALSE
        );
        if (SUCCEEDED(hr))
        {
            PAVP_DEVICE_NORMALMESSAGE("Check preproduction with get_hdcp_status cmd, response is 0x%x", checkPreProductionResponse.Header.PavpCmdStatus);
            //On Production platform, get_hdcp_status cmd will response 0x1003
            bPreProduction = (checkPreProductionResponse.Header.PavpCmdStatus != 0x1003);
        }
        else
            PAVP_DEVICE_ASSERTMESSAGE("Check preproduction error, hr = 0x%x", hr);

        PAVP_DEVICE_NORMALMESSAGE("Getting encrypted kernels to be authenticated and transcrypted.");
        hr = m_pGpuHal->GetInfoToTranscryptKernels(
            bPreProduction,
            pTranscryptMetaData,
            uiTranscryptMetaDataSize,
            pEncryptedKernels,
            uiKernelsSize);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Couldn't get info to transcrypt kernels.");
            break;
        }
        if (pTranscryptMetaData == nullptr || uiTranscryptMetaDataSize == 0)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Couldn't get metadata to transcrypt/re-encrypt encrypted kernels.");
            hr = E_FAIL;
            break;
        }
        if (pEncryptedKernels == nullptr || uiKernelsSize == 0)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Couldn't get encrypted kernels.");
            hr = E_FAIL;
            break;
        }

        if (m_pGpuHal->IsAppIDRegisterMgmtRequired())
        {
            PAVP_DEVICE_NORMALMESSAGE("Programming App ID 0x%x for kernel transcryption.", PAVP_AUTH_KERNEL_TRANSCRYPT_SESSION_ID);
            hr = m_pOsServices->ProgramAppIDRegister(PAVP_PCH_INJECT_KEY_MODE_S1_FR,
                PAVP_AUTH_KERNEL_TRANSCRYPT_SESSION_ID);
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Couldn't program App ID for kernel transcryption.");
                break;
            }
        }
        if ((uiKernelsSize % sizeof(UINT64)) != 0)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Size of kernels in bytes should be multiples of 8 bytes.");
            hr = E_FAIL;
            break;
        }

        PAVP_DEVICE_NORMALMESSAGE("Requesting firmware to authenticate and transcrypt kernels.");
        hr = m_pRootTrustHal->TranscryptKernels(
            uiTranscryptMetaDataSize,
            pTranscryptMetaData,
            uiKernelsSize,
            pEncryptedKernels,
            transcryptedKernels);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Couldn't authenticate and transcrypt kernels.");
            break;
        }

        PAVP_DEVICE_NORMALMESSAGE("Setting transcrypted and authenticated kernels for use by video processor.");
        pCopyOfTranscryptedKernels = MOS_NewArray(BYTE, transcryptedKernels.size());
        if (pCopyOfTranscryptedKernels == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Couldn't allocate memory for copy of transcrypted kernels.");
            hr = E_OUTOFMEMORY;
            break;
        }

        // This copy is needed in order to convert to C interface for Android and VP module.
        MOS_STATUS eStatus = MOS_SecureMemcpy(pCopyOfTranscryptedKernels, transcryptedKernels.size(),
            transcryptedKernels.data(), transcryptedKernels.size());
        if (eStatus != MOS_STATUS_SUCCESS)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Couldn't copy memory for copy of transcrypted kernels.");
            hr = E_FAIL;
            break;
        }

        pMosCp->SetTK(
            reinterpret_cast<uint32_t*>(pCopyOfTranscryptedKernels), transcryptedKernels.size());

        m_TranscryptKernelSize = static_cast<uint32_t>(transcryptedKernels.size());

        PAVP_DEVICE_NORMALMESSAGE("Authenticated kernels are initialized.");

    } while (FALSE);

    if (FAILED(hr))
    {
        MOS_DeleteArray(pCopyOfTranscryptedKernels);
    }

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::InitHwDecodeSession()
{
    HRESULT                             hr                          = S_OK;
    PAVP_GET_CONNECTION_STATE_PARAMS2   sConnState                  = {0, {0}, {0}};
    PAVP_SET_STREAM_KEY_PARAMS          sSetKeyParams;
    PAVP_PCH_INJECT_KEY_MODE            PchInjectKeyMode            = PAVP_PCH_INJECT_KEY_MODE_S1;
    PAVP_FW_PAVP_MODE                   PavpMode;
    PAVP_SESSION_STATUS                 CurrentPavpSessionStatus    = PAVP_SESSION_UNSUPPORTED;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        PAVP_DEVICE_NORMALMESSAGE("Initiating session creation with PCH/GSC.");

        // Verify that the device has been initialized.
        if (!IsInitialized())
        {
            PAVP_DEVICE_ASSERTMESSAGE("Fail to InitHwDecodeSession() due to uninited pavp device.");
            hr = E_UNEXPECTED;
            break;
        }

        // Verify that a valid session ID has been assigned to the device.
        if (!IsSessionIdValid())
        {
            PAVP_DEVICE_ASSERTMESSAGE("Cannot proceed without a valid PAVP session ID.");
            hr = E_UNEXPECTED;
            break;
        }

        // Verify that the HALs and OS Services have been initialized.
        if (m_pRootTrustHal == nullptr ||
            m_pOsServices == nullptr ||
            m_pGpuHal == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Cannot proceed without creating HAL/service objects.");
            hr = E_UNEXPECTED;
            break;
        }

        if (m_PavpSessionType == PAVP_SESSION_TYPE_TRANSCODE)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Wrong path for initiating transcode.");
            hr = E_INVALIDARG;
            break;
        }

        if (m_pRootTrustHal->HasFwCaps(FIRMWARE_CAPS_PAVP_INIT))
        {
            // pavp init need inject the KbKf1 key 
            PchInjectKeyMode = PAVP_PCH_INJECT_KEY_MODE_S1_FR;
        }
        else
        {
            // For firmware version <= 3, we don't support stout mode or isolated decode mode.
            if (m_PavpSessionMode == PAVP_SESSION_MODE_STOUT || m_PavpSessionMode == PAVP_SESSION_MODE_ISO_DEC)
            {
                PAVP_DEVICE_ASSERTMESSAGE("Current firmware version doesn't support isolated/stout decode session");
                hr = E_UNEXPECTED;
                break;
            }

            hr = m_pRootTrustHal->SetSlot(m_cpContext.sessionId);
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("PCH 'SetSlot' call failed. Error = 0x%x.", hr);
                break;
            }
        }

        if (m_pGpuHal->IsAppIDRegisterMgmtRequired())
        {
            hr = m_pOsServices->ProgramAppIDRegister(PchInjectKeyMode);

            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Could not program the App ID register. Error = 0x%x.", hr);
                break;
            }
        }

        if (m_pRootTrustHal->HasFwCaps(FIRMWARE_CAPS_PAVP_INIT))
        {
            if (m_eDrmType == PAVP_DRM_TYPE_FAIRPLAY && m_PavpSessionType != PAVP_SESSION_TYPE_WIDI)
            {
                PAVP_DEVICE_NORMALMESSAGE("ASMF session so not doing pavp init");
            }
            else
            {
#if (_DEBUG || _RELEASE_INTERNAL)
                if (m_bForceLiteModeForDebug && m_bLiteModeEnableHucStreamOutForDebug)
                {
                    hr = m_pRootTrustHal->EnableLiteModeHucStreamOutBit();
                    if (FAILED(hr))
                    {
                        PAVP_DEVICE_ASSERTMESSAGE("EnableLiteModeHucStreamOutBit call failed. Error = 0x%x.", hr);
                        break;
                    }
                }
#endif

                PAVP_DEVICE_NORMALMESSAGE("Requesting firmware do the pavp init");
                PavpMode = static_cast<PAVP_FW_PAVP_MODE>(m_PavpSessionMode);
                hr = m_pRootTrustHal->PavpInit4(
                                            PavpMode,
                                            m_cpContext.sessionId);
                if (FAILED(hr))
                {
                    PAVP_DEVICE_ASSERTMESSAGE("RootTrust 'PavpInit4' call failed. Error = 0x%x.", hr);
                    break;
                }
            }
        }
        else
        {
            MOS_ZeroMemory(&sSetKeyParams, sizeof(PAVP_SET_STREAM_KEY_PARAMS));
            sSetKeyParams.StreamType = PAVP_SET_KEY_ENCRYPT;
            hr = m_pRootTrustHal->GetNonce(
                                true,                                       // bResetStreamKey
                                sizeof(sSetKeyParams.EncryptedEncryptKey),  // uEncE0Size
                                (UINT8 *)sSetKeyParams.EncryptedEncryptKey, // puiEncryptedE0
                                (uint32_t *)&(sConnState.NonceIn));           // puiNonce
            
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("PCH 'GetNonce' call failed. Error = 0x%x.", hr);
                break;
            }

#if !EMULATE_CP_RELEASE_TIMING_TEST
#if (_DEBUG || _RELEASE_INTERNAL)
            BOOL bTestSessionInPlay;
            PAVP_DEVICE_NORMALMESSAGE("Checking to make sure the session is in play after 'GetNonce'.");
            hr = QuerySessionInPlay(bTestSessionInPlay, m_cpContext.sessionId);
            if (SUCCEEDED(hr))
            {
                PAVP_DEVICE_NORMALMESSAGE("PAVP session in play: %d.", bTestSessionInPlay);
                if (!bTestSessionInPlay)
                {
                    PAVP_DEVICE_ASSERTMESSAGE("PAVP session id %d is not in play", m_cpContext.sessionId);
                }
            }
            else
            {
                PAVP_DEVICE_ASSERTMESSAGE("Failed to query session in play. Error = 0x%x.", hr);
            }
#endif
#endif

            // Program the new 'E0' key received from firmware.
            PAVP_DEVICE_NORMALMESSAGE("Programming the new 'E0' key on behalf of ME firmware.");
            hr = SetStreamKey(sSetKeyParams, false);
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Failed to update the key. Error = 0x%x.", hr);
                break;
            }

            PAVP_DEVICE_NORMALMESSAGE("Retrieving connection state for ME firmware verification.");
            hr = GetConnectionState(sConnState);
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Failed to get the connection state. Error = 0x%x.", hr);
                break;
            }
     
            // Let firmware confirm that the key has been updated and check if HDCP is on.
            PAVP_DEVICE_NORMALMESSAGE("Requesting ME firmware connection state verification.");
            hr = m_pRootTrustHal->VerifyConnStatus(sizeof(sConnState.ConnectionStatus), sConnState.ConnectionStatus);
            
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("PCH 'VerifyConnStatus' call failed. Error = 0x%x.", hr);
                break;
            }

            PAVP_DEVICE_NORMALMESSAGE("Connection state verified by PCH/GSC.");
        }
        if (!(m_pRootTrustHal->HasFwCaps(FIRMWARE_CAPS_PAVP_INIT)))
        {
            // Called again because memory status might have changed.
            // This could be optimized by skipping it for lite sessions.
            PAVP_DEVICE_NORMALMESSAGE("Retrieving memory status for ME firmware verification.");
            hr = GetConnectionState(sConnState);
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Failed to get the memory status. Error = 0x%x.", hr);
                break;
            }

            // Let firmware verify heavy mode if necessary.
            PAVP_DEVICE_NORMALMESSAGE("Requesting ME firmware memory status verification.");
            hr = m_pRootTrustHal->VerifyMemStatus(sizeof(sConnState.ProtectedMemoryStatus), sConnState.ProtectedMemoryStatus);
            
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Failed to verify memory status. Error = 0x%x", hr);
                break;
            }

            PAVP_DEVICE_NORMALMESSAGE("Memory status verified by PCH.");
        }

        // Move the session status to 'initialized'.
        hr = m_pOsServices->GetSetPavpSessionStatus(
                                    m_PavpSessionType,
                                    m_PavpSessionMode,
                                    PAVP_SESSION_INITIALIZED,
                                    &CurrentPavpSessionStatus,
                                    nullptr,
                                    nullptr);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Get/Set Session Status call failed. Error = 0x%x.", hr);
            break;
        } 

        PAVP_DEVICE_NORMALMESSAGE("PAVP session is initialized.");

    } while (FALSE);
    MOS_TraceEventExt(EVENT_CP_CREATE, EVENT_TYPE_INFO, &m_cpContext.value, sizeof(uint32_t), nullptr, 0);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::GetConnectionState(PAVP_GET_CONNECTION_STATE_PARAMS2& sConnState) CONST
{
    HRESULT                             hr = S_OK;
    PAVP_GPU_HAL_QUERY_STATUS_PARAMS    sQueryStatus;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        if (m_pOsServices != nullptr && m_pOsServices->IsSimulationEnabled())
        {
            // right now we can't get memory status in the sim environment
            break;
        }

        // Submit a 'query status' command to HW in order to get the connection state.
        MOS_ZeroMemory(&sQueryStatus, sizeof(sQueryStatus));
        sQueryStatus.QueryOperation     = PAVP_QUERY_STATUS_CONNECTION_STATE;
        sQueryStatus.AppId              = m_cpContext.sessionId;
        sQueryStatus.AllocationIndex    = 1; // An arbitrary index. We will only use one allocation entry so it does not matter.
        sQueryStatus.Value.NonceIn      = sConnState.NonceIn;

        hr = BuildAndSubmitCommand(QUERY_STATUS, &sQueryStatus);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to get the connection state from GPU HAL. Error = 0x%x.", hr);
            break;
        }

        MOS_SecureMemcpy(sConnState.ConnectionStatus,
                        sizeof(sConnState.ConnectionStatus),
                        sQueryStatus.OutData,
                        MOS_MIN(sizeof(sConnState.ConnectionStatus), sizeof(sQueryStatus.OutData)));

        PAVP_DEVICE_NORMALMESSAGE("Connection status received from hardware: \
                          \n0x%08x 0x%08x 0x%08x 0x%08x\n0x%08x 0x%08x 0x%08x 0x%08x",
                        (*(int*)&sConnState.ConnectionStatus[0]),  (*(int*)&sConnState.ConnectionStatus[4]), 
                        (*(int*)&sConnState.ConnectionStatus[8]),  (*(int*)&sConnState.ConnectionStatus[12]),
                        (*(int*)&sConnState.ConnectionStatus[16]), (*(int*)&sConnState.ConnectionStatus[20]), 
                        (*(int*)&sConnState.ConnectionStatus[24]), (*(int*)&sConnState.ConnectionStatus[28]));

        // Submit a 'query status' command to HW in order to get the memory mode.
        sQueryStatus.QueryOperation = PAVP_QUERY_STATUS_MEMORY_MODE;
        hr = BuildAndSubmitCommand(QUERY_STATUS, &sQueryStatus);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to get the memory status from GPU HAL. Error = 0x%x.", hr);
            break;
        }

        MOS_SecureMemcpy(sConnState.ProtectedMemoryStatus,
                        sizeof(sConnState.ProtectedMemoryStatus),
                        sQueryStatus.OutData,
                        MOS_MIN(sizeof(sConnState.ProtectedMemoryStatus), sizeof(sQueryStatus.OutData)));

        PAVP_DEVICE_NORMALMESSAGE("Memory status received from hardware: \
                          \n0x%08x 0x%08x 0x%08x 0x%08x",
                        (*(int*)&sConnState.ProtectedMemoryStatus[0]),  (*(int*)&sConnState.ProtectedMemoryStatus[4]), 
                        (*(int*)&sConnState.ProtectedMemoryStatus[8]),  (*(int*)&sConnState.ProtectedMemoryStatus[12]));

    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::SetPlaneEnables()
{
    HRESULT                         hr                 = S_OK;
    DWORD                           dwSessionId        = 0;
    PAVP_GPU_HAL_CRYPTO_COPY_PARAMS sCryptoCopyParams;

    PAVP_DEVICE_FUNCTION_ENTER;
    MOS_ZeroMemory(&sCryptoCopyParams, sizeof(sCryptoCopyParams));

    do
    {
        if (m_pRootTrustHal == nullptr ||
            m_pOsServices == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("HAL/Services have not been created.");
            hr = E_UNEXPECTED;
            break;
        }


        // Init the src data pointer to NULL. This will also avoid crash of UMD when SetPlaneEnables() is called 2nd times onwards.
        sCryptoCopyParams.pSrcData = nullptr;

        // This function may be called multiple times per session, but we should only program the PED packet once.
        if (m_bPlaneEnableSet)
        {
            PAVP_DEVICE_NORMALMESSAGE("Plane enables have already been set. No need to proceed.");
            break;
        }

        if (IsMemoryEncryptionEnabled())
        {
            // We will enable all overlay sprite and planes for decryption. 
            // Presently, Windows only use Sprites for displaying PAVP Heavy Mode conetnts due to restrictions from Desktop Window Manager
            //              Although using Sprites is simple but it wastes Memory/Power BW when Sprite is full screen and Primary Plane is not shutdown
            // Presently, Android also uses Sprites for displaying PAVP Heavy Mode contents but there is big push for switching to Primary Planes
            //              to save Memory/Power BWs. This change is to prepare PAVP driver for future.

            // We enable PLANE_CURSOR_C due to some mismatches between VLV and big Core. 
            // In VLV's display, SpriteD is defined as the second bit, but in IVB, the second bit is for plane CURSOR C. 
            // These differences will be merged in CHV.
            // Allocate memory to store the PED packet.
            sCryptoCopyParams.pSrcData = new (std::nothrow) BYTE[PED_PACKET_SIZE];
            if (sCryptoCopyParams.pSrcData == nullptr)
            {
                PAVP_DEVICE_NORMALMESSAGE("Unable to allocate memory to store the PED paket.");
                hr = E_OUTOFMEMORY;
                break;
            }
            sCryptoCopyParams.dwDataSize = PED_PACKET_SIZE;

            if (m_PavpSessionType == PAVP_SESSION_TYPE_TRANSCODE)
            {
                // For heavy transcode we need to set serpent Memory Configuration via GetPlaneEnable/CRYPTO_COPY to put session in heavy mode.
                PAVP_DEVICE_NORMALMESSAGE("Get encrypted PLANE_PACKET in order to enable heavy mode Memory Configuration later.");
                hr = m_pRootTrustHal->GetPlaneEnables(1, 0, PED_PACKET_SIZE, sCryptoCopyParams.pSrcData);
            }
            else
            {
                PAVP_DEVICE_NORMALMESSAGE("Requesting plane enable packet encryption from PCH: 0x%04x", PlanesToEnable);
                hr = m_pRootTrustHal->GetPlaneEnables(1, PlanesToEnable, PED_PACKET_SIZE, sCryptoCopyParams.pSrcData);
            }
            
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Failed to get plane enables. Error = 0x%x", hr);
                break;
            }

            PAVP_DEVICE_NORMALMESSAGE("Received encrypted plane enable packet: \
                         \n0x%08x 0x%08x 0x%08x 0x%08x",
                        (*(int*)sCryptoCopyParams.pSrcData),        (*(int*)(sCryptoCopyParams.pSrcData + 4)), 
                        (*(int*)(sCryptoCopyParams.pSrcData + 8)),  (*(int*)(sCryptoCopyParams.pSrcData + 12)));
        }
        else
        {
            // Not heavy mode. No need to proceeed.
            PAVP_DEVICE_NORMALMESSAGE("Heavy mode is not in use. No need to proceed.");
            break;
        }

        // Send a 'crypto copy' command to HW in order to set plane enables.
        sCryptoCopyParams.eCryptoCopyType   = CRYPTO_COPY_TYPE_SET_PLANE_ENABLE;
        sCryptoCopyParams.dwAppId           = m_cpContext.sessionId;
        sCryptoCopyParams.dwDataSize        = PED_PACKET_SIZE;
        hr = BuildAndSubmitCommand(CRYPTO_COPY, &sCryptoCopyParams);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to submit 'crypto copy' command. Error = 0x%x.", hr);
            break;
        }

        PAVP_DEVICE_NORMALMESSAGE("Plane enables have been set.");
        m_bPlaneEnableSet = TRUE;

        // Update the Heavy Mode reg key.
        // Get session ID.
        dwSessionId = sCryptoCopyParams.dwAppId;
        m_pOsServices->SessionRegistryUpdate(
                __MEDIA_USER_FEATURE_VALUE_HEAVY_SESSIONS_ID,
                PAVP_SESSION_TYPE_DECODE, // Currently all heavy sessions are decode sessions.
                dwSessionId,
                CP_SESSIONS_UPDATE_OPEN);
    } while (FALSE);

    // Free the allocated memory if necessary.
    if (sCryptoCopyParams.pSrcData)
    {
        delete[] sCryptoCopyParams.pSrcData;
    }

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::TerminateSession(
    uint32_t     SessionId,
    BOOL         bCheckSessionInPlay)
{
    HRESULT                             hr                      = S_OK;
    PAVP_GPU_HAL_KEY_EXCHANGE_PARAMS    sTerminateParams        = { 0, PAVP_KEY_EXCHANGE_TYPE_TERMINATE, { 0 }, { { 0 } } };
    BOOL                                bSessionInPlay          = TRUE;
    DWORD                               SessionInPlayQueryCount = 0;
    PAVP_GPU_REGISTER_VALUE             ullRegValue             = 0;
    BOOL                                bGlobalTerminateReq     = FALSE;

    PAVP_DEVICE_FUNCTION_ENTER;

    PAVP_DEVICE_NORMALMESSAGE("Terminating session ID: %d", SessionId);

    do
    {
        // Verify that GPU HAL and OS Services have been created.
        if (m_pOsServices   == nullptr ||
            m_pGpuHal       == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("GPU HAL or OS Services have not been created, cannot issue a terminate command.");
            hr = E_UNEXPECTED;
            break;
        }

        // Terminate the slot for the FW
        if (m_pRootTrustHal != nullptr && !m_pRootTrustHal->HasFwCaps(FIRMWARE_CAPS_PAVP_INIT))
        {
            hr = m_pRootTrustHal->InvalidateSlot(SessionId);
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Failed to cleanup FW slot!");
                // Continue on to cleanup the GPU, don't fail out.
            }
        }
    
        // Check if the CB submission is deferred
        if (!m_pOsServices->IsCBSubmissionImmediate())
        {
            PAVP_DEVICE_ASSERTMESSAGE("Defer Session Termination");
            hr = S_OK;
            break;
        }

        if (bCheckSessionInPlay)
        {
            hr = QuerySessionInPlay(bSessionInPlay, SessionId);
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Failed to query session in play. Error = 0x%x", hr);
                break;
            }

            if (!bSessionInPlay)
            {
                // Nothing to do for the GPU, already gone
                PAVP_DEVICE_NORMALMESSAGE("GPU Pavp slot already clean.");
                hr = S_OK;
                break;
            }
        }

        PAVP_DEVICE_NORMALMESSAGE("Terminating the session...");

        MOS_ZeroMemory(&sTerminateParams, sizeof(PAVP_GPU_HAL_KEY_EXCHANGE_PARAMS));

        sTerminateParams.KeyExchangeType = PAVP_KEY_EXCHANGE_TYPE_TERMINATE;
        sTerminateParams.AppId           = SessionId;

        hr = BuildAndSubmitCommand(KEY_EXCHANGE, &sTerminateParams);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to send key termination command to hardware. hr = 0x%x", hr);
            break;
        }

        if (!bCheckSessionInPlay)
        {
            PAVP_DEVICE_VERBOSEMESSAGE("Call does not require polling for SiP status change.");
            hr = S_OK;
            break;
        }

        // The above GPU command is non-blocking, so we need to wait for the
        // session to not be in play for the KMD to allow the state to be 
        // reserved by a new session.
        while (SessionInPlayQueryCount < MAX_SESSION_IN_PLAY_QUERIES)
        {
            SessionInPlayQueryCount++;
            // Read the SessionInPlay register with bGpuShouldBeIdle=true
            // Doing that we make sure that reading the SessionInPlay register will not collide with the previous
            // terminate command. Changing this argument may cause the HW to hang. 
            // Please don't change it unless you really know what you are doing.
            hr = m_pOsServices->ReadMMIORegister(PAVP_REGISTER_SESSION_IN_PLAY, ullRegValue, CPavpGpuHal::ConvertRegType2RegOffset(PAVP_REGISTER_SESSION_IN_PLAY));
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Failed to query session in play after terminate. Error = 0x%x", hr);
                break;
            }

            // Mask top 15 bits - ullRegValue & 0x0000ffff
            // Check if ANY of lower 15 bits are turned-on
            // It is reverse logic i.e. if no decode session is alive then schedule Global Terminate
            bGlobalTerminateReq = (ullRegValue & PAVP_DECODE_SESSION_IN_PLAY_MASK) ? FALSE : TRUE;

            // Find out if our session is alive based on the register value we have just read.
            if (!m_pGpuHal->VerifySessionInPlay(SessionId, ullRegValue))
            {
                PAVP_DEVICE_NORMALMESSAGE("Session no longer in play after %d session in play queries", SessionInPlayQueryCount);
                break;
            }

            MosUtilities::MosSleep(1);
        }

        if (SessionInPlayQueryCount == MAX_SESSION_IN_PLAY_QUERIES)
        {
            // Too many iterations. We give up.
            PAVP_DEVICE_ASSERTMESSAGE("Session still in play after %d session in play queries.", MAX_SESSION_IN_PLAY_QUERIES);
            hr = E_FAIL;
            break;
        }

        if (FAILED(hr))
        {
            // The previous 'break' only broke our of the inner 'while' loop.
            break;
        }

        // In case of video path, the FW is no longer responsible for erasing the stream key from the HW slot.
        // Driver is responsible for erasing the video stream key when session is closed in HW, and HDCP requirement associated with 
        // closed FW stream slot will be cleaned.
        if (m_pRootTrustHal != nullptr)
        {
            hr = m_pRootTrustHal->InvalidateStreamKey(SessionId);
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Failed to cleanup FW stream slot!");
                // Don't fail out, continue on to the following global terminate.
            }
        }

        // This function performs a global terminate for decode sessions only if no decode sessions are open.
        // This generates new serpent keys for future heavy mode sessions for better security.
        // For auto teardown (Unsolicited attack) it has been observed that creating a new 
        // session without doing this hangs the HW (IVB). So if the UMD crashes after the last terminate above
        // and before this call we have a problem. 
        // This is called also if this is the last session gracefully terminated.
        // Caution - if Auto Teardown happens then bits in session-in-play register will be turned-on beside this 
        // session this means Gobal Terminate will not be executed unless all PAVP sessions are closed inline.
        // This is inline with PAVP BSpec.
        if (bGlobalTerminateReq)
        {
            hr = GlobalTerminateDecode();
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Could not send global terminate decode. hr = 0x%x", hr);
                // If this fails we are still OK (in the non autoteardown scenario).
                hr = S_OK;
            }
        }
    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

int32_t CPavpDevice::BuildAndSubmitMultiCryptoCopyCommand(PAVP_GPU_HAL_MULTI_CRYPTO_COPY_PARAMS *pBuildMultiCryptoCopyParams, size_t cryptoCopyNumber) CONST
{
    int32_t            hr = 0;
    MOS_COMMAND_BUFFER cmdBuffer;
    MOS_STATUS         eStatus = MOS_STATUS_UNKNOWN;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        if (pBuildMultiCryptoCopyParams == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Invalid parameters passed in to BuildAndSubmitMultiCryptoCopyCommand.");
            hr = E_UNEXPECTED;
            return hr;
        }

        // Verify that the GPU HALs and OS Services have already been initialized.
        if (m_pOsServices                                       == nullptr ||
            m_pGpuHal                                           == nullptr ||
            m_pOsServices->m_pOsInterface                       == nullptr ||
            m_pOsServices->m_pOsInterface->pfnIsGpuContextValid == nullptr ||
            m_pOsServices->m_pOsInterface->pfnSetGpuContext     == nullptr ||
            m_pOsServices->m_pOsInterface->pfnGetCommandBuffer  == nullptr ||
            m_pOsServices->m_pOsInterface->pfnSetPerfTag        == nullptr ||
            m_pOsServices->m_pOsInterface->pfnResetPerfBufferID == nullptr ||
            m_pOsServices->m_pOsInterface->pfnCreateGpuContext  == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("HAL/Services have not been created.");
            hr = E_UNEXPECTED;
            break;
        }

        eStatus = m_pOsServices->m_pOsInterface->pfnIsGpuContextValid(m_pOsServices->m_pOsInterface, MOS_GPU_CONTEXT_VIDEO);
        if (MOS_FAILED(eStatus))
        {
            MOS_GPUCTX_CREATOPTIONS_ENHANCED createOption = {};
            eStatus = m_pOsServices->m_pOsInterface->pfnCreateGpuContext(
                m_pOsServices->m_pOsInterface,
                MOS_GPU_CONTEXT_VIDEO,
                MOS_GPU_NODE_VIDEO,
                &createOption);
            if (MOS_FAILED(eStatus))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Failed to create GPU context. eStatus = 0x%x", eStatus);
                hr = E_UNEXPECTED;
                break;
            }
        }

        hr = m_pOsServices->m_pOsInterface->pfnSetGpuContext(m_pOsServices->m_pOsInterface, MOS_GPU_CONTEXT_VIDEO);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Error, could not set GPU context. hr = 0x%x", hr);
            break;
        }

        // The following 2 lines are needed for performance measurement.
        m_pOsServices->m_pOsInterface->pfnSetPerfTag(m_pOsServices->m_pOsInterface, PERFTAG_PXP);
        m_pOsServices->m_pOsInterface->pfnResetPerfBufferID(m_pOsServices->m_pOsInterface);

        hr = m_pOsServices->m_pOsInterface->pfnGetCommandBuffer(m_pOsServices->m_pOsInterface, &cmdBuffer, 0);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Error, could not get command buffer. hr = 0x%x", hr);
            break;
        }

        //CP component doesn't need attribute, reset it.
        MOS_ZeroMemory(&cmdBuffer.Attributes, sizeof(MOS_COMMAND_BUFFER_ATTRIBUTES));

        PAVP_DEVICE_VERBOSEMESSAGE("Building Multiple CRYPTO_COPY cmd buffer.");
        hr = m_pGpuHal->BuildMultiCryptoCopy(
            &cmdBuffer,
            pBuildMultiCryptoCopyParams,
            cryptoCopyNumber);

    } while (false);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::BuildAndSubmitCommand(PAVP_OPERATION_TYPE type, PVOID pParams) CONST
{
    HRESULT                         hr = S_OK;
    MOS_COMMAND_BUFFER              CmdBuffer;
    BOOL                            ImmediateSubmissionNeeded = FALSE;
    MOS_STATUS                      eStatus = MOS_STATUS_UNKNOWN;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        // Verify that the GPU HALs and OS Services have already been initialized.
        if (m_pOsServices == nullptr ||
            m_pGpuHal == nullptr ||
            m_pOsServices->m_pOsInterface == nullptr ||
            m_pOsServices->m_pOsInterface->pfnIsGpuContextValid == nullptr ||
            m_pOsServices->m_pOsInterface->pfnSetGpuContext == nullptr     ||
            m_pOsServices->m_pOsInterface->pfnGetCommandBuffer == nullptr  ||
            m_pOsServices->m_pOsInterface->pfnSetPerfTag == nullptr        ||
            m_pOsServices->m_pOsInterface->pfnResetPerfBufferID == nullptr ||
            m_pOsServices->m_pOsInterface->pfnCreateGpuContext == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("HAL/Services have not been created.");
            hr = E_UNEXPECTED;
            break;
        }

        ImmediateSubmissionNeeded = m_pOsServices->IsCBSubmissionImmediate();
        if (!ImmediateSubmissionNeeded && (type == CRYPTO_COPY))
        {
            // DX12 uses a deferred submission model.
            // Currently only the 'Set Plane Enable' crypto copy command
            // is handled specially. This command will not be needed in Gen11+.
            PAVP_GPU_HAL_CRYPTO_COPY_PARAMS* params = reinterpret_cast<PAVP_GPU_HAL_CRYPTO_COPY_PARAMS*>(pParams);
            if (params->eCryptoCopyType != CRYPTO_COPY_TYPE_SET_PLANE_ENABLE)
            {
                PAVP_DEVICE_ASSERTMESSAGE("Only 'Set Plane Enable' crypto copy command needs special handling when deferred submissions are in use.");
                hr = E_UNEXPECTED;
                break;
            }

            // Call into KMD to set plane enables using immediate submission
            PAVP_DEVICE_VERBOSEMESSAGE("Using KMD escape to set plane enables in deferred submission case.");
            hr = m_pOsServices->SetPlaneEnablesUsingKmd(
                    m_cpContext.sessionId,
                    (PAVP_PLANE_ENABLE_TYPE)PlanesToEnable,
                    params->pSrcData,
                    params->dwDataSize
                  );

            // This break will break out of the main do/while loop.
            break;
        }

        eStatus = m_pOsServices->m_pOsInterface->pfnIsGpuContextValid(m_pOsServices->m_pOsInterface, MOS_GPU_CONTEXT_VIDEO);
        if (MOS_FAILED(eStatus))
        {
            MOS_GPUCTX_CREATOPTIONS_ENHANCED createOption = {};
            eStatus = m_pOsServices->m_pOsInterface->pfnCreateGpuContext(
                m_pOsServices->m_pOsInterface,
                MOS_GPU_CONTEXT_VIDEO,
                MOS_GPU_NODE_VIDEO,
                &createOption);
            if (MOS_FAILED(eStatus))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Failed to create GPU context. eStatus = 0x%x", eStatus);
                hr = E_UNEXPECTED;
                break;
            }
        }

        hr = m_pOsServices->m_pOsInterface->pfnSetGpuContext(m_pOsServices->m_pOsInterface, MOS_GPU_CONTEXT_VIDEO);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Error, could not set GPU context. hr = 0x%x", hr);
            break;
        }

        // The following 2 lines are needed for performance measurement.
        m_pOsServices->m_pOsInterface->pfnSetPerfTag(m_pOsServices->m_pOsInterface, PERFTAG_PXP);
        m_pOsServices->m_pOsInterface->pfnResetPerfBufferID(m_pOsServices->m_pOsInterface);
        
        hr = m_pOsServices->m_pOsInterface->pfnGetCommandBuffer(m_pOsServices->m_pOsInterface, &CmdBuffer, 0);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Error, could not get command buffer. hr = 0x%x", hr);
            break;
        }

        //CP component doesn't need attribute, reset it.
        MOS_ZeroMemory(&CmdBuffer.Attributes, sizeof(MOS_COMMAND_BUFFER_ATTRIBUTES));

        // Put the commands in the given buffer.
        switch (type)
        {
            case KEY_EXCHANGE:
                PAVP_DEVICE_VERBOSEMESSAGE("Building KEY_EXCHANGE cmd buffer.");
                hr = m_pGpuHal->BuildKeyExchange(
                        &CmdBuffer,
                        *(static_cast<PAVP_GPU_HAL_KEY_EXCHANGE_PARAMS *>(pParams)));
                break;

            case QUERY_STATUS:
                PAVP_DEVICE_VERBOSEMESSAGE("Building QUERY_STATUS cmd buffer.");
                hr = m_pGpuHal->BuildQueryStatus(
                        &CmdBuffer,
                        *(static_cast<PAVP_GPU_HAL_QUERY_STATUS_PARAMS *>(pParams)));
                break;

            case CRYPTO_COPY:
                PAVP_DEVICE_VERBOSEMESSAGE("Building CRYPTO_COPY cmd buffer.");
                hr = m_pGpuHal->BuildCryptoCopy(
                        &CmdBuffer,
                        *(static_cast<PAVP_GPU_HAL_CRYPTO_COPY_PARAMS *>(pParams)));
                break;

            default:
                // Should never get here.
                PAVP_DEVICE_ASSERTMESSAGE("Unrecognized command type requested: %d", type);
                hr = E_INVALIDARG;
                break;
        }

    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::GlobalTerminateDecode()
{
    HRESULT hr = S_OK;
    PAVP_GPU_REGISTER_VALUE rvalue = 0;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        if (m_pOsServices == nullptr)
        {
            hr = E_FAIL;
            CP_OS_ASSERTMESSAGE("Error, OS interface is NULL");
            break;
        }

        hr = m_pOsServices->WriteMMIORegister(
                                PAVP_REGISTER_GLOBAL_TERMINATE,
                                (PAVP_GPU_REGISTER_VALUE)PAVP_REGISTER_GLOBAL_TERMINATE_BIT_DECODE,
                                rvalue);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Error, could not write global terminate register. hr = 0x%x", hr);
            break;
        }
    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

void CPavpDevice::PrintDebugRegisters() CONST
{

#if !EMULATE_CP_RELEASE_TIMING_TEST
#if (_DEBUG || _RELEASE_INTERNAL)
    PAVP_GPU_REGISTER_VALUE ullValue = 0;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        if (m_pOsServices == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("OS Services is NULL.");
            break;
        }

        PAVP_DEVICE_NORMALMESSAGE("Printing PAVP Status/Debug Register values:");
        if (SUCCEEDED(m_pOsServices->ReadMMIORegister(PAVP_REGISTER_SESSION_IN_PLAY, ullValue, CPavpGpuHal::ConvertRegType2RegOffset(PAVP_REGISTER_SESSION_IN_PLAY))))
        {
            PAVP_DEVICE_NORMALMESSAGE("Session-in-play Register: 0x%llx", ullValue);
        }

        if (SUCCEEDED(m_pOsServices->ReadMMIORegister(PAVP_REGISTER_STATUS, ullValue, CPavpGpuHal::ConvertRegType2RegOffset(PAVP_REGISTER_STATUS))))
        {
            PAVP_DEVICE_NORMALMESSAGE("Status Register: 0x%llx", ullValue);
        }
//linux driver is not able to read debug register;
#if defined(WIN32)
        if (SUCCEEDED(m_pOsServices->ReadMMIORegister(PAVP_REGISTER_DEBUG, ullValue, CPavpGpuHal::ConvertRegType2RegOffset(PAVP_REGISTER_DEBUG))))
        {
            PAVP_DEVICE_NORMALMESSAGE("Debug Register: 0x%llx", ullValue);
        }
#endif
    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(1);
#endif // _DEBUG || _RELEASE_INTERNAL
#endif
}

HRESULT CPavpDevice::CheckCSHeartBeat()
{
    HRESULT                           hr         = S_OK;
    PAVP_GET_CONNECTION_STATE_PARAMS2 sConnState = {0, {0}, {0}};

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        PAVP_DEVICE_NORMALMESSAGE("Getting Out-Of-Band(OOB) connection status from PCH.");

        // Verify that the device has been initialized.
        if (!IsInitialized())
        {
            PAVP_DEVICE_ASSERTMESSAGE("Not expected to be called before device is initialized.");
            hr = E_UNEXPECTED;
            break;
        }

        // Verify that a valid session ID has been assigned to the device.
        if (!IsSessionIdValid())
        {
            PAVP_DEVICE_ASSERTMESSAGE("Cannot proceed without a valid PAVP session ID.");
            hr = E_UNEXPECTED;
            break;
        }

        // Verify that the HALs and OS Services have been initialized.
        if (m_pRootTrustHal == nullptr ||
            m_pOsServices == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Cannot proceed without creating HAL/service objects.");
            hr = E_UNEXPECTED;
            break;
        }

        hr = m_pRootTrustHal->GetHeartBeatStatusNonce(
            reinterpret_cast<uint32_t *>(&(sConnState.NonceIn)),
            m_cpContext.sessionId,
            m_eDrmType);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("PCH 'GetNonce' call failed. Error = 0x%x", hr);
            break;
        }

        PAVP_DEVICE_NORMALMESSAGE("Retrieving connection state for ME firmware verification.");
        hr = GetConnectionState(sConnState);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to get the connection state. Error = 0x%x", hr);
            break;
        }
 
        // Let firmware confirm that the key has been updated and check if HDCP is on.
        PAVP_DEVICE_NORMALMESSAGE("Requesting ME firmware connection state verification.");
        hr = m_pRootTrustHal->VerifyHeartBeatConnStatus(
                    sizeof(sConnState.ConnectionStatus), 
                    sConnState.ConnectionStatus, 
                    m_cpContext.sessionId,
                    m_eDrmType);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("PCH 'VerifyConnStatus' call failed. Error = 0x%x", hr);
            break;
        }
    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

BOOL CPavpDevice::IncrementCounter(
    PDWORD              pCounter,
    DWORD               dwIncrement)
{
    BOOL bRet = TRUE;

    PAVP_DEVICE_FUNCTION_ENTER;

    if (pCounter != nullptr)
    {
        (*(reinterpret_cast<PUINT64>(pCounter))) += dwIncrement;
    }
    else
    {
        PAVP_DEVICE_ASSERTMESSAGE("ERROR: pCounter is NULL!");
        bRet = FALSE;
    }

    PAVP_DEVICE_FUNCTION_EXIT(bRet);
    return bRet;
}

BOOL CPavpDevice::CounterSwapEndian(PDWORD pCounter)
{
    BOOL    bRet      = TRUE;
    DWORD   dwTemp[4] = {0}; // use a temp buffer to support in-place copies

    PAVP_DEVICE_FUNCTION_ENTER;

    if (pCounter != nullptr)
    {
        MOS_SecureMemcpy(dwTemp, sizeof(DWORD) * 2, pCounter + 2, sizeof(DWORD) * 2);
        MOS_SecureMemcpy(dwTemp + 2, sizeof(DWORD) * 2, pCounter, sizeof(DWORD) * 2);
        MOS_SecureMemcpy(pCounter, sizeof(DWORD) * 4, dwTemp, sizeof(DWORD) * 4);

        SwapEndian64(pCounter);
        SwapEndian64(pCounter + 2);
    }
    else
    {
        PAVP_DEVICE_ASSERTMESSAGE("ERROR: pCounter is NULL!");
        bRet = FALSE;
    }

    PAVP_DEVICE_FUNCTION_EXIT(bRet);
    return bRet;
}

void CPavpDevice::SessionRegistryUpdate(
    CP_USER_FEATURE_VALUE_ID  UserFeatureValueId, // [in]
    PAVP_SESSION_TYPE         eSessionType,       // [in]
    DWORD                     dwSessionId,        // [in] (In case of software sessions this variable is ignored)
    CP_SESSIONS_UPDATE_TYPE   eUpdateType)        // [in]
{
    if (m_pOsServices == nullptr)
    {
        PAVP_DEVICE_ASSERTMESSAGE("ERROR: OS Services is NULL!");
    }
    else
    {
        m_pOsServices->SessionRegistryUpdate(
                            UserFeatureValueId,
                            eSessionType,
                            dwSessionId,
                            eUpdateType);
    }
}

HRESULT CPavpDevice::InitiateArbitratorSession(bool bPreserveOriginalState)
{
    HRESULT                 hr                          = E_FAIL;
    PAVP_SESSION_STATUS     eArbPavpSessionStatus       = PAVP_SESSION_UNINITIALIZED;
    PAVP_SESSION_STATUS     eArbPreviousStatus          = PAVP_SESSION_UNSUPPORTED;

    // Save current session mode and type before processing
    PAVP_SESSION_MODE       eOriginalPavpSessionMode    = m_PavpSessionMode;
    PAVP_SESSION_TYPE       eOriginalRequestedType      = m_PavpSessionType;
    uint32_t                CpContextSave               = m_cpContext.value;
    PAVP_SESSION_STATUS     CurrentPavpSessionStatus    = PAVP_SESSION_UNSUPPORTED;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        if (m_pRootTrustHal == nullptr ||
            m_pOsServices == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Unexpected error: HALs have not been initialized.");
            hr = E_UNEXPECTED;
            break;
        }

        // Force Heavy Mode for arbitrator session 
        m_PavpSessionMode = PAVP_SESSION_MODE_HEAVY;
        m_PavpSessionType = PAVP_SESSION_TYPE_WIDI;

#if (_DEBUG || _RELEASE_INTERNAL)
        if (m_bForceLiteModeForDebug)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Create a lite mode arbitratory session because m_bForceLiteModeForDebug is true");
            m_PavpSessionMode = PAVP_SESSION_MODE_LITE;
        }
#endif
        MOS_TraceEventExt(EVENT_CP_RESOURCR_SESSION, EVENT_TYPE_START, &m_PavpSessionType, sizeof(uint32_t), &m_PavpSessionMode, sizeof(uint32_t));
        // Bit 15 is not set
        // Reserve a Arbitrator session ID.
        // TODO - change PAVP_SESSION_TYPE_WIDI to some generic name
        PAVP_DEVICE_NORMALMESSAGE("Reserving a PAVP session ID for Arbitrator or Dummy Session.");
        hr = ReserveSessionId(
            PAVP_SESSION_TYPE_WIDI,
            m_PavpSessionMode,
            &eArbPavpSessionStatus,
            &eArbPreviousStatus);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Could not obtain a hardware PAVP session ID.");
            break;
        }

        if (eArbPavpSessionStatus == PAVP_SESSION_READY)
        {
            PAVP_DEVICE_NORMALMESSAGE("Arbitrator session already initialized.");
            hr = S_OK;
            break;
        }

        PAVP_DEVICE_NORMALMESSAGE("Received PAVP session tag 0x%x.", m_cpContext.value);

        hr = InitHwDecodeSession();
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to init HW session. HR 0x%x", hr);
            CleanupHwSession();
            break;
        }
        PAVP_DEVICE_NORMALMESSAGE("Arbitrator PAVP session is initialized.");

        // Go ahead and move it to ready, too.
        hr = m_pOsServices->GetSetPavpSessionStatus(
            PAVP_SESSION_TYPE_WIDI,
            m_PavpSessionMode,
            PAVP_SESSION_READY,
            &CurrentPavpSessionStatus,
            nullptr,
            nullptr);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Get/Set Session Status call failed. Error = 0x%x.", hr);
            CleanupHwSession();
            break;
        }
    } while (FALSE);

    if (bPreserveOriginalState)
    {
        // Restore current session mode 
        m_PavpSessionMode = eOriginalPavpSessionMode;
        m_PavpSessionType = eOriginalRequestedType;
        m_cpContext.value = CpContextSave;

        // Reset plane enable flag to ORIGINAL PAVP session state,
        // if required, in error case or non-error case 
        m_bPlaneEnableSet = FALSE;

        PAVP_DEVICE_NORMALMESSAGE("Cleaning RootTrust HAL for reusing with Normal Session");
        if (m_pRootTrustHal)
        {
            m_pRootTrustHal->Cleanup();
            hr = m_pRootTrustHal->Init();
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Failed to initialize RootTrust HAL. Error = 0x%x", hr);
            }
        }
    }

    MOS_TraceEventExt(EVENT_CP_RESOURCR_SESSION, EVENT_TYPE_END, &hr, sizeof(hr), nullptr, 0);
    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::InitArbitratorSessionRes(PAVP_DEVICE_CONTEXT pDeviceContext)
{
    HRESULT hr = S_OK;

    PAVP_DEVICE_FUNCTION_ENTER;
    PAVP_DEVICE_ASSERT(pDeviceContext);

    do
    {
        if (!IsInitialized())
        {
            hr = Init(pDeviceContext);
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Failed Init() hr=0x%x", hr);
                break;
            }

            hr = SetEncryptionType(PAVP_ENCRYPTION_AES128_CTR);
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Call to SetEncryptionType() Failed. Error code = 0x%x", hr);
                break;
            }

            hr = SetSessionType(PAVP_SESSION_TYPE_WIDI);
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Call to SetSessionType() Failed. Error code = 0x%x", hr);
                break;
            }

            hr = SetSessionMode(PAVP_SESSION_MODE_HEAVY);
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Call to SetSessionMode() Failed. Error code = 0x%x", hr);
                break;
            }
        }

        hr = OpenArbitratorSession();
        if (FAILED(hr))
        {
            hr = InitiateArbitratorSession(false);
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Failed to InitiateArbitratorSession()");
                break;
            }
        }
    } while (false);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::AsyncHwKeyInterfaceToMeGpu(
    PAVP_GET_SET_DATA_NEW_HW_KEY eGetSetCommand,     //[in]
    BYTE                         *pInput,
    uint32_t                       ulInSize,
    BYTE                         *pOutput,
    uint32_t                       ulOutSize)
{
    HRESULT                     hr                  = S_OK;
    PAVPCmdHdr                  *pHeader            = nullptr;

    // Local buffer for IGD-ME private data - required to make PassThrough work!
    UINT8                       IgdData[FIRMWARE_MAX_IGD_DATA_SIZE];
    uint32_t                      IgdDataLen          = sizeof(IgdData);

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        if (m_pRootTrustHal == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Unexpected error: HALs have not been initialized.");
            hr = E_UNEXPECTED;
            break;
        }

        // Verify that the device has been initialized.
        if (!IsInitialized())
        {
            PAVP_DEVICE_ASSERTMESSAGE("Init() must be called before sending data to ME.");
            hr = E_UNEXPECTED;
            break;
        }

        switch (eGetSetCommand)
        {
            case PAVP_SET_CEK_KEY_IN_ME:
                // Set key blob in ME-FW
            case PAVP_GET_CEK_KEY_FROM_ME:
                // Get key blob from ME-FW  
                if (pInput == nullptr ||
                    pOutput == nullptr)
                {
                    PAVP_DEVICE_ASSERTMESSAGE("Invalid parameters from Cmd = 0x%x!", eGetSetCommand);
                    hr = E_INVALIDARG;
                    break;
                }

                hr = m_pRootTrustHal->PassThrough(
                        pInput,
                        ulInSize,
                        pOutput,
                        ulOutSize,
                        IgdData,
                        &IgdDataLen,
                        FALSE);
                pHeader = reinterpret_cast<PAVPCmdHdr*>(pOutput);
                if (FAILED(hr))
                {
                    PAVP_DEVICE_ASSERTMESSAGE("PCH Pass through call failed. Error = 0x%x.", hr);
                    break;
                }
                break;

            case PAVP_SET_CEK_KEY_IN_GPU:
                // Program key rotation blob in GPU
                if (pInput == nullptr)
                {
                    PAVP_DEVICE_ASSERTMESSAGE("Invalid parameters form Cmd = 0x%x!", eGetSetCommand);
                    hr = E_INVALIDARG;
                    break;
                }

                hr = SetKeyRotationBlob(reinterpret_cast<INTEL_OEM_KEYINFO_BLOB *>(pInput));
                if (FAILED(hr))
                {
                    PAVP_DEVICE_ASSERTMESSAGE("PCH 'AsyncHwKeyInterfaceToMeGpu' call failed. Error = 0x%x", hr);
                    break;
                }
                break;

            case PAVP_SET_CEK_INVALIDATE_IN_ME:
                PAVP_DEVICE_NORMALMESSAGE("Invalidate key in ME");
                break;

            default:
                PAVP_DEVICE_ASSERTMESSAGE("Unknown Get/Set operation Ops = %x", eGetSetCommand);
                hr = E_UNEXPECTED;
                break;
        }

    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

static void KeyCopy(
    DWORD(&destKey)[CPavpDevice::PAVP_KEY_SIZE_IN_DWORDS],
    CONST DWORD(&sourceKey)[CPavpDevice::PAVP_KEY_SIZE_IN_DWORDS])
{
    for (uint32_t i = 0; i < CPavpDevice::PAVP_KEY_SIZE_IN_DWORDS; ++i)
    {
        destKey[i] = sourceKey[i];
    }
}

HRESULT CPavpDevice::RotateDecryptKeyInASMFMode(
    CONST DWORD(&updateKey)[PAVP_KEY_SIZE_IN_DWORDS])
{
    HRESULT                     hr = S_OK;
    PAVP_SET_STREAM_KEY_PARAMS  setStreamKeyParams = { PAVP_SET_KEY_DECRYPT, { 0 }, { { 0 } } };

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        // incoming decrypt key is already wrapped in Kb. Just do the update.
        setStreamKeyParams.StreamType = PAVP_SET_KEY_DECRYPT;
        KeyCopy(setStreamKeyParams.EncryptedDecryptKey, updateKey);

        hr = SetStreamKey(setStreamKeyParams);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("SetStreamKey call failed. Error = 0x%x", hr);
            break;
        }

    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::RotateBothKeysInASMFMode(
    CONST DWORD(&decryptUpdateKey)[PAVP_KEY_SIZE_IN_DWORDS],
    CONST DWORD(&encryptUpdateKey)[PAVP_KEY_SIZE_IN_DWORDS])
{
    HRESULT                     hr = S_OK;
    PAVP_SET_STREAM_KEY_PARAMS  setStreamKeyParams = { PAVP_SET_KEY_DECRYPT, { 0 }, { { 0 } } };

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        // Incoming key is Kb wrapped. just update. 
        MOS_ZeroMemory(&setStreamKeyParams, sizeof(setStreamKeyParams));
        setStreamKeyParams.StreamType = PAVP_SET_KEY_BOTH;
        KeyCopy(setStreamKeyParams.EncryptedDecryptKey, decryptUpdateKey);
        KeyCopy(setStreamKeyParams.EncryptedEncryptKey, encryptUpdateKey);

        hr = SetStreamKey(setStreamKeyParams);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("SetStreamKey call failed. Error = 0x%x", hr);
            break;
        }

    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::SetKeyRotationBlob(const INTEL_OEM_KEYINFO_BLOB *pKeyInfoBlob)
{
    HRESULT hr = S_OK;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        // Verify that the device has been initialized.
        if (!IsInitialized())
        {
            PAVP_DEVICE_ASSERTMESSAGE("Init() must be called before sending data to ME.");
            hr = E_UNEXPECTED;
            break;
        }

        if (!pKeyInfoBlob)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Invalid Parameter!");
            hr = E_INVALIDARG;
            break;
        }

        // if the key has not been changed from our cached value, we do not need to update the stream key
        if (m_cachedPR3KeyBlob == *pKeyInfoBlob)
        {
            PAVP_DEVICE_NORMALMESSAGE("Cached PR3 key blob is up-to-date; skipping programming.");
            hr = S_OK;
            break;
        }

        PAVP_DEVICE_NORMALMESSAGE("Updating cached key blob.");
        m_cachedPR3KeyBlob = *pKeyInfoBlob;
        
        // Can use any blob type to extract appId, but the appId must match.
        if (pKeyInfoBlob->sNonHuCData.AppId != m_cpContext.sessionId)
        {
            PAVP_DEVICE_ASSERTMESSAGE("KeyInfo blob appId 0x%x does not belong to this PavpDevice, 0x%x.", pKeyInfoBlob->sNonHuCData.AppId, m_cpContext.sessionId);
            hr = E_INVALIDARG;
            break;
        }

        switch (pKeyInfoBlob->blobType)
        {
            case INTEL_OEM_KEYINFO_NONHUC_BLOB_TYPE:    // Video
                hr = RotateDecryptKeyInASMFMode(
                    reinterpret_cast<const DWORD(&)[PAVP_KEY_SIZE_IN_DWORDS]>(pKeyInfoBlob->sNonHuCData.TranscryptionKeyWrappedByKb));
                break;

            case INTEL_OEM_KEYINFO_NONSECURE_BLOB_TYPE: //  Audio
                // For Gen11, it appears that CEK and SP do not need to be swapped prior to programming
                // Code change added here to centralize it in one place instead of having to update DecryptContent/DecryptAudioContentMultiple/DecryptContentMultiple
                // When those three methods are refactored into one, then this code can be removed
                if (m_pGpuHal->IsPrologCryptoKeyExMgmtRequired())
                {
                    PAVP_DEVICE_VERBOSEMESSAGE("Gen11: Calling RotateBothKeysInASMFMode with CEK as decrypt key and SP as encrypt key");
                    hr = RotateBothKeysInASMFMode(
                        reinterpret_cast<const DWORD(&)[PAVP_KEY_SIZE_IN_DWORDS]>(pKeyInfoBlob->sNonSecureData.CekWrappedByKb),
                        reinterpret_cast<const DWORD(&)[PAVP_KEY_SIZE_IN_DWORDS]>(pKeyInfoBlob->sNonSecureData.SampleKeyWrappedByKb));
                }
                else
                {
                    PAVP_DEVICE_VERBOSEMESSAGE("pre-Gen11: Calling RotateBothKeysInASMFMode with SP as decrypt key and CEK as encrypt key");
                    hr = RotateBothKeysInASMFMode(
                        reinterpret_cast<const DWORD(&)[PAVP_KEY_SIZE_IN_DWORDS]>(pKeyInfoBlob->sNonSecureData.SampleKeyWrappedByKb),
                        reinterpret_cast<const DWORD(&)[PAVP_KEY_SIZE_IN_DWORDS]>(pKeyInfoBlob->sNonSecureData.CekWrappedByKb));
                }
                break;

            case INTEL_OEM_KEYINFO_HUC_BLOB_TYPE:       // HuC Video
                hr = RotateDecryptKeyInASMFMode(
                    reinterpret_cast<const DWORD(&)[PAVP_KEY_SIZE_IN_DWORDS]>(pKeyInfoBlob->sHuCData.CekWrappedByKb));
                break;

            default:
                PAVP_DEVICE_ASSERTMESSAGE("Unknown Get/Set operation Ops = %x", pKeyInfoBlob->blobType);
                hr = E_UNEXPECTED;
                break;
        }

        if (FAILED(hr))
        {
            break;
        }

        PAVP_DEVICE_NORMALMESSAGE("PAVP session is ready for key rotate session.");
        PrintDebugRegisters(); 

    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::SetCryptoKeyExParams(PMHW_PAVP_ENCRYPTION_PARAMS pMhwPavpEncParams)
{
    HRESULT hr = E_UNEXPECTED;
    PAVP_GPU_REGISTER_VALUE ValueRead = 0;
    PAVP_DEVICE_FUNCTION_ENTER;
    do
    {
        if (pMhwPavpEncParams == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("NULL ptr!");
            hr = E_INVALIDARG;
            break;
        }

        if (m_pGpuHal == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Cannot proceed without creating HAL/service objects.");
            hr = E_UNEXPECTED;
            break;
        }

        hr = m_pGpuHal->SetGpuCryptoKeyExParams(pMhwPavpEncParams);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("SetGpuCryptoKeyExParams failed. Error = 0x%x.", hr);
            break;
        }
    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDevice::ProcessUsageTableRequest(uint32_t HWProtectionFunctionID, PBYTE pOutputBufferFromME)
{
    PAVPCmdHdr*    pHeader = nullptr;
    ULONG          ulUsageTableSize = 0;

    if (pOutputBufferFromME == nullptr)
    {
        return E_INVALIDARG;
    }

    pHeader = reinterpret_cast<PAVPCmdHdr*>(pOutputBufferFromME);
    ulUsageTableSize = pHeader->BufferLength;

    return m_pOsServices->ProcessUsageTableRequest(
        HWProtectionFunctionID,
        ulUsageTableSize,
        pOutputBufferFromME + sizeof(PAVPCmdHdr),
        m_cpContext.sessionId);
}
