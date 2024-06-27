/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2017-2020
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
//! \file     media_libva_caps_cp.cpp
//! \brief    This file defines the C++ class/interface for encryption related capbilities.
//!

#include "media_libva_caps_cp.h"
#include "media_libva_caps.h"
#include "media_libva_util.h"
#include "va_protected_content_private.h"

extern "C" MediaLibvaCapsCpInterface* Create_MediaLibvaCapsCp(DDI_MEDIA_CONTEXT *mediaCtx, MediaLibvaCaps *mediaCaps)
{
    return MOS_New(MediaLibvaCapsCp, mediaCtx, mediaCaps);
}

extern "C" void Delete_MediaLibvaCapsCp(MediaLibvaCapsCpInterface* pMediaLibvaCapsCpInterface)
{
    MOS_Delete(pMediaLibvaCapsCpInterface);
}

MediaLibvaCapsCp::MediaLibvaCapsCp(DDI_MEDIA_CONTEXT *mediaCtx, MediaLibvaCaps *mediaCaps)
    : MediaLibvaCapsCpInterface(mediaCtx, mediaCaps)
{

}

MediaLibvaCapsCp::~MediaLibvaCapsCp()
{
    FreeAttributeList();
}

bool MediaLibvaCapsCp::IsDecEncryptionSupported(DDI_MEDIA_CONTEXT *mediaCtx)
{
    if (MEDIA_IS_SKU(&(mediaCtx->SkuTable), FtrEnableMediaKernels) &&
            (!MEDIA_IS_WA(&(mediaCtx->WaTable), WaDisableHuCBasedDRM)))
    {
        return true;
    }
    else
    {
        return false;
    }
}

int32_t MediaLibvaCapsCp::GetEncryptionTypes(VAProfile profile, uint32_t *encryptionType, uint32_t arraySize)
{
    int32_t numTypes;

    if (MediaLibvaCaps::IsAvcProfile(profile))
    {
        numTypes = 3;

        if (arraySize < numTypes)
        {
            return -1;
        }

        encryptionType[0] = VA_ENCRYPTION_TYPE_FULLSAMPLE_CTR;  // ctr fullsample
        encryptionType[1] = VA_ENCRYPTION_TYPE_SUBSAMPLE_CTR;   // ctr subsample
        encryptionType[2] = VA_ENCRYPTION_TYPE_SUBSAMPLE_CBC;   // cbc subsample
        return numTypes;
    }
    else if(
            (profile == VAProfileHEVCMain)       ||
            (profile == VAProfileHEVCMain10)     ||
            (profile == VAProfileHEVCMain12)     ||
            (profile == VAProfileHEVCMain422_10) ||
            (profile == VAProfileHEVCMain422_12) ||
            (profile == VAProfileHEVCMain444)    ||
            (profile == VAProfileHEVCMain444_10) ||
            (profile == VAProfileHEVCMain444_12) ||
            (profile == VAProfileHEVCSccMain)    ||
            (profile == VAProfileHEVCSccMain10)  ||
            (profile == VAProfileHEVCSccMain444)
            )
    {
        numTypes = 3;

        if (arraySize < numTypes)
        {
            return -1;
        }

        encryptionType[0] = VA_ENCRYPTION_TYPE_FULLSAMPLE_CTR;  // ctr fullsample
        encryptionType[1] = VA_ENCRYPTION_TYPE_SUBSAMPLE_CTR;   // ctr subsample
        encryptionType[2] = VA_ENCRYPTION_TYPE_SUBSAMPLE_CBC;   // cbc subsample
        return numTypes;
    }
    else if (MediaLibvaCaps::IsVp9Profile(profile))
    {
        numTypes = 2;

        if (arraySize < numTypes)
        {
            return -1;
        }

        encryptionType[0] = VA_ENCRYPTION_TYPE_SUBSAMPLE_CTR;   // ctr subsample
        encryptionType[1] = VA_ENCRYPTION_TYPE_SUBSAMPLE_CBC;   // cbc subsample
        return numTypes;
    }
    else if (profile == VAProfileAV1Profile0)
    {
        numTypes = 2;

        if (arraySize < numTypes)
        {
            return -1;
        }

        encryptionType[0] = VA_ENCRYPTION_TYPE_SUBSAMPLE_CTR;   // ctr subsample
        encryptionType[1] = VA_ENCRYPTION_TYPE_SUBSAMPLE_CBC;   // cbc subsample
        return numTypes;
    }
    else
    {
        return -1;
    }
}

const uint32_t MediaLibvaCapsCp::m_arrCpSessionMode[] =
{
    VA_PC_SESSION_MODE_LITE,
    VA_PC_SESSION_MODE_HEAVY,
    VA_PC_SESSION_MODE_STOUT,
};

const uint32_t MediaLibvaCapsCp::m_arrCpSessionType[] =
{
    VA_PC_SESSION_TYPE_DISPLAY,
    VA_PC_SESSION_TYPE_TRANSCODE,
};

const uint32_t MediaLibvaCapsCp::m_arrCipherMode[] =
{
    VA_PC_CIPHER_MODE_CBC,
    VA_PC_CIPHER_MODE_CTR,
};

const uint32_t MediaLibvaCapsCp::m_arrCipherSampleType[] =
{
    VA_PC_SAMPLE_TYPE_FULLSAMPLE,
    VA_PC_SAMPLE_TYPE_SUBSAMPLE,
};

const uint32_t MediaLibvaCapsCp::m_arrCpUsage[] =
{
    VA_PC_USAGE_DEFAULT,
    VA_PC_USAGE_SECURE_COMPUTE,
};

VAStatus MediaLibvaCapsCp::LoadCpProfileEntrypoints()
{
    VAStatus status = VA_STATUS_SUCCESS;
    AttribMap *attributeList = nullptr;
    uint32_t configStartIdx = 0, configNum = 0;
    uint32_t ftr_stout = 0;
    uint32_t ftr_stout_transcode = 0;

    DDI_CHK_NULL(m_mediaCtx, "nullptr m_mediaCtx", VA_STATUS_ERROR_OPERATION_FAILED);

#ifdef VA_DRIVER_VTABLE_PROT_VERSION
    if (MEDIA_IS_SKU(&(m_mediaCtx->SkuTable), FtrPAVP))
    {
        VAProfile profile = VAProfileProtected;

        ftr_stout = MEDIA_IS_SKU(&(m_mediaCtx->SkuTable), FtrPAVPStout);
        ftr_stout_transcode = MEDIA_IS_SKU(&(m_mediaCtx->SkuTable), FtrPAVPStoutTranscode);

        status = CreateCpAttributes(profile, (VAEntrypoint)VAEntrypointProtectedContent, &attributeList);
        DDI_CHK_RET(status, "Failed to initialize Caps!");

        configStartIdx = m_cpConfigs.size();

        // Add Widevine config
        for (int32_t i = 0; i < sizeof(m_arrCipherMode)/sizeof(m_arrCipherMode[0]); i++)
        {
            for (int32_t j = 0; j < sizeof(m_arrCipherSampleType)/sizeof(m_arrCipherSampleType[0]); j++)
            {
                AddCpConfig(
                    VA_PC_SESSION_MODE_NONE,
                    VA_PC_SESSION_TYPE_NONE,
                    VA_PC_CIPHER_AES,
                    VA_PC_BLOCK_SIZE_128,
                    m_arrCipherMode[i],
                    m_arrCipherSampleType[j],
                    VA_PC_USAGE_WIDEVINE);
            }
        }

        // other configs
        for (int32_t i1 = 0; i1 < sizeof(m_arrCpSessionMode)/sizeof(m_arrCpSessionMode[0]); i1++)
        {
            // check if stout mode is supported or not
            if (!ftr_stout && (m_arrCpSessionMode[i1] == VA_PC_SESSION_MODE_STOUT))
            {
                continue;
            }

            for (int32_t i2 = 0; i2 < sizeof(m_arrCpSessionType)/sizeof(m_arrCpSessionType[0]); i2++)
            {
                // check if stout transcode is supported or not
                if (!ftr_stout_transcode &&
                    (m_arrCpSessionMode[i1] == VA_PC_SESSION_MODE_STOUT) &&
                    (m_arrCpSessionType[i2] == VA_PC_SESSION_TYPE_TRANSCODE))
                {
                    continue;
                }

                for (int32_t i3 = 0; i3 < sizeof(m_arrCipherMode)/sizeof(m_arrCipherMode[0]); i3++)
                {
                    for (int32_t i4 = 0; i4 < sizeof(m_arrCipherSampleType)/sizeof(m_arrCipherSampleType[0]); i4++)
                    {
                        if((m_arrCpSessionMode[i1] == VA_PC_SESSION_MODE_STOUT) &&
                           (m_arrCpSessionType[i2] == VA_PC_SESSION_TYPE_TRANSCODE))
                        {
                            for (int32_t i5 = 0; i5 < sizeof(m_arrCpUsage)/sizeof(m_arrCpUsage[0]); i5++)
                            {
                                AddCpConfig(
                                    m_arrCpSessionMode[i1],
                                    m_arrCpSessionType[i2],
                                    VA_PC_CIPHER_AES,
                                    VA_PC_BLOCK_SIZE_128,
                                    m_arrCipherMode[i3],
                                    m_arrCipherSampleType[i4],
                                    m_arrCpUsage[i5]);
                            }
                        } else {
                            AddCpConfig(
                                m_arrCpSessionMode[i1],
                                m_arrCpSessionType[i2],
                                VA_PC_CIPHER_AES,
                                VA_PC_BLOCK_SIZE_128,
                                m_arrCipherMode[i3],
                                m_arrCipherSampleType[i4],
                                m_arrCpUsage[0]);
                        }
                    }
                }
            }
        }

        configNum = m_cpConfigs.size() - configStartIdx;
        status = AddProfileEntry(profile, (VAEntrypoint)VAEntrypointProtectedContent, attributeList, configStartIdx, configNum);
        DDI_CHK_RET(status, "Failed to AddProfileEntry of VAEntrypointProtectedContent!");

        // Add VAEntrypointProtectedTEEComm
        attributeList = nullptr;
        status = CreateCpAttributes(profile, VAEntrypointProtectedTEEComm, &attributeList);
        DDI_CHK_RET(status, "Failed to initialize Caps!");

        configStartIdx = m_cpConfigs.size();
        AddCpConfig(
            VA_PC_SESSION_MODE_NONE,
            VA_PC_SESSION_TYPE_NONE);
        configNum = m_cpConfigs.size() - configStartIdx;

        status = AddProfileEntry(profile, VAEntrypointProtectedTEEComm, attributeList, configStartIdx, configNum);
        DDI_CHK_RET(status, "Failed to AddProfileEntry of VAEntrypointProtectedContent!");
    }
#endif

    return status;
}

bool MediaLibvaCapsCp::IsCpConfigId(VAConfigID configId)
{
    return ((configId >= DDI_CP_GEN_CONFIG_ATTRIBUTES_BASE) &&
            (configId < (DDI_CP_GEN_CONFIG_ATTRIBUTES_BASE + m_cpConfigs.size())));
}

VAStatus MediaLibvaCapsCp::CreateCpConfig(
        int32_t profileTableIdx,
        VAEntrypoint entrypoint,
        VAConfigAttrib *attribList,
        int32_t numAttribs,
        VAConfigID *configId)
{
    int32_t startIdx = 0;
    int32_t configNum = 0;
    int32_t j = 0;

#ifdef VA_DRIVER_VTABLE_PROT_VERSION
    DDI_CHK_CONDITION(profileTableIdx < 0,  "profileTableIdx < 0",  VA_STATUS_ERROR_INVALID_PARAMETER);
    DDI_CHK_NULL(GetProfileEntrypoint(profileTableIdx), "Null pointer", VA_STATUS_ERROR_INVALID_PARAMETER);

    startIdx = GetProfileEntrypoint(profileTableIdx)->m_configStartIdx;
    configNum = GetProfileEntrypoint(profileTableIdx)->m_configNum;

    DDI_CHK_CONDITION(startIdx < 0,         "startIdx < 0",         VA_STATUS_ERROR_OPERATION_FAILED);
    DDI_CHK_CONDITION(configNum < 0,        "configNum < 0",        VA_STATUS_ERROR_OPERATION_FAILED);
    DDI_CHK_NULL(configId,                  "Null pointer",         VA_STATUS_ERROR_INVALID_PARAMETER);

    if (numAttribs)
    {
        DDI_CHK_NULL(attribList, "Null pointer", VA_STATUS_ERROR_INVALID_PARAMETER);
    }

    uint32_t cpSessionMode = VA_PC_SESSION_MODE_NONE;
    uint32_t cpSessionType = VA_PC_SESSION_TYPE_NONE;
    uint32_t cipherAlgo = VA_PC_CIPHER_AES;
    uint32_t cipherBlockSize = VA_PC_BLOCK_SIZE_128;
    uint32_t cipherMode = VA_PC_CIPHER_MODE_CTR;
    uint32_t cipherSampleType = VA_PC_SAMPLE_TYPE_FULLSAMPLE;
    uint32_t cpUsage = VA_PC_USAGE_DEFAULT;

    if (entrypoint == VAEntrypointProtectedTEEComm)
    {
        cipherAlgo = 0;
        cipherBlockSize = 0;
        cipherMode = 0;
        cipherSampleType = 0;
        cpUsage = 0;

        for (j = 0; j < numAttribs; j++)
        {
            if (VAConfigAttribProtectedContentSessionMode == (int32_t)attribList[j].type)
            {
                cpSessionMode = attribList[j].value;
            }
            else
            if(VAConfigAttribProtectedContentSessionType == (int32_t)attribList[j].type)
            {
                cpSessionType = attribList[j].value;
            }
        }
    }
    else
    {
        for (j = 0; j < numAttribs; j++)
        {
            if (VAConfigAttribProtectedContentSessionMode == (int32_t)attribList[j].type)
            {
                cpSessionMode = attribList[j].value;
            }
            else
            if(VAConfigAttribProtectedContentSessionType == (int32_t)attribList[j].type)
            {
                cpSessionType = attribList[j].value;
            }
            else
            if(VAConfigAttribProtectedContentCipherAlgorithm == (int32_t)attribList[j].type)
            {
                cipherAlgo = attribList[j].value;
            }
            else
            if(VAConfigAttribProtectedContentCipherBlockSize == (int32_t)attribList[j].type)
            {
                cipherBlockSize = attribList[j].value;
            }
            else
            if(VAConfigAttribProtectedContentCipherMode == (int32_t)attribList[j].type)
            {
                cipherMode = attribList[j].value;
            }
            else
            if(VAConfigAttribProtectedContentCipherSampleType == (int32_t)attribList[j].type)
            {
                cipherSampleType = attribList[j].value;
            }
            else
            if(VAConfigAttribProtectedContentUsage == (int32_t)attribList[j].type)
            {
                cpUsage = attribList[j].value;
            }
        }
    }

    for (j = startIdx; j < (startIdx + configNum); j++)
    {
        if (m_cpConfigs[j].m_CpSessionMode == cpSessionMode &&
            m_cpConfigs[j].m_CpSessionType == cpSessionType &&
            m_cpConfigs[j].m_CipherAlgo == cipherAlgo &&
            m_cpConfigs[j].m_CipherBlockSize == cipherBlockSize &&
            m_cpConfigs[j].m_CipherMode == cipherMode &&
            m_cpConfigs[j].m_CipherSampleType == cipherSampleType &&
            m_cpConfigs[j].m_CpUsage == cpUsage )
        {
            break;
        }
    }
#endif //#ifdef VA_DRIVER_VTABLE_PROT_VERSION

    if (j < (configNum + startIdx))
    {
        *configId = j + DDI_CP_GEN_CONFIG_ATTRIBUTES_BASE;
        return VA_STATUS_SUCCESS;
    }
    else
    {
        *configId = 0xFFFFFFFF;
        return VA_STATUS_ERROR_ATTR_NOT_SUPPORTED;
    }
}

VAStatus MediaLibvaCapsCp::GetCpConfigAttr(
        VAConfigID configId,
        VAProfile *profile,
        VAEntrypoint *entrypoint,
            uint32_t *session_mode,
            uint32_t *session_type,
            uint32_t *cipher_algo,
            uint32_t *cipher_block_size,
            uint32_t *cipher_mode,
            uint32_t *cipher_sample_type,
            uint32_t *usage)
{
    DDI_CHK_NULL(profile, "Null pointer", VA_STATUS_ERROR_INVALID_PARAMETER);
    DDI_CHK_NULL(entrypoint, "Null pointer", VA_STATUS_ERROR_INVALID_PARAMETER);
    DDI_CHK_NULL(session_mode, "Null pointer", VA_STATUS_ERROR_INVALID_PARAMETER);
    DDI_CHK_NULL(session_type, "Null pointer", VA_STATUS_ERROR_INVALID_PARAMETER);
    DDI_CHK_NULL(cipher_algo, "Null pointer", VA_STATUS_ERROR_INVALID_PARAMETER);
    DDI_CHK_NULL(cipher_block_size, "Null pointer", VA_STATUS_ERROR_INVALID_PARAMETER);
    DDI_CHK_NULL(cipher_sample_type, "Null pointer", VA_STATUS_ERROR_INVALID_PARAMETER);
    DDI_CHK_NULL(usage, "Null pointer", VA_STATUS_ERROR_INVALID_PARAMETER);

    int32_t profileTableIdx = -1;
    int32_t configOffset = configId - DDI_CP_GEN_CONFIG_ATTRIBUTES_BASE;
    VAStatus status = GetProfileEntrypointFromConfigId(configId, profile, entrypoint, &profileTableIdx);
    DDI_CHK_RET(status, "Invalide config_id!");
    if (profileTableIdx < 0 || profileTableIdx >= GetProfileEntryCount())
    {
        return VA_STATUS_ERROR_INVALID_CONFIG;
    }

    DDI_CHK_NULL(GetProfileEntrypoint(profileTableIdx), "Null pointer", VA_STATUS_ERROR_INVALID_PARAMETER);
    int32_t configStart = GetProfileEntrypoint(profileTableIdx)->m_configStartIdx;
    int32_t configEnd = GetProfileEntrypoint(profileTableIdx)->m_configStartIdx
        + GetProfileEntrypoint(profileTableIdx)->m_configNum;

    if (configOffset < configStart || configOffset > configEnd)
    {
        return VA_STATUS_ERROR_INVALID_CONFIG;
    }

    if (session_mode)
    {
        *session_mode =  m_cpConfigs[configOffset].m_CpSessionMode;
    }

    if (session_type)
    {
        *session_type =  m_cpConfigs[configOffset].m_CpSessionType;
    }

    if (cipher_algo)
    {
        *cipher_algo = m_cpConfigs[configOffset].m_CipherAlgo;
    }

    if (cipher_block_size)
    {
        *cipher_block_size =  m_cpConfigs[configOffset].m_CipherBlockSize;
    }

    if (cipher_mode)
    {
        *cipher_mode =  m_cpConfigs[configOffset].m_CipherMode;
    }

    if (cipher_sample_type)
    {
        *cipher_sample_type =  m_cpConfigs[configOffset].m_CipherSampleType;
    }

    if (usage)
    {
        *usage =  m_cpConfigs[configOffset].m_CpUsage;
    }

    return VA_STATUS_SUCCESS;
}

VAStatus MediaLibvaCapsCp::AddCpConfig(
    uint32_t cpSessionMode,
    uint32_t cpSessionType,
    uint32_t cipherAlgo,
    uint32_t cipherBlockSize,
    uint32_t cipherMode,
    uint32_t cipherSampleType,
    uint32_t cpUsage)
{

    m_cpConfigs.emplace_back(cpSessionMode, cpSessionType, cipherAlgo, cipherBlockSize, cipherMode, cipherSampleType, cpUsage);
    return VA_STATUS_SUCCESS;
}

VAStatus MediaLibvaCapsCp::CreateAttributeList(AttribMap **attributeList)
{
    DDI_CHK_NULL(attributeList, "Null pointer", VA_STATUS_ERROR_INVALID_PARAMETER);

    *attributeList = MOS_New(AttribMap);
    DDI_CHK_NULL(*attributeList, "Null pointer", VA_STATUS_ERROR_ALLOCATION_FAILED);
    m_attributeLists.push_back(*attributeList);

    return VA_STATUS_SUCCESS;
}

VAStatus MediaLibvaCapsCp::FreeAttributeList()
{
    uint32_t attribListCount = m_attributeLists.size();
    for (uint32_t i = 0; i < attribListCount; i++)
    {
        m_attributeLists[i]->clear();
        MOS_Delete(m_attributeLists[i]);
        m_attributeLists[i] = nullptr;
    }
    m_attributeLists.clear();
    return VA_STATUS_SUCCESS;
}

VAStatus MediaLibvaCapsCp::CreateCpAttributes(
    VAProfile profile,
    VAEntrypoint entrypoint,
    AttribMap **attributeList)
{
    DDI_CHK_NULL(attributeList, "Null pointer", VA_STATUS_ERROR_INVALID_PARAMETER);
    DDI_CHK_NULL(m_mediaCtx, "nullptr m_mediaCtx", VA_STATUS_ERROR_OPERATION_FAILED);

#ifdef VA_DRIVER_VTABLE_PROT_VERSION
    VAStatus status = CreateAttributeList(attributeList);
    DDI_CHK_RET(status, "Failed to initialize Caps!");

    auto attribList = *attributeList;
    DDI_CHK_NULL(attribList, "Null pointer", VA_STATUS_ERROR_INVALID_PARAMETER);

    VAConfigAttrib attrib;

    if (entrypoint == VAEntrypointProtectedContent)
    {
        attrib.type = (VAConfigAttribType) VAConfigAttribProtectedContentSessionMode;
        attrib.value = VA_PC_SESSION_MODE_LITE |
                       VA_PC_SESSION_MODE_HEAVY;
        if (MEDIA_IS_SKU(&(m_mediaCtx->SkuTable), FtrPAVPStout))
        {
            attrib.value |= VA_PC_SESSION_MODE_STOUT;
        }
        (*attribList)[attrib.type] = attrib.value;

        attrib.type = (VAConfigAttribType) VAConfigAttribProtectedContentSessionType;
        attrib.value = VA_PC_SESSION_TYPE_DISPLAY |
                       VA_PC_SESSION_TYPE_TRANSCODE;
        (*attribList)[attrib.type] = attrib.value;

        attrib.type = (VAConfigAttribType) VAConfigAttribProtectedContentCipherAlgorithm;
        attrib.value = VA_PC_CIPHER_AES;
        (*attribList)[attrib.type] = attrib.value;

        attrib.type = (VAConfigAttribType) VAConfigAttribProtectedContentCipherBlockSize;
        attrib.value = VA_PC_BLOCK_SIZE_128;
        (*attribList)[attrib.type] = attrib.value;

        attrib.type = (VAConfigAttribType) VAConfigAttribProtectedContentCipherMode;
        attrib.value = VA_PC_CIPHER_MODE_CBC |
                       VA_PC_CIPHER_MODE_CTR;
        (*attribList)[attrib.type] = attrib.value;

        attrib.type = (VAConfigAttribType) VAConfigAttribProtectedContentCipherSampleType;
        attrib.value = VA_PC_SAMPLE_TYPE_FULLSAMPLE |
                       VA_PC_SAMPLE_TYPE_SUBSAMPLE;
        (*attribList)[attrib.type] = attrib.value;

        attrib.type = (VAConfigAttribType) VAConfigAttribProtectedContentUsage;
        attrib.value = VA_PC_USAGE_DEFAULT |
                       VA_PC_USAGE_WIDEVINE;
        (*attribList)[attrib.type] = attrib.value;
    }
    else if (entrypoint == VAEntrypointProtectedTEEComm)
    {
        attrib.type = (VAConfigAttribType) VAConfigAttribProtectedContentSessionMode;
        attrib.value = VA_PC_SESSION_MODE_NONE;
        (*attribList)[attrib.type] = attrib.value;

        attrib.type = (VAConfigAttribType) VAConfigAttribProtectedContentSessionType;
        attrib.value = VA_PC_SESSION_TYPE_NONE;
        (*attribList)[attrib.type] = attrib.value;
    }
    else
    {
        return VA_STATUS_ERROR_ATTR_NOT_SUPPORTED;
    }
#endif //#ifdef VA_DRIVER_VTABLE_PROT_VERSION

    return VA_STATUS_SUCCESS;
}

VAStatus MediaLibvaCapsCp::AddProfileEntry(
    VAProfile profile,
    VAEntrypoint entrypoint,
    AttribMap *attributeList,
    int32_t configIdxStart,
    int32_t configNum)
{
    VAStatus status = VA_STATUS_SUCCESS;

    status = MediaLibvaCapsCpInterface::AddProfileEntry(profile, entrypoint, attributeList, configIdxStart, configNum);

    return status;
}
