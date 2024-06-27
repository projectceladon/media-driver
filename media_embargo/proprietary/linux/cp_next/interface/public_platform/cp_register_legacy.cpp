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
//! \file     cp_register_legacy.cpp
//! \brief    Helps with gen-specific factory creation of DG1 and previous platforms.
//!
#include "mhw_cp_g12.h"
#include "cp_hal_gpu_g12.h"
#include "cp_hal_pch_unified.h"
#include "cp_hal_gsc_g12.h"

#if IGFX_GEN75_SUPPORTED
#include "cp_hal_gpu_g75.h"
static const bool hswRegisteredPavpGpuHal         = CPavpGpuHalFactory::Register<CPavpGpuHal75>((uint32_t)IGFX_HASWELL);
static const bool hswRegisteredPavpRootTrustHal   = CPavpRootTrustHalFactory::Register<CPavpPchHalUnified>((uint32_t)IGFX_HASWELL);
#endif

#if IGFX_GEN8_SUPPORTED
#include "mhw_cp_g8.h"
#include "cp_hal_gpu_g8.h"
static const bool bdwRegisteredPavpGpuHal         = CPavpGpuHalFactory::Register<CPavpGpuHal8>((uint32_t)IGFX_BROADWELL);
static const bool bdwMhwCpRegistered              = MhwCpFactory::Register<MhwCpG8>(static_cast<int32_t>(IGFX_BROADWELL));
static const bool bdwRegisteredPavpRootTrustHal   = CPavpRootTrustHalFactory::Register<CPavpPchHalUnified>((uint32_t)IGFX_BROADWELL);
static const bool crvRegisteredPavpGpuHal         = CPavpGpuHalFactory::Register<CPavpGpuHal8>((uint32_t)IGFX_CHERRYVIEW);
static const bool crvRegisteredPavpRootTrustHal   = CPavpRootTrustHalFactory::Register<CPavpPchHalUnified>((uint32_t)IGFX_CHERRYVIEW);
#endif

#if IGFX_GEN9_SUPPORTED
#include "mhw_cp_g9.h"
static const bool sklRegisteredPavpGpuHal         = CPavpGpuHalFactory::Register<CPavpGpuHal8>((uint32_t)IGFX_SKYLAKE);
static const bool sklMhwCpRegistered              = MhwCpFactory::Register<MhwCpG9>(static_cast<int32_t>(IGFX_SKYLAKE));
static const bool sklRegisteredPavpRootTrustHal   = CPavpRootTrustHalFactory::Register<CPavpPchHalUnified>((uint32_t)IGFX_SKYLAKE);
static const bool kblRegisteredPavpGpuHal         = CPavpGpuHalFactory::Register<CPavpGpuHal8>((uint32_t)IGFX_KABYLAKE);
static const bool kblMhwCpRegistered              = MhwCpFactory::Register<MhwCpG9>(static_cast<int32_t>(IGFX_KABYLAKE));
static const bool kblRegisteredPavpRootTrustHal   = CPavpRootTrustHalFactory::Register<CPavpPchHalUnified>((uint32_t)IGFX_KABYLAKE);
static const bool cflRegisteredPavpGpuHal         = CPavpGpuHalFactory::Register<CPavpGpuHal8>((uint32_t)IGFX_COFFEELAKE);
static const bool cflMhwCpRegistered              = MhwCpFactory::Register<MhwCpG9>(static_cast<int32_t>(IGFX_COFFEELAKE));
static const bool cflRegisteredPavpRootTrustHal   = CPavpRootTrustHalFactory::Register<CPavpPchHalUnified>((uint32_t)IGFX_COFFEELAKE);
static const bool glvMhwCpRegistered              = MhwCpFactory::Register<MhwCpG9>(static_cast<int32_t>(IGFX_WILLOWVIEW));
static const bool bxtRegisteredPavpGpuHal         = CPavpGpuHalFactory::Register<CPavpGpuHal8>((uint32_t)IGFX_BROXTON);
static const bool bxtMhwCpRegistered              = MhwCpFactory::Register<MhwCpG9>(static_cast<int32_t>(IGFX_BROXTON));
static const bool bxtRegisteredPavpRootTrustHal   = CPavpRootTrustHalFactory::Register<CPavpPchHalUnified>((uint32_t)IGFX_BROXTON);
static const bool gmlRegisteredPavpGpuHal         = CPavpGpuHalFactory::Register<CPavpGpuHal8>((uint32_t)IGFX_GEMINILAKE);
static const bool gmlMhwCpRegistered              = MhwCpFactory::Register<MhwCpG9>(static_cast<int32_t>(IGFX_GEMINILAKE));
static const bool gmlRegisteredPavpRootTrustHal   = CPavpRootTrustHalFactory::Register<CPavpPchHalUnified>((uint32_t)IGFX_GEMINILAKE);
#endif

#if IGFX_GEN10_SUPPORTED
#include "mhw_cp_g10.h"
static const bool cnlRegisteredPavpGpuHal         = CPavpGpuHalFactory::Register<CPavpGpuHal8>((uint32_t)IGFX_CANNONLAKE);
static const bool cnlMhwCpRegistered              = MhwCpFactory::Register<MhwCpG10>(static_cast<int32_t>(IGFX_CANNONLAKE));
static const bool cnlRegisteredPavpRootTrustHal   = CPavpRootTrustHalFactory::Register<CPavpPchHalUnified>((uint32_t)IGFX_CANNONLAKE);
#endif

#if IGFX_GEN11_SUPPORTED
#include "mhw_cp_g11.h"
#include "cp_hal_gpu_g11.h"
static const bool iclRegisteredPavpGpuHal         = CPavpGpuHalFactory::Register<CPavpGpuHal11>((uint32_t)IGFX_ICELAKE);
static const bool iclMhwCpRegistered              = MhwCpFactory::Register<MhwCpG11>(static_cast<int32_t>(IGFX_ICELAKE));
static const bool iclRegisteredPavpRootTrustHal   = CPavpRootTrustHalFactory::Register<CPavpPchHalUnified>((uint32_t)IGFX_ICELAKE);
static const bool icllpRegisteredPavpGpuHal       = CPavpGpuHalFactory::Register<CPavpGpuHal11>((uint32_t)IGFX_ICELAKE_LP);
static const bool icllpMhwCpRegistered            = MhwCpFactory::Register<MhwCpG11>(static_cast<int32_t>(IGFX_ICELAKE_LP));
static const bool jspRegisteredPavpGpuHal         = CPavpGpuHalFactory::Register<CPavpGpuHal11>((uint32_t)IGFX_JASPERLAKE);
static const bool jplMhwCpRegistered              = MhwCpFactory::Register<MhwCpG11>(static_cast<int32_t>(IGFX_JASPERLAKE));
static const bool jspRegisteredPavpRootTrustHal   = CPavpRootTrustHalFactory::Register<CPavpPchHalUnified>((uint32_t)IGFX_JASPERLAKE);
static const bool lfdRegisteredPavpGpuHal         = CPavpGpuHalFactory::Register<CPavpGpuHal11>((uint32_t)IGFX_LAKEFIELD);
static const bool lfdMhwCpRegistered              = MhwCpFactory::Register<MhwCpG11>(static_cast<int32_t>(IGFX_LAKEFIELD));
static const bool lfdRegisteredPavpRootTrustHal   = CPavpRootTrustHalFactory::Register<CPavpPchHalUnified>((uint32_t)IGFX_LAKEFIELD);
#endif

static const bool tglRegisteredPavpRootTrustHal   = CPavpRootTrustHalFactory::Register<CPavpPchHalUnified>((uint32_t)IGFX_TIGERLAKE_LP);
static const bool tglRegisteredPavpGpuHal         = CPavpGpuHalFactory::Register<CPavpGpuHal12>((uint32_t)IGFX_TIGERLAKE_LP);
static const bool tgllpMhwCpRegistered            = MhwCpFactory::Register<MhwCpG12>(static_cast<int32_t>(IGFX_TIGERLAKE_LP));
static const bool dg1RegisteredPavpRootTrustHal   = CPavpRootTrustHalFactory::Register<CPavpGscHalG12>((uint32_t)IGFX_DG1);
static const bool dg1RegisteredPavpGpuHal         = CPavpGpuHalFactory::Register<CPavpGpuHal12>((uint32_t)IGFX_DG1);
static const bool dg1MhwCpRegistered              = MhwCpFactory::Register<MhwCpG12>(static_cast<int32_t>(IGFX_DG1));
