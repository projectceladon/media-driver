/*
// Copyright (C) 2022 Intel Corporation
//
// Licensed under the Apache License,Version 2.0(the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
//!
//! \file     media_extsku_wa_xe_lpm_plus.cpp
//!

#include "linux_skuwa_debug.h"
#include "linux_media_skuwa.h"
#include "media_extsku_wa_xe_lpm_plus.h"

bool InitMtlMediaExtSku(struct GfxDeviceInfo         *devInfo,
                        MediaFeatureTable            *skuTable,
                        struct LinuxDriverInfo       *drvInfo,
                        MediaUserSettingSharedPtr    userSettingPtr)
{
    if ((devInfo == nullptr) || (skuTable == nullptr) || (drvInfo == nullptr))
    {
        DEVINFO_ERROR("null ptr is passed\n");
        return false;
    }

    MEDIA_WR_SKU(skuTable, FtrPAVP, 1);
    MEDIA_WR_SKU(skuTable, FtrPAVPStout, 1);
    MEDIA_WR_SKU(skuTable, FtrPAVPStoutTranscode, 1);
    MEDIA_WR_SKU(skuTable, FtrPAVPIsolatedDecode, 1);
    MEDIA_WR_SKU(skuTable, FtrHWCounterAutoIncrementSupport, 1);
    MEDIA_WR_SKU(skuTable, FtrProtectedGEMContextPatch, 1);
    return true;
}

bool InitMtlMediaExtWa(struct GfxDeviceInfo   *devInfo,
                       MediaWaTable           *waTable,
                       struct LinuxDriverInfo *drvInfo)
{
    if ((devInfo == nullptr) || (waTable == nullptr) || (drvInfo == nullptr))
    {
        DEVINFO_ERROR("null ptr is passed\n");
        return false;
    }

    return true;
}

static struct LinuxDeviceInit mtlExtDeviceInit =
{
    .productFamily    = IGFX_METEORLAKE,
    .InitMediaFeature = InitMtlMediaExtSku,
    .InitMediaWa      = InitMtlMediaExtWa,
};

static bool mtlExtDeviceRegister = DeviceInfoFactory<LinuxDeviceInit>::
    RegisterDevice((uint32_t)IGFX_METEORLAKE + MEDIA_EXT_FLAG, &mtlExtDeviceInit);
