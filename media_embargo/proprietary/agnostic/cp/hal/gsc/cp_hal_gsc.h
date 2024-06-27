/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2013-2020
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

File Name: cp_hal_gsc.h

Abstract:
class for GSC HAL for PAVP functions

Environment:
OS agnostic - should support - Wihdows, Linux, Android
Platform: Should support from DashG-1 onwards

Notes:
Due to open source restrictions, GSC(Graphics Security Controller)
has been replaced with RTE(Root Trust Engine) in many files

======================= end_copyright_notice ==================================*/

#ifndef __CP_HAL_GSC_H__
#define __CP_HAL_GSC_H__

#include <string>
#include <vector>
#include <bitset>
#include "cp_umd_defs.h"
#include "cp_debug.h"
#include "intel_pavp_epid_api.h"
#include "intel_pavp_core_api.h"
#include "cp_hal_root_trust.h"
#include "cp_hal_gsc_defs.h"

#define GSC_HAL_FUNCTION_ENTER    CP_FUNCTION_ENTER(MOS_CP_SUBCOMP_TEE_HAL)
#define GSC_HAL_FUNCTION_EXIT(rc) CP_FUNCTION_EXIT(MOS_CP_SUBCOMP_TEE_HAL, rc)

#define GSC_HAL_ASSERTMESSAGE(_message, ...) \
    CP_ASSERTMESSAGE(MOS_CP_SUBCOMP_TEE_HAL, _message, ##__VA_ARGS__)

#define GSC_HAL_VERBOSEMESSAGE(_message, ...) \
    CP_VERBOSEMESSAGE(MOS_CP_SUBCOMP_TEE_HAL, _message, ##__VA_ARGS__)

#define GSC_HAL_NORMALMESSAGE(_message, ...) \
    CP_NORMALMESSAGE(MOS_CP_SUBCOMP_TEE_HAL, _message, ##__VA_ARGS__)

#define GSC_HAL_ASSERT(_expr) CP_ASSERT(MOS_CP_SUBCOMP_TEE_HAL, _expr)

#define GSC_HAL_CHK_STATUS_MESSAGE_WITH_HR(_stmt, _message, ...)            \
    CP_CHK_STATUS_MESSAGE_WITH_HR(MOS_CP_SUBCOMP_TEE_HAL, _stmt, _message, ##__VA_ARGS__)

#define GSC_HAL_CHK_STATUS(_stmt)            \
    CP_CHK_STATUS(MOS_CP_SUBCOMP_TEE_HAL, _stmt)

#define GSC_HAL_CHK_NULL(_ptr) \
    CP_CHK_NULL(MOS_CP_SUBCOMP_TEE_HAL, _ptr)

typedef enum
{
    GSCCS_MEMORY_HEADER_SUCCESS = 0,
    GSCCS_MEMORY_HEADER_INVALID_HOST_HANDLE,
    GSCCS_MEMORY_HEADER_INVALID_GSC_HANDLE,
    GSCCS_MEMORY_HEADER_INVALID_HEADER_MARKER,
    GSCCS_MEMORY_HEADER_INVALID_GSC_CLIENT,
    GSCCS_MEMORY_HEADER_INVALID_RESERVED_FILED,
    GSCCS_MEMORY_HEADER_INVALID_HEADER_VERSION,
    GSCCS_MEMORY_HEADER_INVALID_PENDING_FLAG,
    GSCCS_MEMORY_HEADER_INVALID_MESSAGE_SIZE,
    GSCCS_MEMORY_HEADER_INVALID_STATUS_FIELD,
    GSCCS_MEMORY_HEADER_INVALID_SESSION_CLEANUP,
    GSCCS_MEMORY_HEADER_INVALID_MESSAGE_EXTENSION,
    GSCCS_MEMORY_HEADER_INVALID_BUS_MESSAGE,
    GSCCS_MEMORY_HEADER_INTERNAL_ERROR,
    GSCCS_MEMORY_HEADER_INVALID_MESSAGE_EXTENSION_LENGTH,
    GSCCS_MEMORY_HEADER_PROXY_NOT_READY
} GSCCS_MEMORY_HEADER_STATUS;

#define SW_PROXY_NOT_READY_RETRY_INTERVAL     50
#define SW_PROXY_NOT_READY_MAX_NUM_RETRY      10

#define SW_PROXY_CHECK_MAX_NUM_RETRY          10
#define SW_PROXY_CHECK_RETRY_INTERVAL         50
#define GSC_OUTPUT_BUFFER_HEAD_MARKER         0xABCDDCBA

class CPavpOsServices;

class CPavpGscHal : public CPavpRootTrustHal
{
public:

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sends a FW PAVP init command to the GSC.
    /// \param      uiPavpMode [in] Pavp Mode (Lite, Heavy or Isolated Decode)
    /// \param      uiAppId [in] the App id (slot id) to use
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT PavpInit4(
        PAVP_FW_PAVP_MODE uiPavpMode,
        uint32_t uiAppId);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Enable huc streamout bit for lite mode for debug purpose.
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT EnableLiteModeHucStreamOutBit();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Sends a Daisy Chain command to the FW to get the Key translated by the FW. 
    /// \par         Details:
    /// \li          Helper function to send a single daisy chain command to the GSC FW and check the return codes. 
    /// 
    /// \param       [in] Key to translate
    /// \param       [in] boolean to indicate whether the key to translate is a decrypt key
    /// \param       [out] Stream Key to hold the returned translated key from the FW.
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT SendDaisyChainCmd(StreamKey pKeyToTranslate, bool IsDKey, StreamKey pTranslatedKey);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Initializes the GSC HAL.
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT Init();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Cleans up the GSC HAL.
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT Cleanup();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Perform EPID Provisioning Function
    /// \param      stEpidBinaryPath     [in]Input Path String to binary file
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT PerformProvisioning(std::string &stEpidResourcePath);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      GetPlayReadyVersion.
    /// \par        Note: This function will return an error if PlayReady support
    /// \li         is not enabled in the BIOS for systems which require it.
    ///
    /// \param      version     [out] PlayReady pk version number
    ///
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT GetPlayReadyVersion(uint32_t &version);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Authenticates and transcrypts encrypted kernels from video processor.
    /// 
    /// \param      uiTranscryptMetaDataSize    [in] Number of bytes in transcrypt metadata structure.
    /// \param      pTranscryptMetaData         [in] Pointer to transcrypt metadata structure.
    /// \param      uiKernelsSize               [in] Number of bytes in encrypted/transcrypted kernels.
    /// \param      pEncryptedKernels           [in] Pointer to encrypted kernels.
    /// \param      transcryptedKernels         [out] Vector of transcrypted kernels.
    /// 
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT TranscryptKernels(
        uint32_t            uiTranscryptMetaDataSize,
        const uint32_t      *pTranscryptMetaData,
        uint32_t            uiKernelsSize,
        const uint32_t      *pEncryptedKernels,
        std::vector<BYTE>&  transcryptedKernels);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sends a FW SET_PAVP_SLOT command. This command is not needed for GSC
    /// \param      uiStreamId [in] The stream id (slot id) to use
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT SetSlot(uint32_t uiStreamId);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Save key blob information.
    /// \param      pStoreKeyBlobParams [in] key blob parameters
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT SaveKeyBlobForPAVPINIT43(
        void *pStoreKeyBlobParams);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sends a FW INVALIDATE_PAVP_SLOT command if you want to free the slot.
    ///             This command frees all FW resources associated with a slot.
    ///             Once a slot is invalidated, it is set by the next PAVP session. 
    ///             This command is no needed for GSC
    /// \param      uiStreamId [in] The stream id (slot id) to use
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT InvalidateSlot(uint32_t uiStreamId);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sends a FW INV_STREAM_KEY command to GSC.
    ///             This command cleans HDCP requirement associated with a stream slot.
    /// \param      uiStreamId [in] The stream id (slot id) to use
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT InvalidateStreamKey(uint32_t uiStreamId);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sends a FW GET_PLANE_ENABLE command and gets the response.
    ///             This command is not needed for GSC
    /// \param      uiMemoryMode        [in] Input memory mode
    /// \param      uiPlaneEnables      [in] Input plane enables
    /// \param      uiPlanesEnableSize  [in] Maximum size in bytes of the pbEncPlaneEnables parameter
    /// \param      pbEncPlaneEnables   [out] Encoded plane enables info as returned
    /// \returns    S_OK on success, failed HRESULT otherwise.  
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT GetPlaneEnables(
        uint8_t   uiMemoryMode,
        uint16_t  uiPlaneEnables,
        uint32_t  uiPlanesEnableSize,
        PBYTE     pbEncPlaneEnables);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sends a FW VERIFY_CONN_STATUS command.
    ///             This command is not needed for GSC
    /// \param      uiCsSize            [in] Size of the pbConnectionStatus parameter
    /// \param      pbConnectionStatus  [out] Pointer to memory which will contain the connection status
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT VerifyConnStatus(
        uint8_t uiCsSize,
        PVOID   pbConnectionStatus);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sends a FW GET_DISPLAY_NONCE command.
    ///             This command is not needed for GSC
    /// \param      bResetStreamKey [in] TRUE for resetting the stream key, false otherwise
    /// \param      uEncE0Size      [in] Size of the puiEncryptedE0 parameter
    /// \param      puiEncryptedE0  [out] Pointer to memory which will contain the encrypted E0 key
    /// \param      puiNonce        [out] Pointer to an INT32 which will contain the nonce
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT GetNonce(
        bool     bResetStreamKey,
        uint8_t  uEncE0Size,
        uint8_t  *puiEncryptedE0,
        uint32_t *puiNonce);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sends a CHECK_PAVP_MODE command and checks the memory status.
    ///             This command is not needed for GSC
    /// \param      uPMemStatusSize [in] Size of the puPMemStatus parameter
    /// \param      puPMemStatus    [out] Pointer to memory which will contain the status array
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT VerifyMemStatus(
        uint8_t uPMemStatusSize,
        PVOID   puPMemStatus);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Performs a pass-through to the GSC FW using the supplied in/out buffers.
    /// \param      pInput          [in] Input buffer
    /// \param      ulInSize        [in] Size of input buffer
    /// \param      pOutput         [out] Output buffer
    /// \param      ulOutSize       [in] Size of output buffer
    /// \param      puiIgdData      [out] IGD data buffer (can be NULL if the FW command does not use IGD data).
    /// \param      puiIgdDataLen   [out] Pointer to uint32_t which will contain the size of the resulted IGD data buffer
    ///                               (can be NULL if the FW command does not use IGD data).
    /// \param      bIsWidiMsg      [in] Indicates whether to use the WiDi session handle for sending this command.
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT PassThrough(
        PBYTE       pInput,
        ULONG       ulInSize,
        PBYTE       pOutput,
        ULONG       ulOutSize,
        uint8_t     *puiIgdData,
        uint32_t    *puiIgdDataLen,
        bool        bIsWidiMsg = false,
        uint8_t     vTag = 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sets the FW Fallback. It signals to fallback to FW v30 if v32 is detected on 
    ///             Windows. Its always FALSE for Android
    ///             This function is not needed for GSC
    /// \param      bFwFallback [in] Input bool
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT SetFwFallback(bool bFwFallback);

    // HSW Android WideVine.
    // HSW Play Ready
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sends a FW GET_WV_HEART_BEAT_DISPLAY_NONCE command.
    ///             This command is no needed for GSC
    /// \param      puiNonce    [out] Pointer to an INT32 which will contain the nonce.
    /// \param      uiStreamId  [in]  Pavp appId/slotId being used for the call.
    /// \param      eDrmType    [in]  Indicates type of DRM used - FW API version, command-ids, Msg strcuts 
    ///                               depends on this
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT GetHeartBeatStatusNonce(
        uint32_t       *puiNonce,
        uint32_t       uiStreamId = 0,
        PAVP_DRM_TYPE  eDrmType = PAVP_DRM_TYPE_NONE);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sends a FW VERIFY_WV_HEART_BEAT_CONN_STATUS command
    ///             This command is no needed for GSC
    /// \param      uiCsSize            [in]  The size of pbConnectionStatus
    /// \param      pbConnectionStatus  [in]  Pointer to memory containing the connection status
    /// \param      uiStreamId          [in]  App ID to verify HDCP status
    /// \param      eDrmType            [in]  Indicates type of DRM used - FW API version, command-ids, Msg strcuts 
    ///                                       depends on this
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT VerifyHeartBeatConnStatus(
        uint32_t        uiCsSize,
        PVOID           pbConnectionStatus,
        uint32_t        uiStreamId = 0,
        PAVP_DRM_TYPE   eDrmType = PAVP_DRM_TYPE_NONE);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      IsKernelAuthenticationSupported.
    /// \par        Note: This function is to test if Authenticated Kernel is supported.
    /// \li
    ///
    /// \returns    TRUE if Authenticated Kernel is supported
    ///             FALSE other wise
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual bool IsKernelAuthenticationSupported();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      HasFwCaps.
    /// \par        Note: This function is to test if requested capablity is supported by firmware.
    ///
    /// \returns    TRUE if current firmware has the requested cap
    ///             FALSE other wise
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual bool HasFwCaps(FIRMWARE_CAPS eFwCaps);

    CPavpGscHal(RootTrust_HAL_USE gscHalUse = RootTrust_HAL_PAVP_USE,
                std::shared_ptr<CPavpOsServices> pOsServices = nullptr,
                bool bEPIDProvisioningMode = true);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Gets the FW PAVP API version.
    /// \returns    The FW API version
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual uint32_t FWApiVersion();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Determines whether the GSC HAL has been initialized.
    /// \returns    TRUE if initialized, false otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual BOOL IsInitialized();

    //default copy constructor are not allowed
    CPavpGscHal(const CPavpGscHal&) = delete;
    CPavpGscHal& operator=(const CPavpGscHal&) = delete;

    //default constructor is not allowed
    CPavpGscHal() = delete;

    ~CPavpGscHal();

protected:
    uint32_t                            m_uiFWApiVersion;       // Indicates supported FW version
    BOOL                                m_Initialized;          // Indicates if CPavpGscHal is initializes
    std::bitset<32>                     m_eFwCaps;              // stores FW capablities
    bool                                m_EPIDProvisioningMode; // Indicates whether provision of certificates needed (currently done in windows only)
    BOOL                                m_AuthenticatedKernelTrans;  //Gen12+ stout not support.
    std::shared_ptr<CPavpOsServices>    m_pOsServices;          // Shared pointer for CP OS services
    uint32_t                            m_uiStreamId;           // Stream ID
    RootTrust_HAL_USE                   m_GscHalUsage;          // to indicate HECI client
    PAVPStoreKeyBlobInputBuff           m_KeyBlobForINIT43;
    bool                                m_bHucAuthCheckReq;     // Indicates if huc auhtentication check is required for PXP operations

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Build and submit GSC Command Buffer
    /// \param      pInputBuffer           [in] Input buffer
    /// \param      InputBufferLength      [in] Size of input buffer
    /// \param      pOutputBuffer          [out] Output buffer
    /// \param      OutputBufferLength     [in] Size of output buffer
    /// \param      pIgdData               [out] IGD data buffer (can be NULL if the FW command does not use IGD data).
    /// \param      pIgdDataLen            [out] Pointer to uint32_t which will contain the size of the resulted IGD data buffer
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT BuildSubmitGSCCommandBuffer(
        uint8_t     *pInputBuffer,
        uint32_t    InputBufferLength,
        uint8_t     *pOutputBuffer,
        uint32_t    OutputBufferLength,
        uint8_t     *pIgdData,
        uint32_t    *pIgdDataLen,
        uint8_t     vTag);

    /// \brief      Get CP binary reource Function
    /// \param      SendCertIn           [in]Certificate structure that will have the
    ///                                      ceritifcate to be sent to FW
    /// \param      uiGroupId            [in]Group ID
    /// \param      stEpidBinaryPath     [in]Input Path String to binary file
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT GetEpidResourceData(SendEpidPubKeyInBuff &SendCertIn, uint32_t uiGroupId, std::string &stEpidBinaryPath);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Gets the FW API version from GSC.
    /// \par        Details:
    /// \li         This is done by sending a FIRMWARE_CMD_GET_GSC_CAPS command to GSC.
    /// \param      puiVersion  [out] Pointer to memory which will contain the FW API version
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT GetFWApiVersionFromGSC(uint32_t *puiVersion);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sets all member variables to a default value.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual void ResetMemberVariables();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Performs I/O Messaging to the GSC FW using the supplied in/out buffers.
    /// \param      pInputBuffer           [in] Input buffer
    /// \param      InputBufferLength      [in] Size of input buffer
    /// \param      pOutputBuffer          [out] Output buffer
    /// \param      OutputBufferLength     [in] Size of output buffer
    /// \param      pIgdData               [out] IGD data buffer (can be NULL if the FW command does not use IGD data).
    /// \param      pIgdDataLen            [out] Pointer to uint32_t which will contain the size of the resulted IGD data buffer
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT IoMessage(
        uint8_t     *pInputBuffer,
        uint32_t    InputBufferLength,
        uint8_t     *pOutputBuffer,
        uint32_t    OutputBufferLength,
        uint8_t     *pIgdData,
        uint32_t    *pIgdDataLen,
        uint8_t     vTag = 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Call and verify firmware command to authenticate and transcrypt encrypted kernels.
    /// 
    /// \param      uiFwCommand     [in] Firmware command ID.
    /// \param      pInfo           [in] Info such as firmware API version to use.
    /// \param      sHeciIn         [in] Input HECI call structure.
    /// \param      uiHeciInSize    [in] Input HECI call structure size in bytes.
    /// \param      sHeciOut        [in] Output HECI call structure.
    /// \param      uiHeciOutSize   [in] Output HECI call structure size in bytes.
    /// 
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT VerifyCallForTranscryptedKernels(uint32_t uiFwCommand, const AuthKernelMetaDataInfo* pInfo,
        uint8_t *pHeciIn, uint32_t uiHeciInSize, uint8_t *pHeciOut, uint32_t uiHeciOutSize);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Write head marker to output buffer
    /// \param      gscOutputBuffer  [in] output buffer resource
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT WriteOutputBufferHeadMarker(MOS_RESOURCE gscOutputBuffer);

    MEDIA_CLASS_DEFINE_END(CPavpGscHal)
};
#endif // __CP_HAL_GSC_H_
