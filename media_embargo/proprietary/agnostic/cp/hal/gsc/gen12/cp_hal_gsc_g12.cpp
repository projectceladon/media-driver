/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2020-2023
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

File Name: cp_hal_gsc_g12.cpp

Abstract:
Implements a class to supply access to the GSC.

Environment:
Windows, Linux, Android

Notes:
None

======================= end_copyright_notice ==================================*/

#include "cp_hal_gsc_g12.h"
#include "cp_os_services.h"
#include "intel_pavp_pr_api.h"

CPavpGscHalG12::CPavpGscHalG12(RootTrust_HAL_USE eGscHalUse, std::shared_ptr<CPavpOsServices> pOsServices, bool bEPIDProvisioningMode)
                                : CPavpGscHal(eGscHalUse, pOsServices, bEPIDProvisioningMode)
{
    ResetMemberVariables();
}

CPavpGscHalG12::~CPavpGscHalG12()
{
    GSC_HAL_FUNCTION_ENTER;

    Cleanup();

    GSC_HAL_FUNCTION_EXIT(0);
}

HRESULT CPavpGscHalG12::Init()
{
    HRESULT hr = S_OK;
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    GSC_HAL_FUNCTION_ENTER;

    do
    {
        if (m_Initialized)
        {
            GSC_HAL_NORMALMESSAGE("Initializing already initialized CPavpGscHal.");
            hr = E_UNEXPECTED;
            break;
        }

        if (m_pOsServices == nullptr)
        {
            hr = E_UNEXPECTED;
            break;
        }

        //create and set GSC context
        hr = m_pOsServices->CreateGSCGPUContext();
        if (FAILED(hr))
        {
            GSC_HAL_ASSERTMESSAGE("failed to create GSC conext");
            MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        //Allocate resource to copy input/output buffers
        hr = AllocateGSCResource();
        if (FAILED(hr))
        {
            GSC_HAL_ASSERTMESSAGE("failed to allocate resource");
            MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        hr = m_pOsServices->NotifyGSCBBCompleteEvent();
        if (FAILED(hr))
        {
            GSC_HAL_ASSERTMESSAGE("Could not register for batch buffer completion notification. Error = 0x%x", eStatus);
            hr = E_FAIL;
            MT_ERR3(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, eStatus, MT_ERROR_CODE, hr);
            break;
        }

#if (_DEBUG || _RELEASE_INTERNAL)
        if (m_GscHalUsage != RootTrust_HAL_WIDI_CLIENT_FW_APP_USE &&
            m_GscHalUsage != RootTrust_HAL_PAVP_CLIENT_FW_APP_USE &&
            m_GscHalUsage != RootTrust_HAL_MKHI_CLIENT_FW_APP_USE &&
            m_GscHalUsage != RootTrust_HAL_GSC_PROXY_CLIENT_FW_APP_USE)
#endif
        {
            //get fw api version
            hr = GetFWApiVersionFromGSC(&m_uiFWApiVersion);
            if (FAILED(hr))
            {
                GSC_HAL_ASSERTMESSAGE("Could not find correct FW API version. Error = 0x%x.", hr);
                MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
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
                    m_eFwCaps.set(FIRMWARE_CAPS_PAVP_INIT);
                    m_eFwCaps.set(FIRMWARE_CAPS_LSPCON);
                    m_eFwCaps.set(FIRMWARE_CAPS_MULTI_SESSION);
                default :
                    break;
            }

            GSC_HAL_NORMALMESSAGE("Init successful. GSC FW API version is 0x%.8x", m_uiFWApiVersion);
        }
        m_Initialized = TRUE;
    } while (FALSE);

    if (FAILED(hr))
    {
        Cleanup();
    }

    GSC_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpGscHalG12::Cleanup()
{
    HRESULT hr = S_OK;
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;
    GSC_HAL_FUNCTION_ENTER;

    if (m_pOsServices == nullptr ||
        m_pOsServices->m_pOsInterface == nullptr ||
        m_pOsServices->m_pOsInterface->osCpInterface == nullptr)
    {
        hr = E_FAIL;
    }
    else
    {
        if (m_pGSCInputOutput != nullptr)
        {
            eStatus = m_pOsServices->m_pOsInterface->osCpInterface->DeAllocateTEEPhysicalBuffer(static_cast<void*>(m_pGSCInputOutput));
            if (MOS_FAILED(eStatus))
            {
                GSC_HAL_ASSERTMESSAGE("Failed to deallocate GSC resource. Error = 0x%x.", eStatus);
                hr = E_FAIL;
                MT_ERR2(MT_CP_HAL_FAIL, MT_ERROR_CODE, eStatus, MT_ERROR_CODE, hr);
            }
            m_pGSCInputOutput = nullptr;
        }
    }

    ResetMemberVariables();
    GSC_HAL_FUNCTION_EXIT(hr);
    return hr;
}

void CPavpGscHalG12::ResetMemberVariables()
{
    CPavpGscHal::ResetMemberVariables();
    m_AuthenticatedKernelTrans = FALSE;
    m_pGSCInputOutput = nullptr;
    m_BufferSize = 0;
}

//To Do: UMD has to use GSC specific memory allocation function after GMM team adds support for it
HRESULT CPavpGscHalG12::AllocateGSCResource()
{
    HRESULT hr = S_OK;
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    GSC_HAL_FUNCTION_ENTER;

    do
    {
        if (m_pOsServices == nullptr ||
            m_pOsServices->m_pOsInterface == nullptr ||
            m_pOsServices->m_pOsInterface->osCpInterface == nullptr ||
            m_pGSCInputOutput != nullptr ||
            m_BufferSize != 0)
        {
            hr = E_UNEXPECTED;
            break;
        }

        eStatus = (MOS_STATUS)m_pOsServices->m_pOsInterface->osCpInterface->AllocateTEEPhysicalBuffer(
                    reinterpret_cast<void**>(&m_pGSCInputOutput),
                    &m_BufferSize);

        if (MOS_FAILED(eStatus) || m_pGSCInputOutput == nullptr || m_BufferSize != GSC_MAX_INPUT_BUFF_SIZE + GSC_MAX_OUTPUT_BUFF_SIZE)
        {
            GSC_HAL_ASSERTMESSAGE("GSC resource was not allocated as expected");
            hr = E_OUTOFMEMORY;
            MT_ERR2(MT_CP_HAL_FAIL, MT_ERROR_CODE, eStatus, MT_ERROR_CODE, hr);
            break;
        }
    } while (FALSE);

    GSC_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpGscHalG12::BuildSubmitGSCCommandBuffer(
    uint8_t     *pInputBuffer,
    uint32_t    InputBufferLength,
    uint8_t     *pOutputBuffer,
    uint32_t    OutputBufferLength,
    uint8_t     *pIgdData,
    uint32_t *   pIgdDataLen,
    uint8_t     vTag)
{
    HRESULT     hr = S_OK;
    MOS_STATUS  eStatus = MOS_STATUS_SUCCESS;
    uint8_t     HeciClient = 0;

    GSC_HAL_FUNCTION_ENTER;

    do
    {
        if (m_pOsServices == nullptr)
        {
            hr = E_UNEXPECTED;
            break;
        }

        if (pInputBuffer == nullptr || pOutputBuffer == nullptr)
        {
            GSC_HAL_ASSERTMESSAGE("Invalid parameter.");
            hr = E_FAIL;
            MT_ERR1(MT_ERR_NULL_CHECK, MT_ERROR_CODE, hr);
            break;
        }

        PAVPCmdHdr* pInHdr = reinterpret_cast<PAVPCmdHdr *>(pInputBuffer);
        HeciClient = IS_CMD_WIDI(pInHdr->CommandId) ? WIDI_ME_FIX_CLIENT : PAVP_ME_FIX_CLIENT;

#if (_DEBUG || _RELEASE_INTERNAL)
        switch (m_GscHalUsage)
        {
            case RootTrust_HAL_WIDI_CLIENT_FW_APP_USE :
                HeciClient = WIDI_ME_FIX_CLIENT;
                break;
            case RootTrust_HAL_PAVP_CLIENT_FW_APP_USE :
            case RootTrust_HAL_GSC_PROXY_CLIENT_FW_APP_USE :
                HeciClient = PAVP_ME_FIX_CLIENT;
                break;
            case RootTrust_HAL_MKHI_CLIENT_FW_APP_USE :
                HeciClient = MKHI_ME_FIX_CLIENT;
                break;
            default :
                break;
        }
#endif

        // InputBuffer length should be less than or equal to Max GSC input buffer.
        // OutputBuffer length should be less than or equal to Max GSC output buffer.
        if (GSC_MAX_INPUT_BUFF_SIZE < InputBufferLength ||
            GSC_MAX_OUTPUT_BUFF_SIZE < OutputBufferLength)
        {
            GSC_HAL_ASSERTMESSAGE("Input buffer length or output buffer length to submit larger than max buffer size");
            GSC_HAL_ASSERTMESSAGE("InputBufferLength %d maxInputBufferSize %d", InputBufferLength, GSC_MAX_INPUT_BUFF_SIZE);
            GSC_HAL_ASSERTMESSAGE("OutputBufferLength %d maxOutputBufferSize %d", OutputBufferLength, GSC_MAX_OUTPUT_BUFF_SIZE);
            hr = E_INVALIDARG;
            MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        hr = WriteResourceInputBuffer(pInputBuffer,
            InputBufferLength);
        if (FAILED(hr))
        {
            GSC_HAL_ASSERTMESSAGE("Failed to update Resoruce input buffer Error = 0x%x.", hr);
            MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        // zero out the UMD FW output buffer so application does not consume stale data in case FW fails to write output
        MOS_ZeroMemory(m_pGSCInputOutput + GSC_MAX_INPUT_BUFF_SIZE, OutputBufferLength);

        // write data present in output buffer being sent by application
        // Application will send vtag in Output buffer for Chrome Widevine usage so don't set output buffer to zero
        eStatus = MOS_SecureMemcpy(m_pGSCInputOutput + GSC_MAX_INPUT_BUFF_SIZE, OutputBufferLength, pOutputBuffer, OutputBufferLength);
        if (MOS_FAILED(eStatus))
        {
            GSC_HAL_ASSERTMESSAGE("Failed to copy output GSC buffer. Error = 0x%x.", eStatus);
            hr = E_FAIL;
            break;
        }

        uint32_t RecvBytes = 0;
        hr = m_pOsServices->CreateSubmitGSCCommandBuffer(m_pGSCInputOutput,
                                                         InputBufferLength,
                                                         OutputBufferLength,
                                                         HeciClient,
                                                         &RecvBytes);
        if (FAILED(hr))
        {
            GSC_HAL_ASSERTMESSAGE("Error submit GSC command buffer. Error = 0x%x.", hr);
            MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        hr = ReadResourceOutputBuffer(pOutputBuffer, OutputBufferLength, pIgdData, pIgdDataLen, RecvBytes);
        if (FAILED(hr))
        {
            GSC_HAL_ASSERTMESSAGE("Error reading GSC output buffer. Error = 0x%x.", hr);
            MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }
        // output buffer's ApiVersion should be non-zoro value.
        if (reinterpret_cast<PAVPCmdHdr *>(pOutputBuffer)->ApiVersion == 0)
        {
            GSC_HAL_ASSERTMESSAGE("OutputBuffer is invalid. Probably FW fails to write output");
            hr = E_FAIL;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

    } while (FALSE);

    GSC_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpGscHalG12::ReadResourceOutputBuffer(
    uint8_t     *pOutputBuffer,
    uint32_t    OutputBufferLength,
    uint8_t     *pIgdData,
    uint32_t    *pIgdDataLen,
    uint32_t    RecvSize)
{
    HRESULT hr = S_OK;
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    GSC_HAL_FUNCTION_ENTER;

    do
    {
        if (m_pOsServices == nullptr ||
            m_pOsServices->m_pOsInterface == nullptr ||
            m_pGSCInputOutput == nullptr)
        {
            hr = E_UNEXPECTED;
            break;
        }

        PAVPCmdHdr *pOutHdr = reinterpret_cast<PAVPCmdHdr *>(m_pGSCInputOutput + GSC_MAX_INPUT_BUFF_SIZE);

        uint32_t PavpAppMsgSize = sizeof(PAVPCmdHdr) + pOutHdr->BufferLength;
        uint32_t PrivateDataLength = 0;
        //WA for FW issue: output bufferlength of pavp_42_get_stream_key should be 132 instead of 136
        if (pOutHdr->CommandId == FIRMWARE_42_GET_STREAM_KEY)
        {
            PavpAppMsgSize = 132;
        }

        if (PavpAppMsgSize > OutputBufferLength)
        {
            GSC_HAL_ASSERTMESSAGE("Buffer received from FW is too large ");
            hr = E_FAIL;
            MT_ERR2(MT_CP_BUFFER_RULE, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        // Output buffer length sent by FW should be less than max GSC output buffer.
        GSC_HAL_ASSERT(GSC_MAX_OUTPUT_BUFF_SIZE >= PavpAppMsgSize);

        eStatus = MOS_SecureMemcpy(pOutputBuffer, PavpAppMsgSize, m_pGSCInputOutput + GSC_MAX_INPUT_BUFF_SIZE, PavpAppMsgSize);
        if (MOS_FAILED(eStatus))
        {
            GSC_HAL_ASSERTMESSAGE("Failed to copy output GSC buffer. Error = 0x%x.", eStatus);
            hr = E_FAIL;
            MT_ERR2(MT_CP_MEM_COPY, MT_ERROR_CODE, eStatus, MT_ERROR_CODE, hr);
            break;
        }

        if (pOutHdr->PavpCmdStatus == FIRMWARE_PRIVATE_INFO_FLAG)
        {
            if (pIgdDataLen == nullptr || pIgdData == nullptr)
            {
                GSC_HAL_ASSERTMESSAGE("UMD FW private data pointer is not initliased");
                hr = E_UNEXPECTED;
                MT_ERR1(MT_ERR_NULL_CHECK, MT_ERROR_CODE, hr);
                break;
            }

            if (pOutHdr->ApiVersion <= FIRMWARE_API_VERSION_4_1)
            {
                if (RecvSize != 0) // if private data is zero, use hard coded value
                {
                    *pIgdDataLen = RecvSize - (sizeof(PAVPCmdHdr) + pOutHdr->BufferLength);
                    GSC_HAL_ASSERT(*pIgdDataLen > 0); //Since FW has sent private data flag, private data should be greater than zero
                }
                else
                {
                    *pIgdDataLen = PAVP_EPID_STREAM_KEY_LEN;  //current design supports private data of fixed 16 bytes size
                }
                // PavpAppMsgSize + private data length should be less than Max GSC output buffer.
                GSC_HAL_ASSERT(GSC_MAX_OUTPUT_BUFF_SIZE >= PavpAppMsgSize + *pIgdDataLen);

                eStatus = MOS_SecureMemcpy(pIgdData, *pIgdDataLen, m_pGSCInputOutput + GSC_MAX_INPUT_BUFF_SIZE + PavpAppMsgSize, *pIgdDataLen);
                if (MOS_FAILED(eStatus))
                {
                    GSC_HAL_ASSERTMESSAGE("Failed to copy private data. Error = 0x%x.", eStatus);
                    hr = E_FAIL;
                    MT_ERR2(MT_CP_MEM_COPY, MT_ERROR_CODE, eStatus, MT_ERROR_CODE, hr);
                    break;
                }
                // Private data length is *pIgdDataLen for FW41-
                PrivateDataLength = *pIgdDataLen;
            }
            else
            {
                eStatus = MOS_SecureMemcpy(pIgdDataLen, SIGMA_PRI_DATE_LEN_SIZE, m_pGSCInputOutput + GSC_MAX_INPUT_BUFF_SIZE + PavpAppMsgSize, SIGMA_PRI_DATE_LEN_SIZE);
                if (MOS_FAILED(eStatus))
                {
                    GSC_HAL_ASSERTMESSAGE("Failed to copy private data length. Error = 0x%x.", eStatus);
                    hr = E_FAIL;
                    MT_ERR2(MT_CP_MEM_COPY, MT_ERROR_CODE, eStatus, MT_ERROR_CODE, hr);
                    break;
                }

                // PavpAppMsgSize + private data length should be less than Max GSC output buffer.
                GSC_HAL_ASSERT(GSC_MAX_OUTPUT_BUFF_SIZE >= PavpAppMsgSize + *pIgdDataLen);

                eStatus = MOS_SecureMemcpy(pIgdData, *pIgdDataLen, m_pGSCInputOutput + GSC_MAX_INPUT_BUFF_SIZE + PavpAppMsgSize + SIGMA_PRI_DATE_LEN_SIZE, *pIgdDataLen);
                if (MOS_FAILED(eStatus))
                {
                    GSC_HAL_ASSERTMESSAGE("Failed to copy private data. Error = 0x%x.", eStatus);
                    hr = E_FAIL;
                    MT_ERR2(MT_CP_MEM_COPY, MT_ERROR_CODE, eStatus, MT_ERROR_CODE, hr);
                    break;
                }

                //Private data length is SIGMA_PRI_DATE_LEN_SIZE + *pIgdDataLen for FW42+
                PrivateDataLength = *pIgdDataLen + SIGMA_PRI_DATE_LEN_SIZE;
            }

#if (_DEBUG || _RELEASE_INTERNAL)
            // Copy private data out to app
            if (((m_GscHalUsage == RootTrust_HAL_WIDI_CLIENT_FW_APP_USE) ||
                 (m_GscHalUsage == RootTrust_HAL_PAVP_CLIENT_FW_APP_USE) ||
                 (m_GscHalUsage == RootTrust_HAL_MKHI_CLIENT_FW_APP_USE) ||
                 (m_GscHalUsage == RootTrust_HAL_GSC_PROXY_CLIENT_FW_APP_USE)) &&
                (PavpAppMsgSize + PrivateDataLength <= OutputBufferLength))
            {
                eStatus = MOS_SecureMemcpy(pOutputBuffer + PavpAppMsgSize, PrivateDataLength,
                    m_pGSCInputOutput + GSC_MAX_INPUT_BUFF_SIZE + PavpAppMsgSize, PrivateDataLength);
                if (MOS_FAILED(eStatus))
                {
                    GSC_HAL_ASSERTMESSAGE("Failed to copy output private GSC data . Error = 0x%x.", eStatus);
                    hr = E_FAIL;
                    MT_ERR2(MT_CP_MEM_COPY, MT_ERROR_CODE, eStatus, MT_ERROR_CODE, hr);
                    break;
                }
            }
            else
            {
                GSC_HAL_NORMALMESSAGE("Failed to copy output private GSC data , invalid output buffer size or GscHalUsage.");
            }
#endif
        }
    } while(FALSE);

    GSC_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpGscHalG12::WriteResourceInputBuffer(
    uint8_t     *pInputBuffer,
    uint32_t    InputBufferLength)
{
    HRESULT hr = S_OK;
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    GSC_HAL_FUNCTION_ENTER;

    do
    {
        if (m_pOsServices == nullptr ||
            m_pOsServices->m_pOsInterface == nullptr ||
            m_pGSCInputOutput == nullptr)
        {
            hr = E_UNEXPECTED;
            break;
        }

        //copying input buffer
        eStatus = MOS_SecureMemcpy(m_pGSCInputOutput, InputBufferLength, pInputBuffer, InputBufferLength);
        if (MOS_FAILED(eStatus))
        {
            GSC_HAL_ASSERTMESSAGE("Failed to copy input GSC buffer. Error = 0x%x.", eStatus);
            hr = E_FAIL;
            MT_ERR2(MT_CP_MEM_COPY, MT_ERROR_CODE, eStatus, MT_ERROR_CODE, hr);
            break;
        }
    } while (FALSE);

    GSC_HAL_FUNCTION_EXIT(hr);
    return hr;
}
