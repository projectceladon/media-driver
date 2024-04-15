/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2013-2017
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
of the Materials, either expressly, by implication, inducement, estoppel or
otherwise. Any license under such intellectual property rights must be express
and approved by Intel in writing.

File Name: intel_pavp_error.h
Abstract:

Environment:

Notes:  

======================= end_copyright_notice ==================================*/

#ifndef __PAVPSDK_ERROR_H__
#define __PAVPSDK_ERROR_H__

#include "mos_defs.h"

#ifndef UINT32
typedef unsigned int UINT32;
#endif
typedef UINT32 PAVP_STATUS;

// Status spaces base values.
#define PAVP_INTERNAL_STATUS_BASE                       0x1000 // Internal status base value.
#define PAVP_INT_SIGMA_STATUS_BASE                      0x3000 // Internal SIGMA status base value.
#define PAVP_INT_PAVP15_STATUS_BASE                     0x4000 // Internal PAVP 1.5 specyfic status base value.
#define PAVP_DEV_INTERNAL_STATUS_BASE                   0x5000 // Internal driver errors.
#define PAVP_DEV_STATE_STATUS_BASE                      0x6000 // Internal driver device state errors.
#define PAVP_DEV_KEY_EXCG_PROTO_STATUS_BASE             0x7000 // Internal driver key exchange protocol errors.
#define PAVP_DEV_GEN_STATUS_BASE                        0x8000 // Internal driver general errors.

#define PAVP_STATUS_SUCCESS                             0x0000 
#define PAVP_STATUS_INTERNAL_ERROR                      0x1000 
#define PAVP_STATUS_UNKNOWN_ERROR                       0x1001 
#define PAVP_STATUS_INCORRECT_API_VERSION               0x1002 
#define PAVP_STATUS_INVALID_FUNCTION                    0x1003 
#define PAVP_STATUS_INVALID_BUFFER_LENGTH               0x1004 
#define PAVP_STATUS_INVALID_PARAMS                      0x1005 
#define PAVP_STATUS_INVALID_SESSION_INPLAY              0x1006
#define PAVP_STATUS_EPID_INVALID_PUBKEY                 0x3000 
#define PAVP_STATUS_SIGMA_SESSION_LIMIT_REACHED         0x3002 
#define PAVP_STATUS_SIGMA_SESSION_INVALID_HANDLE        0x3003 
#define PAVP_STATUS_SIGMA_PUBKEY_GENERATION_FAILED      0x3004 
#define PAVP_STATUS_SIGMA_INVALID_3PCERT_HMAC           0x3005 
#define PAVP_STATUS_SIGMA_INVALID_SIG_INTEL             0x3006 
#define PAVP_STATUS_SIGMA_INVALID_SIG_CERT              0x3007 
#define PAVP_STATUS_SIGMA_EXPIRED_3PCERT                0x3008 
#define PAVP_STATUS_SIGMA_INVALID_SIG_GAGB              0x3009 
#define PAVP_STATUS_PAVP_EPID_INVALID_PATH_ID           0x4000 
#define PAVP_STATUS_PAVP_EPID_INVALID_STREAM_ID         0x4001 
#define PAVP_STATUS_PAVP_EPID_STREAM_SLOT_OWNED         0x4002 
#define PAVP_STATUS_INVALID_STREAM_KEY_SIG              0x4003 
#define PAVP_STATUS_INVALID_TITLE_KEY_SIG               0x4004 
#define PAVP_STATUS_INVALID_TITLE_KEY_TIME              0x4005 
#define PAVP_STATUS_COMMAND_NOT_AUTHORIZED              0x4006 
#define PAVP_STATUS_INVALID_DRM_TIME                    0x4007 
#define PAVP_STATUS_INVALID_TIME_SIG                    0x4009 
#define PAVP_STATUS_TIME_OVERFLOW                       0x400A 
#define PAVP_STATUS_INVALID_ICV                         0x400B

    // PAVP Device Status Codes
    // Internal errors:
#define PAVP_STATUS_BAD_POINTER                         0x5000 
#define PAVP_STATUS_NOT_SUPPORTED                       0x5001 // PAVP is not supported.
#define PAVP_STATUS_CRYPTO_DATA_GEN_ERROR               0x5002 // An error happened in the Crypto Data Gen lib.
#define PAVP_STATUS_MEMORY_ALLOCATION_ERROR             0x5003 
#define PAVP_STATUS_REFRESH_REQUIRED_ERROR              0x5004 
#define PAVP_STATUS_LENGTH_ERROR                        0x5004 
    // Device state errors:
#define PAVP_STATUS_PAVP_DEVICE_NOT_INITIALIZED         0x6000 // Cannot perform this function without first initializing the device.
#define PAVP_STATUS_PAVP_SERVICE_NOT_CREATED            0x6001 // Cannot perform this function without first creating PAVP service.
#define PAVP_STATUS_PAVP_KEY_NOT_EXCHANGED              0x6002 // Cannot perform this function without first doing a key exchange.
#define PAVP_STATUS_PAVP_INVALID_SESSION                0x6003 // Invalid session.
#define PAVP_STATUS_INVALID_KEY_EX_DAA                  0x6004 // DAA selected but not supported.
#define PAVP_STATUS_MEI_INIT                            0x6005 // PAVP DLL was already initialized DLL -> MEI.
#define PAVP_STATUS_MEI_LOAD_FAIL                       0x6006 // Failed to load the PAVP DLL.
#define PAVP_STATUS_MEI_FUNCTION_LOAD_FAIL              0x6007 // Failed to get PAVP DLL functions.
#define PAVP_STATUS_MEI_INIT_FAIL                       0x6008 // PAVP DLL function PavpInit failed.
#define PAVP_STATUS_INVALID_GUID                        0x6009 // Invalid acceleration GUID.
#define PAVP_STATUS_TERMINATE_SESSION_FAIL              0x600a // Terminating session failed.
#define PAVP_STATUS_LOCK_SURFACE_RW_FAIL                0x600b // Failed to lock output surface for read/write.
#define PAVP_STATUS_FAILED_PROGRAM_KEY                  0x600c // Failed to program keys on behalf of ME.
#define PAVP_STATUS_FAILED_PLANE_ENABLE                 0x600d // Failed to set plane enable.
     // Key exchange protocol errors:
#define PAVP_STATUS_KEY_EXCHANGE_TYPE_NOT_SUPPORTED     0x7000 // An invalid key exchange type was used.
#define PAVP_STATUS_PAVP_SERVICE_CREATE_ERROR           0x7001 // A driver error occured when creating the PAVP service.
#define PAVP_STATUS_GET_PUBKEY_FAILED                   0x7002 // Failed to get g^a from PCH but no detailed error was given.
#define PAVP_STATUS_DERIVE_SIGMA_KEYS_FAILED            0x7003 // Sigma keys could not be derived.
#define PAVP_STATUS_CERTIFICATE_EXCHANGE_FAILED         0x7004 // Could not exchange certificates with the PCH but no detailed error was given.
#define PAVP_STATUS_PCH_HMAC_INVALID                    0x7005 // The PCH's HMAC was invalid.
#define PAVP_STATUS_PCH_CERTIFICATE_INVALID             0x7006 // The PCH's certificate was not valid.
#define PAVP_STATUS_PCH_EPID_SIGNATURE_INVALID          0x7007 // The PCH's EPID signature was invalid.
#define PAVP_STATUS_GET_STREAM_KEY_FAILED               0x7008 // Failed to get a stream key from the PCH but no detailed error was given.
#define PAVP_STATUS_GET_CONNSTATE_FAILED                0x7009 // Failed to get a connection state value from the hardware.
#define PAVP_STATUS_GET_CAPS_FAILED                     0x7010 // Failed to get PAVP capabilities from the driver.
#define PAVP_STATUS_GET_FRESHNESS_FAILED                0x7011 // Failed to get a key freshness value from the hardware.
#define PAVP_STATUS_SET_FRESHNESS_FAILED                0x7012 // Failed to set the key freshness value.
#define PAVP_STATUS_SET_STREAM_KEY_FAILED               0x7013 // Failed to update the stream key.
#define PAVP_STATUS_SET_PROTECTED_MEM_FAILED            0x7014 // Failed to set protected memory on/off.
#define PAVP_STATUS_SET_PLANE_ENABLE_FAILED             0x7015 // Failed to set plane enables.
#define PAVP_STATUS_SET_WINDOW_POSITION_FAILED          0x7016 // Failed to set the window position.
#define PAVP_STATUS_AES_DECRYPT_FAILED                  0x7017 // Failed to decrypt.  
#define PAVP_STATUS_AES_ENCRYPT_FAILED                  0x7018 // Failed to encrypt. 
#define PAVP_STATUS_LEGACY_KEY_EXCHANGE_FAILED          0x7019 // Legacy key exchange call failed.
#define PAVP_STATUS_INVALID_NONCE_RETURNED              0x701A // Decrypted nonce value was incorrect.
#define PAVP_STATUS_INVALID_MEMORY_STATUS               0x701B // Decrypted memory status was invalid.
#define PAVP_STATUS_API_UNSUPPORTED                     0x701C // The call is not supported with the API in use.
#define PAVP_STATUS_WRONG_SESSION_TYPE                  0x701D // The call is not supported for the session type in use.
#define PAVP_STATUS_GET_HANDLE_FAILED                   0x701E // Get PAVP handle failed.
#define PAVP_STATUS_GET_PCH_CAPS_FAILED                 0x701F // Get PAVP PCH capabilities failed.
#define PAVP_STATUS_INVALID_CHANNEL                     0x7020 // Unsuccess to create Authenticated Channel.
#define PAVP_STATUS_CAP_NOT_SUPPORTED                   0x7021 // Requested capability is not Supported.

// General errors.
#define PAVP_STATUS_ZERO_GUID                           0x8000 // Zero GUID negotiation failed.
#define PAVP_STATUS_INVALID_SESSION_TYPE                0x8001 // Invalid session type - decode, transcode.
#define PAVP_STATUS_PAVP_DEV_NOT_AVAIL                  0x8002 // Checks if the PAVP device is available in the current platform.
#define PAVP_STATUS_HECI_COMMUNICATION_ERROR            0x80008003 // Returned on communication failure with HECI service, or if said service forwards similar

// PlayReady errors
#define PAVP_STATUS_PLAYREADY_NOT_PROVISIONED           0xE0006 // The BIOS does not support PlayReady HW DRM

#endif // __PAVPSDK_ERROR_H__
