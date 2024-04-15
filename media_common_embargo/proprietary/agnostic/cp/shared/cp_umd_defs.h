/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2012-2017
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

File Name: cp_umd_defs.h

Abstract:
    This defines various types for internal PAVP UMD use between the various PAVP 
    classes. This file is not to be shared with ISVs. 

Environment:
    OS agnostic

Notes:
    None

======================= end_copyright_notice ==================================*/
#ifndef __CP_UMD_DEFS_H__
#define __CP_UMD_DEFS_H__

#include "mos_os.h"
#include "cp_os_defs.h"

typedef PMOS_CONTEXT PAVP_DEVICE_CONTEXT;


#define PAVP_WIDI_SESSION_ID            0x0F


#define PAVP_INVALID_SESSION_ID         0xFF
#define PAVP_KEY_LENGTH                 16      // PAVP keys are always 16 bytes. 

//in Android, PAVP_PRE_SILICON_ENABLED will be depend on HAS_SIM build options in Android.mk
//HDCPSVC is for LSPCON project, which no need to define PAVP_PRE_SILICON_ENABLED
//in Linux, PAVP_PRE_SILICON_ENABLED also depend on HAS_SIM
#if !defined(ANDROID) && !HDCPSVC && !defined(LINUX)
#define PAVP_PRE_SILICON_ENABLED ((_DEBUG || _RELEASE_INTERNAL))
#endif

// Debug only: The PAVP test app sets this bit in the DecryptionBlt pContentKey parameter when "no key exchange" is set. 
// The image is not encrypted and DecryptionBlt then uses CryptoCopy bypass mode.
#define DECRYPTIONBLT_DEBUG_BIT_CRYPTOCOPY_BYPASS   0x00000001

#define PAVP_AUTH_KERNEL_TRANSCRYPT_SESSION_ID      0x86

// Endian swap a 32-bit number (DWORD)
#define SwapEndian32(dw)             \
(                                    \
    (((dw) & 0x000000ff) << 24) |    \
    (((dw) & 0x0000ff00) << 8)  |    \
    (((dw) & 0x00ff0000) >> 8)  |    \
    (((dw) & 0xff000000) >> 24)      \
)                                    \

// Endian swap a 64-bit number
#define SwapEndian64(ptr)                                   \
{                                                           \
    unsigned int Temp = 0;                                  \
    Temp = SwapEndian32(((PUINT)(ptr))[0]);                 \
    ((PUINT)(ptr))[0] = SwapEndian32(((PUINT)(ptr))[1]);    \
    ((PUINT)(ptr))[1] = Temp;                               \
}          

#if PAVP_PRE_SILICON_ENABLED
// -------------------------------------------------------------
// For the builds used in pre-si stage or non-production chip.
// -------------------------------------------------------------
#define PAVP_SIM_QUERY_STATUS_WAIT_ITERATIONS       (6*1000)   // This will cuase max 60 seconds wait.
#define PAVP_SIM_QUERY_STATUS_STORE_DWORD_DISP2VCR  16
#define PAVP_SIM_CONNECTION_VALUE_DATA              0xD0D0D0D0
#endif // PAVP_PRE_SILICON_ENABLED

typedef enum _PAVP_OPERATION_TYPE
{
    KEY_EXCHANGE = 0,   //!< Perform a 'Crypto Key Exchange' command operation
    QUERY_STATUS = 1,   //!< Perform an 'Inline Status Read Request' command operation
    CRYPTO_COPY  = 2    //!< Perform a 'Crypto Copy' command operation
} PAVP_OPERATION_TYPE;

typedef enum {
    CP_SESSIONS_UPDATE_OPEN,
    CP_SESSIONS_UPDATE_CLOSE,
    CP_SESSIONS_UPDATE_ADD,
    CP_SESSIONS_UPDATE_DECREASE
} CP_SESSIONS_UPDATE_TYPE;

typedef enum
{
    PAVP_HWACCESS_GPU_NOT_IDLE, // GPU is not required to be IDLE to perform the escape call.
    PAVP_HWACCESS_GPU_IDLE      // GPU must be IDLE in order to perform the escape call.
} PAVP_HWACCESS_GPU_STATE;

// PAVP decryption blt struct.
typedef struct tagPAVP_DECRYPT_BLT_PARAMS
{
    PMOS_RESOURCE           SrcResource;        //!< Source resource
    PMOS_RESOURCE           DstResource;        //!< Destination resource
    DWORD                   dwSrcResourceSize;  //!< Source resource size
    DWORD                   pIV[4];             //!< Initialization vector
    PVOID                   pContentKey;        //!< Used internally in order to bypass key exchange, or for Widevine audio decryption
    uint32_t                uiContentKeySize;   //!< The ContentKey size
    BOOL                    bPartialDecryption; //!< Indicates if partial decryption was requested
    uint32_t                uiEncryptedBytes;   //!< For partial decryption, number of encrypted bytes in each row.
    uint32_t                uiClearBytes;       //!< For partial decryption, number of clear bytes ((padding)) in each row.
    DWORD                   dwAesMode;          //!< Specifies whether AES-CTR or AES-CBC mode is to be used (only valid for partial decryption)
} PAVP_DECRYPT_BLT_PARAMS;

// PAVP encryption blt struct.
typedef struct tagPAVP_ENCRYPT_BLT_PARAMS
{
    PMOS_RESOURCE           SrcResource;        //!< Source resource
    PMOS_RESOURCE           DstResource;        //!< Destination resource
    DWORD                   dwDstResourceSize;  //!< Source resource size
    DWORD                   pIV[4];             //!< Initialization vector
} PAVP_ENCRYPT_BLT_PARAMS;

typedef enum tagPAVP_PR_DRM_METHOD_ID_DRM_TEE
{
    PAVP_PR_DRM_METHOD_ID_DRM_TEE_AES128CTR_DecryptContent      = 15,
    PAVP_PR_DRM_METHOD_ID_DRM_TEE_H264_PreProcessEncryptedData  = 34,
    PAVP_PR_DRM_METHOD_ID_DRM_TEE_Count                         = 36,
} PAVP_PR_DRM_METHOD_ID_DRM_TEE;

#define DRM_METHOD_ID_DRM_TEE_SERIALIZATION_MASK    0X80000000 
#define SPECIAL_METHOD_EXECUTION_ACK                0xFFFF 

typedef enum tagPAVP_PCH_IOCTL_PROCESS_TYPE
{
    PAVP_PCH_IOCTL_PROCESS_BIOS_DMA_ASSISTED_BUFFER  = 0,
    PAVP_PCH_IOCTL_PROCESS_AUDIO_ASSISTED_BUFFER     = 1,
    PAVP_PCH_IOCTL_PROCESS_HUC_ASSISTED_BUFFER       = 2,
    PAVP_PCH_IOCTL_PROCESS_TYPE_COUNT                = 3,
} PAVP_PCH_IOCTL_PROCESS_TYPE;

typedef enum tagPAVP_PCH_IOCTL_DMA_TYPE
{
    PAVP_PCH_IOCTL_DMA_TYPE_NONE    = 0,
    PAVP_PCH_IOCTL_BIOS_DMA_TYPE    = 1,
    PAVP_PCH_IOCTL_HUC_DMA_TYPE     = 2
} PAVP_PCH_IOCTL_DMA_TYPE;


// ASMF Bit in PAVPC Register is Bit 6
#define PAVPC_ASMF_BIT_MASK    0x40
#define TRANSCODE_APP_ID_MASK  0x80

#endif  // __CP_UMD_DEFS_H__.
