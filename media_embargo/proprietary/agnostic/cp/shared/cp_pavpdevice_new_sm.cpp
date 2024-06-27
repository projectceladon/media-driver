/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2020-2021
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

File Name: cp_pavpdeviceSM.cpp

Abstract:
    Implementation of the PAVP Device class for supporting new state machine.
    Handles all low level PAVP device details.

Environment:
    Windows 10, Linux

Notes:

======================= end_copyright_notice ==================================*/
#include "cp_pavpdevice_new_sm.h"
#include "cp_hal_gpu.h"
#include "cp_os_services.h"
#include "mos_gfxinfo_id.h"

#if MOS_MESSAGES_ENABLED

// Debug, converts session status to a string for debug logs.
const char* StateMachineStatus2Str(PXP_SM_STATUS Status)
{
    switch(Status)
    {
        case PXP_SM_STATUS_SUCCESS:
            return "PXP_SM_STATUS_SUCCESS";
        case PXP_SM_STATUS_RETRY_REQUIRED:
            return "PXP_SM_STATUS_RETRY_REQUIRED";
        case PXP_SM_STATUS_SESSION_NOT_AVAILABLE:
            return "PXP_SM_STATUS_SESSION_NOT_AVAILABLE";
        case PXP_SM_STATUS_ERROR_UNKNOWN:
            return "PXP_SM_STATUS_ERROR_UNKNOWN";
        case PXP_SM_STATUS_INSUFFICIENT_RESOURCE:
            return "PXP_SM_STATUS_INSUFFICIENT_RESOURCE";
        default:
            return "Unknown state machine status";
    }
    return "Unknown state machine status";
}

// Debug, converts session request to a string for debug logs.
const char* StateMachineRequest2Str(_PXP_SM_SESSION_REQ Status)
{
    switch(Status)
    {
        case PXP_SM_REQ_SESSION_ID_INIT:
            return "PXP_SM_REQ_SESSION_ID_INIT";
        case PXP_SM_REQ_SESSION_IN_PLAY:
            return "PXP_SM_REQ_SESSION_IN_PLAY";
        case PXP_SM_REQ_SESSION_TERMINATE:
            return "PXP_SM_REQ_SESSION_TERMINATE";
        default:
            return "Unknown state machine reuest";
    }
    return "Unknown state machine status";
}
#endif

#define PAVP_MAX_RESERVE_SESSION_ID_QUERIES     0x20

HRESULT CPavpDeviceNewSM::GetSessionIDInit(
    PAVP_SESSION_TYPE       eSessionType,
    PAVP_SESSION_MODE       eSessionMode)
{
    HRESULT                 hr                          = S_OK;
    uint32_t                uiQuery                     = 0;
    const uint32_t          uiMaxQueries                = PAVP_MAX_RESERVE_SESSION_ID_QUERIES;
    MosCp                   *pMosCp                     = nullptr;
    uint32_t                CpTag                       = 0;
    BOOL                    bInitSuccessful             = FALSE;
    PXP_SM_STATUS           eStateMahineStatus          = PXP_SM_STATUS_ERROR_UNKNOWN;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        // Validate input parameters.
        if (m_pOsServices == nullptr ||
            m_pOsServices->m_pOsInterface == nullptr ||
            m_pOsServices->m_pOsInterface->osCpInterface == nullptr)
        {
            hr = E_INVALIDARG;
            PAVP_DEVICE_ASSERTMESSAGE("Invalid parameter.");
            break;
        }

        pMosCp = dynamic_cast<MosCp*>(m_pOsServices->m_pOsInterface->osCpInterface);
        if (pMosCp == nullptr)
        {
            PAVP_DEVICE_ASSERTMESSAGE("CP OS Services is NULL.");
            hr = E_UNEXPECTED;
            break;
        }

        // Try to move status to 'in init'.
        // Search session database to see if we can find a slot.
        hr = m_pOsServices->ReqNewPavpSessionState(
                                eSessionType,
                                eSessionMode,
                                PXP_SM_REQ_SESSION_ID_INIT,     // What we want.
                                &eStateMahineStatus,            // status
                                &CpTag);                        // Our pavp session context.
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to retrieve session ID. hr = 0x%x", hr);
            break;
        }
        m_cpContext.value = CpTag;

        switch(eStateMahineStatus)
        {
            case PXP_SM_STATUS_SUCCESS:          // We got what we desired.
                bInitSuccessful = TRUE;
                break;
            case PXP_SM_STATUS_RETRY_REQUIRED:
                MosUtilities::MosSleep(100);
                break;
            default:
                hr = E_FAIL;
                PAVP_DEVICE_ASSERTMESSAGE("Unexpected state machine status: %s", StateMachineStatus2Str(eStateMahineStatus));
                break;
        }

        // hit unexpected state in above switch case
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("GetSessionIDInit failed. hr = 0x%x", hr);
            break;
        }

        if (bInitSuccessful)
        {
            // update cp context in mos cp interface
            pMosCp->UpdateCpContext(m_cpContext.value);
            break;
        }

        if (++uiQuery >= uiMaxQueries)
        {
            hr = E_FAIL;
            PAVP_DEVICE_ASSERTMESSAGE("Pavp set session IN_INIT timeout");
            break;
        }
    } while(TRUE);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDeviceNewSM::CleanupHwSession()
{
    HRESULT             hr                  = S_OK;
    BOOL                bSessionInPlay      = TRUE;
    PXP_SM_STATUS       eSMStatus           = PXP_SM_STATUS_ERROR_UNKNOWN;
    uint32_t            SessionId           = PAVP_INVALID_SESSION_ID;

    PAVP_DEVICE_FUNCTION_ENTER;
    MOS_TraceEventExt(EVENT_CP_DESTROY, EVENT_TYPE_START, &m_cpContext.value, sizeof(uint32_t), nullptr, 0);
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

        if (!IsSessionIdValid())
        {
            PAVP_DEVICE_NORMALMESSAGE("No need to terminate invalid Session ID.");
            break;
        }

        SessionId = m_cpContext.sessionId;
        PAVP_DEVICE_NORMALMESSAGE("Terminating session ID: %d", SessionId);

        BOOL isInitalized = FALSE;
        hr = IsSessionInitialized(isInitalized);
        if (FAILED(hr) || !isInitalized)
        {
            PAVP_DEVICE_ASSERTMESSAGE("PAVP device is not INITALIZED or READY. Do not need to clean up hr=0x%08x, isInitalized=%d",
                hr, isInitalized);
            // clean up context
            m_cpContext.value = 0;
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

        PAVP_DEVICE_NORMALMESSAGE("Requesting KMD to terminate the session...");

        // request to move state to 'PXP_SM_REQ_SESSION_TERMINATE'.
        hr = m_pOsServices->ReqNewPavpSessionState(
                                m_PavpSessionType,
                                m_PavpSessionMode,
                                PXP_SM_REQ_SESSION_TERMINATE,   // What we want.
                                &eSMStatus);                    //status.

        if (FAILED(hr) || eSMStatus != PXP_SM_STATUS_SUCCESS)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Cannot terminate the HW Session.");
            hr = E_UNEXPECTED;
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
        // clean up context
        m_cpContext.value = 0;
    } while (FALSE);

    MOS_TraceEventExt(EVENT_CP_DESTROY, EVENT_TYPE_END, nullptr, 0, nullptr, 0);
    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDeviceNewSM::IsSessionInitialized(BOOL& bIsSessionInitialized)
{
    HRESULT                 hr                      = S_OK;
    bool                    bIsSessionAlive         = false;
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

        //Don't use m_cpcontext as KMD will overwrite it
        CpContext CPctx;
        MOS_ZeroMemory(&CPctx, sizeof(CPctx));
        CPctx.sessionId = m_cpContext.sessionId;

        hr = m_pOsServices->QueryCpContext(&CPctx.value, &bIsSessionAlive);

        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to query whether the session is initialized. Error = 0x%x.", hr);
            break;
        }

        if (bIsSessionAlive)
        {
            if (CPctx.instanceId == m_cpContext.instanceId)
            {
                bIsSessionInitialized = TRUE;
                PAVP_DEVICE_NORMALMESSAGE("The session is alive!");
            }
            else
            {
                bIsSessionInitialized = FALSE;
                PAVP_DEVICE_NORMALMESSAGE("The session is not alive! Tear down happened");
            }
        }
        else
        {
            bIsSessionInitialized = FALSE;
            PAVP_DEVICE_NORMALMESSAGE("The session is not alive!");
        }

    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

CPavpDeviceNewSM::~CPavpDeviceNewSM()
{
    HRESULT hr = S_OK;
    PAVP_DEVICE_FUNCTION_ENTER;

    hr = CleanupHwSession();

    PAVP_DEVICE_FUNCTION_EXIT(0);
}
CPavpDeviceNewSM::CPavpDeviceNewSM() : CPavpDevice()
{
    PAVP_DEVICE_FUNCTION_ENTER;
    PAVP_DEVICE_FUNCTION_EXIT(0);
}

HRESULT CPavpDeviceNewSM::InitHwSession()
{
    HRESULT             hr                          = S_OK;
    INT32               iRetry                      = 0;

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
            hr = GetSessionIDInit(m_PavpSessionType, m_PavpSessionMode);
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
                    m_PavpSessionType != PAVP_SESSION_TYPE_WIDI)
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
            // IsAppIDRegisterMgmtRequired()/SetPavpSessionStatus(READY) before/after GET_STREAM_KEY_EX in CPavpDeviceNewSM::AccessME()
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

    MOS_TraceEventExt(EVENT_CP_CREATE, EVENT_TYPE_END, nullptr, 0, nullptr, 0);
    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDeviceNewSM::InitHwTranscodeSession()
{
    HRESULT                             hr                          = S_OK;
    PAVP_GET_CONNECTION_STATE_PARAMS2   sConnState                  = {0, {0}, {0}};
    PAVP_SET_STREAM_KEY_PARAMS          sSetKeyParams;
    PAVP_PCH_INJECT_KEY_MODE            PchInjectKeyMode            = PAVP_PCH_INJECT_KEY_MODE_S1;
    PXP_SM_STATUS                       eSMStatus                   = PXP_SM_STATUS_ERROR_UNKNOWN;

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

        // Move the session status to 'in play'.
        hr = m_pOsServices->ReqNewPavpSessionState(
            m_PavpSessionType,
            m_PavpSessionMode,
            PXP_SM_REQ_SESSION_IN_PLAY,
            &eSMStatus);
        if (FAILED(hr) || eSMStatus != PXP_SM_STATUS_SUCCESS)
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to set session as InPlay. Error = 0x%x.", hr);
            break;
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

        if (m_pGpuHal->IsPlaneEnablesRequired())
        {
            PAVP_DEVICE_NORMALMESSAGE("Setting plane enables (if necessary).");
            hr = SetPlaneEnables();
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Failed to set plane enables. Error = 0x%x.", hr);
                break;
            }
        }

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

HRESULT CPavpDeviceNewSM::InitHwDecodeSession()
{
    HRESULT                             hr                          = S_OK;
    PAVP_GET_CONNECTION_STATE_PARAMS2   sConnState                  = {0, {0}, {0}};
    PAVP_SET_STREAM_KEY_PARAMS          sSetKeyParams;
    PAVP_PCH_INJECT_KEY_MODE            PchInjectKeyMode            = PAVP_PCH_INJECT_KEY_MODE_S1;
    PAVP_FW_PAVP_MODE                   PavpMode;
    PXP_SM_STATUS                       eSMStatus                   = PXP_SM_STATUS_ERROR_UNKNOWN;

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
                                (uint8_t *)sSetKeyParams.EncryptedEncryptKey, // puiEncryptedE0
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
        if (m_pGpuHal->IsPlaneEnablesRequired())
        {
            PAVP_DEVICE_NORMALMESSAGE("Setting plane enables (if necessary).");
            hr = SetPlaneEnables();
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Failed to set plane enables. Error = 0x%x.", hr);
                break;
            }
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
        hr = m_pOsServices->ReqNewPavpSessionState(
                                    m_PavpSessionType,
                                    m_PavpSessionMode,
                                    PXP_SM_REQ_SESSION_IN_PLAY,
                                    &eSMStatus);
        if (FAILED(hr))
        {
            PAVP_DEVICE_ASSERTMESSAGE("Failed to move session to In_Play. Error = 0x%x.", hr);
            break;
        }

        PAVP_DEVICE_NORMALMESSAGE("PAVP session is initialized.");

    } while (FALSE);
    MOS_TraceEventExt(EVENT_CP_CREATE, EVENT_TYPE_INFO, &m_cpContext.value, sizeof(uint32_t), nullptr, 0);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpDeviceNewSM::SetStreamKey(PAVP_SET_STREAM_KEY_PARAMS& SetKeyParams, bool KeyTranslationRequired /* = true */)
{
    HRESULT                             hr                          = S_OK;
    PAVP_GPU_HAL_KEY_EXCHANGE_PARAMS    SetStreamKeyParams          = { 0, PAVP_KEY_EXCHANGE_TYPE_DECRYPT, { 0 }, { { 0 } } };
    BOOL                                bSetPlaneEnable             = FALSE;

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
                break;

            case PAVP_SET_KEY_ENCRYPT:
                PAVP_DEVICE_NORMALMESSAGE("Updating the 'Encrypt' key.");
                SetStreamKeyParams.KeyExchangeType = PAVP_KEY_EXCHANGE_TYPE_ENCRYPT;
                break;

            case PAVP_SET_KEY_BOTH:
                PAVP_DEVICE_NORMALMESSAGE("Updating both the 'Decrypt' key and the 'Encrypt' key.");
                SetStreamKeyParams.KeyExchangeType = PAVP_KEY_EXCHANGE_TYPE_DECRYPT_ENCRYPT;
                break;

            case PAVP_SET_KEY_WYSIWYS:
                // SetStreamKey() for WYSIWYS should be called only in heavy mode.
                if (IsMemoryEncryptionEnabled())
                {
                    PAVP_DEVICE_NORMALMESSAGE("A WYSIWYS application is updating the 'decrypt' key.");
                    SetStreamKeyParams.KeyExchangeType = PAVP_KEY_EXCHANGE_TYPE_DECRYPT;
                    bSetPlaneEnable  = TRUE;
                }
#if (_DEBUG || _RELEASE_INTERNAL)
                else if (m_bForceLiteModeForDebug)
                {
                    PAVP_DEVICE_ASSERTMESSAGE("A WYSIWYS application is updating the 'decrypt' key, with lite mode for debug.");
                    SetStreamKeyParams.KeyExchangeType = PAVP_KEY_EXCHANGE_TYPE_DECRYPT;
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

        if (bSetPlaneEnable && m_pGpuHal->IsPlaneEnablesRequired())
        {
            // We need to set heavy mode via setplaneenables after sendkeyexchange as
            // The WYSIWYS app has changed the ME/SeC key to S1.
            PAVP_DEVICE_NORMALMESSAGE("Setting Plane enables for a WYSIWYS app");

            hr = SetPlaneEnables();
            if (FAILED(hr))
            {
                PAVP_DEVICE_ASSERTMESSAGE("Failed to set plane enables. Error = 0x%x.", hr);
                break;
            }
        }
    } while (FALSE);

    PAVP_DEVICE_FUNCTION_EXIT(hr);
    return hr;
}

bool CPavpDeviceNewSM::NeedsASMFTranslation()
{
    // if m_bSkipAsmf is set to TRUE, it implies app is using Get key blob API and therefore key translation is not needed
    bool bDrmSkipAsmfTranslation = false;
    HRESULT hr = S_OK;

    PAVP_DEVICE_FUNCTION_ENTER;

    // Modern DRMs like PlayReady, Widevine and Miracast don't require translation, but others do.
    bDrmSkipAsmfTranslation = ((m_eDrmType == PAVP_DRM_TYPE_PLAYREADY) ||
                               (m_eDrmType == PAVP_DRM_TYPE_WIDEVINE) ||
                               (m_eDrmType == PAVP_DRM_TYPE_MIRACAST_RX) ||
                               (m_bSkipAsmf == TRUE));

    PAVP_DEVICE_VERBOSEMESSAGE("bDrmSkipAsmfTranslation=[%d] m_eDrmType=[%d] ", bDrmSkipAsmfTranslation, m_eDrmType);
    PAVP_DEVICE_FUNCTION_EXIT(hr);

    return !bDrmSkipAsmfTranslation;
}

HRESULT CPavpDeviceNewSM::AccessME(
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
    uint8_t                     IgdData[FIRMWARE_MAX_IGD_DATA_SIZE]; // Local buffer for IGD-ME private data.
    uint32_t                    IgdDataLen                  = sizeof(IgdData);
    PAVP_SET_STREAM_KEY_PARAMS  SetKeyParams;

    PAVP_DEVICE_FUNCTION_ENTER;

    do
    {
        MOS_ZeroMemory(IgdData, IgdDataLen);
        MOS_UNUSED(bIsWidiMsg);

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
                            false, //wi-di support not present in Linux stack
                            vTag);
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

                if (pHeader->ApiVersion >= FIRMWARE_API_VERSION_4_3 && IgdDataLen == 2 * PAVP_SIGMA_STREAM_KEY_MAX_LEN  && (m_PavpSessionType == PAVP_SESSION_TYPE_TRANSCODE)) 
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
        else if (pHeader->PavpCmdStatus != PAVP_STATUS_SUCCESS)
        {
            PAVP_DEVICE_ASSERTMESSAGE("RootTrust passthrough call returned ME status failure: 0x%x.", pHeader->PavpCmdStatus);
            break;
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

HRESULT CPavpDeviceNewSM::PowerUpInit()
{
    return E_NOTIMPL;
}

HRESULT CPavpDeviceNewSM::TerminateSession(uint32_t SessionId, BOOL CheckSessionInPlay)
{
    return E_NOTIMPL;
}

HRESULT CPavpDeviceNewSM::ReserveSessionId(
    PAVP_SESSION_TYPE       eSsessionType,
    PAVP_SESSION_MODE       eSsessionMode,
    PAVP_SESSION_STATUS*    pPavpSessionStatus,
    PAVP_SESSION_STATUS*    pPreviousPavpSessionStatus)
{
    return GetSessionIDInit(eSsessionType, eSsessionMode);
}

HRESULT CPavpDeviceNewSM::GlobalTerminateDecode()
{
    return E_NOTIMPL;
}

HRESULT CPavpDeviceNewSM::IsSessionReady(BOOL& bIsSessionReady)
{
    return E_NOTIMPL;
}

HRESULT CPavpDeviceNewSM::OpenArbitratorSession()
{
    return E_NOTIMPL;
}

HRESULT CPavpDeviceNewSM::InitiateArbitratorSession(bool bPreserveOriginalState)
{
    return E_NOTIMPL;
}
