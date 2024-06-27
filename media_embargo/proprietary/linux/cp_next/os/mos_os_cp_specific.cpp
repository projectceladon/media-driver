/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2009-2020
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

======================= end_copyright_notice ==================================*/
//!
//! \file     mos_os_cp_specific.c
//! \brief    OS specific implement for CP related functions
//!

#include "mos_os_cp_specific.h"
#include "mos_os.h"
#include "mos_util_debug.h"
#include "mhw_cp.h"
#include "pavp_types.h"
#include "pxp_statemachine.h"
#include "mos_pxp_sm.h"
#include "cp_os_services.h"

MosCpInterface* Create_MosCp(void *osInterface)
{
    return MOS_New(MosCp, osInterface);
}

void Delete_MosCp(MosCpInterface* pMosCpInterface)
{
    MOS_Delete(pMosCpInterface);
}

MosCp::MosCp(void  *osInterface)
{
    m_osInterface     = static_cast<PMOS_INTERFACE>(osInterface);
    m_cpContext.value = 0;
    m_flushInst       = 0;
}

MosCp::~MosCp()
{
    m_osInterface     = nullptr;
    m_cpContext.value = 0;
}

MOS_STATUS MosCp::RegisterPatchForHM(
    uint32_t                           *pPatchAddress,
    uint32_t                           bWrite,
    MOS_HW_COMMAND                     HwCommandType,
    uint32_t                           forceDwordOffset,
    void                               *plResource,
    void                               *patchLocationList)
{
    MOS_STATUS                  eStatus                     = MOS_STATUS_SUCCESS;

    MOS_UNUSED(pPatchAddress);
    MOS_UNUSED(plResource);

    MOS_OS_CHK_NULL_RETURN(m_osInterface);

    MOS_CP_COMMAND_PROPERTIES cpCmdProps;
    MOS_ZeroMemory(&cpCmdProps, sizeof(cpCmdProps));

    if (IsHMEnabled())
    {
        PLATFORM Platform;
        m_osInterface->pfnGetPlatform(m_osInterface, &Platform);
        MOS_OS_CHK_STATUS_RETURN(MhwCpBase::InitCpCmdProps(
            Platform,
            HwCommandType,
            &cpCmdProps,
            forceDwordOffset));

        // In some case, HW write resource as clear. Use WRITE_WA to identify this issue
        // Case example: Tile record is written as clear by HW in secure encode scenario
        bool writeAsSecure = (bWrite == WRITE_WA) ? false : bWrite;
        cpCmdProps.writeAsSecure =
            (cpCmdProps.HasDecryptBits || cpCmdProps.ForceEncryptionForNoMocs) ? writeAsSecure : 0;
    }

    auto pPatchLocationList = static_cast<PPATCHLOCATIONLIST>(patchLocationList);

    pPatchLocationList->cpCmdProps = cpCmdProps.Value;

    return  MOS_STATUS_SUCCESS;
}

MOS_STATUS MosCp::PermeatePatchForHM(
    void    *virt,
    void    *pvCurrentPatch,
    void    *resource)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    PPATCHLOCATIONLIST  pCurrentPatch = static_cast<PPATCHLOCATIONLIST>(pvCurrentPatch);

    MOS_OS_CHK_NULL_RETURN(pCurrentPatch);
    MOS_OS_CHK_NULL_RETURN(m_osInterface);
    MOS_OS_CHK_NULL_RETURN(m_osInterface->pOsContext);

    MOS_CP_COMMAND_PROPERTIES cpCmdProps;
    cpCmdProps.Value = pCurrentPatch->cpCmdProps;

    // This is the resource for which patching will be done
    auto pResource = static_cast<PMOS_RESOURCE>(resource);
    MOS_OS_CHK_NULL_RETURN(pResource);

    if (nullptr != pResource->pGmmResInfo)
    {
        // patch cpCmdProps and surface tracking for CP
        if (IsHMEnabled())
        {
            uint32_t targetTag = m_cpContext.heavyMode ? m_cpContext.value : 0;

            if ((cpCmdProps.HasDecryptBits || cpCmdProps.ForceEncryptionForNoMocs) &&
                cpCmdProps.writeAsSecure)
            {
                pResource->pGmmResInfo->GetSetCpSurfTag(true, targetTag);
            }
            CpContext ctx = {0};
            ctx.value = pResource->pGmmResInfo->GetSetCpSurfTag(false, 0);

            if (cpCmdProps.HasDecryptBits && ctx.heavyMode)
            {
                if (cpCmdProps.DecryptBitDwordOffset == 0)
                {
                    pCurrentPatch->AllocationOffset |= (1 << cpCmdProps.DecryptBitNumber);
                }
                else
                {
                    *((uint32_t *)((uint8_t *)virt + pCurrentPatch->PatchOffset) + cpCmdProps.DecryptBitDwordOffset) |=
                            (1 << cpCmdProps.DecryptBitNumber);
                }
            }
        }
        else
        {
            pResource->pGmmResInfo->GetSetCpSurfTag(true, 0);
        }
        // End patch cpCmdProps and surface tracking for CP
    }

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MosCp::PermeateBBPatchForHM()
{
    // do nothing
    return  MOS_STATUS_SUCCESS;
}

//Surface tracking via arbitrary session related code will be cleaned up at later stage.
//https://jira.devtools.intel.com/browse/VSMGWL-28075
void MosCp::PrepareCpTagCache(void)
{
    if (m_flushInst != m_cpContext.instanceId)
    {
        auto it = m_surfTags.begin();
        while (it != m_surfTags.end())
        {
            CpContext ctx;
            ctx.value = it->second;
            // to simplify flush logic, remove all node with instance < current instance-2
            // the flush is requred because, hwc mos instance is alive across all secure playback.
            // this cache will grow unlimited size if have no flush.
            int step = (ctx.instanceId < m_cpContext.instanceId)?
                        m_cpContext.instanceId - ctx.instanceId:
                        ctx.instanceId - m_cpContext.instanceId;
            if (step > 2)
            {
                m_surfTags.erase(it++);
            }
            else
            {
                ++it;
            }
        }
        m_flushInst = m_cpContext.instanceId;
    }
}

bool MosCp::UpdateCpTagCache(int32_t handle, PGMM_RESOURCE_INFO pGmmResInfo)
{
    bool teardown = false;
    CpContext ctx;
    uint32_t save;

    if (pGmmResInfo == nullptr)
    {
        MOS_OS_ASSERTMESSAGE("invalid parameters");
        return true;
    }
    save = pGmmResInfo->GetSetCpSurfTag(false, 0);
    ctx.value = save;

    auto it = m_surfTags.find(handle);
    if (it != m_surfTags.end())
    {
        ctx.value = it->second; // use value in cache
    }
    // check whether teardown happened.
    if (ctx.instanceId != 0 && ctx.instanceId != m_cpContext.instanceId)
    {
        MOS_OS_ASSERTMESSAGE("teardown detected, current =%x, surface =%x", m_cpContext.value, ctx.value);
        teardown = true;
    }
    ctx.instanceId = m_cpContext.instanceId;
    // always add/update to cache, so that this frame will be handle in normal in next time.
    m_surfTags[handle] = ctx.value;

    if (ctx.value != save)
    {
        pGmmResInfo->GetSetCpSurfTag(true, ctx.value);
    }
    return teardown;
}

MOS_STATUS MosCp::PrepareResources(
    void               *source[],
    uint32_t           sourceCount,
    void               *target[],
    uint32_t           targetCount)
{
    bool bHardwareProtection = false;
    bool bTeardownDetected   = false;
    PMOS_RESOURCE  *ppSource = reinterpret_cast<PMOS_RESOURCE*>(source);
    PMOS_RESOURCE  *ppTarget = reinterpret_cast<PMOS_RESOURCE*>(target);

    MOS_OS_ASSERT(m_osInterface);

    if ((ppSource == nullptr && sourceCount > 0) ||
        (ppTarget == nullptr && targetCount > 0))
    {
        MOS_OS_ASSERTMESSAGE("invalid parameters");
        return MOS_STATUS_INVALID_PARAMETER;
    }

    // in clear view, there is no source, only render target.
    // pick up surface protected state from target in this case.
    if (sourceCount == 0)
    {
        ppSource    = ppTarget;
        sourceCount = targetCount;
    }

    for (uint32_t uiIndex = 0; uiIndex < sourceCount; uiIndex++)
    {
        // as long as any of the resource is marked as hardware protection state
        // driver should enable hardware protection.
        if ( ( nullptr != ppSource[uiIndex] ) &&
               nullptr != ppSource[uiIndex]->pGmmResInfo &&
               ppSource[uiIndex]->pGmmResInfo->GetSetCpSurfTag(false, 0) != 0)
        {
            m_cpContext.value = ppSource[uiIndex]->pGmmResInfo->GetSetCpSurfTag(false, 0);
            bHardwareProtection = true;
            break;
        }
    }

    if (!bHardwareProtection)
    {
        for (uint32_t uiIndex = 0; uiIndex < targetCount; uiIndex++)
        {
            // for clear input, driver should also enable hardware protection by target
            if ((nullptr != ppTarget[uiIndex]) &&
                nullptr != ppTarget[uiIndex]->pGmmResInfo &&
                ppTarget[uiIndex]->pGmmResInfo->GetSetCpSurfTag(false, 0) != 0)
            {
                m_cpContext.value   = ppTarget[uiIndex]->pGmmResInfo->GetSetCpSurfTag(false, 0);
                bHardwareProtection = true;
                break;
            }
        }
    }

    // clear cp context if cp not required.
    if (!bHardwareProtection)
    {
        m_cpContext.value = 0;
    }
    else
    {
        PrepareCpTagCache();
        for (uint32_t uiIndex = 0; uiIndex < sourceCount; uiIndex++)
        {
            // process each input surface for cptag cache
            if ( ( nullptr != ppSource[uiIndex] ) &&
                   nullptr != ppSource[uiIndex]->pGmmResInfo &&
                   nullptr != ppSource[uiIndex]->bo &&
                   ppSource[uiIndex]->pGmmResInfo->GetSetCpSurfTag(false, 0) != 0)
            {
                bTeardownDetected |= UpdateCpTagCache(ppSource[uiIndex]->bo->handle, ppSource[uiIndex]->pGmmResInfo);
            }
        }
    }

    return bTeardownDetected? MOS_STATUS_INVALID_HANDLE : MOS_STATUS_SUCCESS;
}

bool MosCp::IsCpEnabled()
{
    return m_cpContext.enable;
}

void MosCp::SetCpEnabled(
    bool          bCpInUse)
{
    m_cpContext.enable = bCpInUse;
}

void MosCp::UpdateCpContext(
    uint32_t    value)
{
    m_cpContext.value = value;
}

bool MosCp::IsHMEnabled()
{
    return (m_cpContext.enable && m_cpContext.heavyMode);
}

bool MosCp::IsTSEnabled()
{
    return (IsCpEnabled() && (m_cpContext.sessionId & TRANSCODE_APP_ID_MASK));
}

bool MosCp::IsIDMEnabled()
{
    return (m_cpContext.enable && m_cpContext.isolatedDecodeMode);
}

void MosCp::RegisterAndCheckProtectedGemCtx(bool bRegister, void *identifier, bool *bIsStale)
{
    std::map<void*, CpContext>::iterator it = m_protectedgemctxmap.find(identifier);

    do
    {
        if (bRegister)
        {
            if (it != m_protectedgemctxmap.end())
            {
                m_protectedgemctxmap[identifier] = m_cpContext;
            }
            else
            {
                m_protectedgemctxmap.insert({identifier, m_cpContext});
            }
        }
        else // check if context is valid
        {
            if (bIsStale == nullptr)
            {
                MOS_OS_ASSERTMESSAGE("Input parameter is null");
                break;
            }
            if (it != m_protectedgemctxmap.end() &&
                it->second.value == m_cpContext.value)
            {
                *bIsStale = false;
            }
            else
            {
                *bIsStale = true;
            }
        }
    } while (false);
}

bool MosCp::IsSMEnabled()
{
    return (m_cpContext.enable && m_cpContext.stoutMode);
}

bool MosCp::IsCencCtxBasedSubmissionEnabled()
{
    return true; //PXP stack on Linux is POR from TGL+
}

bool MosCp::IsTearDownHappen()
{
    return m_cpContext.tearDown;
}

uint32_t MosCp::GetSessionId()
{
    uint32_t uSessionId = 0xFF;

    if(m_cpContext.enable)
    {
        uSessionId = m_cpContext.sessionId;
    }

    return uSessionId;
}

bool MosCp::RenderBlockedFromCp()
{
    MOS_OS_ASSERT(m_osInterface);

    bool bStatus = false;
    if (m_osInterface)
    {
        bool isPavpStoutTranscode = IsSMEnabled() && (m_cpContext.sessionId & 0x80);
        if(isPavpStoutTranscode)
        {
            bStatus = false; //Do not have kernel for stout transcode, will remove once it ready.
        }
        else
        {
            bStatus = IsIDMEnabled() || IsSMEnabled();
        }
    }

    return bStatus;
}

bool MosCp::GetResourceEncryption(
    void          *pvResource)
{
    PMOS_RESOURCE     pResource = static_cast<PMOS_RESOURCE>(pvResource);
    MOS_STATUS          eStatus = MOS_STATUS_SUCCESS;
    bool                 retVal = false;
    CpContext               tag = {0};

    MOS_OS_CHK_NULL(pResource);
    MOS_OS_CHK_NULL(pResource->pGmmResInfo);

    tag.value = pResource->pGmmResInfo->GetSetCpSurfTag(false, 0);
    if (tag.enable && tag.heavyMode)
    {
        retVal = true;
    }

finish:
    return retVal;
}

MOS_STATUS MosCp::SetResourceEncryption(
    void          *pvResource,
    bool          bEncryption)
{
    PMOS_RESOURCE pResource = static_cast<PMOS_RESOURCE>(pvResource);
    MOS_STATUS    eStatus   = MOS_STATUS_SUCCESS;

    MOS_OS_CHK_NULL_RETURN(pResource);
    MOS_OS_CHK_NULL_RETURN(pResource->pGmmResInfo);

    if (bEncryption)
    {
        //Set cp tag for secure input surface when it is not be set
        uint32_t value = pResource->pGmmResInfo->GetSetCpSurfTag(false, 0);
        if(value == 0)
        {
            pResource->pGmmResInfo->GetSetCpSurfTag(true, CP_SURFACE_TAG);
        }
    }
    else
    {
        pResource->pGmmResInfo->GetSetCpSurfTag(true, 0);  //clear surface CpTag to 0
    }
    return eStatus;
}

MOS_STATUS MosCp::AllocateTEEPhysicalBuffer(
    void        **ppGSCBuffer,
    uint32_t    *pBufferSize)
{
    MOS_STATUS              eStatus = MOS_STATUS_SUCCESS;
    do
    {
        if (pBufferSize == nullptr)
        {
            eStatus = MOS_STATUS_INVALID_PARAMETER;
            break;
        }
        // for windows KMD write to this location after allocating memory,
        *pBufferSize = GSC_MAX_INPUT_BUFF_SIZE + GSC_MAX_OUTPUT_BUFF_SIZE;

        *ppGSCBuffer = MOS_AllocAndZeroMemory(*pBufferSize);
        if (*ppGSCBuffer == nullptr)
        {
            eStatus = MOS_STATUS_NULL_POINTER;
        }

    } while(false);

    return eStatus;
}

MOS_STATUS MosCp::DeAllocateTEEPhysicalBuffer(
    void     *pGSCBuffer)
{
    MOS_STATUS              eStatus = MOS_STATUS_SUCCESS;
    MOS_FreeMemory(pGSCBuffer);

    return eStatus;
}

void MosCp::SetTK(
    uint32_t        *pTKs,
    uint32_t        uiTKsSize)
{
    MOS_UNUSED(pTKs);
    MOS_UNUSED(uiTKsSize);
    MOS_OS_VERBOSEMESSAGE("Not yet implemented."); // Use VERBOSEMESSAGE due to ULT will call into it.
}

MOS_STATUS MosCp::GetTK(
    uint32_t        **ppTKs,
    uint32_t        *pTKsSize,
    uint32_t        *pTKsUpdateCnt)
{
    MOS_UNUSED(ppTKs);
    MOS_UNUSED(pTKsSize);
    MOS_UNUSED(pTKsUpdateCnt);
    MOS_OS_ASSERTMESSAGE("Not yet implemented.");
    return MOS_STATUS_UNKNOWN;
}

void* MosCp::GetOcaDumper()
{
    MOS_OS_ASSERTMESSAGE("Not yet implemented.");
    return nullptr;
}

MOS_STATUS MosCp::CreateOcaDumper()
{
    MOS_OS_ASSERTMESSAGE("Not yet implemented.");
    return MOS_STATUS_UNKNOWN;
}

bool MosCp::CpRegisterAccessible()
{
    if (IsHMEnabled())
    {
        PLATFORM Platform;
        m_osInterface->pfnGetPlatform(m_osInterface, &Platform);

        // PAVP register is not HW whitelisted on Gen11+ Linux
        return (Platform.eRenderCoreFamily >= IGFX_GEN11_CORE)? false : true;
    }
    return false;
}
