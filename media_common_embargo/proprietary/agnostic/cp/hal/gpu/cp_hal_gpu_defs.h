/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2011-2017
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

File Name: cp_hal_gpu_defs.h

Abstract:
Defintions / macros for HAL layer

Environment:
Platform Agnostic - should support - Gen 6, Gen 7, Gen 7.5

======================= end_copyright_notice ==================================*/

/// \file cp_hal_gpu_defs.h 
/// Defintions / macros for HAL layer

#ifndef __CP_HAL_GPU_DEFS_H__
#define __CP_HAL_GPU_DEFS_H__

#include "cp_umd_defs.h"
#include "mhw_cp.h"
#include "mhw_cp_hwcmd_common.h"

#define DWORD_TO_BYTE(x) ((x)*sizeof(DWORD))

#define PAVP_MAX_KEY_EXCHANGE_SPACE                 256
#define PAVP_MAX_QUERY_STATUS_SPACE                 256
#define PAVP_MAX_CRYPTO_COPY_SPACE                  256
#define PAVP_INVALID_PATCH_OFFSET                   9999    // this value must be greater than any of the
                                                            // above three values - If CmdPatchOffset returns
                                                            // this value then AddPatchEntry will error out

#define PAVP_SESSION_SLOT_FROM_ID(x)                (((x) & 0x7f))

#define PAVP_CRYPTO_COPY_SRC_ALLOC_OFFSET                   0
#define PAVP_CRYPTO_COPY_DST_ALLOC_OFFSET                   0
#define PAVP_CRYPTO_COPY_SRC_PATCH_OFFSET                   sizeof(DWORD)
#define PAVP_CRYPTO_COPY_SRC_48BIT_PATCH_OFFSET             sizeof(DWORD)
#define PAVP_CRYPTO_COPY_DST_PATCH_OFFSET                   (3*sizeof(DWORD))
#define PAVP_CRYPTO_COPY_DST_48BIT_PATCH_OFFSET             (5*sizeof(DWORD))

#define PAVP_QUERY_STATUS_OUT_DATA_OFFSET                   0
#define PAVP_QUERY_STATUS_STORE_DWORD_OFFSET                32
#define PAVP_QUERY_STATUS_OUT_DATA_PATCH_OFFSET             (2*sizeof(DWORD))
#define PAVP_QUERY_STATUS_OUT_DATA_48BIT_PATCH_OFFSET       (2*sizeof(DWORD))
#define PAVP_QUERY_STATUS_STORE_DWORD_PATCH_OFFSET          (4*sizeof(DWORD))
#define PAVP_QUERY_STATUS_STORE_DWORD_48BIT_PATCH_OFFSET    (5*sizeof(DWORD))
#define PAVP_QUERY_STATUS_CS_OUT_DATA_SIZE                  32
#define PAVP_QUERY_STATUS_KF_OUT_DATA_SIZE                  16
#define PAVP_QUERY_STATUS_MS_OUT_DATA_SIZE                  16
#define PAVP_QUERY_STATUS_STDW_OUT_DATA_SIZE                8
#define PAVP_QUERY_STATUS_MAX_OUT_DATA_SIZE                 (PAVP_QUERY_STATUS_CS_OUT_DATA_SIZE + PAVP_QUERY_STATUS_STDW_OUT_DATA_SIZE)
#define PAVP_QUERY_STATUS_TIMEOUT                           60    // 60 milliseconds, this has to do with hardware softreset time being 60 ms
#define PAVP_QUERY_STATUS_MAX_DWORDS                        8
#define PAVP_QUERY_STATUS_BUFFER_INITIAL_VALUE              0xf

#define PAVP_FLUSH_DW_PATCH_OFFSET                          sizeof(DWORD)
#define PAVP_FLUSH_DW_48BIT_PATCH_OFFSET                    sizeof(DWORD)

// PCH key mode, PAVP APPID reg(0x320FC) bit26-28
#define PAVP_APPID_INJECT_KEY_MODE_SHIFT                    26

typedef enum tagPAVP_PCH_INJECT_KEY_MODE
{
    PAVP_PCH_INJECT_KEY_MODE_S1                 = 0, //!< S1 key only
    PAVP_PCH_INJECT_KEY_MODE_S1_FR              = 1, //!< S1 key + Secure App characteristics
    PAVP_PCH_INJECT_KEY_MODE_S1_FR_KR           = 2, //!< S1 key + Secure PAVP Func Register + Secure PAVP Key Register
} PAVP_PCH_INJECT_KEY_MODE;

typedef enum tagPAVP_KEY_EXCHANGE_TYPE
{
    PAVP_KEY_EXCHANGE_TYPE_DECRYPT              = 0, //!< Update Sn_d
    PAVP_KEY_EXCHANGE_TYPE_ENCRYPT              = 1, //!< Update Sn_e
    PAVP_KEY_EXCHANGE_TYPE_DECRYPT_ENCRYPT      = 2, //!< Update both Sn_d and Sn_e
    PAVP_KEY_EXCHANGE_TYPE_FIXED                = 4, //!< Reset to a new S1 (for fixed key exchange) - deprecated.
    PAVP_KEY_EXCHANGE_TYPE_DECRYPT_FRESHNESS    = 8, //!< Set decrypt freshness value
    PAVP_KEY_EXCHANGE_TYPE_ENCRYPT_FRESHNESS    = 16,//!< Set encrypt freshness value
    PAVP_KEY_EXCHANGE_TYPE_TERMINATE            = 32,//!< End session
    PAVP_KEY_EXCHANGE_TYPE_KEY_ROTATION         = 64 //!< Set decrypt key rotation
} PAVP_KEY_EXCHANGE_TYPE;

typedef enum tagPAVP_QUERY_STATUS_OPERATION
{
    PAVP_QUERY_STATUS_MEMORY_MODE,          //!< Lite or Serpent
    PAVP_QUERY_STATUS_CONNECTION_STATE,     //!< 32 bytes of data for Gen7
    PAVP_QUERY_STATUS_FRESHNESS,            //!< Get freshness value
    PAVP_QUERY_STATUS_WIDI_COUNTER          //!< Get ((riv xor streamCtr) || inputCtr), HDCP2 IV (also known as "p")
} PAVP_QUERY_STATUS_OPERATION;

typedef struct tagPAVP_MI_SET_APP_ID_CMD_PARAMS
{
    DWORD   dwAppId;
} PAVP_MI_SET_APP_ID_CMD_PARAMS;

typedef struct tagPAVP_MFX_CRYPTO_COPY_BASE_ADDR_CMD_PARAMS
{
    DWORD   dwSrcAddr;
    DWORD   dwDstAddr;
} PAVP_MFX_CRYPTO_COPY_BASE_ADDR_CMD_PARAMS;

typedef struct tagPAVP_MFX_CRYPTO_COPY_CMD_PARAMS
{
    DWORD                   dwSize;
    DWORD                   dwAesCtr[4];
    MHW_PAVP_CRYPTO_COPY_TYPE eCryptoCopyType;

    // Partial Decryption parameters
    uint32_t uiEncryptedBytes;
    uint32_t uiClearBytes;
} PAVP_MFX_CRYPTO_COPY_CMD_PARAMS;

typedef struct tagPAVP_MFX_CRYPTO_KEY_EXCHANGE_CMD_PARAMS
{
    CRYPTO_KEY_EXCHANGE_KEY_USE         KeyExchangeKeyUse;
    CRYPTO_KEY_EXCHANGE_DECRYPT_ENCRYPT KeyExchangeKeySelect;
    DWORD                               EncryptedDecryptionKey[4];  //!< Encrypted decrypt key for set decrypt key or set both keys
    DWORD                               EncryptedEncryptionKey[4];  //!< Encrypted encrypt key for set encrypt key or set both keys
} PAVP_MFX_CRYPTO_KEY_EXCHANGE_CMD_PARAMS;

typedef struct tagPAVP_MFX_CRYPTO_INLINE_STATUS_READ_CMD_PARAMS
{
    CRYPTO_INLINE_STATUS_READ_TYPE StatusReadType;
    union
    {
        DWORD   NonceIn;                   //!< Nonce value that is expected to be read back in connection state
        DWORD   KeyRefreshType;            //!< Encrypt(1) or decrypt(0) key freshness
    } Value;
} PAVP_MFX_CRYPTO_INLINE_STATUS_READ_CMD_PARAMS;
///@}

/// \defgroup PublicFn Parameters used by public functions of CPavpGpuHal
/// @{
typedef struct tagPAVP_GPU_HAL_KEY_EXCHANGE_PARAMS
{
    DWORD                   AppId;
    PAVP_KEY_EXCHANGE_TYPE  KeyExchangeType;
    DWORD                   EncryptedDecryptionKey[4];  //!< Encrypted decrypt key for set decrypt key or set both keys
    union {
        DWORD               EncryptedEncryptionKey[4];  //!< Encrypted encrypt key for set encrypt key or set both keys
        DWORD               EncryptedDecryptionRotationKey[4];  //!< Encrypted decrypt key for rotating set decrypt key
    };
} PAVP_GPU_HAL_KEY_EXCHANGE_PARAMS;

typedef struct tagPAVP_GPU_HAL_QUERY_STATUS_PARAMS
{        
    DWORD                       AppId;
    DWORD                       AllocationIndex;
    PAVP_QUERY_STATUS_OPERATION QueryOperation;
    union
    {
        DWORD   NonceIn;                                //!< Nonce value that is expected to be read back in connection state
        DWORD   KeyRefreshType;                         //!< Encrypt or decrypt key freshness
    } Value;
    DWORD       OutData[PAVP_QUERY_STATUS_MAX_DWORDS];  //!< Connection state, memory status, or freshness value
} PAVP_GPU_HAL_QUERY_STATUS_PARAMS;

typedef struct tagPAVP_GPU_HAL_CRYPTO_COPY_PARAMS
{
    MHW_PAVP_CRYPTO_COPY_TYPE   eCryptoCopyType;
    DWORD                   dwAppId;
    DWORD                   dwDataSize;
    uint32_t*               pHwctr;

    union
    {
        // AES decrypt paramters
        struct
        {
            PMOS_RESOURCE       SrcResource;
            PMOS_RESOURCE       DstResource;
            DWORD               dwAesCtr[4];

            // Partial Decryption parameters
            uint32_t                uiEncryptedBytes;
            uint32_t                uiClearBytes;

            // AES mode (CTR or CBC)
            MHW_PAVP_CRYPTO_COPY_AES_MODE eAesMode;
        };

        // Set plane enables / write CB2 parameters
        PBYTE               pSrcData;
    };
} PAVP_GPU_HAL_CRYPTO_COPY_PARAMS;

typedef struct tagPAVP_GPU_HAL_MULTI_CRYPTO_COPY_PARAMS
{
    MHW_PAVP_CRYPTO_COPY_TYPE eCryptoCopyType;
    uint32_t                  dwAppId;
    uint32_t                  dwDataSize;
    uint32_t                  offset;
    PMOS_RESOURCE             SrcResource;
    PMOS_RESOURCE             DstResource;
    uint32_t                  dwAesCtr[4];

   // Partial Decryption parameters
    uint32_t uiEncryptedBytes;
    uint32_t uiClearBytes;

   // AES mode (CTR or CBC)
   MHW_PAVP_CRYPTO_COPY_AES_MODE eAesMode;
} PAVP_GPU_HAL_MULTI_CRYPTO_COPY_PARAMS;
/// @}
#endif // __CP_HAL_GPU_DEFS_H__
