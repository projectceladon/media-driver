/*
// Copyright (C) 2017 Intel Corporation
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
//! \file     media_extskuwa_g10.cpp
//!

#include "igfxfmid.h"
#include "linux_system_info.h"
#include "skuwa_factory.h"
#include "linux_skuwa_debug.h"
#include "linux_media_skuwa.h"

#define CNL_C0_STEPPING_REV 0x3

//extern template class DeviceInfoFactory<GfxDeviceInfo>;
typedef DeviceInfoFactory<LinuxDeviceInit> DeviceInit;

static bool InitGen10MediaExtSku(struct GfxDeviceInfo *devInfo,
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
    MEDIA_WR_SKU(skuTable, FtrMedia4KLace, 1);
    MEDIA_WR_SKU(skuTable, FtrDNDisableFor4K, 1);
    if (devInfo->eGTType == GTTYPE_GT3)
    {
        MEDIA_WR_SKU(skuTable, FtrMedia4KLace, 1);
        MEDIA_WR_SKU(skuTable, FtrDNDisableFor4K, 0);
    }
    MEDIA_WR_SKU(skuTable, FtrFDFB, 1);

    /* 360Stich is disabled by default */
    MEDIA_WR_SKU(skuTable, Ftr360Stitch, 0);
    MEDIA_WR_SKU(skuTable, FtrHDR,       1);
    MEDIA_WR_SKU(skuTable, FtrCapturePipe, 1);
    MEDIA_WR_SKU(skuTable, FtrLensCorrectionFilter, 1);

    /* Is it necessary to enable 3Pplugin on Gen10 ? */
    MEDIA_WR_SKU(skuTable, Ftr3pPlugin, 0);

    MEDIA_WR_SKU(skuTable, FtrDisableVeboxDIScalar, 1);
    MEDIA_WR_SKU(skuTable, FtrFilmModeDetection, 1);

    MEDIA_WR_SKU(skuTable, FtrPAVPWinWidevine, 1);
    return true;
}

static bool InitGen10MediaExtWa(struct GfxDeviceInfo *devInfo,
                             MediaWaTable *waTable,
                             struct LinuxDriverInfo *drvInfo)
{
    if ((devInfo == nullptr) || (waTable == nullptr) || (drvInfo == nullptr))
    {
        DEVINFO_ERROR("null ptr is passed\n");
        return false;
    }
    MEDIA_WR_WA(waTable, WaProgramAppIDFor2VDBox, 0);

    if (drvInfo->devRev <= CNL_C0_STEPPING_REV)
    {
        MEDIA_WR_WA(waTable, WaUseStallingScoreBoard, 1);
    }

    return true;
}

struct LinuxDeviceInit cnlExtDeviceInit =
{
    .productFamily    = IGFX_CANNONLAKE,
    .InitMediaFeature = InitGen10MediaExtSku,
    .InitMediaWa      = InitGen10MediaExtWa,
};

static bool cnlExtDeviceRegister = DeviceInfoFactory<LinuxDeviceInit>::
    RegisterDevice((uint32_t)IGFX_CANNONLAKE + MEDIA_EXT_FLAG, &cnlExtDeviceInit);
