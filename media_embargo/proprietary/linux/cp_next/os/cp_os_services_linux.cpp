/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2013-2022
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

File Name: cp_os_services_linux.cpp

Abstract:
    Implements the CP OS sevices linux class in the OS agnostic design.

Environment:
    linux
======================= end_copyright_notice ==================================*/

#include "cp_os_services_linux.h"
#include "mos_utilities.h"
#include <sys/mman.h>
#include <fcntl.h>
#include "mos_pxp_sm.h"
#include "cp_hal_gsc.h"

#define GSC_EXTENSION_HEADER_SIZE 8
#define VTAG_EXTENSION_PAYLOAD_SIZE 4

#define  HOST_SESSION_HANDLE_REQ_ALLOC 1

// This is the implementation of the PavpOsServicesFactory header as
// declared in OsServices.
std::shared_ptr<CPavpOsServices> CPavpOsServices::PavpOsServicesFactory(
    HRESULT&            hr,                 // [out]
    PAVP_DEVICE_CONTEXT pDeviceContext)     // [in]
{
    CP_OS_FUNCTION_ENTER;

    std::shared_ptr<CPavpOsServices> pOs;

    pOs = std::make_shared<CPavpOsServices_Linux>(hr, pDeviceContext);
    if (pOs == NULL)
    {
        CP_OS_ASSERTMESSAGE("Error: Failed to allocate memory for an OS Services object.");
        hr = E_OUTOFMEMORY;
    }

    CP_OS_FUNCTION_EXIT(hr);
    return pOs;
}

CPavpOsServices_Linux::CPavpOsServices_Linux(
    HRESULT&            hr,
    PAVP_DEVICE_CONTEXT pDeviceContext) : CPavpOsServices(hr, pDeviceContext)
{
    CP_OS_FUNCTION_ENTER;

    ResetMemberVariables();

    do
    {
        // Verify that the base class constructor passed.
        if (FAILED(hr) || m_pOsInterface == nullptr)
        {
            CP_OS_ASSERTMESSAGE("CPavpOsServices constructor failed. Error = 0x%x.", hr);
            break;
        }
        // set context based scheduling, latest i915 don't support legacy workload submit
        Mos_SetVirtualEngineSupported(m_pOsInterface, true);
        m_pOsInterface->pfnVirtualEngineSupported(m_pOsInterface, true, true);

        // If it was not created yet, create a GPU context for the MFX engine,
        // and update the current GPU context so the video context would be used when calling MOS.
        hr = CreateVideoGPUContext();
        if (FAILED(hr))
        {
            CP_OS_ASSERTMESSAGE("Unable to create GPU video context. hr = 0x%x", hr);
            break;
        }
    } while (FALSE);

    CP_OS_FUNCTION_EXIT(hr);
}

CPavpOsServices_Linux::~CPavpOsServices_Linux()
{
    CP_OS_FUNCTION_ENTER;

    CP_OS_FUNCTION_EXIT(S_OK);
}

void CPavpOsServices_Linux::ResetMemberVariables()
{
    CP_OS_FUNCTION_ENTER;

    m_cpContext.value   = 0;

    CP_OS_FUNCTION_EXIT(S_OK);
}

HRESULT CPavpOsServices_Linux::OpenProtectedSession(
    PAVP_SESSION_HANDLE* pPavpSessionHandle)
{
    return S_OK;
}

HRESULT CPavpOsServices_Linux::SendRecvKMDMsg(PXP_PROTECTION_INFO *pInputOutput)
{
    HRESULT     hr      = S_OK;
    int32_t     status  = 0;
    do
    {
        if (m_pOsInterface == nullptr ||
            m_pOsInterface->osCpInterface == nullptr)
        {
            CP_OS_ASSERTMESSAGE("MOS interface is nullptr");
            hr = E_UNEXPECTED;
            break;
        }

        MosCp *pMosCp = dynamic_cast<MosCp*>(m_pOsInterface->osCpInterface);
        if (pMosCp == nullptr)
        {
            hr = E_UNEXPECTED;
            CP_OS_ASSERTMESSAGE("pMosCP is nullptr")
            break;
        }

        if (m_pOsInterface->pOsContext->fd < 0)
        {
            CP_OS_ASSERTMESSAGE("File descriptor of I915 is not initialized");
            hr = E_FAIL;
            break;
        }

        status = mos_pxp_sm_submit(m_pOsInterface->pOsContext->fd, pInputOutput);
        if (status != 0)
        {
            hr = E_FAIL;
            CP_OS_ASSERTMESSAGE("i915 retuned error %d, errno %d", status, errno);
            break;
        }
    } while (false);

    return hr;
}

HRESULT CPavpOsServices_Linux::QueryCpContext(uint32_t *pPavpSessionId, bool *pIsSessionInitlaized)
{
    HRESULT                 hr      = S_OK;
    PXP_PROTECTION_INFO     PxpInfo;

    CP_OS_FUNCTION_ENTER;

    do
    {
        if (m_pOsInterface == nullptr ||
            pPavpSessionId == nullptr ||
            pIsSessionInitlaized == nullptr)
        {
            CP_OS_ASSERTMESSAGE("Invald input parameter.");
            hr = E_UNEXPECTED;
            break;
        }

        MOS_ZeroMemory(&PxpInfo, sizeof(PxpInfo));

        PxpInfo.Action                          = PXP_ACTION_QUERY_CP_CONTEXT;
        PxpInfo.QueryCpContext.PxpSessionId     = *pPavpSessionId;

        hr = SendRecvKMDMsg(&PxpInfo);
        if (FAILED(hr) || PxpInfo.eSMStatus != PXP_SM_STATUS_SUCCESS)
        {
            CP_OS_ASSERTMESSAGE("Error while sending data to KMD. hr = 0x%x eSMStatus = %s", hr, StateMachineStatus2Str(PxpInfo.eSMStatus));
            hr = E_FAIL;
            break;
        }
        else
        {
            *pPavpSessionId             = PxpInfo.QueryCpContext.PxpSessionId;
            if ( PxpInfo.QueryCpContext.bsessionalive > 0)
            {
                *pIsSessionInitlaized   = true;
            }
            else
            {
                *pIsSessionInitlaized   = false;
            }
        }
    } while (FALSE);

    return hr;
}

HRESULT CPavpOsServices_Linux::GetSetPavpSessionStatus(
    PAVP_SESSION_TYPE       eSessionType,                   // [in]
    PAVP_SESSION_MODE       eSessionMode,                   // [in]
    PAVP_SESSION_STATUS     eReqSessionStatus,              // [in]
    PAVP_SESSION_STATUS     *pPavpSessionStatus,             // [out]
    PAVP_SESSION_STATUS     *pPreviousPavpSessionStatus,     // [in, optional]
    uint32_t                *pPavpSessionId)                 // [out, optional]
{
    return E_NOTIMPL;
}

HRESULT CPavpOsServices_Linux::ReqNewPavpSessionState(
    PAVP_SESSION_TYPE       eSessionType,                   // [in]
    PAVP_SESSION_MODE       eSessionMode,                   // [in]
    PXP_SM_SESSION_REQ      eReqSessionStatus,              // [in]
    PXP_SM_STATUS           *pSMStatus,                     // [out]
    uint32_t                *pPavpSessionId)                // [out optional, in optional]
{
    HRESULT                 hr      = S_OK;
    PXP_PROTECTION_INFO     PxpInfo;

    CP_OS_FUNCTION_ENTER;

    do
    {
        if (m_pOsInterface == nullptr ||
            m_pOsInterface->osCpInterface == nullptr)
        {
            CP_OS_ASSERTMESSAGE("MOS interface is nullptr");
            hr = E_UNEXPECTED;
            break;
        }

        MosCp *pMosCp = dynamic_cast<MosCp*>(m_pOsInterface->osCpInterface);

        if (pMosCp == nullptr)
        {
            hr = E_UNEXPECTED;
            CP_OS_ASSERTMESSAGE("pMosCP is nullptr")
            break;
        }
        if (pSMStatus == nullptr ||
            (pPavpSessionId    == nullptr &&             // pPavpSessionId is used only when moving to 'in init' status.
            eReqSessionStatus == PXP_SM_REQ_SESSION_ID_INIT))
        {
            CP_OS_ASSERTMESSAGE("Error: Invalid parameter.");
            hr = E_INVALIDARG;
            break;
        }
        MOS_ZeroMemory(&PxpInfo, sizeof(PxpInfo));

        CP_OS_VERBOSEMESSAGE("Requesting new state %s from KMD.", StateMachineRequest2Str(eReqSessionStatus));

        PxpInfo.Action                              = PXP_ACTION_SET_SESSION_STATUS;
        PxpInfo.SetSessionStatus.SessionType        = KmSessionTypeFromApiSessionType(eSessionType);
        PxpInfo.SetSessionStatus.SessionMode        = KmSessionModeFromApiSessionMode(eSessionMode);
        PxpInfo.SetSessionStatus.ReqSessionState    = eReqSessionStatus;
        PxpInfo.SetSessionStatus.PxpSessionId       = m_cpContext.enable ? m_cpContext.sessionId : PAVP_INVALID_SESSION_ID;

        hr = SendRecvKMDMsg(&PxpInfo);
        if (FAILED(hr))
        {
            CP_OS_ASSERTMESSAGE("Error while sending data to KMD. hr = 0x%x eSMStatus = %s", hr, StateMachineStatus2Str(PxpInfo.eSMStatus));
            hr = E_FAIL;
            break;
        }

        *pSMStatus = PxpInfo.eSMStatus;
        if (*pSMStatus != PXP_SM_STATUS_SUCCESS)
        {
            CP_OS_ASSERTMESSAGE("KMD returned error.");
            hr = E_FAIL;
            break;
        }

        CP_OS_VERBOSEMESSAGE("State Machine Status: %s.", StateMachineStatus2Str(*pSMStatus));
        CP_OS_VERBOSEMESSAGE("Session ID: 0x%x.", PxpInfo.SetSessionStatus.PxpSessionId);

        switch (eReqSessionStatus)
        {
            case PXP_SM_REQ_SESSION_ID_INIT:
                // If requesting 'in init' and it succeeded, return the new session ID.
                *pPavpSessionId = PxpInfo.SetSessionStatus.PxpSessionId;
                m_cpContext.value = PxpInfo.SetSessionStatus.PxpSessionId;
                break;

            case PXP_SM_REQ_SESSION_IN_PLAY:
                // If moving to 'initialized', need to inform the device context, so that
                // other parts of the driver are aware we are in a PAVP session (and in heavy mode)
                pMosCp->UpdateCpContext(m_cpContext.value);
                break;

            case PXP_SM_REQ_SESSION_TERMINATE:
                pMosCp->UpdateCpContext(0);
                break;
            default:
                hr = E_FAIL;
        }
    } while (FALSE);

    CP_OS_FUNCTION_EXIT(hr);

    return hr;
}

BOOL CPavpOsServices_Linux::IsFwFallbackEnabled()
{
    CP_OS_VERBOSEMESSAGE("FW Fallback is not applicable to Android/Linux, returning FALSE");
    return FALSE;
}

HRESULT CPavpOsServices_Linux::CloneToLinearVideoResource(
    CONST PMOS_RESOURCE     SrcShapeResource,           // [in]
    PMOS_RESOURCE           *pDstResource,              // [out]
    CONST PMOS_RESOURCE     SrcDataResource,            // [in, optional]
    DWORD                   dwSrcDataSize)              // [in, optional] not used in Linux
{
    HRESULT hr = S_OK;

    CP_OS_FUNCTION_ENTER;

    do
    {
        if (SrcShapeResource == NULL ||
            pDstResource     == NULL)
        {
            CP_OS_ASSERTMESSAGE("Input parameter is NULL.");
            hr = E_INVALIDARG;
            break;
        }

        // Allocate a 1D buffer resource.
        // This resource will be released in FreeResource function.
        hr = AllocateLinearResource(
                MOS_GFXRES_BUFFER,          // eType
                SrcShapeResource->iPitch,   // dwWidth
                SrcShapeResource->iHeight,  // dwHeight
                __FUNCTION__,               // pResourceName
                Format_Buffer,              // eFormat
                *pDstResource);             // pResource
        if (FAILED(hr))
        {
            CP_OS_ASSERTMESSAGE("Failed to allocate resource. Error = 0x%x.", hr);
            break;
        }

        // Clone the resource data if the SrcDataResource is given.
        if (SrcDataResource != NULL)
        {
            hr = CopyVideoResource(SrcDataResource, *pDstResource);
            if (FAILED(hr))
            {
                CP_OS_ASSERTMESSAGE("Failed to clone the resource. Error = 0x%x.", hr);
            }
        }

    } while (FALSE);

    // No need to clean the resources if there was failure as cleanup will be triggered by the device if there was some failure.

    CP_OS_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpOsServices_Linux::GetGSCCommandBuffer(MOS_COMMAND_BUFFER* pCmdBuffer)
{
    return E_NOTIMPL;
}

HRESULT CPavpOsServices_Linux::SubmitGSCCommandBuffer(MOS_COMMAND_BUFFER* pCmdBuffer)
{
    return E_NOTIMPL;
}

HRESULT CPavpOsServices_Linux::AllocateGSCHostSessionHandle(UINT64 *pGscHostSessionHandle)
{
    HRESULT                 hr = S_OK;
    PXP_PROTECTION_INFO     PxpInfo;

    do
    {
        if (pGscHostSessionHandle == nullptr)
        {
            CP_OS_ASSERTMESSAGE("Invald input parameter.");
            hr = E_UNEXPECTED;
            break;
        }

        MOS_ZeroMemory(&PxpInfo, sizeof(PxpInfo));

        PxpInfo.Action                                      = PXP_ACTION_HOST_SESSION_HANDLE_REQ;
        PxpInfo.HostSessionHandleReq.request_type           = HOST_SESSION_HANDLE_REQ_ALLOC;

        hr = SendRecvKMDMsg(&PxpInfo);
        if (FAILED(hr) || PxpInfo.eSMStatus != PXP_SM_STATUS_SUCCESS)
        {
            CP_OS_ASSERTMESSAGE("Error while sending data to KMD. hr = 0x%x eSMStatus = %s", hr, StateMachineStatus2Str(PxpInfo.eSMStatus));
            hr = E_FAIL;
            break;
        }
        *pGscHostSessionHandle = PxpInfo.HostSessionHandleReq.host_session_handle;
    } while (false);

    return hr;
}

HRESULT CPavpOsServices_Linux::FreeGSCHostSessionHandle(UINT64 GscHostSessionHandle)
{
    return S_OK;
}

HRESULT CPavpOsServices_Linux::CreateSubmitGSCCommandBuffer(uint8_t  *pGSCInputOutput,
                                                            uint32_t Inputbufferlength,
                                                            uint32_t Outputbufferlength,
                                                            uint8_t  HeciClient,
                                                            uint32_t *RecvBytes)
{
    MOS_UNUSED(HeciClient);
    MOS_UNUSED(Outputbufferlength);
    return SendRecvDataTEE( pGSCInputOutput, Inputbufferlength, GSC_MAX_OUTPUT_BUFF_SIZE, pGSCInputOutput + GSC_MAX_INPUT_BUFF_SIZE, RecvBytes);
}

HRESULT CPavpOsServices_Linux::SubmitGSCCommand(
    PMOS_RESOURCE       pGSCInputResource,
    PMOS_RESOURCE       pGSCOutputResource,
    uint32_t            inputBufferLength,
    uint32_t            outputBufferLength)
{
    HRESULT                 hr      = S_OK;
    PXP_PROTECTION_INFO     PxpInfo;
    CP_OS_FUNCTION_ENTER;

    if (pGSCInputResource == nullptr ||
        pGSCOutputResource == nullptr)
    {
        CP_OS_ASSERTMESSAGE("Parameters are NULL.");
        hr = E_INVALIDARG;
        return hr;
    }

    if (m_pOsInterface == nullptr                  ||
        m_pOsInterface->pfnLockResource == nullptr ||
        m_pOsInterface->pfnUnlockResource == nullptr) 
    {
        CP_OS_ASSERTMESSAGE("OsInterface is NULL.");
        hr = E_INVALIDARG;
        return hr;
    }

    do
    {
        MOS_LOCK_PARAMS lockFlags {};

        lockFlags.ReadOnly = 1;
        uint8_t* pHeciInputData = (uint8_t*)m_pOsInterface->pfnLockResource(
            m_pOsInterface,
            pGSCInputResource,
            &lockFlags);

        lockFlags.WriteOnly = 1;
        uint8_t* pHeciOutputData = (uint8_t*)m_pOsInterface->pfnLockResource(
            m_pOsInterface,
            pGSCOutputResource,
            &lockFlags);

        if(pHeciInputData == nullptr ||
            pHeciInputData == nullptr) 
        {
            CP_OS_ASSERTMESSAGE("Lock GSC resource failed.");
            hr = E_INVALIDARG;
            break;
        }

        CP_OS_VERBOSEMESSAGE("Submitting GSC message via IOCTL, input size = %d, output buffer size = %d.", inputBufferLength, outputBufferLength);

        PxpInfo.Action                          = PXP_ACTION_TEE_COMMUNICATION;
        PxpInfo.TEEIoMessage.msg_in      = pHeciInputData;
        PxpInfo.TEEIoMessage.msg_in_size        = inputBufferLength;
        PxpInfo.TEEIoMessage.msg_out     = pHeciOutputData;
        PxpInfo.TEEIoMessage.msg_out_buf_size   = outputBufferLength;

        hr = SendRecvKMDMsg(&PxpInfo);
        if (FAILED(hr) || PxpInfo.eSMStatus != PXP_SM_STATUS_SUCCESS)
        {
            CP_OS_ASSERTMESSAGE("Error while sending data to KMD. hr = 0x%x eSMStatus = %s", hr, StateMachineStatus2Str(PxpInfo.eSMStatus));
            hr = E_FAIL;
            break;
        }
    } while (FALSE);

    m_pOsInterface->pfnUnlockResource(
        m_pOsInterface,
        pGSCInputResource);

    m_pOsInterface->pfnUnlockResource(
        m_pOsInterface,
        pGSCOutputResource);

    CP_OS_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpOsServices_Linux::GetGSCCommandBufferRequired(bool &cmdBufferRequiredForSubmit)
{
    cmdBufferRequiredForSubmit =  false;
    return S_OK;
}

// Currently vtag is the only GSC command header extension.
// In future more extensions can be added as required.
HRESULT CPavpOsServices_Linux::WriteGSCHeaderExtension(uint8_t *pGSCHeaderExtension, uint8_t *pExtensionBuffer, uint32_t extensionSize)
{
    HRESULT                 hr                  = S_OK;
    GSC_HEADER_EXTENSION    GSCBufferExtension  = {0};

    do
    {
        if (pGSCHeaderExtension == nullptr ||
            pExtensionBuffer == nullptr || 
            extensionSize < GSC_EXTENSION_HEADER_SIZE + VTAG_EXTENSION_PAYLOAD_SIZE)
        {
            CP_OS_ASSERTMESSAGE("Parameters are invalid for writing GSC header extension.");
            hr = E_INVALIDARG;
            break;
        }

        GSCBufferExtension.extensiontype        = 1;    //vtag extension type is 1
        GSCBufferExtension.extension_length     = GSC_EXTENSION_HEADER_SIZE + VTAG_EXTENSION_PAYLOAD_SIZE;
        GSCBufferExtension.reserved             = 0;    // reserved bytes to be send as 0
        GSCBufferExtension.extension_exists     = 0;    // currently no extension exists other than vtag

        if (MOS_FAILED(MOS_SecureMemcpy(
                        pGSCHeaderExtension,
                        GSC_EXTENSION_HEADER_SIZE,
                        &GSCBufferExtension,
                        GSC_EXTENSION_HEADER_SIZE)))
        {
            hr = E_FAIL;
            CP_OS_ASSERTMESSAGE("Failed to copy header of GSC extension");
        }
        if (MOS_FAILED(MOS_SecureMemcpy(
                        pGSCHeaderExtension + GSC_EXTENSION_HEADER_SIZE,
                        VTAG_EXTENSION_PAYLOAD_SIZE,
                        pExtensionBuffer,
                        VTAG_EXTENSION_PAYLOAD_SIZE)))
        {
            hr = E_FAIL;
            CP_OS_ASSERTMESSAGE("Failed to copy Vtag as GSC header extension");
            break;
        }
    }while (false);

    return hr;
}

HRESULT CPavpOsServices_Linux::GetGSCHeaderExtensionSize(uint32_t *pExtensionSize)
{
    *pExtensionSize = GSC_EXTENSION_HEADER_SIZE + VTAG_EXTENSION_PAYLOAD_SIZE;
    return S_OK;
}

HRESULT CPavpOsServices_Linux::IsHucAuthenticated(bool &bIsHucAuthenticated)
{
    HRESULT         hr          = S_OK;

    CP_OS_FUNCTION_ENTER;
    do
    {
        MEDIA_FEATURE_TABLE *pSkuTable           = nullptr;
        pSkuTable = m_pOsInterface->pfnGetSkuTable(m_pOsInterface);
        if (!pSkuTable)
        {
            CP_OS_ASSERTMESSAGE("SKU Table is null");
            hr = E_FAIL;
            break;
        }
        bIsHucAuthenticated = MEDIA_IS_SKU(pSkuTable, FtrEnableProtectedHuc);
    } while (false);

    return hr;
}

HRESULT CPavpOsServices_Linux::NotifyGSCBBCompleteEvent()
{
    return S_OK;
}

HRESULT CPavpOsServices_Linux::CreateGSCGPUContext()
{
    // not necessary to create GSC context really
    return S_OK;
}

HRESULT CPavpOsServices_Linux::CopyVideoResource(
    const PMOS_RESOURCE SrcResource,    // [in]
    PMOS_RESOURCE       DstResource)    // [in]
{
    HRESULT         hr          = S_OK;
    MOS_STATUS      eStatus     = MOS_STATUS_UNKNOWN;
    PVOID           pSrcData    = NULL;
    PVOID           pDstData    = NULL;
    DWORD           dwSrcSize   = 0;
    DWORD           dwDstSize   = 0;

    CP_OS_FUNCTION_ENTER;

    do
    {
        // Verify that the resources are not NULL.
        if (SrcResource == NULL ||
            DstResource == NULL)
        {
            CP_OS_ASSERTMESSAGE("Error: resources are NULL.");
            hr = E_INVALIDARG;
            break;
        }

        dwSrcSize = GetResourceSize(SrcResource);
        dwDstSize = GetResourceSize(DstResource);

        if (dwSrcSize > dwDstSize)
        {
            hr = E_FAIL;
            CP_OS_ASSERTMESSAGE("Size of Source (%d) is greater than size of destination (%d).", dwSrcSize, dwDstSize);
            break;
        }

        pDstData = LockResource(*DstResource, LockType::Write);
        if (pDstData == NULL)
        {
            CP_OS_ASSERTMESSAGE("Error locking the destination resource.");
            hr = E_FAIL;
            break;
        }

        pSrcData = LockResource(*SrcResource, LockType::Read);
        if (pSrcData == NULL)
        {
            CP_OS_ASSERTMESSAGE("Error locking the source resource.");
            hr = E_FAIL;
            break;
        }

        eStatus = MOS_SecureMemcpy(
                    pDstData,
                    dwDstSize,
                    pSrcData,
                    dwSrcSize);
        if (MOS_FAILED(eStatus))
        {
            hr = E_FAIL;
            CP_OS_ASSERTMESSAGE("Failed to copy memory. Error = %d.", eStatus);
        }
    } while (FALSE);

    // Unlock the surfaces if needed.
    UNLOCK_RESOURCE(DstResource, pDstData);
    UNLOCK_RESOURCE(SrcResource, pSrcData);

    CP_OS_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpOsServices_Linux::CopyVideoResourceToSystemResource(
    CONST PMOS_RESOURCE     pSrcResource,
    CONST PMOS_RESOURCE     pDstResource,
    DWORD                   DstSize)
{
    HRESULT     hr       = S_OK;
    MOS_STATUS  eStatus = MOS_STATUS_UNKNOWN;
    DWORD       SrcSize = 0;
    PVOID       pSrcData = NULL;
    PVOID       pDstData = NULL;

    CP_OS_FUNCTION_ENTER;

    do
    {
        if (pSrcResource == NULL ||
            pDstResource == NULL)
        {
            CP_OS_ASSERTMESSAGE("Parameters are NULL.");
            hr = E_INVALIDARG;
            break;
        }

        SrcSize = GetResourceSize(pSrcResource);

        if (SrcSize < DstSize)
        {
            CP_OS_ASSERTMESSAGE("Src surface size is less than dst size.");
            hr = E_FAIL;
            break;
        }

        CP_OS_NORMALMESSAGE("Src size is %d bytes", SrcSize);
        CP_OS_NORMALMESSAGE("Copying %d bytes to dst", DstSize);

        pDstData = LockResource(*pDstResource, LockType::Write);
        if (pDstData == NULL)
        {
            CP_OS_ASSERTMESSAGE("Error locking the destination resource.");
            hr = E_FAIL;
            break;
        }

        pSrcData = LockResource(*pSrcResource, LockType::Read);
        if (pSrcData == NULL)
        {
            CP_OS_ASSERTMESSAGE("Error locking the src resource.");
            hr = E_FAIL;
            break;
        }

        eStatus = MOS_SecureMemcpy(
            pDstData,
            SrcSize,
            pSrcData,
            DstSize);
        if (MOS_FAILED(eStatus))
        {
            hr = E_FAIL;
            CP_OS_ASSERTMESSAGE("Failed to copy memory. Error = %d.", eStatus);
        }

    } while (FALSE);

    CP_OS_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpOsServices_Linux::CopyVideoResourceToSystemResourceWithBCS(
    CONST PMOS_RESOURCE     pSrcResource,
    CONST PMOS_RESOURCE     pDstResource,
    DWORD DstSize)
{
    return E_NOTIMPL;
}


#if (_DEBUG || _RELEASE_INTERNAL)
void CPavpOsServices_Linux::LogSurfaceBytes(
    CONST PMOS_RESOURCE     pResource,
    DWORD                   Size)
{
    CP_OS_FUNCTION_ENTER;
    CP_OS_NORMALMESSAGE("Debug function not supported in Linux");
    CP_OS_FUNCTION_EXIT(S_OK);
}
#endif

HRESULT CPavpOsServices_Linux::BltSurface(
    const PMOS_RESOURCE SrcResource,    // [in]
    PMOS_RESOURCE       DstResource,    // [in]
    BOOL                isHeavyMode)    // [in]
{
    HRESULT hr = S_OK;

    CP_OS_FUNCTION_ENTER;

    // No need to check the input parameters, validation is already done inside the CopyVideoResource function.
    hr = CopyVideoResource(SrcResource, DstResource);
    if (FAILED(hr))
    {
        CP_OS_ASSERTMESSAGE("Failed to copy the resources. Error = 0x%x.", hr);
    }

    CP_OS_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpOsServices_Linux::SyncOnSubmit(
    PMOS_COMMAND_BUFFER     pCmdBuf,            // [in]
    BOOL                    bWaitForCBRender    // [in]
    ) const
{
    HRESULT                 hr          = S_OK;

    CP_OS_FUNCTION_ENTER;

    do
    {
        // Verify that the needed OS interface is valid.
        if (m_pOsInterface                             == NULL ||
            m_pOsInterface->pfnWaitAndReleaseCmdBuffer == NULL)
        {
            CP_OS_ASSERTMESSAGE("Error: OS interface is NULL.");
            hr = E_UNEXPECTED;
            break;
        }

        // Wait for the command buffer to be rendered.
        // This command is important for EncryptionBlt and WYSIWYS as this is the only way to assure GPU waiting.
        if (bWaitForCBRender)
        {
            m_pOsInterface->pfnWaitAndReleaseCmdBuffer(m_pOsInterface, pCmdBuf);
        }
    } while (FALSE);

    CP_OS_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpOsServices_Linux::WriteAppIDRegister(PAVP_PCH_INJECT_KEY_MODE ePchInjectKeyMode, BYTE SessionId)
{
    return E_NOTIMPL;
}

HRESULT CPavpOsServices_Linux::SessionRegistryUpdate(
    CP_USER_FEATURE_VALUE_ID  UserFeatureValueId,
    PAVP_SESSION_TYPE         eSessionType,
    DWORD                     dwSessionId,
    CP_SESSIONS_UPDATE_TYPE   eUpdateType)
{
    return E_NOTIMPL;
}

BOOL CPavpOsServices_Linux::IsSimulationEnabled()
{
    return FALSE;
}

HRESULT CPavpOsServices_Linux::ReadMMIORegister(
    PAVP_GPU_REGISTER_TYPE      eRegType,       // [in]
    PAVP_GPU_REGISTER_VALUE     &rValue,        // [out]
    PAVP_GPU_REGISTER_OFFSET    Offset,         // [in, optional]
    ULONG                       Index,          // [in, optional]
    PAVP_HWACCESS_GPU_STATE     GpuState)       // [in, optional]
{
    HRESULT                 hr                  = S_OK;
    uint32_t                SpecialHandlingReq  = 0;

    CP_OS_FUNCTION_ENTER;

    do
    {
        CP_OS_VERBOSEMESSAGE("Requesting KMD to read MMIO register.");

        // special handling is required for specific regsiters
        SpecialHandlingReq = IsSpecialRegHandlingReq(eRegType);
        if (SpecialHandlingReq)
        {
            CP_OS_ASSERTMESSAGE("linux driver is not able to handle debug register");
            hr = E_FAIL;
            break;
        }
        int         ret     = 0;
        uint64_t    regval  = 0;

        ret = mos_reg_read( m_pOsInterface->pOsContext->bufmgr, Offset, &regval);
        if (ret)
        {
            CP_OS_ASSERTMESSAGE("KMD returned error while reading mmio reg %d, ret");
            hr = E_FAIL;
            break;
        }
        else
        {
            rValue = static_cast<PAVP_GPU_REGISTER_VALUE>(regval);
        }
    } while (FALSE);

    CP_OS_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpOsServices_Linux::WriteMMIORegister(
    PAVP_GPU_REGISTER_TYPE      eRegType,    // [in]
    PAVP_GPU_REGISTER_VALUE     ullValue,    // [in]
    PAVP_GPU_REGISTER_VALUE     &rValue)     // [out]
{
    return E_NOTIMPL;
}

HRESULT CPavpOsServices_Linux::SetWiDiSessionParameters(
    uint64_t          rIV,                // [in]
    uint32_t          StreamCtr,          // [in]
    BOOL              bWiDiEnabled,       // [in]
    PAVP_SESSION_MODE ePavpSessionMode,   // [in]
    ULONG             HDCPType,           // [in]
    BOOL              bDontMoveSession)   // [in]
{
    // implmentation will be added once HDCP interfaces are defined
    return E_NOTIMPL;
}

HRESULT CPavpOsServices_Linux::SendRecvDataTEE(
    uint8_t     *pInput,
    uint32_t    InputLen,
    uint32_t    OutputBufferLen,
    uint8_t     *pOutput,
    uint32_t    *pRecvSize)
{
    HRESULT                 hr      = S_OK;
    PXP_PROTECTION_INFO     PxpInfo;
    CP_OS_FUNCTION_ENTER;

    do
    {
        if (pInput == nullptr ||
            pOutput == nullptr ||
            pRecvSize == nullptr)
        {
            CP_OS_ASSERTMESSAGE("Parameters are NULL.");
            hr = E_INVALIDARG;
            break;
        }
        MOS_ZeroMemory(&PxpInfo, sizeof(PxpInfo));

        PxpInfo.Action                          = PXP_ACTION_TEE_COMMUNICATION;
        PxpInfo.TEEIoMessage.msg_in             = pInput;
        PxpInfo.TEEIoMessage.msg_in_size        = InputLen;
        PxpInfo.TEEIoMessage.msg_out_buf_size   = OutputBufferLen;
        PxpInfo.TEEIoMessage.msg_out            = pOutput;

        hr = SendRecvKMDMsg(&PxpInfo);
        if (FAILED(hr) || PxpInfo.eSMStatus != PXP_SM_STATUS_SUCCESS)
        {
            CP_OS_ASSERTMESSAGE("Error while sending data to KMD. hr = 0x%x eSMStatus = %s", hr, StateMachineStatus2Str(PxpInfo.eSMStatus));
            hr = E_FAIL;
            break;
        }
        else
        {
            *pRecvSize = PxpInfo.TEEIoMessage.msg_out_size;
        }

    } while (FALSE);

    CP_OS_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpOsServices_Linux::QuerySessionStatus(PAVP_SESSION_STATUS *pCurrentStatus) // [out]
{
    return E_NOTIMPL;
}

HRESULT CPavpOsServices_Linux::GetEpidResourcePath(std::string & stEpidResourcePath)
{
    std::string epidResourcePath = "/etc/systemd/system/";
    stEpidResourcePath.clear();
    stEpidResourcePath.append(epidResourcePath);
    return S_OK;
}
