/*
* Copyright (c) 2021, Intel Corporation
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
//! \file     cp_register_adln.cpp
//! \brief    Helps with gen-specific factory creation.
//!

#include "mhw_cp_g12.h"
#include "cp_hal_pch_unified.h"
#include "cp_hal_gpu_g12.h"

static bool         adlnRegisteredPavpGpuHal        = CPavpGpuHalFactory::Register<CPavpGpuHal12>((uint32_t)IGFX_ALDERLAKE_N);
static const bool   adlnRegisteredPavpRootTrustHal  = CPavpRootTrustHalFactory::Register<CPavpPchHalUnified>((uint32_t)IGFX_ALDERLAKE_N);
static bool         adlnMhwCpRegistered             = MhwCpFactory::Register<MhwCpG12>((uint32_t)IGFX_ALDERLAKE_N);
