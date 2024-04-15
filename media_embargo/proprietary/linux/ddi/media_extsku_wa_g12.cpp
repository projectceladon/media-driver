/*
// Copyright (C) 2017-2020 Intel Corporation
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
//! \file     media_extsku_wa_g12.cpp
//!

#include "linux_skuwa_debug.h"
#include "linux_media_skuwa.h"
#include "media_extsku_wa_g12.h"

static struct LinuxCodecInfo tglExtCodecInfo =
{
    .avcDecoding        = 1,
    .mpeg2Decoding      = 1,
    .vp8Decoding        = 0,
    .vc1Decoding        = SET_STATUS_BY_FULL_OPEN_SOURCE(1, 0),
    .jpegDecoding       = 1,
    .avcEncoding        = SET_STATUS_BY_FULL_OPEN_SOURCE(1, 0),
    .mpeg2Encoding      = SET_STATUS_BY_FULL_OPEN_SOURCE(1, 0),
    .hevcDecoding       = 1,
    .hevcEncoding       = SET_STATUS_BY_FULL_OPEN_SOURCE(1, 0),
    .jpegEncoding       = 1,
    .avcVdenc           = 1,
    .vp9Decoding        = 1,
    .hevc10Decoding     = 1,
    .vp9b10Decoding     = 1,
    .hevc10Encoding     = SET_STATUS_BY_FULL_OPEN_SOURCE(1, 0),
    .hevc12Encoding     = SET_STATUS_BY_FULL_OPEN_SOURCE(1, 0),
    .vp8Encoding        = 0,
    .hevcVdenc          = 1,
    .vp9Vdenc           = 1,
    .adv0Decoding       = 1,
    .adv1Decoding       = 1,
};

bool InitTglMediaExtSku(struct GfxDeviceInfo *devInfo,
                             MediaFeatureTable *skuTable,
                             struct LinuxDriverInfo *drvInfo,
                             MediaUserSettingSharedPtr userSettingPtr)
{
    if ((devInfo == nullptr) || (skuTable == nullptr) || (drvInfo == nullptr))
    {
        DEVINFO_ERROR("null ptr is passed\n");
        return false;
    }

    if (drvInfo->hasBsd)
    {
        LinuxCodecInfo *codecInfo = &tglExtCodecInfo;

        //TODO: Move to open folder when B0 stepping upstream
        if (drvInfo->devRev > 0)
        {
            MEDIA_WR_SKU(skuTable, FtrIntelAV1VLDDecoding8bit420, codecInfo->adv0Decoding);
            MEDIA_WR_SKU(skuTable, FtrIntelAV1VLDDecoding10bit420, codecInfo->adv1Decoding);
        }
    }

    MEDIA_WR_SKU(skuTable, FtrPAVP, 1);
    MEDIA_WR_SKU(skuTable, FtrPAVPStout, 1);
    MEDIA_WR_SKU(skuTable, FtrPAVPIsolatedDecode, 1);
    MEDIA_WR_SKU(skuTable, FtrHWCounterAutoIncrementSupport, 1);
    MEDIA_WR_SKU(skuTable, FtrIStabFilter, 0);
    MEDIA_WR_SKU(skuTable, FtrLace, 1);
    MEDIA_WR_SKU(skuTable, FtrFDFB, 1);

    MEDIA_WR_SKU(skuTable, Ftr3pPlugin, 0);
    MEDIA_WR_SKU(skuTable, Ftr360Stitch, 0);
    MEDIA_WR_SKU(skuTable, FtrHDR,       1);
    MEDIA_WR_SKU(skuTable, FtrCapturePipe, 1);
    MEDIA_WR_SKU(skuTable, FtrLensCorrectionFilter, 1);
    MEDIA_WR_SKU(skuTable, FtrDisableVeboxDIScalar, 1);
    MEDIA_WR_SKU(skuTable, FtrFilmModeDetection, 1);
    MEDIA_WR_SKU(skuTable, FtrPAVPWinWidevine, 1);

    if(drvInfo->devId == 0xFF20 || drvInfo->devId == 0x0201)
    {
        MEDIA_WR_SKU(skuTable, FtrConditionalBatchBuffEnd, 1);
    }
    return true;
}

bool InitTglMediaExtWa(struct GfxDeviceInfo *devInfo,
                             MediaWaTable *waTable,
                             struct LinuxDriverInfo *drvInfo)
{
    if ((devInfo == nullptr) || (waTable == nullptr) || (drvInfo == nullptr))
    {
        DEVINFO_ERROR("null ptr is passed\n");
        return false;
    }
    MEDIA_WR_WA(waTable, WaProgramAppIDFor2VDBox, 0);
    MEDIA_WR_WA(waTable, WaCheckCryptoCopyRollover, 1);
    
    // remove this when B0 stepping
    MEDIA_WR_WA(waTable, WaHEVCVDEncY210LinearInputNotSupported, 1);

    return true;
}

static struct LinuxDeviceInit tgllpExtDeviceInit =
{
    .productFamily    = IGFX_TIGERLAKE_LP,
    .InitMediaFeature = InitTglMediaExtSku,
    .InitMediaWa      = InitTglMediaExtWa,
};

static bool tgllpExtDeviceRegister = DeviceInfoFactory<LinuxDeviceInit>::
    RegisterDevice((uint32_t)IGFX_TIGERLAKE_LP + MEDIA_EXT_FLAG, &tgllpExtDeviceInit);
