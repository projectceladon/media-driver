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
//! \file     media_extsku_wa_g12_dg1.cpp
//!

#include "media_extsku_wa_g12.h"

static struct LinuxDeviceInit dg1ExtDeviceInit =
{
    .productFamily    = IGFX_DG1,
    .InitMediaFeature = InitTglMediaExtSku,
    .InitMediaWa      = InitTglMediaExtWa,
};

static bool dg1ExtDeviceRegister = DeviceInfoFactory<LinuxDeviceInit>::
    RegisterDevice((uint32_t)IGFX_DG1 + MEDIA_EXT_FLAG, &dg1ExtDeviceInit);
