/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2013-2021
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

File Name: cp_hal_gsc.cpp

Abstract:
Implements a class to supply access to the GSC. 

Environment:
Windows, Linux, Android

Notes:
None

======================= end_copyright_notice ==================================*/

#include "cp_hal_gsc.h"
#include "cp_os_services.h"
#include "intel_pavp_pr_api.h"

CPavpGscHal::CPavpGscHal(RootTrust_HAL_USE eGscHalUse, std::shared_ptr<CPavpOsServices> pOsServices, bool bEPIDProvisioningMode)
                            : m_EPIDProvisioningMode(bEPIDProvisioningMode), m_pOsServices(pOsServices)
{
    MediaUserSettingSharedPtr   userSettingPtr = nullptr;
#if (_DEBUG || _RELEASE_INTERNAL)
    m_GscHalUsage = eGscHalUse;
#else
    m_GscHalUsage = RootTrust_HAL_NONE;
    MOS_UNUSED(eGscHalUse);
#endif
    if (m_pOsServices && m_pOsServices->m_pOsInterface)
    {
        userSettingPtr = m_pOsServices->m_pOsInterface->pfnGetUserSettingInstance(m_pOsServices->m_pOsInterface);
    }

    MosUtilities::MosUtilitiesInit(userSettingPtr);

    // Set to latest platform to read all registry keys
    ResetMemberVariables();
}

HRESULT CPavpGscHal::Init()
{
    return E_NOTIMPL;
}

void CPavpGscHal::ResetMemberVariables()
{
    m_uiFWApiVersion = FIRMWARE_API_VERSION_1_5;
    m_Initialized = FALSE;
    m_EPIDProvisioningMode = false;
    m_uiStreamId = PAVP_INVALID_SESSION_ID;
    m_KeyBlobForINIT43.SubSessionId = 0;
    m_bHucAuthCheckReq = false;
    m_KeyBlobForINIT43.InitFlags    = {0};
    m_AuthenticatedKernelTrans      = false;

    // clear all firmware caps bits.
    m_eFwCaps.reset();
}

HRESULT CPavpGscHal::SetSlot(uint32_t uiStreamId)
{
    return E_NOTIMPL;
}

HRESULT CPavpGscHal::InvalidateSlot(uint32_t streamId)
{
    return E_NOTIMPL;
}

HRESULT CPavpGscHal::InvalidateStreamKey(uint32_t streamId)
{
    HRESULT             hr              = S_OK;
    InvStreamKeyInBuff  InvStreamKeyIn  = {{0, 0, 0, 0}, 0, 0, 0};
    InvStreamKeyOutBuff InvStreamKeyOut = {{0, 0, 0, 0}, 0};

    GSC_HAL_FUNCTION_ENTER;
    GSC_HAL_VERBOSEMESSAGE("streamId: 0x%x.", streamId);

    do
    {
        if (!m_Initialized)
        {
            GSC_HAL_ASSERTMESSAGE("Error, not initialized.");
            hr = E_FAIL;
            MT_ERR1(MT_CP_HAL_NOT_INITIALIZED, MT_ERROR_CODE, hr);
            break;
        }

        // Safety net, just in case
        if (streamId == PAVP_INVALID_SESSION_ID)
        {
            GSC_HAL_ASSERTMESSAGE("Error, Invalid Stream ID 0x%x", streamId);
            hr = E_FAIL;
            MT_ERR3(MT_ERR_INVALID_ARG, MT_CP_STREAM_ID, streamId, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }
        InvStreamKeyIn.Header.ApiVersion   = m_uiFWApiVersion;
        InvStreamKeyIn.Header.BufferLength = sizeof(InvStreamKeyIn) - sizeof(InvStreamKeyIn.Header);
        InvStreamKeyIn.StreamId            = streamId;
        InvStreamKeyIn.Header.CommandId    = FIRMWARE_CMD_INV_STREAM_KEY;
        hr = IoMessage(
            reinterpret_cast<PBYTE>(&InvStreamKeyIn),
            sizeof(InvStreamKeyIn),
            reinterpret_cast<PBYTE>(&InvStreamKeyOut),
            sizeof(InvStreamKeyOut),
            nullptr,
            nullptr);
        if (FAILED(hr))
        {
            GSC_HAL_ASSERTMESSAGE("Error sending INV_STREAM_KEY command. Error = 0x%x.", hr);
            MT_ERR1(MT_CP_HAL_FAIL, MT_ERROR_CODE, hr);
            break;
        }
        else if (InvStreamKeyOut.Header.PavpCmdStatus != PAVP_STATUS_SUCCESS)
        {
            hr = E_ABORT;
            GSC_HAL_ASSERTMESSAGE("FW returned error (0x%x) to invalid stream key.", InvStreamKeyOut.Header.PavpCmdStatus);
            MT_ERR2(MT_CP_HAL_STATUS_CHECK, MT_ERROR_CODE, InvStreamKeyOut.Header.PavpCmdStatus, MT_ERROR_CODE, hr);
            break;
        }
    } while (FALSE);

    GSC_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpGscHal::GetPlaneEnables(uint8_t uiMemoryMode, uint16_t uiPlaneEnables, uint32_t uiPlanesEnableSize, PBYTE pbEncPlaneEnables)
{
    return E_NOTIMPL;
}

HRESULT CPavpGscHal::VerifyConnStatus(uint8_t uiCsSize, PVOID pbConnectionStatus)
{
    return E_NOTIMPL;
}

HRESULT CPavpGscHal::GetNonce(bool bResetStreamKey, uint8_t uEncE0Size, uint8_t * puiEncryptedE0, uint32_t * puiNonce)
{
    return E_NOTIMPL;
}

HRESULT CPavpGscHal::VerifyMemStatus(uint8_t uPMemStatusSize, PVOID puPMemStatus)
{
    return E_NOTIMPL;
}

HRESULT CPavpGscHal::GetPlayReadyVersion(uint32_t &version)
{
    HRESULT                         hr = S_OK;
    Pr30GetPlayReadySupportInBuf    request = { { 0, 0, 0, 0 } };
    Pr30GetPlayReadySupportOutBuf   response = { { 0, 0, 0, 0 }, 0, 0, 0, 0 };

    GSC_HAL_FUNCTION_ENTER;

    do
    {
        if (!m_Initialized)
        {
            GSC_HAL_ASSERTMESSAGE("Error, GscHal not initialized.");
            hr = E_FAIL;
            MT_ERR1(MT_CP_HAL_NOT_INITIALIZED, MT_ERROR_CODE, hr);
            break;
        }

        version = 0;

        // This command requires the driver to use FW api version 3.0
        //To Do : move to latest FW API version once FW starts supporting it
        request.Header.ApiVersion = FIRMWARE_API_VERSION_3;
        request.Header.CommandId = FIRMWARE_CMD_GET_PR_PK_VERSION;
        request.Header.PavpCmdStatus = PAVP_STATUS_SUCCESS;
        request.Header.BufferLength = sizeof(request) - sizeof(request.Header);

        hr = IoMessage(
            reinterpret_cast<PBYTE>(&request),
            sizeof(request),
            reinterpret_cast<PBYTE>(&response),
            sizeof(response),
            nullptr,
            nullptr);

        if (response.Header.PavpCmdStatus == PAVP_STATUS_PLAYREADY_NOT_PROVISIONED)
        {
            GSC_HAL_ASSERTMESSAGE("BIOS does not support PlayReady HW DRM. Error = 0x%x.", response.Header.PavpCmdStatus);
            hr = E_FAIL;
            MT_ERR2(MT_CP_HAL_STATUS_CHECK, MT_ERROR_CODE, response.Header.PavpCmdStatus, MT_ERROR_CODE, hr);
            break;
        }
        else if (response.Header.PavpCmdStatus != PAVP_STATUS_SUCCESS)
        {
            GSC_HAL_ASSERTMESSAGE("Firmware failed to execute GET PLAYREADY VERSION command. Error = 0x%x.", response.Header.PavpCmdStatus);
            hr = E_FAIL;
            MT_ERR2(MT_CP_HAL_STATUS_CHECK, MT_ERROR_CODE, response.Header.PavpCmdStatus, MT_ERROR_CODE, hr);
            break;
        }
        else if (FAILED(hr))
        {
            GSC_HAL_ASSERTMESSAGE("Error sending GET PLAYREADY VERSION command. Error = 0x%x.", hr);
            MT_ERR1(MT_CP_HAL_FAIL, MT_ERROR_CODE, hr);
            break;
        }

        version = response.Build;

    } while (FALSE);

    GSC_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpGscHal::SaveKeyBlobForPAVPINIT43(void* pStoreKeyBlobParams)
{
    PAVP_STATUS status = PAVP_STATUS_SUCCESS;
    HRESULT     hr = S_OK;

    GSC_HAL_FUNCTION_ENTER;

    do
    {
        // Validate input parameters.
        if (pStoreKeyBlobParams == nullptr)
        {
            GSC_HAL_ASSERTMESSAGE("Invalid parameter.");
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        memset(&m_KeyBlobForINIT43, 0, sizeof(PAVPStoreKeyBlobInputBuff));

        if (m_uiFWApiVersion >= FIRMWARE_API_VERSION_4_3)
        {
            m_KeyBlobForINIT43 = *(PAVPStoreKeyBlobInputBuff*)pStoreKeyBlobParams;
        }
    } while (FALSE);

    GSC_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpGscHal::PassThrough(
    PBYTE       pInput,
    ULONG       ulInSize,
    PBYTE       pOutput,
    ULONG       ulOutSize,
    uint8_t     *puiIgdData,                         // Might be nullptr.
    uint32_t    *puiIgdDataLen,
    bool        bIsWidiMsg,
    uint8_t     vTag)
{
    HRESULT     hr = S_OK;

    GSC_HAL_FUNCTION_ENTER;

    do
    {
        if (!m_Initialized)
        {
            GSC_HAL_ASSERTMESSAGE("Error, GscHal not initialized.");
            hr = E_FAIL;
            MT_ERR1(MT_CP_HAL_NOT_INITIALIZED, MT_ERROR_CODE, hr);
            break;
        }

        // Validate input parameters.
        if (pInput == nullptr ||
            pOutput == nullptr ||
            ulInSize == 0 ||  // Cannot be negative.
            ulOutSize == 0)   // Cannot be negative.
        {
            GSC_HAL_ASSERTMESSAGE("Invalid parameter.");
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        GSC_HAL_VERBOSEMESSAGE("inSize 0x%x outSize 0x%x",
            ulInSize, ulOutSize);

        GSC_HAL_VERBOSEMESSAGE("Command id 0x%x", (reinterpret_cast<PAVPCmdHdr *>(pInput))->CommandId);

        hr = IoMessage(
            pInput,
            ulInSize,
            pOutput,
            ulOutSize,
            puiIgdData,
            puiIgdDataLen,
            vTag);

#if (_DEBUG || _RELEASE_INTERNAL)
        //set the driver FW private data length as 0 for FW App GUID
        if (m_GscHalUsage == RootTrust_HAL_WIDI_CLIENT_FW_APP_USE || m_GscHalUsage == RootTrust_HAL_PAVP_CLIENT_FW_APP_USE)
        {
            *puiIgdDataLen = 0;
        }
#endif
        if (FAILED(hr))
        {
            GSC_HAL_ASSERTMESSAGE("Failed sending FW Passthrough CMD. Command status 0x%x",
                                  (reinterpret_cast<PAVPCmdHdr *>(pOutput))->PavpCmdStatus);
            MT_ERR2(MT_CP_HAL_FAIL, MT_ERROR_CODE, (reinterpret_cast<PAVPCmdHdr *>(pOutput))->PavpCmdStatus,
                                                                                                 MT_ERROR_CODE, hr);
            break;
        }
    } while (FALSE);

    GSC_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpGscHal::SetFwFallback(bool bFwFallback)
{
    return E_NOTIMPL;
}

HRESULT CPavpGscHal::Cleanup()
{
    HRESULT hr = S_OK;
    GSC_HAL_FUNCTION_ENTER;

    ResetMemberVariables();

    GSC_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpGscHal::GetHeartBeatStatusNonce(uint32_t *puiNonce, uint32_t uiStreamId, PAVP_DRM_TYPE eDrmType)
{
    return E_NOTIMPL;
}

HRESULT CPavpGscHal::VerifyHeartBeatConnStatus(uint32_t uiCsSize, PVOID pbConnectionStatus, uint32_t uiStreamId, PAVP_DRM_TYPE eDrmType)
{
    return E_NOTIMPL;
}

bool CPavpGscHal::IsKernelAuthenticationSupported()
{
    GSC_HAL_FUNCTION_ENTER;
    GSC_HAL_FUNCTION_EXIT(m_AuthenticatedKernelTrans);
    return m_AuthenticatedKernelTrans;
}

bool CPavpGscHal::HasFwCaps(FIRMWARE_CAPS eFwCaps)
{
    bool bRet = false;
    GSC_HAL_FUNCTION_ENTER;

    do
    {
        if (eFwCaps > FIRMWARE_CAPS_MAX)
        {
            GSC_HAL_ASSERTMESSAGE("Invalid firmware capability requested %d", eFwCaps);
            MT_ERR2(MT_CP_HAL_FW_RULE, MT_CP_FW_CAPABILITY, eFwCaps, MT_CODE_LINE, __LINE__);
            break;
        }
        bRet = m_eFwCaps.test(eFwCaps);
    } while (false);

    GSC_HAL_FUNCTION_EXIT(bRet);
    return bRet;
}

HRESULT CPavpGscHal::SendDaisyChainCmd(StreamKey pKeyToTranslate, bool IsDKey, StreamKey pTranslatedKey)
{
    HRESULT                             hr = S_OK;
    MOS_STATUS                          ReturnStatus = MOS_STATUS_SUCCESS;
    uint8_t                             IgdData[FIRMWARE_MAX_IGD_DATA_SIZE]; // Local buffer for IGD-ME private data.
    uint32_t                            IgdDataLen = sizeof(IgdData);
    PAVPCmdHdr                          *pHeader = nullptr;
    GetDaisyChainKeyInBuf               KeyInCmd32;
    GetDaisyChainKeyOutBuf              KeyOutCmd32;
    GetDaisyChainKeyInBuf42             KeyInCmd42;
    GetDaisyChainKeyOutBuf42            KeyOutCmd42;
    void *                              KeyInCmd;
    void *                              KeyOutCmd;
    uint32_t                              KeyInCmdSize;
    uint32_t                              KeyOutCmdSize;

    GSC_HAL_FUNCTION_ENTER;

    do
    {
        if (!m_Initialized)
        {
            GSC_HAL_ASSERTMESSAGE("Error, GscHal not initialized.");
            hr = E_FAIL;
            MT_ERR1(MT_CP_HAL_NOT_INITIALIZED, MT_ERROR_CODE, hr);
            break;
        }

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
            ReturnStatus = MOS_SecureMemcpy(KeyInCmd42.stream_key, KeyInCmd42.key_size, pKeyToTranslate, sizeof(StreamKey));
            if (MOS_STATUS_SUCCESS != ReturnStatus)
            {
                GSC_HAL_ASSERTMESSAGE("MOS Secure Copy of Input Key returned failure: error = %d.", ReturnStatus);
                hr = E_FAIL;
                MT_ERR2(MT_ERR_MOS_STATUS_CHECK, MT_ERROR_CODE, ReturnStatus, MT_ERROR_CODE, hr);
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

            KeyInCmd32.Header.ApiVersion    = m_uiFWApiVersion;
            KeyInCmd32.Header.CommandId     = FIRMWARE_CMD_DAISY_CHAIN;
            KeyInCmd32.Header.PavpCmdStatus = 0;
            KeyInCmd32.Header.BufferLength  = sizeof(KeyInCmd32) - sizeof(KeyInCmd32.Header);
            KeyInCmd32.Reserved0            = 0;
            KeyInCmd32.IsDKey               = IsDKey ? 1 : 0;
            KeyInCmd32.Reserved1            = 0;

            //copy the incoming key to the command structure.
            ReturnStatus = MOS_SecureMemcpy(KeyInCmd32.SnWrappedInPrevKey, sizeof(KeyInCmd32.SnWrappedInPrevKey), pKeyToTranslate, sizeof(StreamKey));
            if (MOS_STATUS_SUCCESS != ReturnStatus)
            {
                GSC_HAL_ASSERTMESSAGE("MOS Secure Copy of Input Key returned failure: error = %d.", ReturnStatus);
                hr = E_FAIL;
                MT_ERR2(MT_ERR_MOS_STATUS_CHECK, MT_ERROR_CODE, ReturnStatus, MT_ERROR_CODE, hr);
                break;
            }
            KeyInCmd      = &KeyInCmd32;
            KeyOutCmd     = &KeyOutCmd32;
            KeyInCmdSize  = sizeof(KeyInCmd32);
            KeyOutCmdSize = sizeof(KeyOutCmd32);
        }

        hr = IoMessage(
            reinterpret_cast<PBYTE>(KeyInCmd),
            KeyInCmdSize,
            reinterpret_cast<PBYTE>(KeyOutCmd),
            KeyOutCmdSize,
            IgdData,
            &IgdDataLen);

        if (m_uiFWApiVersion > FIRMWARE_API_VERSION_4_1)
        {
            pHeader = &(reinterpret_cast<GetDaisyChainKeyOutBuf42 *>(KeyOutCmd)->Header);
        }
        else
        {
            pHeader = &(reinterpret_cast<GetDaisyChainKeyOutBuf *>(KeyOutCmd)->Header);
        }

        if (pHeader == nullptr)
        {
            GSC_HAL_ASSERTMESSAGE("Daisy chain command returned nullptr Header");
            hr = E_FAIL;
            MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        //check the return code
        if (FAILED(hr))
        {
            GSC_HAL_ASSERTMESSAGE("Daisy chain command returned failure: hr = 0x%x. Status Returned = 0x%x", hr, pHeader->PavpCmdStatus);
            MT_ERR2(MT_CP_HAL_FAIL, MT_ERROR_CODE, pHeader->PavpCmdStatus, MT_ERROR_CODE, hr);
            break;
        }

        if ((m_uiFWApiVersion > FIRMWARE_API_VERSION_4_1) && (pHeader->PavpCmdStatus == PAVP_STATUS_SUCCESS))
        {
            if (KeyOutCmd42.key_size > 0)
            {
                GSC_HAL_ASSERTMESSAGE("Length of wrapped stream key received from CSME: %d", KeyOutCmd42.key_size);
                if (KeyOutCmd42.key_size == PAVP_EPID_STREAM_KEY_LEN)
                {
                    //copy the translated key from the CSME returned command structure...
                    ReturnStatus = MOS_SecureMemcpy(pTranslatedKey, sizeof(StreamKey), KeyOutCmd42.wrapped_stream_key, PAVP_EPID_STREAM_KEY_LEN);
                    if (MOS_STATUS_SUCCESS != ReturnStatus)
                    {
                        GSC_HAL_ASSERTMESSAGE("MOS Secure Copy of CSME returned wrapped stream key returned failure: error = %d.", ReturnStatus);
                        hr = E_FAIL;
                        MT_ERR2(MT_ERR_MOS_STATUS_CHECK, MT_ERROR_CODE, ReturnStatus, MT_ERROR_CODE, hr);
                        break;
                    }
                }
                else
                {
                    GSC_HAL_ASSERTMESSAGE("Daisy chain command returned wrapped stream key != Key length. Length returned: %d.", IgdDataLen);
                    hr = E_FAIL;
                    MT_ERR2(MT_CP_HAL_KEY_RULE, MT_CP_KEY_LENGTH, IgdDataLen, MT_ERROR_CODE, hr);
                    break;
                }
            }
        }
        //since this is a private FW API, the key is returned as private data.
        else if ((m_uiFWApiVersion <= FIRMWARE_API_VERSION_4_1) && (pHeader->PavpCmdStatus == FIRMWARE_PRIVATE_INFO_FLAG))
        {
            GSC_HAL_VERBOSEMESSAGE("ME firmware returned private data.");
            pHeader->PavpCmdStatus ^= FIRMWARE_PRIVATE_INFO_FLAG;

            if (IgdDataLen > 0)
            {
                GSC_HAL_VERBOSEMESSAGE("Length of Private data received from CSME: %d", IgdDataLen);
                if (IgdDataLen == PAVP_EPID_STREAM_KEY_LEN)
                {
                    //Copy the translated key from the CSME returned command structure.
                    ReturnStatus = MOS_SecureMemcpy(pTranslatedKey, sizeof(StreamKey), IgdData, PAVP_EPID_STREAM_KEY_LEN);
                    if (MOS_STATUS_SUCCESS != ReturnStatus)
                    {
                        GSC_HAL_ASSERTMESSAGE("MOS Secure Copy of CSME returned private data returned failure: error = %d.", ReturnStatus);
                        hr = E_FAIL;
                        MT_ERR2(MT_ERR_MOS_STATUS_CHECK, MT_ERROR_CODE, ReturnStatus, MT_ERROR_CODE, hr);
                        break;
                    }
                }
                else
                {
                    GSC_HAL_ASSERTMESSAGE("Daisy chain command returned private data != Key length. Length returned: %d.", IgdDataLen);
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
            GSC_HAL_ASSERTMESSAGE("ME returned Invalid Buffer Length");
            hr = E_FAIL;
            MT_ERR2(MT_CP_HAL_STATUS_CHECK, MT_ERROR_CODE, pHeader->PavpCmdStatus, MT_ERROR_CODE, hr);
            break;
        }
        else
        {
            GSC_HAL_ASSERTMESSAGE("Daisy chain command returned no private data where expected. Status returned: 0x%x.", pHeader->PavpCmdStatus);
            hr = E_FAIL;
            MT_ERR2(MT_CP_HAL_STATUS_CHECK, MT_ERROR_CODE, pHeader->PavpCmdStatus, MT_ERROR_CODE, hr);
            break;
        }

    } while (FALSE);

    GSC_HAL_FUNCTION_EXIT(hr);
    return hr;
}

CPavpGscHal::~CPavpGscHal()
{
    MediaUserSettingSharedPtr   userSettingPtr = nullptr;
    GSC_HAL_FUNCTION_ENTER;

    if (m_pOsServices && m_pOsServices->m_pOsInterface)
    {
        userSettingPtr = m_pOsServices->m_pOsInterface->pfnGetUserSettingInstance(m_pOsServices->m_pOsInterface);
    }

    Cleanup();
    MosUtilities::MosUtilitiesClose(userSettingPtr);

    GSC_HAL_FUNCTION_EXIT(0);
}

HRESULT CPavpGscHal::WriteOutputBufferHeadMarker(MOS_RESOURCE gscOutputBuffer)
{
    HRESULT hr = E_FAIL;

    MOS_LOCK_PARAMS lockFlags {};
    lockFlags.WriteOnly = 1;
    uint32_t *pData = (uint32_t *)m_pOsServices->m_pOsInterface->pfnLockResource(
        m_pOsServices->m_pOsInterface,
        &gscOutputBuffer,
        &lockFlags);
    if (pData)
    {
        *pData = GSC_OUTPUT_BUFFER_HEAD_MARKER;
        m_pOsServices->m_pOsInterface->pfnUnlockResource(
            m_pOsServices->m_pOsInterface,
            &gscOutputBuffer);
        hr = S_OK;
    }

    return hr;
}

HRESULT CPavpGscHal::BuildSubmitGSCCommandBuffer(
    uint8_t     *pInputBuffer,
    uint32_t    InputBufferLength,
    uint8_t     *pOutputBuffer,
    uint32_t    OutputBufferLength,
    uint8_t     *pIgdData,
    uint32_t    *pIgdDataLen,
    uint8_t     vTag)
{
    HRESULT                     hr = S_OK;

    GSC_HAL_FUNCTION_ENTER;
    GSC_HAL_FUNCTION_EXIT(hr);

    return hr;
}

uint32_t CPavpGscHal::FWApiVersion()
{
    return m_uiFWApiVersion;
}

BOOL CPavpGscHal::IsInitialized()
{
    return m_Initialized;
}

HRESULT CPavpGscHal::GetFWApiVersionFromGSC(uint32_t * puiVersion)
{
    HRESULT                     hr = S_OK;
    GetCapsInBuff               sGetCapsIn = { { 0, 0, 0, 0 } };
    GetCapsOutBuff              sGetCapsOut = { { 0, 0, 0, 0 },{ PAVP_ENCRYPTION_NONE, PAVP_COUNTER_TYPE_C, PAVP_LITTLE_ENDIAN, 0,{ 0 } } };
    GetCapsInBuff42             sGetCapsIn42  = {{0, 0, 0, 0}};
    GetCapsOutBuff42            sGetCapsOut42 = {0};

    GSC_HAL_FUNCTION_ENTER;

    do
    {
        // Validate input parameters.
        if (puiVersion == nullptr)
        {
            GSC_HAL_ASSERTMESSAGE("Invalid parameter.");
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        sGetCapsIn42.Header.ApiVersion    = FIRMWARE_API_VERSION_4_2;
        sGetCapsIn42.Header.CommandId     = FIRMWARE_42_CMD_GET_PCH_CAPABILITIES;  //pavp_42_get_capabilities
        sGetCapsIn42.Header.PavpCmdStatus = PAVP_STATUS_SUCCESS;
        sGetCapsIn42.Header.BufferLength  = sizeof(sGetCapsIn42) - sizeof(sGetCapsIn42.Header);

        // First, test if fw supports the desired version
        hr = IoMessage(
            (uint8_t *)&sGetCapsIn42,
            sizeof(sGetCapsIn42),
            (uint8_t *)&sGetCapsOut42,
            sizeof(sGetCapsOut42),
            nullptr,
            nullptr);

        if ((SUCCEEDED(hr)) && 
            (sGetCapsOut42.Header.PavpCmdStatus == PAVP_STATUS_SUCCESS) && 
            (sGetCapsOut42.Header.BufferLength == sizeof(GetCapsOutBuff42) - sizeof(PAVPCmdHdr)) &&
            (sGetCapsOut42.Caps.PAVPMaxVersion >= FIRMWARE_API_VERSION_4_1))
        {
            *puiVersion = sGetCapsOut42.Caps.PAVPMaxVersion;
            GSC_HAL_VERBOSEMESSAGE("FW API version: 0x%.8x.", *puiVersion);
            break;
        }
        else // Do not fail directly for this, will fallback
        {
            GSC_HAL_VERBOSEMESSAGE("Get FW42 CAPS call received failure from FW, hr = 0x%x, PavpCmdStatus = 0x%x, BufferLength = %d, PAVPMaxVersion = %d",
                hr, sGetCapsOut42.Header.PavpCmdStatus, sGetCapsOut42.Header.BufferLength, sGetCapsOut42.Caps.PAVPMaxVersion);
        }

        // If the call did not succeed, fallback to legacy check
        sGetCapsIn.Header.ApiVersion = FIRMWARE_API_VERSION_4_1;
        sGetCapsIn.Header.CommandId = FIRMWARE_CMD_GET_PCH_CAPS;
        sGetCapsIn.Header.PavpCmdStatus = PAVP_STATUS_SUCCESS;
        sGetCapsIn.Header.BufferLength = sizeof(sGetCapsIn) - sizeof(sGetCapsIn.Header);

        hr = IoMessage(
            (uint8_t*)&sGetCapsIn,
            sizeof(sGetCapsIn),
            (uint8_t*)&sGetCapsOut,
            sizeof(sGetCapsOut),
            nullptr,
            nullptr);
        if (FAILED(hr))
        {
            GSC_HAL_ASSERTMESSAGE("Failed sending GET PCH CAPS command. Error = 0x%x", hr);
            hr = E_FAIL;
            MT_ERR1(MT_CP_HAL_FAIL, MT_ERROR_CODE, hr);
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
            GSC_HAL_VERBOSEMESSAGE("FW API version received: 0x%.8x.", *puiVersion);
        }
        else
        {
            GSC_HAL_ASSERTMESSAGE("Get PCH CAPS call received failure from FW, Status = 0x%x", sGetCapsOut.Header.PavpCmdStatus);
            hr = E_FAIL;
            MT_ERR2(MT_CP_HAL_STATUS_CHECK, MT_ERROR_CODE, sGetCapsOut.Header.PavpCmdStatus, MT_ERROR_CODE, hr);
            break;
        }
        if (*puiVersion < FIRMWARE_API_VERSION_4_1)
        {
            GSC_HAL_VERBOSEMESSAGE("Old FW version Detected: 0x%.8x.", *puiVersion);
            hr = E_FAIL;
            MT_ERR2(MT_CP_HAL_FW_RULE, MT_CP_FW_API_VERSION, *puiVersion, MT_ERROR_CODE, hr);
            break;
        }

    } while (FALSE);

    GSC_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpGscHal::IoMessage(
    uint8_t     *pInputBuffer,
    uint32_t    InputBufferLength,
    uint8_t     *pOutputBuffer,
    uint32_t    OutputBufferLength,
    uint8_t     *pIgdData,
    uint32_t    *pIgdDataLen,
    uint8_t     vTag)
{
    PAVP_STATUS     status = PAVP_STATUS_INVALID_PARAMS;
    HRESULT         hr = S_OK;

    PAVPCmdHdr            *pstPavpCommandHeader = nullptr;

    GSC_HAL_FUNCTION_ENTER;

    do
    {
        if (pInputBuffer == nullptr ||
            pOutputBuffer == nullptr)
        {
            GSC_HAL_ASSERTMESSAGE("Invalid parameter.");
            hr = E_FAIL;
            MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        pstPavpCommandHeader = (PAVPCmdHdr *)pInputBuffer;

        // FW v4.2 has been repurposed to carry slot id in the pavp header status
        // If slot ID is valid, PavpHeader::status bit[0] will be set, bits[17:2] indicates the slot ID
        // WA for sigma21_open_session output status not clean
        if (m_uiFWApiVersion >= FIRMWARE_API_VERSION_4_2 &&
            pstPavpCommandHeader->ApiVersion >= FIRMWARE_API_VERSION_4_2 &&
            m_uiStreamId != PAVP_INVALID_SESSION_ID &&
            pstPavpCommandHeader->CommandId != FIRMWARE_42_OPEN_SIGMA_SESSION)
        {
            PAVP42_CMD_STATUS_SET_APPID(pstPavpCommandHeader->PavpCmdStatus, m_uiStreamId);
            GSC_HAL_VERBOSEMESSAGE("AppID 0x%x Set in Pavp Cmd Header Status", m_uiStreamId);
        }
        else if ((m_uiFWApiVersion == FIRMWARE_API_VERSION_4_1 ||
                 (m_uiFWApiVersion >= FIRMWARE_API_VERSION_4_2 && pstPavpCommandHeader->ApiVersion <= FIRMWARE_API_VERSION_4_1)) &&
                 m_uiStreamId != PAVP_INVALID_SESSION_ID)
        {
            // FW v4.1 has been repurposed to carry slot id in the pavp header status
            // If slot ID is valid, PavpHeader::status bit[31] will be set, bits[7:0] indicates the slot ID
            PAVP_CMD_STATUS_SET_APPID(pstPavpCommandHeader->PavpCmdStatus, m_uiStreamId);
            GSC_HAL_VERBOSEMESSAGE("AppID 0x%x Set in Pavp Cmd Header Status", m_uiStreamId);
        }
        MOS_TraceEventExt(EVENT_HECI_IOMSG, EVENT_TYPE_START, pInputBuffer, sizeof(PAVPCmdHdr), nullptr, 0);
        MOS_TraceDumpExt("HeciMessageGSC", 0, pInputBuffer, InputBufferLength);

        status = BuildSubmitGSCCommandBuffer(
            pInputBuffer,
            InputBufferLength,
            pOutputBuffer,
            OutputBufferLength,
            pIgdData,
            pIgdDataLen,
            vTag);

        MOS_TraceDumpExt(
            "HeciMessageGSC",
            1,
            pOutputBuffer,
            MOS_MIN(OutputBufferLength, reinterpret_cast<PAVPCmdHdr *>(pOutputBuffer)->BufferLength + sizeof(PAVPCmdHdr)));
        MOS_TraceEventExt(EVENT_HECI_IOMSG, EVENT_TYPE_END, pOutputBuffer, sizeof(PAVPCmdHdr), &status, sizeof(status));

        if (status != PAVP_STATUS_SUCCESS)
        {
            GSC_HAL_ASSERTMESSAGE(
                "Failed sending FW CMD. "
                "status %x "
                " status description '%s'",
                status, PavpStatusToString(status));

            hr = E_FAIL;
            MT_ERR2(MT_CP_HAL_STATUS_CHECK, MT_ERROR_CODE, status, MT_ERROR_CODE, hr);
            break;
        }
    } while (FALSE);

    GSC_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpGscHal::EnableLiteModeHucStreamOutBit()
{
    HRESULT                                hr = S_OK;
    PavpCmdDbgInjectCharacteristicRegExIn  sPavpCmdDbgInjectCharacteristicRegExIn = { { 0, 0, 0, 0 }, { 0 }, { 0 } };
    PavpCmdDbgInjectCharacteristicRegExOut sPavpCmdDbgInjectCharacteristicRegExOut = { { 0, 0, 0, 0 } };

    GSC_HAL_FUNCTION_ENTER;

    do
    {
        sPavpCmdDbgInjectCharacteristicRegExIn.Header.ApiVersion = m_uiFWApiVersion;
        sPavpCmdDbgInjectCharacteristicRegExIn.Header.CommandId = PAVP_CMD_ID_DBG_INJECT_CHARACTERISTIC_REG_EX;
        sPavpCmdDbgInjectCharacteristicRegExIn.Header.BufferLength = sizeof(sPavpCmdDbgInjectCharacteristicRegExIn) - sizeof(PAVPCmdHdr);
        sPavpCmdDbgInjectCharacteristicRegExIn.Header.PavpCmdStatus = PAVP_STATUS_SUCCESS;
        //SecurePAVPCharacteristicRegsiter DW1 bit 13 is used to programming huc streamout bit
        sPavpCmdDbgInjectCharacteristicRegExIn.Mask[1] = 0x2000;
        sPavpCmdDbgInjectCharacteristicRegExIn.CharacteristicReg[1] = 0x2000;

        hr = IoMessage(
            (uint8_t *)&sPavpCmdDbgInjectCharacteristicRegExIn,
            sizeof(sPavpCmdDbgInjectCharacteristicRegExIn),
            (uint8_t *)&sPavpCmdDbgInjectCharacteristicRegExOut,
            sizeof(sPavpCmdDbgInjectCharacteristicRegExOut),
            nullptr,
            nullptr);

        if (FAILED(hr))
        {
            GSC_HAL_ASSERTMESSAGE("Error sending PavpCmdDbgInjectCharacteristicRegExIn command, Command Id 0x%x, Error = 0x%x.", sPavpCmdDbgInjectCharacteristicRegExIn.Header.CommandId, hr);
            MT_ERR2(MT_CP_HAL_FAIL, MT_CP_COMMAND_ID, sPavpCmdDbgInjectCharacteristicRegExIn.Header.CommandId, MT_ERROR_CODE, hr);
            break;
        }
        else if (sPavpCmdDbgInjectCharacteristicRegExOut.Header.PavpCmdStatus != PAVP_STATUS_SUCCESS)
        {
            hr = E_ABORT;
            GSC_HAL_ASSERTMESSAGE("PavpCmdDbgInjectCharacteristicRegExIn FW report error (0x%x).", sPavpCmdDbgInjectCharacteristicRegExOut.Header.PavpCmdStatus);
            MT_ERR2(MT_CP_HAL_STATUS_CHECK, MT_ERROR_CODE, sPavpCmdDbgInjectCharacteristicRegExOut.Header.PavpCmdStatus, MT_ERROR_CODE, hr);
            break;
        }
    } while (FALSE);

    GSC_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpGscHal::PavpInit4(
    PAVP_FW_PAVP_MODE uiPavpMode,
    uint32_t uiAppId)
{
    HRESULT             hr                      = S_OK;
    PavpInit4InBuff     sPavpInit4In            = {{0, 0, 0, 0}, 0, 0};
    PavpInit4OutBuff    sPavpInit4Out           = {{0, 0, 0, 0}};
    PavpInit43InBuff    sPavpInit43In           = { {0, 0, 0, 0}, 0, 0, 0, {0} };
    PavpInit43Flags     sPavpInit43Flags        = { 0 };
    PavpInit43OutBuff   sPavpInit43Out          = { {0, 0, 0, 0}, {0} };
    uint32_t            bIsTranscode            = uiAppId & TRANSCODE_APP_ID_MASK;
    PAVPCmdHdr          *pInBuffHdr             = nullptr;
    PAVPCmdHdr          *pOutBuffHdr            = nullptr;
    bool                bIsHucAuthenticated     = false;

    GSC_HAL_FUNCTION_ENTER;

    GSC_HAL_VERBOSEMESSAGE("AppID: 0x%x, PavpMode: 0x%x, isTranscode: 0x%x.", uiAppId, uiPavpMode, bIsTranscode);

    do
    {
        if (!m_Initialized)
        {
            GSC_HAL_ASSERTMESSAGE("Error, not initialized.");
            hr = E_FAIL;
            MT_ERR1(MT_CP_HAL_NOT_INITIALIZED, MT_ERROR_CODE, hr);
            break;
        }

        if (m_bHucAuthCheckReq)
        {
            hr = m_pOsServices->IsHucAuthenticated(bIsHucAuthenticated);
            if ((hr != S_OK ) || !bIsHucAuthenticated)
            {
                hr = E_FAIL;
                MT_ERR1(MT_CP_HUC_NOT_AUTHENTICATED, MT_ERROR_CODE, hr);
                break;
            }
        }

        if (uiAppId == PAVP_INVALID_SESSION_ID)
        {
            GSC_HAL_ASSERTMESSAGE("Error, Invalid Stream ID 0x%x", uiAppId);
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

                hr = IoMessage(
                    (uint8_t *)& sPavpInit43In,
                    sizeof(sPavpInit43In),
                    (uint8_t *)& sPavpInit43Out,
                    sizeof(sPavpInit43Out),
                    nullptr,
                    nullptr);
                break;
            }
            default:
            {
                sPavpInit4In.Header.ApiVersion = m_uiFWApiVersion;
                sPavpInit4In.Header.CommandId = FIRMWARE_CMD_PAVP_INIT4;
                sPavpInit4In.Header.PavpCmdStatus = PAVP_STATUS_SUCCESS;
                sPavpInit4In.Header.BufferLength = sizeof(sPavpInit4In) - sizeof(sPavpInit4In.Header);
                sPavpInit4In.PavpMode = uiPavpMode;
                sPavpInit4In.AppID = uiAppId;
                pInBuffHdr = reinterpret_cast<PAVPCmdHdr*>(&sPavpInit4In);
                pOutBuffHdr = reinterpret_cast<PAVPCmdHdr*>(&sPavpInit4Out);

                hr = IoMessage(
                    (uint8_t *)& sPavpInit4In,
                    sizeof(sPavpInit4In),
                    (uint8_t *)& sPavpInit4Out,
                    sizeof(sPavpInit4Out),
                    nullptr,
                    nullptr);
                break;
            }
        }

        if (FAILED(hr))
        {
            GSC_HAL_ASSERTMESSAGE("Error sending PAVP INIT command, Command Id 0x%x, Error = 0x%x.", pInBuffHdr->CommandId, hr);
            pOutBuffHdr->PavpCmdStatus = PAVP_STATUS_INTERNAL_ERROR;
            MT_ERR2(MT_CP_INIT_FW, MT_CP_COMMAND_ID, pInBuffHdr->CommandId, MT_FUNC_RET, hr);
            break;
        }
        else if (pOutBuffHdr->PavpCmdStatus != PAVP_STATUS_SUCCESS)
        {
            hr = E_ABORT;
            GSC_HAL_ASSERTMESSAGE("PAVP INIT FW report error (0x%x).", pOutBuffHdr->PavpCmdStatus);
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

    }while (FALSE);

    GSC_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpGscHal::GetEpidResourceData(SendEpidPubKeyInBuff &SendCertIn, uint32_t uiGroupId, std::string &stEpidResourcePath)
{
    HRESULT         hr = S_OK;
    size_t          uiEPIDParameterDataSize = 0;
    size_t          uiCertificateDataSize = 0;
    MOS_STATUS      eStatus = MOS_STATUS_SUCCESS;
    std::streampos  Begin = 0;
    std::streampos  End = 0;
    char*           pResourceData = nullptr;
    EpidCert*       pEPIDCertificate = nullptr;
    char            ErrorBuffer[MAX_PATH] = { 0 };
    std::ifstream   File;

    GSC_HAL_FUNCTION_ENTER;

    do {
        if (stEpidResourcePath.length() == 0)
        {
            GSC_HAL_ASSERTMESSAGE("Path length is zero!");
            hr = E_FAIL;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }
        // Copy EPID standard parameters from binary resource
        File.open(stEpidResourcePath + FILE_EPID_CERTIFICATES_BINARY, std::ios::binary);

        if (!File.is_open())
        {
            GSC_HAL_ASSERTMESSAGE("File not found Error");
            hr = E_FAIL;
            MT_ERR1(MT_CP_PROVISION_CERT_NOT_FOUND, MT_ERROR_CODE, hr);
            break;
        }

        Begin = File.tellg();
        File.seekg(0, std::ios::end);
        End = File.tellg();

        uint32_t MaxCerts = MAX_NUM_OF_EPID_CERTIFICATES;
        MOS_TraceEvent(EVENT_CP_CERT_COUNT, EVENT_TYPE_INFO, &MaxCerts, sizeof(uint32_t), nullptr, 0);

        uiEPIDParameterDataSize = (size_t)std::streampos(End - Begin);
        if (uiEPIDParameterDataSize != (MAX_NUM_OF_EPID_CERTIFICATES * sizeof(EpidCert) + sizeof(EpidParams)))
        {
            GSC_HAL_ASSERTMESSAGE(" File size incorrect, potential tempering with EPID file!");
            hr = E_FAIL;
            File.close();
            MT_ERR1(MT_CP_PROVISION_CERT_CHECK, MT_ERROR_CODE, hr);
            break;
        }
        else if (uiEPIDParameterDataSize < (sizeof(EpidCert) + sizeof(EpidParams)))
        {
            GSC_HAL_ASSERTMESSAGE("Data read too small, potential tempering with file!");
            hr = E_FAIL;
            File.close();
            MT_ERR1(MT_CP_PROVISION_CERT_CHECK, MT_ERROR_CODE, hr);
            break;
        }
        pResourceData = new (std::nothrow) char[uiEPIDParameterDataSize];
        if (pResourceData == nullptr)
        {
            GSC_HAL_ASSERTMESSAGE("pResourceData Could not be Allocated!");
            hr = E_FAIL;
            File.close();
            MT_ERR1(MT_ERR_NULL_CHECK, MT_ERROR_CODE, hr);
            break;
        }
        File.seekg(0, std::ios::beg);
        File.read(pResourceData, uiEPIDParameterDataSize);
        File.close();

        GSC_HAL_NORMALMESSAGE("Data EPID Parameters Size %d", uiEPIDParameterDataSize);
        eStatus = MOS_SecureMemcpy(&(SendCertIn.GlobalParameters), sizeof(EpidParams), pResourceData, sizeof(EpidParams));

        if (eStatus != MOS_STATUS_SUCCESS)
        {
            GSC_HAL_ASSERTMESSAGE("MOS_SecureMemcpy failed with error = %d.", eStatus);
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_MOS_STATUS_CHECK, MT_ERROR_CODE, eStatus, MT_ERROR_CODE, hr);
            break;
        }
        // Prepare and copy the certificate for the given group
        uiCertificateDataSize = (size_t)std::streampos(End - Begin) - sizeof(EpidParams);
        GSC_HAL_NORMALMESSAGE("Total Certificates Size %d", uiCertificateDataSize);
        if (uiCertificateDataSize == 0)
        {
            GSC_HAL_ASSERTMESSAGE("Data Returned is 0!");
            hr = E_FAIL;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }
        // Check if having an integer number of certificates
        if (uiCertificateDataSize % sizeof(EpidCert) != 0)
        {
            GSC_HAL_ASSERTMESSAGE("Size of available certificates in not correct, division by certificate size giving non integer number");
            hr = E_FAIL;
            MT_ERR2(MT_CP_HAL_EPID_CERT, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
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
                GSC_HAL_NORMALMESSAGE("GroupID  %d, Counter %d", pEPIDCertificate->Gid, i);
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
            GSC_HAL_ASSERTMESSAGE("Certificate not found!!, Group ID %d", uiGroupId);
            hr = E_FAIL;
            MT_ERR2(MT_CP_HAL_EPID_CERT, MT_CP_GROUP_ID, uiGroupId, MT_ERROR_CODE, hr);
            break;
        }
        // Check whether not running OOB
        if ((uint8_t*)pEPIDCertificate >= (uint8_t*)pResourceData + uiCertificateDataSize + sizeof(EpidParams))
        {
            GSC_HAL_ASSERTMESSAGE("Certificate Size Mismatch");
            hr = E_FAIL;
            MT_ERR2(MT_CP_HAL_EPID_CERT, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }
        // Copy certificate to the buffer
        eStatus = MOS_SecureMemcpy(&(SendCertIn.Certificate), sizeof(EpidCert), pEPIDCertificate, sizeof(EpidCert));
        if (eStatus)
        {
            GSC_HAL_ASSERTMESSAGE("Data Returned is 0!");
            hr = E_FAIL;
            MT_ERR2(MT_ERR_MOS_STATUS_CHECK, MT_ERROR_CODE, eStatus, MT_ERROR_CODE, hr);
            break;
        }
    } while (FALSE);

    // clear buffer
    if (pResourceData != nullptr)
    {
        delete[] pResourceData;
        pResourceData = nullptr;
    }

    GSC_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpGscHal::VerifyCallForTranscryptedKernels(uint32_t uiFwCommand, const AuthKernelMetaDataInfo* pInfo,
    uint8_t * pHeciIn, uint32_t uiHeciInSize, uint8_t * pHeciOut, uint32_t uiHeciOutSize)
{
    HRESULT             hr              = S_OK;
    PAVPCmdHdr         *pPavpHdr        = nullptr;
    MOS_STATUS          eStatus         = MOS_STATUS_SUCCESS;

    GSC_HAL_FUNCTION_ENTER;

    do
    {
        if (!m_Initialized)
        {
            GSC_HAL_ASSERTMESSAGE("GSC HAL is not initialized.");
            hr = E_FAIL;
            MT_ERR1(MT_CP_HAL_NOT_INITIALIZED, MT_ERROR_CODE, hr);
            break;
        }

        GSC_HAL_CHK_NULL(pHeciIn);
        GSC_HAL_CHK_NULL(pHeciOut);

        pPavpHdr = reinterpret_cast<PAVPCmdHdr *>(pHeciIn);

        // Verify metadata info version and be backwards compatible with empty metadata info
        if (pInfo != nullptr)
        {
            if (pInfo->Version != METADATA_INFO_VERSION_0_0)
            {
                GSC_HAL_ASSERTMESSAGE("Metadata info version 0x%x is not supported.", pInfo->Version);
                hr = E_FAIL;
                MT_ERR2(MT_CP_HAL_VERIFY_TRANS_KERNEL, MT_CP_METADATA_INFO_VERSION, pInfo->Version, MT_ERROR_CODE, hr);
                break;
            }
            GSC_HAL_VERBOSEMESSAGE("Metadata info version 0x%x", pInfo->Version);
            pPavpHdr->ApiVersion = pInfo->FWApiVersion;
        }
        else
        {
            //To Do : move to latest FW API version once FW starts supporting it
            GSC_HAL_VERBOSEMESSAGE("Metadata info version is not available");
            pPavpHdr->ApiVersion = FIRMWARE_API_VERSION_4;
        }

        // Verify validity of derived FW API version
        if (pPavpHdr->ApiVersion < FIRMWARE_API_VERSION_4)
        {
            GSC_HAL_ASSERTMESSAGE("Command 0x%08x has invalid FW API versions: 0x%x != 0x%x", uiFwCommand,
                pPavpHdr->ApiVersion, m_uiFWApiVersion);
            hr = E_FAIL;
            MT_ERR3(MT_CP_HAL_VERIFY_TRANS_KERNEL, MT_CP_COMMAND, uiFwCommand, MT_CP_FW_API_VERSION, pPavpHdr->ApiVersion, MT_ERROR_CODE, hr);
            break;
        }

        if (uiFwCommand != FIRMWARE_CMD_AUTH_KERNEL_SETUP &&
            uiFwCommand != FIRMWARE_CMD_AUTH_KERNEL_SEND &&
            uiFwCommand != FIRMWARE_CMD_AUTH_KERNEL_DONE )
        {
            GSC_HAL_ASSERTMESSAGE("Invalid command 0x%08x.", uiFwCommand);
            hr = E_INVALIDARG;
            MT_ERR2(MT_CP_HAL_VERIFY_TRANS_KERNEL, MT_CP_COMMAND, uiFwCommand, MT_ERROR_CODE, hr);
            break;
        }

        pPavpHdr->CommandId  = uiFwCommand;
        pPavpHdr->PavpCmdStatus = PAVP_STATUS_SUCCESS;
        GSC_HAL_ASSERT(uiHeciInSize >= sizeof(PAVPCmdHdr)); // Input buffer should always at least include a pavp command header.
        pPavpHdr->BufferLength = uiHeciInSize - sizeof(PAVPCmdHdr);

        hr = IoMessage(
                pHeciIn,
                uiHeciInSize,
                pHeciOut,
                uiHeciOutSize,
                nullptr,
                nullptr);
        if (FAILED(hr))
        {
            GSC_HAL_ASSERTMESSAGE("Error sending command 0x%08x. Error = 0x%x.", uiFwCommand, hr);
            MT_ERR2(MT_CP_HAL_FAIL, MT_CP_COMMAND, uiFwCommand, MT_ERROR_CODE, hr);
            break;
        }

        pPavpHdr = reinterpret_cast<PAVPCmdHdr *>(pHeciOut);
        if (pPavpHdr->PavpCmdStatus != ENCRYPTED_KERNEL_SUCCESS)
        {
            GSC_HAL_ASSERTMESSAGE("Error reported from command 0x%08x. Status = 0x%x.",
                uiFwCommand, pPavpHdr->PavpCmdStatus);
            hr = E_FAIL;
            MT_ERR3(MT_CP_HAL_STATUS_CHECK, MT_CP_COMMAND, uiFwCommand, MT_ERROR_CODE, pPavpHdr->PavpCmdStatus, MT_ERROR_CODE, hr);
            break;
        }

    } while (FALSE);

finish:
    GSC_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpGscHal::TranscryptKernels(
    uint32_t              uiTranscryptMetaDataSize,
    const uint32_t*       pTranscryptMetaData,
    uint32_t              uiKernelsSize,
    const uint32_t*       pEncryptedKernels,
    std::vector<BYTE>&  transcryptedKernels)
{
    HRESULT                         hr = S_OK;
    MOS_STATUS                      eStatus = MOS_STATUS_SUCCESS;

    const AuthKernelMetaDataInfo*   pInfo = nullptr;
    AuthKernelSetupInBuf            sAuthKernelSetupIn = {};
    AuthKernelSetupOutBuf           sAuthKernelSetupOut = {};
    AuthKernelSendInBuf             sAuthKernelSendIn = {};
    AuthKernelSendOutBuf            sAuthKernelSendOut = {};
    AuthKernelDoneInBuf             sAuthKernelDoneIn = {};
    AuthKernelDoneOutBuf            sAuthKernelDoneOut = {};

    DWORD                           dwTransferSizeCurrent = 0;
    DWORD                           dwTransferSizeRemaining = 0;
    uint32_t                          uiMetaDataSize = 0;
    uint32_t                          uiDataSize = 0;
    const BYTE*                     pMetaData = nullptr;
    const BYTE*                     pEncryptedData = nullptr;
    const BYTE*                     pEncryptedDataPtr = nullptr;

    GSC_HAL_FUNCTION_ENTER;

    do
    {
        if (!m_Initialized)
        {
            GSC_HAL_ASSERTMESSAGE("GSC HAL is not initialized.");
            hr = E_FAIL;
            MT_ERR1(MT_CP_HAL_NOT_INITIALIZED, MT_ERROR_CODE, hr);
            break;
        }

        if (pTranscryptMetaData == nullptr || uiTranscryptMetaDataSize == 0)
        {
            GSC_HAL_ASSERTMESSAGE("Transcrypt metadata is invalid.");
            hr = E_FAIL;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        if (pEncryptedKernels == nullptr || uiKernelsSize == 0)
        {
            GSC_HAL_ASSERTMESSAGE("Encrypted kernel pointer is invalid.");
            hr = E_FAIL;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        pMetaData = reinterpret_cast<const BYTE*>(pTranscryptMetaData);
        uiMetaDataSize = uiTranscryptMetaDataSize;

        if (uiMetaDataSize == sizeof(AuthKernelMetaData))
        {
            GSC_HAL_VERBOSEMESSAGE("Metadata does not have version information.");
        }
        else if (uiMetaDataSize == sizeof(AuthKernelMetaDataInfo) + sizeof(AuthKernelMetaData))
        {
            pInfo = reinterpret_cast<const AuthKernelMetaDataInfo*>(pMetaData);
            pMetaData += sizeof(AuthKernelMetaDataInfo);
            uiMetaDataSize -= sizeof(AuthKernelMetaDataInfo);
        }
        else
        {
            GSC_HAL_ASSERTMESSAGE("Size of metadata is incorrect.");
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
            GSC_HAL_ASSERTMESSAGE("Failed to copy transcrypt metadata with error = %d.", eStatus);
            hr = E_FAIL;
            MT_ERR2(MT_ERR_MOS_STATUS_CHECK, MT_ERROR_CODE, eStatus, MT_ERROR_CODE, hr);
            break;
        }

        sAuthKernelSetupIn.AppID = EK_APP_ID;
        hr = VerifyCallForTranscryptedKernels(
            FIRMWARE_CMD_AUTH_KERNEL_SETUP, pInfo, (uint8_t *)&sAuthKernelSetupIn, sizeof(sAuthKernelSetupIn),
            (uint8_t *)&sAuthKernelSetupOut, sizeof(sAuthKernelSetupOut));
        if (FAILED(hr))
        {
            MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        pEncryptedData = reinterpret_cast<const BYTE*>(pEncryptedKernels);
        uiDataSize = uiKernelsSize;

        if (uiDataSize % sizeof(uint64_t) != 0)
        {
            GSC_HAL_ASSERTMESSAGE("Kernels size of 0x%x is not multiple of 8 bytes.", uiDataSize);
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
            GSC_HAL_ASSERTMESSAGE("Failed to reserve %d bytes for kernels to be transcrypted.", uiDataSize);
            GSC_HAL_ASSERTMESSAGE("Exception:  %s.", e.what());
            hr = E_OUTOFMEMORY;
            MT_ERR2(MT_CP_KERNEL_TRANSCRYPT, MT_GENERIC_VALUE, uiDataSize, MT_ERROR_CODE, hr);
            break;
        }
        pEncryptedDataPtr = pEncryptedData;
        dwTransferSizeCurrent = MAX_KERNEL_SIZE_PER_HECI;
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
                GSC_HAL_ASSERTMESSAGE("Failed to copy chunk of encrypted kernels of size 0x%x bytes to FW buffer.", dwTransferSizeCurrent);
                hr = E_FAIL;
                MT_ERR3(MT_ERR_MOS_STATUS_CHECK, MT_GENERIC_VALUE, dwTransferSizeCurrent, MT_ERROR_CODE, eStatus, MT_ERROR_CODE, hr);
                break;
            }

            hr = VerifyCallForTranscryptedKernels(
                FIRMWARE_CMD_AUTH_KERNEL_SEND, pInfo, (uint8_t *)&sAuthKernelSendIn, sizeof(PAVPCmdHdr) + dwTransferSizeCurrent,
                (uint8_t *)&sAuthKernelSendOut, sizeof(PAVPCmdHdr) + dwTransferSizeCurrent);
            if (FAILED(hr))
            {
                MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }

            try
            {
                transcryptedKernels.insert(transcryptedKernels.end(), sAuthKernelSendOut.EncKernel, sAuthKernelSendOut.EncKernel + dwTransferSizeCurrent);
            }
            catch (const std::exception &e)
            {
                GSC_HAL_ASSERTMESSAGE("Failed to copy chunk of transcrypted kernels of size 0x%x bytes from FW buffer.", dwTransferSizeCurrent);
                GSC_HAL_ASSERTMESSAGE("Exception:  %s.", e.what());
                hr = E_FAIL;
                MT_ERR2(MT_CP_KERNEL_TRANSCRYPT, MT_GENERIC_VALUE, dwTransferSizeCurrent, MT_ERROR_CODE, hr);
                break;
            }

            pEncryptedDataPtr += dwTransferSizeCurrent;
            dwTransferSizeRemaining -= dwTransferSizeCurrent;
        }

        if (FAILED(hr))
        {
            MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        hr = VerifyCallForTranscryptedKernels(
            FIRMWARE_CMD_AUTH_KERNEL_DONE, pInfo, (uint8_t *)&sAuthKernelDoneIn, sizeof(sAuthKernelDoneIn),
            (uint8_t *)&sAuthKernelDoneOut, sizeof(sAuthKernelDoneOut));
        if (FAILED(hr))
        {
            MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        if (sAuthKernelDoneOut.KernelSize != uiDataSize)
        {
            GSC_HAL_ASSERTMESSAGE("Kernel size is 0x%x but FW received 0x%x.", uiDataSize,
                sAuthKernelDoneOut.KernelSize);
            hr = E_FAIL;
            MT_ERR3(MT_CP_KERNEL_RULE, MT_GENERIC_VALUE, uiDataSize, MT_GENERIC_VALUE, sAuthKernelDoneOut.KernelSize, MT_ERROR_CODE, hr);
            break;
        }

        if (!sAuthKernelDoneOut.IsKernelHashMatched)
        {
            GSC_HAL_ASSERTMESSAGE("FW didn't get a matching hash for the kernel.");
            hr = E_FAIL;
            MT_ERR1(MT_CP_KERNEL_RULE, MT_ERROR_CODE, hr);
            break;
        }

    } while (FALSE);

    GSC_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpGscHal::PerformProvisioning(std::string &stEpidResourcePath)
{
    HRESULT                      hr = S_OK;
    CheckEpidStatusInBuff        CheckStsIn = { { 0 } };
    CheckEpidStatusOutBuff       CheckStsOut = { { 0 } };
    SendEpidPubKeyInBuff         SendCertIn = { { 0 } };
    SendEpidPubKeyOutBuff        SendCertOut = { { 0 } };
    uint32_t                       uiGroupId = 0;
    uint32_t                       uiAttempts = 0;
    GSC_HAL_FUNCTION_ENTER;

    do
    {
        if (!m_Initialized)
        {
            GSC_HAL_ASSERTMESSAGE("GscHal is not Initialized");
            hr = E_FAIL;
            MT_ERR1(MT_CP_HAL_NOT_INITIALIZED, MT_ERROR_CODE, hr);
            break;
        }

        if (m_uiFWApiVersion > FIRMWARE_API_VERSION_4_1)
        {
            GSC_HAL_NORMALMESSAGE("Provision is not needed");
            hr = S_OK;
            break;
        }

        // Heci switches to EPID_PROVISIONING_COMPLETE / EPID_NO_PUB_SAFEID_KEY after max 5s from S3
        // As such looping 50 times with 0.1 s sleep
        do
        {
            // Build message header
            CheckStsIn.Header.ApiVersion = FIRMWARE_API_VERSION_1_5;
            CheckStsIn.Header.BufferLength = 0;
            CheckStsIn.Header.CommandId = FIRMWARE_CMD_CHECK_EPID_STATUS;
            CheckStsIn.Header.PavpCmdStatus = EPID_PROV_STATUS_SUCCESS;

            hr = IoMessage(
                (uint8_t*)&CheckStsIn,
                sizeof(CheckStsIn),
                (uint8_t*)&CheckStsOut,
                sizeof(CheckStsOut),
                nullptr,
                nullptr
            );
            if (FAILED(hr))
            {
                GSC_HAL_ASSERTMESSAGE("Error sending FIRMWARE CMD CHECK EPID STATUS. Error = 0x%x", hr);
                MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }
            // Check if the Session Manager CheckEPIDStatus function succeeded
            if (CheckStsOut.Header.PavpCmdStatus != EPID_PROV_STATUS_SUCCESS)
            {
                GSC_HAL_ASSERTMESSAGE("FW returned error for FIRMWARE CMD CHECK EPID STATUS, Status = 0x%x", CheckStsOut.Header.PavpCmdStatus);
                hr = E_FAIL;
                MT_ERR2(MT_CP_HAL_STATUS_CHECK, MT_ERROR_CODE, CheckStsOut.Header.PavpCmdStatus, MT_ERROR_CODE, hr);
                break;
            }
            // Check return EPID status
            if (CheckStsOut.EPIDStatus.ProvisionStatus == EPID_SUCCESS)
            {
                // Provisioning is not needed
                GSC_HAL_NORMALMESSAGE("Provisioning is not needed");
                break;
            }
            else if (CheckStsOut.EPIDStatus.ProvisionStatus == EPID_INIT_IN_PROGRESS || CheckStsOut.EPIDStatus.ProvisionStatus == EPID_PROVISIONING_IN_PROGRESS)
            {
                // Session Manager is not ready for the provisioning
                GSC_HAL_NORMALMESSAGE("Provision CheckEPIDStatus in progess, Status = %d", CheckStsOut.EPIDStatus.ProvisionStatus);
                uiAttempts++;
                // if still not changing state to ready after the NUM_EPID_ATTEMPTS
                if (uiAttempts >= NUM_EPID_ATTEMPTS)
                {
                    GSC_HAL_ASSERTMESSAGE("Heci Not Ready");
                    hr = E_FAIL;
                    MT_ERR2(MT_CP_HAL_EPID_STATUS, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                    break;
                }
                else
                {
                    GSC_HAL_NORMALMESSAGE("Attempting to conntect to Heci , Attempt number %d", uiAttempts);
                    MosUtilities::MosSleep(SLEEP_300MS_PER_ATTEMPT);
                    hr = S_FALSE;
                    continue;
                }
            }
            else if (CheckStsOut.EPIDStatus.ProvisionStatus != EPID_NO_PUB_SAFEID_KEY) {
                // Unexpected return status
                GSC_HAL_ASSERTMESSAGE("Provision CheckEPIDStatus Failed, Status = %d", CheckStsOut.EPIDStatus.ProvisionStatus);
                hr = E_UNEXPECTED;
                MT_ERR2(MT_CP_HAL_EPID_STATUS, MT_ERROR_CODE, CheckStsOut.EPIDStatus.ProvisionStatus, MT_ERROR_CODE, hr);
                break;
            }
            /*** Step 2: Perform provisioning ***/
            uiGroupId = CheckStsOut.GroupId;
            // Build message header
            SendCertIn.Header.ApiVersion = FIRMWARE_API_VERSION_1_5;
            SendCertIn.Header.BufferLength = sizeof(SendCertIn) - sizeof(SendCertIn.Header);
            SendCertIn.Header.CommandId = FIRMWARE_CMD_SEND_EPID_PUBKEY;
            SendCertIn.Header.PavpCmdStatus = EPID_PROV_STATUS_SUCCESS;

            hr = GetEpidResourceData(SendCertIn, uiGroupId, stEpidResourcePath);
            if (FAILED(hr))
            {
                GSC_HAL_ASSERTMESSAGE("GetReourceData Failure!");
                MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }
            // Send the message
            hr = IoMessage(
                (uint8_t*)&SendCertIn,
                sizeof(SendCertIn),
                (uint8_t*)&SendCertOut,
                sizeof(SendCertOut),
                nullptr,
                nullptr);

            if (FAILED(hr))
            {
                GSC_HAL_ASSERTMESSAGE("Call to FIRMWARE CMD SEND EPID PUBKEY failed. Error = 0x%x", hr);
                MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }
            // Check if the Session Manager SetCertificate function succeeded
            if (SendCertOut.Header.PavpCmdStatus != EPID_PROV_STATUS_SUCCESS) {
                // Provisioning failed
                GSC_HAL_ASSERTMESSAGE("FW returned error for Firmware Send EPID Pubkey. Error = 0x%x", SendCertOut.Header.PavpCmdStatus);
                hr = E_FAIL;
                MT_ERR2(MT_CP_HAL_STATUS_CHECK, MT_ERROR_CODE, SendCertOut.Header.PavpCmdStatus, MT_ERROR_CODE, hr);
                break;
            }

        } while (hr == S_FALSE);

    } while (FALSE);

    GSC_HAL_FUNCTION_EXIT(hr);
    return hr;
}
