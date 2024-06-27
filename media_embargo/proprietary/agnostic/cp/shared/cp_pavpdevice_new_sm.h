/*===================== begin_copyright_notice ==================================
INTEL CONFIDENTIAL
Copyright 2020
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

File Name: cp_pavpdeviceSM.h

Abstract:
    This class contains common code for all content protection interfaces.

Environment:
    OS Agnostic - should support - Windows, Linux

Notes:
    None

======================= end_copyright_notice ==================================*/
#ifndef __CP_PAVPDEVICENEWSM_H__
#define __CP_PAVPDEVICENEWSM_H__

#include "cp_pavpdevice.h"

class CPavpDeviceNewSM : public CPavpDevice
{
public:

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Checks whether the session is initialized.
    /// \param       bIsSessionInitialized [out] Indicates whether the session is in any state other than PAVP_SESSION_INITIALIZED.
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT IsSessionInitialized(BOOL& bIsSessionInitialized); // [out]

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       check for open arbitrator session
    /// \par         Details:
    /// \li          This api is for quick check whether arbitrator session has been setup, if session already
    ///              exists, will setup context in UMD.
    ///
    /// \return      S_OK upon success, a failed if session is not alive.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT OpenArbitratorSession();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Checks whether the session is ready for decode or not.
    /// \param       bIsSessionReady [out] Indicates whether the streamkey has bben injected or not
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT IsSessionReady(BOOL& bIsSessionReady); // [out]

protected:

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Terminates the PAVP session, resets PAVP device to an uninitialized state.
    /// \par         Details:
    /// \li          this is called to clean up PAVP hardware state, also called in error state cleanup
    ///
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT CleanupHwSession();

    CPavpDeviceNewSM();

    virtual ~CPavpDeviceNewSM();

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
    /// \brief       Ask KMD for new session ID in In INIT state
    /// \par         Details:
    /// \li          UMD requests KMD to allocate available session ID and move it ot In INIT
    ///
    /// \param       eSsessionType      [in] The requested PAVP session type
    /// \param       eSsessionMode      [in] The requested PAVP session Mode
    /// \return      S_OK upon success, a failed HRESULT otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT GetSessionIDInit(
        PAVP_SESSION_TYPE       eSessionType,
        PAVP_SESSION_MODE       eSessionMode);

/******Below functions are not valid for new state machine********/

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
    /// \brief       Wrapper around TerminateSession to abstract out the bool flag.
    /// \par         Details:
    ///
    /// \param       SessionId          [in] The pavp session id to terminate
    /// \return      S_OK upon success, a failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT TerminateSessionWithConfirmation(uint32_t SessionId)
    {
        return E_NOTIMPL;
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

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Checks to see if translation from daisy-chaining to ASMF is requrired for the platform
    ///              and for the specified DRM type (i.e., PlayReady3 does not need translation).
    ///
    /// \return      true if required or false
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool NeedsASMFTranslation();

    CPavpDeviceNewSM(const CPavpDeviceNewSM&) = delete;
    CPavpDeviceNewSM& operator=(const CPavpDeviceNewSM&) = delete;

    MEDIA_CLASS_DEFINE_END(CPavpDeviceNewSM)
};

#endif // __CP_PAVPDEVICENEWSM_H__