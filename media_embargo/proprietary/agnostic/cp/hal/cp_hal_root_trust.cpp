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

File Name: cp_hal_root_trust.cpp

Abstract:
Implements functions of interface class inherited by PCH and GSC classes.

Environment:
Windows, Linux, Android

Notes:
None

======================= end_copyright_notice ==================================*/

#include "cp_hal_root_trust.h"
#include "cp_hal_pch_unified.h"
#include "cp_hal_gsc.h"
#include "cp_hal_gsc_g12.h"
#include "cp_os_services.h"
#include "media_interfaces_mhw.h"

CPavpRootTrustHal* CPavpRootTrustHal::RootHal_Factory(
                                            RootTrust_HAL_USE roottrustHalUse,
                                            std::shared_ptr<CPavpOsServices> pOsServices)
{
    HRESULT hr = S_OK;
    CPavpRootTrustHal *pCPavpRootTrustHal = nullptr;
    PLATFORM sPlatform;
    DWORD isGSCTunnelingSupported = 0;

    if (pOsServices == nullptr)
    {
        PCH_HAL_ASSERTMESSAGE("Wrong Parameters! OsServices is invalid!");
        MT_ERR1(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__);
        return nullptr;
    }

    MOS_ZeroMemory(&sPlatform, sizeof(sPlatform));
    hr = pOsServices->GetPlatform(sPlatform);
    if (FAILED(hr))
    {
        PCH_HAL_ASSERTMESSAGE("Failed to get platform information during root of trust creation. Error = 0x%x.", hr);
        MT_ERR2(MT_CP_HAl_ROOT_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
        return nullptr;
    }
    PCH_HAL_VERBOSEMESSAGE("RootHal_Factory eProductFamily %d roottrustHalUse %d ", sPlatform.eProductFamily, roottrustHalUse);

#if (_DEBUG || _RELEASE_INTERNAL)
    //1: enable GSC virtual engine submission testing on ICL 2: force to use PCH stub
    pOsServices->ReadRegistry(__MEDIA_USER_FEATURE_VALUE_PAVP_GSC_TUNNELING_SUPPORTED_ID, &isGSCTunnelingSupported);

    if (isGSCTunnelingSupported == 1)
    {
        pCPavpRootTrustHal = MOS_New(CPavpGscHalG12, roottrustHalUse, pOsServices);
    }
    else if (isGSCTunnelingSupported == 2)
    {
        pCPavpRootTrustHal = MOS_New(CPavpPchHalUnified, roottrustHalUse, pOsServices);
    }
    else
#endif
    {
        pCPavpRootTrustHal = CPavpRootTrustHalFactory::Create(sPlatform.eProductFamily, roottrustHalUse, pOsServices);
        PCH_HAL_VERBOSEMESSAGE("pCPavpRootTrustHal %p eProductFamily %d roottrustHalUse %d ", pCPavpRootTrustHal, sPlatform.eProductFamily, roottrustHalUse);
    }

    return pCPavpRootTrustHal;
}

CPavpRootTrustHal* CPavpRootTrustHal::RootHal_Factory_Default()
{
    CPavpRootTrustHal *pCPavpRootTrustHal = nullptr;
    pCPavpRootTrustHal = new (std::nothrow) CPavpPchHalUnified();
    return pCPavpRootTrustHal;
}
