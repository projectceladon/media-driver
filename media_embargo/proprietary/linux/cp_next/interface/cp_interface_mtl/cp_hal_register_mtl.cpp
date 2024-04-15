/*
* Copyright (c) 2021-2022, Intel Corporation
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
//! \file     cp_hal_register_mtl.cpp
//! \brief    Helps with gen-specific factory creation.
//!

#include "cp_hal_gsc_xe_lpm_plus.h"
#include "cp_hal_gpu_xe_lpm_plus.h"
#include "mhw_cp_xe_lpm_plus.h"

static bool         mtlMhwCpRegistered              = MhwCpFactory::Register<MhwCpG13>((uint32_t)IGFX_METEORLAKE);
static bool         mtlRegisteredPavpGpuHal         = CPavpGpuHalFactory::Register<CPavpGpuHal13>((uint32_t)IGFX_METEORLAKE);
static const bool   mtlRegisteredPavpRootTrustHal   = CPavpRootTrustHalFactory::Register<CPavpGscHalG13>((uint32_t)IGFX_METEORLAKE);
