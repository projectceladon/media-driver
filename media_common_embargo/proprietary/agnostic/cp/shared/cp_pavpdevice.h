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

File Name: cp_pavpdevice.h

Abstract:
    This class contains common code for all DXVA content protection interfaces.

Environment:
    OS Agnostic - should support - Windows Vista, Windows 7 (D3D9), 8 (D3D11.1), Linux, Android

Notes:
    None

======================= end_copyright_notice ==================================*/
#ifndef __CP_PAVPDEVICE_H__
#define __CP_PAVPDEVICE_H__

#include <tuple>
#include <memory>
#include <bitset>
#include <unordered_map>

#include "cp_user_setting_define.h"
#include "cp_drm_defs.h"
#include "pavp_types.h"
#include "mhw_cp_interface.h" // For storing Crypto Key Ex params in Encryption Param struct in SetCryptoKeyExParams()
#include "cp_umd_defs.h"
#include "cp_hal_gpu_defs.h"
#include "intel_pavp_core_api.h"
#include "cp_debug.h"
#include "cp_hal_root_trust.h"

typedef std::unordered_multimap<CP_HWDRM_IV, AudioBufferInfo> AudioBufferMap;

typedef std::unordered_multimap<CP_HWDRM_IV, RolloverMapInfo> RolloverMap;

#define PAVP_DEVICE_FUNCTION_ENTER    CP_FUNCTION_ENTER(MOS_CP_SUBCOMP_DEVICE)
#define PAVP_DEVICE_FUNCTION_EXIT(hr) CP_FUNCTION_EXIT(MOS_CP_SUBCOMP_DEVICE, hr)

#define PAVP_DEVICE_ASSERTMESSAGE(_message, ...) \
    CP_ASSERTMESSAGE(MOS_CP_SUBCOMP_DEVICE, _message, ##__VA_ARGS__)

#define PAVP_DEVICE_VERBOSEMESSAGE(_message, ...) \
    CP_VERBOSEMESSAGE(MOS_CP_SUBCOMP_DEVICE, _message, ##__VA_ARGS__)

#define PAVP_DEVICE_NORMALMESSAGE(_message, ...) \
    CP_NORMALMESSAGE(MOS_CP_SUBCOMP_DEVICE, _message, ##__VA_ARGS__)

#define PAVP_DEVICE_ASSERT(_expr) CP_ASSERT(MOS_CP_SUBCOMP_DEVICE, _expr)

// restart PAVP init in case of teadown, limit max retry
#define PAVP_TEARDOWN_RETRY                     0x2

class CPavpOsServices;
class CPavpGpuHal;
class CPavpRootTrust;
class CodechalCencDecode;
class CodechalCencDecodeNext;
class MhwInterfaces;

const UINT16 PlanesToEnable =   (PAVP_PLANE_DISPLAY_A | PAVP_PLANE_DISPLAY_B | PAVP_PLANE_DISPLAY_C) |
                                (PAVP_PLANE_SPRITE_A | PAVP_PLANE_SPRITE_B | PAVP_PLANE_SPRITE_C) |
                                PAVP_PLANE_CURSOR_C;

class CPavpDevice
{
public:   

    // TODO: Use this assert once mainline Android builds support c++11
    //static_assert(0 == PAVP_ROTATION_KEY_BLOB_SIZE % sizeof(DWORD), "Key size is not divisible into uint32_t");
    static CONST uint32_t PAVP_KEY_SIZE_IN_DWORDS = PAVP_ROTATION_KEY_BLOB_SIZE / sizeof(DWORD);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Determines if PAVP is enabled in this environment.
    /// \par         Details:
    /// \li          Checks the sku table inside the adapter info parameter to see if PAVP has been 
    ///              forcefully disabled.
    ///
    /// \param       pSkuTable [in] Pointer to the SKU table structure
    /// \return      TRUE if PAVP is enabled, FALSE if not or pAdapterInfo is NULL. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL IsPavpEnabled(MEDIA_FEATURE_TABLE *pSkuTable);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Determines if PAVP will work on the provided decode mode with the given crypto type.
    /// \par         Details:
    /// \li          This static method can be used without a creating an object.
    ///
    /// \param       eEncryptionType   [in] The incoming encryption type used by the application for decode.
    /// \param       eDecodeMode       [in] The decode format and mode that will be used.
    /// \param       pSkuTable         [in] The SKU table
    /// \param       bIgnoreDecodeMode [in, optional] Ignore the decode profile and just check encryption.
    ///              Set to TRUE only when the decode profile is unknown.
    /// \return      TRUE if PAVP the encryption type and decode format are compatible. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    static  BOOL IsDecodeProfileSupported(
                    PAVP_ENCRYPTION_TYPE    eEncryptionType,
                    uint32_t                codechalMode,
                    MEDIA_FEATURE_TABLE     *pSkuTable,
                    BOOL                    bIgnoreDecodeMode = FALSE);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Determines if PAVP will work on the provided encode mode with the given crypto type.
    /// \par         Details:
    /// \li          This static method can be used without a creating an object.
    ///
    /// \param       eEncryptionType   [in] The outgoing encryption type used by the hardware when encoding the stream.
    /// \param       eEncodeMode       [in] The encode mode that will be used.
    /// \param       pSkuTable         [in] The SKU table
    /// \param       bIgnoreEncodeMode [in, optional] Ignore the encode profile and just check encryption.
    ///              Set to TRUE only when the profile is unknown.
    /// \return      TRUE if PAVP the encryption type and encode format are compatible. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    static  BOOL IsEncodeProfileSupported(
                    PAVP_ENCRYPTION_TYPE eEncryptionType,
                    uint32_t             codechalMode,
                    MEDIA_FEATURE_TABLE  *pSkuTable,
                    BOOL                 bIgnoreEncodeMode = FALSE);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Determines if PAVP will work on the provided encode mode with the given pavp session mode.
    /// \par         Details:
    /// \li          This method is used to check if pavp session mode is supported on provided codec mode
    ///
    /// \param       pavpSessionMode   [in] The pavp session mode that will be used..
    /// \param       pavpSessionType   [in] The pavp session type that will be used.
    /// \param       codechalMode      [in] The codec mode that will be used.
    /// \param       pSkuTable         [in] The SKU table
    /// \return      TRUE if PAVP the session mode and encode format are compatible. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsPavpSessionModeSupported(
                    PAVP_SESSION_MODE    pavpSessionMode,
                    PAVP_SESSION_TYPE    pavpSessionType,
                    CODECHAL_MODE        codechalMode,
                    MEDIA_FEATURE_TABLE *pSkuTable);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Returns the PAVP session type currently in use.
    /// \par         Details:
    /// \li          Returns the most recently set PAVP session type.
    ///
    /// \return      A PAVP_SESSION_TYPE. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    PAVP_SESSION_TYPE GetSessionType();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Returns the PAVP encryption type currently in use.
    /// \par         Details:
    /// \li          This is meant for the encode path to understand how to interpret encryption inputs.
    ///
    /// \return      A PAVP_ENCRYPTION_TYPE. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    PAVP_ENCRYPTION_TYPE GetEncryptionType();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       open arbitrator session
    /// \par         Details:
    /// \li          This api is for quick check whether arbitrator session has been setup, if session already
    ///              exists, will setup context in UMD.
    ///
    /// \return      S_OK upon success, a failed if session is not alive.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT OpenArbitratorSession();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Checks if a PAVP session is alive at the hardware level.
    /// \par         Details:
    /// \li          Typically used to determine if the hardware has unexpectedly torn down PAVP.
    ///
    /// \param       bSessionInPlay [out] TRUE if the hardware has a session alive for this device object.
    ///              Only valid if S_OK is returned.
    /// \param       sessionId [in] session id to query
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT QuerySessionInPlay(BOOL &bSessionInPlay, uint32_t sessionId);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Increments the given counter by the value specified.
    /// \par         Details:
    /// \li          The counter should be a 16 byte buffer.
    /// \li          The counter type is hard coded to type 'C' (128-bit counter).
    ///
    /// \param       pCounter       [in] The counter to increment.
    /// \param       dwIncrement    [in] The incrementation value. 
    /// \return      TRUE on success.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL IncrementCounter(
        PDWORD              pCounter,
        DWORD               dwIncrement);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Swaps the endianness of a counter.
    /// \par         Details:
    /// \li          The counter should be a 16 byte buffer.
    /// \li          The counter type is hard coded to type 'C' (128-bit counter).
    ///
    /// \param       pCounter     [in] The counter to be formatted.
    /// \return      TRUE on success.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CounterSwapEndian(PDWORD pCounter);

    /// \brief       Sets WiDi session parameters and marks the session as 'ready'.
    /// \par         Details:
    /// \li          The given parameter will be later read by the media driver while performing WiDi encode.
    ///
    /// \param       rIV          [in] The IV to be used for WiDi encoding.
    /// \param       StreamCtr    [in] The stream counter to be used for WiDi encoding.
    /// \param       bWiDiEnabled [in] Indicates whether WiDi is currently enabled.
    /// \param       ePavpSessionMode [in] Pavp session mode
    /// \param       HDCPType     [in] Indicates the type of HDCP authenticated
    /// \param       bDontMoveSession - [in] Indicates that as a side effect this method should not move the 
    ///                                 status of the PAVP session at all.
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT SetWiDiSessionParameters(
        UINT64            rIV,                        // [in]
        uint32_t            StreamCtr,                  // [in]
        BOOL              bWiDiEnabled,               // [in]
        PAVP_SESSION_MODE ePavpSessionMode,           // [in]
        ULONG             HDCPType,                   // [in]
        BOOL              bDontMoveSession = FALSE);  // [in]

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Checks whether the session is initialized.
    /// \param       bIsSessionInitialized [out] Indicates whether the session is in any state other than PAVP_SESSION_READY or PAVP_SESSION_INITIALIZED.
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT IsSessionInitialized(BOOL& bIsSessionInitialized); // [out]

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Checks whether the session is alive.
    /// \param       bIsSessionAlive [out] Indicates whether the session is in any state other than PAVP_SESSION_READY.
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT IsSessionReady(BOOL& bIsSessionReady); // [out]

    // HSW Android WideVine.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Start HSW WideVine connection status heart beat message.
    /// \par         Details:
    /// \li          This function should be called after set entitlement key.
    ///
    /// \return      status_ok on success, error codes on failure.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT CheckCSHeartBeat();
    
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Returns the current session ID reserved for this PAVP device.
    /// \return      The current session ID or PAVP_INVALID_SESSION_ID if no session ID is reserved.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    uint8_t GetSessionId();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Returns the current device/crypto session status that if memory encryption is enabled.
    /// \return      
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool IsMemoryEncryptionEnabled() const;

    bool IsArbitratorSessionRequired() const;
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Returns CP tag of current CP context of PAVP device/crypto session
    /// \return      
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    uint32_t GetCpTag() { return m_cpContext.value; }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Saves the Crypto Key ex params in MHW
    /// \return      
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT SetCryptoKeyExParams(PMHW_PAVP_ENCRYPTION_PARAMS pMhwPavpEncParams);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Create a PAVP arbitrator session (dummy) session with session id=15, for protected resource
    /// \par         Details:
    /// \li          Create an arbitrator session for some usages where we need PAVP alive in the background.
    ///
    /// \param       pDeviceContext [in] A pointer to a memory region represending a device context.
    ///                             The OS services class will interpret it based on the OS.
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT InitArbitratorSessionRes(PAVP_DEVICE_CONTEXT pDeviceContext);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Get CENC decoder from crypto session
    /// \par         Details:
    /// \li          CENC decoder is for crypto session PPED, decoder need it for decode.
    ///
    /// \param       void
    /// \return      CENC decoder instance
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    CodechalCencDecode *GetCencDecoder(void) { return m_cencDecoder; }

    CodechalCencDecodeNext *GetCencDecoderNext(void) { return m_cencDecoderNext; }
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Returns true if GetStreamKey is being used
    /// \return
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool IsGSK() { return m_bGSK; }

protected:

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Builds and Submits a multipleCryptoCopy command buffer.
    /// \param       pBuildMultiCryptoCopyParams  [in] Point to the multiple crypto copy parameteres
    /// \param       cryptocopyNumber             [in] The Number of multiple crypto copy parameteres
    /// \return      0 upon success, a failed int32_t otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    int32_t BuildAndSubmitMultiCryptoCopyCommand(PAVP_GPU_HAL_MULTI_CRYPTO_COPY_PARAMS *pBuildMultiCryptoCopyParams, size_t cryptoCopyNumber) CONST;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Builds and Submits a command buffer.
    /// \par         Details:
    /// \li          Based on the type of command requested, uses the GPU HAL to build a command buffer and
    ///              uses the OS services to submit it.
    ///
    /// \param       type    [in] The type of PAVP operation (key exchange, crypto copy, query status).
    /// \param       pParams [in] A void * that's cast depending on the type parameter. See the GPU HAL header file
    ///              to determine which structure is expected by which command.
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT BuildAndSubmitCommand(PAVP_OPERATION_TYPE type, PVOID pParams) CONST;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Reads PAVP debug hardware registers and prints their values.
    /// \par         Details:
    /// \li          Does nothing if the OS services object has not been created (requires class initialization).
    /// \li          Only available on debug or "release internal" drivers.
    /// \li          Useful for diagnosing problems at the hardware level.
    ///
    /// \return      no return value.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    void PrintDebugRegisters() CONST;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Checks to see if translation from daisy-chaining to ASMF is requrired for the platform
    ///              and for the specified DRM type (i.e., PlayReady3 does not need translation).
    ///
    /// \return      true if required or false
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual bool NeedsASMFTranslation();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Translates the StreamKey from (Sn)Sn-1 to (Sn)Kb for decryption or encryption (or both).
    /// \par         Details:
    /// \li          The incoming keys are expected to be encrypted with the previous key [(Sn)Sn-1].
    /// \li          UMD needs to send this key to ME to get (Sn)Kb which is required for ASMF mode.
    ///
    /// \param       SetKeyParams [in] An input structure specifying which key(s) to update and the corresponding
    ///                                packet(s).
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT TranslateStreamKeyForASMFMode(PAVP_SET_STREAM_KEY_PARAMS& SetKeyParams);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Uses GPU HAL services to request a signed connection state and memory status from hardware.
    /// \par         Details:
    /// \li          Can't be called until the Init() function has called and succeeded.
    /// \li          A valid nonce should be in the ConnState parameter.
    ///
    /// \param       ConnState [out] The structure that will receive the connection state and memory status.
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT GetConnectionState(PAVP_GET_CONNECTION_STATE_PARAMS2& ConnState) CONST;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Obtains the plane enable packet from firmware and programs it, enabling PAVP heavy mode
    ///              and enabling display plane encryption.
    /// \par         Details:
    /// \li          When a platform supports per-app lite vs. heavy mode, this sets the session in heavy mode.
    /// \li          Currently all display planes are enabled (but only overlay enabling is actually required).
    /// \li          Plane enables are necessary before heavy mode content can be decrypted.
    ///
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT SetPlaneEnables();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Returns true if the current session ID is valid.
    ///
    /// \return      TRUE if the session ID has been set and is valid.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsSessionIdValid() CONST;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Terminates the PAVP session, resets PAVP device to an uninitialized state.
    /// \par         Details:
    /// \li          this is called to clean up PAVP hardware state, also called in error state cleanup
    ///
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT CleanupHwSession();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Class constructor
    /// \par         Details:
    /// \li          The constructor is minimal. All major initialization is done in the Init() function. 
    ///
    /// \param       No input parameters. 
    /// \return      No value returned. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    CPavpDevice();
    
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Class destructor
    /// \par         Details:
    /// \li          Cleans up PAVP at the hardware level, closes open MEI connection, releases all resources.
    ///
    ///              The destructor must be virtual, because a virtual table is created for this class instances.
    ///              If not, a derived class destructor would not be called when calling the base class destructor.
    ///
    /// \param       No input parameters. 
    /// \return      No value returned.  
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~CPavpDevice();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Returns true if the Init() function was called and succeeded.
    /// \par         Details:
    /// \li          This lets other areas of the driver know if Init() needs to be called or not.
    ///
    /// \return      TRUE if Init() has been called and succeeded. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsInitialized() CONST;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Checks the given key exchange type and prepares the hardware/firmware for its use.
    /// \par         Details:
    /// \li          This call is only expected to be made once for the life of the object. 
    /// \li          Should be called as soon as the application's desired key exchange type is known. 
    /// \li          This call is required before the application can udpate any keys.  
    /// \li          Pre-condition: PCH HAL/GSC HAL layer has been set up in the constructor. 
    /// \li          Post-condition: ME connection will be opened; IsInitialized() function will return TRUE. 
    /// \li          Post-condition: If this is a "displayable" session, the hardware should have a session in play.
    ///
    /// \param       pDeviceContext          [in] A pointer to a memory region represending a device context.
    ///                                           The OS services class will interpret it based on the OS.
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT Init(PAVP_DEVICE_CONTEXT pDeviceContext);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Create PAVP HW session for key exchange.
    /// \par         Details:
    /// \li          This call is only expected to be made once for the life of the crypto session. 
    /// \li          Should be called as soon as the application's desired key exchange type is known. 
    /// \li          This call is required before the application can udpate any keys.  
    /// \li          Pre-condition: Init has been called
    ///
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT InitHwSession();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Write register
    /// \par         Details:
    /// \li          This call is used to call write register through cp os services.
    /// \li          UserFeatureValueId: feature value ID.
    /// \li          dwData: feature value.
    /// \li          Pre-condition: Init has been called
    ///
    /// \return      void.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    void WriteRegistry(
        CP_USER_FEATURE_VALUE_ID  UserFeatureValueId,
        DWORD                     dwData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Decide if we want to use older FW API version
    /// \par         Details:
    /// \li          Failure was observed for few use cases on SKL in case newer API version is used
    /// \li          Therefore, it was decided to use older FW API version
    /// \li          FWFallback logic should be removed once master driver stop supporting effected hardware
    /// \li          Details can be found here https://jira.devtools.intel.com/browse/VSMGWL-25042
    ///
    /// \return      True if fallback is required, false otherwise.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool IsFWFallbackRequired();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Establishes the transcode PAVP HW key exchange.
    /// \par         Details:
    /// \li          This call is only expected to be made once for the life of the crypto session. 
    /// \li          Should be called as soon as the application's desired key exchange type is known. 
    /// \li          This call is required before the application can udpate any keys.  
    /// \li          Pre-condition: Init has been called
    ///
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT InitHwTranscodeSession();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Establishes the decode PAVP HW key exchange.
    /// \par         Details:
    /// \li          This call is only expected to be made once for the life of the crypto session. 
    /// \li          Should be called as soon as the application's desired key exchange type is known. 
    /// \li          This call is required before the application can udpate any keys.  
    /// \li          Pre-condition: Init has been called
    ///
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT InitHwDecodeSession();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Gets, authenticates, transcrypts, then sets encrypted kernels for video processor.
    /// \par         Details:
    /// \li          This call is required once before start of stout mode decode session initialization.  
    /// \li          Pre-condition: Init has been called.
    ///
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT InitAuthenticatedKernels();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Creates an OS Services object.
    ///
    /// \param       pDeviceContext a pointer to a memory region represending a device context. 
    ///                             The OS services class will interpret it based on the OS.
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT CreateOsServices(PAVP_DEVICE_CONTEXT pDeviceContext);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Sets the PAVP session type which is stored in the object for later use. 
    /// \par         Details:
    /// \li          This should be called before the Init() function is called. 
    /// \li          The session type can only be set once unless the object is destroyed. 
    /// \li          If this is not called before Init(), the value defaults to a 'decode' session. 
    ///
    /// \param       eSessionType [in] The PAVP_SESSION_TYPE value that the application is requesting.
    /// \return      S_OK upon success, E_INVALIDARG if the type is not supported.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT SetSessionType(PAVP_SESSION_TYPE eSessionType);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Sets the PAVP session mode which is stored in the object for later use. 
    /// \par         Details:
    /// \li          This should be called before the Init() function is called. 
    /// \li          The session type can only be set once unless the object is destroyed. 
    /// \li          If this is not called before Init(), the value defaults to a 'Lite' session. 
    ///
    /// \param       eSessionType [in] The PAVP_SESSION_MODE value that the application is requesting.
    /// \return      S_OK upon success, E_INVALIDARG if the type is not supported.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT SetSessionMode(PAVP_SESSION_MODE eSessionMode);

#if(_DEBUG || _RELEASE_INTERNAL)
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Sets the PAVP crypto session type which is stored in the object for later use.
    /// \par         Details:
    /// \li          This should be called after the Init() is done.
    /// \li          This is only to enable FW team to do their GSC testing
    ///
    /// \param       ePavpCryptoSessionType [in] The PAVP_CRYPTO_SESSION_TYPE value based on crypto GUID.
    /// \return      void.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    void SetPavpCryptSessionType(PAVP_CRYPTO_SESSION_TYPE ePavpCryptoSessionType);
#endif

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Sets the DRM Type which is stored in the object for later use. 
    /// \par         Details:
    /// \li          This should be called before the Init() function is called. 
    /// \li          The Drm type can only be set once unless the object is destroyed. 
    /// \li          If this is not called before Init(), the value defaults to a PAVP_DRM_TYPE_NONE. 
    ///
    /// \param       eDrmType [in] The PAVP_DRM_TYPE value that the application is requesting.
    /// \return      S_OK upon success, E_INVALIDARG if the type is not supported.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT SetDrmType(PAVP_DRM_TYPE eDrmType);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Sets the PAVP Encryption type which is stored in the object for later use. 
    /// \par         Details:
    /// \li          Typically will be called once right after object creation. 
    /// \li          This call is required before the SetCounterValue function can be used. 
    ///
    /// \param       eEncryptionType [in] The PAVP_ENCRYPTION_TYPE value that the application will use to encrypt content.
    /// \return      S_OK upon success, E_INVALIDARG if the type is not supported.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT SetEncryptionType(PAVP_ENCRYPTION_TYPE eEncryptionType);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Encrypts an incoming source surface and performs a blt to another memory region. 
    /// \par         Details:
    /// \li          Can handle system -> graphics memory blts 
    /// \li          The source surface should be NV12 clear surface
    ///
    /// \param       EncryptBlt [in] General parameters needed about the surfaces in order to perform the blt.
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT DoEncryptionBlt(PAVP_ENCRYPT_BLT_PARAMS& EncryptBlt) CONST;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Encrypts an incoming linear source surface and performs a blt to another linear memory region. 
    /// \par         Details:
    /// \li          Can handle system -> graphics memory blts 
    /// \li          The source surface should be NV12 clear surface
    ///
    /// \param       EncryptBlt [in] General parameters needed about the surfaces in order to perform the blt.
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT DoEncryptionBltLinearBuffer(PAVP_ENCRYPT_BLT_PARAMS& EncryptBlt) CONST;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Retrieves the current Widi counter value and populates the appropriate value
    /// \par         Details:
    /// \li          Populates the appropriate values in the given IV structure
    ///
    /// \param       IV [out] An output structure to hold the Widi counter value
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT GetWiDiCounter(DWORD IV[4]) CONST;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Find which subsample the current buffer belongs to
    ///
    /// \param      eachRegionOffset  [in]  the offset of each subsample
    /// \param      offset            [in]  Indicates the offset in input buffer
    /// \return     The subsample which current buffer belongs
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    uint32_t FindSubsamplePos(std::vector<size_t> &eachRegionOffset, uint32_t offset);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      assemble audio cryptocopy Param
    ///
    /// \param      pIV                 [in]  The IV + counter to decrypt or encrypt the surface (128 bit = 4 DWORDS).
    /// \param      pInputResource      [in]  The source resource which contains the AES or serpent encrypted data
    /// \param      pOutputResource     [out] The destination resource, which will contain the transcrypted data.
    /// \param      bufSize             [in]  The size of the buffer in bytes
    /// \param      uiAesMode           [in]  AES mode for input data
    /// \param      uiCryptoCopyMode    [in]  Crypto copy mode to use
    /// \param      offset              [in]  Indicates the offset of the buffer to be transcrypted in the inputResource
    /// \param      sCryptoCopyParams   [in]  The crypto copy parameteres
    /// \return     0 upon success, a failed int32_t otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    inline int32_t AssembleAudioCryptoCopyParams(
        uint32_t                               *pIV,
        PMOS_RESOURCE                          pInputResource,
        PMOS_RESOURCE                          pOutputResource,
        uint32_t                               bufSize,
        uint32_t                               uiAesMode,
        uint32_t                               uiCryptoCopyMode,
        uint32_t                               offset,
        PAVP_GPU_HAL_MULTI_CRYPTO_COPY_PARAMS  &sCryptoCopyParams);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Decrypts a linear buffer and transcrypts it using AES-CTR with another key.
    ///
    /// \param      inputBufferMap          [in]  track of bytes within the encrypted buffer and clear buffer
    /// \param      inputAllBuffer          [in]  The source resource
    /// \param      bufSize                 [in]  The size of the surface in bytes
    /// \param      outputAllBuffer         [out] The destination resource, which will contain the transcrypted data
    /// \param      m_AudioDecryptParams    [in]  Audio transcryption parameters and context
    /// \return     0 upon success, a failed int32_t otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    int32_t TranscryptAudioLinearBuffer(
        AudioBufferMap      &inputBufferMap,
        uint8_t             *inputAllBuffer,
        size_t              bufSize,
        uint8_t             *outputAllBuffer,
        AudioDecryptParams  &m_AudioDecryptParams);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Decrypts a linear buffer and transcrypts it using AES-CTR with another key.
    ///
    /// \param      pDecryptIV      [in]  The IV + counter that was used to decrypt the surface (128 bit = 4 DWORDS).
    /// \param      pEncryptIV      [in]  The IV + counter that was used to encrypt the surface.
    /// \param      pSrcResource    [in]  The source resource which contains the AES encrypted data.
    /// \param      pDstResource    [out] The destination resource, which will contain the transcrypted data.
    /// \param      bufSize         [in]  The size of the surface in bytes
    /// \param      uiAesMode       [in]  AES mode for input data
    /// \return     S_OK if successfull, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT TranscryptLinearBuffer(
        PDWORD  pDecryptIV,
        PDWORD  pEncryptIV,
        PBYTE   pSrcResource,
        PBYTE   pDstResource,
        SIZE_T  bufSize,
        uint32_t  uiAesMode = CRYPTO_COPY_AES_MODE_CTR);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Decrypts or encrypts a linear buffer.
    ///
    /// \param      pIV             [in]  The IV + counter to decrypt or encrypt the surface (128 bit = 4 DWORDS).
    /// \param      pSrcResource    [in]  The source resource which contains the AES encrypted data.
    /// \param      pDstResource    [out] The destination resource, which will contain the transcrypted data.
    /// \param      bufSize         [in]  The size of the buffer in bytes
    /// \param      uiAesMode       [in]  AES mode for input data
    /// \param      uiCryptoCopyMode [in] Crypto copy mode to use
    /// \return     S_OK if successfull, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT TranscryptLinearBuffer(
        PDWORD  pIV,
        PBYTE   pSrcResource,
        PBYTE   pDstResource,
        SIZE_T  bufSize,
        uint32_t  uiAesMode,
        uint32_t  uiCryptoCopyMode);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Decrypts an incoming source surface and performs a blt to another memory region. 
    /// \par         Details:
    /// \li          Can handle system -> graphics memory blts 
    /// \li          If PAVP heavy mode is used, the output surface will not be accessible by software
    ///              and must be displayed using overlay hardware so it can be decrypted. 
    ///
    /// \param       DecryptBlt [in] General parameters needed about the surfaces in order to perform the blt.
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT DoDecryptionBlt(PAVP_DECRYPT_BLT_PARAMS& DecryptBlt) CONST;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Decrypts an incoming linear source surface and performs a blt to another linear memory. 
    /// \par         Details:
    /// \li          Can handle system -> graphics memory blts 
    /// \li          If PAVP heavy mode is used, the output surface will not be accessible by software
    ///              and must be displayed using overlay hardware so it can be decrypted. 
    ///
    /// \param       DecryptBlt [in] General parameters needed about the surfaces in order to perform the blt.
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT DoDecryptionBltLinearBuffer(PAVP_DECRYPT_BLT_PARAMS& DecryptBlt) CONST;
    
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Retrieves a random value from hardware that an app can XOR with the old key to update it. 
    /// \par         Details:
    /// \li          The hardware will not start using the new key until the SetFreshnessValue has been called. 
    /// \li          If PAVP heavy mode is used, the application is required to make at least 1 overlay flip
    ///              before refreshing its first session key. This is because we cache the session-key-encrypyed
    ///              plane enable packet on some platforms until the first flip and refreshing the key invalidates it. 
    ///
    /// \param       GetFreshness [out] An output structure to hold the freshness value.
    /// \param       eType        [in]  An input specifying which key (decode/decrypt or encode/encrypt) should be updated.
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT GetFreshnessValue(PAVP_GET_FRESHNESS_PARAMS& GetFreshness, PAVP_FRESHNESS_REQUEST_TYPE eType) CONST;
    
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Tells the hardware to XOR its old key with the most recent value returned in 
    ///              GetFresshnessValue(). 
    /// \par         Details:
    /// \li          After this call returns, the crypto engine will begin using the new key.
    /// \li          The application should synchronize this with its decoding.
    ///
    /// \param       eType [in] Specifies which key (decode/decrypt or encode/encrypt) should be updated.
    ///                         It's important the app uses this consistently between GetFreshness/SetFreshness calls.
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT SetFreshnessValue(PAVP_FRESHNESS_REQUEST_TYPE eType) CONST;
    
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Tells the hardware to immediately use a new key for decryption or encryption (or both). 
    /// \par         Details:
    /// \li          After this call returns, the crypto engine will begin using the new key. 
    /// \li          The application should synchronize this with its decoding.
    /// \li          The new keys are expected to be encrypted with the previous key. 
    /// \li          If both keys are to be updated, the application must use provide 2 key packets. 
    /// 
    /// \param       SetKeyParams [in] An input structure specifying which key(s) to update and the corresponding
    ///                                packet(s).
    /// \param       KeyTranslationRequired [in] Optional parameter that indicates whether keys should be translated
    ///                                     from daisy-chain to kb-based
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT SetStreamKey(PAVP_SET_STREAM_KEY_PARAMS& SetKeyParams, bool KeyTranslationRequired = true);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Save key blob information.
    /// \param      pStoreKeyBlobParams [in] key blob parameters
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT StoreKeyBlob(void *pStoreKeyBlobParams);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Reserves a hardware PAVP session for the application.
    /// \par         Details:
    /// \li          Calls to KMD or CPMgr to see if any sessions are available.
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    HRESULT ReserveSessionSlot(uint32_t *pAppID);
 

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Passes input and output data unfiltered to the PCH/GSC for direct app <-> ME/SEC communication. 
    /// \par         Details:
    /// \li          The contents of the buffers should adhere to the ME firmware specification. 
    /// \li          Anything but basic NULL input buffer checking will be done by the ME/SEC firmware.
    /// \li          Parses return data from FW and will inject key from FW to GPU and set session status to ready
    /// \li          if FW sets header bit  
    /// 
    /// \param       pInput    [in]  The input buffer to pass through.
    /// \param       ulInSize  [in]  The size, in bytes, of the pInput buffer.
    /// \param       pOutput   [out] The output buffer which may receive data from the PCH/GSC.
    /// \param       ulOutSize [out] The size, in bytes, of the pInput buffer.
    /// \param       bIsWidiMsg[in]  True if message is for widi
    /// \param       vTag      [in]  Indicates vtag value if sandboxing is used
    /// \return      S_OK upon success, a failed HRESULT otherwise. Additional details may gathered from the
    ///              output buffer headers. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT AccessME(
        PBYTE   pInput,
        ULONG   ulInSize,
        PBYTE   pOutput,
        ULONG   ulOutSize,
        bool    bIsWidiMsg = false,
        uint8_t vTag       = 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Updates a DWORD value from a registry key which represent the open session status.
    /// \par         Details:
    /// \li          The data will be read from the specified registry key. In Linux it will be read from /etc/registry.txt
    /// \li          Afterwards the value is updated according the CP_SESSIONS_UPDATE_TYPE.
    /// \li          This function is a wrapper for OS Services function (SessionRegistryUpdate).
    ///
    /// \param       UserFeatureValueId [in] the key value ID, detailed registry info can be found in mos_utilities.cpp
    /// \param       eSessionType       [in] an enum that represent the session type (Transcode/Decode).
    /// \param       dwSessionId        [in] the session ID in DWORD.
    /// \param       eUpdateType        [in] an enum that reperesnt the required update option (open/close).
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    void SessionRegistryUpdate(    
        CP_USER_FEATURE_VALUE_ID  UserFeatureValueId, // [in]
        PAVP_SESSION_TYPE         eSessionType,       // [in]
        DWORD                     dwSessionId,        // [in] (In case of software sessions this variable is ignored)
        CP_SESSIONS_UPDATE_TYPE   eUpdateType);       // [in]

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Create a PAVP arbitrator session (dummy) session with session id=15
    /// \par         Details:
    /// \li          Create an arbitrator session for some usages where we need PAVP alive in the background.
    /// \li          We must follow the calling sequence:
    /// \li              (1) Init()
    /// \li              (2) InitiateArbitratorSession(), which could be optional.
    /// \li              (3) InitHwSession()
    ///
    /// \param       bPreserveOriginalState: preserve/recover the state of original session after creating dummy sesion.
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT InitiateArbitratorSession(bool bPreserveOriginalState=true);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Parse usage table data from ME and call escape function to interact with KMD 
    ///
    /// \param       HWProtectionFunctionID  [in] function ID to ME FW
    /// \param       pOutputBufferFromME     [in] usage table from ME FW.
    /// \return      S_OK upon success, a failed HRESULT otherwise. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT ProcessUsageTableRequest(uint32_t HWProtectionFunctionID, PBYTE pOutputBufferFromME);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Gets/Sets key rotation blob from ME/SEC. 
    /// \par         Details:
    /// \li          This function gets the key rotation blob from ME/SEC after first app key injection. The 
    ///              blob will be combination of 3 keys - (Sn)Sigma, (Sn)Kb, (Kb)Sn
    ///              Application gets this blob and extracts (Sn)Sigma for its use while pass rest of blob 
    ///              as a parameter to BeginFrame call.
    ///
    /// \param       bGetSetCommand  [in]     An enum indicating if data is gotten from ME-FW or data is set in GPU
    /// \par         Details -
    ///                     PAVP_GET_SET_HW_KEY_NONE - Return App ID from UMD without intreaction with GPU and ME
    ///                     PAVP_SET_CEK_KEY_IN_ME - Program CEK in ME 
    ///                     PAVP_GET_CEK_KEY_FROM_ME - Obtain CEK from ME
    ///                     PAVP_SET_CEK_KEY_IN_GPU - Program key rotation blob on GPU
    ///                     PAVP_SET_CEK_INVALIDATE_IN_ME - Cancel CEK in ME
    /// \param       pInput          [in]     Input to ME-FW.
    /// \param       ulInSize        [in]     Input to ME-FW. Default is 0 to support Windows DDIs
    /// \param       pOutput         [out]    Output from ME-FW.
    /// \param       ulOutSize       [out]    Output from ME-FW.
    /// \return      S_OK upon success, a failed HRESULT otherwise. Additional details may gathered from the
    ///              output buffer headers. 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT AsyncHwKeyInterfaceToMeGpu(
        PAVP_GET_SET_DATA_NEW_HW_KEY eGetSetCommand,       //[in]
        BYTE                         *pInput,              //[in]
        uint32_t                       ulInSize = 0,         //[in]
        BYTE                         *pOutput = nullptr,   //[out]
        uint32_t                       ulOutSize = 0);       //[out]

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Sets key rotation blob in GPU
    /// \li          Programs key rotation blobs for different DRMs
    ///
    /// \param       pKeyInfoBlob [in] Reference to key rotation blob
    HRESULT SetKeyRotationBlob(const INTEL_OEM_KEYINFO_BLOB *pKeyInfoBlob);

    //Please keep m_cpContext as first in class so it can be shared with other .so/.dll
    CpContext           m_cpContext;                // local cp context, give access to child class

#if (_DEBUG || _RELEASE_INTERNAL)
    BOOL                        m_bForceLiteModeForDebug;
    BOOL                        m_bLiteModeEnableHucStreamOutForDebug;
    PAVP_CRYPTO_SESSION_TYPE    m_PavpCryptoSessionMode; //!< Audio, Video, MECOMM, MECOMM PAVP (FW App Testing), MECOMM WIDI (FW App Testing)
#endif

    PAVP_DRM_TYPE       m_eDrmType;                 // Indicates DRM type in use
    BOOL                m_bSkipAsmf;                // Skip ASMF translation
    std::bitset<32>     m_bitGfxInfoIds;
    uint32_t            m_TranscryptKernelSize;     //!< Size of transcrypted kernels, populated in InitAuthenticatedKernels()
    CodechalCencDecode *m_cencDecoder;
    CodechalCencDecodeNext *m_cencDecoderNext;
    PVOID               m_pDeviceContext;           //!< Indicate associated device context.
    bool                m_bHwSessionIsAlive;        //!< True if InitHwSession() or InitiateArbitratorSession() was called and finished
    PAVP_SESSION_TYPE   m_PavpSessionType;          //!< Displayable session or transcode
    PAVP_SESSION_MODE   m_PavpSessionMode;          //!< Lite,Heavy or IsolatedDecode
    bool                m_bFwFallbackEnabled;       //!< Indicates current firmware fall back is enabled or not.   
    bool                m_bGSK;                     //!< True if GetStreamKey is used

    /// \defgroup HALsAndServices HALs and services
    /// @{
    std::shared_ptr<CPavpOsServices>        m_pOsServices;
    CPavpRootTrustHal                       *m_pRootTrustHal;
    CPavpGpuHal                             *m_pGpuHal;
    ///@}

    void SetHwSessionAlive() { m_bHwSessionIsAlive = true; }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Wrapper around TerminateSession to abstract out the bool flag.
    /// \par         Details:
    ///
    /// \param       SessionId          [in] The pavp session id to terminate
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT TerminateSessionWithConfirmation(uint32_t SessionId)
    {
        return TerminateSession(SessionId, TRUE);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Called if this is the first session since driver power up or reset.
    ///              Called just once to do power up setup upon fist PAVP session after boot
    /// \par         Details:
    /// \li          Initializes CB2 buffer and "zero" session terminate
    ///
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT PowerUpInit();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Sends a key termination command to GPU and ME
    /// \par         Details:
    ///
    /// \param       SessionId          [in] The pavp session id to terminate
    /// \param       CheckSessionInPlay [in] Flag to indicate whether to qualify the cmd with reads of the
    ///                                      SessionInPlay register.
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT TerminateSession(uint32_t SessionId, BOOL CheckSessionInPlay = FALSE);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Reserves a hardware PAVP session for the application.
    /// \par         Details:
    /// \li          Calls to KMD or CPMgr to see if any sessions are available.
    ///
    /// \param       eSsessionType      [in] The requested PAVP session type
    /// \param       eSsessionMode      [in] The requested PAVP session Mode
    /// \param       pPavpSessionStatus [out] Will contain the session status struct.
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT ReserveSessionId(
        PAVP_SESSION_TYPE       eSsessionType,
        PAVP_SESSION_MODE       eSsessionMode,
        PAVP_SESSION_STATUS*    pPavpSessionStatus,
        PAVP_SESSION_STATUS*    pPreviousPavpSessionStatus);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Sets the global terminate decode bit in HW to allow recovery from auto teardown
    /// \par         Details:
    /// \li          Verifies that no sessions are in play as GPU will hang.
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT GlobalTerminateDecode();

private:
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Sets all member variables to a default value.
    /// \par         Details:
    /// \li          Created for convenience to reset all member variables at once.
    /// \li          Useful since multiple functions might can reset variables (for example the contructor or Cleanup).
    /// \li          If a new member variable is added, be sure to add it here for proper initialization.
    /// \li          Not meant to be called by any external caller.
    ///
    /// \return      No value returned.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    void ResetMemberVariables();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Creates and instantiates the GPU/PCH/GSC HALs and Operating System Services objects.
    /// \par         Details:
    /// \li
    ///
    /// \param       pDeviceContext [in] A pointer to a memory region represending a device context.
    ///                             The OS services class will interpret it based on the OS.
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT CreateHalsAndServices(PAVP_DEVICE_CONTEXT pDeviceContext);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Terminates the PAVP session, closes the ME connection, releases all resources,
    ///              and resets the device to an uninitialized state.
    /// \par         Details:
    /// \li          If this is called the device returns to an default state and Init() must be called again.
    /// \li          This is the one and only function that should be in charge or cleaning up resources.
    /// \li          Not meant to be called by any external caller (used by destructor).
    ///
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT Cleanup();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Rotates just the decryption key in the GPU in ASMF mode
    /// \par         Details:
    /// \li          injects the updated key wrapped in Kb.
    ///
    /// \param       updateKey [in] Reference to the new decrypt key
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT RotateDecryptKeyInASMFMode(
        CONST DWORD(&updateKey)[PAVP_KEY_SIZE_IN_DWORDS]);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Rotates both the encryption and decryption keys in the GPU in ASMF mode
    /// \par         Details:
    /// \li          injects the updated keys wrapped in Kb.
    ///
    /// \param       decryptUpdateKey [in] Pointer to the new decrypt key
    /// \param       encryptUpdateKey [in] Pointer to the new encrypt key
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT RotateBothKeysInASMFMode(
        CONST DWORD(&decryptUpdateKey)[PAVP_KEY_SIZE_IN_DWORDS],
        CONST DWORD(&encryptUpdateKey)[PAVP_KEY_SIZE_IN_DWORDS]);


    CPavpDevice(const CPavpDevice&) = delete;
    CPavpDevice& operator=(const CPavpDevice&) = delete;

    /// \defgroup StateVariables Device state variables
    /// @{
    PAVP_ENCRYPTION_TYPE            m_eCryptoType;           //!< Indicates crypto type in use
    BOOL                            m_bPlaneEnableSet;       //!< Indicates the PED packet has been programmed
    INTEL_OEM_KEYINFO_BLOB          m_cachedPR3KeyBlob;      //!< Holds the driver cache key blob to avoid resetting the stream key on each frame
    MEDIA_WA_TABLE                  m_waTable;               //!< Media WA Table.
    /// @}

    MEDIA_CLASS_DEFINE_END(CPavpDevice)
};
#endif // __CP_PAVPDEVICE_H__
