/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2015-2020
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

======================= end_copyright_notice ==================================*/
//!
//! \file     va_protected_content_private.h
//! \brief    Protected content private API
//! \details  This file contains the "Protected Content Private API".
//!

#ifndef VA_PROTECTED_CONTENT_PRIVATE_H
#define VA_PROTECTED_CONTENT_PRIVATE_H

#include <stdint.h>
#include <va/va.h>
#include "va_protected_content_widevine.h"
#include "va_protected_content.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup api_intel Protected Content(PC) Private API
 *
 * @{
 *
 * The protected content API uses definitions and methodologies from
 va_protected_content interface.
 * - This interface *SHOULD ONLY* be used with va_protected_content interface
 * - Enables test tools and Intel specific features

 */

/**
 * \brief Protected content session mode.
 *
 * This attribute sets the config of the protected session mode via
 * vaCreateConfig, then using this config to create the protected session
 * VAProtectedSessionID. This mode determines the protection level of the
 * session.
 */
#define VAConfigAttribProtectedContentSessionMode 1001
/**
 * \brief Protected content session type.
 *
 * This attribute sets the config of the protected session type via
 * vaCreateConfig, then using this config to create the protected session
 * VAProtectedSessionID. This type determines the type of the session, it could
 * be for display or for transcode.
 */
#define VAConfigAttribProtectedContentSessionType 1002

/** attribute values for VAConfigAttribEncryption */
#define VA_ENCRYPTION_TYPE_BASIC 0x00000010
#define VA_ENCRYPTION_TYPE_CENC_CTR_LENGTH 0x00000020
#define VA_ENCRYPTION_TYPE_CTR_TO_SURFACE 0x00000040
#define VA_ENCRYPTION_TYPE_SURFACE_TO_CTR 0x00000080

/** attribute values for VAConfigAttribContentProtectionSessionMode */
#define VA_PC_SESSION_MODE_LITE 0x00000001
#define VA_PC_SESSION_MODE_HEAVY 0x00000002
#define VA_PC_SESSION_MODE_STOUT 0x00000004

/** attribute values for VAConfigAttribContentProtectionSessionType */
#define VA_PC_SESSION_TYPE_DISPLAY 0x00000001
#define VA_PC_SESSION_TYPE_TRANSCODE 0x00000002

/** attribute values for VAConfigAttribContentProtectionCipherAlgorithm */
#define VA_PC_CIPHER_AES 0x00000001

/** attribute values for VAConfigAttribContentProtectionCipherBlockSize */
#define VA_PC_BLOCK_SIZE_128 0x00000001
#define VA_PC_BLOCK_SIZE_192 0x00000002
#define VA_PC_BLOCK_SIZE_256 0x00000004

/** attribute values for VAConfigAttribContentProtectionCipherMode */
#define VA_PC_CIPHER_MODE_ECB 0x00000001

/** attribute values for VAConfigAttribContentProtectionUsage */
#define VA_PC_USAGE_PLAYREADY 0x00000002
#define VA_PC_USAGE_WIDI_STREAM 0x00000004
#define VA_PC_USAGE_MIRACAST_RX 0x00000008
#define VA_PC_USAGE_MIRACAST_TX 0x00000010
#define VA_PC_USAGE_FAIRPLAY 0x00000020
#define VA_PC_USAGE_PASS_WIDI 0x00000040
#define VA_PC_USAGE_PASS_PAVP 0x00000080
#define VA_PC_USAGE_SECURE_COMPUTE 0x00000100

/** \brief TeeExec Private Function Codes. */
typedef enum _VA_TEE_EXEC_PRIVATE_FUNCTION_ID {
  // 0x40000100~0x4000001FF reserved for private TEE Exec GPU function
  VA_TEE_EXEC_GPU_FUNCID_SET_STREAM_KEY = 0x40000100,
  VA_TEE_EXEC_GPU_FUNCID_GET_SESSION_ID = 0x40000101,
  VA_TEE_EXEC_GPU_FUNCID_GET_WIDI_PARAMS = 0x40000102,
  VA_TEE_EXEC_GPU_FUNCID_IS_SESSION_ALIVE = 0x40000103,
  VA_TEE_EXEC_GPU_FUNCID_GET_SESSION_HANDLE = 0x40000104,
  VA_TEE_EXEC_GPU_FUNCID_RESERVE_SESSION_SLOT = 0x40000105,

} VA_TEE_EXEC_PRIVATE_FUNCTION_ID;

/**@}*/

#ifdef __cplusplus
}
#endif

#endif//VA_PROTECTED_CONTENT_PRIVATE_H
