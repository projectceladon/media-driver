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
//! \file     media_extskuwa_g9.cpp
//!

#include "igfxfmid.h"
#include "linux_system_info.h"
#include "skuwa_factory.h"
#include "linux_skuwa_debug.h"
#include "linux_media_skuwa.h"

//extern template class DeviceInfoFactory<GfxDeviceInfo>;
typedef DeviceInfoFactory<LinuxDeviceInit> DeviceInit;

static bool InitSklMediaExtSku(struct GfxDeviceInfo *devInfo,
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
    MEDIA_WR_SKU(skuTable, FtrPAVPIsolatedDecode, 0);
    MEDIA_WR_SKU(skuTable, FtrIStabFilter, 0);

    MEDIA_WR_SKU(skuTable, FtrLace, 1);
    MEDIA_WR_SKU(skuTable, FtrMedia4KLace, 0);
    MEDIA_WR_SKU(skuTable, FtrDNDisableFor4K, 1);
    if ((devInfo->eGTType == GTTYPE_GT3) ||
        (devInfo->eGTType == GTTYPE_GT4))
    {
        MEDIA_WR_SKU(skuTable, FtrMedia4KLace, 1);
        MEDIA_WR_SKU(skuTable, FtrDNDisableFor4K, 0);
    }

    MEDIA_WR_SKU(skuTable, FtrFDFB, 1);
    MEDIA_WR_SKU(skuTable, Ftr360Stitch, 1);
    MEDIA_WR_SKU(skuTable, FtrHDR, 0);
    MEDIA_WR_SKU(skuTable, FtrCapturePipe, 1);
    MEDIA_WR_SKU(skuTable, FtrLensCorrectionFilter, 1);
    MEDIA_WR_SKU(skuTable, Ftr3pPlugin, 1);
    MEDIA_WR_SKU(skuTable, FtrFilmModeDetection, 1);

    return true;
}

static bool InitSklMediaExtWa(struct GfxDeviceInfo *devInfo,
                             MediaWaTable *waTable,
                             struct LinuxDriverInfo *drvInfo)
{
    if ((devInfo == nullptr) || (waTable == nullptr) || (drvInfo == nullptr))
    {
        DEVINFO_ERROR("null ptr is passed\n");
        return false;
    }

    MEDIA_WR_WA(waTable, WaProgramAppIDFor2VDBox, drvInfo->hasBsd2);

    return true;
}

static bool InitBxtMediaExtSku(struct GfxDeviceInfo *devInfo,
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
    MEDIA_WR_SKU(skuTable, FtrPAVPIsolatedDecode, 0);
    MEDIA_WR_SKU(skuTable, FtrIStabFilter, 0);
    MEDIA_WR_SKU(skuTable, FtrLace, 0);
    MEDIA_WR_SKU(skuTable, FtrMedia4KLace, 0);
    MEDIA_WR_SKU(skuTable, FtrDNDisableFor4K, 1);
    MEDIA_WR_SKU(skuTable, FtrFDFB, 1);

    MEDIA_WR_SKU(skuTable, FtrHDR, 0);
    MEDIA_WR_SKU(skuTable, FtrCapturePipe, 1);
    MEDIA_WR_SKU(skuTable, FtrLensCorrectionFilter, 1);
    MEDIA_WR_SKU(skuTable, Ftr3pPlugin, 1);
    MEDIA_WR_SKU(skuTable, FtrFilmModeDetection, 1);
    return true;
}

static bool InitBxtMediaExtWa(struct GfxDeviceInfo *devInfo,
                             MediaWaTable *waTable,
                             struct LinuxDriverInfo *drvInfo)
{
    if ((devInfo == nullptr) || (waTable == nullptr) || (drvInfo == nullptr))
    {
        DEVINFO_ERROR("null ptr is passed\n");
        return false;
    }

    MEDIA_WR_WA(waTable, WaProgramAppIDFor2VDBox, 0);
    MEDIA_WR_WA(waTable, WaHuCStreamOutOffsetIsStreamInOffsetForPAVP, 1);

    return true;
}


static bool InitKblMediaExtSku(struct GfxDeviceInfo *devInfo,
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
    MEDIA_WR_SKU(skuTable, FtrMedia4KLace, 0);
    MEDIA_WR_SKU(skuTable, FtrDNDisableFor4K, 1);
    if ((devInfo->eGTType == GTTYPE_GT3) ||
        (devInfo->eGTType == GTTYPE_GT4))
    {
        MEDIA_WR_SKU(skuTable, FtrMedia4KLace, 1);
        MEDIA_WR_SKU(skuTable, FtrDNDisableFor4K, 0);
    }

    MEDIA_WR_SKU(skuTable, FtrFDFB, 1);
    MEDIA_WR_SKU(skuTable, Ftr360Stitch, 1);
    MEDIA_WR_SKU(skuTable, FtrHDR, 1);
    MEDIA_WR_SKU(skuTable, FtrCapturePipe, 1);
    MEDIA_WR_SKU(skuTable, FtrLensCorrectionFilter, 1);
    MEDIA_WR_SKU(skuTable, Ftr3pPlugin, 1);
    MEDIA_WR_SKU(skuTable, FtrFilmModeDetection, 1);

    MEDIA_WR_SKU(skuTable, FtrPAVPWinWidevine, 1);
    return true;
}

static bool InitKblMediaExtWa(struct GfxDeviceInfo *devInfo,
                             MediaWaTable *waTable,
                             struct LinuxDriverInfo *drvInfo)
{
    if ((devInfo == nullptr) || (waTable == nullptr) || (drvInfo == nullptr))
    {
        DEVINFO_ERROR("null ptr is passed\n");
        return false;
    }

    MEDIA_WR_WA(waTable, WaProgramAppIDFor2VDBox, drvInfo->hasBsd2);
    MEDIA_WR_WA(waTable, WaHuCStreamOutOffsetIsStreamInOffsetForPAVP, 1);

    return true;
}

static bool InitGlkMediaExtSku(struct GfxDeviceInfo *devInfo,
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
    MEDIA_WR_SKU(skuTable, FtrMedia4KLace, 0);
    MEDIA_WR_SKU(skuTable, FtrFDFB, 1);
    MEDIA_WR_SKU(skuTable, FtrCapturePipe, 1);
    MEDIA_WR_SKU(skuTable, FtrLensCorrectionFilter, 1);
    MEDIA_WR_SKU(skuTable, Ftr3pPlugin, 1);
    MEDIA_WR_SKU(skuTable, FtrFilmModeDetection, 1);

    return true;
}

static bool InitGlkMediaExtWa(struct GfxDeviceInfo *devInfo,
                             MediaWaTable *waTable,
                             struct LinuxDriverInfo *drvInfo)
{
    if ((devInfo == nullptr) || (waTable == nullptr) || (drvInfo == nullptr))
    {
        DEVINFO_ERROR("null ptr is passed\n");
        return false;
    }

    MEDIA_WR_WA(waTable, WaProgramAppIDFor2VDBox, 0);
    MEDIA_WR_WA(waTable, WaHuCStreamOutOffsetIsStreamInOffsetForPAVP, 1);

    return true;
}

static struct LinuxDeviceInit sklExtDeviceInit =
{
    .productFamily    = IGFX_SKYLAKE,
    .InitMediaFeature = InitSklMediaExtSku,
    .InitMediaWa      = InitSklMediaExtWa,
};

static struct LinuxDeviceInit bxtExtDeviceInit =
{
    .productFamily    = IGFX_BROXTON,
    .InitMediaFeature = InitBxtMediaExtSku,
    .InitMediaWa      = InitBxtMediaExtWa,
};

static struct LinuxDeviceInit kblExtDeviceInit =
{
    .productFamily    = IGFX_KABYLAKE,
    .InitMediaFeature = InitKblMediaExtSku,
    .InitMediaWa      = InitKblMediaExtWa,
};

/* CFL uses the same code as that on KBL */
static struct LinuxDeviceInit cflExtDeviceInit =
{
    .productFamily    = IGFX_COFFEELAKE,
    .InitMediaFeature = InitKblMediaExtSku,
    .InitMediaWa      = InitKblMediaExtWa,
};

static struct LinuxDeviceInit glkExtDeviceInit =
{
    .productFamily    = IGFX_GEMINILAKE,
    .InitMediaFeature = InitGlkMediaExtSku,
    .InitMediaWa      = InitGlkMediaExtWa,
};

static bool sklExtDeviceRegister = DeviceInfoFactory<LinuxDeviceInit>::
    RegisterDevice((uint32_t)IGFX_SKYLAKE + MEDIA_EXT_FLAG, &sklExtDeviceInit);

static bool bxtExtDeviceRegister = DeviceInfoFactory<LinuxDeviceInit>::
    RegisterDevice((uint32_t)IGFX_BROXTON + MEDIA_EXT_FLAG, &bxtExtDeviceInit);

static bool kblExtDeviceRegister = DeviceInfoFactory<LinuxDeviceInit>::
    RegisterDevice((uint32_t)IGFX_KABYLAKE + MEDIA_EXT_FLAG, &kblExtDeviceInit);

static bool cflExtDeviceRegister = DeviceInfoFactory<LinuxDeviceInit>::
    RegisterDevice((uint32_t)IGFX_COFFEELAKE + MEDIA_EXT_FLAG, &cflExtDeviceInit);

static bool glkExtDeviceRegister = DeviceInfoFactory<LinuxDeviceInit>::
    RegisterDevice((uint32_t)IGFX_GEMINILAKE + MEDIA_EXT_FLAG, &glkExtDeviceInit);
