/*
* Copyright (c) 2022, Intel Corporation
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
//! \file      ddi_cp_utils.h
//! \brief     utils for ddi cp
//!

#ifndef  __DDI_CP_UTILS_H__
#define  __DDI_CP_UTILS_H__

#include "va_protected_content_private.h"
#include "intel_pavp_types.h"
#include "intel_pavp_core_api.h"

// TODO update media_libva_util.h to media_libva_util_next.h when media_libva_util_next.h open source done
#include "media_libva_util.h"
#define DDI_CP_FUNC_ENTER                                                   \
    MOS_FUNCTION_TRACE(MOS_COMPONENT_DDI, MOS_SUBCOMP_CP)
#define DDI_CP_ASSERTMESSAGE(_message, ...)                                 \
    MOS_ASSERTMESSAGE(MOS_COMPONENT_DDI, MOS_SUBCOMP_CP, _message, ##__VA_ARGS__)
// TODO end 

enum ENCRYPTION_PARAMS_INDEX
{
    DECODE_ENCRYPTION_PARAMS_INDEX      = 0,
    ENCODE_ENCRYPTION_PARAMS_INDEX      = DECODE_ENCRYPTION_PARAMS_INDEX,
    CENC_ENCRYPTION_PARAMS_INDEX        = 1,
    NUM_ENCRYPTION_PARAMS_INDEX
};

struct DDI_CP_CONFIG_ATTR
{
    VAProfile           profile;
    VAEntrypoint        entrypoint;
    uint32_t            uiSessionMode;
    uint32_t            uiSessionType;
    uint32_t            uiCipherAlgo;
    uint32_t            uiCipherBlockSize;
    uint32_t            uiCipherMode;
    uint32_t            uiCipherSampleType;
    uint32_t            uiUsage;
};
typedef struct DDI_CP_CONFIG_ATTR *PDDI_CP_CONFIG_ATTR;

class DdiCpUtils
{
private:

public:
    DdiCpUtils(){};
    ~DdiCpUtils(){};

    static PAVP_ENCRYPTION_TYPE _GetEncryptionType(
        uint32_t uiCipherBlockSize,
        uint32_t uiCipherMode)
    {
        DDI_CP_FUNC_ENTER;

        if (uiCipherBlockSize != VA_PC_BLOCK_SIZE_128)
        {
            return PAVP_ENCRYPTION_NONE;
        }

        switch (uiCipherMode)
        {
        case VA_PC_CIPHER_MODE_CBC:
            return PAVP_ENCRYPTION_AES128_CBC;
        case VA_PC_CIPHER_MODE_CTR:
            return PAVP_ENCRYPTION_AES128_CTR;
        default:
            DDI_CP_ASSERTMESSAGE("Unsupported cipher mode %d", uiCipherMode);
            MT_ERR1(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__);
        }

        return PAVP_ENCRYPTION_NONE;
    }

    static PAVP_SESSION_MODE _GetSessionMode(uint32_t uiSessionMode)
    {
        DDI_CP_FUNC_ENTER;

        switch (uiSessionMode)
        {
        case VA_PC_SESSION_MODE_NONE:
        case VA_PC_SESSION_MODE_LITE:
            return PAVP_SESSION_MODE_LITE;
        case VA_PC_SESSION_MODE_HEAVY:
            return PAVP_SESSION_MODE_HEAVY;
        case VA_PC_SESSION_MODE_STOUT:
            return PAVP_SESSION_MODE_STOUT;
        default:
            DDI_CP_ASSERTMESSAGE("Unsupported session mode %d", uiSessionMode);
            MT_ERR1(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__);
            break;
        }

        return PAVP_SESSION_MODE_INVALID;
    }

    static PAVP_SESSION_TYPE _GetSessionType(uint32_t uiSessionType)
    {
        DDI_CP_FUNC_ENTER;

        switch (uiSessionType)
        {
        case VA_PC_SESSION_TYPE_NONE:
        case VA_PC_SESSION_TYPE_DISPLAY:
            return PAVP_SESSION_TYPE_DECODE;
        case VA_PC_SESSION_TYPE_TRANSCODE:
            return PAVP_SESSION_TYPE_TRANSCODE;
        default:
            DDI_CP_ASSERTMESSAGE("Unsupported session type %d", uiSessionType);
            MT_ERR1(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__);
            break;
        }

        return PAVP_SESSION_TYPE_INVALID;
    }

    static PAVP_CRYPTO_SESSION_TYPE _GetCryptoSessionType(
        VAEntrypoint entrypoint,
        uint32_t     uiSessionMode,
        uint32_t     uiSessionType,
        uint32_t     uiUsage)
    {
        DDI_CP_FUNC_ENTER;

        PAVP_CRYPTO_SESSION_TYPE crypto_session_type = PAVP_CRYPTO_SESSION_TYPE_NONE;

        do
        {
            if (entrypoint == VAEntrypointProtectedTEEComm)
            {
                crypto_session_type = PAVP_CRYPTO_SESSION_TYPE_PAVP_MECOMM;
                break;
            }

            if (entrypoint != (VAEntrypoint)VAEntrypointProtectedContent)
            {
                crypto_session_type = PAVP_CRYPTO_SESSION_TYPE_NONE;
                break;
            }

            switch (uiUsage)
            {
#if (_DEBUG || _RELEASE_INTERNAL)
            case VA_PC_USAGE_PASS_WIDI:
                crypto_session_type = PAVP_CRYPTO_SESSION_TYPE_WIDI_MECOMM_FW_APP;
                break;
            case VA_PC_USAGE_PASS_PAVP:
                crypto_session_type = PAVP_CRYPTO_SESSION_TYPE_PAVP_MECOMM_FW_APP;
                break;
#endif
            case VA_PC_USAGE_WIDEVINE:
            case VA_PC_USAGE_PLAYREADY:
            case VA_PC_USAGE_MIRACAST_RX:
            case VA_PC_USAGE_MIRACAST_TX:
            case VA_PC_USAGE_SECURE_COMPUTE:
                crypto_session_type = PAVP_CRYPTO_SESSION_TYPE_NONE;
                break;
            case VA_PC_USAGE_DEFAULT:
            case VA_PC_USAGE_WIDI_STREAM:
                crypto_session_type = PAVP_CRYPTO_SESSION_TYPE_PAVP_VIDEO;
                break;
            default:
                DDI_CP_ASSERTMESSAGE("Unsupported session type %d", uiSessionType);
                MT_ERR1(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__);
            }

        } while (false);

        return crypto_session_type;
    }

    static PAVP_DRM_TYPE _GetDrmType(
        uint32_t usage)
    {
        DDI_CP_FUNC_ENTER;
        
        switch (usage)
        {
        case VA_PC_USAGE_WIDEVINE:
            return PAVP_DRM_TYPE_WIDEVINE;
        case VA_PC_USAGE_PLAYREADY:
            return PAVP_DRM_TYPE_PLAYREADY;
        case VA_PC_USAGE_MIRACAST_RX:
            return PAVP_DRM_TYPE_MIRACAST_RX;
        case VA_PC_USAGE_MIRACAST_TX:
            return PAVP_DRM_TYPE_MIRACAST_TX;
        case VA_PC_USAGE_FAIRPLAY:
            return PAVP_DRM_TYPE_FAIRPLAY;
        case VA_PC_USAGE_SECURE_COMPUTE:
            return PAVP_DRM_TYPE_SECURE_COMPUTE;
        default:
            break;
        }

        return PAVP_DRM_TYPE_NONE;
    }

    MEDIA_CLASS_DEFINE_END(DdiCpUtils)
};

#endif // __DDI_CP_UTILS_H__