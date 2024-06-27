/*
* Copyright (c) 2018, Intel Corporation
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
* OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*/
//!
//! \file     mos_gfxinfo_id.h
//! \brief    GfxInfo ID for events
//! \details  
//!

#ifndef __MOS_GFXINFO_ID_H__
#define __MOS_GFXINFO_ID_H__

#include "igdinfo.h"

// Do not change these values!!! Put new value after this block.
typedef enum _GFXINFO_ID_CP
{
    GFXINFO_ID_CP_PLAYREADY = 1,                          //! gfxinfo id for CP PlayReady features
    GFXINFO_ID_CP_WIDEVINE = 2,                           //! gfxinfo id for CP Widevine features
    GFXINFO_ID_CP_WYSIWYS = 3,                            //! gfxinfo id for CP WYSIWYS features
    GFXINFO_ID_CP_WIDI = 4,                               //! gfxinfo id for CP Widi features
    GFXINFO_ID_CP_MIRACAST = 5,                           //! gfxinfo id for CP Miracast features
    GFXINFO_ID_CP_ID = 6,                                 //! gfxinfo id for CP Indrect Display
    GFXINFO_ID_CP_FAIRPLAY = 7,                           //! gfxinfo id for CP Fairplay
    GFXINFO_ID_CP_DX12_PROTECTED_RESOURCE_SESSION = 8,    //! gfxinfo id for CP DX12 protecdted resource session
    GFXINFO_ID_CP_SW_AES = 9,                             //! gfxinfo id for CP SW AES
    GFXINFO_ID_CP_SCD = 10,                               //! gfxinfo id for CP screen capture defense
    GFXINFO_ID_CP_AUDIO_DECRYPTION = 11,                  //! gfxinfo id for CP audio decryption
    GFXINFO_ID_CP_HUC_HEADER_PARSING = 12,                //! gfxinfo id for CP HuC header parsing
    GFXINFO_ID_CP_AUTH_CHANNEL = 13,                      //! gfxinfo id for CP authenticated channel
    GFXINFO_ID_CP_GKB = 14,                               //! gfxinfo id for CP GKB
    GFXINFO_ID_CP_EPID_KEY_EXCHANGE = 15,                 //! gfxinfo id for CP EPID key exchange
    GFXINFO_ID_CP_WIRED_HDCP_22 = 16,                     //! gfxinfo id for CP wired HDCP 2.2
    GFXINFO_ID_CP_AUTO_TEARDOWN = 17,                     //! gfxinfo id for CP auto tear down
    GFXINFO_ID_CP_SESSION_TYPE_MODE = 18,                 //! gfxinfo id for CP session type and mode
    GFXINFO_ID_CP_EPID_PROVISIONING = 19,                 //! gfxinfo id for CP EPID provisioning

} GFXINFO_ID_CP;

typedef enum _GFXINFO_ERRID_CP
{
    // Do not change these values!!! Put new value after this block.
    // CP DX11 API error events
    GFXINFO_ERRID_CP_DX11_GetContentProtectionCaps = 100,
    GFXINFO_ERRID_CP_DX11_CreateCryptoSession = 101,
    GFXINFO_ERRID_CP_DX11_NegotiateCryptoSessionKeyExchange = 102,
    GFXINFO_ERRID_CP_DX11_EncryptionBlt = 103,
    GFXINFO_ERRID_CP_DX11_DecryptionBlt = 104,
    GFXINFO_ERRID_CP_DX11_NegotiateAuthenticatedChannelKeyExchange = 105,
    GFXINFO_ERRID_CP_DX11_QueryAuthenticatedChannel = 106,
    GFXINFO_ERRID_CP_DX11_ConfigureAuthenticatedChannel = 107,
    GFXINFO_ERRID_CP_DX11_GetDataForNewHardwareKey = 108,

    // Do not change these values!!! Put new value after this block.
    // CP DX12 API error events
    GFXINFO_ERRID_CP_DX12_CreateProtectedResourceSession = 200,
    GFXINFO_ERRID_CP_DX12_OpenProtectedResourceSession = 201,

    // Do not change these values!!! Put new value after this block.
    // OPM API error events
    GFXINFO_ERRID_CP_OPMCreateProtectedOutput = 300,
    GFXINFO_ERRID_CP_OPMCreateProtectedOutputForNonLocalDisplay = 301,
    GFXINFO_ERRID_CP_OPMGetRandomNumber = 302,
    GFXINFO_ERRID_CP_OPMSetSigningKeyAndSequenceNumbers = 303,
    GFXINFO_ERRID_CP_OPMGetInformation = 304,
    GFXINFO_ERRID_CP_OPMGetCoppCompatibleInformation = 305,
    GFXINFO_ERRID_CP_OPMConfigureProtectedOutput = 306,
    GFXINFO_ERRID_CP_OPMDestroyProtectedOutput = 307,

    // Do not change these values!!! Put new value after this block.
    // Misc error events
    GFXINFO_ERRID_CP_EPID_PROVISIONING = 400,
    GFXINFO_ERRID_CP_PCH_FW_IO_MESSAGE = 401,

} GFXINFO_ERRID_CP;

#endif //__MOS_GFXINFO_ID_H__
