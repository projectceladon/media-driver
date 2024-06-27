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

File Name: cp_hal_pch_unified.cpp

Abstract:
    Implements a class to supply access to the PCH. This is a parallel class to the already avialble one implementing hecisession
    and removing the need to communicate with pavp DLL. As a  first step this iplementation will be only called for Windows and
    then expanded to Android leading to removing the cp_pch_hal.cpp. The switch between to the 2 routes performed in pavpdevice

Environment:
    Windows, Linux, Android

Notes:
    None

======================= end_copyright_notice ==================================*/

#include <cstring>
#include <fstream>
#include <new>
#include "cp_hal_pch_unified.h"
#include "cp_debug.h"
#include "intel_pavp_heci_api.h"
#include "mos_utilities.h"
#include "mos_util_user_interface.h"
#include "intel_pavp_core_api.h"
#include "intel_pavp_pr_api.h"
#include "cp_os_services.h"
#include "mos_gfxinfo_id.h"

#ifndef E_UNEXPECTED
#define E_UNEXPECTED 0x8000FFFFL
#endif

GUID PAVP_GUID    = {0xfbf6fcf1, 0x96cf, 0x4e2e, {0xa6, 0xa6, 0x1b, 0xab, 0x8c, 0xbe, 0x36, 0xb1}};
GUID WIDI_GUID    = {0xb638ab7e, 0x94e2, 0x4ea2, {0xa5, 0x52, 0xd1, 0xc5, 0x4b, 0x62, 0x7f, 0x04}};

#define SUSPENDRESUME_CLEANUPINIT_MAX_RETRY 15

CPavpPchHalUnified::CPavpPchHalUnified(RootTrust_HAL_USE ePchHalUse, std::shared_ptr<CPavpOsServices> pOsServices, bool bEPIDProvisioningMode) :
    m_ePchHalUse(ePchHalUse), m_EPIDProvisioningMode (bEPIDProvisioningMode), m_pOsServices(pOsServices)
{
    MediaUserSettingSharedPtr   userSettingPtr = nullptr;

    m_IsWidiEnabled = (ePchHalUse == RootTrust_HAL_WIDI_USE) ? TRUE : FALSE;
    if (m_pOsServices && m_pOsServices->m_pOsInterface)
    {
        userSettingPtr = m_pOsServices->m_pOsInterface->pfnGetUserSettingInstance(m_pOsServices->m_pOsInterface);
    }
    MosUtilities::MosUtilitiesInit(userSettingPtr);
    PLATFORM sPlatform = { IGFX_UNKNOWN };
    if(nullptr != m_pOsServices)
    {
        m_pOsServices->GetPlatform(sPlatform);
    }
    else
    {
        PCH_HAL_ASSERTMESSAGE("Os Service is NULL");
        MT_ERR1(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__);
    }
    ResetMemberVariables();
    m_SuspendResumeInitRetry = 0;
}

CPavpPchHalUnified::~CPavpPchHalUnified()
{
    MediaUserSettingSharedPtr   userSettingPtr  = nullptr;
    if (m_pOsServices && m_pOsServices->m_pOsInterface)
    {
        userSettingPtr  = m_pOsServices->m_pOsInterface->pfnGetUserSettingInstance(m_pOsServices->m_pOsInterface);
    }
    Cleanup();
    // Don't reset this variable to zero in Cleanup() or anywhere else otherwise execution can go in infinite loop
    // as this variable is used to track number of Cleanup() Init() retries after suspend resume.
    m_SuspendResumeInitRetry        = 0;
    MosUtilities::MosUtilitiesClose(userSettingPtr);
}

void CPavpPchHalUnified::ResetMemberVariables()
{
    // only reset dynamic class member, static info like m_ePchHalUse 
    // should not reset, these info are required to survive across class cleanup and init.
    m_uiFWApiVersion                = FIRMWARE_API_VERSION_2;
    m_Initialized                   = FALSE;        
    m_uiStreamId                    = PAVP_INVALID_SESSION_ID;
    m_bFwFallback                   = FALSE;
    m_hPavpSession                  = NULL;
    m_hWiDiSession                  = NULL;
    m_KeyBlobForINIT43.SubSessionId = 0;
    m_KeyBlobForINIT43.InitFlags    = {0};
    m_AuthenticatedKernelTrans      = FALSE;

    // clear all firmware cap bit.
    m_eFwCaps.reset();
}

HRESULT CPavpPchHalUnified::Init()
{
    HRESULT             hr                    = S_OK;
        
    PCH_HAL_FUNCTION_ENTER;

    do
    {
        if (m_Initialized)
        {
            PCH_HAL_NORMALMESSAGE("Already Initialized.");
            break;
        }
        
        if (m_ePchHalUse == RootTrust_HAL_WIDI_USE)
        {
            PCH_HAL_NORMALMESSAGE("Session Init with WiDi Guid..", hr);
            hr = SessionInit(&WIDI_GUID);
        }
        else
        {
            hr = SessionInit(&PAVP_GUID);
        }        
        if (FAILED(hr))
        {
            PCH_HAL_ASSERTMESSAGE("Init Failed Session Creation");            
            MT_ERR1(MT_CP_SESSION_INIT, MT_ERROR_CODE, hr);
            break;
        }
        
        hr = GetFWApiVersionFromPCH(&m_uiFWApiVersion);
        if (FAILED(hr))
        {
            PCH_HAL_ASSERTMESSAGE("Could not find FW API version. Error = 0x%x.", hr);
            MT_ERR1(MT_CP_HAL_FW_RULE, MT_ERROR_CODE, hr);
            break;
        }

        switch (m_uiFWApiVersion)
        {
            case FIRMWARE_API_VERSION_4_3 :
                m_eFwCaps.set(FIRMWARE_CAPS_PAVP_INIT43);
            case FIRMWARE_API_VERSION_4_2 :
                m_eFwCaps.set(FIRMWARE_CAPS_ONDIECA);
            case FIRMWARE_API_VERSION_4_1 :
                m_eFwCaps.set(FIRMWARE_CAPS_PAVP_UNIFIED_INIT);
            case FIRMWARE_API_VERSION_4 :
                m_eFwCaps.set(FIRMWARE_CAPS_PAVP_INIT);
            case FIRMWARE_API_VERSION_32 :
                m_eFwCaps.set(FIRMWARE_CAPS_MULTI_SESSION);
            default :
                break;
        }

        PCH_HAL_NORMALMESSAGE("Init successful. FW API version is 0x%.8x", m_uiFWApiVersion);
        m_Initialized = TRUE;
        
    } while (FALSE);

    if (FAILED(hr))
    {
        Cleanup();        
    }
    
    PCH_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpPchHalUnified::Cleanup()
{
    HRESULT             hr              = S_OK;
    
    PCH_HAL_FUNCTION_ENTER;

    do
    {
        // Do not invalidate the slot by default here.
        // The fw slot needs to track the actual session life in the GPU,
        // so let the owning PavpDevice clear the slot at the same time
        // the GPU slot is cleared.

        hr = SessionCleanup();
        if (FAILED(hr))
        {             
            PCH_HAL_ASSERTMESSAGE("Failed to clean up Session , Error = 0x%x.", hr);                
            MT_ERR2(MT_CP_SESSION_CLEANUP, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
        }

    } while (FALSE);
    ResetMemberVariables();

    PCH_HAL_FUNCTION_EXIT(hr);
    return hr;   
}

HRESULT CPavpPchHalUnified::SendRecvCmd(
    PVOID   pIn,
    ULONG   inSize,
    PVOID   pOut,
    ULONG   outSize)
{
    HRESULT     hr     = S_OK;
    PAVPCmdHdr* pInHdr = reinterpret_cast<PAVPCmdHdr *>(pIn);

    PCH_HAL_FUNCTION_ENTER;

    do
    {
        // Validate input parameters.
        if (pIn     == NULL ||
            pOut    == NULL ||
            inSize  == 0    ||  // Cannot be negative.
            outSize == 0)       // Cannot be negative.
        {
            PCH_HAL_ASSERTMESSAGE("Invalid parameter.");
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        pInHdr->BufferLength  = inSize - sizeof(PAVPCmdHdr);

        PCH_HAL_ASSERT(inSize >= sizeof(PAVPCmdHdr)); // Input buffers should always at least include a header.
       
        PCH_HAL_VERBOSEMESSAGE(
                "Printing FW command before calling m_pfnStartIO: "
                "Opcode %x ApiVersion %x Buffer length %x Command status %x" ,
                pInHdr->CommandId, pInHdr->ApiVersion, pInHdr->BufferLength,
                pInHdr->PavpCmdStatus);

        hr = IoMessage(
                        static_cast<PBYTE>(pIn),
                        inSize,
                        static_cast<PBYTE>(pOut),
                        outSize,
                        NULL,
                        NULL);

        PCH_HAL_VERBOSEMESSAGE(
                "Printing FW command after calling StartIO: "
                "Opcode %x ApiVersion %x Buffer length %x Command status %x" ,
                pInHdr->CommandId, pInHdr->ApiVersion, pInHdr->BufferLength,
                pInHdr->PavpCmdStatus);

        if (FAILED(hr))
        {
            if (hr == PAVP_STATUS_HECI_COMMUNICATION_ERROR)
            {
                // On Suspend-resume, the HECI driver may take up to 1.5 sec to come alive.
                // if pavp create call in this time window, it will fail. 
                // reconnect to heci service in this case, so that the next call could succeed.
                // pavp device class will do the retry if it is necessary.
                // the reconnect should not apply to IoMessage, which is used in AccessME, KeyExchange.
                // KeyExchange path will recreate crypto session if this error detected.
                // Shorten the sleep interval to 100ms to ensure every call can return within 3 second in DDI level.
                if (m_SuspendResumeInitRetry <= SUSPENDRESUME_CLEANUPINIT_MAX_RETRY)
                {
                    MosUtilities::MosSleep(100);
                    m_SuspendResumeInitRetry++;
                    Cleanup();
                    Init();
                }
                else
                {
                    PCH_HAL_ASSERTMESSAGE(
                        "Maximum number of retries for Cleanup()-Init() after suspend-resume has been completed without success");
                    MT_ERR1(MT_CP_RETRY_FAIL, MT_CODE_LINE, __LINE__);
                }
            }
            else
            {
                uint32_t uHeaderStatus = (reinterpret_cast<PAVPCmdHdr *>(pOut))->PavpCmdStatus;
                PCH_HAL_ASSERTMESSAGE(
                    "Failed sending FW CMD. "
                    "Opcode %x status %x Header status %x",
                    pInHdr->CommandId, hr, uHeaderStatus);
                MT_ERR3(MT_CP_CMD_SEND_FAIL, MT_CP_COMMAND_ID, pInHdr->CommandId, MT_ERROR_CODE, uHeaderStatus, MT_FUNC_RET, hr);
            }
        }
    } while (FALSE);

    PCH_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpPchHalUnified::InvalidateSlot(uint32_t streamId)
{
    HRESULT         hr          = S_OK;
    SetSlotInBuff   sSetSlotIn  = {{0,0,0,0},0};
    SetSlotOutBuff  sSetSlotOut = {{0,0,0,0}};

    PCH_HAL_FUNCTION_ENTER;
    PCH_HAL_VERBOSEMESSAGE("streamId: 0x%x.", streamId);

    do
    {
        if (!m_Initialized)
        {
            PCH_HAL_ASSERTMESSAGE("Error, not initialized.");
            hr = E_FAIL;
            MT_ERR1(MT_CP_HAL_NOT_INITIALIZED, MT_ERROR_CODE, hr);
            break;
        }

        // Safety net, just in case
        if (streamId == PAVP_INVALID_SESSION_ID)
        {
            PCH_HAL_ASSERTMESSAGE("Error, Invalid Stream ID 0x%x", streamId);
            hr = E_FAIL;
            MT_ERR3(MT_ERR_INVALID_ARG, MT_CP_STREAM_ID, streamId, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        sSetSlotIn.Header.ApiVersion = m_uiFWApiVersion;
        sSetSlotIn.PavpSlot          = streamId;

        switch (m_uiFWApiVersion)
        {
            case FIRMWARE_API_VERSION_32:
                sSetSlotIn.Header.CommandId  = FIRMWARE_CMD_INVALIDATE_PAVP_SLOT;
                hr = SendRecvCmd(
                    &sSetSlotIn,
                    sizeof(sSetSlotIn),
                    &sSetSlotOut,
                    sizeof(sSetSlotOut));
                break;
            case FIRMWARE_API_VERSION_4:
                sSetSlotIn.Header.CommandId  = FIRMWARE_CMD_INVALIDATE_PAVP_SLOT4;
                hr = SendRecvCmd(
                    &sSetSlotIn,
                    sizeof(sSetSlotIn),
                    &sSetSlotOut,
                    sizeof(sSetSlotOut));
                 break;
            default:       
                PCH_HAL_ASSERTMESSAGE("No invalidate slot for FW version: 0x%x", m_uiFWApiVersion);
                MT_ERR1(MT_CP_INVALID_SLOT, MT_CP_FW_API_VERSION, m_uiFWApiVersion);
                hr = S_OK;
                break;
        }

        if (FAILED(hr))
        {
            PCH_HAL_ASSERTMESSAGE("Error sending INVALIDATE SLOT command. Error = 0x%x.", hr);
            MT_ERR2(MT_CP_CMD_SEND_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        // Success if we reach here

        if (streamId == m_uiStreamId)
        {
            // This call is cleaning up the session associated with the owning
            // CryptoSession (as opposed to something cleaning up an ungracefully
            // terminated slot, or a teardown recovery that would clean up
            // every slot).
            // So reset the variables for it.
            m_uiStreamId = PAVP_INVALID_SESSION_ID;
        }

    } while (FALSE);

    PCH_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpPchHalUnified::InvalidateStreamKey(uint32_t streamId)
{
    HRESULT             hr              = S_OK;
    InvStreamKeyInBuff  InvStreamKeyIn  = {{0, 0, 0, 0}, 0, 0, 0};
    InvStreamKeyOutBuff InvStreamKeyOut = {{0, 0, 0, 0}, 0};

    PCH_HAL_FUNCTION_ENTER;
    PCH_HAL_VERBOSEMESSAGE("streamId: 0x%x.", streamId);

    do
    {
        if (!m_Initialized)
        {
            PCH_HAL_ASSERTMESSAGE("Error, not initialized.");
            hr = E_FAIL;
            MT_ERR1(MT_CP_HAL_NOT_INITIALIZED, MT_ERROR_CODE, hr);
            break;
        }

        // Safety net, just in case
        if (streamId == PAVP_INVALID_SESSION_ID)
        {
            PCH_HAL_ASSERTMESSAGE("Error, Invalid Stream ID 0x%x", streamId);
            hr = E_FAIL;
            MT_ERR3(MT_ERR_INVALID_ARG, MT_CP_STREAM_ID, streamId, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }
        InvStreamKeyIn.Header.ApiVersion   = m_uiFWApiVersion;
        InvStreamKeyIn.Header.BufferLength = sizeof(InvStreamKeyIn) - sizeof(InvStreamKeyIn.Header);
        InvStreamKeyIn.StreamId            = streamId;
        InvStreamKeyIn.Header.CommandId    =  FIRMWARE_CMD_INV_STREAM_KEY;
        hr = SendRecvCmd(
            &InvStreamKeyIn,
            sizeof(InvStreamKeyIn),
            &InvStreamKeyOut,
            sizeof(InvStreamKeyOut));
        if (FAILED(hr))
        {
            PCH_HAL_ASSERTMESSAGE("Error sending INV_STREAM_KEY command. Error = 0x%x.", hr);
            MT_ERR2(MT_CP_CMD_SEND_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }
    } while (FALSE);

    PCH_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpPchHalUnified::SetSlot(uint32_t uiStreamId)
{
    HRESULT         hr          = S_OK;
    SetSlotInBuff   sSetSlotIn  = {{0, 0, 0, 0}, 0};
    SetSlotOutBuff  sSetSlotOut = {{0, 0, 0, 0}};

    PCH_HAL_FUNCTION_ENTER;

    PCH_HAL_VERBOSEMESSAGE("streamId: 0x%x.", uiStreamId);
    
    do
    {
        if (!m_Initialized)
        {
            PCH_HAL_ASSERTMESSAGE("Error, not initialized.");
            hr = E_FAIL;
            MT_ERR1(MT_CP_HAL_NOT_INITIALIZED, MT_ERROR_CODE, hr);
            break;
        }
        
        if (uiStreamId == PAVP_INVALID_SESSION_ID)
        {
            PCH_HAL_ASSERTMESSAGE("Error, Invalid Stream ID 0x%x", m_uiStreamId);
            hr = E_FAIL;
            MT_ERR3(MT_ERR_INVALID_ARG, MT_CP_STREAM_ID, m_uiStreamId, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        m_uiStreamId        = uiStreamId;
        sSetSlotIn.Header.ApiVersion = m_uiFWApiVersion;
        sSetSlotIn.PavpSlot = m_uiStreamId;

        switch (m_uiFWApiVersion)
        {
            case FIRMWARE_API_VERSION_32:
                sSetSlotIn.Header.CommandId  = FIRMWARE_CMD_SET_PAVP_SLOT_EX;
                hr = SendRecvCmd(
                    &sSetSlotIn,
                    sizeof(sSetSlotIn),
                    &sSetSlotOut,
                    sizeof(sSetSlotOut));
                break;
            case FIRMWARE_API_VERSION_3:
                sSetSlotIn.Header.CommandId  = FIRMWARE_CMD_SET_PAVP_SLOT;
                hr = SendRecvCmd(
                    &sSetSlotIn,
                    sizeof(sSetSlotIn),
                    &sSetSlotOut,
                    sizeof(sSetSlotOut));
                break;
            default:
                PCH_HAL_ASSERTMESSAGE("Unexpected FW version: got 0x%x", m_uiFWApiVersion);
                hr = E_UNEXPECTED;
                MT_ERR2(MT_CP_HAL_FW_RULE, MT_CP_FW_API_VERSION, m_uiFWApiVersion, MT_ERROR_CODE, hr);
                break;
        }

        if (FAILED(hr))
        {
            PCH_HAL_ASSERTMESSAGE("Error sending SET SLOT command. Error = 0x%x.", hr);
            MT_ERR2(MT_CP_CMD_SEND_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }
    } while (FALSE);

    PCH_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpPchHalUnified::EnableLiteModeHucStreamOutBit()
{
    HRESULT                                hr = S_OK;
    PavpCmdDbgInjectCharacteristicRegExIn  sPavpCmdDbgInjectCharacteristicRegExIn = {{ 0, 0, 0, 0 }, { 0 }, { 0 }};
    PavpCmdDbgInjectCharacteristicRegExOut sPavpCmdDbgInjectCharacteristicRegExOut = { { 0, 0, 0, 0 }};

    PCH_HAL_FUNCTION_ENTER;

    do
    {
        sPavpCmdDbgInjectCharacteristicRegExIn.Header.ApiVersion = m_uiFWApiVersion;
        sPavpCmdDbgInjectCharacteristicRegExIn.Header.CommandId = PAVP_CMD_ID_DBG_INJECT_CHARACTERISTIC_REG_EX;
        sPavpCmdDbgInjectCharacteristicRegExIn.Header.BufferLength = sizeof(sPavpCmdDbgInjectCharacteristicRegExIn) - sizeof(PAVPCmdHdr);
        sPavpCmdDbgInjectCharacteristicRegExIn.Header.PavpCmdStatus = PAVP_STATUS_SUCCESS;
        //SecurePAVPCharacteristicRegsiter DW1 bit 13 is used to programming huc streamout bit
        sPavpCmdDbgInjectCharacteristicRegExIn.Mask[1] = 0x2000;
        sPavpCmdDbgInjectCharacteristicRegExIn.CharacteristicReg[1] = 0x2000;
        
        hr = SendRecvCmd(
            &sPavpCmdDbgInjectCharacteristicRegExIn,
            sizeof(sPavpCmdDbgInjectCharacteristicRegExIn),
            &sPavpCmdDbgInjectCharacteristicRegExOut,
            sizeof(sPavpCmdDbgInjectCharacteristicRegExOut));

        if (FAILED(hr))
        {
            PCH_HAL_ASSERTMESSAGE("Error sending PavpCmdDbgInjectCharacteristicRegExIn command, Command Id 0x%x, Error = 0x%x.", sPavpCmdDbgInjectCharacteristicRegExIn.Header.CommandId, hr);
            MT_ERR2(MT_CP_CMD_SEND_FAIL, MT_CP_COMMAND_ID, sPavpCmdDbgInjectCharacteristicRegExIn.Header.CommandId, MT_ERROR_CODE, hr);
            break;
        }
        else if (sPavpCmdDbgInjectCharacteristicRegExOut.Header.PavpCmdStatus != PAVP_STATUS_SUCCESS)
        {
            hr = E_ABORT;
            PCH_HAL_ASSERTMESSAGE("PavpCmdDbgInjectCharacteristicRegExIn FW report error (0x%x).", sPavpCmdDbgInjectCharacteristicRegExOut.Header.PavpCmdStatus);
            MT_ERR2(MT_CP_HAL_STATUS_CHECK, MT_ERROR_CODE, sPavpCmdDbgInjectCharacteristicRegExOut.Header.PavpCmdStatus, MT_FUNC_RET, hr);
            break;
        }
    } while (FALSE);

    PCH_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpPchHalUnified::PavpInit4(
    PAVP_FW_PAVP_MODE uiPavpMode,
    uint32_t uiAppId)
{
    HRESULT             hr                      = S_OK;
    PavpInit4InBuff     sPavpInit4In            = {{0, 0, 0, 0}, 0, 0};
    PavpInit4OutBuff    sPavpInit4Out           = {{0, 0, 0, 0}};
    PavpInit43InBuff    sPavpInit43In           = { {0, 0, 0, 0}, 0, 0, 0, {0} };
    PavpInit43Flags     sPavpInit43Flags        = { 0 };
    PavpInit43OutBuff   sPavpInit43Out          = { {0, 0, 0, 0}, {0} };
    uint32_t              bIsTranscode            = uiAppId & TRANSCODE_APP_ID_MASK;
    PAVPCmdHdr         *pInBuffHdr              = nullptr;
    PAVPCmdHdr         *pOutBuffHdr             = nullptr;

    PCH_HAL_FUNCTION_ENTER;

    PCH_HAL_VERBOSEMESSAGE("AppID: 0x%x, PavpMode: 0x%x, isTranscode: 0x%x.", uiAppId, uiPavpMode, bIsTranscode);

    do
    {
        if (!m_Initialized)
        {
            PCH_HAL_ASSERTMESSAGE("Error, not initialized.");
            hr = E_FAIL;
            MT_ERR1(MT_CP_HAL_NOT_INITIALIZED, MT_ERROR_CODE, hr);
            break;
        }
        
        if (uiAppId == PAVP_INVALID_SESSION_ID)
        {
            PCH_HAL_ASSERTMESSAGE("Error, Invalid Stream ID 0x%x", uiAppId);
            hr = E_FAIL;
            MT_ERR3(MT_ERR_INVALID_ARG, MT_CP_STREAM_ID, uiAppId, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        m_uiStreamId = uiAppId;

        switch (m_uiFWApiVersion)
        {
            case FIRMWARE_API_VERSION_4_3:
            {
                if (bIsTranscode && (uiPavpMode == PAVP_FW_MODE_STOUT))
                {
                    sPavpInit43Flags.Value = m_KeyBlobForINIT43.InitFlags.Value;
                }
                else
                {
                    sPavpInit43Flags.SubSessionEnabled = 0;
                    sPavpInit43Flags.DisableHuc = 0;
                }

                sPavpInit43In.Header.ApiVersion = m_uiFWApiVersion;
                sPavpInit43In.Header.CommandId = FIRMWARE_43_CMD_PAVP_INIT;
                sPavpInit43In.Header.BufferLength = sizeof(PavpInit43InBuff) - sizeof(PAVPCmdHdr);
                sPavpInit43In.Header.PavpCmdStatus = PAVP_STATUS_SUCCESS;
                sPavpInit43In.PavpMode = uiPavpMode;
                sPavpInit43In.InitFlags = sPavpInit43Flags;
                pInBuffHdr = reinterpret_cast<PAVPCmdHdr*>(&sPavpInit43In);
                pOutBuffHdr = reinterpret_cast<PAVPCmdHdr*>(&sPavpInit43Out);

                if (sPavpInit43Flags.SubSessionEnabled != 0)
                {
                    MOS_SecureMemcpy(
                        sPavpInit43In.Signature,
                        sizeof(sPavpInit43In.Signature),
                        &m_KeyBlobForINIT43.Signature[0],
                        sizeof(m_KeyBlobForINIT43.Signature));
                    sPavpInit43In.SubSessionId = m_KeyBlobForINIT43.SubSessionId;
                }

                hr = SendRecvCmd(
                    &sPavpInit43In,
                    sizeof(sPavpInit43In),
                    &sPavpInit43Out,
                    sizeof(sPavpInit43Out));
                break;
            }
            default:
            {
                sPavpInit4In.Header.ApiVersion = m_uiFWApiVersion;
                sPavpInit4In.Header.CommandId = FIRMWARE_CMD_PAVP_INIT4;
                sPavpInit4In.PavpMode = uiPavpMode;
                sPavpInit4In.AppID = uiAppId;
                pInBuffHdr = reinterpret_cast<PAVPCmdHdr*>(&sPavpInit4In);
                pOutBuffHdr = reinterpret_cast<PAVPCmdHdr*>(&sPavpInit4Out);

                hr = SendRecvCmd(
                    &sPavpInit4In,
                    sizeof(sPavpInit4In),
                    &sPavpInit4Out,
                    sizeof(sPavpInit4Out));
                break;
            }
        }

        if (FAILED(hr))
        {
            PCH_HAL_ASSERTMESSAGE("Error sending PAVP INIT command, Command Id 0x%x, Error = 0x%x.", pInBuffHdr->CommandId, hr);
            MT_ERR2(MT_CP_INIT_FW, MT_CP_COMMAND_ID, pInBuffHdr->CommandId, MT_FUNC_RET, hr);
            break;
        }
        else if (pOutBuffHdr->PavpCmdStatus != PAVP_STATUS_SUCCESS)
        {
            hr = E_ABORT;
            PCH_HAL_ASSERTMESSAGE("PAVP INIT FW report error (0x%x).", pOutBuffHdr->PavpCmdStatus);
            MT_ERR2(MT_CP_INIT_FW, MT_ERROR_CODE, pOutBuffHdr->PavpCmdStatus, MT_FUNC_RET, hr);
            break;
        }
        else if (m_uiFWApiVersion == FIRMWARE_API_VERSION_4_3)
        {
            MOS_SecureMemcpy(
                &m_ResponseForINIT43,
                sizeof(PavpInit43OutBuff),
                &sPavpInit43Out,
                sizeof(PavpInit43OutBuff));
        }
    } while (FALSE);

    PCH_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpPchHalUnified::GetFWApiVersionFromPCH(uint32_t *puiVersion)
{
    HRESULT                     hr = S_OK;
    GetCapsInBuff               sGetCapsIn    = {{0, 0, 0, 0}};
    GetCapsOutBuff              sGetCapsOut   = {{0, 0, 0, 0}, {PAVP_ENCRYPTION_NONE, PAVP_COUNTER_TYPE_C, PAVP_LITTLE_ENDIAN, 0, {0 }}};
    GetCapsInBuff42             sGetCapsIn42  = {{0, 0, 0, 0}};
    GetCapsOutBuff42            sGetCapsOut42 = {0};
    CheckVersionSupportInBuff   ChkVersionIn  = {{0, 0, 0, 0}};
    CheckVersionSupportOutBuff  ChkVersionOut = {{0, 0, 0, 0}};

    PCH_HAL_FUNCTION_ENTER;

    do
    {
        // Validate input parameters.
        if (puiVersion == NULL)
        {
            PCH_HAL_ASSERTMESSAGE("Invalid parameter.");
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        sGetCapsIn42.Header.ApiVersion = FIRMWARE_API_VERSION_4_2;
        sGetCapsIn42.Header.CommandId  = FIRMWARE_42_CMD_GET_PCH_CAPABILITIES;  //pavp_42_get_capabilities

        // First, test if fw supports the desired version
        hr = SendRecvCmd(
            &sGetCapsIn42,
            sizeof(sGetCapsIn42),
            &sGetCapsOut42,
            sizeof(sGetCapsOut42));

        if ((SUCCEEDED(hr)) &&
            (sGetCapsOut42.Header.PavpCmdStatus == PAVP_STATUS_SUCCESS) &&
            (sGetCapsOut42.Header.BufferLength == sizeof(GetCapsOutBuff42) - sizeof(PAVPCmdHdr)))
        {
            *puiVersion = sGetCapsOut42.Caps.PAVPMaxVersion;
            PCH_HAL_VERBOSEMESSAGE("FW API version: 0x%.8x.", *puiVersion);
            hr = S_OK;
            break;
        }

        // If the call did not succeed, fallback to legacy check
        // GetPchCaps will fail if a fw version outside of 2-3.0 is passed in.
        sGetCapsIn.Header.ApiVersion = FIRMWARE_API_VERSION_2;
        sGetCapsIn.Header.CommandId  = FIRMWARE_CMD_GET_PCH_CAPS;

        hr = SendRecvCmd(
            &sGetCapsIn,
            sizeof(sGetCapsIn),
            &sGetCapsOut,
            sizeof(sGetCapsOut));
        if (FAILED(hr))
        {
            PCH_HAL_ASSERTMESSAGE("Error sending GET PCH CAPS command. Error = 0x%x.", hr);
            hr = E_FAIL;
            MT_ERR2(MT_CP_CMD_SEND_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        if (sGetCapsOut.Header.PavpCmdStatus == PAVP_STATUS_SUCCESS)
        {
            *puiVersion = sGetCapsOut.Caps.PAVPVersion;

            //fall back to 4.1 if we used FW4.1 get caps command
            if (*puiVersion >= FIRMWARE_API_VERSION_4_2)
            {
                *puiVersion = FIRMWARE_API_VERSION_4_1;
            }

            PCH_HAL_VERBOSEMESSAGE("FW API version received: 0x%.8x.", *puiVersion);
            hr = S_OK;
            break;
        }

        // If the call did not succeed, fallback to legacy check
        // Keep FIRMWARE_CMD_CHECK_PCH_SUPPORT just to avoid risk that some old FW doesn't support FIRMWARE_CMD_GET_PCH_CAPS
        // FIRMWARE_CMD_CHECK_PCH_SUPPORT only check whether FW support 3.2 API, not return real FW version
        ChkVersionIn.Header.ApiVersion = FIRMWARE_API_VERSION_32;
        ChkVersionIn.Header.CommandId  = FIRMWARE_CMD_CHECK_PCH_SUPPORT;

        hr = SendRecvCmd(
            &ChkVersionIn,
            sizeof(ChkVersionIn),
            &ChkVersionOut,
            sizeof(ChkVersionOut));
        if (FAILED(hr))
        {
            *puiVersion = FIRMWARE_API_VERSION_2;
            PCH_HAL_ASSERTMESSAGE("Error sending CHECK PCH SUPPORT command. Error = 0x%x.", hr);
            hr = E_FAIL;
            MT_ERR2(MT_CP_CMD_SEND_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        if (ChkVersionOut.Header.PavpCmdStatus == PAVP_STATUS_SUCCESS)
        {
            *puiVersion = FIRMWARE_API_VERSION_32;
            PCH_HAL_VERBOSEMESSAGE("FW API version: 0x%.8x.", *puiVersion);
            hr = S_OK;
            break;
        }

    } while (FALSE);

    if (puiVersion != NULL &&
        *puiVersion > FIRMWARE_API_VERSION_3 &&
        m_bFwFallback)
    {
        *puiVersion = FIRMWARE_API_VERSION_3;
        PCH_HAL_VERBOSEMESSAGE("FW v3+ detected and FwFallback enabled, Falling back to FW API version: 0x%.8x.", *puiVersion);
    }

    PCH_HAL_FUNCTION_EXIT(hr);
    return hr;
}

bool CPavpPchHalUnified::IsKernelAuthenticationSupported()
{
    PCH_HAL_FUNCTION_ENTER;
    PCH_HAL_FUNCTION_EXIT(m_AuthenticatedKernelTrans);
    return m_AuthenticatedKernelTrans;
}

bool CPavpPchHalUnified::HasFwCaps(FIRMWARE_CAPS eFwCaps)
{
    bool bRet = FALSE;
    PCH_HAL_FUNCTION_ENTER;

    do
    {
        if (eFwCaps > FIRMWARE_CAPS_MAX)
        {
            PCH_HAL_ASSERTMESSAGE("Invalid firmware capability requested %d", eFwCaps);
            MT_ERR2(MT_CP_HAL_FW_RULE, MT_CP_FW_CAPABILITY, eFwCaps, MT_CODE_LINE, __LINE__);
            break;
        }
        bRet = m_eFwCaps.test(eFwCaps);
    } while (false);

    PCH_HAL_FUNCTION_EXIT(bRet);
    return bRet;
}

HRESULT CPavpPchHalUnified::SetFwFallback(bool bFwFallback)
{
    PCH_HAL_FUNCTION_ENTER;

    m_bFwFallback = bFwFallback;
    if (m_bFwFallback && (m_uiFWApiVersion > FIRMWARE_API_VERSION_3))
    {
        PCH_HAL_VERBOSEMESSAGE("FW version was 0x%x, reducing to FIRMWARE_API_VERSION_3.", m_uiFWApiVersion);
        // reduce firmware version, but not reduce firmware capability.
        m_uiFWApiVersion = FIRMWARE_API_VERSION_3;
        m_eFwCaps.reset();
    }
    PCH_HAL_FUNCTION_EXIT(0);
    return S_OK;
}

HRESULT CPavpPchHalUnified::GetPlaneEnables(
    uint8_t     uiMemoryMode,
    uint16_t    uiPlaneEnables,
    uint32_t    uiPlanesEnableSize,
    PBYTE       pbEncPlaneEnables)
{
    HRESULT                 hr              = S_OK;
    MOS_STATUS              eStatus         = MOS_STATUS_SUCCESS;
    GetPlaneEnableOutBuff   sGetPlaneOut    = {{0, 0, 0, 0}, {0}};
    
    PCH_HAL_FUNCTION_ENTER;
    
    PCH_HAL_NORMALMESSAGE("planeEnables 0x%x", uiPlaneEnables);

    do
    {
        if (!m_Initialized)
        {
            PCH_HAL_ASSERTMESSAGE("Error, not initialized.");
            hr = E_FAIL;
            MT_ERR1(MT_CP_HAL_NOT_INITIALIZED, MT_ERROR_CODE, hr);
            break;
        }

        // Validate input parameters.
        if (pbEncPlaneEnables   == NULL ||
            uiPlanesEnableSize  == 0)       // Cannot be negative.
        {
            PCH_HAL_ASSERTMESSAGE("Invalid parameter.");
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        switch (m_uiFWApiVersion)
        {
            // GetPlaneEnable command is removed in FW v4.1
            // Add support in FW4.2 for Gen9 CPU + Gen12 PCH
            case FIRMWARE_API_VERSION_4:
            case FIRMWARE_API_VERSION_4_2:
            {
                // The plane enable packet changed because apps can be lite or heavy,
                // and the plane enable packet became the method to actually put a session in heavy mode.
                GetPlaneEnableInBuff_api4 sGetPlaneApi4In = { { 0, 0, 0, 0 }, 0, 0, 0 };
                PCH_HAL_VERBOSEMESSAGE("Sending GET PLANE ENABLE FwApiv4 command.");
                sGetPlaneApi4In.Header.ApiVersion = m_uiFWApiVersion;
                sGetPlaneApi4In.Header.CommandId  = FIRMWARE_CMD_GET_PLANE_ENABLE4;
                sGetPlaneApi4In.PlaneEnables      = uiPlaneEnables;
                sGetPlaneApi4In.MemoryMode        = uiMemoryMode;
                sGetPlaneApi4In.AppID             = m_uiStreamId;

                hr = SendRecvCmd(
                    &sGetPlaneApi4In,
                    sizeof(sGetPlaneApi4In),
                    &sGetPlaneOut,
                    sizeof(sGetPlaneOut));
                break;
            }
            case FIRMWARE_API_VERSION_32:
            {
                // The plane enable packet changed to support Modular DRM
                GetPlaneEnableInBuff_api32 sGetPlaneApi32In = { { 0, 0, 0, 0 }, 0, 0, 0 };

                sGetPlaneApi32In.Header.ApiVersion = FIRMWARE_API_VERSION_32;
                sGetPlaneApi32In.Header.CommandId  = FIRMWARE_CMD_GET_PLANE_ENABLE_EX;
                sGetPlaneApi32In.PlaneEnables      = uiPlaneEnables;
                sGetPlaneApi32In.MemoryMode        = uiMemoryMode;
                sGetPlaneApi32In.PavpSlot          = m_uiStreamId;

                hr = SendRecvCmd(
                    &sGetPlaneApi32In,
                    sizeof(sGetPlaneApi32In),
                    &sGetPlaneOut,
                    sizeof(sGetPlaneOut));
                break;
            }
            case FIRMWARE_API_VERSION_3:
            {
                // The plane enable packet changed because apps can be lite or heavy,
                // and the plane enable packet became the method to actually put a session in heavy mode.
                GetPlaneEnableInBuff_api3 sGetPlaneApi3In = { { 0, 0, 0, 0 }, 0, 0 };
                PCH_HAL_VERBOSEMESSAGE("Sending GET PLANE ENABLE FwApiv3 command.");
                sGetPlaneApi3In.Header.ApiVersion = FIRMWARE_API_VERSION_3;
                sGetPlaneApi3In.Header.CommandId  = FIRMWARE_CMD_GET_PLANE_ENABLE;
                sGetPlaneApi3In.PlaneEnables      = uiPlaneEnables;
                sGetPlaneApi3In.MemoryMode        = uiMemoryMode;

                hr = SendRecvCmd(
                    &sGetPlaneApi3In,
                    sizeof(sGetPlaneApi3In),
                    &sGetPlaneOut,
                    sizeof(sGetPlaneOut));
                break;
            }
            default:
                PCH_HAL_ASSERTMESSAGE("Unexpected FW version: got 0x%x", m_uiFWApiVersion);
                hr = E_FAIL;
                MT_ERR2(MT_CP_HAL_FW_RULE, MT_CP_FW_API_VERSION, m_uiFWApiVersion, MT_ERROR_CODE, hr);
                break;
        }

        if (FAILED(hr))
        {
            PCH_HAL_ASSERTMESSAGE("Error sending GET PLANE ENABLE command. Error = 0x%x.", hr);
            MT_ERR2(MT_CP_CMD_SEND_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }
        else if (sGetPlaneOut.Header.PavpCmdStatus != PAVP_STATUS_SUCCESS)
        {
            hr = E_FAIL;
            PCH_HAL_ASSERTMESSAGE("Fw returned an error for GET PLANE ENABLE command. Error = 0x%x.", sGetPlaneOut.Header.PavpCmdStatus);
            MT_ERR2(MT_CP_HAL_STATUS_CHECK, MT_ERROR_CODE, sGetPlaneOut.Header.PavpCmdStatus, MT_FUNC_RET, hr);
            break;
        }

        eStatus = MOS_SecureMemcpy(
            pbEncPlaneEnables,
            uiPlanesEnableSize,
            &sGetPlaneOut.EncPlaneEnables,
            MOS_MIN(uiPlanesEnableSize, sizeof(sGetPlaneOut.EncPlaneEnables)));
        if (eStatus != MOS_STATUS_SUCCESS)
        {
            PCH_HAL_ASSERTMESSAGE("MOS_SecureMemcpy failed with error = %d.", eStatus);
            hr = E_INVALIDARG;
            MT_ERR3(MT_CP_MEM_COPY, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, eStatus, MT_FUNC_RET, hr);
            break;
        }
    } while (FALSE);

    PCH_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpPchHalUnified::VerifyConnStatus(
    uint8_t uiCsSize,
    PVOID pbConnectionStatus)
{
    HRESULT                       hr              = S_OK;
    MOS_STATUS                    eStatus         = MOS_STATUS_SUCCESS;
    VerifyConnStatusOutBuff       sVerifyCsOut    = {{0,0,0,0}};

    PCH_HAL_FUNCTION_ENTER;

    do
    {
        if (!m_Initialized)
        {
            PCH_HAL_ASSERTMESSAGE("Error, not initialized.");
            hr = E_FAIL;
            MT_ERR1(MT_CP_HAL_NOT_INITIALIZED, MT_ERROR_CODE, hr);
            break;
        }

        // Validate input parameters.
        if (pbConnectionStatus  == NULL ||
            uiCsSize            == 0)       // Cannot be negative.
        {
            PCH_HAL_ASSERTMESSAGE("Invalid parameter.");
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        switch (m_uiFWApiVersion)
        {
            case FIRMWARE_API_VERSION_32:
            {
                VerifyConnStatusInBuff_api32 sVerifyCsApi32In = { { 0, 0, 0, 0 }, { 0 }, 0 };
                sVerifyCsApi32In.Header.ApiVersion = FIRMWARE_API_VERSION_32;
                sVerifyCsApi32In.Header.CommandId  = FIRMWARE_CMD_VERIFY_CONN_STATUS_EX;
                sVerifyCsApi32In.PavpSlot          = m_uiStreamId;

                eStatus = MOS_SecureMemcpy(
                        &sVerifyCsApi32In.ConnectionStatus,
                        sizeof(sVerifyCsApi32In.ConnectionStatus),
                        pbConnectionStatus,
                        MOS_MIN(uiCsSize, sizeof(sVerifyCsApi32In.ConnectionStatus)));
                if (eStatus != MOS_STATUS_SUCCESS)
                {
                    PCH_HAL_ASSERTMESSAGE("MOS_SecureMemcpy for StreamID 0x%x failed with error = %d.", m_uiStreamId, eStatus);
                    hr = E_INVALIDARG;
                    MT_ERR3(MT_CP_MEM_COPY, MT_CP_STREAM_ID, m_uiStreamId, MT_ERROR_CODE, eStatus, MT_FUNC_RET, hr);
                    break;
                }

                hr = SendRecvCmd(
                        &sVerifyCsApi32In,
                        sizeof(sVerifyCsApi32In),
                        &sVerifyCsOut,
                        sizeof(sVerifyCsOut));
                break;
            }
            case FIRMWARE_API_VERSION_3:
            {
                VerifyConnStatusInBuff_api3 sVerifyCsApi3In = { { 0, 0, 0, 0 }, { 0 } };
                sVerifyCsApi3In.Header.ApiVersion = FIRMWARE_API_VERSION_3;
                sVerifyCsApi3In.Header.CommandId  = FIRMWARE_CMD_VERIFY_CONN_STATUS;

                eStatus = MOS_SecureMemcpy(
                    &sVerifyCsApi3In.ConnectionStatus,
                    sizeof(sVerifyCsApi3In.ConnectionStatus),
                    pbConnectionStatus,
                    MOS_MIN(uiCsSize, sizeof(sVerifyCsApi3In.ConnectionStatus)));
                if (eStatus != MOS_STATUS_SUCCESS)
                {
                    PCH_HAL_ASSERTMESSAGE("MOS_SecureMemcpy failed with error = %d.", eStatus);
                    hr = E_INVALIDARG;
                    MT_ERR2(MT_CP_MEM_COPY, MT_ERROR_CODE, eStatus, MT_FUNC_RET, hr);
                    break;
                }

                hr = SendRecvCmd(
                    &sVerifyCsApi3In,
                    sizeof(sVerifyCsApi3In),
                    &sVerifyCsOut,
                    sizeof(sVerifyCsOut));
                break;
            }
            default:
                PCH_HAL_ASSERTMESSAGE("Unexpected FW version: got 0x%x", m_uiFWApiVersion);
                hr = E_FAIL;
                MT_ERR2(MT_CP_HAL_FW_RULE, MT_CP_FW_API_VERSION, m_uiFWApiVersion, MT_ERROR_CODE, hr);
                break;
        }

        if (FAILED(hr))
        {
            PCH_HAL_ASSERTMESSAGE("Error sending VERIFY CONN STATUS command. Error = 0x%x.", hr);
            MT_ERR2(MT_CP_CMD_SEND_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }
        else if (sVerifyCsOut.Header.PavpCmdStatus != PAVP_STATUS_SUCCESS)
        {
            hr = E_ABORT;
            PCH_HAL_ASSERTMESSAGE("Fw returned an error for VERIFY CONN STATUS command. Status = 0x%x.", sVerifyCsOut.Header.PavpCmdStatus);
            MT_ERR2(MT_CP_HAL_STATUS_CHECK, MT_ERROR_CODE, sVerifyCsOut.Header.PavpCmdStatus, MT_FUNC_RET, hr);
            break;
        }
    } while (FALSE);
   
    PCH_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpPchHalUnified::GetNonce(
    bool        bResetStreamKey,
    uint8_t     uEncE0Size,
    uint8_t     *puiEncryptedE0,
    uint32_t    *puiNonce)
{
    HRESULT              hr              = S_OK;
    MOS_STATUS           eStatus         = MOS_STATUS_SUCCESS;
    GetDispNonceOutBuff  sGetNonceOut    = {{0, 0, 0, 0}, {0}, 0};

    PCH_HAL_FUNCTION_ENTER;
    
    do
    {
        if (!m_Initialized)
        {
            PCH_HAL_ASSERTMESSAGE("Error, not initialized.");
            hr = E_FAIL;
            MT_ERR1(MT_CP_HAL_NOT_INITIALIZED, MT_ERROR_CODE, hr);
            break;
        }

        // Validate input parameters.
        if (puiEncryptedE0  == NULL ||
            puiNonce        == NULL ||
            uEncE0Size      == 0)       // Cannot be negative.
        {
            PCH_HAL_ASSERTMESSAGE("Invalid parameter.");
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        PCH_HAL_VERBOSEMESSAGE("bResetStreamKey =  %d.", bResetStreamKey);

        switch (m_uiFWApiVersion)
        {
            case FIRMWARE_API_VERSION_32:
            {
                GetDispNonceExInBuff sGetNonceExIn = { { 0, 0, 0, 0 }, 0, 0 };
                sGetNonceExIn.Header.ApiVersion = FIRMWARE_API_VERSION_32;
                sGetNonceExIn.Header.CommandId  = FIRMWARE_CMD_GET_DISPLAY_NONCE_EX;
                sGetNonceExIn.resetStreamKey    = bResetStreamKey;
                sGetNonceExIn.PavpSlot          = m_uiStreamId;
                hr = SendRecvCmd(
                    &sGetNonceExIn,
                    sizeof(sGetNonceExIn),
                    &sGetNonceOut,
                    sizeof(sGetNonceOut));
                break;
            }
            case FIRMWARE_API_VERSION_3:
            {
                GetDispNonceInBuff sGetNonceIn = { { 0, 0, 0, 0 }, 0 };
                sGetNonceIn.Header.ApiVersion  = FIRMWARE_API_VERSION_3;
                sGetNonceIn.Header.CommandId   = FIRMWARE_CMD_GET_DISPLAY_NONCE;
                sGetNonceIn.resetStreamKey     = bResetStreamKey;
                hr = SendRecvCmd(
                    &sGetNonceIn,
                    sizeof(sGetNonceIn),
                    &sGetNonceOut,
                    sizeof(sGetNonceOut));
                break;
            }
            default:
                PCH_HAL_ASSERTMESSAGE("Unexpected FW version: got 0x%x", m_uiFWApiVersion);
                hr = E_FAIL;
                MT_ERR2(MT_CP_HAL_FW_RULE, MT_CP_FW_API_VERSION, m_uiFWApiVersion, MT_ERROR_CODE, hr);
                break;
        }

        if (FAILED(hr))
        {
            PCH_HAL_ASSERTMESSAGE("Error sending GET NONCE command for StreamID 0x%x. Error = 0x%x.", m_uiStreamId, hr);
            MT_ERR3(MT_CP_CMD_SEND_FAIL, MT_CP_STREAM_ID, m_uiStreamId, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }
        else if (sGetNonceOut.Header.PavpCmdStatus != PAVP_STATUS_SUCCESS)
        {
            hr = E_FAIL;
            PCH_HAL_ASSERTMESSAGE("Fw returned an error for GET NONCE command for StreamID 0x%x. Error = 0x%x.", m_uiStreamId, sGetNonceOut.Header.PavpCmdStatus);
            MT_ERR3(MT_CP_HAL_STATUS_CHECK, MT_CP_STREAM_ID, m_uiStreamId, MT_ERROR_CODE, sGetNonceOut.Header.PavpCmdStatus, MT_FUNC_RET, hr);
            break;
        }

        eStatus = MOS_SecureMemcpy(
                puiEncryptedE0,
                uEncE0Size,
                sGetNonceOut.EncryptedE0,
                MOS_MIN(uEncE0Size, sizeof(sGetNonceOut.EncryptedE0)));
        if (eStatus != MOS_STATUS_SUCCESS)
        {
            PCH_HAL_ASSERTMESSAGE("MOS_SecureMemcpy failed with error = %d.", eStatus);
            hr = E_INVALIDARG;
            MT_ERR2(MT_CP_MEM_COPY, MT_ERROR_CODE, eStatus, MT_FUNC_RET, hr);
            break;
        }

        *puiNonce = sGetNonceOut.Nonce;

    } while (FALSE);
   
    PCH_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpPchHalUnified::SendDaisyChainCmd(StreamKey pKeyToTranslate, bool IsDKey, StreamKey pTranslatedKey)
{
    HRESULT                             hr = S_OK;
    MOS_STATUS                          ReturnStatus = MOS_STATUS_SUCCESS;
    uint8_t                             IgdData[FIRMWARE_MAX_IGD_DATA_SIZE]; // Local buffer for IGD-ME private data.
    uint32_t                            IgdDataLen = sizeof(IgdData);
    PAVPCmdHdr                          *pHeader = NULL;
    GetDaisyChainKeyInBuf               KeyInCmd32;
    GetDaisyChainKeyOutBuf              KeyOutCmd32;
    GetDaisyChainKeyInBufLegacyAPI      KeyInCmdLegacyAPI;
    GetDaisyChainKeyInBuf42             KeyInCmd42;
    GetDaisyChainKeyOutBuf42            KeyOutCmd42;
    void *                              KeyInCmd;
    void *                              KeyOutCmd;
    uint32_t                            KeyInCmdSize;
    uint32_t                            KeyOutCmdSize;

    PCH_HAL_FUNCTION_ENTER;

    do
    {
        if (m_uiFWApiVersion > FIRMWARE_API_VERSION_4_1)
        {
            MOS_ZeroMemory(&KeyInCmd42, sizeof(KeyInCmd42));
            MOS_ZeroMemory(&KeyOutCmd42, sizeof(KeyOutCmd42));

            //fill the header info for the CSME Command structures...
            KeyInCmd42.Header.ApiVersion    = m_uiFWApiVersion;
            KeyInCmd42.Header.CommandId     = FIRMWARE_42_CMD_DAISY_CHAIN;
            KeyInCmd42.Header.PavpCmdStatus = 0;
            KeyInCmd42.Header.BufferLength  = sizeof(KeyInCmd42) - sizeof(KeyInCmd42.Header);
            KeyInCmd42.key_type             = IsDKey ? 0 : 1;
            KeyInCmd42.key_size             = sizeof(StreamKey);

            //copy the incoming key to the command structure...
            ReturnStatus = MOS_SecureMemcpy(KeyInCmd42.stream_key, sizeof(StreamKey), pKeyToTranslate, sizeof(StreamKey));
            if (MOS_STATUS_SUCCESS != ReturnStatus)
            {
                PCH_HAL_ASSERTMESSAGE("MOS Secure Copy of Input Key returned failure: error = %d.", ReturnStatus);
                hr = E_FAIL;
                MT_ERR2(MT_CP_MEM_COPY, MT_ERROR_CODE, ReturnStatus, MT_FUNC_RET, hr);
                break;
            }
            KeyInCmd      = &KeyInCmd42;
            KeyOutCmd     = &KeyOutCmd42;
            KeyInCmdSize  = sizeof(KeyInCmd42);
            KeyOutCmdSize = sizeof(KeyOutCmd42);
        }
        else
        {
            MOS_ZeroMemory(&KeyInCmd32, sizeof(KeyInCmd32));
            MOS_ZeroMemory(&KeyOutCmd32, sizeof(KeyOutCmd32));

            //fill the header info for the CSME Command structures...
            KeyInCmd32.Header.ApiVersion    = VER_PLAYREADY_PAVP_API;
            KeyInCmd32.Header.CommandId     = FIRMWARE_CMD_DAISY_CHAIN;
            KeyInCmd32.Header.PavpCmdStatus = 0;
            KeyInCmd32.Header.BufferLength  = sizeof(KeyInCmd32) - sizeof(KeyInCmd32.Header);
            KeyInCmd32.Reserved0            = 0;
            KeyInCmd32.IsDKey               = IsDKey ? 1 : 0;
            KeyInCmd32.Reserved1            = 0;

            //copy the incoming key to the command structure...
            ReturnStatus = MOS_SecureMemcpy(KeyInCmd32.SnWrappedInPrevKey, sizeof(KeyInCmd32.SnWrappedInPrevKey), pKeyToTranslate, sizeof(StreamKey));
            if (MOS_STATUS_SUCCESS != ReturnStatus)
            {
                PCH_HAL_ASSERTMESSAGE("MOS Secure Copy of Input Key returned failure: error = %d.", ReturnStatus);
                hr = E_FAIL;
                MT_ERR2(MT_CP_MEM_COPY, MT_ERROR_CODE, ReturnStatus, MT_FUNC_RET, hr);
                break;
            }
            KeyInCmd      = &KeyInCmd32;
            KeyOutCmd     = &KeyOutCmd32;
            KeyInCmdSize  = sizeof(KeyInCmd32);
            KeyOutCmdSize = sizeof(KeyOutCmd32);
        }

        hr = PassThrough(
            reinterpret_cast<PBYTE>(KeyInCmd),
            KeyInCmdSize,
            reinterpret_cast<PBYTE>(KeyOutCmd),
            KeyOutCmdSize,
            IgdData,
            &IgdDataLen,
            FALSE);

        //since this is a private FW api, the key is returned as private data...
        if (m_uiFWApiVersion > FIRMWARE_API_VERSION_4_1)
        {
            pHeader = &(reinterpret_cast<GetDaisyChainKeyOutBuf42 *>(KeyOutCmd)->Header);
        }
        else
        {
            pHeader = &(reinterpret_cast<GetDaisyChainKeyOutBuf *>(KeyOutCmd)->Header);
        }

        //check the return code...
        if (FAILED(hr))
        {
            PCH_HAL_ASSERTMESSAGE("Daisy chain command returned failure: hr = 0x%x. Status Returned = 0x%x", hr, pHeader->PavpCmdStatus);
            MT_ERR2(MT_CP_HAL_STATUS_CHECK, MT_ERROR_CODE, pHeader->PavpCmdStatus, MT_FUNC_RET, hr);
            break;
        }

        /*
        https://hsdes.intel.com/appstore/article/#/1405645552/main
        FIRMWARE_CMD_DAISY_CHAIN FW API definition was changed in driver and FW without changing FW API version. 
        Therefore there is no way for driver to know which API definition should be used with which FW version resulting in error during key rotation if driver and FW version are misaligned.
        This issue was caused by DCN 4163116 CL 537968.
        As a workaround it was decided if FW return invalid buffer length with first try which is with new API structure then a retry with old API structure would be made
        */
        if ((m_uiFWApiVersion <= FIRMWARE_API_VERSION_4_1) && (pHeader->PavpCmdStatus == PAVP_STATUS_INVALID_BUFFER_LENGTH))
        {
            PCH_HAL_ASSERTMESSAGE("ME returned Invalid Buffer Length after first try. Retry it with old FW API");
            MT_ERR1(MT_CP_HAL_STATUS_CHECK, MT_ERROR_CODE, pHeader->PavpCmdStatus);
            pHeader = NULL;
            MOS_ZeroMemory(&KeyInCmdLegacyAPI, sizeof(KeyInCmdLegacyAPI));
            MOS_ZeroMemory(&KeyOutCmd, sizeof(KeyOutCmd));

            //fill the old API header info for the CSME Command structures...
            KeyInCmdLegacyAPI.Header.ApiVersion    = VER_PLAYREADY_PAVP_API;
            KeyInCmdLegacyAPI.Header.CommandId     = FIRMWARE_CMD_DAISY_CHAIN;
            KeyInCmdLegacyAPI.Header.PavpCmdStatus = 0;
            KeyInCmdLegacyAPI.Header.BufferLength  = sizeof(KeyInCmdLegacyAPI) - sizeof(KeyInCmdLegacyAPI.Header);
            KeyInCmdLegacyAPI.SessionId            = 0;  // since FW doesnt use sessionid we can hard code it.
            KeyInCmdLegacyAPI.IsDKey               = IsDKey ? 1: 0;

            //copy the incoming key to the command structure...
            ReturnStatus = MOS_SecureMemcpy(KeyInCmdLegacyAPI.SnWrappedInPrevKey, sizeof(KeyInCmdLegacyAPI.SnWrappedInPrevKey), pKeyToTranslate, sizeof(StreamKey));
            if (MOS_STATUS_SUCCESS != ReturnStatus)
            {
                PCH_HAL_ASSERTMESSAGE("MOS Secure Copy of Input Key returned failure: error = %d.", ReturnStatus);
                hr = E_FAIL;
                MT_ERR2(MT_CP_MEM_COPY, MT_ERROR_CODE, ReturnStatus, MT_FUNC_RET, hr);
                break;
            }

            hr = PassThrough(
                reinterpret_cast<PBYTE>(&KeyInCmdLegacyAPI),
                sizeof(KeyInCmdLegacyAPI),
                reinterpret_cast<PBYTE>(&KeyOutCmd32),
                sizeof(KeyOutCmd32),
                IgdData,
                &IgdDataLen,
                FALSE);

            //check the return code...
            if (FAILED(hr))
            {
                PCH_HAL_ASSERTMESSAGE("Daisy chain command returned failure: hr = 0x%x. Status Returned = 0x%x", hr, KeyOutCmd32.Header.PavpCmdStatus);
                MT_ERR2(MT_CP_HAL_STATUS_CHECK, MT_ERROR_CODE, KeyOutCmd32.Header.PavpCmdStatus, MT_FUNC_RET, hr);
                break;
            }

            pHeader = &(KeyOutCmd32.Header);
        }

        if ((m_uiFWApiVersion > FIRMWARE_API_VERSION_4_1) && (pHeader->PavpCmdStatus == PAVP_STATUS_SUCCESS))
        {
            if (KeyOutCmd42.key_size > 0)
            {
                PCH_HAL_VERBOSEMESSAGE("Length of wrapped stream key received from CSME: %d", KeyOutCmd42.key_size);
                if (KeyOutCmd42.key_size == PAVP_EPID_STREAM_KEY_LEN)
                {
                    //copy the translated key from the CSME returned command structure...
                    ReturnStatus = MOS_SecureMemcpy(pTranslatedKey, sizeof(StreamKey), KeyOutCmd42.wrapped_stream_key, PAVP_EPID_STREAM_KEY_LEN);
                    if (MOS_STATUS_SUCCESS != ReturnStatus)
                    {
                        PCH_HAL_ASSERTMESSAGE("MOS Secure Copy of CSME returned wrapped stream key returned failure: error = %d.", ReturnStatus);
                        hr = E_FAIL;
                        MT_ERR2(MT_CP_MEM_COPY, MT_ERROR_CODE, ReturnStatus, MT_FUNC_RET, hr);
                        break;
                    }
                }
                else
                {
                    PCH_HAL_ASSERTMESSAGE("Daisy chain command returned wrapped stream key != Key length. Length returned: %d.", IgdDataLen);
                    hr = E_FAIL;
                    MT_ERR2(MT_CP_HAL_KEY_RULE, MT_CP_KEY_LENGTH, IgdDataLen, MT_ERROR_CODE, hr);
                    break;
                }
            }
        }
        else if ((m_uiFWApiVersion <= FIRMWARE_API_VERSION_4_1) && (pHeader->PavpCmdStatus == FIRMWARE_PRIVATE_INFO_FLAG))
        {
            PCH_HAL_VERBOSEMESSAGE("ME firmware returned private data.");
            pHeader->PavpCmdStatus ^= FIRMWARE_PRIVATE_INFO_FLAG;

            if (IgdDataLen > 0)
            {
                PCH_HAL_VERBOSEMESSAGE("Length of Private data received from CSME: %d", IgdDataLen);
                if (IgdDataLen == PAVP_EPID_STREAM_KEY_LEN)
                {
                    //copy the translated key from the CSME returned command structure...
                    ReturnStatus = MOS_SecureMemcpy(pTranslatedKey, sizeof(StreamKey), IgdData, PAVP_EPID_STREAM_KEY_LEN);
                    if (MOS_STATUS_SUCCESS != ReturnStatus)
                    {
                        PCH_HAL_ASSERTMESSAGE("MOS Secure Copy of CSME returned private data returned failure: error = %d.", ReturnStatus);
                        hr = E_FAIL;
                        MT_ERR2(MT_CP_MEM_COPY, MT_ERROR_CODE, ReturnStatus, MT_FUNC_RET, hr);
                        break;
                    }
                }
                else
                {
                    PCH_HAL_ASSERTMESSAGE("Daisy chain command returned private data != Key length. Length returned: %d.", IgdDataLen);
                    hr = E_FAIL;
                    MT_ERR2(MT_CP_HAL_KEY_RULE, MT_CP_KEY_LENGTH, IgdDataLen, MT_ERROR_CODE, hr);
                    break;
                }
            }

            // Clear private data
            MOS_ZeroMemory(IgdData, FIRMWARE_MAX_IGD_DATA_SIZE);
        }
        else if (pHeader->PavpCmdStatus == PAVP_STATUS_INVALID_BUFFER_LENGTH)
        {
            PCH_HAL_ASSERTMESSAGE("ME returned Invalid Buffer Length after second retry");
            hr = E_FAIL;
            MT_ERR2(MT_CP_HAL_STATUS_CHECK, MT_ERROR_CODE, pHeader->PavpCmdStatus, MT_FUNC_RET, hr);
            break;
        }
        else
        {
            PCH_HAL_ASSERTMESSAGE("Daisy chain command returned no private data where expected. Status returned: 0x%x.", pHeader->PavpCmdStatus);
            hr = E_FAIL;
            MT_ERR2(MT_CP_HAL_STATUS_CHECK, MT_ERROR_CODE, pHeader->PavpCmdStatus, MT_FUNC_RET, hr);
            break;
        }

    } while (FALSE);

    PCH_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpPchHalUnified::VerifyMemStatus(
    uint8_t uPMemStatusSize,
    PVOID puPMemStatus)
{
    HRESULT                 hr              = S_OK;
    INT                     eStatus         = MOS_STATUS_SUCCESS;
    CheckPavpModeOutBuff    sCheckModeOut   = {{0,0,0,0}};

    PCH_HAL_FUNCTION_ENTER;

    do
    {
        if (!m_Initialized)
        {
            PCH_HAL_ASSERTMESSAGE("Error, not initialized.");
            hr = E_FAIL;
            MT_ERR1(MT_CP_HAL_NOT_INITIALIZED, MT_ERROR_CODE, hr);
            break;
        }

        // Validate input parameters.
        if (puPMemStatus    == NULL ||
            uPMemStatusSize == 0)       // Cannot be negative.
        {
            PCH_HAL_ASSERTMESSAGE("Invalid parameter.");
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }
    
        PCH_HAL_VERBOSEMESSAGE("mem status size 0x%x", uPMemStatusSize);

        switch (m_uiFWApiVersion)
        {
            case FIRMWARE_API_VERSION_32:
            {
                CheckPavpModeExInBuff sCheckModeExIn = { { 0, 0, 0, 0 }, { 0 }, 0 };
                sCheckModeExIn.Header.ApiVersion     = FIRMWARE_API_VERSION_32;
                sCheckModeExIn.Header.CommandId      = FIRMWARE_CMD_CHECK_PAVP_MODE_EX;
                sCheckModeExIn.PavpSlot              = m_uiStreamId;

                eStatus = MOS_SecureMemcpy(
                        sCheckModeExIn.ProtectedMemoryStatus,
                        sizeof(sCheckModeExIn.ProtectedMemoryStatus),
                        puPMemStatus,
                        MOS_MIN(uPMemStatusSize, sizeof(sCheckModeExIn.ProtectedMemoryStatus)));
                if (eStatus != MOS_STATUS_SUCCESS)
                {
                    PCH_HAL_ASSERTMESSAGE("SecureCopyMemory failed for StreamID 0x%x with error = %d.", m_uiStreamId, eStatus);
                    hr = E_INVALIDARG;
                    MT_ERR3(MT_CP_MEM_COPY, MT_CP_STREAM_ID, m_uiStreamId, MT_ERROR_CODE, eStatus, MT_FUNC_RET, hr);
                    break;
                }

                hr = SendRecvCmd(
                        &sCheckModeExIn,
                        sizeof(sCheckModeExIn),
                        &sCheckModeOut,
                        sizeof(sCheckModeOut));
                break;
            }
            case FIRMWARE_API_VERSION_3:
            {
                CheckPavpModeInBuff sCheckModeIn = { { 0, 0, 0, 0 }, { 0 } };
                sCheckModeIn.Header.ApiVersion   = FIRMWARE_API_VERSION_3;
                sCheckModeIn.Header.CommandId    = FIRMWARE_CMD_CHECK_PAVP_MODE;

                eStatus = MOS_SecureMemcpy(
                    sCheckModeIn.ProtectedMemoryStatus,
                    sizeof(sCheckModeIn.ProtectedMemoryStatus),
                    puPMemStatus,
                    MOS_MIN(uPMemStatusSize, sizeof(sCheckModeIn.ProtectedMemoryStatus)));
                if (eStatus != MOS_STATUS_SUCCESS)
                {
                    PCH_HAL_ASSERTMESSAGE("SecureCopyMemory failed with error = %d.", eStatus);
                    hr = E_INVALIDARG;
                    MT_ERR2(MT_CP_MEM_COPY, MT_ERROR_CODE, eStatus, MT_FUNC_RET, hr);
                    break;
                }

                hr = SendRecvCmd(
                    &sCheckModeIn,
                    sizeof(sCheckModeIn),
                    &sCheckModeOut,
                    sizeof(sCheckModeOut));
                break;
            }
            default:
                PCH_HAL_ASSERTMESSAGE("Unexpected FW version: got 0x%x", m_uiFWApiVersion);
                hr = E_FAIL;
                MT_ERR2(MT_CP_HAL_FW_RULE, MT_CP_FW_API_VERSION, m_uiFWApiVersion, MT_ERROR_CODE, hr);
                break;
        }

        if (FAILED(hr))
        {
            PCH_HAL_ASSERTMESSAGE("Error sending CHECK PAVP MODE command. Error = 0x%x.", hr);
            MT_ERR2(MT_CP_CMD_SEND_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }
        else if (sCheckModeOut.Header.PavpCmdStatus != PAVP_STATUS_SUCCESS)
        {
            hr = E_ABORT;
            PCH_HAL_ASSERTMESSAGE("Fw returned an error for CHECK PAVP MODE command. Status = 0x%x.", sCheckModeOut.Header.PavpCmdStatus);
            MT_ERR2(MT_CP_HAL_STATUS_CHECK, MT_ERROR_CODE, sCheckModeOut.Header.PavpCmdStatus, MT_FUNC_RET, hr);
            break;
        }
    } while (FALSE);
  
    PCH_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpPchHalUnified::SaveKeyBlobForPAVPINIT43(void *pStoreKeyBlobParams)
{
    PAVP_STATUS status = PAVP_STATUS_SUCCESS;
    HRESULT     hr     = S_OK;

    PCH_HAL_FUNCTION_ENTER;

    do
    {
        // Validate input parameters.
        if (pStoreKeyBlobParams == nullptr)
        {
            PCH_HAL_ASSERTMESSAGE("Invalid parameter.");
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        memset(&m_KeyBlobForINIT43, 0, sizeof(PAVPStoreKeyBlobInputBuff));

        if (m_uiFWApiVersion >= FIRMWARE_API_VERSION_4_3)
        {
            m_KeyBlobForINIT43 = *(PAVPStoreKeyBlobInputBuff*)pStoreKeyBlobParams;
        }

    } while (FALSE);

    PCH_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpPchHalUnified::PassThrough(
    PBYTE       pInput,
    ULONG       ulInSize,
    PBYTE       pOutput,
    ULONG       ulOutSize,
    uint8_t     *puiIgdData,                         // Might be NULL.
    uint32_t    *puiIgdDataLen,
    bool        bIsWidiMsg,
    uint8_t     vTag)
{
    PAVP_STATUS status = PAVP_STATUS_SUCCESS;
    HRESULT     hr     = S_OK;

    PCH_HAL_FUNCTION_ENTER;
    MOS_UNUSED(vTag);
    do
    {

        if (!m_Initialized)
        {
            PCH_HAL_ASSERTMESSAGE("Error, not initialized.");
            hr = E_FAIL;
            MT_ERR1(MT_CP_HAL_NOT_INITIALIZED, MT_ERROR_CODE, hr);
            break;
        }
    
        // Validate input parameters.
        if (pInput        == NULL   ||
            pOutput       == NULL   ||
            ulInSize      == 0      ||  // Cannot be negative.
            ulOutSize     == 0)         // Cannot be negative.
        {
            PCH_HAL_ASSERTMESSAGE("Invalid parameter.");
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        PCH_HAL_VERBOSEMESSAGE("inSize 0x%x outSize 0x%x",
            ulInSize, ulOutSize);
        
        PCH_HAL_VERBOSEMESSAGE("Command id 0x%x", (reinterpret_cast<PAVPCmdHdr *>(pInput))->CommandId);

        PCH_HAL_VERBOSEMESSAGE("%s for StartIo3", (bIsWidiMsg) ? "using WidiSession handle" : "using PavpSessionHandle");

        hr = IoMessage(
                    pInput,
                    ulInSize,
                    pOutput,
                    ulOutSize, 
                    puiIgdData,
                    puiIgdDataLen,
                    bIsWidiMsg);

        if (FAILED(hr))
        {
               PCH_HAL_ASSERTMESSAGE("Failed sending FW Passthrough CMD. "
                    "DLL status %x Header status %x description '%s'",
                    status, (reinterpret_cast<PAVPCmdHdr *>(pOutput))->PavpCmdStatus, PavpStatusToString(status));
               MT_ERR2(MT_CP_CMD_SEND_FAIL, MT_ERROR_CODE, status, MT_ERROR_CODE, (reinterpret_cast<PAVPCmdHdr *>(pOutput))->PavpCmdStatus);
            break;
        }
    } while (FALSE);

    PCH_HAL_FUNCTION_EXIT(hr);
    return hr;
}

uint32_t CPavpPchHalUnified::FWApiVersion()
{
    return m_uiFWApiVersion;
}

BOOL CPavpPchHalUnified::IsInitialized()
{
    return m_Initialized;
}

HRESULT CPavpPchHalUnified::GetHeartBeatStatusNonce(
    uint32_t        *puiNonce,
    uint32_t        uiStreamId,
    PAVP_DRM_TYPE   eDrmType)
{
    GetHeartBeatNonceInBuff   getNonceIn    = {{0, 0, 0, 0}, 0};
    GetHeartBeatNonceOutBuff  getNonceOut   = {{0, 0, 0, 0}, 0};
    HRESULT                   hr            = S_OK;

    PCH_HAL_FUNCTION_ENTER;

    do
    {
        if (!m_Initialized)
        {
            PCH_HAL_ASSERTMESSAGE("Error, not initialized.");
            hr = E_FAIL;
            MT_ERR1(MT_CP_HAL_NOT_INITIALIZED, MT_ERROR_CODE, hr);
            break;
        }

        // Validate input parameter.
        if (puiNonce == NULL)
        {
            PCH_HAL_ASSERTMESSAGE("Invalid parameter.");
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

#if (_DEBUG || _RELEASE_INTERNAL)
        GetDebugHeartBeatEncKeyIn  getEncKeyIn  = { { 0, 0, 0, 0 }, 0};
        GetDebugHeartBeatEncKeyOut getEncKeyOut = { { 0, 0, 0, 0 }, { 0 } };
        
        if (eDrmType == PAVP_DRM_TYPE_PLAYREADY)
        {
            getEncKeyIn.Header.ApiVersion = FIRMWARE_API_VERSION_3;
        }
        else
        {
            getEncKeyIn.Header.ApiVersion = WVFIRMWARE_API_VERSION_1;
        }
        getEncKeyIn.Header.CommandId  = FIRMWARE_CMD_GET_DBG_HEART_BEAT_ENC_KEY;
        getEncKeyIn.StreamID          = uiStreamId;

        hr = SendRecvCmd(
                &getEncKeyIn,
                sizeof(getEncKeyIn),
                &getEncKeyOut,
                sizeof(getEncKeyOut));
        if (FAILED(hr))
        {
            PCH_HAL_ASSERTMESSAGE("Error sending DBG_HEART_BEAT_ENC_KEY command. Error = 0x%x.", hr);
            MT_ERR2(MT_CP_CMD_SEND_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        PCH_HAL_NORMALMESSAGE("Encryption key = 0x%x 0x%x 0x%x 0x%x", 
            (uint32_t *)&getEncKeyOut.EncKey[0], 
            (uint32_t *)&getEncKeyOut.EncKey[4], 
            (uint32_t *)&getEncKeyOut.EncKey[8], 
            (uint32_t *)&getEncKeyOut.EncKey[12]); 
#endif

        getNonceIn.Header.ApiVersion = FIRMWARE_API_VERSION_3;
        getNonceIn.Header.CommandId  = FIRMWARE_CMD_GET_PR_HEART_BEAT_NONCE;
        getNonceIn.StreamId          = uiStreamId;

        hr = SendRecvCmd(
                &getNonceIn,
                sizeof(getNonceIn),
                &getNonceOut,
                sizeof(getNonceOut));
        if (FAILED(hr))
        {
            PCH_HAL_ASSERTMESSAGE("Error sending GET NONCE command. Error = 0x%x.", hr);
            MT_ERR2(MT_CP_CMD_SEND_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }
        else if (getNonceOut.Header.PavpCmdStatus != PAVP_STATUS_SUCCESS)
        {
            hr = E_FAIL;
            PCH_HAL_ASSERTMESSAGE("Error in GET NONCE command. CmdStatus 0x%x", getNonceOut.Header.PavpCmdStatus);
            MT_ERR2(MT_CP_HAL_STATUS_CHECK, MT_ERROR_CODE, getNonceOut.Header.PavpCmdStatus, MT_FUNC_RET, hr);
            break;
        }

        *puiNonce = getNonceOut.Nonce;

    } while (FALSE);

    PCH_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpPchHalUnified::VerifyHeartBeatConnStatus(
    uint32_t        uiCsSize,
    PVOID           pbConnectionStatus,
    uint32_t        uiStreamId,
    PAVP_DRM_TYPE   eDrmType)
{
    VerifyHeartBeatInBuff       verifyCsIn      = {{0, 0, 0, 0}, 0, {0}};
    VerifyHeartBeatOutBuff      verifyCsOut     = {{0, 0, 0, 0}, 0};
    HRESULT                     hr              = S_OK;
    MOS_STATUS                  eStatus         = MOS_STATUS_SUCCESS;
    uint32_t                    uiApiVersion    = 0;

    PCH_HAL_FUNCTION_ENTER;

    do
    {
        if (!m_Initialized)
        {
            PCH_HAL_ASSERTMESSAGE("Error, not initialized.");
            hr = E_FAIL;
            MT_ERR1(MT_CP_HAL_NOT_INITIALIZED, MT_ERROR_CODE, hr);
            break;
        }

        // Validate input parameters.
        if (pbConnectionStatus  == NULL ||
            uiCsSize            == 0)       // Cannot be negative.
        {
            PCH_HAL_ASSERTMESSAGE("Invalid parameter.");
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        eStatus = MOS_SecureMemcpy(
                &verifyCsIn.ConnectionStatus,
                sizeof(verifyCsIn.ConnectionStatus),
                pbConnectionStatus,
                MOS_MIN(uiCsSize, sizeof(verifyCsIn.ConnectionStatus)));
        if (eStatus != MOS_STATUS_SUCCESS)
        {
            PCH_HAL_ASSERTMESSAGE("MOS_SecureMemcpy failed with error = %d.", eStatus);
            hr = E_INVALIDARG;
            MT_ERR2(MT_CP_MEM_COPY, MT_ERROR_CODE, eStatus, MT_FUNC_RET, hr);
            break;
        }

        if ((eDrmType == PAVP_DRM_TYPE_PLAYREADY) ||
            (eDrmType == PAVP_DRM_TYPE_WIDEVINE))
        {
            uiApiVersion = FIRMWARE_API_VERSION_3;
        }
        else
        {
            uiApiVersion = WVFIRMWARE_API_VERSION_1;
        }

        verifyCsIn.Header.ApiVersion = uiApiVersion;
        verifyCsIn.Header.CommandId  = FIRMWARE_CMD_VERIFY_PR_HEART_BEAT_STATUS;
        verifyCsIn.StreamId          = uiStreamId;

        hr = SendRecvCmd(
                &verifyCsIn,
                sizeof(verifyCsIn),
                &verifyCsOut,
                sizeof(verifyCsOut));
        if (FAILED(hr))
        {
            PCH_HAL_ASSERTMESSAGE("Error sending VERIFY CONN STATUS command. Hr 0x%x", hr);
            MT_ERR2(MT_CP_CMD_SEND_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }
        else if (verifyCsOut.Header.PavpCmdStatus != PAVP_STATUS_SUCCESS)
        {
            hr = E_FAIL;
            PCH_HAL_ASSERTMESSAGE("Error in VERIFY CONN STATUS command. CmdStatus 0x%x", verifyCsOut.Header.PavpCmdStatus);
            MT_ERR2(MT_CP_HAL_STATUS_CHECK, MT_ERROR_CODE, verifyCsOut.Header.PavpCmdStatus, MT_FUNC_RET, hr);
            break;
        }

        // This used to check the HdcpStatus field of verifyCsOut, but due to
        // different interpretations of the proper API definition of the field,
        // it is not meaningful data for the driver.

    } while (FALSE);

    PCH_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpPchHalUnified::SessionInit(
    const GUID* pCpMsgClass
    )
{
    HRESULT             hr                   = S_OK;

    PCH_HAL_FUNCTION_ENTER;
    
    do
    {
        if (pCpMsgClass == NULL)
        {
            PCH_HAL_ASSERTMESSAGE("Invalid Msg Class Argument");
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }
        /*Checks if the session to be created is PAVP*/
        if (!m_IsWidiEnabled)
        {
            if (!m_hPavpSession)
            {
                /*Creating PAVP session, passing WiDi enabled flag as false*/
                m_hPavpSession = CHeciSessionInterface::PchHal_HeciSessionFactory(
                    &PAVP_GUID,
                    FALSE,
                    m_pOsServices
                    );
            }

            if (m_hPavpSession == NULL)
            {
                PCH_HAL_ASSERTMESSAGE("Failed to create HECI session");
                hr = E_OUTOFMEMORY;
                MT_ERR2(MT_CP_SESSION_CREATE, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }
        }    

        /*Checks if the session to be created is WIDI.*/
        if (m_IsWidiEnabled)
        {
            /* For WiDi create a PAVP session */
            if (!m_hPavpSession)
            {
                m_hPavpSession = CHeciSessionInterface::PchHal_HeciSessionFactory(
                    &PAVP_GUID,
                    FALSE,
                    m_pOsServices
                    );
            }
            
            /*checks if the session is not null*/
            if (m_hPavpSession == NULL)
            {
                PCH_HAL_ASSERTMESSAGE("Failed to create HECI session");
                hr = E_OUTOFMEMORY;
                MT_ERR2(MT_CP_SESSION_CREATE, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }

            /*Create a WiDi session*/
            if (!m_hWiDiSession)
            {
                PCH_HAL_NORMALMESSAGE("Calling Heci Session factory with WIDI guid and is Widi Enabled = %d", m_IsWidiEnabled);
                m_hWiDiSession = CHeciSessionInterface::PchHal_HeciSessionFactory(
                    &WIDI_GUID,
                    m_IsWidiEnabled,
                    m_pOsServices
                    );
            }
            
            if (m_hWiDiSession == NULL)
            {
                PCH_HAL_ASSERTMESSAGE("Failed to create HECI session");
                hr = E_OUTOFMEMORY;
                MT_ERR2(MT_CP_SESSION_CREATE, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }
        }    
    } while (FALSE);

    if (FAILED(hr) &&
        m_hPavpSession != NULL)
    {
        delete m_hPavpSession;        
        m_hPavpSession = NULL;
    }
    if (FAILED(hr) &&
        m_hWiDiSession != NULL)
    {
        delete m_hWiDiSession;
        m_hWiDiSession = NULL;
    }


    PCH_HAL_FUNCTION_EXIT(hr);
    return hr;
}


HRESULT CPavpPchHalUnified::IoMessage(
    uint8_t     *pInputBuffer,
    uint32_t    InputBufferLength,
    uint8_t     *pOutputBuffer,
    uint32_t    OutputBufferLength,
    uint8_t     *pIgdData, 
    uint32_t    *IgdDataLen,
    BOOL        bIsWidiMsg) 
{
    PAVP_STATUS     status            = PAVP_STATUS_INVALID_PARAMS;
    HRESULT         hr                = S_OK;

    // Local pointer to create WiDi session or PAVP session based on bIsWidiMsg
    CHeciSessionInterface *pSession             = NULL;
    PAVPCmdHdr            *pstPavpCommandHeader = NULL;

    PCH_HAL_FUNCTION_ENTER;

    do
    {
        if (m_hPavpSession == NULL ||
            pInputBuffer  == NULL ||
            pOutputBuffer == NULL)
        {
            PCH_HAL_ASSERTMESSAGE("Invalid parameter.");
            hr = E_FAIL;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        pstPavpCommandHeader = (PAVPCmdHdr *)pInputBuffer;

        bIsWidiMsg = IS_CMD_WIDI(pstPavpCommandHeader->CommandId);
        
        pSession = bIsWidiMsg ? m_hWiDiSession : m_hPavpSession;
        
        if (!pSession)
        {
            PCH_HAL_ASSERTMESSAGE("Unable to create heci session");
            hr = E_FAIL;
            MT_ERR2(MT_CP_SESSION_CREATE, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        // FW v4.2 has been repurposed to carry slot id in the pavp header status
        // If slot ID is valid, PavpHeader::status bit[0] will be set, bits[17:2] indicates the slot ID
        // WA for sigma21_open_session output status not clean
        if (m_uiFWApiVersion >= FIRMWARE_API_VERSION_4_2 &&
            pstPavpCommandHeader->ApiVersion >= FIRMWARE_API_VERSION_4_2 &&
            m_uiStreamId != PAVP_INVALID_SESSION_ID &&
            pstPavpCommandHeader->CommandId != FIRMWARE_42_OPEN_SIGMA_SESSION)
        {
            PAVP42_CMD_STATUS_SET_APPID(pstPavpCommandHeader->PavpCmdStatus, m_uiStreamId);
            PCH_HAL_VERBOSEMESSAGE("AppID 0x%x Set in Pavp Cmd Header Status", m_uiStreamId);
        }
        else if ((m_uiFWApiVersion == FIRMWARE_API_VERSION_4_1 ||
                 (m_uiFWApiVersion >= FIRMWARE_API_VERSION_4_2 && pstPavpCommandHeader->ApiVersion <= FIRMWARE_API_VERSION_4_1)) &&
                 m_uiStreamId != PAVP_INVALID_SESSION_ID)
        {
            // FW v4.1 has been repurposed to carry slot id in the pavp header status
            // If slot ID is valid, PavpHeader::status bit[31] will be set, bits[7:0] indicates the slot ID
            PAVP_CMD_STATUS_SET_APPID(pstPavpCommandHeader->PavpCmdStatus, m_uiStreamId);
            PCH_HAL_VERBOSEMESSAGE("AppID 0x%x Set in Pavp Cmd Header Status", m_uiStreamId);
        }

        MOS_TraceEventExt(EVENT_HECI_IOMSG, EVENT_TYPE_START,
                          pInputBuffer, sizeof(PAVPCmdHdr),
                          nullptr, 0);
        MOS_TraceDumpExt("HeciMessage", 0, pInputBuffer, InputBufferLength);
        PERF_UTILITY_START("EVENT_HECI_IOMSG", PERF_CP, PERF_LEVEL_HAL);

        status = pSession->IoMessage(
                pInputBuffer,
                InputBufferLength,
                pOutputBuffer,
                OutputBufferLength,
                pIgdData,
                IgdDataLen);
        PERF_UTILITY_STOP("EVENT_HECI_IOMSG", PERF_CP, PERF_LEVEL_HAL);
        MOS_TraceDumpExt(
            "HeciMessage",
            1,
            pOutputBuffer,
            MOS_MIN(OutputBufferLength, reinterpret_cast<PAVPCmdHdr *>(pOutputBuffer)->BufferLength + sizeof(PAVPCmdHdr)));

        if (status != PAVP_STATUS_SUCCESS && status != PAVP_STATUS_INVALID_PARAMS)
        {
            // for telemetry events
            PAVPCmdHdr *pHeaderReq = reinterpret_cast<PAVPCmdHdr *>(pInputBuffer);
            PAVPCmdHdr *pHeaderRes = reinterpret_cast<PAVPCmdHdr *>(pOutputBuffer);
            MosUtilities::MosGfxInfoRTErr(
                1,
                GFXINFO_COMPID_UMD_MEDIA_CP,
                GFXINFO_ERRID_CP_PCH_FW_IO_MESSAGE,
                status,
                3,
                "data2", GFXINFO_PTYPE_UINT32, &(pHeaderReq->ApiVersion),
                "data3", GFXINFO_PTYPE_UINT32, &(pHeaderReq->CommandId),
                "data4", GFXINFO_PTYPE_UINT32, &(pHeaderRes->PavpCmdStatus));
        }

        MOS_TraceEventExt(EVENT_HECI_IOMSG, EVENT_TYPE_END,
                          pOutputBuffer, sizeof(PAVPCmdHdr),
                          &status, sizeof(status));

        if (status != PAVP_STATUS_SUCCESS)
        {
            PCH_HAL_ASSERTMESSAGE(
                "Failed sending FW CMD. "
                "status %x "
                " status description '%s'",
                status, PavpStatusToString(status) );
            MT_ERR2(MT_CP_CMD_SEND_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, status);

            if (status == PAVP_STATUS_HECI_COMMUNICATION_ERROR)
            {
                PCH_HAL_ASSERTMESSAGE("HECI communication error encountered, propagating error.");
                hr = PAVP_STATUS_HECI_COMMUNICATION_ERROR;
                MT_ERR2(MT_CP_COMMUNICATION_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }

            hr = E_FAIL;
            break;
        }

        // output buffer's ApiVersion should be non-zero value.
        if (reinterpret_cast<PAVPCmdHdr *>(pOutputBuffer)->ApiVersion == 0)
        {
            PCH_HAL_ASSERTMESSAGE("OutputBuffer is invalid. Probably FW fails to write output");
            hr = E_FAIL;
            MT_ERR2(MT_CP_INVALID_BUFFER, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }
    } while (FALSE);

    PCH_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpPchHalUnified::SessionCleanup()
{
    HRESULT     hr = S_OK;
    PCH_HAL_FUNCTION_ENTER; 
    BOOL destroyPAVPSession = FALSE;
    BOOL destroyWiDiSession = FALSE;

    do
    {
        if (m_hPavpSession == NULL)
        {
            hr = E_FAIL;
            break;
        }
        destroyPAVPSession = CHeciSessionInterface::PchHal_DestroyHeciSessionFactory(FALSE);
        
        if (destroyPAVPSession)
        {
            delete m_hPavpSession;
            m_hPavpSession = NULL;
        }


        if (m_hWiDiSession == NULL)
        {
            PCH_HAL_VERBOSEMESSAGE("WiDi: Received NULL as Session");
            break;
        }

        if (m_hWiDiSession)
        {
            destroyWiDiSession = CHeciSessionInterface::PchHal_DestroyHeciSessionFactory(m_IsWidiEnabled);
            
            if (destroyWiDiSession)
            {
                delete m_hWiDiSession;
                m_hWiDiSession = NULL;
            }
        }
    } while (FALSE);
    
    PCH_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpPchHalUnified::PerformProvisioning(std::string &stEpidResourcePath)
{
    HRESULT                     hr                          = S_OK;
    CheckEpidStatusInBuff       CheckStsIn                  = { {0} };
    CheckEpidStatusOutBuff      CheckStsOut                 = { {0} };
    SendEpidPubKeyInBuff        SendCertIn                  = { {0} };
    SendEpidPubKeyOutBuff       SendCertOut                 = { {0} };
    uint32_t                    uiGroupId                   = 0;
    uint32_t                    uiAttempts                  = 0;
    PCH_HAL_FUNCTION_ENTER;

    do
    {
        if (!m_Initialized)
        {
            PCH_HAL_ASSERTMESSAGE("PchHal is not Initialized");
            hr = E_FAIL;
            MT_ERR1(MT_CP_HAL_NOT_INITIALIZED, MT_ERROR_CODE, hr);
            break;
        }

        if (m_uiFWApiVersion > FIRMWARE_API_VERSION_4_1)
        {
            PCH_HAL_NORMALMESSAGE("Provision is not needed");
            hr = S_OK;
            break;
        }
        // FW team 
        // Heci switches to EPID_PROVISIONING_COMPLETE / EPID_NO_PUB_SAFEID_KEY after max 5s from S3
        // As such looping 50 times with 0.1 s sleep
        do
        {
            // Check whether cp_resources.bin exists
            hr = VerifyEpidResourceData(stEpidResourcePath);
            if (FAILED(hr))
            {
                PCH_HAL_ASSERTMESSAGE("ReourceData not found!");
                MT_ERR2(MT_CP_RESOURCE_DATA, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }

            // Build message header
            // According to FW team use FIRMWARE_API_VERSION_1_5 instead of m_uiFWApiVersion to remove possibility of command being rejected
            CheckStsIn.Header.ApiVersion    = FIRMWARE_API_VERSION_1_5;
            CheckStsIn.Header.BufferLength  = 0;
            CheckStsIn.Header.CommandId     = FIRMWARE_CMD_CHECK_EPID_STATUS;
            CheckStsIn.Header.PavpCmdStatus = EPID_PROV_STATUS_SUCCESS;

            hr = IoMessage(
                (uint8_t*)&CheckStsIn,
                sizeof(CheckStsIn),
                (uint8_t*)&CheckStsOut,
                sizeof(CheckStsOut),
                NULL,
                NULL
                );
            if (FAILED(hr))
            {
                PCH_HAL_ASSERTMESSAGE("Call to IO Message, Command ID %d", CheckStsIn.Header.CommandId);
                MT_ERR2(MT_CP_IO_MSG, MT_CP_COMMAND_ID, CheckStsIn.Header.CommandId, MT_ERROR_CODE, hr);
                break;
            }
            // Check if the Session Manager CheckEpIdStatus function succeeded
            if (CheckStsOut.Header.PavpCmdStatus != EPID_PROV_STATUS_SUCCESS)
            {
                PCH_HAL_ASSERTMESSAGE("Provision CheckSafeIdStatus Failed, Status = %d", CheckStsOut.Header.PavpCmdStatus);
                hr = E_FAIL;
                MT_ERR2(MT_CP_HAL_STATUS_CHECK, MT_ERROR_CODE, CheckStsOut.Header.PavpCmdStatus, MT_FUNC_RET, hr);
                break;
            }            
            // Check return EpId status
            if (CheckStsOut.EPIDStatus.ProvisionStatus == EPID_SUCCESS)
            {
                // Provisioning is not needed
                PCH_HAL_NORMALMESSAGE("Provisioning is not needed");
                break;
            }
            else if (CheckStsOut.EPIDStatus.ProvisionStatus == EPID_INIT_IN_PROGRESS || CheckStsOut.EPIDStatus.ProvisionStatus == EPID_PROVISIONING_IN_PROGRESS) 
            {
                // Session Manager is not ready for the provisioning
                PCH_HAL_NORMALMESSAGE("Provision CheckEPIDStatus in progess, Status = %d", CheckStsOut.EPIDStatus.ProvisionStatus);
                uiAttempts++;
                // if still not changing state to ready after the NUM_EPID_ATTEMPTS
                if (uiAttempts >= NUM_EPID_ATTEMPTS)
                {
                    PCH_HAL_ASSERTMESSAGE("Heci Not Ready");
                    hr = E_FAIL;
                    MT_ERR2(MT_CP_HAL_EPID_STATUS, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                    break;
                }
                else
                {
                    PCH_HAL_NORMALMESSAGE("Attempting to conntect to Heci , Attempt number %d", uiAttempts);
                    MosUtilities::MosSleep(SLEEP_300MS_PER_ATTEMPT);
                    hr = S_FALSE;
                    continue;
                }
                
            }
            else if (CheckStsOut.EPIDStatus.ProvisionStatus != EPID_NO_PUB_SAFEID_KEY) {
                // Unexpected return status
                PCH_HAL_ASSERTMESSAGE("Provision CheckEPIDStatus Failed, Status = %d", CheckStsOut.EPIDStatus.ProvisionStatus);
                hr = E_UNEXPECTED;
                MT_ERR2(MT_CP_HAL_EPID_STATUS, MT_ERROR_CODE, CheckStsOut.EPIDStatus.ProvisionStatus, MT_ERROR_CODE, hr);
                break;
            }
            /*** Step 2: Perform provisioning ***/
            uiGroupId = CheckStsOut.GroupId;
            // Build message header
            SendCertIn.Header.ApiVersion    = FIRMWARE_API_VERSION_1_5;
            SendCertIn.Header.BufferLength  = sizeof(SendEpidPubKeyInBuff)-sizeof(PAVPCmdHdr);
            SendCertIn.Header.CommandId     = FIRMWARE_CMD_SEND_EPID_PUBKEY;
            SendCertIn.Header.PavpCmdStatus = EPID_PROV_STATUS_SUCCESS;
            
            hr = GetEpidResourceData(SendCertIn, uiGroupId, stEpidResourcePath);
            if (FAILED(hr))
            {
                PCH_HAL_ASSERTMESSAGE("GetReourceData Failure!");
                MT_ERR2(MT_CP_RESOURCE_DATA, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }
            // Send the message                
            hr = IoMessage(
                (uint8_t*)&SendCertIn,
                sizeof(SendCertIn),
                (uint8_t*)&SendCertOut,
                sizeof(SendCertOut),
                NULL,
                NULL
                );

            // for telemetry events
            if (SUCCEEDED(hr) && SendCertOut.Header.PavpCmdStatus == EPID_PROV_STATUS_SUCCESS)
            {
                MosUtilities::MosGfxInfo(
                    1,
                    GFXINFO_COMPID_UMD_MEDIA_CP,
                    GFXINFO_ID_CP_EPID_PROVISIONING,
                    1,
                    "data1", GFXINFO_PTYPE_UINT32, &uiGroupId);
            }
            else
            {
                MosUtilities::MosGfxInfoRTErr(
                    1,
                    GFXINFO_COMPID_UMD_MEDIA_CP,
                    GFXINFO_ERRID_CP_EPID_PROVISIONING,
                    SendCertOut.Header.PavpCmdStatus,
                    1,
                    "data2", GFXINFO_PTYPE_UINT32, &uiGroupId);
            }

            if (FAILED(hr))
            {
                PCH_HAL_ASSERTMESSAGE("IO Message Failure!");
                MT_ERR2(MT_CP_IO_MSG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }
            // Check if the Session Manager SetCertificate function succeeded
            if (SendCertOut.Header.PavpCmdStatus != EPID_PROV_STATUS_SUCCESS) {
                // Provisioning failed
                PCH_HAL_ASSERTMESSAGE("Provisioning Failed");
                hr = E_FAIL;
                MT_ERR2(MT_CP_HAL_STATUS_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }

        } while (hr == S_FALSE);

    } while (FALSE);

    PCH_HAL_FUNCTION_EXIT(hr);
    return hr;

}

HRESULT CPavpPchHalUnified::GetEpidResourceData(SendEpidPubKeyInBuff &SendCertIn, uint32_t uiGroupId, std::string &stEpidResourcePath)
{
    HRESULT         hr                      = S_OK;
    size_t          uiEPIDParameterDataSize = 0;
    size_t          uiCertificateDataSize   = 0;
    MOS_STATUS      eStatus                 = MOS_STATUS_SUCCESS;
    std::streampos  Begin                   = 0;
    std::streampos  End                     = 0;
    char*           pResourceData           = NULL;
    EpidCert*       pEPIDCertificate        = NULL;
    char            ErrorBuffer[MAX_PATH]   = {0};
    std::ifstream   File;    

    PCH_HAL_FUNCTION_ENTER;

    do{
        if (stEpidResourcePath.length() == 0)
        {
            PCH_HAL_ASSERTMESSAGE("Path length is zero!");
            hr = E_FAIL;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }
        // Copy EPID standard parameters from binary resource
        File.open(stEpidResourcePath + FILE_EPID_CERTIFICATES_BINARY, std::ios::binary);

        if (!File.is_open())
        {
            PCH_HAL_ASSERTMESSAGE("File not found Error");
            hr = E_FAIL;
            MT_ERR2(MT_CP_PROVISION_CERT_NOT_FOUND, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

            Begin = File.tellg();
            File.seekg(0, std::ios::end);
            End = File.tellg();

            uint32_t MaxCerts = MAX_NUM_OF_EPID_CERTIFICATES;
            MOS_TraceEvent(EVENT_CP_CERT_COUNT, EVENT_TYPE_INFO, &MaxCerts, sizeof(uint32_t), nullptr, 0);

            uiEPIDParameterDataSize = (size_t)std::streampos(End - Begin);
            if (uiEPIDParameterDataSize != (MAX_NUM_OF_EPID_CERTIFICATES*sizeof(EpidCert) + sizeof(EpidParams)))
            {
                PCH_HAL_ASSERTMESSAGE(" File size incorrect, potential tempering with EPID file!");
                hr = E_FAIL;
                MT_ERR2(MT_CP_PROVISION_CERT_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                File.close();
                break;
            }
            else if (uiEPIDParameterDataSize < (sizeof(EpidCert)+sizeof(EpidParams)))
            {
                PCH_HAL_ASSERTMESSAGE("Data read too small, potential tempering with file!");
                hr = E_FAIL;
                MT_ERR2(MT_CP_PROVISION_CERT_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                File.close();
                break;
            }
            pResourceData = new (std::nothrow) char[uiEPIDParameterDataSize];
            if (pResourceData == NULL)
            {
                PCH_HAL_ASSERTMESSAGE("pResourceData Could not be Allocated!");
                hr = E_FAIL;
                MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                File.close();
                break;
            }
            File.seekg(0, std::ios::beg);
            File.read(pResourceData, uiEPIDParameterDataSize);
            File.close();
            
            PCH_HAL_NORMALMESSAGE("Data EPID Parameters Size %d", uiEPIDParameterDataSize);
            eStatus = MOS_SecureMemcpy(&(SendCertIn.GlobalParameters), sizeof(EpidParams), pResourceData, sizeof(EpidParams));

            if (eStatus != MOS_STATUS_SUCCESS)
            {
                PCH_HAL_ASSERTMESSAGE("MOS_SecureMemcpy failed with error = %d.", eStatus);
                hr = E_INVALIDARG;
                MT_ERR2(MT_CP_MEM_COPY, MT_ERROR_CODE, eStatus, MT_FUNC_RET, hr);
                break;
            }
            // Prepare and copy the certificate for the given group
            uiCertificateDataSize = (size_t)std::streampos(End - Begin) - sizeof(EpidParams);
            PCH_HAL_NORMALMESSAGE("Total Certificates Size %d", uiCertificateDataSize);
            if (uiCertificateDataSize == 0)
            {
                PCH_HAL_ASSERTMESSAGE("Data Returned is 0!");
                hr = E_FAIL;
                MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }
            // Check if having an integer number of certificates
            if (uiCertificateDataSize % sizeof(EpidCert) != 0)
            {
                PCH_HAL_ASSERTMESSAGE("Size of available certificates in not correct, division by certificate size giving non integer number");
                hr = E_FAIL;
                MT_ERR2(MT_CP_PROVISION_CERT_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }
            // Start at the beginning
            pEPIDCertificate = (EpidCert*)(pResourceData + sizeof(EpidParams));
            int nNumofCertificates = uiCertificateDataSize / sizeof(EpidCert);
            bool bGroupFound = false;
            // Move to the appropriate entry            
            for (int i = 0; i < nNumofCertificates; i++)
            {                
                if (SwapEndian32(pEPIDCertificate->Gid) == uiGroupId)
                {
                    PCH_HAL_NORMALMESSAGE("GroupID  %d, Counter %d", pEPIDCertificate->Gid, i);
                    bGroupFound = true;
                    break;
                }
                else
                {
                    pEPIDCertificate++;
                }
            } 
            if (bGroupFound == false)
            {
                MOS_TraceEvent(EVENT_CP_CERT_NOT_FOUND, EVENT_TYPE_INFO, &uiGroupId, sizeof(uint32_t), nullptr, 0);
                PCH_HAL_ASSERTMESSAGE("Certificate not found!!, Group ID %d", uiGroupId);
                hr = E_FAIL;
                MT_ERR2(MT_CP_HAL_EPID_CERT, MT_CP_GROUP_ID, uiGroupId, MT_ERROR_CODE, hr);
                break;
            }
            // Check whether not running OOB
            if ((uint8_t*)pEPIDCertificate >= (uint8_t*)pResourceData + uiCertificateDataSize + sizeof(EpidParams))
            {
                PCH_HAL_ASSERTMESSAGE("Certificate Size Mismatch");
                hr = E_FAIL;
                MT_ERR2(MT_CP_HAL_EPID_CERT, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }         
            // Copy certificate to the buffer
            eStatus = MOS_SecureMemcpy(&(SendCertIn.Certificate), sizeof(EpidCert), pEPIDCertificate, sizeof(EpidCert));
            if (eStatus)
            {
                PCH_HAL_ASSERTMESSAGE("Data Returned is 0!");
                hr = E_FAIL;
                MT_ERR2(MT_CP_MEM_COPY, MT_ERROR_CODE, eStatus, MT_FUNC_RET, hr);
                break;
            }

    } while (FALSE);

    // clear buffer
    if (pResourceData != NULL)
    {
        delete[] pResourceData;
        pResourceData = NULL;
    }

    PCH_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpPchHalUnified::VerifyEpidResourceData(std::string &stEpidResourcePath)
{
    HRESULT        hr                      = S_OK;
    std::ifstream  File;

    PCH_HAL_FUNCTION_ENTER;

    do
    {
        if (stEpidResourcePath.length() == 0)
        {
            PCH_HAL_ASSERTMESSAGE("Path length is zero!");
            hr = E_FAIL;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }
        
        File.open(stEpidResourcePath + FILE_EPID_CERTIFICATES_BINARY, std::ios::binary);

        if (!File.is_open())
        {
            PCH_HAL_ASSERTMESSAGE("File not found Error");
            hr = E_FAIL;
            MT_ERR2(MT_CP_PROVISION_CERT_NOT_FOUND, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        File.close();

    } while (FALSE);

    PCH_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpPchHalUnified::GetPlayReadyVersion(uint32_t &version)
{
    HRESULT                         hr              = S_OK;
    Pr30GetPlayReadySupportInBuf    request         = { {0, 0, 0, 0} };
    Pr30GetPlayReadySupportOutBuf   response        = { { 0, 0, 0, 0 }, 0, 0, 0, 0 };

    PCH_HAL_FUNCTION_ENTER;

    do
    {
        version = 0;

        // This command requires the driver to use FW api version 3.0
        request.Header.ApiVersion = FIRMWARE_API_VERSION_3;
        request.Header.CommandId  = FIRMWARE_CMD_GET_PR_PK_VERSION;

        hr = SendRecvCmd(
            &request, sizeof(request),
            &response, sizeof(response));
        if (response.Header.PavpCmdStatus == PAVP_STATUS_PLAYREADY_NOT_PROVISIONED)
        {
            PCH_HAL_ASSERTMESSAGE("BIOS does not support PlayReady HW DRM. Error = 0x%x.", response.Header.PavpCmdStatus);
            hr = E_FAIL;
            MT_ERR2(MT_CP_HAL_STATUS_CHECK, MT_ERROR_CODE, response.Header.PavpCmdStatus, MT_FUNC_RET, hr);
            break;
        }
        else if (response.Header.PavpCmdStatus != PAVP_STATUS_SUCCESS)
        {
            PCH_HAL_ASSERTMESSAGE("Firmware failed to execute GET PLAYREADY VERSION command. Error = 0x%x.", response.Header.PavpCmdStatus);
            hr = E_FAIL;
            MT_ERR2(MT_CP_CMD_EXECUTE_FAIL, MT_ERROR_CODE, response.Header.PavpCmdStatus, MT_FUNC_RET, hr);
            break;
        }
        else if (FAILED(hr))
        {
            PCH_HAL_ASSERTMESSAGE("Error sending GET PLAYREADY VERSION command. Error = 0x%x.", hr);
            MT_ERR2(MT_CP_CMD_SEND_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        version = response.Build;

    } while (FALSE);

    PCH_HAL_FUNCTION_EXIT(hr);
    return hr;
}

template<typename HeciIn, typename HeciOut>
HRESULT CPavpPchHalUnified::VerifyCallForTranscryptedKernels(uint32_t uiFwCommand, const AuthKernelMetaDataInfo* pInfo, 
    HeciIn& sHeciIn, uint32_t uiHeciInSize, HeciOut& sHeciOut, uint32_t uiHeciOutSize)
{
    HRESULT hr              = S_OK;

    PCH_HAL_FUNCTION_ENTER;

    do
    {
        if (!m_Initialized)
        {
            PCH_HAL_ASSERTMESSAGE("PCH HAL is not initialized.");
            hr = E_FAIL;
            MT_ERR1(MT_CP_HAL_NOT_INITIALIZED, MT_ERROR_CODE, hr);
            break;
        }

        // Verify metadata info version and be backwards compatible with empty metadata info
        if (pInfo != NULL)
        {
            if (pInfo->Version != METADATA_INFO_VERSION_0_0)
            {
                PCH_HAL_ASSERTMESSAGE("Metadata info version 0x%x is not supported.", pInfo->Version);
                hr = E_FAIL;
                MT_ERR2(MT_CP_HAL_VERIFY_TRANS_KERNEL, MT_CP_METADATA_INFO_VERSION, pInfo->Version, MT_ERROR_CODE, hr);
                break;
            }
            PCH_HAL_VERBOSEMESSAGE("Metadata info version 0x%x", pInfo->Version); 
            sHeciIn.Header.ApiVersion = pInfo->FWApiVersion;
        }
        else
        {
            PCH_HAL_VERBOSEMESSAGE("Metadata info version is not available"); 
            sHeciIn.Header.ApiVersion = FIRMWARE_API_VERSION_4;
        }

        // Verify validity of derived FW API version
        if (sHeciIn.Header.ApiVersion < FIRMWARE_API_VERSION_4)
        {
            PCH_HAL_ASSERTMESSAGE("Command 0x%08x has invalid FW API versions: 0x%x != 0x%x", uiFwCommand, 
                sHeciIn.Header.ApiVersion, m_uiFWApiVersion);
            hr = E_FAIL;
            MT_ERR3(MT_CP_HAL_VERIFY_TRANS_KERNEL, MT_CP_COMMAND, uiFwCommand, MT_CP_FW_API_VERSION, 
                                                      sHeciIn.Header.ApiVersion, MT_ERROR_CODE, hr);
            break;
        }

        if (uiFwCommand != FIRMWARE_CMD_AUTH_KERNEL_SETUP &&
            uiFwCommand != FIRMWARE_CMD_AUTH_KERNEL_SEND &&
            uiFwCommand != FIRMWARE_CMD_AUTH_KERNEL_DONE )
        {
            PCH_HAL_ASSERTMESSAGE("Invalid command 0x%08x.", uiFwCommand);
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CP_COMMAND, uiFwCommand, MT_ERROR_CODE, hr);
            break;
        }

        sHeciIn.Header.CommandId  = uiFwCommand;
        hr = SendRecvCmd(
                &sHeciIn,
                uiHeciInSize,
                &sHeciOut,
                uiHeciOutSize);

        if (FAILED(hr))
        {
            PCH_HAL_ASSERTMESSAGE("Error sending command 0x%08x. Error = 0x%x.", uiFwCommand, hr);
            MT_ERR2(MT_CP_CMD_SEND_FAIL, MT_CP_COMMAND, uiFwCommand, MT_ERROR_CODE, hr);
            break;
        }

        if (sHeciOut.Header.PavpCmdStatus != ENCRYPTED_KERNEL_SUCCESS)
        {
            PCH_HAL_ASSERTMESSAGE("Error reported from command 0x%08x. Status = 0x%x.",
                uiFwCommand, sHeciOut.Header.PavpCmdStatus);
            hr = E_FAIL;
            MT_ERR3(MT_CP_HAL_STATUS_CHECK, MT_CP_COMMAND, uiFwCommand, MT_ERROR_CODE,
                                    sHeciOut.Header.PavpCmdStatus, MT_ERROR_CODE, hr);
            break;
        }

    } while (FALSE);

    PCH_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpPchHalUnified::TranscryptKernels(
    uint32_t                uiTranscryptMetaDataSize,
    const uint32_t          *pTranscryptMetaData,
    uint32_t                uiKernelsSize,
    const uint32_t          *pEncryptedKernels,
    std::vector<BYTE>&      transcryptedKernels)
{
    HRESULT                         hr                          = S_OK;
    MOS_STATUS                      eStatus                     = MOS_STATUS_SUCCESS;

    const AuthKernelMetaDataInfo*   pInfo                       = NULL;
    AuthKernelSetupInBuf            sAuthKernelSetupIn          = {};
    AuthKernelSetupOutBuf           sAuthKernelSetupOut         = {};
    AuthKernelSendInBuf             sAuthKernelSendIn           = {};
    AuthKernelSendOutBuf            sAuthKernelSendOut          = {};
    AuthKernelDoneInBuf             sAuthKernelDoneIn           = {};
    AuthKernelDoneOutBuf            sAuthKernelDoneOut          = {};

    DWORD                           dwTransferSizeCurrent       = 0;
    DWORD                           dwTransferSizeRemaining     = 0;
    uint32_t                        uiMetaDataSize              = 0;
    uint32_t                        uiDataSize                  = 0;
    const BYTE*                     pMetaData                   = NULL;
    const BYTE*                     pEncryptedData              = NULL;
    const BYTE*                     pEncryptedDataPtr           = NULL;

    PCH_HAL_FUNCTION_ENTER;

    do
    {
        if (!m_Initialized)
        {
            PCH_HAL_ASSERTMESSAGE("PCH HAL is not initialized.");
            hr = E_FAIL;
            MT_ERR1(MT_CP_HAL_NOT_INITIALIZED, MT_ERROR_CODE, hr);
            break;
        }

        if (pTranscryptMetaData == NULL || uiTranscryptMetaDataSize == 0)
        {
            PCH_HAL_ASSERTMESSAGE("Transcrypt metadata is invalid.");
            hr = E_FAIL;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        if (pEncryptedKernels == NULL || uiKernelsSize == 0)
        {
            PCH_HAL_ASSERTMESSAGE("Encrypted kernel pointer is invalid.");
            hr = E_FAIL;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        pMetaData       = reinterpret_cast<const BYTE*>(pTranscryptMetaData);
        uiMetaDataSize  = uiTranscryptMetaDataSize;

        if (uiMetaDataSize == sizeof(AuthKernelMetaData))
        {
            PCH_HAL_VERBOSEMESSAGE("Metadata does not have version information.");
        }
        else if (uiMetaDataSize == sizeof(AuthKernelMetaDataInfo) + sizeof(AuthKernelMetaData))
        {
            pInfo           = reinterpret_cast<const AuthKernelMetaDataInfo*>(pMetaData);
            pMetaData       += sizeof(AuthKernelMetaDataInfo);
            uiMetaDataSize  -= sizeof(AuthKernelMetaDataInfo);
        }
        else
        {
            PCH_HAL_ASSERTMESSAGE("Size of metadata is incorrect.");
            hr = E_FAIL;
            MT_ERR1(MT_CP_HAL_METADATA, MT_ERROR_CODE, hr);
            break;
        }

        eStatus = MOS_SecureMemcpy(
            &sAuthKernelSetupIn.MetaData,
            sizeof(sAuthKernelSetupIn.MetaData),
            pMetaData,
            uiMetaDataSize);
        if (eStatus != MOS_STATUS_SUCCESS)
        {
            PCH_HAL_ASSERTMESSAGE("Failed to copy transcrypt metadata with error = %d.", eStatus);
            hr = E_FAIL;
            MT_ERR2(MT_CP_MEM_COPY, MT_ERROR_CODE, eStatus, MT_FUNC_RET, hr);
            break;
        }

        sAuthKernelSetupIn.AppID = EK_APP_ID;
        hr = VerifyCallForTranscryptedKernels<AuthKernelSetupInBuf, AuthKernelSetupOutBuf>(
            FIRMWARE_CMD_AUTH_KERNEL_SETUP, pInfo, sAuthKernelSetupIn, sizeof(sAuthKernelSetupIn),
            sAuthKernelSetupOut, sizeof(sAuthKernelSetupOut));
        if (FAILED(hr))
        {
            break;
        }

        pEncryptedData  = reinterpret_cast<const BYTE*>(pEncryptedKernels);
        uiDataSize      = uiKernelsSize;

        if (uiDataSize % sizeof(uint64_t) != 0)
        {
            PCH_HAL_ASSERTMESSAGE("Kernels size of 0x%x is not multiple of 8 bytes.", uiDataSize);
            hr = E_FAIL;
            MT_ERR2(MT_CP_KERNEL_RULE, MT_GENERIC_VALUE, uiDataSize, MT_ERROR_CODE, hr);
            break;
        }

        try
        {
            transcryptedKernels.clear();
            transcryptedKernels.reserve(uiDataSize);
        }
        catch (const std::exception &e)
        {
            PCH_HAL_ASSERTMESSAGE("Failed to reserve %d bytes for kernels to be transcrypted.", uiDataSize);
            PCH_HAL_ASSERTMESSAGE("Exception:  %s.", e.what());
            hr = E_OUTOFMEMORY;
            MT_ERR2(MT_CP_KERNEL_TRANSCRYPT, MT_GENERIC_VALUE, uiDataSize, MT_ERROR_CODE, hr);
            break;
        }
        pEncryptedDataPtr       = pEncryptedData;
        dwTransferSizeCurrent   = MAX_KERNEL_SIZE_PER_HECI;
        dwTransferSizeRemaining = uiDataSize;
        while (dwTransferSizeRemaining > 0)
        {
            if (dwTransferSizeRemaining < MAX_KERNEL_SIZE_PER_HECI)
            {
                dwTransferSizeCurrent = dwTransferSizeRemaining;
            }
            
            eStatus = MOS_SecureMemcpy(sAuthKernelSendIn.EncKernel, dwTransferSizeCurrent, pEncryptedDataPtr, dwTransferSizeCurrent);
            if (eStatus != MOS_STATUS_SUCCESS)
            {
                PCH_HAL_ASSERTMESSAGE("Failed to copy chunk of encrypted kernels of size 0x%x bytes to FW buffer.", dwTransferSizeCurrent);
                hr = E_FAIL;
                MT_ERR3(MT_CP_MEM_COPY, MT_GENERIC_VALUE, dwTransferSizeCurrent, MT_ERROR_CODE, eStatus, MT_FUNC_RET, hr);
                break;
            }

            hr = VerifyCallForTranscryptedKernels<AuthKernelSendInBuf, AuthKernelSendOutBuf>(
                FIRMWARE_CMD_AUTH_KERNEL_SEND, pInfo, sAuthKernelSendIn, sizeof(PAVPCmdHdr) + dwTransferSizeCurrent,
                sAuthKernelSendOut, sizeof(PAVPCmdHdr) + dwTransferSizeCurrent);
            if (FAILED(hr))
            {
                break;
            }

            try
            {
                transcryptedKernels.insert(transcryptedKernels.end(), sAuthKernelSendOut.EncKernel, sAuthKernelSendOut.EncKernel + dwTransferSizeCurrent);
            }
            catch (const std::exception &e)
            {
                PCH_HAL_ASSERTMESSAGE("Failed to copy chunk of transcrypted kernels of size 0x%x bytes from FW buffer.", dwTransferSizeCurrent);
                PCH_HAL_ASSERTMESSAGE("Exception:  %s.", e.what());
                hr = E_FAIL;
                MT_ERR2(MT_CP_KERNEL_TRANSCRYPT, MT_GENERIC_VALUE, dwTransferSizeCurrent, MT_ERROR_CODE, hr);
                break;
            }

            pEncryptedDataPtr       += dwTransferSizeCurrent;
            dwTransferSizeRemaining -= dwTransferSizeCurrent;
        }

        if (FAILED(hr))
        {
            break;
        }

        hr = VerifyCallForTranscryptedKernels<AuthKernelDoneInBuf, AuthKernelDoneOutBuf>(
            FIRMWARE_CMD_AUTH_KERNEL_DONE, pInfo, sAuthKernelDoneIn, sizeof(sAuthKernelDoneIn),
            sAuthKernelDoneOut, sizeof(sAuthKernelDoneOut));
        if (FAILED(hr))
        {
            break;
        }

        if (sAuthKernelDoneOut.KernelSize != uiDataSize)
        {
            PCH_HAL_ASSERTMESSAGE("Kernel size is 0x%x but FW received 0x%x.", uiDataSize,
                sAuthKernelDoneOut.KernelSize);
            hr = E_FAIL;
            MT_ERR3(MT_CP_KERNEL_RULE, MT_GENERIC_VALUE, uiDataSize, MT_GENERIC_VALUE,
                                    sAuthKernelDoneOut.KernelSize, MT_ERROR_CODE, hr);
            break;
        }

        if (!sAuthKernelDoneOut.IsKernelHashMatched)
        {
            PCH_HAL_ASSERTMESSAGE("FW didn't get a matching hash for the kernel.");
            hr = E_FAIL;
            MT_ERR2(MT_CP_KERNEL_RULE, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

    } while (FALSE);

    PCH_HAL_FUNCTION_EXIT(hr);
    return hr;
}
