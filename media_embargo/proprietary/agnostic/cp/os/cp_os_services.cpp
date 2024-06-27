/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2012-2021
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

File Name: cp_os_services.cpp

Abstract:
    Implementation of the CP OS sevices base class in the OS agnostic design.
    Handles all and abstracts CP access from UMD to KMD. The class is derived by
    cp_pavposservices_win and cp_pavposservices_linux which implement the
    os_dependant parts

Environment:
    Windows 7, Windows 8, Linux

Notes:

======================= end_copyright_notice ==================================*/

#include "cp_os_services.h"

#include "GmmLib.h"
#include "mos_interface.h"

// Timeout attempting to write to register in KMD.
#define MAX_PAVP_APPID_WRITE_ATTEMPTS 20

CPavpOsServices::CPavpOsServices(HRESULT& hr, PAVP_DEVICE_CONTEXT pDeviceContext)
{
    CP_OS_FUNCTION_ENTER;

    hr = E_FAIL;

    ResetMemberVariables();

    do
    {
        if (pDeviceContext == nullptr)
        {
            CP_OS_ASSERTMESSAGE("Error: pDeviceContext is NULL.");
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        m_pOsInterface = nullptr;
        m_pPavpDeviceContext = pDeviceContext;

        // Initialize MOS interface.
        hr = InitMosInterface(m_pPavpDeviceContext);
        if (FAILED(hr) || m_pOsInterface == nullptr)
        {
            CP_OS_ASSERTMESSAGE("Error initializing MOS interface. hr = 0x%x", hr);
            MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

    } while (FALSE);

    CP_OS_FUNCTION_EXIT(hr);
};

CPavpOsServices::~CPavpOsServices()
{
    CP_OS_FUNCTION_ENTER;

    DestroyMosInterface();

    CP_OS_FUNCTION_EXIT(0);
}

HRESULT CPavpOsServices::ProgramAppIDRegister(PAVP_PCH_INJECT_KEY_MODE ePchInjectKeyMode, BYTE SessionId)
{
    HRESULT     hr          = E_FAIL;
    uint32_t    uiIteration = 1;

    CP_OS_FUNCTION_ENTER;

    // Busy wait to set the appid register.
    for (; uiIteration <= MAX_PAVP_APPID_WRITE_ATTEMPTS; uiIteration++)
    {
        hr = WriteAppIDRegister(ePchInjectKeyMode, SessionId);
        if (SUCCEEDED(hr))
        {
            break;
        }
        MosUtilities::MosSleep(1);
    }

    CP_OS_VERBOSEMESSAGE("Attempted to write the App ID register %d times(s).", uiIteration);

    CP_OS_FUNCTION_EXIT(hr);
    return hr;
}

void CPavpOsServices::ResetMemberVariables()
{
    CP_OS_FUNCTION_ENTER;

    m_pOsInterface = nullptr;
    m_pPavpDeviceContext = nullptr;
    m_isForceBltCopyEnabled = 0;

    CP_OS_FUNCTION_EXIT(0);
}

BOOL CPavpOsServices::ReportRegistryEnabled()
{
    HRESULT hr     = S_OK;
    DWORD   dwData = 0;
    BOOL    bReportRegistry = false;
    CP_OS_FUNCTION_ENTER;

    hr = ReadRegistry(
        __MEDIA_USER_FEATURE_VALUE_CP_REPORTS_ENABLED_ID,
        &dwData);
    if (SUCCEEDED(hr))
    {
        bReportRegistry = (dwData != 0);
    }
    else
    {
        CP_OS_NORMALMESSAGE("Failed to read '%s' key.", __MEDIA_USER_FEATURE_VALUE_CP_REPORTS_ENABLED);
    }

    CP_OS_FUNCTION_EXIT(0);
    return bReportRegistry;
}

void CPavpOsServices::WriteRegistry(
    CP_USER_FEATURE_VALUE_ID  UserFeatureValueId,
    DWORD                     dwData)
{
    MOS_STATUS                        eStatus                = MOS_STATUS_SUCCESS;
    MOS_USER_FEATURE_VALUE_WRITE_DATA UserFeatureWriteData = __NULL_USER_FEATURE_VALUE_WRITE_DATA__;

    CP_OS_FUNCTION_ENTER;

    do
    {
        if (m_pOsInterface == nullptr)
        {
            //if m_pOsInterface is nullptr, cannot get correct reg key path
            CP_OS_VERBOSEMESSAGE("m_pOsInterface is nullptr");
            break;
        }

        if (!ReportRegistryEnabled())
        {
            CP_OS_VERBOSEMESSAGE("CP registry reports are disabled.");
            break;
        }

        CP_OS_VERBOSEMESSAGE("Reporting value 0x%x to registry '%s'.",
            dwData,
            MosUtilities::MosUserFeatureLookupValueName(UserFeatureValueId));

        UserFeatureWriteData.ValueID = UserFeatureValueId;
        UserFeatureWriteData.Value.u32Data = dwData;

        eStatus = MOS_UserFeature_WriteValues_ID(nullptr, &UserFeatureWriteData, 1, m_pOsInterface->pOsContext);
        if (eStatus != MOS_STATUS_SUCCESS)
        {
             CP_OS_VERBOSEMESSAGE("MOS failed to write registry '%s'. status = %d",
                 MosUtilities::MosUserFeatureLookupValueName(UserFeatureValueId),
                 eStatus);
        }
    } while (FALSE);

    CP_OS_FUNCTION_EXIT(0);
}

HRESULT CPavpOsServices::ReadRegistry(
    CP_USER_FEATURE_VALUE_ID  UserFeatureValueId,
    PDWORD                    pdwData)
{
    HRESULT                     hr              = E_FAIL;
    MOS_STATUS                  eStatus         = MOS_STATUS_SUCCESS;
    MOS_USER_FEATURE_VALUE_DATA UserFeatureData;
    CP_OS_FUNCTION_ENTER;

    MOS_ZeroMemory(&UserFeatureData, sizeof(UserFeatureData));
    do
    {
        if (pdwData == nullptr)
        {
            CP_OS_ASSERTMESSAGE("pdwData is NULL.");
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        if (m_pOsInterface == nullptr)
        {
            //if m_pOsInterface is nullptr, cannot get correct reg key path
            CP_OS_VERBOSEMESSAGE("m_pOsInterface is nullptr");
            break;
        }

        // Read from the reg key.
        eStatus = MOS_UserFeature_ReadValue_ID(nullptr, UserFeatureValueId, &UserFeatureData, m_pOsInterface->pOsContext);
        if (eStatus == MOS_STATUS_SUCCESS)
        {
            *pdwData = static_cast<DWORD>(UserFeatureData.u32Data);
            hr = S_OK;
        }
        else
        {
             CP_OS_VERBOSEMESSAGE("MOS failed to read registry '%s'. status = %d",
                 MosUtilities::MosUserFeatureLookupValueName(UserFeatureValueId),
                 eStatus);
        }
    } while (FALSE);

    CP_OS_FUNCTION_EXIT(hr);
    return hr;
}

KM_PAVP_SESSION_TYPE CPavpOsServices::KmSessionTypeFromApiSessionType(PAVP_SESSION_TYPE type)
{
    KM_PAVP_SESSION_TYPE ReturnType = KM_PAVP_SESSION_TYPES_MAX;

    CP_OS_FUNCTION_ENTER;

    switch (type)
    {
        case PAVP_SESSION_TYPE_DECODE:
            ReturnType = KM_PAVP_SESSION_TYPE_DECODE;
            break;
        case PAVP_SESSION_TYPE_TRANSCODE:
            ReturnType = KM_PAVP_SESSION_TYPE_TRANSCODE;
            break;
        case PAVP_SESSION_TYPE_WIDI:
            ReturnType = KM_PAVP_SESSION_TYPE_WIDI;
            break;
        default:
            CP_OS_ASSERTMESSAGE("Invalid session type: %d.", type);
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_GENERIC_VALUE, type);
            break;
    }

    CP_OS_FUNCTION_EXIT(0);
    return ReturnType;
}

KM_PAVP_SESSION_MODE CPavpOsServices::KmSessionModeFromApiSessionMode(PAVP_SESSION_MODE mode)
{
    KM_PAVP_SESSION_MODE ReturnMode = KM_PAVP_SESSION_MODES_MAX;

    CP_OS_FUNCTION_ENTER;

    switch (mode)
    {
        case PAVP_SESSION_MODE_LITE:
            ReturnMode = KM_PAVP_SESSION_MODE_LITE;
            break;
        case PAVP_SESSION_MODE_HEAVY:
            ReturnMode = KM_PAVP_SESSION_MODE_HEAVY;
            break;
        case PAVP_SESSION_MODE_ISO_DEC:
            ReturnMode = KM_PAVP_SESSION_MODE_ISO_DEC;
            break;
        case PAVP_SESSION_MODE_STOUT:
            ReturnMode = KM_PAVP_SESSION_MODE_STOUT;
            break;
        case PAVP_SESSION_MODE_THV_D:
            ReturnMode = KM_PAVP_SESSION_MODE_THV_D;
            break;
        case PAVP_SESSION_MODE_HUC_GUARD:
            ReturnMode = KM_PAVP_SESSION_MODE_HUC_GUARD;
            break;
        default:
            CP_OS_ASSERTMESSAGE("Invalid session mode: %d.", mode);
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_GENERIC_VALUE, mode);
            break;
    }

    CP_OS_FUNCTION_EXIT(0);
    return ReturnMode;
}


HRESULT CPavpOsServices::InitMosInterface(
    PMOS_CONTEXT    pOsDriverContext,  // [in]
    PMOS_INTERFACE& pOsInterface)      // [in]
{
    HRESULT hr = S_OK;
    MOS_STATUS eStatus;

    CP_OS_FUNCTION_ENTER;

    do
    {
        pOsInterface = static_cast<PMOS_INTERFACE>(MOS_AllocAndZeroMemory(sizeof(MOS_INTERFACE)));
        if (pOsInterface == nullptr)
        {
            CP_OS_ASSERTMESSAGE("Unable to allocate memory for MOS interface.");
            hr = E_OUTOFMEMORY;
            MT_ERR2(MT_ERR_MEM_ALLOC, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        // Mos_InitInterface populates m_pOsInterface's function table with the OS specific functions
        // and stores the driver context, among some other configurations.
        // The component is used for tracking memory allocations.
        eStatus = Mos_InitInterface(
                pOsInterface,
                pOsDriverContext,
                COMPONENT_CP);
        if (MOS_FAILED(eStatus))
        {
            CP_OS_ASSERTMESSAGE("Error initializing MOS interface. eStatus = 0x%x.", eStatus);
            DestroyMosInterface(pOsInterface);
            hr = E_FAIL;
            MT_ERR2(MT_CP_INIT_MOS_INTERFACE, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, eStatus);
            break;
        }

    } while (FALSE);

    CP_OS_FUNCTION_EXIT(hr);
    return hr;
}

void CPavpOsServices::DestroyMosInterface(PMOS_INTERFACE& pOsInterface)
{
    CP_OS_FUNCTION_ENTER;

    if (pOsInterface != nullptr)
    {
        if (pOsInterface->pfnDestroy != nullptr)
        {
            pOsInterface->pfnDestroy(pOsInterface, FALSE);
        }

        MOS_FreeMemAndSetNull(pOsInterface);
    }

    CP_OS_FUNCTION_EXIT(0);
}

HRESULT CPavpOsServices::GetPlatform(PLATFORM& pPlatform)
{
    HRESULT hr = S_OK;

    CP_OS_FUNCTION_ENTER;

    do
    {
        // Verify that the MOS interface is valid.
        if (m_pOsInterface                 == nullptr ||
            m_pOsInterface->pfnGetPlatform == nullptr)
        {
            hr = E_UNEXPECTED;
            CP_OS_ASSERTMESSAGE("OS interface is NULL.");
            MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        m_pOsInterface->pfnGetPlatform(
            m_pOsInterface,
            &pPlatform);

    } while (FALSE);

    CP_OS_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpOsServices::GetWaTable(MEDIA_WA_TABLE& sWATable)
{
    HRESULT hr = S_OK;

    CP_OS_FUNCTION_ENTER;

    do
    {
        // Verify that the MOS interface is valid.
        if (m_pOsInterface                 == nullptr ||
            m_pOsInterface->pfnGetWaTable  == nullptr)
        {
            hr = E_UNEXPECTED;
            CP_OS_ASSERTMESSAGE("OS interface is NULL.");
            MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        MEDIA_WA_TABLE *pWATable = m_pOsInterface->pfnGetWaTable(
            m_pOsInterface);

        if(pWATable == nullptr)
        {
            hr = E_UNEXPECTED;
            CP_OS_ASSERTMESSAGE("Pointer to Workaround table is NULL.");
            MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        sWATable = *pWATable;


    } while (FALSE);

    CP_OS_FUNCTION_EXIT(hr);
    return hr;
}

MEDIA_FEATURE_TABLE *CPavpOsServices::GetSkuTable()
{
    HRESULT hr = S_OK;
    MEDIA_FEATURE_TABLE* pSkuTable = nullptr;
    CP_OS_FUNCTION_ENTER;

    // Verify that the MOS interface is valid.
    if (m_pOsInterface                 == nullptr ||
        m_pOsInterface->pfnGetSkuTable  == nullptr)
    {
        CP_OS_ASSERTMESSAGE("OS interface is NULL.");
        MT_ERR1(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__);
        return nullptr;
    }

    pSkuTable = m_pOsInterface->pfnGetSkuTable(
        m_pOsInterface);

    if (pSkuTable == nullptr)
    {
        CP_OS_ASSERTMESSAGE("pSkuTable is NULL.");
        MT_ERR1(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__);
    }
    return pSkuTable;
}

HRESULT CPavpOsServices::CreateVideoGPUContext()
{
    HRESULT hr = S_OK;
    MOS_STATUS eStatus;

    CP_OS_FUNCTION_ENTER;

    do
    {
        // Verify that the needed MOS interface is valid.
        if (m_pOsInterface                      == nullptr ||
            m_pOsInterface->pfnSetGpuContext    == nullptr ||
            m_pOsInterface->pfnCreateGpuContext == nullptr)
        {
            CP_OS_ASSERTMESSAGE("OS interface is NULL.");
            hr = E_UNEXPECTED;
            MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }
        
        MOS_GPUCTX_CREATOPTIONS_ENHANCED createOption;
        eStatus = m_pOsInterface->pfnCreateGpuContext(
                 m_pOsInterface,
                 MOS_GPU_CONTEXT_VIDEO,
                 MOS_GPU_NODE_VIDEO,
                 &createOption);
        if (MOS_FAILED(eStatus))
        {
            CP_OS_ASSERTMESSAGE("MOS failed to create GPU Context. eStatus = 0x%x", eStatus);
            hr = E_FAIL;
            MT_ERR2(MT_MOS_GPUCXT_CREATE, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        eStatus = m_pOsInterface->pfnSetGpuContext(m_pOsInterface, MOS_GPU_CONTEXT_VIDEO);
        if (MOS_FAILED(eStatus))
        {
            CP_OS_ASSERTMESSAGE("MOS failed to set the new GPU Context. eStatus = 0x%x", eStatus);
            hr = E_FAIL;
            MT_ERR2(MT_MOS_GPUCXT_SET, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }
    } while (FALSE);

    CP_OS_FUNCTION_EXIT(hr);
    return hr;
}


HRESULT CPavpOsServices::AllocateLinearResource(
    MOS_GFXRES_TYPE eType,          // [in]
    DWORD           dwWidth,        // [in]
    DWORD           dwHeight,       // [in]
    PCSTR           pResourceName,  // [in, optional]
    MOS_FORMAT      eFormat,        // [in]
    PMOS_RESOURCE& pResource,      // [out]
    BOOL            bSystemMem,         // [in, optional]
    BOOL            bLockable) const    // [in, optional]
{
    HRESULT                 hr = S_OK;
    MOS_ALLOC_GFXRES_PARAMS sMosAllocParams;
    MOS_STATUS              eStatus;

    CP_OS_FUNCTION_ENTER;

    do
    {
        // Verify that the needed OS interface is valid.
        if (m_pOsInterface                      == nullptr ||
            m_pOsInterface->pfnAllocateResource == nullptr)
        {
            CP_OS_ASSERTMESSAGE("Error: OS interface is NULL.");
            hr = E_UNEXPECTED;
            MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        // We only support
        if (eType != MOS_GFXRES_BUFFER &&
            eType != MOS_GFXRES_2D)
        {
            CP_OS_ASSERTMESSAGE("Error: Only 'BUFFER' and '2D' types are supported in CP.");
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        if (eType == MOS_GFXRES_BUFFER &&
            dwHeight != 1)
        {
            CP_OS_ASSERTMESSAGE("Error: Resources of type 'BUFFER' must have height = 1.");
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        pResource = MOS_New(MOS_RESOURCE);
        if (pResource == nullptr)
        {
            CP_OS_ASSERTMESSAGE("Error: Failed to allocate memory for MOS_RESOURCE.");
            hr = E_OUTOFMEMORY;
            MT_ERR2(MT_ERR_MEM_ALLOC, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }
        MOS_ZeroMemory(pResource, sizeof(*pResource));

        MOS_ZeroMemory(&sMosAllocParams, sizeof(sMosAllocParams));
        sMosAllocParams.Type        = eType;
        sMosAllocParams.dwWidth     = dwWidth;
        sMosAllocParams.dwHeight    = dwHeight;
        sMosAllocParams.TileType    = MOS_TILE_LINEAR;
        sMosAllocParams.Format      = eFormat;
        sMosAllocParams.pBufName    = pResourceName;

        //For cryptocopy only, apply for only small lmem bar, full lmem bar still use default memory policy
        if (m_isForceBltCopyEnabled ||
            MEDIA_IS_SKU(m_pOsInterface->pfnGetSkuTable(m_pOsInterface), FtrLimitedLMemBar))
        {
            if (bSystemMem)
            {
                sMosAllocParams.dwMemType = MOS_MEMPOOL_SYSTEMMEMORY;
            }
            else if (!bLockable)
            {
                sMosAllocParams.Flags.bNotLockable = TRUE;
                sMosAllocParams.dwMemType = MOS_MEMPOOL_DEVICEMEMORY;
            }
            else if (bLockable)
            {
                sMosAllocParams.Flags.bNotLockable = FALSE;
                sMosAllocParams.dwMemType = MOS_MEMPOOL_VIDEOMEMORY;
            }
        }

        if (MEDIA_IS_SKU(m_pOsInterface->pfnGetSkuTable(m_pOsInterface), FtrSAMediaCachePolicy))
        {
            sMosAllocParams.ResUsageType = MOS_HW_RESOURCE_USAGE_CP_INTERNAL_WRITE;
        }

        eStatus = m_pOsInterface->pfnAllocateResource(
                 m_pOsInterface,
                 &sMosAllocParams,
                 pResource);
        if (MOS_FAILED(eStatus))
        {
            CP_OS_ASSERTMESSAGE("Failed to allocate resource. Error = 0x%x.", eStatus);
            hr = E_FAIL;
            MT_ERR2(MT_CP_RESOURCE_ALLOC, MT_ERROR_CODE, eStatus, MT_FUNC_RET, hr);
        }
    } while (FALSE);

    CP_OS_FUNCTION_EXIT(hr);
    return hr;
}

void *CPavpOsServices::LockResource(MOS_RESOURCE& res, const LockType lockType) const
{
    if (m_pOsInterface == nullptr ||
        m_pOsInterface->pfnLockResource == nullptr)
    {
        CP_OS_ASSERTMESSAGE("OS interface is NULL.");
        MT_ERR1(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__);
        return nullptr;
    }

    MOS_LOCK_PARAMS sMosLockParams{};

    switch (lockType)
    {
    case LockType::Read:
        sMosLockParams.ReadOnly = TRUE;
        break;
    case LockType::Write:
        sMosLockParams.WriteOnly = TRUE;
        break;
    default:
        CP_OS_ASSERTMESSAGE("Programmer error, should never get to this case.");
        MT_ERR1(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__);
        return nullptr;
    }

    return m_pOsInterface->pfnLockResource(
                m_pOsInterface,
                &res,
                &sMosLockParams);
}

HRESULT CPavpOsServices::FillResourceMemory(
    MOS_RESOURCE&   sResource,  // [in]
    uint32_t        uiSize,     // [in]
    UCHAR           ucValue)    // [in]

{
    HRESULT hr       = S_OK;
    PVOID   pSurface = nullptr;

    CP_OS_FUNCTION_ENTER;

    do
    {
        if (uiSize == 0)
        {
            // Nothing to do.
            CP_OS_ASSERTMESSAGE("Received uiSize = 0. This should never happen!");
            MT_ERR1(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__);
            break;
        }

        // Lock the surface for writing.
        pSurface = LockResource(sResource, LockType::Write);
        if (pSurface == nullptr)
        {
            CP_OS_ASSERTMESSAGE("Error: Failed locking the resource.");
            hr = E_FAIL;
            MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        // Fill the surface with the wanted value.
        MOS_FillMemory(pSurface, uiSize, ucValue);

    } while (FALSE);

    // Unlock the resource if needed.
    UNLOCK_RESOURCE(&sResource, pSurface);

    CP_OS_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpOsServices::CopyToLinearBufferResource(
    MOS_RESOURCE&   sResource,  // [in]
    PBYTE           pSrc,       // [in]
    uint32_t        srcSize)    // [in]
{
    HRESULT hr = S_OK;
    PVOID   pSurface = nullptr;
    MOS_STATUS eStatus;

    CP_OS_FUNCTION_ENTER;

    do
    {
        if (pSrc == nullptr ||
            srcSize == 0)
        {
            CP_OS_ASSERTMESSAGE("Invalid parameter.");
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }
        if (m_pOsInterface == nullptr ||
            m_pOsInterface->pfnGetResourceInfo == nullptr)
        {
            CP_OS_ASSERTMESSAGE("m_pOsInterface is invalid.");
            hr = E_FAIL;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        MOS_SURFACE resourceInfo = {};
        eStatus = m_pOsInterface->pfnGetResourceInfo(
            m_pOsInterface,
            &sResource,
            &resourceInfo);
        if (MOS_FAILED(eStatus))
        {
            CP_OS_ASSERTMESSAGE("Failed to get resource info. eStatus = 0x%x.", eStatus);
            hr = E_FAIL;
            MT_ERR2(MT_CP_RESOURCE_GET, MT_ERROR_CODE, eStatus, MT_FUNC_RET, hr);
            break;
        }

        if (resourceInfo.Type != MOS_GFXRES_BUFFER ||
            resourceInfo.TileType != MOS_TILE_LINEAR)
        {
            CP_OS_ASSERTMESSAGE("Can only operate on allocations that are Type MOS_GFXRES_BUFFER and TileType MOS_TILE_LINEAR.");
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        pSurface = LockResource(sResource, LockType::Write);
        if (pSurface == nullptr)
        {
            CP_OS_ASSERTMESSAGE("Error: Failed locking the resource.");
            hr = E_FAIL;
            MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        const MOS_STATUS err = MOS_SecureMemcpy(
            pSurface,
            resourceInfo.dwSize,
            pSrc,
            srcSize);
        if (err != MOS_STATUS_SUCCESS)
        {
            CP_OS_ASSERTMESSAGE("SecureCopyMemory failed with error = %d.", err);
            hr = E_FAIL;
            MT_ERR2(MT_CP_MEM_COPY, MT_ERROR_CODE, err, MT_FUNC_RET, hr);
            break;
        }
    } while (FALSE);

    // Unlock the resource if needed.
    UNLOCK_RESOURCE(&sResource, pSurface);

    CP_OS_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpOsServices::FreeResource(PMOS_RESOURCE &pResource) const
{
    HRESULT hr = S_OK;

    CP_OS_FUNCTION_ENTER;

    do
    {
        // Verify that the needed MHAL interface is valid.
        if (m_pOsInterface == nullptr)
        {
            hr = E_UNEXPECTED;
            CP_OS_ASSERTMESSAGE("OS interface is NULL.");
            MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        // Validate input parameters.
        if (pResource == nullptr)
        {
            CP_OS_ASSERTMESSAGE("Invalid parameter.");
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        // Free the resource if needed.
        FREE_RESOURCE(pResource);

        MOS_Delete(pResource);

    } while (FALSE);

    CP_OS_FUNCTION_EXIT(hr);
    return hr;
}

ULONG CPavpOsServices::GetResourceSize(CONST PMOS_RESOURCE pResource)  // [in]
{
    ULONG uiResSize = 0;

    CP_OS_FUNCTION_ENTER;

    // Verify that the resource is not NULL.
    if (pResource              != nullptr &&
        pResource->pGmmResInfo != nullptr)
    {
        //In WYSIWYS test, DecryptionBLT is validated, it will call CopyBufferToResource to do buffer copy;
        //However the dst size call GetSizeMainSurface() but copy size call GetSizeSurface() when copy resource;
        //The copy size will align 64k when local memory due to local memory WA from D3D which makes the copy size larger than dst size;
        //Fix it to change dst size also call GetSizeSurface().
        uiResSize = static_cast<ULONG>(pResource->pGmmResInfo->GetSizeSurface());
    }
    else
    {
        CP_OS_ASSERTMESSAGE("Error: resource is NULL.");
        MT_ERR1(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__);
    }

    CP_OS_FUNCTION_EXIT(0);
    return uiResSize;
}

HRESULT CPavpOsServices::AllocateResource(
    PBYTE           pSrcBuffer,
    PMOS_RESOURCE   pResource,
    SIZE_T          size,
    GMM_GFX_SIZE_T  *pResourceSize,
    Mos_MemPool     memType,
    BOOL            notLockable)
{
    HRESULT                 hr = S_OK;
    INT                     err = 0;
    MOS_ALLOC_GFXRES_PARAMS allocateParams = {};
    MOS_LOCK_PARAMS         lockParams = {};
    PBYTE                   pResourceData = nullptr;
    BOOL                    didAllocateSrcSurf = FALSE;
    MOS_SURFACE             resourceInfo = {};
    MOS_STATUS              eStatus;

    CP_OS_FUNCTION_ENTER;

    do
    {
        if (pResource == nullptr ||
            pResourceSize == nullptr ||
            size == 0)  // Cannot be negative.
        {
            CP_OS_ASSERTMESSAGE("Invalid parameter.");
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        // Verify that the needed MHAL interface is valid.
        if (m_pOsInterface == nullptr ||
            m_pOsInterface->pfnAllocateResource == nullptr ||
            m_pOsInterface->pfnFreeResource == nullptr ||
            m_pOsInterface->pfnGetResourceInfo == nullptr ||
            m_pOsInterface->pfnLockResource == nullptr ||
            m_pOsInterface->pfnUnlockResource == nullptr)
        {
            CP_OS_ASSERTMESSAGE("MHAL interface is invalid.");
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        if (notLockable && 
            memType == Mos_MemPool::MOS_MEMPOOL_SYSTEMMEMORY)
        {
            CP_OS_ASSERTMESSAGE("Notlockable system memory doesn't make sense");
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }
        
        if (notLockable &&
            pSrcBuffer != nullptr)
        {
            CP_OS_ASSERTMESSAGE("Source buffer is not expected by not lockable resource.");
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        // Allocate the source resource.
        allocateParams.Format = Format_Buffer;
        allocateParams.dwBytes = static_cast<DWORD>(size);
        allocateParams.Type = MOS_GFXRES_BUFFER;
        allocateParams.TileType = MOS_TILE_LINEAR;
        allocateParams.pBufName = "CPavpOsServices_Resource";
        allocateParams.Flags.bNotLockable = notLockable ? 1 : 0;
        allocateParams.dwMemType = memType;

        if (MEDIA_IS_SKU(m_pOsInterface->pfnGetSkuTable(m_pOsInterface), FtrSAMediaCachePolicy))
        {
            allocateParams.ResUsageType = MOS_HW_RESOURCE_USAGE_CP_INTERNAL_WRITE;
        }

        eStatus = m_pOsInterface->pfnAllocateResource(
            m_pOsInterface,
            &allocateParams,
            pResource);
        if (MOS_FAILED(eStatus))
        {
            CP_OS_ASSERTMESSAGE("Allocation of the resource failed. eStatus = 0x%x.", eStatus);
            hr = E_FAIL;
            MT_ERR2(MT_CP_RESOURCE_ALLOC, MT_ERROR_CODE, eStatus, MT_FUNC_RET, hr);
            break;
        }
        didAllocateSrcSurf = TRUE;

        // Copy from the CPU to the GPU only if asked to
        if (pSrcBuffer != nullptr)
        {
            // Fill in the src resource with the input buffer.
            lockParams.WriteOnly = 1;
            pResourceData = static_cast<PBYTE>(m_pOsInterface->pfnLockResource(
                m_pOsInterface,
                pResource,
                &lockParams));
            if (pResourceData == nullptr)
            {
                CP_OS_ASSERTMESSAGE("Error locking the resource.");
                hr = E_FAIL;
                MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }

            eStatus = m_pOsInterface->pfnGetResourceInfo(
                m_pOsInterface,
                pResource,
                &resourceInfo);
            if (MOS_FAILED(eStatus))
            {
                CP_OS_ASSERTMESSAGE("Failed to get resource info. eStatus = 0x%x.", eStatus);
                hr = E_FAIL;
                MT_ERR2(MT_CP_RESOURCE_GET, MT_ERROR_CODE, eStatus, MT_FUNC_RET, hr);
                break;
            }

            err = MOS_SecureMemcpy(
                pResourceData,
                size,
                pSrcBuffer,
                size);
            if (err != 0)
            {
                CP_OS_ASSERTMESSAGE("SecureCopyMemory failed with error = %d.", err);
                hr = E_FAIL;
                MT_ERR2(MT_CP_MEM_COPY, MT_ERROR_CODE, err, MT_FUNC_RET, hr);
                break;
            }
            m_pOsInterface->pfnUnlockResource(
                m_pOsInterface,
                pResource);
        }

        *pResourceSize = pResource->pGmmResInfo ? pResource->pGmmResInfo->GetSizeMainSurface() : 0;

        hr = S_OK;

    } while (FALSE);

    if (FAILED(hr) &&
        m_pOsInterface != nullptr &&
        m_pOsInterface->pfnFreeResource != nullptr)
    {
        // Free the allocated resources.
        if (didAllocateSrcSurf)
        {
            m_pOsInterface->pfnFreeResource(m_pOsInterface, pResource);
        }

        pResourceSize = 0;
    }

    CP_OS_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpOsServices::UpdateResourceUsageType(
    PMOS_RESOURCE           pOsResource,
    MOS_HW_RESOURCE_DEF     resUsageType)
{
    HRESULT                 hr          = S_OK;
    MOS_STATUS              eStatus     = MOS_STATUS_SUCCESS;

    CP_OS_FUNCTION_ENTER;

    do
    {
        if (nullptr == pOsResource ||
            nullptr == m_pOsInterface ||
            nullptr == m_pOsInterface->pfnUpdateResourceUsageType)
        {
            hr = E_FAIL;
            break;
        }

        if (!MEDIA_IS_SKU(m_pOsInterface->pfnGetSkuTable(m_pOsInterface), FtrSAMediaCachePolicy))
        {
            CP_OS_NORMALMESSAGE("Stand Alone media cache policy is not supported");
            break;
        }

        eStatus = m_pOsInterface->pfnUpdateResourceUsageType(pOsResource, resUsageType);
        if (MOS_FAILED(eStatus))
        {
            CP_OS_ASSERTMESSAGE("Failed to update reource gmm usage type. eStatus = 0x%x.", eStatus);
            hr = E_FAIL;
            MT_ERR2(MT_CP_RESOURCE_UPDATE, MT_ERROR_CODE, eStatus, MT_FUNC_RET, hr);
            break;
        }
    } while (FALSE);

    CP_OS_FUNCTION_EXIT(hr);
    return hr;

}

HRESULT CPavpOsServices::DeallocateResource(
    PBYTE           pDstBuffer,
    PMOS_RESOURCE   pResource,
    SIZE_T          size)
{
    HRESULT             hr = S_OK;
    INT                 err = 0;
    MOS_LOCK_PARAMS     lockParams = {};
    PBYTE               pResourceData = nullptr;
    MOS_SURFACE         resourceInfo = {};
    MOS_STATUS          eStatus;

    CP_OS_FUNCTION_ENTER;

    do
    {
        if (pResource == nullptr)
        {
            CP_OS_ASSERTMESSAGE("Invalid parameter.");
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        // Verify that the needed MHAL interface is valid.
        if (m_pOsInterface == nullptr ||
            m_pOsInterface->pfnGetResourceInfo == nullptr ||
            m_pOsInterface->pfnLockResource == nullptr ||
            m_pOsInterface->pfnUnlockResource == nullptr)
        {
            CP_OS_ASSERTMESSAGE("MHAL interface is invalid.");
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        // Copy from the GPU to the CPU only if asked to
        if (pDstBuffer != nullptr)
        {
            // Fill in the output buffer with the output data.
            lockParams.ReadOnly = 1;
            pResourceData = static_cast<PBYTE>(m_pOsInterface->pfnLockResource(
                m_pOsInterface,
                pResource,
                &lockParams));
            if (pResourceData == nullptr)
            {
                CP_OS_ASSERTMESSAGE("Error locking the destination resource.");
                hr = E_FAIL;
                MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }

            eStatus = m_pOsInterface->pfnGetResourceInfo(
                m_pOsInterface,
                pResource,
                &resourceInfo);
            if (MOS_FAILED(eStatus))
            {
                CP_OS_ASSERTMESSAGE("Failed to get resource info. eStatus = 0x%x.", eStatus);
                hr = E_FAIL;
                MT_ERR2(MT_CP_RESOURCE_GET, MT_ERROR_CODE, eStatus, MT_FUNC_RET, hr);
                break;
            }

            err = MOS_SecureMemcpy(
                pDstBuffer,
                size,
                pResourceData,
                size);
            if (err != 0)
            {
                CP_OS_ASSERTMESSAGE("SecureCopyMemory failed with error = %d.", err);
                hr = E_FAIL;
                MT_ERR2(MT_CP_MEM_COPY, MT_ERROR_CODE, err, MT_FUNC_RET, hr);
                break;
            }

            // Unlock the surface.
            m_pOsInterface->pfnUnlockResource(
                m_pOsInterface,
                pResource);
        }

        hr = S_OK;
    } while (FALSE);

    // Free the allocated resources.
    if (pResource != nullptr &&
        m_pOsInterface != nullptr &&
        m_pOsInterface->pfnFreeResource != nullptr)
    {
        m_pOsInterface->pfnFreeResource(m_pOsInterface, pResource);
    }

    CP_OS_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpOsServices::PrepareDecryptionResources(
    PBYTE           pSrcBuffer,
    SIZE_T          bufSize,
    PMOS_RESOURCE   pSrcResource,
    PMOS_RESOURCE   pDstResource,
    GMM_GFX_SIZE_T  *pResourceSize,
    BOOL            isDstResHwWriteOnly)
{
    HRESULT                 hr = S_OK;
    BOOL                    didAllocateSrcSurf = FALSE;
    Mos_MemPool             memType = Mos_MemPool::MOS_MEMPOOL_VIDEOMEMORY;

    CP_OS_FUNCTION_ENTER;

    do
    {
        if (pSrcResource == nullptr ||
            pDstResource == nullptr ||
            pResourceSize == nullptr ||
            pSrcBuffer == nullptr ||
            bufSize == 0) // Cannot be negative.
        {
            CP_OS_ASSERTMESSAGE("Invalid parameter.");
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        // Allocate the source resource and copy from CPU to GPU
        hr = AllocateResource(pSrcBuffer, pSrcResource, bufSize, pResourceSize);
        if (FAILED(hr))
        {
            CP_OS_ASSERTMESSAGE("Could not allocate source resource.");
            MT_ERR2(MT_CP_RESOURCE_ALLOC, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }
        didAllocateSrcSurf = TRUE;

        // Allocate the destination resource
        if(isDstResHwWriteOnly){
            memType = Mos_MemPool::MOS_MEMPOOL_DEVICEMEMORY;
        }
        hr = AllocateResource(nullptr, pDstResource, bufSize, pResourceSize, memType, isDstResHwWriteOnly);
        if (FAILED(hr))
        {
            CP_OS_ASSERTMESSAGE("Could not allocate destination resource.");
            MT_ERR2(MT_CP_RESOURCE_ALLOC, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        *pResourceSize = pDstResource->pGmmResInfo ? (pDstResource->pGmmResInfo->GetSizeMainSurface()) : 0;

        hr = S_OK;

    } while (FALSE);

    if (FAILED(hr) &&
        m_pOsInterface != nullptr &&
        m_pOsInterface->pfnFreeResource != nullptr)
    {
        // Free the allocated resources.
        if (didAllocateSrcSurf)
        {
            m_pOsInterface->pfnFreeResource(m_pOsInterface, pSrcResource);
        }

        pResourceSize = 0;
    }

    CP_OS_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpOsServices::ReleaseDecryptionResources(
    PMOS_RESOURCE   pSrcResource,
    PMOS_RESOURCE   pDstResource,
    SIZE_T          bufSize,
    PBYTE           pDstBuffer)
{
    HRESULT             hr = S_OK;
    INT                 err = 0;
    MOS_LOCK_PARAMS     lockParams = {};
    PBYTE               pResourceData = nullptr;
    MOS_SURFACE         resourceInfo = {};

    CP_OS_FUNCTION_ENTER;

    do
    {
        if (pSrcResource == nullptr ||
            pDstResource == nullptr ||
            pDstBuffer == nullptr)
        {
            CP_OS_ASSERTMESSAGE("Invalid parameter.");
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        hr = DeallocateResource(nullptr, pSrcResource, bufSize);
        if (FAILED(hr))
        {
            CP_OS_ASSERTMESSAGE("Failed to deallocate source resource.");
            MT_ERR2(MT_CP_RESOURCE_DEALLOC, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            // must attempt to free resources, don't break
        }

        hr = DeallocateResource(pDstBuffer, pDstResource, bufSize);
        if (FAILED(hr))
        {
            CP_OS_ASSERTMESSAGE("Failed to deallocate destination resource.");
            MT_ERR2(MT_CP_RESOURCE_DEALLOC, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            // must attempt to free resources, don't break
        }

        hr = S_OK;
    } while (FALSE);

    CP_OS_FUNCTION_EXIT(hr);
    return hr;
}

ULONG CPavpOsServices::GetResourcePitch(CONST PMOS_RESOURCE pResource)  // [in]
{
    ULONG uiResPitch = 0;

    CP_OS_FUNCTION_ENTER;

    // Verify that the resource is not NULL.
    if (pResource != nullptr &&
        pResource->pGmmResInfo != nullptr)
    {
        uiResPitch = static_cast<ULONG>(pResource->pGmmResInfo->GetRenderPitch());
    }
    else
    {
        CP_OS_ASSERTMESSAGE("Error: resource is NULL.");
        MT_ERR1(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__);
    }

    CP_OS_FUNCTION_EXIT(0);
    return uiResPitch;
}
