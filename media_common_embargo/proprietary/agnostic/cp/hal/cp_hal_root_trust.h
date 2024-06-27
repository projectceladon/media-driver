/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2012-2023
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

File Name: cp_hal_root_trust.h

Abstract:
Defines a interface class which is implemented by PCH and GSC classes.

Environment:
Windows, Linux, Android

Notes:
None

======================= end_copyright_notice ==================================*/

#ifndef __CP_HAL_ROOT_TRUST_H__
#define __CP_HAL_ROOT_TRUST_H__

#include <string>
#include <vector>
#include <memory>
#include "intel_pavp_error.h"
#include "intel_pavp_epid_api.h"
#include "cp_factory.h"

// These numbers of attempts suggested by ME Team (up to 15 sec)
#define     NUM_EPID_ATTEMPTS       50
#define     SLEEP_300MS_PER_ATTEMPT 300
#define     WIDI_COMMAND_ID_MASK    0x00000003
#define     IS_CMD_WIDI(CommandId)  ((CommandId >> 16) == WIDI_COMMAND_ID_MASK) && \
                                    (CommandId != CMD_MIRACAST_RX_GENERAL) && \
                                    (CommandId != MIRACAST_RX_GET_R_IV)
#define FILE_EPID_CERTIFICATES_BINARY   "cp_resources.bin"
// MAX_NUM_OF_EPID_CERTIFICATES Defines the maxiumim number fo ceritficates. 
// 2014.7: for each group (i.e WPT, LPT...), there is a maximum of 400 certificates,To be on the safe side 3 groups are supported with max number of ceritifcates giving the number 3x400 = 1200
// 2015.5: Up to SKL-H the number of certificates in cp_resources.bin are 1302
// 2015.11: Adding BXT EPID Certs (125 certs) to cp_resources.bin, Total 1302 + 125 = 1427.
// 2016.11: Adding GLK EPID Certs (100 certs) to cp_resources.bin, Total 1427 + 100 = 1527.
// 2016.12: Adding CNL EPID Certs (401 certs) to cp_resources.bin, Total 1527 + 401 = 1928
// 2017.10: Adding CNL EPID Certs (1 certs) to cp_resources.bin, Total 1928 + 1 = 1929.
// 2017.11: Adding CNL EPID Certs (1 certs) to cp_resources.bin, Total 1929 + 1 = 1930.
// 2018.2: Adding CNL-H B0 EPID Certs (400 certs) to cp_resources.bin, Total 1930 + 400 = 2330.
// 2018.5: Adding LKF EPID Certs (402 certs) to cp_resources.bin, Total 2330 + 402 = 2732.
// 2019.3: Adding CML EPID Certs (402 certs) to cp_resources.bin, Total 2732 + 402 = 3134. 
// 2019.3: Adding ICL and ICL_N EPID Certs (414 certs) to cp_resources.bin, Total 3134 + 414 = 3548.
// 2019.5: Adding KBL EPID Certs (200 certs) to cp_resources.bin, Total 3548 + 200 = 3748.
// 2019.8: Adding LKF B0 EPID Certs (402 certs) to cp_resources.bin, Total 3748 + 402 = 4150.
// 2019.10: Adding TGL A3 EPID Certs (403 certs) to cp_resources.bin, Total 4150 + 403 = 4553
// 2019.10: Adding KBL EPID Certs for SVN3(group ID 8066 - 8465, 400 certs) to cp_resources.bin, Total 4553 + 400 = 4953
// 2019.10: Adding DG1 EPID Cert (1 cert) to cp_resources.bin, Total 4953 + 1 = 4954.
// 2019.12: Adding EHL EPID Certs (402 certs) to cp_resources.bin, Total 4954 + 402 = 5356.
// 2020.06: Adding DG1 B step EPID Certs (402 certs) to cp_resources.bin, Total 5356 + 402 = 5758.
// 2020.07: Adding JSL A1 EPID Certs (402 certs) to cp_resources.bin, Total 5758 + 402 = 6160.
// 2021.08: Adding GLK EPID Certs (400 certs) to cp_resources.bin, Total 6160 + 400 = 6560.
#define MAX_NUM_OF_EPID_CERTIFICATES    6560

class CPavpOsServices;
class MhwInterfaces;

// firmware capability, 32bit width, used as bit position in roottrust hal
enum FIRMWARE_CAPS
{
    FIRMWARE_CAPS_MULTI_SESSION     = 0,        // has multi pavp session support, start from fw version 3.2
    FIRMWARE_CAPS_PAVP_INIT         = 1,        // has pavp init command support, fw do the connection checking instead of cp driver.
                                                // start from fw version 4.0
    FIRMWARE_CAPS_LSPCON            = 2,        // has lspcon command support, will be removed in gen10?
    FIRMWARE_CAPS_PAVP_UNIFIED_INIT = 3,        // FW takes AppId in the input command header which allows unified init flows for all DRMs
    FIRMWARE_CAPS_ONDIECA           = 4,        // has OnDieCa support start from fw version 4.2
    FIRMWARE_CAPS_PAVP_INIT43       = 5,        // FW support pavp_init43 command
    FIRMWARE_CAPS_MAX               = 31,       // max firmware capability.
};

class CPavpRootTrustHal
{
public:

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \enum   Indicates whether the HAL has been created for PAVP or WiDi use.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    typedef enum
    {
        RootTrust_HAL_NONE,
        RootTrust_HAL_PAVP_USE,
        RootTrust_HAL_WIDI_USE
#if (_DEBUG || _RELEASE_INTERNAL)
        ,RootTrust_HAL_WIDI_CLIENT_FW_APP_USE
        ,RootTrust_HAL_PAVP_CLIENT_FW_APP_USE
        ,RootTrust_HAL_MKHI_CLIENT_FW_APP_USE
        ,RootTrust_HAL_GSC_PROXY_CLIENT_FW_APP_USE
#endif
    } RootTrust_HAL_USE;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sends a FW SET_PAVP_SLOT command to the RootTrust.
    /// \param      uiStreamId [in] The stream id (slot id) to use
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT SetSlot(
        uint32_t uiStreamId) = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Save key blob information to RootTrust.
    /// \param      pStoreKeyBlobParams [in] key blob parameters
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT SaveKeyBlobForPAVPINIT43(
        void *pStoreKeyBlobParams) = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Enable huc streamout bit for lite mode for debug purpose.
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT EnableLiteModeHucStreamOutBit() = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sends a FW PAVP init command to the RootTrust.
    /// \param      uiPavpMode [in] Pavp Mode (Lite, Heavy or Isolated Decode)
    /// \param      uiAppId [in] the App id (slot id) to use
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT PavpInit4(
        PAVP_FW_PAVP_MODE uiPavpMode,
        uint32_t uiAppId) = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sends a FW INVALIDATE_PAVP_SLOT command to the Root of Trust. if you want to free the slot.
    ///             This command frees all FW resources associated with a slot.
    ///             Once a slot is invalidated, it is set by the next PAVP session.
    /// \param      uiStreamId [in] The stream id (slot id) to use
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT InvalidateSlot(uint32_t uiStreamId) = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sends a FW INV_STREAM_KEY command to the Root of Trust.
    ///             This command cleans HDCP requirement associated with a stream slot.
    /// \param      uiStreamId [in] The stream id (slot id) to use
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT InvalidateStreamKey(uint32_t uiStreamId) = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sends a FW GET_PLANE_ENABLE command to the Root of Trust , and gets the response.
    /// \param      uiMemoryMode        [in] Input memory mode
    /// \param      uiPlaneEnables      [in] Input plane enables
    /// \param      uiPlanesEnableSize  [in] Maximum size in bytes of the pbEncPlaneEnables parameter
    /// \param      pbEncPlaneEnables   [out] Encoded plane enables info as returned from the Root of Trust
    /// \returns    S_OK on success, failed HRESULT otherwise.  
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT GetPlaneEnables(
        uint8_t     uiMemoryMode,
        uint16_t    uiPlaneEnables,
        uint32_t    uiPlanesEnableSize,
        PBYTE       pbEncPlaneEnables) = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sends a FW VERIFY_CONN_STATUS command to the Root of Trust.
    /// \param      uiCsSize            [in] Size of the pbConnectionStatus parameter
    /// \param      pbConnectionStatus  [out] Pointer to memory which will contain the connection status
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT VerifyConnStatus(
        uint8_t uiCsSize,
        PVOID   pbConnectionStatus) = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sends a FW GET_DISPLAY_NONCE command to the Root of Trust.
    /// \param      bResetStreamKey [in] true for resetting the stream key, false otherwise
    /// \param      uEncE0Size      [in] Size of the puiEncryptedE0 parameter
    /// \param      puiEncryptedE0  [out] Pointer to memory which will contain the encrypted E0 key
    /// \param      puiNonce        [out] Pointer to an INT32 which will contain the nonce
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT GetNonce(
        bool     bResetStreamKey,
        uint8_t  uEncE0Size,
        uint8_t  *puiEncryptedE0,
        uint32_t *puiNonce) = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Sends a Daisy Chain command to the FW to get the Key translated by the FW. 
    /// \par         Details:
    /// \li          Helper function to send a single daisy chain command to the root of trust FW and check the return codes. 
    /// 
    /// \param       [in] Key to translate
    /// \param       [in] boolean to indicate whether the key to translate is a decrypt key
    /// \param       [out] Stream Key to hold the returned translated key from the FW.
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT SendDaisyChainCmd(StreamKey pKeyToTranslate, bool IsDKey, StreamKey pTranslatedKey) = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sends a CHECK_PAVP_MODE command to Root of Trust FW, and checks the memory status.
    /// \param      uPMemStatusSize [in] Size of the puPMemStatus parameter
    /// \param      puPMemStatus    [out] Pointer to memory which will contain the status array
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT VerifyMemStatus(
        uint8_t uPMemStatusSize,
        PVOID   puPMemStatus) = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Performs a pass-through to the Root of Trust FW using the supplied in/out buffers.
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
        PBYTE    pInput,
        ULONG    ulInSize,
        PBYTE    pOutput,
        ULONG    ulOutSize,
        uint8_t  *puiIgdData,
        uint32_t *puiIgdDataLen,
        bool     bIsWidiMsg = false,
        uint8_t  vTag = 0) = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Initializes the RootTrust HAL.
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT Init() = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sets the FW Fallback in RootTrust HAL. It signals to fallback to FW v30 if v32 is detected on 
    ///             Windows. Its always FALSE for Android
    /// \param      bFwFallback [in] Input bool
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT SetFwFallback(bool bFwFallback) = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Cleans up the RootTrust HAL.
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT Cleanup() = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Creates either CPavpGscHal or CPavpPchHalUnified constructor.
    /// \par        Details:
    /// \param      roottrustHalUse             [in] GUID represents the session class (PAVP/WIDI) for PCH case.
    /// \param      pOsServices                 [in] Pointer to CPavpOsServices
    /// \returns    CPavpRootTrustHal*
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CPavpRootTrustHal* RootHal_Factory(
        RootTrust_HAL_USE roottrustHalUse = RootTrust_HAL_PAVP_USE,
        std::shared_ptr<CPavpOsServices> pOsServices = nullptr);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Creates either CPavpGscHal or CPavpPchHalUnified constructor.
    /// \returns    CPavpRootTrustHal*
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CPavpRootTrustHal* RootHal_Factory_Default();

    virtual ~CPavpRootTrustHal() {};

    // HSW Android WideVine.
    // HSW Play Ready
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sends a FW GET_WV_HEART_BEAT_DISPLAY_NONCE command to the Root of Trust.
    /// \param      puiNonce    [out] Pointer to an INT32 which will contain the nonce.
    /// \param      uiStreamId  [in]  Pavp appId/slotId being used for the call.
    /// \param      eDrmType    [in]  Indicates type of DRM used - FW API version, command-ids, Msg strcuts 
    ///                               depends on this
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT GetHeartBeatStatusNonce(
        uint32_t         *puiNonce,
        uint32_t         uiStreamId = 0,
        PAVP_DRM_TYPE    eDrmType = PAVP_DRM_TYPE_NONE) = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Sends a FW VERIFY_WV_HEART_BEAT_CONN_STATUS command to the Root of Trust.
    /// \param      uiCsSize            [in]  The size of pbConnectionStatus
    /// \param      pbConnectionStatus  [in]  Pointer to memory containing the connection status
    /// \param      uiStreamId          [in]  App ID to verify HDCP status
    /// \param      eDrmType            [in]  Indicates type of DRM used - FW API version, command-ids, Msg strcuts 
    ///                                       depends on this
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT VerifyHeartBeatConnStatus(
        uint32_t          uiCsSize,
        PVOID           pbConnectionStatus,
        uint32_t            uiStreamId = 0,
        PAVP_DRM_TYPE   eDrmType = PAVP_DRM_TYPE_NONE) = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Perform EPID Provisioning Function
    /// \param      stEpidBinaryPath     [in]Input Path String to binary file
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT PerformProvisioning(std::string &stEpidResourcePath) = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      GetPlayReadyVersion.
    /// \par        Note: This function will return an error if PlayReady support
    /// \li         is not enabled in the BIOS for systems which require it.
    ///
    /// \param      version     [out] PlayReady pk version number
    ///
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT GetPlayReadyVersion(uint32_t &version) = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      IsKernelAuthenticationSupported.
    /// \par        Note: This function is to test if Authenticated Kernel is supported.
    /// \li
    ///
    /// \returns    TRUE if Authenticated Kernel is supported
    ///             FALSE other wise
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual bool IsKernelAuthenticationSupported() = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      HasFwCaps.
    /// \par        Note: This function is to test if requested capablity is supported by firmware.
    /// \li
    ///
    /// \returns    TRUE if current firmware has the requested cap
    ///             FALSE other wise
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual bool HasFwCaps(FIRMWARE_CAPS eFwCaps) = 0;

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
        uint32_t              uiTranscryptMetaDataSize,
        const uint32_t*       pTranscryptMetaData,
        uint32_t              uiKernelsSize,
        const uint32_t*       pEncryptedKernels,
        std::vector<BYTE>&  transcryptedKernels) = 0;

    PavpInit43OutBuff   m_ResponseForINIT43;       //!< PAVP_INIT43 output buffer from FW

    MEDIA_CLASS_DEFINE_END(CPavpRootTrustHal)
};

static const char * PavpStatusToString(PAVP_STATUS Status)
{
#if defined(_RELEASE_INTERNAL) || defined (_DEBUG)
    switch (Status)
    {
    case PAVP_STATUS_SUCCESS:                           return "Success";
    case PAVP_STATUS_INTERNAL_ERROR:                    return "Internal error";
    case PAVP_STATUS_UNKNOWN_ERROR:                     return "Unknown error";
    case PAVP_STATUS_INCORRECT_API_VERSION:             return "Wrong API version";
    case PAVP_STATUS_INVALID_FUNCTION:                  return "Invalid function";
    case PAVP_STATUS_INVALID_BUFFER_LENGTH:             return "Invalid buffer length";
    case PAVP_STATUS_INVALID_PARAMS:                    return "Invalid parameters";
    case PAVP_STATUS_EPID_INVALID_PUBKEY:               return "Invalid EPID public key";
    case PAVP_STATUS_SIGMA_SESSION_LIMIT_REACHED:       return "Session limit reached";
    case PAVP_STATUS_SIGMA_SESSION_INVALID_HANDLE:      return "Invalid session handle";
    case PAVP_STATUS_SIGMA_PUBKEY_GENERATION_FAILED:    return "Public key generation failed";
    case PAVP_STATUS_SIGMA_INVALID_3PCERT_HMAC:         return "Invalid HMAC on certificate";
    case PAVP_STATUS_SIGMA_INVALID_SIG_INTEL:           return "Invalid Intel signature";
    case PAVP_STATUS_SIGMA_INVALID_SIG_CERT:            return "Invalid signature on certificate";
    case PAVP_STATUS_SIGMA_EXPIRED_3PCERT:              return "Expired certificate";
    case PAVP_STATUS_SIGMA_INVALID_SIG_GAGB:            return "Invalid signature over public keys";
    case PAVP_STATUS_PAVP_EPID_INVALID_PATH_ID:         return "Invalid path ID";
    case PAVP_STATUS_PAVP_EPID_INVALID_STREAM_ID:       return "Invalid stream ID";
    case PAVP_STATUS_PAVP_EPID_STREAM_SLOT_OWNED:       return "Stream slot already owned";
    case PAVP_STATUS_INVALID_STREAM_KEY_SIG:            return "Invalid stream key signature";
    case PAVP_STATUS_INVALID_TITLE_KEY_SIG:             return "Invalid title key signature";
    case PAVP_STATUS_INVALID_TITLE_KEY_TIME:            return "Invalid title key time";
    case PAVP_STATUS_COMMAND_NOT_AUTHORIZED:            return "Command not authorized";
    case PAVP_STATUS_INVALID_DRM_TIME:                  return "Invalid DRM time";
    case PAVP_STATUS_INVALID_TIME_SIG:                  return "Invalid time signature";
    case PAVP_STATUS_TIME_OVERFLOW:                     return "Time overflow";
    case PAVP_STATUS_INVALID_ICV:                       return "Data integrity check failed";
    default:                                            return "Unknown Error";
    }

    return "PAVP error error";
#else
    return "PAVP error error";
#endif
}

typedef CpFactory<CPavpRootTrustHal, CPavpRootTrustHal::RootTrust_HAL_USE, std::shared_ptr<CPavpOsServices>> CPavpRootTrustHalFactory;

#endif // __CP_HAL_ROOT_TRUST_UNIFED_H__
