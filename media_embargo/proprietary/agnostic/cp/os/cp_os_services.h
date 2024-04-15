/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2012-2022
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

File Name: cp_os_services.h

Abstract:
    This code is part of the PCP design.
    OSservices layer is responsible for OS dependant operations regarding communication with KMD and GPU.
    The layer should support both Windows and Linux/Android architectures. 
    This class acts as an pure interface and contains common code for CP UMD OSserevices layer. 

Environment:
    OS agnostic

Notes:
    None

======================= end_copyright_notice ==================================*/
#ifndef __CP_OS_SERVICES_H__
#define __CP_OS_SERVICES_H__

#include <string>
#include "cp_umd_defs.h"
#include "cp_hal_gpu_defs.h" // for enums.
#include "cp_debug.h"
#include "intel_pavp_core_api.h"
#include "mos_os.h"
#include "pavp_types.h"
#include "cp_user_settings_mgr_ext.h"

#define GSC_MAX_INPUT_BUFF_SIZE 32*1024
#define GSC_MAX_OUTPUT_BUFF_SIZE 32*1024
#define WIDI_ME_FIX_CLIENT 18
#define PAVP_ME_FIX_CLIENT 17
#define MKHI_ME_FIX_CLIENT 7
#define EMULATION_ME_FIX_CLIENT 5

#define CP_OS_FUNCTION_ENTER    CP_FUNCTION_ENTER(MOS_CP_SUBCOMP_OS)
#define CP_OS_FUNCTION_EXIT(hr) CP_FUNCTION_EXIT(MOS_CP_SUBCOMP_OS, hr)

#define CP_OS_ASSERTMESSAGE(_message, ...) \
    CP_ASSERTMESSAGE(MOS_CP_SUBCOMP_OS, _message, ##__VA_ARGS__)

#define CP_OS_VERBOSEMESSAGE(_message, ...) \
    CP_VERBOSEMESSAGE(MOS_CP_SUBCOMP_OS, _message, ##__VA_ARGS__)

#define CP_OS_NORMALMESSAGE(_message, ...) \
    CP_NORMALMESSAGE(MOS_CP_SUBCOMP_OS, _message, ##__VA_ARGS__)

#define CP_OS_ASSERT(_expr) CP_ASSERT(MOS_CP_SUBCOMP_OS, _expr)

#define CP_OS_CHK_NULL_RETURN(_ptr) \
    CP_CHK_NULL_RETURN(MOS_CP_SUBCOMP_OS, _ptr)

#define CP_OS_CHK_STATUS_RETURN(_stmt) \
    CP_CHK_STATUS_RETURN(MOS_CP_SUBCOMP_OS, _stmt)

#define UNLOCK_RESOURCE(pResource, pSurface)                            \
    if (pSurface                            != nullptr &&               \
        m_pOsInterface                      != nullptr &&               \
        m_pOsInterface->pfnUnlockResource   != nullptr)                 \
    {                                                                   \
        m_pOsInterface->pfnUnlockResource(m_pOsInterface, pResource);   \
    }

#define FREE_RESOURCE(pResource)                                        \
    if (!Mos_ResourceIsNull(pResource)             &&                   \
        m_pOsInterface                  != nullptr &&                   \
        m_pOsInterface->pfnFreeResource != nullptr)                     \
    {                                                                   \
        m_pOsInterface->pfnFreeResource(m_pOsInterface, pResource);     \
    }

#define FIRST_TRANSCODE_SESSION_ID 0x70 // In transcode sessions the hardware returns the session id starts from 0x80.
                                        // Because transcode sessions should start from 16 and on we reduce 0x70. 
                                        // In order to set the first transcode session to 16 (0x10).

class CPavpOsServices
{

public:

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Creates a new CPavpOsServices object
    /// \par         Details:
    ///
    /// \param       hr             [out] return value  S_OK upon success, a failed HRESULT otherwise.
    /// \param       pDeviceContext [in] device context to be used.
    /// \return      New instance of CPavpOsServices.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    static std::shared_ptr<CPavpOsServices> PavpOsServicesFactory(
        HRESULT&            hr,                     // [out]
        PAVP_DEVICE_CONTEXT pDeviceContext);        // [in]

    CPavpOsServices(HRESULT& hr, PAVP_DEVICE_CONTEXT pDeviceContext);

    CPavpOsServices(const CPavpOsServices&) = delete;

    CPavpOsServices& operator=(const CPavpOsServices&) = delete;

    virtual ~CPavpOsServices();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Initialize the MOS interface
    /// \param       pOsDriverContext    [in]  A pointer to the device context which is used by MOS.
    /// \param       pOsInterface        [out] The MOS interface to initialize.
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    static HRESULT InitMosInterface(
        PMOS_CONTEXT    pOsDriverContext,  // [in]
        PMOS_INTERFACE& pOsInterface);     // [out]

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Destroys the MOS interface
    /// \param       pOsInterface        [in] The MOS interface to destroy.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void DestroyMosInterface(PMOS_INTERFACE& pOsInterface); // [in]

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Programs the app ID register so the ME can inject a key into the GPU.
    /// \par         Details:
    /// \li          Should be called right before a key will be injected from the PCH using DMI.
    /// \li          Automatically attempts to retry a set amount of times before failing.
    /// \li          FW 4.0 require program Appid with S1 key + FR
    /// \li          legacy FW use default pch key type, S1 key only.
    ///
    /// \param       ePchInjectKeyMode    [in] pch key inject mode.
    /// \param       SessionId            [in] The overriding session ID.
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT ProgramAppIDRegister(CONST PAVP_PCH_INJECT_KEY_MODE ePchInjectKeyMode, BYTE SessionId = PAVP_INVALID_SESSION_ID);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Reads an MMIO register value.
    /// \par         Details:
    /// \li          Can read MMIO value of registers that are part of PAVP_GPU_REGISTER_TYPE enum or denoted by Offset enum.
    /// \li          This is done by calling KMD.
    ///
    /// \param       eRegType [in] The MMIO register type to read (see PAVP_GPU_REGISTER_TYPE).
    /// \param       rValue   [out] Output from the MMIO read.
    /// \param       Offset   [in, optional] Register offset
    /// \param       Index    [in, optional] Some MMIO registers have both a base offset and index.
    /// \param       GpuState [in, optional] Defines whether the Read mmio escape should be handled with HW access. 
    ///                                      Setting this param to PAVP_HWACCESS_GPU_IDLE cause the OS to perform second level of synchronization.
    ///                                      In practical it means that only a single thread is within the KMD, the graphics HW is idle and
    ///                                      No DMA bufferes are being processed by the driver.
    ///                                      Therefore setting this param to PAVP_HWACCESS_GPU_IDLE will ensure faster escape execution, but it should be carefully used.
    ///                                      This parameter is relevant for Windows only, we ignore it in Linux.
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT ReadMMIORegister(
        PAVP_GPU_REGISTER_TYPE      eRegType,                               // [in]
        PAVP_GPU_REGISTER_VALUE&    rValue,                                 // [out]
        PAVP_GPU_REGISTER_OFFSET    Offset = INVALID_REG_OFFSET,            // [in, optional]
        ULONG                       Index = 0,                              // [in, optional]
        PAVP_HWACCESS_GPU_STATE     GpuState = PAVP_HWACCESS_GPU_IDLE) = 0; // [in, optional]

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
    virtual HRESULT WriteMMIORegister(
        PAVP_GPU_REGISTER_TYPE      eRegType,                // [in]
        PAVP_GPU_REGISTER_VALUE     ullValue,                // [in]
        PAVP_GPU_REGISTER_VALUE&    rValue) = 0;             // [out]

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       get simulation enable state, always return false in release driver
    ///
    /// \return      simulation enable state
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual BOOL IsSimulationEnabled() = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Sets WiDi session parameters and marks the session as 'ready'.
    /// \par         Details:
    /// \li          The given parameter will be later read by the media driver while performing WiDi encode.
    ///
    /// \param       rIV              [in] The IV to be used for WiDi encoding.
    /// \param       StreamCtr        [in] The stream counter to be used for WiDi encoding.
    /// \param       bWiDiEnabled     [in] Indicates whether WiDi is currently enabled.
    /// \param       ePavpSessionMode [in] Heavy/Lite/ID session
    /// \param       HDCPType         [in] Indicates the type of HDCP authenticated
    /// \param       bDontMoveSession [in] Indicates whether move PAVP session status in coreu. used in
    ///                                 hdcp2_destroy to clear widi parameter without changing session status.
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT SetWiDiSessionParameters(
                uint64_t            rIV,                            // [in]
                uint32_t            StreamCtr,                      // [in]
                BOOL                bWiDiEnabled,                   // [in]
                PAVP_SESSION_MODE   ePavpSessionMode,               // [in]
                ULONG               HDCPType,                       // [in]
                BOOL                bDontMoveSession = FALSE) = 0;  // [in]

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Returns the current session status.
    ///
    /// \param       pCurrentStatus [out] The current session status.
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT QuerySessionStatus(PAVP_SESSION_STATUS *pCurrentStatus) = 0;  // [out]

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Try setting the desired state for this session by calling to KMD using PavpEscape.
    /// \par         Details:
    /// \li          Try setting the desired state for this session by calling to KMD using PavpEscape.
    /// \li          Upon return, caches the actual state of the session into pPavpSessionStatus.
    /// \li          If requesting 'in init' the new session ID will be returned in pPavpSessionId
    ///
    /// \param       eSessionType                   [in] Decode/Transcode session
    /// \param       eSessionMode                   [in] Heavy/Lite/ID session
    /// \param       eReqSessionStatus              [in] The required status for the session 
    /// \param       pPavpSessionStatus             [in] The requested session state.
    /// \param       pPreviousPavpSessionStatus     [in, optional] Previous Session state (even if reqSessionStatus is not successful)
    /// \param       pPavpSessionId                 [out, optional] The allocated sessionID (only for IN_INIT request)
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT GetSetPavpSessionStatus(
        PAVP_SESSION_TYPE       eSessionType,                           // [in]
        PAVP_SESSION_MODE       eSessionMode,                           // [in]
        PAVP_SESSION_STATUS     eReqSessionStatus,                      // [in]
        PAVP_SESSION_STATUS*    pPavpSessionStatus,                     // [in]
        PAVP_SESSION_STATUS*    pPreviousPavpSessionStatus = nullptr,   // [in, optional]
        uint32_t*               pPavpSessionId = nullptr) = 0;          // [out, optional]

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       UMD either requests new session id or change state of existing app id
    ///
    /// \param       eSessionType               [in] Decode/Transcode session
    /// \param       eSessionMode               [in] Lite/Heavy/ID session mode
    /// \param       eReqSessionStatus          [in] The required status for the session
    /// \param       pSMStatus                  [out] KMD status returned to UMD
    /// \param       pPavpSessionId             [out, in] out-cpcontext for INIT, in-session id for InPlay, Terminate
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT ReqNewPavpSessionState(
        PAVP_SESSION_TYPE       eSessionType,                   // [in]
        PAVP_SESSION_MODE       eSessionMode,                   // [in]
        PXP_SM_SESSION_REQ      eReqSessionStatus,              // [in]
        PXP_SM_STATUS           *pSMStatus,                     // [out]
        uint32_t                *pPavpSessionId = nullptr) = 0; // [out optional, in optional]

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Query Cp Context
    ///
    /// \param       pPavpSessionId             [out] cpcontext, [in] session id
    /// \param       pIsSessionInitlaized       [out] is sessioninitialized
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT QueryCpContext(
        uint32_t                  *pPavpSessionId,                    // [in/out]
        bool                    *pIsSessionInitlaized) = 0;         // [out]

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Creates a cloned linear resource based on a given resource properties and a given data (optional).
    /// \par         Details:
    /// \li          If the source resource is in the system memory, the function clones it into the video memory, makes it linear
    ///              and copies all its data to the new resource.
    /// \li          If the source resource is in the video memory, the function clones it to a new linear resource.
    /// \li          If the pTargetDataResource is specified, its surface is copied to the newly created resource. 
    ///
    /// \param       SrcShapeResource      [in] A resource with the wanted shape for the new resource.
    /// \param       pDestResource         [out] A pointer to the output resource (will be filled in by the function).
    /// \param       SrcDataResource       [in, optional] A resource with a data to be copied to the new resources (optional).
    /// \param       dwSrcDataSize         [in, optional] The size of SrcDataResource (optional).
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT CloneToLinearVideoResource(
       CONST PMOS_RESOURCE     SrcShapeResource,                // [in]
       PMOS_RESOURCE           *pDstResource,                   // [out]
       CONST PMOS_RESOURCE     SrcDataResource = nullptr,       // [in, optional]
       DWORD                   dwSrcDataSize = 0) = 0;          // [in, optional]

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Register for GSC Batch buffer completion event
    /// \par         Details:
    /// \li          UMD needs to wait till GSC batch buffer is completed.
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT NotifyGSCBBCompleteEvent() = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Create and Submit GSC buffer
    /// \par         Details:
    /// \param       pGSCInputOutput    [in] pointer to inputoutput GSC buffer
    /// \param       Inputbufferlength  [in] input bufer length
    /// \param       Outputbufferlength [in] output bufer length
    /// \param       HeciCLient         [in] indicate which heci client to use
    /// \param       RecvBytes          [out] number of bytes sent by FW
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT CreateSubmitGSCCommandBuffer(uint8_t  *pGSCInputOutput,
                                                 uint32_t Inputbufferlength,
                                                 uint32_t Outputbufferlength,
                                                 uint8_t  HeciClient,
                                                 uint32_t *RecvBytes) = 0;

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
        uint32_t            outputBufferLength) = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Check if command buffer is necessary for GSC command submitting
    /// \param       cmdBufferRequiredForSubmit [out] true for command buffer in use.
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT GetGSCCommandBufferRequired(bool &cmdBufferRequiredForSubmit) = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Write GSC command header extension in command buffer
    /// \param       pGSCBufferExtension[in] GSC buffer header part that will be used to send extension
    /// \param       pExtensionBuffer [in] pointer to extension command buffer data
    /// \param       extensionSize [in] size of extension
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT WriteGSCHeaderExtension(uint8_t *pGSCBufferExtension, uint8_t *pExtensionBuffer, uint32_t extensionSize) = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Return size of GSC header extension
    /// \param      pExtensionSize size of GSC header extension
    /// \return     S_OK upon success, a failed HRESULT otherwise.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT GetGSCHeaderExtensionSize(uint32_t *pExtensionSize) = 0;

    /// \brief       Check if huc is authenticated for pxp operations
    /// \param       bIsHucAuthenticated [out] true if huc is autheticated else false
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT IsHucAuthenticated(bool &bIsHucAuthenticated) = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Get GSCCommand Buffer
    /// \par         Details:
    /// \param       pCmdBuffer [out] gpu command buffer
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT GetGSCCommandBuffer(MOS_COMMAND_BUFFER* pCmdBuffer) = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Submit GSCCommand Buffer
    /// \par         Details:
    /// \param       pCmdBuffer [in] gpu command buffer
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT SubmitGSCCommandBuffer(MOS_COMMAND_BUFFER* pCmdBuffer) = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Allocate GSC host session handle
    ///
    /// \par         Details:
    /// \param       pGscHostSessionHandle [in] pointer to GSC host session handle
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT AllocateGSCHostSessionHandle(UINT64 *pGscHostSessionHandle) = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Free GSC host session handle
    ///
    /// \par         Details:
    /// \param       GscHostSessionHandle [in] GSC host session handle
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT FreeGSCHostSessionHandle(UINT64 GscHostSessionHandle) = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Creates a new instance of GPU Context for the GSC
    /// \par         Details:
    /// \li          GPU Context is a mechanism for sending a command buffer to a specific HW unit
    /// \li          This should be created before any command is sent to the target HW engine
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT CreateGSCGPUContext() = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Copies a video mem surface to a system memory surface. Copies only DstSize
    ///
    /// \param       SrcResource        [in] in video memory
    /// \param       DstResource        [in] in system memory
    /// \param       DstSize            [in] size of system memory, we copy this amount
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT CopyVideoResourceToSystemResource(
        CONST PMOS_RESOURCE     pSrcResource,             // [in]
        CONST PMOS_RESOURCE     pDstResource,
        DWORD DstSize) = 0;        // [in] 

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Copies a video mem surface to a system memory surface. Copies only DstSize
    ///
    /// \param       SrcResource        [in] in video memory
    /// \param       DstResource        [in] in system memory
    /// \param       DstSize            [in] size of system memory, we copy this amount
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT CopyVideoResourceToSystemResourceWithBCS(
        CONST PMOS_RESOURCE     pSrcResource,             // [in]
        CONST PMOS_RESOURCE     pDstResource,
        DWORD DstSize) = 0;        // [in] 

#if (_DEBUG || _RELEASE_INTERNAL)
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       logs up to Size of a surface's bytes for debug only
    ///
    /// \param       Resource        [in] in video or system memory
    /// \param       Size            [in] size of memory
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual void LogSurfaceBytes(
        CONST PMOS_RESOURCE     pResource,             
        DWORD                   Size) = 0; 

#endif

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Blt from the source resource to the destination resource.
    /// \par         Details:
    /// \li          The blt operation might need to resize and tile the surface according to the destination resource properties.
    /// \li          The function returns after the blt operation is over.
    ///
    /// \param       SrcResource [in] The source resource
    /// \param       DstResource [in] The destination resource
    /// \param       isHeavyMode [in] Whether or not heavy mode is enabled
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT BltSurface(
        CONST PMOS_RESOURCE     SrcResource,
        PMOS_RESOURCE           DstResource,
        BOOL                    isHeavyMode) = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Allocates a resource and copies a possible buffer into it.
    /// \param       pSrcBuffer     [in] The source buffer
    /// \param       pSrcResource   [in] Resource pointer to allocate resource
    /// \param       size           [in] Size of source buffer and allocated resource
    /// \param       pResourceSize  [out] size of the aligned resource, >= size
    /// \param       memType        [in] Memory type of Mos_MemPool
    /// \param       notLockable    [in] indicate this resource is notLockable
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT AllocateResource(
        PBYTE           pSrcBuffer,
        PMOS_RESOURCE   pSrcResource,
        SIZE_T          size,
        GMM_GFX_SIZE_T  *pResourceSize,
        Mos_MemPool     memType = MOS_MEMPOOL_VIDEOMEMORY,
        BOOL            notLockable = false);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Update gmm usage type of resource
    /// \param       pOsResource        [in] The source buffer
    /// \param       resUsageType    [in] Resource pointer to allocate resource
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT UpdateResourceUsageType(
        PMOS_RESOURCE           pOsResource,
        MOS_HW_RESOURCE_DEF     resUsageType);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Deallocates a resource and copies its contents to a  possible buffer.
    /// \param       pDstBuffer     [in] The dest buffer
    /// \param       pDstResource   [in] Resource pointer to deallocate resource
    /// \param       size           [in] Size of source buffer and allocated resource
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT DeallocateResource(
        PBYTE           pDstBuffer,
        PMOS_RESOURCE   pDstResource,
        SIZE_T          size);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Allocates the necessary resources (src and dst as linear resources) and copies the
    ///              buffer to the new src resource.
    /// \param       pSrcBuffer     [in] The source buffer
    /// \param       bufSize        [in] Size of pSrcBuffer
    /// \param       pSrcResource   [in] Resource pointer to allocate resource
    /// \param       pDstResource   [in] Resource pointer to allocate resource
    /// \param       *pResourceSize [out] Size of the allocated resources
    /// \param       isDstResHwWriteOnly [in] Indicate the destination resource is HardwareWriteOnly
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT PrepareDecryptionResources(
        PBYTE           pSrcBuffer,
        SIZE_T          bufSize,
        PMOS_RESOURCE   pSrcResource,
        PMOS_RESOURCE   pDstResource,
        GMM_GFX_SIZE_T  *pResourceSize,
        BOOL            isDstResHwWriteOnly = FALSE);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Copies resource to dest buffer and deallocates the resources.
    /// \param       pSrcResource   [in] Resource pointer to deallocate
    /// \param       pDstResource   [in] Resource pointer to deallocate
    /// \param       bufSize        [in] Width of pDstBuffer
    /// \param       pDstBuffer     [out] Destination buffer
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT ReleaseDecryptionResources(
        PMOS_RESOURCE   pSrcResource,
        PMOS_RESOURCE   pDstResource,
        SIZE_T          bufSize,
        PBYTE           pDstBuffer);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Frees the given resource.
    ///
    /// \param       Resource [in] The resource to destroy.
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT FreeResource(PMOS_RESOURCE &Resource) const;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Returns the size of the resource surface.
    ///
    /// \param       Resource [in] The resource to get the size of.
    /// \return      The size of the resource surface.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    ULONG GetResourceSize(CONST PMOS_RESOURCE Resource);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Returns the pitch of the resource surface.
    ///
    /// \param       Resource [in] The resource to get the pitch of.
    /// \return      The pitch of the resource surface.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    ULONG GetResourcePitch(CONST PMOS_RESOURCE Resource);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Creates a new instance of GPU Context for the MFX engine
    /// \par         Details:
    /// \li          GPU Context is a mechanism for addressing a command buffer to a specific HW engine
    /// \li          This should be created before any command is sent to the target HW engine
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT CreateVideoGPUContext();

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Whether this OS supports parsing assistance from KMD.
    /// \details     This is needed because sometimes (for example, in Linux/Android) the UMD needs to add
    ///              commands such as MI_BATCH_BUFFER_END to the end of batch buffers manually. TRUE if there
    ///              is assistance (and therefore no need to add the command).
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual BOOL IsParsingAssistanceInKmd() = 0;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Whether EPID Provisioning Enabled.
    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual bool IsEPIDProvisioningEnabled() = 0;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Whether command buffer submission is immediate or deferred.
    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual BOOL IsCBSubmissionImmediate() = 0;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Retrieve CP Resource
    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT GetEpidResourcePath(std::string & stEpidResourcePath) = 0;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Use KMD for setting plane enables.
    /// \details    On Windows, DX12 imposes constraints on when command buffers can be created from UMD.
    ///             Using KMD to set plane enables will be cleaner approach than putting plane enables
    ///             in prolog or issuing during command buffer recording.
    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT SetPlaneEnablesUsingKmd(
        uint32_t                SessionId,            // [in] Session ID to be used for setting plane enables
        PAVP_PLANE_ENABLE_TYPE  PlanesToEnable,       // [in] Planes to enable
        PVOID                   pPlaneData,           // [in] Encrypted plane enable data
        uint32_t                  uiPlaneDataSize) = 0; // [in] Size of encrypted plane enable data packet

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Query current usage table from KMD memory
    /// \details     
    /// \return      TRUE if usage table KMD memory is available
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT QueryUsageTableFromKmd(uint32_t SessionId, PVOID pSessionUsageTable, ULONG* pulUsageTableSize) = 0;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Delete current usage table from KMD memory
    /// \details     
    /// \return      TRUE if usage table KMD memory is deleted
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT DeleteUsageTableFromKmd(uint32_t SessionId) = 0;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       handle usage table in Widevine in Windows
    /// \details     
    /// \return      TRUE if usage table is handled correctly
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT ProcessUsageTableRequest(
        uint32_t HWProtectionFunctionID,
        ULONG ulUsageTableSize,
        PBYTE pUsageTableFromME,
        uint32_t SessionId) = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Writes a DWORD value to a registry key
    /// \par         Details:
    /// \li          The requested data will be written to the specified registry key. In Linux will be written to /etc/registry.txt
    ///
    /// \param       UserFeatureValueId [in] The key value ID, detailed registry info can be found in mos_utilities.cpp
    /// \param       dwData             [in] Data to be written to the key value.
    /// \return      void
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    void WriteRegistry(
        CP_USER_FEATURE_VALUE_ID  UserFeatureValueId, // [in]
        DWORD                     dwData);            // [in]

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Reads a DWORD value from a registry key
    /// \par         Details:
    /// \li          The data will be read from the specified registry key. In Linux will be read from /etc/registry.txt
    ///
    /// \param       UserFeatureValueId [in] The key value ID, detailed registry info can be found in mos_utilities.cpp
    /// \param       pdwData            [in] A pointer to store the data from the key.
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT ReadRegistry(
        CP_USER_FEATURE_VALUE_ID  UserFeatureValueId, // [in]
        PDWORD                    pdwData);           // [out]

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Updates a DWORD value from a registry key which represent the open session status.
    /// \par         Details:
    /// \li          The data will be read from the specified registry key. In Linux it will be read from /etc/registry.txt
    /// \li          Afterwards the value is updated according the CP_SESSIONS_UPDATE_TYPE.
    ///
    /// \param       UserFeatureValueId [in] the key value ID, detailed registry info can be found in mos_utilities.cpp
    /// \param       eSessionType       [in] an enum that represent the session type (Transcode/Decode).
    /// \param       dwSessionId        [in] the session ID in DWORD.
    /// \param       eUpdateType        [in] an enum that reperesnt the required update option (open/close).
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT SessionRegistryUpdate(
        CP_USER_FEATURE_VALUE_ID  UserFeatureValueId, // [in]
        PAVP_SESSION_TYPE         eSessionType,       // [in]
        DWORD                     dwSessionId,        // [in]
        CP_SESSIONS_UPDATE_TYPE   eUpdateType) = 0;   // [in]

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Gets current platform info
    /// \param       platform [out] Current platform info
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT GetPlatform(PLATFORM& sPlatform);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Gets WA Table info
    /// \param       WA Table [out] Current platform info
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT GetWaTable(MEDIA_WA_TABLE& pWATable);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Gets sku Table info
    /// \return      Pointer to Sku Table if successful, otherwise nullptr. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual MEDIA_FEATURE_TABLE *GetSkuTable();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Allocates a new linear resource with the given parameters.
    ///
    /// \param       eType          [in] The resource type (buffer/2D).
    /// \param       dwWidth        [in] The resource width.
    /// \param       dwHeight       [in] The resource height.
    /// \param       pResourceName  [in, optional] A name to be attached to the resource for debugging purposes (can be NULL).
    /// \param       eFormat        [in] The resource format.
    /// \param       pResource      [out] The created resource.
    /// \param       bSystemMem     [in] Represent whether resource allocated in system memory.
    /// \param       bLockable      [in] Represent whether resource is lockable.
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT AllocateLinearResource(
        MOS_GFXRES_TYPE eType,              // [in]
        DWORD           dwWidth,            // [in]
        DWORD           dwHeight,           // [in]
        PCSTR           pResourceName,      // [in, optional]
        MOS_FORMAT      eFormat,            // [in]
        PMOS_RESOURCE& pResource,          // [out]
        BOOL            bSystemMem = FALSE,        // [in, optional]
        BOOL            bLockable = FALSE) const;    // [in, optional]

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Copies from CPU to a GPU linear, buffer resource, locking and unlocking as needed.
    ///
    /// \param       sResource  [in] The resource to copy to.
    /// \param       pSrc       [in] The system memory.
    /// \param       srcSize    [in] The system memory size.
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT CopyToLinearBufferResource(
        MOS_RESOURCE&   sResource,  // [in]
        PBYTE           pSrc,       // [in]
        uint32_t            srcSize);   // [in]

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Indicates the way in which a resource can be locked, for either just reading or additionally
    ///        writing. This is used by CPavpOsServices::LockResource.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    enum class LockType 
    {
        Read,
        Write
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Locks the resource.
    /// \par         Details:
    /// \li          The function assumes sResource is a video resource. 
    ///
    /// \param       sResource  [in] The resource to lock.
    /// \param       lockType   [in] How the resource should be locked. See CPavpOsServices::LockType.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    void *LockResource(MOS_RESOURCE& res, const LockType lockType) const;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Fills the given resource with the given data.
    /// \par         Details:
    /// \li          Locks a resource and fills it up with one recurring char.
    /// \li          The function assumes the destination buffer has enough space for the operation.
    ///
    /// \param       sResource  [in] The resource to fill.
    /// \param       uiSize     [in] The size of the buffer to fill.
    /// \param       ucValue    [in] The value to fill in each byte of the resource.
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT FillResourceMemory(
        MOS_RESOURCE&   sResource,  // [in]
        uint32_t            uiSize,     // [in]
        UCHAR           ucValue);   // [in]

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Waits for the buffer to be flushed down
    /// \par        Details:
    /// \li         This function assumes the current GPU context used by MOS is the video context.
    /// \li         It does nothing for windows.
    ///
    /// \param      pCmdBuf          [in] ptr to MOS command buffer submitted.
    /// \param      bWaitForCBRender [in] Wait for the command buffer flush to complete before
    ///                                      running any further instructions.
    /// \return     S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT SyncOnSubmit(
        PMOS_COMMAND_BUFFER     pCmdBuf,            // [in]
        BOOL                    bWaitForCBRender   // [in]
        ) const = 0;

    PAVP_DEVICE_CONTEXT m_pPavpDeviceContext;
    PMOS_INTERFACE      m_pOsInterface;
    DWORD               m_isForceBltCopyEnabled;

protected:

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Converts a PAVP_SESSION_TYPE value to a KM_PAVP_SESSION_TYPE applicable for KMD
    /// \param       type [in] The PAVP_SESSION_TYPE value to convert.
    /// \return      The corresponding KM_PAVP_SESSION_TYPE value.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    static KM_PAVP_SESSION_TYPE KmSessionTypeFromApiSessionType(PAVP_SESSION_TYPE type); 

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Converts a PAVP_SESSION_MODE value to a KM_PAVP_SESSION_MODE applicable for KMD
    /// \param       mode [in] The PAVP_SESSION_MODE value to convert.
    /// \return      The corresponding KM_PAVP_SESSION_MODE value.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    static KM_PAVP_SESSION_MODE KmSessionModeFromApiSessionMode(PAVP_SESSION_MODE mode);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Contacts the KMD to actually write the PAVP register.
    /// \par         Details:
    /// \li          A failure here may be expected. Another app may be initializing PAVP.
    ///
    /// \param       ePchInjectKeyMode [in] The pch inject key mode.
    /// \param       SessionId         [in] The overriding session ID.
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT WriteAppIDRegister(PAVP_PCH_INJECT_KEY_MODE ePchInjectKeyMode, BYTE SessionId = PAVP_INVALID_SESSION_ID) = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Clears class members 
    ///
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    void ResetMemberVariables();

private:

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Initialize the MOS interface
    /// \param       pOsDriverContext [in] A pointer to the device context which is used by MOS.
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT InitMosInterface(PMOS_CONTEXT pOsDriverContext)     // [in]
    {
        return InitMosInterface(pOsDriverContext, m_pOsInterface);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Destroys the MOS interface
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    void DestroyMosInterface()
    {
        DestroyMosInterface(m_pOsInterface);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Controls whether CP registry keys are reported.
    /// \par         Details:
    /// \li          Controls whether CP registry keys are reported, and is called WriteRegistry().
    /// \return      TRUE if whether CP registry writes are enabled.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL ReportRegistryEnabled();

    MEDIA_CLASS_DEFINE_END(CPavpOsServices)
};

#endif // __CP_OS_SERVICES_H__
