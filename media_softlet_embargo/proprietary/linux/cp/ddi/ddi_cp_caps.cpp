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
//! \file     ddi_cp_caps.cpp
//! \brief    cp caps source file
//!

#include "ddi_cp_caps.h"

static const bool registeredCpCapsImp = CpCapsFactory::Register<DdiCpCaps>(CapsImp);

DdiCpCapsInterface *CreateDdiCpCaps()
{
    DdiCpCapsInterface *ddiCpCaps = nullptr;
    ddiCpCaps = CpCapsFactory::Create(CapsImp);
    if(ddiCpCaps == nullptr)
    {
        DDI_CP_ASSERTMESSAGE("Create Cp Caps failed.");
    }
    
    return ddiCpCaps;
}

VAStatus DdiCpCaps::UpdateSecureDecodeProfile(ProfileMap *profileMap)
{
    DDI_CP_FUNC_ENTER;
    DDI_CP_CHK_NULL(profileMap, "Invalid profile data", VA_STATUS_ERROR_INVALID_PARAMETER);
    std::vector<uint32_t> supportedEncryptionType;

    for (auto profileMapIter: *profileMap)
    {
        bool isSupportedCodecType = false;
        supportedEncryptionType.clear();
        
        // Check if supported codec type and get its encryption type
        auto profile = profileMapIter.first;
        for(auto encryptionType: DecodeEncryptionType)
        {
            auto iter = find(encryptionType.first.begin(), encryptionType.first.end(), profile);
            if(iter != encryptionType.first.end())
            {
                isSupportedCodecType = true;
                supportedEncryptionType = encryptionType.second;
                break;
            }
        }

        if(isSupportedCodecType && supportedEncryptionType.size() > 0)
        {
            for(auto entrypointMapIter: *profileMapIter.second)
            {
                auto entrypoint     = entrypointMapIter.first;
                auto entrypointData = entrypointMapIter.second;
                DDI_CHK_NULL(entrypointData, "Null pointer", VA_STATUS_ERROR_INVALID_PARAMETER);

                auto attriblist     = (AttribList*)entrypointData->attribList;
                DDI_CHK_NULL(attriblist, "Null pointer", VA_STATUS_ERROR_INVALID_PARAMETER);

                auto configData     = (ConfigDataList*)entrypointData->configDataList;
                DDI_CHK_NULL(configData, "Null pointer", VA_STATUS_ERROR_INVALID_PARAMETER);
                
                // Update in attribute
                for (auto &attr : *attriblist)
                {
                    if(attr.type == VAConfigAttribEncryption)
                    {
                        attr.value = 0;
                        for(auto type : supportedEncryptionType)
                        {
                            attr.value |= type;
                        }
                    }
                }

                // Update in config data
                if(entrypoint == VAEntrypointVLD)
                {
                    bool insertRequired = true;
                    for (auto type : supportedEncryptionType)
                    {
                        for (auto config : *configData)
                        {
                            if (config.data.encryptType == type)
                            {
                                insertRequired = false;
                                break;
                            }
                        }
                        if (insertRequired)
                        {
                            uint32_t oriSize = configData->size();
                            configData->reserve(oriSize << 1);
                            ConfigDataList::iterator iter = configData->begin();
                            for (uint32_t index = 0; index < oriSize; ++index, ++iter)
                            {
                                ComponentData tmpData    = *iter;
                                tmpData.data.encryptType = type;
                                configData->insert(configData->end(), tmpData);
                            }
                        }
                    }
                }
            }
        }
    }
    return VA_STATUS_SUCCESS;
}

VAStatus DdiCpCaps::InitProfileMap(DDI_MEDIA_CONTEXT *mediaCtx, ProfileMap *profileMap)
{
    DDI_CP_FUNC_ENTER;

    DDI_CP_CHK_NULL(mediaCtx, "Invalid media context", VA_STATUS_ERROR_INVALID_PARAMETER);
    DDI_CP_CHK_NULL(profileMap, "Invalid profile data", VA_STATUS_ERROR_INVALID_PARAMETER);

    uint32_t ftrPAVP = MEDIA_IS_SKU(&(mediaCtx->SkuTable), FtrPAVP);
    uint32_t ftrStout = MEDIA_IS_SKU(&(mediaCtx->SkuTable), FtrPAVPStout);
    uint32_t ftrStoutTranscode = MEDIA_IS_SKU(&(mediaCtx->SkuTable), FtrPAVPStoutTranscode);

    m_isEntryptSupported =  (MEDIA_IS_SKU(&(mediaCtx->SkuTable), FtrEnableMediaKernels) && 
        (!MEDIA_IS_WA(&(mediaCtx->WaTable), WaDisableHuCBasedDRM)));

    if(ftrPAVP)
    {
        if(ftrStout)
        {
            for (int i = 0; i < attribList_VAProfileProtected_VAEntrypointProtectedContent.size(); i++)
            {
                if (attribList_VAProfileProtected_VAEntrypointProtectedContent.at(i).type == (VAConfigAttribType)VAConfigAttribProtectedContentSessionMode)
                {
                    attribList_VAProfileProtected_VAEntrypointProtectedContent.at(i).value |= VA_PC_SESSION_MODE_STOUT;
                }
            }

            configDataList_VAProfileProtected_VAEntrypointProtectedContent_Base.insert(
                configDataList_VAProfileProtected_VAEntrypointProtectedContent_Base.end(),
                configDataList_VAProfileProtected_VAEntrypointProtectedContent_Stout_Base.begin(), configDataList_VAProfileProtected_VAEntrypointProtectedContent_Stout_Base.end());
        }

        if(ftrStoutTranscode)
        {
            configDataList_VAProfileProtected_VAEntrypointProtectedContent_Base.insert(
                configDataList_VAProfileProtected_VAEntrypointProtectedContent_Base.end(),
                configDataList_VAProfileProtected_VAEntrypointProtectedContent_Stout_Transcode.begin(), configDataList_VAProfileProtected_VAEntrypointProtectedContent_Stout_Transcode.end());
        }

        profileMap->insert(profileMap_VAProfileProtected.begin(), profileMap_VAProfileProtected.end());
    }

    if(m_isEntryptSupported)
    {
        return UpdateSecureDecodeProfile(profileMap);
    }

    return VA_STATUS_SUCCESS;
}

bool DdiCpCaps::IsCpConfigId(VAConfigID configId)
{
    DDI_CP_FUNC_ENTER;

    VAConfigID curConfigId = REMOVE_CONFIG_ID_OFFSET(configId);

    return (curConfigId >= DDI_CP_GEN_CONFIG_ATTRIBUTES_BASE);
}

uint32_t DdiCpCaps::GetCpConfigId(VAConfigID configId)
{
    DDI_CP_FUNC_ENTER;

    return REMOVE_CONFIG_ID_CP_OFFSET(configId);
}
