/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2012-2018
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

File Name: cp_hal_pch_unified.h

Abstract:
    Defines a class to supply access to the PCH.

Environment:
    Windows, Linux, Android

Notes:
    None

======================= end_copyright_notice ==================================*/

#ifndef __CP_HAL_PCH_UNIFED_H__
#define __CP_HAL_PCH_UNIFED_H__

#include <string>
#include <vector>
#include <bitset>
#include "cp_umd_defs.h"
#include "cp_debug.h"
#include "intel_pavp_epid_api.h"
#include "intel_pavp_core_api.h"
#include "cp_hal_root_trust.h"
#ifdef WIN32
#include "cp_heci_dma_win.h"
#endif
#include "cp_heci_session_interface.h"

#define PCH_HAL_FUNCTION_ENTER    CP_FUNCTION_ENTER(MOS_CP_SUBCOMP_PCH_HAL)
#define PCH_HAL_FUNCTION_EXIT(rc) CP_FUNCTION_EXIT(MOS_CP_SUBCOMP_PCH_HAL, rc)

#define PCH_HAL_ASSERTMESSAGE(_message, ...) \
    CP_ASSERTMESSAGE(MOS_CP_SUBCOMP_PCH_HAL, _message, ##__VA_ARGS__)

#define PCH_HAL_VERBOSEMESSAGE(_message, ...) \
    CP_VERBOSEMESSAGE(MOS_CP_SUBCOMP_PCH_HAL, _message, ##__VA_ARGS__)

#define PCH_HAL_NORMALMESSAGE(_message, ...) \
    CP_NORMALMESSAGE(MOS_CP_SUBCOMP_PCH_HAL, _message, ##__VA_ARGS__)

#define PCH_HAL_ASSERT(_expr) CP_ASSERT(MOS_CP_SUBCOMP_PCH_HAL, _expr)

class CPavpOsServices;

class CPavpPchHalUnified : public CPavpRootTrustHal
{

public:

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sends a FW SET_PAVP_SLOT command to the PCH.
    /// \param      uiStreamId [in] The stream id (slot id) to use
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT SetSlot(
        uint32_t uiStreamId);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Save key blob information.
    /// \param      pStoreKeyBlobParams [in] key blob parameters
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT SaveKeyBlobForPAVPINIT43(
        void *pStoreKeyBlobParams);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Enable huc streamout bit for lite mode for debug purpose.
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT EnableLiteModeHucStreamOutBit();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sends a FW PAVP init command to the PCH.
    /// \param      uiPavpMode [in] Pavp Mode (Lite, Heavy or Isolated Decode)
    /// \param      uiAppId [in] the App id (slot id) to use
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT PavpInit4(
        PAVP_FW_PAVP_MODE uiPavpMode,    
        uint32_t uiAppId);
    
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sends a FW INVALIDATE_PAVP_SLOT command to the PCH if you want to free the slot.
    ///             This command frees all FW resources associated with a slot.
    ///             Once a slot is invalidated, it is set by the next PAVP session.
    /// \param      uiStreamId [in] The stream id (slot id) to use
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT InvalidateSlot(uint32_t uiStreamId);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sends a FW INV_STREAM_KEY command to the PCH if you want to release the stream slot.
    /// \param      uiStreamId [in] The stream id (slot id) to use
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT InvalidateStreamKey(uint32_t uiStreamId);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sends a FW GET_PLANE_ENABLE command to the PCH , and gets the response.
    /// \param      uiMemoryMode        [in] Input memory mode
    /// \param      uiPlaneEnables      [in] Input plane enables
    /// \param      uiPlanesEnableSize  [in] Maximum size in bytes of the pbEncPlaneEnables parameter
    /// \param      pbEncPlaneEnables   [out] Encoded plane enables info as returned from the PCH
    /// \returns    S_OK on success, failed HRESULT otherwise.  
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT GetPlaneEnables(
        uint8_t     uiMemoryMode,
        uint16_t    uiPlaneEnables, 
        uint32_t    uiPlanesEnableSize,
        PBYTE       pbEncPlaneEnables);
    
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sends a FW VERIFY_CONN_STATUS command to the PCH.
    /// \param      uiCsSize            [in] Size of the pbConnectionStatus parameter
    /// \param      pbConnectionStatus  [out] Pointer to memory which will contain the connection status
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT VerifyConnStatus(
        uint8_t uiCsSize,
        PVOID pbConnectionStatus);
    
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sends a FW GET_DISPLAY_NONCE command to the PCH.
    /// \param      bResetStreamKey [in] TRUE for resetting the stream key, false otherwise
    /// \param      uEncE0Size      [in] Size of the puiEncryptedE0 parameter
    /// \param      puiEncryptedE0  [out] Pointer to memory which will contain the encrypted E0 key
    /// \param      puiNonce        [out] Pointer to an INT32 which will contain the nonce
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT GetNonce(
        bool        bResetStreamKey,
        uint8_t     uEncE0Size,
        uint8_t     *puiEncryptedE0,
        uint32_t    *puiNonce);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Sends a Daisy Chain command to the FW to get the Key translated by the FW. 
    /// \par         Details:
    /// \li          Helper function to send a single daisy chain command to the CSME FW and check the return codes. 
    /// 
    /// \param       [in] Key to translate
    /// \param       [in] boolean to indicate whether the key to translate is a decrypt key
    /// \param       [out] Stream Key to hold the returned translated key from the FW.
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT SendDaisyChainCmd(StreamKey pKeyToTranslate, bool IsDKey, StreamKey pTranslatedKey);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sends a CHECK_PAVP_MODE command to PCH FW, and checks the memory status.
    /// \param      uPMemStatusSize [in] Size of the puPMemStatus parameter
    /// \param      puPMemStatus    [out] Pointer to memory which will contain the status array
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT VerifyMemStatus(
        uint8_t uPMemStatusSize,
        void    *puPMemStatus);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Performs a pass-through to the PCH FW using the supplied in/out buffers.
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
    HRESULT PassThrough(
        PBYTE       pInput, 
        ULONG       ulInSize,
        PBYTE       pOutput, 
        ULONG       ulOutSize,
        uint8_t     *puiIgdData, 
        uint32_t    *puiIgdDataLen,
        bool        bIsWidiMsg = false,
        uint8_t     vTag = 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Gets the FW API version.
    /// \returns    The FW API version (for example, 0x00010005 for PAVP 1.5).
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    uint32_t FWApiVersion();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Determines whether the PCH HAL has been initialized.
    /// \returns    TRUE if initialized, false otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsInitialized();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Initializes the PCH HAL.
    /// \param      phPavpDll [in, optional] Input handle to PAVP DLL
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT Init();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sets the FW Fallback in PCH HAL. It signals to fallback to FW v30 if v32 is detected on 
    ///             Windows. Its always FALSE for Android
    /// \param      bFwFallback [in] Input bool
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT SetFwFallback(bool bFwFallback);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Cleans up the PCH HAL.
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT Cleanup();

    CPavpPchHalUnified(RootTrust_HAL_USE pchHalUse = RootTrust_HAL_PAVP_USE,
                       std::shared_ptr<CPavpOsServices> pOsServices = nullptr,
                       bool bEPIDProvisioningMode = true);

    CPavpPchHalUnified(const CPavpPchHalUnified&) = delete;
    CPavpPchHalUnified& operator=(const CPavpPchHalUnified&) = delete;
     ~CPavpPchHalUnified();

    // HSW Android WideVine.
    // HSW Play Ready
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sends a FW GET_WV_HEART_BEAT_DISPLAY_NONCE command to the PCH.
    /// \param      puiNonce    [out] Pointer to an INT32 which will contain the nonce.
    /// \param      uiStreamId  [in]  Pavp appId/slotId being used for the call.
    /// \param      eDrmType    [in]  Indicates type of DRM used - FW API version, command-ids, Msg strcuts 
    ///                               depends on this
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT GetHeartBeatStatusNonce(
        uint32_t        *puiNonce,
        uint32_t        uiStreamId = 0,
        PAVP_DRM_TYPE   eDrmType = PAVP_DRM_TYPE_NONE);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Sends a FW VERIFY_WV_HEART_BEAT_CONN_STATUS command to the PCH
    /// \param      uiCsSize            [in]  The size of pbConnectionStatus
    /// \param      pbConnectionStatus  [in]  Pointer to memory containing the connection status
    /// \param      uiStreamId          [in]  App ID to verify HDCP status
    /// \param      eDrmType            [in]  Indicates type of DRM used - FW API version, command-ids, Msg strcuts 
    ///                                       depends on this
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT VerifyHeartBeatConnStatus(
        uint32_t        uiCsSize,
        PVOID           pbConnectionStatus,
        uint32_t        uiStreamId = 0,
        PAVP_DRM_TYPE   eDrmType = PAVP_DRM_TYPE_NONE);
    
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Performs I/O Messaging to the PCH FW using the supplied in/out buffers.  
    /// \param      pInputBuffer           [in] Input buffer
    /// \param      InputBufferLength      [in] Size of input buffer
    /// \param      pOutputBuffer          [out] Output buffer
    /// \param      OutputBufferLength     [in] Size of output buffer
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT IoMessage(
        uint8_t     *pInputBuffer,
        uint32_t    InputBufferLength,
        uint8_t     *pOutputBuffer,
        uint32_t    OutputBufferLength,
        uint8_t     *pIgdData, 
        uint32_t *   IgdDataLen,
        BOOL        bIsWidiMsg = FALSE);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Init the Heci Session
    /// \param      pCpMsgClass     [in] Type of Session to be creates
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT SessionInit(const GUID* pCpMsgClass);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Close/Clean the Heci Session
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT SessionCleanup();    

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Perform EPID Provisioning Function
    /// \param      stEpidBinaryPath     [in]Input Path String to binary file
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT PerformProvisioning(std::string &stEpidResourcePath);
    
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Get CP binary reource Function
    /// \param      SendCertIn           [in]Certificate structure that will have the
    ///                                      ceritifcate to be sent to FW
    /// \param      uiGroupId            [in]Group ID
    /// \param      stEpidBinaryPath     [in]Input Path String to binary file
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT GetEpidResourceData(SendEpidPubKeyInBuff &SendCertIn, uint32_t uiGroupId, std::string &stEpidBinaryPath);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Check CP binary reource Function
    /// \param      stEpidBinaryPath     [in]Input Path String to binary file
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT VerifyEpidResourceData(std::string &stEpidBinaryPath);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      GetPlayReadyVersion.
    /// \par        Note: This function will return an error if PlayReady support
    /// \li         is not enabled in the BIOS for systems which require it.
    ///
    /// \param      version     [out] PlayReady pk version number
    ///
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT GetPlayReadyVersion(uint32_t &version);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Test of the FW version we are using supports a feature from version 3.0
    /// \par        Details:
    /// \li         Looks for minimum version 3.0 or latter.
    /// \li         Used to create a single point of maintenance when bumping versions. 
    /// 
    /// \returns    TRUE if supported, FALSE otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL FwSupports3();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Test of the FW version we are using supports a feature from version 3.2
    /// \par        Details:
    /// \li         Looks for minimum version 3.2 or latter.
    /// \li         Used to create a single point of maintenance when bumping versions.
    /// 
    /// \returns    TRUE if supported, FALSE otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL FwSupports32();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Test of the FW version we are using supports a feature from version 4
    /// \par        Details:
    /// \li         Looks for minimum version 3.2 or latter.
    /// \li         Used to create a single point of maintenance when bumping versions.
    /// 
    /// \returns    TRUE if supported, FALSE otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL FwSupports4();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      IsKernelAuthenticationSupported.
    /// \par        Note: This function is to test if Authenticated Kernel is supported.
    /// \li
    ///
    /// \returns    TRUE if Authenticated Kernel is supported
    ///             FALSE other wise
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool IsKernelAuthenticationSupported();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      HasFwCaps.
    /// \par        Note: This function is to test if requested capablity is supported by firmware.
    /// \li         
    ///
    /// \returns    TRUE if current firmware has the requested cap
    ///             FALSE other wise
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool HasFwCaps(FIRMWARE_CAPS eFwCaps);

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
    HRESULT TranscryptKernels(
        uint32_t             uiTranscryptMetaDataSize,
        const uint32_t      *pTranscryptMetaData,
        uint32_t             uiKernelsSize,
        const uint32_t      *pEncryptedKernels,
        std::vector<BYTE>   &transcryptedKernels);

private:
    RootTrust_HAL_USE       m_ePchHalUse;
    uint32_t                m_uiFWApiVersion;
    BOOL                    m_Initialized;
    BOOL                    m_bFwFallback;
    uint32_t                m_uiStreamId;
    CHeciSessionInterface*  m_hPavpSession;
    CHeciSessionInterface*  m_hWiDiSession;
    // Variable set whether provision of certificates needed (currently done in windows only)
    bool                    m_EPIDProvisioningMode;
    BOOL                    m_IsWidiEnabled;
    BOOL                    m_AuthenticatedKernelTrans;  //all platform with PCH on master branch should be true
    std::bitset<32>         m_eFwCaps;
    uint32_t                m_SuspendResumeInitRetry;
    std::shared_ptr<CPavpOsServices> m_pOsServices;
    
    PAVPStoreKeyBlobInputBuff        m_KeyBlobForINIT43;

    
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sends a FW command to PCH.
    /// \par        Details:
    /// \li         The function populates the FW header.
    /// \li         The function cannot be used for commands require the use of IGD data.
    /// \param      pIn         [in] Input buffer
    /// \param      inSize      [in] Size of input buffer
    /// \param      pOut        [out] Output buffer
    /// \param      outSize     [in] Size of output buffer
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT SendRecvCmd(
        PVOID  pIn, 
        ULONG  inSize, 
        PVOID  pOut,
        ULONG  outSize);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Gets the FW API version from PCH.
    /// \par        Details:
    /// \li         This is done by sending a FIRMWARE_CMD_GET_PCH_CAPS command to PCH.
    /// \param      puiVersion  [out] Pointer to memory which will contain the FW API version
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT GetFWApiVersionFromPCH(uint32_t *puiVersion);
    
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sets all member variables to a default value.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    void ResetMemberVariables();

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
    template<typename HeciIn, typename HeciOut>
    HRESULT VerifyCallForTranscryptedKernels(uint32_t uiFwCommand, const AuthKernelMetaDataInfo* pInfo, 
        HeciIn& sHeciIn, uint32_t uiHeciInSize, HeciOut& sHeciOut, uint32_t uiHeciOutSize);

    MEDIA_CLASS_DEFINE_END(CPavpPchHalUnified)
};

#endif // __CP_HAL_PCH_UNIFED_H__
