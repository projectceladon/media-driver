/*
//               INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2010-2017 Intel Corporation. All Rights Reserved.
//
*/
#ifndef _INTEL_PAVP_TYPES_H
#define _INTEL_PAVP_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    // Available encryption types
    PAVP_ENCRYPTION_NONE            = 1,
    PAVP_ENCRYPTION_AES128_CTR      = 2,
    PAVP_ENCRYPTION_AES128_CBC      = 4,
    PAVP_ENCRYPTION_AES128_ECB      = 8,
    PAVP_ENCRYPTION_DECE_AES128_CTR = 16
} PAVP_ENCRYPTION_TYPE;

typedef enum 
{
    // Available counter types
    PAVP_COUNTER_TYPE_NONE = 0,
    //PAVP_COUNTER_TYPE_A    = 1,  // 32-bit counter, 96 zeroes (CTG, ELK, ILK, SNB)
    //PAVP_COUNTER_TYPE_B    = 2,  // 64-bit counter, 32-bit Nonce, 32-bit zero (SNB)
    PAVP_COUNTER_TYPE_C    = 4   // Standard AES counter
} PAVP_COUNTER_TYPE;

typedef enum
{
    // Available endianness types
    PAVP_LITTLE_ENDIAN  = 1,
    PAVP_BIG_ENDIAN     = 2,
} PAVP_ENDIANNESS_TYPE;

typedef enum
{
    // Available crypto session types
    PAVP_CRYPTO_SESSION_TYPE_NONE        = 0,
    PAVP_CRYPTO_SESSION_TYPE_PAVP_MECOMM,
    PAVP_CRYPTO_SESSION_TYPE_PAVP_VIDEO,
    PAVP_CRYPTO_SESSION_TYPE_PAVP_AUDIO
#if (_DEBUG || _RELEASE_INTERNAL)
    ,PAVP_CRYPTO_SESSION_TYPE_PAVP_MECOMM_FW_APP
    ,PAVP_CRYPTO_SESSION_TYPE_WIDI_MECOMM_FW_APP
    ,PAVP_CRYPTO_SESSION_TYPE_MKHI_MECOMM_FW_APP
    ,PAVP_CRYPTO_SESSION_TYPE_GSC_PROXY_MECOMM_FW_APP
#endif
} PAVP_CRYPTO_SESSION_TYPE;

typedef enum
{
    // Available loosley speaking DRM types
    PAVP_DRM_TYPE_NONE,
    PAVP_DRM_TYPE_LEGACY,
    PAVP_DRM_TYPE_BLURAY,
    PAVP_DRM_TYPE_MEDIA_VAULT,
    PAVP_DRM_TYPE_WIDEVINE,
    PAVP_DRM_TYPE_PLAYREADY,
    PAVP_DRM_TYPE_MIRACAST_RX,
    PAVP_DRM_TYPE_INDIRECT_DISPLAY,
    PAVP_DRM_TYPE_MIRACAST_TX,
    PAVP_DRM_TYPE_FAIRPLAY,
    PAVP_DRM_TYPE_SECURE_COMPUTE,
    PAVP_DRM_TYPE_INVALID           = 0xFF
} PAVP_DRM_TYPE;

#ifdef __cplusplus
}
#endif

#endif // _INTEL_PAVP_TYPES_H
