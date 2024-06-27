/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2013-2017
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

File Name: cp_hal_gpu_g8.cpp

Abstract:
    HAL layer for PAVP functions prototypes

Environment:
    OS agnostic - should support - Windows Vista, Windows 7 (D3D9), 8 (D3D11.1), Linux, Android
    Platform specific - should support - Gen 8

Notes:
    None

======================= end_copyright_notice ==================================*/

#include "cp_hal_gpu_legacy.h"
#include "cp_hal_gpu_g8.h"
#include "cp_os_services.h"

HRESULT CPavpGpuHal8::CheckAsmfModeSupport()
{
    HRESULT                 hr = E_FAIL;
    PAVP_GPU_REGISTER_VALUE sPavpCValue = 0;

    GPU_HAL_FUNCTION_ENTER;

    do
    {
        if (m_pOsServices == nullptr)
        {
            hr = E_UNEXPECTED;
            GPU_HAL_ASSERTMESSAGE("m_pOsServices was nullptr");
            MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        hr = m_pOsServices->ReadMMIORegister(PAVP_REGISTER_PAVPC, sPavpCValue, CPavpGpuHal::ConvertRegType2RegOffset(PAVP_REGISTER_PAVPC));
        if (FAILED(hr))
        {
            GPU_HAL_ASSERTMESSAGE("Error reading PAVP_REGISTER_PAVPC register, hr = 0x%x.", hr);
            MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        if (sPavpCValue & PAVPC_ASMF_BIT_MASK)
        {
            hr = S_OK;
        }
    } while (false);

    GPU_HAL_FUNCTION_EXIT(hr);
    return hr;
}
