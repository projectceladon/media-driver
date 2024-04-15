/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2013-2023
Intel Corporation All Rights Reserved.

The source code contained or described herein and all documents related to the 
source code ("Material") are owned by Intel Corporation or its suppliers or 
licensors. Title to the Material remains with Intel Corporation or its suppliers 
and licensors. The Material contains trade secrets and proprietary and confidential 
information of Intel or its suppliers and licensors. The Material is protected by 
worldwide copyright and trade secret laws and treaty provisions. No part of the 
Material may be used, copied, reproduced, modified, published, uploaded, posted, 
transmitted, distributed, or disclosed in any way without Intel's prior express 
written permission. 

No license under any patent, copyright, trade secret or other intellectual 
property right is granted to or conferred upon you by disclosure or delivery 
of the Materials, either expressly, by implication, inducement, estoppel 
or otherwise. Any license under such intellectual property rights must be 
express and approved by Intel in writing. 

File Name: cp_hal_gpu_g75.h

Abstract:
    Header for HAL layer for PAVP functions prototypes

Environment:
    OS agnostic - should support - Windows Vista, Windows 7 (D3D9), 8 (D3D11.1), Linux, Android
    Platform specific - should support - Gen 7.5

Notes:
    None

======================= end_copyright_notice ==================================*/

#ifndef __CP_HAL_GPU_G75_H__
#define __CP_HAL_GPU_G75_H__

#include "cp_hal_gpu_legacy.h"

/// \class CPavpGpuHal75
/// \brief HAL layer for PAVP functions prototypes for Gen 75
class CPavpGpuHal75 : public CPavpGpuHalLegacy
{
public:

    CPavpGpuHal75(PLATFORM& sPlatform, MEDIA_WA_TABLE& sWaTable, HRESULT& hr, CPavpOsServices& OsServices)
                    : CPavpGpuHalLegacy(sPlatform, sWaTable, hr, OsServices)
    {
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////

};

#endif // __CP_HAL_GPU_G75_H__
