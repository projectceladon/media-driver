/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2010-2021
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

File Name: cp_pavposservices_linux.h

Abstract:
    This code is part of the CPHAL design.
    OSservices layer is responsible for OS dependant operations regaring communication with KMD and GPU.
    The layer should support both Windows and Linux/Android architectures.
    This class implements the OSserivces layer Linux side, and is derived from Cp_pavposservices class
    Functions in the public interface will return INT32 so agnostic code can call it.

Environment:
    Linux/Android

Notes:
    None
======================= end_copyright_notice ==================================*/
#ifndef __CP_OS_SERVICES_LINUX_H__
#define __CP_OS_SERVICES_LINUX_H__

#include "cp_os_services.h"
#include "mos_defs.h"
#include "pxp_statemachine.h"

class CPavpOsServices_Linux : public CPavpOsServices {

public:

    CPavpOsServices_Linux(HRESULT& hr, PAVP_DEVICE_CONTEXT pDeviceContext);

    ~CPavpOsServices_Linux();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Sets the desired state for this session.
    /// \par         Details:
    /// \li          Try setting the desired state for this session by calling to KMD.
    /// \li          Upon return, caches the actual state of the session into pPavpSessionStatus.
    /// \li          If requesting 'in init' the new session ID will be returned in pPavpSessionId
    ///
    /// \param       eSessionType               [in] Decode/Transcode session
    /// \param       eSessionMode               [in] Lite/Heavy/ID session mode
    /// \param       eReqSessionStatus          [in] The required status for the session
    /// \param       pPavpSessionStatus         [in] The requested session state.
    /// \param       pPreviousPavpSessionStatus [in, optional] Previous Session state (even if reqSessionStatus is not successful)
    /// \param       pPavpSessionId             [out, optional] The allocated sessionID (only for IN_INIT request)
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT GetSetPavpSessionStatus(
        PAVP_SESSION_TYPE       eSessionType,                           // [in]
        PAVP_SESSION_MODE       eSessionMode,                           // [in]
        PAVP_SESSION_STATUS     eReqSessionStatus,                      // [in]
        PAVP_SESSION_STATUS     *pPavpSessionStatus,                     // [out]
        PAVP_SESSION_STATUS     *pPreviousPavpSessionStatus = NULL,      // [in, optional]
        uint32_t                *pPavpSessionId = NULL);                 // [out, optional]

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       UMD either requests new session id or change state of existing app id
    /// \par         Details:
    ///
    /// \param       eSessionType               [in] Decode/Transcode session
    /// \param       eSessionMode               [in] Lite/Heavy/ID session mode
    /// \param       eReqSessionStatus          [in] The required status for the session
    /// \param       eSMStatus                  [out] KMD status returned to UMD
    /// \param       pPavpSessionId             [out, in] out-cpcontext for INIT, in-session id for InPlay, Terminate
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT ReqNewPavpSessionState(
        PAVP_SESSION_TYPE       eSessionType,                   // [in]
        PAVP_SESSION_MODE       eSessionMode,                   // [in]
        PXP_SM_SESSION_REQ      eReqSessionStatus,              // [in]
        PXP_SM_STATUS           *eSMStatus,                     // [out]
        uint32_t                *pPavpSessionId = nullptr);     // [out optional, in optional]

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Query Cp Context
    ///
    /// \param       pPavpSessionId             [out] cpcontext, [in] session id
    /// \param       pIsSessionInitialized      [out] is sessioninitialized
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT QueryCpContext(uint32_t *pPavpSessionId, bool *pIsSessionInitialized);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Kmd escape wrapper for opening a protected session
    /// \par         Details:
    /// \li          Kmd escape wrapper for opening a protected session. The Pavp session handle
    /// \li          associated with the session is added to KMD session database
    ///
    /// \param       pPavpSessionHandle [in] Private handle of the opened session
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT OpenProtectedSession(PAVP_SESSION_HANDLE* pPavpSessionHandle);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Reads an MMIO register value.
    /// \par         Details:
    /// \li          Can read MMIO value of registers that are denoted by Offset.
    /// \li          This is done by calling KMD.
    ///
    /// \param       eRegType [in] The MMIO register type to read (see PAVP_GPU_REGISTER_TYPE).
    /// \param       rValue   [out] Output from the MMIO read.
    /// \param       Offset   [in, optional] Register offset
    /// \param       Index    [in, optional] Some MMIO registers have both a base offset and index.
    /// \param       GpuState [in, optional] Defines whether the Read mmio escape should be handled with HW access. Not relevant for linux.
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT ReadMMIORegister(
        PAVP_GPU_REGISTER_TYPE      eRegType,                               // [in]
        PAVP_GPU_REGISTER_VALUE&    rValue,                                 // [out]
        PAVP_GPU_REGISTER_OFFSET    Offset = INVALID_REG_OFFSET,            // [in, optional]
        ULONG                       Index = 0,                              // [in, optional]
        PAVP_HWACCESS_GPU_STATE     GpuState = PAVP_HWACCESS_GPU_IDLE);     // [in, optional]

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Writes a value to an MMIO register.
    /// \par         Details:
    /// \li          Writes then reads MMIO register that are part of PAVP_GPU_REGISTER_TYPE enum.
    /// \li          This is done by calling KMD.
    ///
    /// \param       eRegType [in] The MMIO register type to write (see PAVP_GPU_REGISTER_TYPE).
    /// \param       ullValue [in] The value to be written into the register.
    /// \param       rValue   [out] Register value after write.
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT WriteMMIORegister(
        PAVP_GPU_REGISTER_TYPE      regType,    // [in]
        PAVP_GPU_REGISTER_VALUE     ullValue,   // [in]
        PAVP_GPU_REGISTER_VALUE&    rValue);    // [out]

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Send recieve message to Fw via KMD
    /// \par         Details:
    /// \li          Return the parameters used by the media driver while performing WiDi encode.
    ///
    /// \param      pInput           [in] Pointer to the input data (FW command)
    /// \param      InputLen         [in] The buffer size of Input.
    /// \param      OutputBufferLen  [in] The buffer size of Output
    /// \param      pOutput          [out] Pointer to the location of FW response
    /// \param      pRecvSize        [out] Pointer to the buffer size of Output.
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT SendRecvDataTEE(
            uint8_t     *pInput,
            uint32_t    InputLen,
            uint32_t    OutputBufferLen,
            uint8_t     *pOutput,
            uint32_t    *pRecvSize);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Send/Recv data to KMD
    ///
    /// \param      pInputOutput    [in/out] Pointer to the input/output data send/recv from KMD
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT SendRecvKMDMsg(PXP_PROTECTION_INFO *pInputOutput);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Sets WiDi session parameters and marks the session as 'ready'.
    /// \par         Details:
    /// \li          The given parameter will be later read by the media driver while performing WiDi encode.
    ///
    /// \param       rIV              [in] The IV to be used for WiDi encoding.
    /// \param       StreamCtr        [in] The stream counter to be used for WiDi encoding.
    /// \param       bWiDiEnabled     [in] Indicates whether WiDi is currently enabled.
    /// \param       ePavpSessionMode [in] Heavy/Lite/ID session
    /// \param       HDCPType         [in] HDCP type
    /// \param       bDontMoveSession [in] whether to move PAVP session state in KMD
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT SetWiDiSessionParameters(
        uint64_t          rIV,                       // [in]
        uint32_t          StreamCtr,                 // [in]
        BOOL              bWiDiEnabled,              // [in]
        PAVP_SESSION_MODE ePavpSessionMode,          // [in]
        ULONG             HDCPType,                  // [in]
        BOOL              bDontMoveSession = FALSE); // [in]

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Returns the current session status.
    ///
    /// \param       pCurrentStatus [out] The current session status.
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT QuerySessionStatus(PAVP_SESSION_STATUS *pCurrentStatus);    // [out]


    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Creates a cloned linear resource based on a given resource properties and a given data (optional).
    /// \par         Details:
    /// \li          The function clones the given source resource to a new resource in the video memory.
    /// \li          If the SrcDataResource is given, the function copies all its data to the new resource.
    ///
    /// \param       SrcShapeResource       [in] A resource with the wanted shape for the new resource.
    /// \param       pDstResource           [out] A pointer to the output resource (will be filled in by the function).
    /// \param       SrcDataResource        [in, optional] A resource with a data to be copied to the new resources (optional).
    /// \param       dwSrcDataSize          [in, optional] The size of SrcDataResource (optional). Not used in Linux.
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT CloneToLinearVideoResource(
        const PMOS_RESOURCE SrcShapeResource,           // [in]
        PMOS_RESOURCE       *pDstResource,              // [out]
        const PMOS_RESOURCE SrcDataResource = NULL,     // [in, optional]
        DWORD               dwSrcDataSize = 0);         // [in, optional]

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Copies a video mem surface to a system memory surface. Copies only DstSize
    ///
    /// \param       SrcResource        [in] in video memory
    /// \param       DstResource        [in] in system memory
    /// \param       DstSize            [in] size of system memory, we copy this amount
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT CopyVideoResourceToSystemResource(
        CONST PMOS_RESOURCE     pSrcResource,       // [in]
        CONST PMOS_RESOURCE     pDstResource,       // [in]
        DWORD DstSize);                             // [in]

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Copies a video mem surface to a system memory surface. Copies only DstSize
    ///
    /// \param       SrcResource        [in] in video memory
    /// \param       DstResource        [in] in system memory
    /// \param       DstSize            [in] size of system memory, we copy this amount
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT CopyVideoResourceToSystemResourceWithBCS(
        CONST PMOS_RESOURCE     pSrcResource,             // [in]
        CONST PMOS_RESOURCE     pDstResource,             // [in]
        DWORD DstSize);        // [in]

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Blt from the source resource to the destination resource.
    ///
    /// \param       SrcResource [in] The source resource
    /// \param       DstResource [in] The destination resource
    /// \param       isHeavyMode [in] Whether or not heavy mode is enabled
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT BltSurface(
        const PMOS_RESOURCE SrcResource,    // [in]
        PMOS_RESOURCE       DstResource,
        BOOL                isHeavyMode);   // [in]

    BOOL IsSimulationEnabled();

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Whether this OS support parsing assistance from KMD.
    /// \details     This is needed because sometimes (for example, in Linux/Android) the UMD needs to add
    ///              commands such as MI_BATCH_BUFFER_END to the end of batch buffers manually. TRUE if there
    ///              is assistance (and therefore no need to add the command).
    ///
    /// \return      In Linux this function always returns FALSE.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsParsingAssistanceInKmd() { return FALSE; }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Used to check if EPID provisioning enabled
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool IsEPIDProvisioningEnabled() { return true; }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Whether command buffer submission is immediate or deferred.
    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsCBSubmissionImmediate() { return TRUE; }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Retrieve CP Resource String
    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT GetEpidResourcePath(std::string & stEpidResourcePath);

    HRESULT WriteAppIDRegister(PAVP_PCH_INJECT_KEY_MODE ePchInjectKeyMode, BYTE SessionId = PAVP_INVALID_SESSION_ID);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Register for GSC Batch buffer completion event
    /// \par         Details:
    /// \li          UMD needs to wait till GSC batch buffer is completed.
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT NotifyGSCBBCompleteEvent();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Create and Submit GSC buffer
    /// \par         Details:
    /// \param       pGSCInputOutput    [in] pointer to inputoutput GSC buffer
    /// \param       Inputbufferlength  [in] input bufer length
    /// \param       Outputbufferlength [in] output bufer length
    /// \param       HeciClient         [in] indicate which Heci client to use
    /// \param       RecvBytes          [out] number of bytes sent by FW
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT CreateSubmitGSCCommandBuffer(uint8_t  *pGSCInputOutput,
                                         uint32_t Inputbufferlength,
                                         uint32_t Outputbufferlength,
                                         uint8_t  HeciClient,
                                         uint32_t *RecvBytes);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Submit GSC command
    /// \par         Details:
    /// \param       pGSCInput                  [in] pointer to input GSC buffer
    /// \param       pGSCOutput                 [in] pointer to output GSC buffer
    /// \param       Inputbufferlength          [in] input buffer length
    /// \param       Outputbufferlength         [in] output buffer length
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT SubmitGSCCommand(
        PMOS_RESOURCE       pGSCInputResource,
        PMOS_RESOURCE       pGSCOutputResource,
        uint32_t            inputBufferLength,
        uint32_t            outputBufferLength);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Check if command buffer is necessary for GSC command submitting
    /// \param       cmdBufferRequiredForSubmit [out] true for command buffer in use.
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT GetGSCCommandBufferRequired(bool &cmdBufferRequiredForSubmit);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Write GSC command header extension in command buffer
    /// \param       pGSCBufferExtension[in] GSC buffer header part that will be used to send extension
    /// \param       pExtensionBuffer [in] pointer to extension command buffer data
    /// \param       extensionSize [in] size of extension
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT WriteGSCHeaderExtension(uint8_t *pGSCBufferExtension, uint8_t *pExtensionBuffer, uint32_t extensionSize);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Return size of GSC header extension
    /// \param      pExtensionSize size of GSC header extension
    /// \return     S_OK upon success, a failed HRESULT otherwise.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT GetGSCHeaderExtensionSize(uint32_t *pExtensionSize);
    
    /// \brief       Check if huc is authenticated for pxp operations
    /// \param       bIsHucAuthenticated [out] true if huc is autheticated else false
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT IsHucAuthenticated(bool &bIsHucAuthenticated);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Get GSCCommand Buffer
    /// \par         Details:
    /// \param       pCmdBuffer [out] gpu command buffer
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT GetGSCCommandBuffer(MOS_COMMAND_BUFFER* pCmdBuffer);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Submit GSCCommand Buffer
    /// \par         Details:
    /// \param       pCmdBuffer [in] gpu command buffer
    /// \param       bNeedOca   [in] if need OCA dump
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT SubmitGSCCommandBuffer(MOS_COMMAND_BUFFER* pCmdBuffer);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Allocate GSC host session handle
    ///
    /// \par         Details:
    /// \param       pGscHostSessionHandle [in] pointer to GSC host session handle
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT AllocateGSCHostSessionHandle(UINT64 *pGscHostSessionHandle);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Free GSC host session handle
    ///
    /// \par         Details:
    /// \param       GscHostSessionHandle [in] GSC host session handle
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT FreeGSCHostSessionHandle(UINT64 GscHostSessionHandle);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Creates a new instance of GPU Context for the GSC
    /// \par         Details:
    /// \li          GPU Context is a mechanism for sending a command buffer to a specific HW unit
    /// \li          This should be created before any command is sent to the target HW engine
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT CreateGSCGPUContext();

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Use KMD for setting plane enables.
    /// \details    On Windows, DX12 imposes constraints on when command buffers can be created from UMD.
    ///             Using KMD to set plane enables will be cleaner approach than putting plane enables
    ///             in prolog or issuing during command buffer recording.
    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT SetPlaneEnablesUsingKmd(
        uint32_t                SessionId,        // [in] Session ID to be used for setting plane enables
        PAVP_PLANE_ENABLE_TYPE  PlanesToEnable,   // [in] Planes to enable
        PVOID                   pPlaneData,       // [in] Encrypted plane enable data
        uint32_t                uiPlaneDataSize)  // [in] Size of encrypted plane enable data packet
        { return E_NOTIMPL; }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Not implemented in Linux.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT SessionRegistryUpdate(
        CP_USER_FEATURE_VALUE_ID  UserFeatureValueId, // [in]
        PAVP_SESSION_TYPE         eSessionType,       // [in]
        DWORD                     dwSessionId,        // [in]
        CP_SESSIONS_UPDATE_TYPE   eUpdateType);       // [in]

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Query current usage table from KMD memory
    /// \return      TRUE if usage table KMD memory is available
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT QueryUsageTableFromKmd(uint32_t SessionId, PVOID pSessionUsageTable, ULONG* pulUsageTableSize)
    {
        return S_OK;
    }

    uint32_t IsSpecialRegHandlingReq(PAVP_GPU_REGISTER_TYPE eRegType)
    {
        switch(eRegType)
        {
        case PAVP_REGISTER_DEBUG:
            return 1;
        default:
            return 0;
        }
    }
    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Delete current usage table from KMD memory
    /// \return      TRUE if usage table KMD memory is deleted
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT DeleteUsageTableFromKmd(uint32_t SessionId)
    {
        return S_OK;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       handle usage table in Widevine in Windows
    /// \return      TRUE if usage table is handled correctly
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT ProcessUsageTableRequest(
        uint32_t    HWProtectionFunctionID,
        ULONG       ulUsageTableSize,
        PBYTE       pUsageTableFromME,
        uint32_t    SessionId)
    {
        return S_OK;
    }

private:

    // Define copy constructor and operator= as private non declared to make sure the object won't duplicate.
    CPavpOsServices_Linux(const CPavpOsServices_Linux& other);
    CPavpOsServices_Linux& operator=(const CPavpOsServices_Linux& other);

    void ResetMemberVariables();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Waits for the buffer to be flushed down
    /// \par        Details:
    /// \li         This function assumes the current GPU context used by MOS is the video context.
    /// \li         The reason to have two SubmitCommandBuffer() functions, and not have only one with
    /// \li         a default parameter, then that default param will exist in windows too.
    ///
    /// \param      pCmdBuf          [in] ptr to MOS command buffer submitted.
    /// \param      bWaitForCBRender [in] Wait for the command buffer flush to complete before
    ///                                      running any further instructions.
    /// \return     S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT SyncOnSubmit(
        PMOS_COMMAND_BUFFER     pCmdBuf,            // [in]
        BOOL                    bWaitForCBRender    // [in]
        ) const;

    /// \brief       Indicates if we should fallback to v30 FW or not. Fallback is applicable only for Windows.
    /// \par         Details:
    /// \li          Should be called before initializing PCH HAL.
    ///
    /// \return      TRUE or FALSE
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsFwFallbackEnabled();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Copies the data of the given SrcResource to the DstResource
    ///
    /// \param       SrcResource [in] A resource with a data to be copied to the destination resource.
    /// \param       DstResource [in] The destination resource.
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT CopyVideoResource(
        const PMOS_RESOURCE SrcResource,    // [in]
        PMOS_RESOURCE       DstResource);   // [in]

#if (_DEBUG || _RELEASE_INTERNAL)
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       logs up to Size of a surface's bytes for debug only - NOT supported in Linux
    ///
    /// \param       Resource        [in] in video or system memory
    /// \param       Size            [in] size of memory
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    void LogSurfaceBytes(
        CONST PMOS_RESOURCE     pResource,
        DWORD                   Size);
#endif

    uint32_t GetRegisterOffset(PAVP_GPU_REGISTER_TYPE regType);

    CpContext             m_cpContext;

    MEDIA_CLASS_DEFINE_END(CPavpOsServices_Linux)
};

#endif // __CP_OS_SERVICES_LINUX_H__
