/*
// Copyright (C) 2021 Intel Corporation
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
//! \file     media_extsku_wa_g12.h
//!

#ifndef __MEDIA_EXTSKU_WA_G12_H__
#define __MEDIA_EXTSKU_WA_G12_H__

#include "igfxfmid.h"
#include "linux_system_info.h"
#include "skuwa_factory.h"

//extern template class DeviceInfoFactory<GfxDeviceInfo>;
typedef DeviceInfoFactory<LinuxDeviceInit> DeviceInit;

bool InitTglMediaExtSku(struct GfxDeviceInfo *devInfo,
                             MediaFeatureTable *skuTable,
                             struct LinuxDriverInfo *drvInfo,
                             MediaUserSettingSharedPtr userSettingPtr);

bool InitTglMediaExtWa(struct GfxDeviceInfo *devInfo,
                             MediaWaTable *waTable,
                             struct LinuxDriverInfo *drvInfo);

#endif //__MEDIA_EXTSKU_WA_G12_H__
