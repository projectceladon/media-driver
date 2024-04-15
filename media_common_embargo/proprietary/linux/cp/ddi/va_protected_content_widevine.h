/*
 * Copyright (c) 2018-2020 Intel Corporation. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL INTEL AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file va_protected_content_widevine.h
 * \brief Protected Content Widevine DRM specific API
 *
 * This file contains the \ref api_protected_content_widevine "Protected Content
 * Widevine DRM Specific API".
 */

#ifndef VA_PROTECTED_CONTENT_WIDEVINE_H
#define VA_PROTECTED_CONTENT_WIDEVINE_H

#include <stdint.h>
#include <va/va.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup api_intel Protected Content(PC) Widevine DRM API
 *
 * @{
 *
 * The protected content API uses definitions and methodologies from
 va_protected_content interface.
 * - This interface *SHOULD ONLY* be used with va_protected_content interface
 * - Enables Widevine DRM specific features

 */

/** attribute values for VAConfigAttribContentProtectionUsage */
#define VA_PC_USAGE_WIDEVINE 0x00000001

/** \brief TeeExec PASS_THROUGH Function Structure for Query Key Blob via
 * OEMCrypto_DecryptCENC.
 *
 * OEMCryptoResult OEMCrypto_DecryptCENC(
 *     [in] const OEMCrypto_SESSION session,
 *     [in] const uint8_t *data_addr,
 *     [in] size_t data_length,
 *     [in] bool is_encrypted,
 *     [in] const uint8_t *iv,
 *     [in] size_t block_offset,
 *     [in/out] OEMCrypto_DestBufferDesc *out_buffer,
 *     [in] const OEMCrypto_CENCEncryptPatternDesc *pattern,
 *     [in] unit8_t subsample_flags);
 *
 * enum OEMCryptoBufferType {
 *   OEMCrypto_BufferType_Clear,
 *   OEMCrypto_BufferType_Secure,
 *   OEMCrypto_BufferType_Direct
 * } OEMCrytoBufferType;
 *
 * typedef struct {
 *   OEMCryptoBufferType type;
 *   union {
 *     struct { // type == OEMCrypto_BufferType_Clear
 *       uint8_t *address;
 *       size_t max_length;
 *     } clear;
 *     struct { // type == OEMCrypto_BufferType_Secure
 *       void *handle;
 *       size_t max_length;
 *       size_t offset;
 *     } secure;
 *     struct { // type == OEMCrypto_BufferType_Direct
 *       bool is_video;
 *     } direct;
 *   } buffer;
 * }
 * OEMCrypto_DestBufferDesc;
 *
 * typedef struct {
 *   size_t encrypt; // number of 16 byte blocks to decrypt.
 *   size_t skip;    // number of 16 byte blocks to leave in clear.
 *   size_t offset;  // deprecated.
 * } OEMCrypto_CENCEncryptPatternDesc;
 *
 * #define OEMCrypto_FirstSubsample     1
 * #define OEMCrypto_LastSubsample      2
 *
 */
/** \brief As per CDM document:
 *   OEMCrypto_DecryptCENC calls are made after OEMCrypto_SelectKey ID. This way
 *   Key ID is already selected in FW.
 *        CDM->libOEMCrypto => To Query Key Blob
 *        type = OEMCrypto_BufferType_Secure
 */
typedef struct {
  uint8_t hw_key_data[16]; // [out]
} OEMCrypto_KeyBlobInfo;

/* \brief This is private CDM/IHV(Intel) specific call.
 *        CDM->libOEMCrypto => To Query Policy Blob
 */
typedef struct {
  uint8_t policy_blob_info[16]; //[out]
} OEMCrypto_PolicyBlobInfo;
/*
OEMCryptoResult OEMCrypto_QueryPolicyBlob(OEMCrypto_PolicyBlobInfo *blob_out);
*/

/* \brief This is the output value of VA_TEE_EXEC_TEE_FUNCID_HW_UPDATE function id */
typedef struct {
  uint8_t session_id;   // 1 byte
  uint8_t drm_type;     // 1 byte
  uint8_t content_type; // 1 byte
  uint8_t reserved;     // 1 byte
} PAVP_ExtendedSessionId;

/**@}*/

#ifdef __cplusplus
}
#endif

#endif//VA_PROTECTED_CONTENT_WIDEVINE_H
