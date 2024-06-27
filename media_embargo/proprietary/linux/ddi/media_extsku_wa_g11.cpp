/*
// Copyright (C) 2017-2018 Intel Corporation
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
//! \file     media_extskuwa_g11.cpp
//!

#include "igfxfmid.h"
#include "linux_system_info.h"
#include "skuwa_factory.h"
#include "linux_skuwa_debug.h"
#include "linux_media_skuwa.h"

//extern template class DeviceInfoFactory<GfxDeviceInfo>;
typedef DeviceInfoFactory<LinuxDeviceInit> DeviceInit;

static bool InitIclMediaExtSku(struct GfxDeviceInfo *devInfo,
                             MediaFeatureTable *skuTable,
                             struct LinuxDriverInfo *drvInfo,
                             MediaUserSettingSharedPtr userSettingPtr)
{
    if ((devInfo == nullptr) || (skuTable == nullptr) || (drvInfo == nullptr))
    {
        DEVINFO_ERROR("null ptr is passed\n");
        return false;
    }

    MEDIA_WR_SKU(skuTable, FtrPAVP, 1);
    MEDIA_WR_SKU(skuTable, FtrPAVPStout, 1);
    MEDIA_WR_SKU(skuTable, FtrPAVPIsolatedDecode, 1);
    MEDIA_WR_SKU(skuTable, FtrHWCounterAutoIncrementSupport, 1);
    MEDIA_WR_SKU(skuTable, FtrIStabFilter, 0);
    MEDIA_WR_SKU(skuTable, FtrLace, 1);

    MEDIA_WR_SKU(skuTable, FtrFDFB, 1);
    /* enabled by default */
    MEDIA_WR_SKU(skuTable, Ftr360Stitch, 0);
    MEDIA_WR_SKU(skuTable, FtrHDR,       1);
    MEDIA_WR_SKU(skuTable, FtrCapturePipe, 1);
    MEDIA_WR_SKU(skuTable, FtrLensCorrectionFilter, 1);
    MEDIA_WR_SKU(skuTable, FtrDisableVeboxDIScalar, 1);
    MEDIA_WR_SKU(skuTable, FtrFilmModeDetection, 1);

    MEDIA_WR_SKU(skuTable, Ftr3pPlugin, 0);

    MEDIA_WR_SKU(skuTable, FtrPAVPWinWidevine, 1);

    if ((devInfo->productFamily == IGFX_LAKEFIELD)
        || (devInfo->productFamily == IGFX_JASPERLAKE))
    {
        MEDIA_WR_SKU(skuTable, FtrDisableVEBoxFeatures, 1);
        MEDIA_WR_SKU(skuTable, FtrCapturePipe, 0);
        MEDIA_WR_SKU(skuTable, FtrDisableVeboxDIScalar, 0);
        MEDIA_WR_SKU(skuTable, FtrLensCorrectionFilter, 0);
        MEDIA_WR_SKU(skuTable, FtrFilmModeDetection, 0);
        MEDIA_WR_SKU(skuTable, FtrDisableVDBox2SFC, 1);
    }

    return true;
}

static bool InitIclMediaExtWa(struct GfxDeviceInfo *devInfo,
                             MediaWaTable *waTable,
                             struct LinuxDriverInfo *drvInfo)
{
    if ((devInfo == nullptr) || (waTable == nullptr) || (drvInfo == nullptr))
    {
        DEVINFO_ERROR("null ptr is passed\n");
        return false;
    }
    MEDIA_WR_WA(waTable, WaProgramAppIDFor2VDBox, 0);

    return true;
}

static struct LinuxDeviceInit iclExtDeviceInit =
{
    .productFamily    = IGFX_ICELAKE,
    .InitMediaFeature = InitIclMediaExtSku,
    .InitMediaWa      = InitIclMediaExtWa,
};

static struct LinuxDeviceInit jslExtDeviceInit =
{
    .productFamily    = IGFX_JASPERLAKE,
    .InitMediaFeature = InitIclMediaExtSku,
    .InitMediaWa      = InitIclMediaExtWa,
};

static bool iclExtDeviceRegister = DeviceInfoFactory<LinuxDeviceInit>::
    RegisterDevice((uint32_t)IGFX_ICELAKE + MEDIA_EXT_FLAG, &iclExtDeviceInit);

static bool icllpExtDeviceRegister = DeviceInfoFactory<LinuxDeviceInit>::
    RegisterDevice((uint32_t)IGFX_ICELAKE_LP + MEDIA_EXT_FLAG, &iclExtDeviceInit);

static bool lkfExtDeviceRegister = DeviceInfoFactory<LinuxDeviceInit>::
    RegisterDevice((uint32_t)IGFX_LAKEFIELD + MEDIA_EXT_FLAG, &iclExtDeviceInit);

static bool jslExtDeviceRegister = DeviceInfoFactory<LinuxDeviceInit>::
    RegisterDevice((uint32_t)IGFX_JASPERLAKE + MEDIA_EXT_FLAG, &jslExtDeviceInit);
