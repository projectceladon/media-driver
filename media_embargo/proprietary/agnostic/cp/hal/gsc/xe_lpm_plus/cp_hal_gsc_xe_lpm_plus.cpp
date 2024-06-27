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

File Name: cp_hal_gsc_xe_lpm_plus.cpp

Abstract:
Implements a class to supply access to the GSC.

Environment:
Windows, Linux, Android

Notes:
None

======================= end_copyright_notice ==================================*/

#include "cp_hal_gsc_xe_lpm_plus.h"
#include "cp_os_services.h"
#include "mhw_cp_xe_lpm_plus.h"
#include "hal_oca_interface.h"
#include "intel_pavp_pr_api.h"
#include "cp_debug_ext.h"

CPavpGscHalG13::CPavpGscHalG13(RootTrust_HAL_USE eGscHalUse, std::shared_ptr<CPavpOsServices> pOsServices, bool bEPIDProvisioningMode)
                                : CPavpGscHal (eGscHalUse, pOsServices, bEPIDProvisioningMode)
{
    ResetMemberVariables();
}

CPavpGscHalG13::~CPavpGscHalG13()
{
    GSC_HAL_FUNCTION_ENTER;

    Cleanup();

    GSC_HAL_FUNCTION_EXIT(0);
}

HRESULT CPavpGscHalG13::Init()
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

        GSC_HAL_CHK_NULL(m_pOsServices);
        GSC_HAL_CHK_NULL(m_pOsServices->m_pOsInterface);
        GSC_HAL_CHK_NULL(m_pOsServices->m_pOsInterface->pfnRegisterBBCompleteNotifyEvent);

        // Create MHW interfaces if needed
        if (nullptr == m_pMhwInterfaces)
        {
            MhwInterfacesNext::CreateParams params;
            params.m_isCp    = true;
            m_pMhwInterfaces = MhwInterfacesNext::CreateFactory(params, m_pOsServices->m_pOsInterface);

            if (nullptr == m_pMhwInterfaces ||
                nullptr == m_pMhwInterfaces->m_cpInterface ||
                nullptr == m_pMhwInterfaces->m_miItf)
            {
                GSC_HAL_ASSERTMESSAGE("MhwInterfaces::CreateFactory returned a NULL instance.");
                MT_ERR(MT_ERR_NULL_CHECK);
                hr = E_UNEXPECTED;
                break;
            }
        }

        //create and set GSC context
        hr = m_pOsServices->CreateGSCGPUContext();
        if (FAILED(hr))
        {
            GSC_HAL_ASSERTMESSAGE("Failed to create GSC conext");
            MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        //Allocate resource to copy input/output buffers
        hr = AllocateGSCResource();
        if (FAILED(hr))
        {
            GSC_HAL_ASSERTMESSAGE("Failed to allocate resource");
            MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        hr = m_pOsServices->AllocateGSCHostSessionHandle(&m_GSCHostSessionHandle);
        if (FAILED(hr))
        {
           GSC_HAL_ASSERTMESSAGE("Failed to allocate GSC host session handle");
           MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
           break;
        }

        GSC_HAL_CHK_STATUS_MESSAGE_WITH_HR(m_pOsServices->m_pOsInterface->pfnRegisterBBCompleteNotifyEvent(
            m_pOsServices->m_pOsInterface,
            MOS_GPU_CONTEXT_TEE),
            "Failed to register GSC BB complete notify event, Error 0x%x", eStatus) ;

        if (m_pOsServices->m_pOsInterface->osCpInterface->GetOcaDumper() == nullptr)
        {
            m_pOsServices->m_pOsInterface->osCpInterface->CreateOcaDumper();
        }

#if (_DEBUG || _RELEASE_INTERNAL)
        if (m_GscHalUsage != RootTrust_HAL_WIDI_CLIENT_FW_APP_USE &&
            m_GscHalUsage != RootTrust_HAL_PAVP_CLIENT_FW_APP_USE &&
            m_GscHalUsage != RootTrust_HAL_MKHI_CLIENT_FW_APP_USE &&
            m_GscHalUsage != RootTrust_HAL_GSC_PROXY_CLIENT_FW_APP_USE )
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

finish:
    if (FAILED(hr))
    {
        Cleanup();
    }

    GSC_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpGscHalG13::Cleanup()
{
    HRESULT                 hr          = S_OK;
    MOS_STATUS              eStatus     = MOS_STATUS_SUCCESS;

    GSC_HAL_FUNCTION_ENTER;

    do
    {
        GSC_HAL_CHK_NULL(m_pOsServices);
        GSC_HAL_CHK_NULL(m_pOsServices->m_pOsInterface);
        GSC_HAL_CHK_NULL(m_pOsServices->m_pOsInterface->pfnFreeResource);

        m_pOsServices->m_pOsInterface->pfnFreeResource(
            m_pOsServices->m_pOsInterface,
            &m_GSCInputBuffer);

        m_pOsServices->m_pOsInterface->pfnFreeResource(
            m_pOsServices->m_pOsInterface,
            &m_GSCOutputBuffer);

        m_pOsServices->FreeGSCHostSessionHandle(m_GSCHostSessionHandle);

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

        ResetMemberVariables();
    } while (FALSE);

finish:
    GSC_HAL_FUNCTION_EXIT(hr);
    return hr;
}

void CPavpGscHalG13::ResetMemberVariables()
{
    CPavpGscHal::ResetMemberVariables();
    m_AuthenticatedKernelTrans = FALSE;

    memset(&m_GSCInputBuffer, 0, sizeof(m_GSCInputBuffer));
    memset(&m_GSCOutputBuffer, 0, sizeof(m_GSCOutputBuffer));
    memset(&m_GSCCSMemoryhdr, 0, sizeof(m_GSCCSMemoryhdr));
    m_pMhwInterfaces        = nullptr;
    m_GSCHostSessionHandle = 0;

    m_maxInputBufferSize    = GSC_MAX_INPUT_BUFF_SIZE_XE_LPM_PLUS;
    m_maxOutputBufferSize   = GSC_MAX_OUTPUT_BUFF_SIZE_XE_LPM_PLUS;
    m_bHucAuthCheckReq      = true;     // authenticated huc check is required from MTL+
}

HRESULT CPavpGscHalG13::AllocateGSCResource()
{
    HRESULT         hr              = S_OK;
    MOS_STATUS      eStatus         = MOS_STATUS_SUCCESS;

    GSC_HAL_FUNCTION_ENTER;

    do
    {
        GSC_HAL_CHK_NULL(m_pOsServices);
        GSC_HAL_CHK_NULL(m_pOsServices->m_pOsInterface);
        GSC_HAL_CHK_NULL(m_pOsServices->m_pOsInterface->pfnAllocateResource);

        MOS_ALLOC_GFXRES_PARAMS allocParams;
        MOS_ZeroMemory(&allocParams, sizeof(MOS_ALLOC_GFXRES_PARAMS));
        allocParams.Type            = MOS_GFXRES_BUFFER;
        allocParams.TileType        = MOS_TILE_LINEAR;
        allocParams.Format          = Format_Buffer;
        allocParams.dwBytes         = m_maxInputBufferSize;
        allocParams.ResUsageType    = MOS_HW_RESOURCE_USAGE_CP_INTERNAL_WRITE;
        allocParams.pBufName        = "GSC HECI input command buffer";

        GSC_HAL_CHK_STATUS_MESSAGE_WITH_HR(m_pOsServices->m_pOsInterface->pfnAllocateResource(
            m_pOsServices->m_pOsInterface,
            &allocParams,
            &m_GSCInputBuffer),
            "Failed to allocate %s", allocParams.pBufName);

        MOS_ZeroMemory(&allocParams, sizeof(MOS_ALLOC_GFXRES_PARAMS));
        allocParams.Type            = MOS_GFXRES_BUFFER;
        allocParams.TileType        = MOS_TILE_LINEAR;
        allocParams.Format          = Format_Buffer;
        allocParams.dwBytes         = m_maxOutputBufferSize;
        allocParams.ResUsageType    = MOS_HW_RESOURCE_USAGE_CP_INTERNAL_WRITE;
        allocParams.pBufName        = "GSC HECI output command buffer";

        GSC_HAL_CHK_STATUS_MESSAGE_WITH_HR(m_pOsServices->m_pOsInterface->pfnAllocateResource(
             m_pOsServices->m_pOsInterface,
             &allocParams,
             &m_GSCOutputBuffer),
             "Failed to allocate %s", allocParams.pBufName);

    } while (FALSE);

finish:
    GSC_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpGscHalG13::BuildSubmitGSCCommandBuffer(
    uint8_t     *pInputBuffer,
    uint32_t    InputBufferLength,
    uint8_t     *pOutputBuffer,
    uint32_t    OutputBufferLength,
    uint8_t     *pIgdData,
    uint32_t    *pIgdDataLen,
    uint8_t     vTag)
{
    HRESULT             hr                      = S_OK;
    MOS_STATUS          eStatus                 = MOS_STATUS_SUCCESS;
    MOS_COMMAND_BUFFER  CmdBuffer;
    uint32_t            SWProxyCheckCnt         = 0;
    uint32_t            SWProxyRetryCnt         = 0;
    uint8_t             HeciClient              = 0;
    uint32_t            SWProxyNotReadyRetryCnt = 0;
    uint8_t             *pExtension             = nullptr;

    GSC_HAL_FUNCTION_ENTER;

    GSC_HAL_CHK_NULL(pInputBuffer);
    GSC_HAL_CHK_NULL(pOutputBuffer);
    GSC_HAL_CHK_NULL(m_pOsServices);

    do
    {
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

        uint32_t extensionSize = 0;
        hr = m_pOsServices->GetGSCHeaderExtensionSize(&extensionSize);
        if (FAILED(hr))
        {
           GSC_HAL_ASSERTMESSAGE("Failed to obtain GSC extension size. Error = 0x%x.", hr);
           MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
           break;
        }
        else if (extensionSize != 0)
        {
            if (extensionSize < sizeof(vTag))
            {
                GSC_HAL_ASSERTMESSAGE("unexpected header length");
                break;
            }
            if (pExtension == nullptr)
            {
                pExtension = (uint8_t *)MOS_AllocAndZeroMemory(extensionSize);
                GSC_HAL_CHK_NULL(pExtension);
            }
            hr = m_pOsServices->WriteGSCHeaderExtension(
                pExtension,
                &vTag,
                extensionSize);
        
            if (FAILED(hr))
            {
                GSC_HAL_ASSERTMESSAGE("Failed to write GSC Header");
                MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }
        }

        
        uint32_t submitInputLength = sizeof(GSC_CS_MEMORY_HEADER)+ extensionSize + InputBufferLength ;
        uint32_t submitOutputLength = sizeof(GSC_CS_MEMORY_HEADER) + extensionSize + OutputBufferLength  + FIRMWARE_MAX_IGD_DATA_SIZE;

#if (_DEBUG || _RELEASE_INTERNAL)
        submitInputLength = (m_GscHalUsage == RootTrust_HAL_GSC_PROXY_CLIENT_FW_APP_USE) ? InputBufferLength : submitInputLength;
        submitOutputLength = (m_GscHalUsage == RootTrust_HAL_GSC_PROXY_CLIENT_FW_APP_USE) ? OutputBufferLength : submitOutputLength;
#endif

        if (m_maxInputBufferSize < submitInputLength || 
            m_maxOutputBufferSize < submitOutputLength)
        {
            GSC_HAL_ASSERTMESSAGE("Input buffer length or output buffer length to submit larger than max buffer size");
            GSC_HAL_ASSERTMESSAGE("submitInputLength %d maxInputBufferSize %d", submitInputLength, m_maxInputBufferSize);
            GSC_HAL_ASSERTMESSAGE("submitOutputLength %d maxOutputBufferSize %d", submitOutputLength, m_maxOutputBufferSize);
            hr = E_INVALIDARG;
            MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        hr = WriteResourceInputBuffer(pInputBuffer, InputBufferLength, HeciClient, pExtension, extensionSize);
        if (FAILED(hr))
        {
           GSC_HAL_ASSERTMESSAGE("Failed to write resource input buffer. Error = 0x%x.", hr);
           MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
           break;
        }

        hr = WriteOutputBufferHeadMarker(m_GSCOutputBuffer);
        if (FAILED(hr))
        {
            GSC_HAL_ASSERTMESSAGE("Failed to write gsc output buffer marker. Error = 0x%x.", hr);
            MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        bool cmdBufferRequiredForSubmit = false;
        hr = m_pOsServices->GetGSCCommandBufferRequired(cmdBufferRequiredForSubmit);
        if (FAILED(hr))
        {
            GSC_HAL_ASSERTMESSAGE("Failed to know if command buffer is required. Error = 0x%x.", hr);
            break;
        }

        if(cmdBufferRequiredForSubmit)
        {
            hr = m_pOsServices->GetGSCCommandBuffer(&CmdBuffer);
            if (FAILED(hr))
            {
                GSC_HAL_ASSERTMESSAGE("Failed to get command buffer. Error = 0x%x.", hr);
                MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }

            hr = BuildCommandBuffer(&CmdBuffer, submitInputLength, submitOutputLength);
            if (FAILED(hr))
            {
                GSC_HAL_ASSERTMESSAGE("Failed to build command buffer. Error = 0x%x.", hr);
                MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }

            hr = m_pOsServices->SubmitGSCCommandBuffer(&CmdBuffer);
            if (FAILED(hr))
            {
                GSC_HAL_ASSERTMESSAGE("Failed to Submit GSCCommand Buffer. Error = 0x%x.", hr);
                MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }
        }
        else
        {
            hr = m_pOsServices->SubmitGSCCommand(&m_GSCInputBuffer, &m_GSCOutputBuffer, submitInputLength, submitOutputLength);
            if (FAILED(hr))
            {
                GSC_HAL_ASSERTMESSAGE("Failed to Submit GSCCommand. Error = 0x%x.", hr);
                MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }
        }

        hr = ReadResourceOutputBuffer(pOutputBuffer, OutputBufferLength, pIgdData, pIgdDataLen);
        if (hr == E_ABORT)
        {
            if (SWProxyCheckCnt < SW_PROXY_CHECK_MAX_NUM_RETRY)
            {
                SWProxyCheckCnt++;
                GSC_HAL_VERBOSEMESSAGE("SW proxy check failed, retry ... %d", SWProxyCheckCnt);
                MosUtilities::MosSleep(SW_PROXY_CHECK_RETRY_INTERVAL);
                continue;
            }
            else
            {
                GSC_HAL_ASSERTMESSAGE("SW proxy check failed after maximum number of retry");
                hr = E_TIMEOUT;
                MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }
        }
        else if (FAILED(hr))
        {
            GSC_HAL_ASSERTMESSAGE("Failed to read resource output buffer. Error = 0x%x.", hr);
            MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

#if (_DEBUG || _RELEASE_INTERNAL)
        if (RootTrust_HAL_GSC_PROXY_CLIENT_FW_APP_USE == m_GscHalUsage)
        {
            // FW team requests directy return output for GSC passthrough client without status check.
            break;
        }
#endif

        if (GSCCS_MEMORY_HEADER_PROXY_NOT_READY == m_GSCCSMemoryhdr.DW8.status)
        {
            if (SWProxyNotReadyRetryCnt < SW_PROXY_NOT_READY_MAX_NUM_RETRY)
            {
                GSC_HAL_NORMALMESSAGE("SW proxy is not ready, need retry from UMD");
                SWProxyNotReadyRetryCnt++;
                MosUtilities::MosSleep(SW_PROXY_NOT_READY_RETRY_INTERVAL);
                continue;
            }
            else
            {
                GSC_HAL_ASSERTMESSAGE("SW proxy still not ready after maximum number of retry");
                hr = E_TIMEOUT;
                MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }
        }

        if (m_GSCCSMemoryhdr.DW7.pending)
        {
            if (SWProxyRetryCnt < SW_PROXY_MAX_RETRY_POLLING)
            {
                GSC_HAL_NORMALMESSAGE("SW proxy is needed, GSC firmware returns pending status");
                SWProxyRetryCnt++;
                MosUtilities::MosSleep(SW_PROXY_POLLING_INTERVAL);
                continue;
            }
            else
            {
                GSC_HAL_ASSERTMESSAGE("Pending status is returned after maximum number of retry");
                hr = E_TIMEOUT;
                MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }
        }

#if PAVP_PRE_SILICON_ENABLED 
        if (m_pOsServices && m_pOsServices->IsSimulationEnabled())
        {
            // skip ApiVersion check
            GSC_HAL_NORMALMESSAGE("presi environment! skip ApiVersion check!");
        }
        else
#endif
        {   // output buffer's ApiVersion should be non-zero value.
            if (reinterpret_cast<PAVPCmdHdr *>(pOutputBuffer)->ApiVersion == 0)
            {
                GSC_HAL_ASSERTMESSAGE("OutputBuffer is invalid. Probably FW fails to write output");
                hr = E_FAIL;
                MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }
        }

        break;

    } while (TRUE);

finish:

    if (pExtension)
    {
        MOS_SafeFreeMemory(pExtension);
        pExtension = nullptr;
    }
    GSC_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpGscHalG13::ReadResourceOutputBuffer(
    uint8_t     *pOutputBuffer,
    uint32_t    OutputBufferLength,
    uint8_t     *pIgdData,
    uint32_t    *pIgdDataLen,
    uint32_t    RecvBytes)
{
    HRESULT             hr                  = S_OK;
    MOS_STATUS          eStatus             = MOS_STATUS_SUCCESS;
    uint8_t*            pHeciOutputData     = nullptr;
    uint32_t            PavpAppMsgSize      = 0;
    uint32_t            PrivDataOffset      = 0;
    uint32_t            PrivDataLength      = 0;
    CpOcaDumper*        ocaDumper           = nullptr;
    MOS_UNUSED(RecvBytes);
    
    GSC_HAL_FUNCTION_ENTER;

    GSC_HAL_CHK_NULL(pOutputBuffer);
    GSC_HAL_CHK_NULL(m_pOsServices);
    GSC_HAL_CHK_NULL(m_pOsServices->m_pOsInterface);
    GSC_HAL_CHK_NULL(m_pOsServices->m_pOsInterface->pfnLockResource);
    GSC_HAL_CHK_NULL(m_pOsServices->m_pOsInterface->pfnUnlockResource);
    GSC_HAL_CHK_NULL(m_pOsServices->m_pOsInterface->pfnGetResourceGfxAddress);
    GSC_HAL_CHK_NULL(m_pOsServices->m_pOsInterface->osCpInterface);

    do
    {
        MOS_LOCK_PARAMS lockFlags {};
        lockFlags.ReadOnly = 1;
        pHeciOutputData = (uint8_t*)m_pOsServices->m_pOsInterface->pfnLockResource(
            m_pOsServices->m_pOsInterface,
            &m_GSCOutputBuffer,
            &lockFlags);
        GSC_HAL_CHK_NULL(pHeciOutputData);

        if (HECI_VALIDITY_MARKER == *(uint32_t *)pHeciOutputData)
        {
             GSC_HAL_CHK_STATUS_MESSAGE_WITH_HR(MOS_SecureMemcpy(
                &m_GSCCSMemoryhdr,
                sizeof(GSC_CS_MEMORY_HEADER),
                pHeciOutputData,
                sizeof(GSC_CS_MEMORY_HEADER)),
                "Failed to copy memory. Error = 0x%x.", eStatus);

             MOS_TraceDumpExt("GSCCS Memory header", 0, pHeciOutputData, sizeof(GSC_CS_MEMORY_HEADER));

#if (_DEBUG || _RELEASE_INTERNAL)
             // In case of GSC proxy pass through, directly return GSCCS memory
             // header to application for the negative case validation.
             if (m_GscHalUsage == RootTrust_HAL_GSC_PROXY_CLIENT_FW_APP_USE)
             {
                 GSC_HAL_CHK_STATUS_MESSAGE_WITH_HR(MOS_SecureMemcpy(
                     pOutputBuffer,
                     sizeof(GSC_CS_MEMORY_HEADER),
                     pHeciOutputData,
                     sizeof(GSC_CS_MEMORY_HEADER)),
                     "Failed to copy memory. Error = 0x%x.", eStatus);

                 pOutputBuffer += sizeof(GSC_CS_MEMORY_HEADER);
             }
#endif
            GSC_CS_MEMORY_HEADER *pGSCHdr = reinterpret_cast<GSC_CS_MEMORY_HEADER *>(pHeciOutputData);
            // skip the extension in output buffer as there is no use of extension in output buffer for now
            pHeciOutputData += sizeof(GSC_CS_MEMORY_HEADER) + pGSCHdr->DW7.extension_size;

             // Upon sleep resume, KMD need a time window to establish sw proxy, UMD need to retry to send
             // HECI command. In sw proxy not ready case, the output data is invalid and should be ignored.
             // And, if GSC firmware returns pending status, PAVP payload is ignored.
             if (GSCCS_MEMORY_HEADER_PROXY_NOT_READY == m_GSCCSMemoryhdr.DW8.status || m_GSCCSMemoryhdr.DW7.pending)
             {
                 GSC_HAL_NORMALMESSAGE("SW proxy is not ready or firmware returns pending status, PAVP command buffer is ignored");
                 hr = S_OK;
                 break;
             }
        }
        else if (GSC_OUTPUT_BUFFER_HEAD_MARKER == *(uint32_t *)pHeciOutputData)
        {
            GSC_HAL_VERBOSEMESSAGE("Found output buffer head marker, output buffer is aborted");
            hr = E_ABORT;
            break;
        }

        PAVPCmdHdr *pOutHdr = reinterpret_cast<PAVPCmdHdr *>(pHeciOutputData);
        PavpAppMsgSize = sizeof(PAVPCmdHdr) + pOutHdr->BufferLength;

#if (_DEBUG || _RELEASE_INTERNAL)
        if (m_GscHalUsage == RootTrust_HAL_GSC_PROXY_CLIENT_FW_APP_USE)
        {
            PavpAppMsgSize = OutputBufferLength;
        }
#endif

        if (PavpAppMsgSize > OutputBufferLength)
        {
            GSC_HAL_ASSERTMESSAGE("Buffer received from FW is too large ");
            hr = E_FAIL;
            MT_ERR2(MT_CP_BUFFER_RULE, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        if (PavpAppMsgSize > m_maxOutputBufferSize)
        {
            GSC_HAL_ASSERTMESSAGE("PavpAppMsgSize %d sent by FW should be less than m_maxOutputBufferSize %d",
                PavpAppMsgSize,
                m_maxOutputBufferSize);
            hr = E_FAIL;
            MT_ERR2(MT_CP_BUFFER_RULE, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        GSC_HAL_CHK_STATUS_MESSAGE_WITH_HR(MOS_SecureMemcpy(
            pOutputBuffer,
            PavpAppMsgSize,
            pHeciOutputData,
            PavpAppMsgSize),
            "Failed to copy output GSC buffer. Error = 0x%x.", eStatus);

        if (pOutHdr->PavpCmdStatus == FIRMWARE_PRIVATE_INFO_FLAG)
        {
            GSC_HAL_CHK_NULL(pIgdDataLen);
            GSC_HAL_CHK_NULL(pIgdData);

            if (pOutHdr->ApiVersion <= FIRMWARE_API_VERSION_4_1)
            {
                *pIgdDataLen = PAVP_EPID_STREAM_KEY_LEN;  //current design supports private data of fixed 16 bytes size
                PrivDataOffset = PavpAppMsgSize;
                PrivDataLength = *pIgdDataLen;  //private data length is *pIgdDataLen for FW41-
            }
            else
            {
                GSC_HAL_CHK_STATUS_MESSAGE_WITH_HR(MOS_SecureMemcpy(
                    pIgdDataLen,
                    SIGMA_PRI_DATE_LEN_SIZE,
                    pHeciOutputData + PavpAppMsgSize,
                    SIGMA_PRI_DATE_LEN_SIZE),
                    "Failed to copy private data length. Error = 0x%x.", eStatus);

                PrivDataOffset = PavpAppMsgSize + SIGMA_PRI_DATE_LEN_SIZE;
                PrivDataLength = *pIgdDataLen + SIGMA_PRI_DATE_LEN_SIZE;  //private data length is SIGMA_PRI_DATE_LEN_SIZE + *pIgdDataLen for FW42+
            }

            if (PavpAppMsgSize + *pIgdDataLen > m_maxOutputBufferSize)
            {
                GSC_HAL_ASSERTMESSAGE("PavpAppMsgSize %d + private data length %d should be less than max GSC output buffer %d",
                    PavpAppMsgSize,
                    *pIgdDataLen,
                    m_maxOutputBufferSize);
                hr = E_FAIL;
                MT_ERR2(MT_CP_BUFFER_RULE, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }

            GSC_HAL_CHK_STATUS_MESSAGE_WITH_HR(MOS_SecureMemcpy(
                pIgdData,
                *pIgdDataLen,
                pHeciOutputData + PrivDataOffset,
                *pIgdDataLen),
                "Failed to copy private data. Error = 0x%x.", eStatus);

#if (_DEBUG || _RELEASE_INTERNAL)
            // Copy private data out to app
            if (((m_GscHalUsage == RootTrust_HAL_WIDI_CLIENT_FW_APP_USE) ||
                 (m_GscHalUsage == RootTrust_HAL_PAVP_CLIENT_FW_APP_USE) ||
                 (m_GscHalUsage == RootTrust_HAL_MKHI_CLIENT_FW_APP_USE) ||
                 (m_GscHalUsage == RootTrust_HAL_GSC_PROXY_CLIENT_FW_APP_USE)) &&
                (PavpAppMsgSize + PrivDataLength <= OutputBufferLength))
            {
                GSC_HAL_CHK_STATUS_MESSAGE_WITH_HR(MOS_SecureMemcpy(
                    pOutputBuffer + PavpAppMsgSize,
                    PrivDataLength,
                    pHeciOutputData + PavpAppMsgSize,
                    PrivDataLength),
                    "Failed to copy output private GSC data. Error = 0x%x.", eStatus);
            }
            else
            {
                GSC_HAL_NORMALMESSAGE("Failed to copy output private GSC data, invalid output buffer size or GscHalUsage.");
            }
#endif
        }
    } while (FALSE);

    //OCA Dump
    ocaDumper = static_cast<CpOcaDumper *>(m_pOsServices->m_pOsInterface->osCpInterface->GetOcaDumper());
    if (ocaDumper)
    {
        uint64_t gfxAddr = m_pOsServices->m_pOsInterface->pfnGetResourceGfxAddress(m_pOsServices->m_pOsInterface, &m_GSCOutputBuffer);
        ocaDumper->SetIoMessage(CP_IO_MESSAGE_TYPE_OUTPUT, gfxAddr, sizeof(GSC_CS_MEMORY_HEADER) + PavpAppMsgSize + PrivDataLength, pHeciOutputData);
    }
    else
    {
        GSC_HAL_NORMALMESSAGE("OCA Dumpper Not Valid");
    }

    if (pHeciOutputData != nullptr)
    {
        m_pOsServices->m_pOsInterface->pfnUnlockResource(
            m_pOsServices->m_pOsInterface,
            &m_GSCOutputBuffer);
    }

finish:
    GSC_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpGscHalG13::WriteResourceInputBuffer(
    uint8_t     *pInputBuffer,
    uint32_t    InputBufferLength,
    uint8_t     HeciClient,
    uint8_t     *pExtensionBuffer,
    uint32_t    extensionSize)
{
    HRESULT                 hr              = S_OK;
    MOS_STATUS              eStatus         = MOS_STATUS_SUCCESS;
    uint8_t*                pHeciInputData  = nullptr;
    GSC_CS_MEMORY_HEADER    GSCCSMemoryhdr  = { 0 };
    CpOcaDumper*            ocaDumper           = nullptr;
    GSC_HAL_FUNCTION_ENTER;

    do
    {
        GSC_HAL_CHK_NULL(pInputBuffer);
        GSC_HAL_CHK_NULL(m_pOsServices);
        GSC_HAL_CHK_NULL(m_pOsServices->m_pOsInterface);
        GSC_HAL_CHK_NULL(m_pOsServices->m_pOsInterface->pfnLockResource);
        GSC_HAL_CHK_NULL(m_pOsServices->m_pOsInterface->pfnUnlockResource);
        GSC_HAL_CHK_NULL(m_pOsServices->m_pOsInterface->pfnGetResourceGfxAddress);
        GSC_HAL_CHK_NULL(m_pOsServices->m_pOsInterface->osCpInterface);

        if (extensionSize)
        {
            GSC_HAL_CHK_NULL(pExtensionBuffer);
        }
        MOS_LOCK_PARAMS lockFlags {};
        lockFlags.WriteOnly = 1;
        pHeciInputData = (uint8_t*)m_pOsServices->m_pOsInterface->pfnLockResource(
            m_pOsServices->m_pOsInterface,
            &m_GSCInputBuffer,
            &lockFlags);
        GSC_HAL_CHK_NULL(pHeciInputData);

#if (_DEBUG || _RELEASE_INTERNAL)
        if (m_GscHalUsage == RootTrust_HAL_GSC_PROXY_CLIENT_FW_APP_USE)
        {
            GSC_HAL_CHK_STATUS_MESSAGE_WITH_HR(MOS_SecureMemcpy(
                pHeciInputData,
                m_maxInputBufferSize,
                pInputBuffer,
                InputBufferLength),
                "Failed to copy memory. Error = 0x%x.", eStatus);

            break;
        }
#endif

        GSCCSMemoryhdr.DW0.validity_marker          = HECI_VALIDITY_MARKER;
        GSCCSMemoryhdr.DW1.gsc_address              = HeciClient;
        GSCCSMemoryhdr.DW1.header_version           = 1;
        GSCCSMemoryhdr.DW2_3.host_session_handle    = m_GSCHostSessionHandle;
        GSCCSMemoryhdr.DW6.message_size             = sizeof(GSCCSMemoryhdr) + InputBufferLength + extensionSize;
        GSCCSMemoryhdr.DW7.extension_size           = extensionSize;
        if (m_GSCCSMemoryhdr.DW7.pending)
        {
            GSCCSMemoryhdr.DW4_5.gsc_message_handle = m_GSCCSMemoryhdr.DW4_5.gsc_message_handle;
        }

        GSC_HAL_CHK_STATUS_MESSAGE_WITH_HR(MOS_SecureMemcpy(
            pHeciInputData,
            m_maxInputBufferSize,
            &GSCCSMemoryhdr,
            sizeof(GSCCSMemoryhdr)),
            "Failed to copy memory. Error = 0x%x.", eStatus);
        if (extensionSize)
        {
            GSC_HAL_CHK_STATUS_MESSAGE_WITH_HR(MOS_SecureMemcpy(
                pHeciInputData + sizeof(GSCCSMemoryhdr),
                m_maxInputBufferSize - sizeof(GSCCSMemoryhdr),
                pExtensionBuffer,
                extensionSize),
                "Failed to copy memory. Error = 0x%x.", eStatus);
        }
        GSC_HAL_CHK_STATUS_MESSAGE_WITH_HR(MOS_SecureMemcpy(
            pHeciInputData + sizeof(GSCCSMemoryhdr) + extensionSize,
            m_maxInputBufferSize - (sizeof(GSCCSMemoryhdr) + extensionSize),
            pInputBuffer,
            InputBufferLength),
            "Failed to copy memory. Error = 0x%x.", eStatus);

        memset(&m_GSCCSMemoryhdr, 0, sizeof(m_GSCCSMemoryhdr));
        //OCA Dump
        ocaDumper = static_cast<CpOcaDumper *>(m_pOsServices->m_pOsInterface->osCpInterface->GetOcaDumper());
        if (ocaDumper)
        {
            uint64_t gfxAddr = m_pOsServices->m_pOsInterface->pfnGetResourceGfxAddress(m_pOsServices->m_pOsInterface, &m_GSCOutputBuffer);
            ocaDumper->SetIoMessage(CP_IO_MESSAGE_TYPE_INPUT, gfxAddr, sizeof(GSCCSMemoryhdr) + InputBufferLength + extensionSize, pHeciInputData);
        }
        else
        {
            GSC_HAL_NORMALMESSAGE("OCA Dumpper Not Valid");
        }
    } while (FALSE);

finish:
    m_pOsServices->m_pOsInterface->pfnUnlockResource(
        m_pOsServices->m_pOsInterface,
        &m_GSCInputBuffer);

    GSC_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpGscHalG13::BuildCommandBuffer(
    PMOS_COMMAND_BUFFER pCmdBuffer,
    uint32_t              InputBufferLength,
    uint32_t              OutputBufferLength)
{
    HRESULT                     hr                  = S_OK;
    MOS_STATUS                  eStatus             = MOS_STATUS_SUCCESS;
    MhwCpG13                    *mhwCp              = nullptr;
    PMOS_CMD_BUF_ATTRI_VE       pAttriGSC           = nullptr;
    PMOS_CONTEXT                pOsContext          = nullptr;
    PMHW_MI_MMIOREGISTERS       pMmioRegisters      = nullptr;
    CpOcaDumper*                ocaDumper           = nullptr;

    GSC_HAL_FUNCTION_ENTER;

    do
    {
        GSC_HAL_CHK_NULL(pCmdBuffer);
        GSC_HAL_CHK_NULL(m_pOsServices);
        GSC_HAL_CHK_NULL(m_pOsServices->m_pOsInterface);
        GSC_HAL_CHK_NULL(m_pOsServices->m_pOsInterface->pOsContext);
        GSC_HAL_CHK_NULL(m_pMhwInterfaces);
        GSC_HAL_CHK_NULL(m_pMhwInterfaces->m_miItf);
        GSC_HAL_CHK_NULL(m_pMhwInterfaces->m_miItf->GetMmioRegisters());

        pOsContext = m_pOsServices->m_pOsInterface->pOsContext;
        pMmioRegisters = m_pMhwInterfaces->m_miItf->GetMmioRegisters();

        mhwCp = dynamic_cast<MhwCpG13*>(m_pMhwInterfaces->m_cpInterface);
        GSC_HAL_CHK_NULL(mhwCp);

        // Add OCA support
        HalOcaInterfaceNext::On1stLevelBBStart(*pCmdBuffer, (MOS_CONTEXT_HANDLE)pOsContext, m_pOsServices->m_pOsInterface->CurrentGpuContextHandle, m_pMhwInterfaces->m_miItf, *pMmioRegisters);

        // AddProlog
        m_pMhwInterfaces->m_miItf->AddProtectedProlog(pCmdBuffer);

        MHW_GSC_HECI_Params         GSCHeciParams;
        MOS_ZeroMemory(&GSCHeciParams, sizeof(GSCHeciParams));
        GSCHeciParams.presSrc                   = &m_GSCInputBuffer;
        GSCHeciParams.inputBufferLength         = InputBufferLength;
        GSCHeciParams.presDst                   = &m_GSCOutputBuffer;
        GSCHeciParams.outputBufferLength        = OutputBufferLength;

        // Add OCA support
        HalOcaInterfaceNext::OnIndirectState(*pCmdBuffer, (MOS_CONTEXT_HANDLE)pOsContext, GSCHeciParams.presSrc, 0, false, GSCHeciParams.inputBufferLength);
        HalOcaInterfaceNext::OnIndirectState(*pCmdBuffer, (MOS_CONTEXT_HANDLE)pOsContext, GSCHeciParams.presDst, 0, false, GSCHeciParams.outputBufferLength);
        HalOcaInterfaceNext::OnDispatch(*pCmdBuffer, *(m_pOsServices->m_pOsInterface), m_pMhwInterfaces->m_miItf, *pMmioRegisters);
        
        GSC_HAL_CHK_STATUS_MESSAGE_WITH_HR(mhwCp->AddGSCHeciCmd(m_pOsServices->m_pOsInterface, pCmdBuffer, &GSCHeciParams),
            "Add GSC HECI command failed. Error = 0x%x.", eStatus);

        

        auto& flushDwParams = m_pMhwInterfaces->m_miItf->GETPAR_MI_FLUSH_DW();
        flushDwParams = {};
        GSC_HAL_CHK_STATUS_MESSAGE_WITH_HR(m_pMhwInterfaces->m_miItf->ADDCMD_MI_FLUSH_DW(pCmdBuffer),
            "Add MI_FLUSH_CMD failed. Error = 0x%x.", eStatus);

        GSC_HAL_CHK_STATUS_MESSAGE_WITH_HR(m_pMhwInterfaces->m_miItf->AddMiBatchBufferEnd(pCmdBuffer, nullptr),
            "Add MI BB end command failed. Error = 0x%x.", eStatus);

        // Add OCA support
        ocaDumper = static_cast<CpOcaDumper *>(m_pOsServices->m_pOsInterface->osCpInterface->GetOcaDumper());
        if (ocaDumper)
        {
#if !EMUL
            HalOcaInterfaceNext::DumpCpIoMsg(*pCmdBuffer, m_pOsServices->m_pOsInterface->pOsContext, ocaDumper, CP_IO_MESSAGE_TYPE_OUTPUT);  // Suppose here can only dump last output once current command hang.
            HalOcaInterfaceNext::DumpCpIoMsg(*pCmdBuffer, m_pOsServices->m_pOsInterface->pOsContext, ocaDumper, CP_IO_MESSAGE_TYPE_INPUT);   // Suppose here always dump input of current command.
#endif
        }
        else
        {
            GSC_HAL_NORMALMESSAGE("OCA Dumpper Not Valid");
        }
        
        HalOcaInterfaceNext::On1stLevelBBEnd(*pCmdBuffer, *m_pOsServices->m_pOsInterface);

    } while (FALSE);

finish:
    GSC_HAL_FUNCTION_EXIT(hr);
    return hr;
}
