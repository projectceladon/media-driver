/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2018
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

File Name: intel_pavp_core_api_internal.h

Abstract:

Environment:

Notes:

======================= end_copyright_notice ==================================*/

#ifndef _INTEL_PAVP_CORE_API_INTERNAL_H
#define _INTEL_PAVP_CORE_API_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#if (_DEBUG) || (_RELEASE_INTERNAL)
//  Debug hook for audio transcryption
#define PAVP_TRANSCRYPT_LINEAR_BUFFER 0xFFF

    typedef struct tagPAVP_TRANSCRYPT_LINEAR_BUFFER_PARAMS
{
    DWORD dwAesMode;       // MHW_PAVP_CRYPTO_COPY_AES_MODE
    DWORD dwDecryptIV[4];  // IV to use for decrypting input buffer
    DWORD dwEncryptIV[4];  // IV to use for encrypting output buffer
    DWORD dwBufferSize;    // size of input/output buffer
    BYTE *pbBuffer;        // buffer containing encrypted/transcrypted data
} PAVP_TRANSCRYPT_LINEAR_BUFFER_PARAMS;
#endif

#ifdef __cplusplus
}
#endif

#endif  // _INTEL_PAVP_CORE_API_INTERNAL_H
