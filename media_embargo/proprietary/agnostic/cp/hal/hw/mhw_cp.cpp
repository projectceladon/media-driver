/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2014 - 2021
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
//! \file     mhw_cp.cpp
//! \brief    MHW interface for content protection
//! \details  Impelements the functionalities across all platforms for content protection
//!

#include "mhw_cp.h"
#include "mhw_cp_hwcmd_common.h"
#include "mhw_mi.h"
#include "mhw_mi_itf.h"
#include "mhw_vdbox.h"
#include "mhw_vdbox_mfx_interface.h"

#define PAVP_SESSION_ID_MASK 0xFF
#define CODECHAL_ENCODE_STATUS_NUM 512

const std::map<MOS_HW_COMMAND, DecryptBitOffset> *MhwCpBase::m_cpCmdPropsMap = nullptr;

MhwCpInterface* Create_MhwCp(PMOS_INTERFACE   osInterface)
{
    MOS_STATUS eStatus = MOS_STATUS_UNKNOWN;
    MHW_CP_FUNCTION_ENTER;
    MhwCpInterface* ret = nullptr;

    if (osInterface == nullptr)
    {
        eStatus = MOS_STATUS_NULL_POINTER;
    }

    ret  = MhwCpBase::CpFactory(osInterface, eStatus);

    MOS_FUNCTION_EXIT(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW, eStatus);

    return ret; 
}

void Delete_MhwCp(MhwCpInterface* pMhwCpInterface)
{
    MOS_Delete(pMhwCpInterface);
}

MhwCpBase::MhwCpBase(MOS_INTERFACE& osInterface) : m_hwUnit(nullptr),
                                                   m_mhwMiInterface(nullptr),
                                                   m_sizeOfCmdIndirectCryptoState(0),
                                                   m_userEncryptionParams(nullptr),
                                                   m_osInterface(osInterface)
{
    MOS_ZeroMemory(&m_encryptionParams, sizeof(MHW_PAVP_ENCRYPTION_PARAMS));
    m_encryptionParams.PavpEncryptionType  = CP_SECURITY_NONE;
    m_encryptionParams.HostEncryptMode     = CP_TYPE_NONE;
}

MhwCpBase::~MhwCpBase()
{
    if (this->m_hwUnit)
    {
        for (int i = 0; i < PAVP_HW_UNIT_MAX_COUNT; ++i)
        {
            MOS_Delete(this->m_hwUnit[i]);
        }
        MOS_DeleteArray(this->m_hwUnit);
        this->m_hwUnit = nullptr;
    }
}

MhwCpInterface* MhwCpBase::CpFactory(PMOS_INTERFACE osInterface, MOS_STATUS& eStatus)
{
    MhwCpInterface* cpInterface = nullptr;
    PLATFORM        platform;

    MHW_CP_FUNCTION_ENTER;

    eStatus = MOS_STATUS_UNKNOWN;

    do
    {
        if (osInterface == nullptr)
        {
            eStatus = MOS_STATUS_NULL_POINTER;
            break;
        }

        try
        {
            osInterface->pfnGetPlatform(osInterface, &platform);
            cpInterface = MhwCpFactory::Create(platform.eProductFamily, eStatus, *osInterface);
        }
        catch (const std::exception &err)
        {
            MOS_ASSERTMESSAGE(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW, "Caught exception creating the generation-specifc PAVP HW Interface: %s.", err.what());
            eStatus = MOS_STATUS_UNKNOWN;
            MT_ERR2(MT_CP_MHW_INTERFACE_CREATE_FAIL, MT_COMPONENT, MOS_COMPONENT_CP, MT_SUB_COMPONENT, MOS_CP_SUBCOMP_MHW);
            break;
        }

        if (cpInterface == nullptr || eStatus != MOS_STATUS_SUCCESS)
        {
            MOS_ASSERTMESSAGE(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW, "Allocation failed. eStatus = %d", eStatus);
            MT_ERR3(MT_CP_MHW_ALLOCATION_FAIL, MT_COMPONENT, MOS_COMPONENT_CP, MT_SUB_COMPONENT, MOS_CP_SUBCOMP_MHW, MT_ERROR_CODE, eStatus);
            // if the failure is caused by some internal task of HW units,
            // Pavp might be pointing an object with bad data members.
            // so it's better to claim the whole allocation failed and delete the object created
            MOS_Delete(cpInterface);  // ok to delete nullptr pointer
            cpInterface = nullptr;
            break;
        }

    } while (false);

    MOS_FUNCTION_EXIT(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW, eStatus);
    return cpInterface;
}

MOS_STATUS MhwCpBase::AddProlog(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(cmdBuffer);

    MosCp *pMosCp = dynamic_cast<MosCp*>(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(pMosCp);

    MHW_PAVP_PROLOG_TRACE_DATA eventData = 
    {
        osInterface->pfnGetGpuContext(osInterface),
        osInterface->osCpInterface->IsCpEnabled(),
        m_encryptionParams.CpTag,
    };
    MOS_TraceEventExt(EVENT_MHW_PROLOG, EVENT_TYPE_INFO, &cmdBuffer->pCmdBase, sizeof(uint32_t), &eventData, sizeof(eventData));

    if (osInterface->osCpInterface->IsCpEnabled() == false)
    {
        // the operation is not needed
        return MOS_STATUS_SUCCESS;
    }

    // Tracking for Epilog/Prolog location in command buffer
    uint32_t *pLocationAddress = (uint32_t*)cmdBuffer->pCmdPtr;
    uint32_t offset           = cmdBuffer->iOffset;

    auto hwUnit = MhwCpBase::getHwUnit(osInterface);
    MHW_CP_CHK_NULL(hwUnit);
    MHW_CP_CHK_STATUS(hwUnit->AddProlog(osInterface, cmdBuffer));

    MHW_CP_FUNCTION_EXIT(eStatus);
    return eStatus;
}

MOS_STATUS MhwCpBase::AddEpilog(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    // Tracking for Epilog/Prolog location in command buffer
    uint32_t *pLocationAddress = nullptr;
    uint32_t offset           = 0;

    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(cmdBuffer);

    MOS_TraceEventExt(EVENT_MHW_EPILOG, EVENT_TYPE_INFO, &cmdBuffer->pCmdBase, sizeof(uint32_t), nullptr, 0);

    if (osInterface->osCpInterface->IsCpEnabled() == false)
    {
        // the operation is not needed
        return MOS_STATUS_SUCCESS;
    }

    pLocationAddress = (uint32_t*)cmdBuffer->pCmdPtr;
    offset           = cmdBuffer->iOffset;

    auto hwUnit = MhwCpBase::getHwUnit(osInterface);
    MHW_CP_CHK_NULL(hwUnit);
    MHW_CP_CHK_STATUS(hwUnit->AddEpilog(osInterface, cmdBuffer));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpBase::AddMiSetAppId(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer,
    uint32_t            appId)
{
    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(cmdBuffer);
    auto hwUnit = MhwCpBase::getHwUnit(osInterface);
    MHW_CP_CHK_NULL(hwUnit);
    MHW_CP_CHK_STATUS(hwUnit->AddMiSetAppId(osInterface, cmdBuffer, appId));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpBase::AddCheckForEarlyExit(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(cmdBuffer);

    auto hwUnit = MhwCpBase::getHwUnit(osInterface);
    MHW_CP_CHK_NULL(hwUnit);
    MHW_CP_CHK_STATUS(hwUnit->AddCheckForEarlyExit(osInterface, cmdBuffer));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpBase::UpdateStatusReportNum(
    uint32_t            cencBufIndex,
    uint32_t            statusReportNum,
    PMOS_RESOURCE       resource,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    MHW_MI_STORE_DATA_PARAMS         storeDataParams;
    MHW_MI_FLUSH_DW_PARAMS           flushDwParams;
    MHW_PAVP_CRYPTO_COPY_ADDR_PARAMS cryptoCopyAddrParams;
    MHW_PAVP_CRYPTO_COPY_PARAMS      cryptoCopyParams;
    uint32_t                         baseOffset;
    uint8_t*                         currEncryptedStatusOffset;
    MOS_STATUS                       eStatus = MOS_STATUS_SUCCESS;

    baseOffset =
        MHW_CACHELINE_SIZE * 3 * cencBufIndex;

    MOS_ZeroMemory(&storeDataParams, sizeof(storeDataParams));
    storeDataParams.pOsResource      = resource;
    storeDataParams.dwResourceOffset = baseOffset;
    storeDataParams.dwValue = statusReportNum + 1;  // +1 to force only non-zero values

    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiStoreDataImmCmd(cmdBuffer, &storeDataParams));

    MOS_ZeroMemory(&flushDwParams, sizeof(flushDwParams));
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiFlushDwCmd(
        cmdBuffer,
        &flushDwParams));

    MOS_ZeroMemory(&cryptoCopyAddrParams, sizeof(cryptoCopyAddrParams));
    cryptoCopyAddrParams.bIsDestRenderTarget = true;
    cryptoCopyAddrParams.dwSize              = MHW_CACHELINE_SIZE * 3 * CENC_NUM_DECRYPT_SHARED_BUFFERS;
    cryptoCopyAddrParams.presSrc             = resource;
    cryptoCopyAddrParams.presDst             = resource;
    MHW_CP_CHK_STATUS(AddMfxCryptoCopyBaseAddr(cmdBuffer, &cryptoCopyAddrParams));

    MOS_ZeroMemory(&cryptoCopyParams, sizeof(cryptoCopyParams));
    cryptoCopyParams.dwCopySize       = sizeof(uint32_t);
    cryptoCopyParams.dwSrcBaseOffset  = baseOffset;
    cryptoCopyParams.dwDestBaseOffset = baseOffset + MHW_CACHELINE_SIZE;
    if (m_osInterface.osCpInterface->IsHMEnabled())
    {
        cryptoCopyParams.ui8SelectionForIndData = CRYPTO_COPY_TYPE_CLEAR_TO_SERPENT_ENCRYPT;  //Input is in clear, output to be Serpent mode encrypted
    }

    MHW_CP_CHK_STATUS(AddMfxCryptoCopy(cmdBuffer, &cryptoCopyParams));

    return eStatus;
}

MOS_STATUS MhwCpBase::CheckStatusReportNum(
    void                        *mfxRegisters,
    uint32_t                    cencBufIndex,
    PMOS_RESOURCE               resource,
    PMOS_COMMAND_BUFFER         cmdBuffer)
{
    MHW_CP_FUNCTION_ENTER;

    MHW_MI_FLUSH_DW_PARAMS                          flushDwParams;
    MHW_PAVP_CRYPTO_COPY_ADDR_PARAMS                cryptoCopyAddrParams;
    MHW_PAVP_CRYPTO_COPY_PARAMS                     cryptoCopyParams;
    MHW_MI_CONDITIONAL_BATCH_BUFFER_END_PARAMS      conditionalBatchBufferEndParams;
    MHW_MI_STORE_REGISTER_MEM_PARAMS                registerMemParams;
    MHW_MI_LOAD_REGISTER_IMM_PARAMS                 registerImmParams;
    MHW_MI_COPY_MEM_MEM_PARAMS                      copyMemMemParams;
    MHW_MI_ATOMIC_PARAMS                            atomicParams;
    MHW_MI_STORE_DATA_PARAMS                        storeDataParams;
    MOS_STATUS                                      eStatus = MOS_STATUS_SUCCESS;

    MHW_CP_CHK_NULL(mfxRegisters);
    MHW_CP_CHK_NULL(cmdBuffer);

    auto mmioRegisters = (MmioRegistersMfx *)mfxRegisters;

    uint32_t baseOffset =
        MHW_CACHELINE_SIZE *
        3 *
        cencBufIndex;

    MOS_ZeroMemory(&cryptoCopyAddrParams, sizeof(cryptoCopyAddrParams));
    cryptoCopyAddrParams.bIsDestRenderTarget = true;
    cryptoCopyAddrParams.dwSize = MHW_CACHELINE_SIZE * 3 * CENC_NUM_DECRYPT_SHARED_BUFFERS;
    cryptoCopyAddrParams.presSrc = resource;
    cryptoCopyAddrParams.presDst = resource;
    MHW_CP_CHK_STATUS(this->AddMfxCryptoCopyBaseAddr(cmdBuffer, &cryptoCopyAddrParams));

    MOS_ZeroMemory(&cryptoCopyParams, sizeof(cryptoCopyParams));
    cryptoCopyParams.dwCopySize = sizeof(uint32_t);
    cryptoCopyParams.dwSrcBaseOffset = baseOffset;
    cryptoCopyParams.dwDestBaseOffset = baseOffset + (MHW_CACHELINE_SIZE * 2);
    if (m_osInterface.osCpInterface->IsHMEnabled())
    {
        cryptoCopyParams.ui8SelectionForIndData = CRYPTO_COPY_TYPE_CLEAR_TO_SERPENT_ENCRYPT; //Input is in clear, output to be Serpent mode encrypted
    }

    MHW_CP_CHK_STATUS(this->AddMfxCryptoCopy(cmdBuffer, &cryptoCopyParams));

    MOS_ZeroMemory(&flushDwParams, sizeof(flushDwParams));
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiFlushDwCmd(
        cmdBuffer,
        &flushDwParams));

    // Save original decode fingerprint for later comparison
    MOS_ZeroMemory(&copyMemMemParams, sizeof(copyMemMemParams));
    copyMemMemParams.presSrc = resource;
    copyMemMemParams.dwSrcOffset = baseOffset + (MHW_CACHELINE_SIZE * 2);
    copyMemMemParams.presDst = resource;
    copyMemMemParams.dwDstOffset =
        baseOffset + (MHW_CACHELINE_SIZE * 2) + (sizeof(uint32_t) * 2);
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiCopyMemMemCmd(
        cmdBuffer,
        &copyMemMemParams));
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiFlushDwCmd(
        cmdBuffer,
        &flushDwParams));

    // If fingerprints matching, convert to 0, else value remains same
    MOS_ZeroMemory(&registerMemParams, sizeof(registerMemParams));
    registerMemParams.presStoreBuffer = resource;
    registerMemParams.dwOffset = baseOffset + MHW_CACHELINE_SIZE;
    registerMemParams.dwRegister = mmioRegisters->generalPurposeRegister0LoOffset;
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiLoadRegisterMemCmd(
        cmdBuffer,
        &registerMemParams));
    MOS_ZeroMemory(&registerImmParams, sizeof(registerImmParams));
    registerImmParams.dwData = 0;
    registerImmParams.dwRegister = mmioRegisters->generalPurposeRegister4LoOffset;
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiLoadRegisterImmCmd(
        cmdBuffer,
        &registerImmParams));
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiFlushDwCmd(
        cmdBuffer,
        &flushDwParams));

    MOS_ZeroMemory(&atomicParams, sizeof(atomicParams));
    atomicParams.pOsResource = resource;
    atomicParams.dwResourceOffset = baseOffset + (MHW_CACHELINE_SIZE * 2);
    atomicParams.dwDataSize = sizeof(uint32_t);
    atomicParams.Operation = MHW_MI_ATOMIC_CMP;
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiAtomicCmd(
        cmdBuffer,
        &atomicParams));
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiFlushDwCmd(
        cmdBuffer,
        &flushDwParams));

    // If matches original decode fingerprint, covert to 0 (Lo)and 0 (Hi), else
    // stays the same 0 (Lo) and 1 (Hi), extract Hi DW.
    MOS_ZeroMemory(&storeDataParams, sizeof(storeDataParams));
    storeDataParams.pOsResource = resource;
    storeDataParams.dwResourceOffset =
        baseOffset + (MHW_CACHELINE_SIZE * 2) + sizeof(uint32_t);
    storeDataParams.dwValue = 1;
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiStoreDataImmCmd(
        cmdBuffer,
        &storeDataParams));
    MOS_ZeroMemory(&registerMemParams, sizeof(registerMemParams));
    registerMemParams.presStoreBuffer = resource;
    registerMemParams.dwOffset =
        baseOffset + (MHW_CACHELINE_SIZE * 2) + (sizeof(uint32_t) * 2);
    registerMemParams.dwRegister = mmioRegisters->generalPurposeRegister0LoOffset;
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiLoadRegisterMemCmd(
        cmdBuffer,
        &registerMemParams));
    MOS_ZeroMemory(&registerImmParams, sizeof(registerImmParams));
    registerImmParams.dwData = 1;
    registerImmParams.dwRegister = mmioRegisters->generalPurposeRegister0HiOffset;
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiLoadRegisterImmCmd(
        cmdBuffer,
        &registerImmParams));
    registerImmParams.dwData = 0;
    registerImmParams.dwRegister = mmioRegisters->generalPurposeRegister4LoOffset;
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiLoadRegisterImmCmd(
        cmdBuffer,
        &registerImmParams));
    registerImmParams.dwData = 0;
    registerImmParams.dwRegister = mmioRegisters->generalPurposeRegister4HiOffset;
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiLoadRegisterImmCmd(
        cmdBuffer,
        &registerImmParams));
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiFlushDwCmd(
        cmdBuffer,
        &flushDwParams));

    MOS_ZeroMemory(&atomicParams, sizeof(atomicParams));
    atomicParams.pOsResource = resource;
    atomicParams.dwResourceOffset = baseOffset + (MHW_CACHELINE_SIZE * 2);
    atomicParams.dwDataSize = sizeof(uint64_t);
    atomicParams.Operation = MHW_MI_ATOMIC_CMP;
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiAtomicCmd(
        cmdBuffer,
        &atomicParams));
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiFlushDwCmd(
        cmdBuffer,
        &flushDwParams));

    // Move MI_ATOMIC Hi DW to 1st cacheline for conditional BB end data DW
    MOS_ZeroMemory(&copyMemMemParams, sizeof(copyMemMemParams));
    copyMemMemParams.presSrc = resource;
    copyMemMemParams.dwSrcOffset = baseOffset + (MHW_CACHELINE_SIZE * 2) + sizeof(uint32_t);
    copyMemMemParams.presDst = resource;
    copyMemMemParams.dwDstOffset = baseOffset + (sizeof(uint32_t) * 2);
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiCopyMemMemCmd(
        cmdBuffer,
        &copyMemMemParams));
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiFlushDwCmd(
        cmdBuffer,
        &flushDwParams));

    // conditional batch buffer end based on whether or not the header is valid
    MOS_ZeroMemory(&conditionalBatchBufferEndParams, sizeof(conditionalBatchBufferEndParams));
    conditionalBatchBufferEndParams.presSemaphoreBuffer =
        resource;
    conditionalBatchBufferEndParams.dwOffset = baseOffset + (sizeof(uint32_t) * 2);
    conditionalBatchBufferEndParams.dwValue = 0;
    conditionalBatchBufferEndParams.bDisableCompareMask = true;
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiConditionalBatchBufferEndCmd(
        cmdBuffer,
        &conditionalBatchBufferEndParams));

    return eStatus;
}

MOS_STATUS MhwCpBase::RegisterMiInterface(
    MhwMiInterface* miInterface)
{
    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(miInterface);

    // MhwCpBase remember this instance first
    this->m_mhwMiInterface = miInterface;

    // Then propagate to CpHwUnit
    // get pointer to the Render Object first
    // Note: don't use getHwUnit(), this must go specificly to the VEBox and VDBox objects
    //       because at the init time, osInterface might not be pointing to VEBox and VDBox.
    auto renderUnit = m_hwUnit[PAVP_HW_UNIT_RCS];
    MHW_CP_CHK_NULL(renderUnit);

    auto veboxUnit = m_hwUnit[PAVP_HW_UNIT_VECS];
    MHW_CP_CHK_NULL(veboxUnit);

    auto vdboxUnit = m_hwUnit[PAVP_HW_UNIT_VCS];
    MHW_CP_CHK_NULL(vdboxUnit);

    // register
    MHW_CP_CHK_STATUS(renderUnit->RegisterMiInterface(miInterface));

    MHW_CP_CHK_STATUS(veboxUnit->RegisterMiInterface(miInterface));

    MHW_CP_CHK_STATUS(vdboxUnit->RegisterMiInterface(miInterface));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpBase::RegisterMiInterfaceNext(
    std::shared_ptr<mhw::mi::Itf> m_miItf)
{
    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(m_miItf);

    // MhwCpBase remember this instance first
    this->m_miItf = m_miItf;

    // Then propagate to CpHwUnit
    // get pointer to the Render Object first
    // Note: don't use getHwUnit(), this must go specificly to the VEBox and VDBox objects
    //       because at the init time, osInterface might not be pointing to VEBox and VDBox.
    auto renderUnit = m_hwUnit[PAVP_HW_UNIT_RCS];
    MHW_CP_CHK_NULL(renderUnit);

    auto veboxUnit = m_hwUnit[PAVP_HW_UNIT_VECS];
    MHW_CP_CHK_NULL(veboxUnit);

    auto vdboxUnit = m_hwUnit[PAVP_HW_UNIT_VCS];
    MHW_CP_CHK_NULL(vdboxUnit);

    // register
    MHW_CP_CHK_STATUS(renderUnit->RegisterMiInterfaceNext(m_miItf));

    MHW_CP_CHK_STATUS(veboxUnit->RegisterMiInterfaceNext(m_miItf));

    MHW_CP_CHK_STATUS(vdboxUnit->RegisterMiInterfaceNext(m_miItf));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}
    CpHwUnit* MhwCpBase::getHwUnit(PMOS_INTERFACE osInterface)
{
    CpHwUnit* hwUnit = nullptr;

    MHW_CP_FUNCTION_ENTER;

    if (MOS_RCS_ENGINE_USED(osInterface->pfnGetGpuContext(osInterface)))
    {
        // Render
        hwUnit = m_hwUnit[PAVP_HW_UNIT_RCS];
    }
    else if (MOS_VCS_ENGINE_USED(osInterface->pfnGetGpuContext(osInterface)))
    {
        // VDBox
        hwUnit = m_hwUnit[PAVP_HW_UNIT_VCS];
    }
    else if (MOS_VECS_ENGINE_USED(osInterface->pfnGetGpuContext(osInterface)))
    {
        // VEBox
        hwUnit = m_hwUnit[PAVP_HW_UNIT_VECS];
    }
    else if (MOS_TEECS_ENGINE_USED(osInterface->pfnGetGpuContext(osInterface)))
    {
        // GSC
        hwUnit = m_hwUnit[PAVP_HW_UNIT_GCS];
    }
    else
    {
        MOS_ASSERTMESSAGE(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW, "no supported hw unit defined.use Gpu Context %d", osInterface->pfnGetGpuContext(osInterface));
        MT_ERR3(MT_CP_MHW_UNIT_NOT_SUPPORT, MT_COMPONENT, MOS_COMPONENT_CP, MT_SUB_COMPONENT, MOS_CP_SUBCOMP_MHW, 
                                                      MT_MOS_GPUCXT, osInterface->pfnGetGpuContext(osInterface));
    }

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return hwUnit;
}

MOS_STATUS MhwCpBase::SetCpCopy(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer,
    PMHW_CP_COPY_PARAMS params)
{
    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(cmdBuffer);
    MHW_CP_CHK_NULL(params);

    MHW_PAVP_CRYPTO_COPY_ADDR_PARAMS  cryptoCopyAddrParams;
    MOS_ZeroMemory(&cryptoCopyAddrParams, sizeof(cryptoCopyAddrParams));
    cryptoCopyAddrParams.bIsDestRenderTarget    = true;
    cryptoCopyAddrParams.dwSize                 = params->size;
    cryptoCopyAddrParams.presSrc                = params->presSrc;
    cryptoCopyAddrParams.presDst                = params->presDst;
    MHW_CP_CHK_STATUS(this->AddMfxCryptoCopyBaseAddr(cmdBuffer, &cryptoCopyAddrParams));

    MHW_PAVP_CRYPTO_COPY_PARAMS cryptoCopyParams;
    MOS_ZeroMemory(&cryptoCopyParams, sizeof(cryptoCopyParams));
    if (params->isEncodeInUse)
    {
        cryptoCopyParams.ui8SelectionForIndData = osInterface->osCpInterface->IsCpEnabled() ? CRYPTO_COPY_TYPE_SERPENT_TO_AES_ENCRYPT : CRYPTO_COPY_TYPE_BYPASS;
    }
    else
    {
        cryptoCopyParams.ui8SelectionForIndData = osInterface->osCpInterface->IsHMEnabled() ? CRYPTO_COPY_TYPE_CLEAR_TO_SERPENT_ENCRYPT : CRYPTO_COPY_TYPE_BYPASS;
        cryptoCopyParams.dwCopySize = params->size;
    }

    cryptoCopyParams.CmdMode       = (params->isEncodeInUse) ? CRYPTO_COPY_CMD_LIST_MODE:CRYPTO_COPY_CMD_LEGACY_MODE;
    cryptoCopyParams.LengthOfTable = params->lengthOfTable;
    MHW_CP_CHK_STATUS(this->AddMfxCryptoCopy(cmdBuffer, &cryptoCopyParams));

    if (params->isEncodeInUse)
    {
        // This wait cmd is needed to make sure crypto copy command is done as suggested by HW folk in encode cases
        MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMfxWaitCmd(cmdBuffer, nullptr, m_osInterface.osCpInterface->IsCpEnabled() ? true : false));
    }

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpBase::AddCpCopy(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer,
    PMHW_ADD_CP_COPY_PARAMS params)
{
    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(cmdBuffer);
    MHW_CP_CHK_NULL(params);

    MHW_PAVP_CRYPTO_COPY_ADDR_PARAMS cryptoCopyAddrParams;
    MOS_ZeroMemory(&cryptoCopyAddrParams, sizeof(cryptoCopyAddrParams));
    cryptoCopyAddrParams.bIsDestRenderTarget = false;
    cryptoCopyAddrParams.dwSize              = params->size;
    cryptoCopyAddrParams.presSrc             = params->presSrc;
    cryptoCopyAddrParams.presDst             = params->presDst;
    cryptoCopyAddrParams.offset              = params->offset;
    MHW_CP_CHK_STATUS(this->AddMfxCryptoCopyBaseAddr(cmdBuffer, &cryptoCopyAddrParams));

    MHW_PAVP_CRYPTO_COPY_PARAMS cryptoCopyParams;
    MOS_ZeroMemory(&cryptoCopyParams, sizeof(cryptoCopyParams));
    cryptoCopyParams.AesMode                 = CRYPTO_COPY_AES_MODE_CTR;
    cryptoCopyParams.ui8SelectionForIndData  = params->bypass ? CRYPTO_COPY_TYPE_BYPASS : CRYPTO_COPY_TYPE_CLEAR_TO_SERPENT_ENCRYPT;
    cryptoCopyParams.dwCopySize              = params->size;
    cryptoCopyParams.dwSrcBaseOffset         = 0;
    cryptoCopyParams.dwDestBaseOffset        = 0;
    MHW_CP_CHK_STATUS(this->AddMfxCryptoCopy(cmdBuffer, &cryptoCopyParams));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpBase::AddMfxCryptoKeyExchange(
    PMOS_COMMAND_BUFFER                          cmdBuffer,
    PMHW_PAVP_MFX_CRYPTO_KEY_EXCHANGE_CMD_PARAMS params)
{
    auto cmd = g_cInit_MFX_CRYPTO_KEY_EXCHANGE_CMD;

    // Populate the fields as requested by the caller.
    switch (params->KeyExchangeKeyUse)
    {
    case CRYPTO_TERMINATE_KEY:
        cmd.DW1.KeyUseIndicator = CRYPTO_TERMINATE_KEY;
        MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, &cmd, sizeof(cmd)));
        break;
    case CRYPTO_DECRYPT_AND_USE_NEW_KEY:
        cmd.DW1.KeyUseIndicator = CRYPTO_DECRYPT_AND_USE_NEW_KEY;
        if (params->KeyExchangeKeySelect & MHW_CRYPTO_DECRYPTION_KEY)
        {
            // DWORDLength is the number of DWORDs taken by the keys
            cmd.DW0.DWordLength            = KEY_DWORDS_PER_AES_BLOCK;
            cmd.DW1.DecryptionOrEncryption = CRYPTO_DECRYPT;
            MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, &cmd, sizeof(cmd)));
            MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, params->EncryptedDecryptionKey, sizeof(params->EncryptedDecryptionKey)));
        }
        if (params->KeyExchangeKeySelect & MHW_CRYPTO_ENCRYPTION_KEY)
        {
            // DWORDLength is the number of DWORDs taken by the keys
            cmd.DW0.DWordLength            = KEY_DWORDS_PER_AES_BLOCK;
            cmd.DW1.DecryptionOrEncryption = CRYPTO_ENCRYPT;
            MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, &cmd, sizeof(cmd)));
            // Populate the key blob.
            MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, params->EncryptedEncryptionKey, sizeof(params->EncryptedEncryptionKey)));
        }
        break;
    case CRYPTO_USE_NEW_FRESHNESS_VALUE:
        cmd.DW1.KeyUseIndicator        = CRYPTO_USE_NEW_FRESHNESS_VALUE;
        if (params->KeyExchangeKeySelect == MHW_CRYPTO_DECRYPTION_KEY)
        {
            cmd.DW1.DecryptionOrEncryption = CRYPTO_DECRYPT;
        }
        else
        {
            cmd.DW1.DecryptionOrEncryption = CRYPTO_ENCRYPT;
        }
        MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, &cmd, sizeof(cmd)));
        break;
    default:
        return MOS_STATUS_INVALID_PARAMETER;
        break;
    }

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpBase::RefreshCounter(
    PMOS_INTERFACE osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    MOS_UNUSED(osInterface);
    MOS_UNUSED(cmdBuffer);
    return MOS_STATUS_UNIMPLEMENTED;
}

MOS_STATUS MhwCpBase::ReadEncodeCounterFromHW(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer,
    PMOS_RESOURCE       resource,
    uint16_t            currentIndex)
{
    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(cmdBuffer);
    MHW_CP_CHK_NULL(resource);

    MHW_PAVP_MFX_CRYPTO_INLINE_STATUS_READ_CMD_PARAMS cryptoInlineStatusReadParams;

    uint32_t offset = currentIndex * sizeof(CP_ENCODE_HW_COUNTER);

    //Send inline command to get the Counter value and cache it in the HW Counter query buffer
    MOS_ZeroMemory(&cryptoInlineStatusReadParams, sizeof(cryptoInlineStatusReadParams));
    cryptoInlineStatusReadParams.StatusReadType   = MHW_CRYPTO_INLINE_GET_HW_COUNTER_READ;
    cryptoInlineStatusReadParams.presReadData     = resource;
    cryptoInlineStatusReadParams.dwSize           = sizeof(CP_ENCODE_HW_COUNTER);
    cryptoInlineStatusReadParams.dwReadDataOffset = offset;

    // dummy write inside CRYPTO_INLINE_STATUS_READ to satisfy requirement of pipelined media store DW
    cryptoInlineStatusReadParams.presWriteData = resource;
    cryptoInlineStatusReadParams.dwWriteDataOffset = sizeof(CP_ENCODE_HW_COUNTER) * CODECHAL_ENCODE_STATUS_NUM;

    MHW_CP_CHK_STATUS(this->AddMfxCryptoInlineStatusRead(
        osInterface,
        cmdBuffer,
        &cryptoInlineStatusReadParams));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpBase::AddHcpAesState(
    bool                 bDecodeInUse,
    PMOS_COMMAND_BUFFER  cmdBuffer,
    PMHW_BATCH_BUFFER    batchBuffer,
    PMHW_PAVP_AES_PARAMS params)
{
    auto            cmd       = g_cInit_HCP_AES_STATE_CMD;
    uint32_t        aesCtr[4] = {0, 0, 0, 0};
    uint32_t        counterIncrement = 0;
    MOS_LOCK_PARAMS lockFlags;

    MHW_CP_FUNCTION_ENTER;
    MHW_CP_CHK_NULL(params);

    auto miInterface      = this->m_mhwMiInterface;
    auto encryptionParams = this->GetEncryptionParams();
    MHW_CP_CHK_NULL(encryptionParams);

    MHW_CP_CHK_STATUS_MESSAGE(MOS_SecureMemcpy(
        aesCtr,
        CODEC_DWORDS_PER_AES_BLOCK * sizeof(uint32_t),
        encryptionParams->dwPavpAesCounter,
        CODEC_DWORDS_PER_AES_BLOCK * sizeof(uint32_t)),
        "Failed to copy memory!\n");

    if (encryptionParams->PavpEncryptionType == CP_SECURITY_DECE_AES128_CTR)
    {
        if (encryptionParams->pSubSampleMappingBlock != nullptr)
        {
            PMHW_SUB_SAMPLE_MAPPING_BLOCK pSubSampleBlocks = reinterpret_cast<PMHW_SUB_SAMPLE_MAPPING_BLOCK>(encryptionParams->pSubSampleMappingBlock);
            PMHW_SUB_SAMPLE_MAPPING_BLOCK pCurrentBlock = &(pSubSampleBlocks[params->dwSliceIndex]);

            cmd.DW7.InitPktBytesLengthInBytes = pCurrentBlock->ClearSize;
            cmd.DW8.EncryptedBytesLengthInBytes = pCurrentBlock->EncryptedSize;
            cmd.DW9.ClearBytesLengthInBytes = pCurrentBlock->ClearSize;
            cmd.DW8.DetectAndHandlePartialBlockAtTheEndOfSlice = 1;

            uint32_t encryptedSize = 0;
            for(uint32_t i = 0; i < params->dwSliceIndex; i++)
            {
                encryptedSize += pSubSampleBlocks[i].EncryptedSize;
            }
            counterIncrement = encryptedSize >> 4;
        }
        else
        {
            //Fair Play
            uint32_t nalUnitLength = (params->bShortFormatInUse) ? MOS_NAL_UNIT_STARTCODE_LENGTH : MOS_NAL_UNIT_LENGTH;
            uint32_t zeroPadding   = params->dwDataStartOffset - params->dwTotalBytesConsumed;
            uint32_t numClearBytes = MHW_PAVP_DECE_NUM_CLEAR_BYTES_IN_SLICE - zeroPadding;

            if (params->dwDataLength == MHW_PAVP_DECE_NUM_CLEAR_BYTES_IN_SLICE)
            {
                //Reset Counter to ZERO
                aesCtr[0] = aesCtr[1] = aesCtr[2] = aesCtr[3] = 0;

                //No increment as well
                counterIncrement                    = 0;
                cmd.DW7.InitPktBytesLengthInBytes   = MHW_PAVP_DECE_NUM_CLEAR_BYTES_IN_SLICE;
                cmd.DW8.EncryptedBytesLengthInBytes = 0;
            }
            else
            {
                counterIncrement                    = (params->dwTotalBytesConsumed >> 4) - (params->dwSliceIndex << 3);
                cmd.DW7.InitPktBytesLengthInBytes   = numClearBytes - nalUnitLength;
                cmd.DW8.EncryptedBytesLengthInBytes = params->dwDataLength - numClearBytes;
            }

            // indicate initial packet is encrypted or not
            cmd.DW7.InitialFairPlayOrDeceStartPacketType = 0;  //default

            cmd.DW9.ClearBytesLengthInBytes = numClearBytes;
        }
    }
    else if (encryptionParams->PavpEncryptionType == CP_SECURITY_AES128_CBC)
    {
        // CBC
        MHW_CP_CHK_NULL(params->presDataBuffer);

        uint32_t *lastEncryptedOword = nullptr;
        MOS_ZeroMemory(&lockFlags, sizeof(MOS_LOCK_PARAMS));
        lockFlags.ReadOnly         = 1;
        uint32_t *firstEncryptedOword = static_cast<uint32_t*>(m_osInterface.pfnLockResource(
            &m_osInterface,
            params->presDataBuffer,
            &lockFlags));
        MHW_CP_CHK_NULL(firstEncryptedOword);

        lastEncryptedOword =
            firstEncryptedOword +
            ((params->dwDataStartOffset >> 4) * 4);

        if (lastEncryptedOword != firstEncryptedOword)
        {
            lastEncryptedOword -= KEY_DWORDS_PER_AES_BLOCK;  // The block size of AES-128 is 128 bits, so the subtrahend is 4.
            aesCtr[0] = lastEncryptedOword[0];
            aesCtr[1] = lastEncryptedOword[1];
            aesCtr[2] = lastEncryptedOword[2];
            aesCtr[3] = lastEncryptedOword[3];
        }

        m_osInterface.pfnUnlockResource(&m_osInterface, params->presDataBuffer);

        counterIncrement = 0;
    }
    else if (encryptionParams->PavpEncryptionType == CP_SECURITY_SKIP_AES128_CBC)
    {
        uint32_t dwCbcSkipInitial = encryptionParams->dwPavpAesCbcSkipInitial;
        uint32_t dwCbcSkipEncrypted = encryptionParams->dwPavpAesCbcSkipEncrypted;
        uint32_t dwCbcSkipClear = encryptionParams->dwPavpAesCbcSkipClear;

        bool bCbcSkipInitialType = 0; // is initial encrypted or clear
        bool bLastPartialBlock = true;

        MHW_CP_CHK_NULL(params->presDataBuffer);

        if (dwCbcSkipEncrypted != 0)    // if dwCbcSkipEncrypted == 0, entire slice is clear
        {
            MOS_ZeroMemory(&lockFlags, sizeof(MOS_LOCK_PARAMS));
            lockFlags.ReadOnly = 1;
            uint32_t* firstEncryptedOword = static_cast<uint32_t*>(m_osInterface.pfnLockResource(
                                                                                                 &m_osInterface,
                                                                                                 params->presDataBuffer,
                                                                                                 &lockFlags));
            MHW_CP_CHK_NULL(firstEncryptedOword);

            CalcAesCbcSkipState(params, &bCbcSkipInitialType, &dwCbcSkipInitial, aesCtr, &bLastPartialBlock, (uint8_t*)firstEncryptedOword);

            m_osInterface.pfnUnlockResource(&m_osInterface, params->presDataBuffer);
        }

        cmd.DW7.InitPktBytesLengthInBytes = dwCbcSkipInitial; // param a
        cmd.DW7.ProgrammedInitialCounterValue = 0;
        cmd.DW7.InitialFairPlayOrDeceStartPacketType = bCbcSkipInitialType;    // is initial encrypted or clear

        // Number of encrypted bytes.
        cmd.DW8.EncryptedBytesLengthInBytes = encryptionParams->dwPavpAesCbcSkipEncrypted; // param b
        cmd.DW8.DetectAndHandlePartialBlockAtTheEndOfSlice = bLastPartialBlock; // handle last AES block not a multiple of 16 bytes

        cmd.DW9.ClearBytesLengthInBytes = encryptionParams->dwPavpAesCbcSkipClear; // param c

        counterIncrement = 0;
    }
    else
    {
        // Standard AES CTR mode
        counterIncrement = params->dwDataStartOffset >> 4;
    }

    // Decode increments the counter before adding the AES state to the command buffer
    if (bDecodeInUse)
    {
        IncrementCounter(
            aesCtr,
            counterIncrement);
    }

    cmd.DW4.InitializationValue3 = aesCtr[3];
    cmd.DW3.InitializationValue2 = aesCtr[2];
    cmd.DW2.InitializationValue1 = aesCtr[1];
    cmd.DW1.InitializationValue0 = aesCtr[0];

    // Encode increments the counter after adding the AES state to the command buffer
    // and the counter is incremented for last pass only
    if (!bDecodeInUse && params->bLastPass)
    {
        IncrementCounter(
            encryptionParams->dwPavpAesCounter,
            encryptionParams->uiCounterIncrement);
    }

    cmd.DW5.MonotonicEncryptionCounterCheckEnable = (MHW_PAVP_ENCRYPTION_TYPE(encryptionParams->PavpEncryptionType) == MHW_PAVP_AES_TYPE_CTR) ? 1 : 0;
    cmd.DW7.ProgrammedInitialCounterValue              = 0;  //default
    cmd.DW8.DetectAndHandlePartialBlockAtTheEndOfSlice = 1;

    if (cmdBuffer == nullptr && batchBuffer == nullptr)
    {
        MHW_ASSERTMESSAGE("%s: There was no valid buffer to add the HW command to.\n", __FUNCTION__);
        MT_ERR1(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__);
    }

    MHW_CP_CHK_STATUS(Mhw_AddCommandCmdOrBB(&m_osInterface, cmdBuffer, batchBuffer, &cmd, sizeof(cmd)));

    // This wait cmd is needed to make sure pavp command is done before decode starts
    if (miInterface != nullptr)
    {
        MHW_CP_CHK_STATUS(miInterface->AddMfxWaitCmd(cmdBuffer, batchBuffer, false));
    }
    else
    {
        auto &mfxWaitParams = m_miItf->MHW_GETPAR_F(MFX_WAIT)();
        mfxWaitParams                     = {};
        mfxWaitParams.iStallVdboxPipeline = false;
        MHW_CP_CHK_STATUS(m_miItf->MHW_ADDCMD_F(MFX_WAIT)(cmdBuffer, batchBuffer));
    }
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpBase::AddVvcpAesIndState(
    bool                 bDecodeInUse,
    PMOS_COMMAND_BUFFER  cmdBuffer,
    PMHW_BATCH_BUFFER batchBuffer,
    PMHW_PAVP_AES_PARAMS params)
{
    MOS_UNUSED(bDecodeInUse);
    MOS_UNUSED(cmdBuffer);
    MOS_UNUSED(batchBuffer);
    MOS_UNUSED(params);
    return MOS_STATUS_UNIMPLEMENTED;
}

MOS_STATUS MhwCpBase::AddHucAesState(
    PMOS_COMMAND_BUFFER  cmdBuffer,
    PMHW_PAVP_AES_PARAMS params)
{
    auto cmd = g_cInit_HUC_AES_STATE_CMD;

    MHW_CP_CHK_NULL(cmdBuffer);
    MHW_CP_CHK_NULL(params);

    auto aesCtr = this->GetCpAesCounter();
    MHW_CP_CHK_NULL(aesCtr);

    cmd.DW1.InitializationValue0 = aesCtr[0];
    cmd.DW2.InitializationValue1 = aesCtr[1];
    cmd.DW3.InitializationValue2 = aesCtr[2];
    cmd.DW4.InitializationValue3 = aesCtr[3];

    cmd.DW5.MonotonicEncryptionCounterCheckEnable = (MHW_PAVP_ENCRYPTION_TYPE(this->GetCpEncryptionType()) == MHW_PAVP_AES_TYPE_CTR) ? 1 : 0;
    cmd.DW8.DetectAndHandlePartialBlockAtTheEndOfSlice = 1;

    MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, &cmd, sizeof(cmd)));

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpBase::AddHucAesIndState(
    PMOS_COMMAND_BUFFER  cmdBuffer,
    PMHW_PAVP_AES_PARAMS params)
{
    auto                cmd = g_cInit_HUC_AES_IND_STATE_CMD;
    MHW_RESOURCE_PARAMS resourceParams;

    MHW_CP_CHK_NULL(cmdBuffer);
    MHW_CP_CHK_NULL(params);
    MHW_CP_CHK_NULL(params->presDataBuffer);

    MOS_ZeroMemory(&resourceParams, sizeof(resourceParams));
    resourceParams.dwLsbNum        = MHW_VDBOX_HUC_GENERAL_STATE_SHIFT;
    resourceParams.presResource    = params->presDataBuffer;
    resourceParams.dwOffset        = params->dwOffset;
    resourceParams.pdwCmd          = (uint32_t*)&(cmd.QW1.DW1.Value);
    resourceParams.dwLocationInCmd = 1;
    resourceParams.bIsWritable     = false;

    cmd.DW4.NumberOfEntriesMinus1 = params->dwSliceIndex - 1;

    do
    {
        auto hwUnit = this->getHwUnit(&m_osInterface);
        MHW_CP_CHK_NULL(hwUnit);
        MHW_CP_CHK_STATUS(hwUnit->pfnAddResourceToCmd(
            &m_osInterface,
            cmdBuffer,
            &resourceParams));
    } while (false);

    MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, &cmd, sizeof(cmd)));

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpBase::SetHucIndirectCryptoState(
    PMHW_PAVP_AES_IND_PARAMS params)
{
    PHUC_INDIRECT_CRYPTO_STATE_G8LP cmd;
    MOS_LOCK_PARAMS                 lockFlags;
    uint32_t                        i;

    MHW_CP_CHK_NULL(params);
    MHW_CP_CHK_NULL(params->presCryptoBuffer);
    MHW_CP_CHK_NULL(params->pSegmentInfo);

    MHW_ASSERT(params->dwNumSegments != 0);

    lockFlags.Value     = 0;
    lockFlags.WriteOnly = 1;
    cmd =
        static_cast<PHUC_INDIRECT_CRYPTO_STATE_G8LP>(m_osInterface.pfnLockResource(&m_osInterface, params->presCryptoBuffer, &lockFlags));

    MHW_CP_CHK_NULL(cmd);
    cmd += params->dwOffset;

    for (i = 0; i < params->dwNumSegments; i++)
    {
        MOS_ZeroMemory(&cmd[i], sizeof(HUC_INDIRECT_CRYPTO_STATE_G8LP));
        cmd[i].DW0.AESCtrOrIVDW0 = params->pSegmentInfo[i].dwAesCbcIvOrCtr[0];
        cmd[i].DW1.AESCtrOrIVDW1 = params->pSegmentInfo[i].dwAesCbcIvOrCtr[1];
        cmd[i].DW2.AESCtrOrIVDW2 = params->pSegmentInfo[i].dwAesCbcIvOrCtr[2];
        cmd[i].DW3.AESCtrOrIVDW3 = params->pSegmentInfo[i].dwAesCbcIvOrCtr[3];

        cmd[i].DW6.InitByteLength     = params->pSegmentInfo[i].dwInitByteLength;
        cmd[i].DW6.PartialBlockLength = params->pSegmentInfo[i].dwPartialAesBlockSizeInBytes;
        cmd[i].DW7.AESEnable =
            (params->pSegmentInfo[i].dwInitByteLength == params->pSegmentInfo[i].dwSegmentLength) ? false : true;

        if (this->GetCpEncryptionType() == CP_SECURITY_AES128_CBC)
        {
            cmd[i].DW7.AESMode = 0;  // According to fulsim, CBC = 0, CTR = 1
            if (((params->pSegmentInfo[i].dwSegmentLength - params->pSegmentInfo[i].dwInitByteLength) & 0xF) == 0)
            {
                cmd[i].DW7.AESCbcFlavor = 0;  // CBC
            }
            else
            {
                cmd[i].DW7.AESCbcFlavor = 1;  // CBC+CTS 1
            }
            cmd[i].DW7.AESCtrFlavor = 1;  // fulsim set as 1, BSpec is not clear about this
        }
        else if (this->GetHostEncryptMode() == CENC_TYPE_CTR_LENGTH ||
            this->GetCpEncryptionType() == CP_SECURITY_AES128_CTR ||
            this->GetHostEncryptMode() == CP_TYPE_BASIC)
        {
            cmd[i].DW7.AESMode      = 1;  // According to fulsim, CBC = 0, CTR = 1
            cmd[i].DW7.AESCtrFlavor = 2;  // fulsim set as 1, BSpec is not clear about this
        }

        cmd[i].DW8.EndAddress   = params->pSegmentInfo[i].dwSegmentStartOffset + params->pSegmentInfo[i].dwSegmentLength - 1;
        cmd[i].DW9.StartAddress = params->pSegmentInfo[i].dwSegmentStartOffset;

        MHW_VERBOSEMESSAGE("Seg %d, AES %d, InitByteLength %d, PartialBlockLength %d", i, cmd[i].DW7.AESEnable, cmd[i].DW6.InitByteLength, cmd[i].DW6.PartialBlockLength);
        MHW_VERBOSEMESSAGE("Count %08x %08x %08x %08x", cmd[i].DW0.AESCtrOrIVDW0, cmd[i].DW1.AESCtrOrIVDW1, cmd[i].DW2.AESCtrOrIVDW2, cmd[i].DW3.AESCtrOrIVDW3);
        MHW_VERBOSEMESSAGE("StartAddress %08x EndAddress %08x", cmd[i].DW9.Value, cmd[i].DW8.Value);
    }
    MOS_TraceDumpExt("HuCIndCryptoState", params->dwNumSegments, cmd, params->dwNumSegments * sizeof(*cmd));
    m_osInterface.pfnUnlockResource(&m_osInterface, params->presCryptoBuffer);

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpBase::SetVvcpIndirectCryptoState(
    PMHW_PAVP_AES_IND_PARAMS params)
{
    MOS_UNUSED(params);
    return MOS_STATUS_UNIMPLEMENTED;
}

MOS_STATUS MhwCpBase::SetProtectionSettingsForMiFlushDw(
    PMOS_INTERFACE osInterface,
    void           *cmd)
{
    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(cmd);

    static_cast<MI_FLUSH_DW_PAVP*>(cmd)->ProtectedMemoryEnable = 
                osInterface->osCpInterface->IsHMEnabled();

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpBase::SetProtectionSettingsForMfxWait(
    PMOS_INTERFACE osInterface,
    void           *cmd)
{
    auto mfxWait = reinterpret_cast<PMFX_WAIT_PAVP>(cmd);  // DW0

    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(cmd);
    MHW_CP_CHK_NULL(osInterface);

    //set protecting bit base on pavp status
    mfxWait->PavpSyncControlFlag = osInterface->osCpInterface->IsCpEnabled();

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpBase::SetProtectionSettingsForMfxPipeModeSelect(
    uint32_t *data)
{
    auto aesControl = reinterpret_cast<PMFX_PIPE_MODE_SELECT_PAVP>(++data);  // DW1

    MHW_CP_FUNCTION_ENTER;
    MHW_CP_CHK_NULL(data);

    if (m_encryptionParams.HostEncryptMode != CP_TYPE_NONE)
    {
        aesControl->AesEnable                                     = m_osInterface.osCpInterface->IsCpEnabled() ? (GetHostEncryptMode() == CP_TYPE_BASIC) : false;
        aesControl->AesOperationSelectionForTheIndirectSourceData = MHW_PAVP_ENCRYPTION_TYPE(GetCpEncryptionType());
        aesControl->DECEEnable                                    = (GetCpEncryptionType() == CP_SECURITY_DECE_AES128_CTR);
        aesControl->AesCtrFlavor                                  = MHW_PAVP_COUNTER_TYPE_C;
    }

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpBase::SetProtectionSettingsForHcpPipeModeSelect(
    uint32_t *pData,
    bool     twoPassScalableEncode)
{
    auto aesControl = reinterpret_cast<PMFX_PIPE_MODE_SELECT_PAVP>(++pData);  // DW1

    MHW_CP_FUNCTION_ENTER;
    MHW_CP_CHK_NULL(pData);

    if (m_encryptionParams.HostEncryptMode != CP_TYPE_NONE)
    {
        // For two pass Secure scalable encode workload, AesEnable should be set to false.
        // The actual PAVP bitstream encryption happens in Cryptocopy not PIPE_MODE_SELECT.
        // During the first pass, setting AesEnable in PipeModeSelect to False will force the Tiles
        // to be written to memory in clear(for lite mode) or serpent encrypted(for heavy mode).
        // During second pass, mode 0x4 cryptocopy will automatically serpent decrypt the
        // input(for heavy mode) or no - decrypt read(for lite mode, since input is in clear).
        aesControl->AesEnable                                     = twoPassScalableEncode ? false : (m_osInterface.osCpInterface->IsCpEnabled() ? (GetHostEncryptMode() == CP_TYPE_BASIC) : false);
        aesControl->AesOperationSelectionForTheIndirectSourceData = MHW_PAVP_ENCRYPTION_TYPE(GetCpEncryptionType());
        aesControl->AesCtrFlavor                                  = MHW_PAVP_COUNTER_TYPE_C;

    }

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpBase::SetProtectionSettingsForHucPipeModeSelect(
    uint32_t *data)
{
    PMFX_PIPE_MODE_SELECT_PAVP aesControl = reinterpret_cast<PMFX_PIPE_MODE_SELECT_PAVP>(++data);  // DW1
    CP_MODE               HostEncryptMode = GetHostEncryptMode();
    MHW_CP_FUNCTION_ENTER;
    MHW_CP_CHK_NULL(data);

    if (HostEncryptMode != CP_TYPE_NONE)
    {
        aesControl->AesCtrFlavor              = MHW_PAVP_COUNTER_TYPE_C;

        switch (HostEncryptMode)
        {
        case CP_TYPE_BASIC:
            aesControl->AesEnable                                     = m_osInterface.osCpInterface->IsCpEnabled();
            aesControl->AesOperationSelectionForTheIndirectSourceData = MHW_PAVP_ENCRYPTION_TYPE(GetCpEncryptionType());
            break;
        case CENC_TYPE_CBC:
        case CENC_TYPE_CTR_LENGTH:
        case CENC_TYPE_CTR:
            aesControl->AesEnable = m_osInterface.osCpInterface->IsCpEnabled() ? true : false;
            break;
        default:
            break;
        }
    }

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpBase::SetMfxProtectionState(
    bool                        isDecodeInUse,
    PMOS_COMMAND_BUFFER         cmdBuffer,
    PMHW_BATCH_BUFFER           batchBuffer,
    PMHW_CP_SLICE_INFO_PARAMS   sliceInfoParam)
{
    MHW_PAVP_AES_PARAMS aesParams;

    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(sliceInfoParam)

    if (m_osInterface.osCpInterface->IsCpEnabled() && GetHostEncryptMode() == CP_TYPE_BASIC)
    {
        MOS_ZeroMemory(&aesParams, sizeof(aesParams));
        aesParams.presDataBuffer = sliceInfoParam->presDataBuffer;
        if (GetCpEncryptionType() == CP_SECURITY_DECE_AES128_CTR)
        {
            aesParams.dwDataLength      = sliceInfoParam->dwDataLength[1];
            aesParams.dwDataStartOffset = sliceInfoParam->dwDataStartOffset[1];
        }
        else
        {
            aesParams.dwDataLength      = sliceInfoParam->dwDataLength[0];
            aesParams.dwDataStartOffset = sliceInfoParam->dwDataStartOffset[0];
        }
        aesParams.dwSliceIndex                         = sliceInfoParam->dwSliceIndex;
        aesParams.dwTotalBytesConsumed                 = sliceInfoParam->dwTotalBytesConsumed;
        aesParams.bLastPass                            = sliceInfoParam->bLastPass;
        aesParams.dwDefaultDeceInitPacketLengthInBytes = MHW_PAVP_DECE_NUM_CLEAR_BYTES_IN_SLICE;

        MHW_CP_CHK_STATUS(this->AddMfxAesState(isDecodeInUse, cmdBuffer, batchBuffer, &aesParams));
    }

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpBase::SetHcpProtectionState(
    bool                        isDecodeInUse,
    PMOS_COMMAND_BUFFER         cmdBuffer,
    PMHW_BATCH_BUFFER           batchBuffer,
    PMHW_CP_SLICE_INFO_PARAMS   sliceInfoParam)
{
    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(sliceInfoParam);
    MHW_CP_CHK_NULL(m_mhwMiInterface);

    if (m_osInterface.osCpInterface->IsCpEnabled() && GetHostEncryptMode() == CP_TYPE_BASIC)
    {
        MHW_PAVP_AES_PARAMS aesParams;
        MOS_ZeroMemory(&aesParams, sizeof(aesParams));
        aesParams.presDataBuffer = sliceInfoParam->presDataBuffer;
        aesParams.dwOffset = sliceInfoParam->dwSliceIndex * sizeof(HUC_INDIRECT_CRYPTO_STATE_G8LP);
        if (GetCpEncryptionType() == CP_SECURITY_DECE_AES128_CTR)
        {
            aesParams.dwDataLength      = sliceInfoParam->dwDataLength[1];
            aesParams.dwDataStartOffset = sliceInfoParam->dwDataStartOffset[1];
        }
        else
        {
            aesParams.dwDataLength      = sliceInfoParam->dwDataLength[0];
            aesParams.dwDataStartOffset = sliceInfoParam->dwDataStartOffset[0];
        }
        aesParams.dwSliceIndex                         = sliceInfoParam->dwSliceIndex;
        aesParams.dwTotalBytesConsumed                 = sliceInfoParam->dwTotalBytesConsumed;
        aesParams.bLastPass                            = sliceInfoParam->bLastPass;
        aesParams.dwDefaultDeceInitPacketLengthInBytes = MHW_PAVP_DECE_NUM_CLEAR_BYTES_IN_SLICE;
        aesParams.bShortFormatInUse                    = 0;
        MHW_CP_CHK_STATUS(this->AddHcpAesState(isDecodeInUse, cmdBuffer, batchBuffer, &aesParams));
    }

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

bool MhwCpBase::IsSplitHucStreamOutNeeded()
{
    return false;
}

bool MhwCpBase::IsHwCounterIncrement(PMOS_INTERFACE pOsInterface)
{
    return false;
}

void MhwCpBase::RegisterParams(void* params)
{
    MHW_CP_FUNCTION_ENTER;
    MosCp *mosCp = dynamic_cast<MosCp*>(m_osInterface.osCpInterface);
    MHW_CP_CHK_NULL_NO_STATUS_RETURN(mosCp);

    if (params)
    {
        // register the pointer.
        m_userEncryptionParams = static_cast<PMHW_PAVP_ENCRYPTION_PARAMS>(params);
        // initialized CP context from cp tag
        mosCp->UpdateCpContext(m_userEncryptionParams->CpTag);
    }

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
}

MOS_STATUS MhwCpBase::UpdateParams(bool isInput)
{

    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(m_userEncryptionParams);
    MHW_CP_CHK_NULL(m_osInterface.osCpInterface);

    MosCp *pMosCp = dynamic_cast<MosCp*>(m_osInterface.osCpInterface);
    MHW_CP_CHK_NULL(pMosCp);

    if (isInput)
    {
        // update internal encryptionParams from user.
        m_encryptionParams = *m_userEncryptionParams;
        // update CP context from cp tag
        pMosCp->UpdateCpContext(m_encryptionParams.CpTag);
    }
    else
    {
        // current encryption params write back to user if needed
        *m_userEncryptionParams = m_encryptionParams;
    }

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

PMHW_PAVP_ENCRYPTION_PARAMS MhwCpBase::GetEncryptionParams()
{
    MHW_CP_FUNCTION_ENTER;
    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return &m_encryptionParams;
}

void MhwCpBase::SetHostEncryptMode(CP_MODE mode)
{
    m_encryptionParams.HostEncryptMode = mode;
}

CP_MODE MhwCpBase::GetHostEncryptMode() const
{
    return m_encryptionParams.HostEncryptMode;
}

void MhwCpBase::SetCpSecurityType(CP_SECURITY_TYPE type)
{
    this->m_encryptionParams.PavpEncryptionType = type;
}

CP_SECURITY_TYPE MhwCpBase::GetCpEncryptionType() const
{
    return m_encryptionParams.PavpEncryptionType;
}

void MhwCpBase::SetCpAesCounter(uint32_t *counter)
{
    if (counter)
    {
        MOS_SecureMemcpy(this->m_encryptionParams.dwPavpAesCounter,
            CODEC_DWORDS_PER_AES_BLOCK * sizeof(uint32_t),
            counter,
            CODEC_DWORDS_PER_AES_BLOCK * sizeof(uint32_t));
    }
}

uint32_t *MhwCpBase::GetCpAesCounter()
{
    return this->m_encryptionParams.dwPavpAesCounter;
}

CpHwUnit::CpHwUnit(MOS_INTERFACE& osInterface) : m_mhwMiInterface(nullptr),
                                                 m_osInterface(osInterface)
{
    if (osInterface.bUsesGfxAddress)
    {
        pfnAddResourceToCmd = Mhw_AddResourceToCmd_GfxAddress;
    }
    else  //if (osInterface.bUsesPatchList)
    {
        pfnAddResourceToCmd = Mhw_AddResourceToCmd_PatchList;
    }

    MOS_ZeroMemory(&m_CryptoKeyExParams, sizeof(MHW_PAVP_MFX_CRYPTO_KEY_EXCHANGE_CMD_PARAMS));
    m_bValidKeySet = false;
}

MOS_STATUS CpHwUnit::AddCheckForEarlyExit(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    MOS_UNUSED(osInterface);
    MOS_UNUSED(cmdBuffer);
    return MOS_STATUS_SUCCESS;
}
MOS_STATUS CpHwUnit::SaveEncryptionParams(PMHW_PAVP_ENCRYPTION_PARAMS pEncParams)
{
    MOS_UNUSED(pEncParams);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS CpHwUnit::AddMiSetAppId(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer,
    uint32_t            appId)
{
    MI_SET_APP_ID_CMD miSetAppidCmd = g_cInit_MI_SET_APP_ID_CMD;

    MHW_CP_FUNCTION_ENTER;

    MOS_UNUSED(osInterface);

    // A mechanism for UMD to derive the PAVP App ID values.
    miSetAppidCmd.DW0.Value |= appId & PAVP_SESSION_ID_MASK;

    MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, &miSetAppidCmd, sizeof(miSetAppidCmd)));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS CpHwUnit::AddMfxCryptoInlineStatusRead(
    PMOS_INTERFACE                                     osInterface,
    PMOS_COMMAND_BUFFER                                cmdBuffer,
    PMHW_PAVP_MFX_CRYPTO_INLINE_STATUS_READ_CMD_PARAMS params)
{
    MHW_CP_FUNCTION_ENTER;

    MOS_UNUSED(osInterface);
    MOS_UNUSED(cmdBuffer);
    MOS_UNUSED(params);
    return MOS_STATUS_UNIMPLEMENTED;
}

MOS_STATUS CpHwUnit::RegisterMiInterface(
    MhwMiInterface* miInterface)
{
    // miInterface was already checked in MhwCpInterface::RegisterMiInterface()
    // no need to re-check here
    m_mhwMiInterface = miInterface;
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS CpHwUnit::RegisterMiInterfaceNext(
    std::shared_ptr<mhw::mi::Itf> m_miItfIn)
{
    // miInterface was already checked in MhwCpInterface::RegisterMiInterface()
    // no need to re-check here
    m_miItf = m_miItfIn;
    return MOS_STATUS_SUCCESS;
}

// ---------- common RCS unit implement ----------
CpHwUnitRcs::CpHwUnitRcs(MOS_STATUS& eStatus, MOS_INTERFACE& osInterface) : CpHwUnit(osInterface)
{
    eStatus = MOS_STATUS_SUCCESS;
}

MOS_STATUS CpHwUnitRcs::AddEpilog(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    // Render
    PIPE_CONTROL_PAVP*      pipeControl = nullptr;
    MHW_PIPE_CONTROL_PARAMS pipeControlParams;

    MOS_FUNCTION_ENTER(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW);

    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(cmdBuffer);
    MHW_CP_CHK_NULL(m_mhwMiInterface);

    MOS_ZeroMemory(&pipeControlParams, sizeof(pipeControlParams));
    pipeControlParams.dwFlushMode = MHW_FLUSH_WRITE_CACHE;

    // get the address of the PIPE_CONTROL going to be added
    pipeControl = reinterpret_cast<PIPE_CONTROL_PAVP*>(cmdBuffer->pCmdPtr + 1);  // DW1

    // actually add the PIPE_CONTROL
    MHW_CP_CHK_STATUS(this->m_mhwMiInterface->AddPipeControl(cmdBuffer, nullptr, &pipeControlParams));

    // set the ProtectedMemoryDisable bit to ON for Gen8+
    pipeControl->ProtectedMemoryDisable = osInterface->osCpInterface->IsHMEnabled();

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

// ---------- common VECS unit implement ----------
CpHwUnitVecs::CpHwUnitVecs(MOS_STATUS& eStatus, MOS_INTERFACE& osInterface) : CpHwUnit(osInterface)
{
    eStatus = MOS_STATUS_SUCCESS;
}

MOS_STATUS CpHwUnitVecs::AddProlog(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    // VEBox
    MHW_MI_FLUSH_DW_PARAMS miFlushDwParams;

    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(osInterface->osCpInterface);

    MHW_CP_CHK_NULL(cmdBuffer);
    MHW_CP_CHK_NULL(m_mhwMiInterface);

    MosCp *pMosCp = dynamic_cast<MosCp*>(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(pMosCp);

    MOS_ZeroMemory(&miFlushDwParams, sizeof(miFlushDwParams));
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiFlushDwCmd(cmdBuffer, &miFlushDwParams));

    // Send MI_SET_APPID
    MHW_CP_CHK_STATUS(AddMiSetAppId(osInterface, cmdBuffer, pMosCp->GetSessionId()));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS CpHwUnitVecs::AddEpilog(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    // VEBox
    MHW_MI_FLUSH_DW_PARAMS miFlushDwParams;

    MOS_UNUSED(osInterface);
    MHW_CP_CHK_NULL(m_mhwMiInterface);
    MHW_CP_CHK_NULL(cmdBuffer);

    // get the address of the MI_FLUSH_DW going to be added
    auto miFlushDw = reinterpret_cast<MI_FLUSH_DW_PAVP*>(cmdBuffer->pCmdPtr);
    MOS_ZeroMemory(&miFlushDwParams, sizeof(miFlushDwParams));
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiFlushDwCmd(cmdBuffer, &miFlushDwParams));
    // but patch the protection bit off
    miFlushDw->ProtectedMemoryEnable = 0;

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

// ---------- common VCS unit implement ----------
CpHwUnitVcs::CpHwUnitVcs(MOS_STATUS& eStatus, MOS_INTERFACE& osInterface) : CpHwUnit(osInterface)
{
    MOS_ZeroMemory(&m_VcsEncryptionParams, sizeof(MHW_PAVP_ENCRYPTION_PARAMS));
    eStatus = MOS_STATUS_SUCCESS;
    MOS_ZeroMemory(&m_VcsEncryptionParams, sizeof(m_VcsEncryptionParams));
    MOS_ZeroMemory(m_cacheEncryptDecryptKey, sizeof(m_cacheEncryptDecryptKey));
}

MOS_STATUS CpHwUnitVcs::AddProlog(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    // VDBox
    MHW_MI_FLUSH_DW_PARAMS miFlushDwParams;

    MHW_CP_FUNCTION_ENTER;
    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(osInterface->osCpInterface);

    MosCp *pMosCp = dynamic_cast<MosCp*>(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(pMosCp);

    MHW_CP_CHK_NULL(cmdBuffer);
    MHW_CP_CHK_NULL(m_mhwMiInterface);

    MOS_ZeroMemory(&miFlushDwParams, sizeof(miFlushDwParams));
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiFlushDwCmd(cmdBuffer, &miFlushDwParams));

    // Send MI_SET_APPID
    MHW_CP_CHK_STATUS(AddMiSetAppId(osInterface, cmdBuffer, pMosCp->GetSessionId()));
    // Send MFX_WAIT after set app id
    // This wait cmd is needed to make sure pavp command is done before decode starts
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMfxWaitCmd(cmdBuffer, nullptr, false));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS CpHwUnitVcs::AddEpilog(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    // VDBox
    MHW_MI_FLUSH_DW_PARAMS miFlushDwParams;

    MOS_UNUSED(osInterface);
    MHW_CP_CHK_NULL(m_mhwMiInterface);
    MHW_CP_CHK_NULL(cmdBuffer);

    // get the address of the MI_FLUSH_DW going to be added
    auto miFlushDw = reinterpret_cast<MI_FLUSH_DW_PAVP*>(cmdBuffer->pCmdPtr);
    MOS_ZeroMemory(&miFlushDwParams, sizeof(miFlushDwParams));
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiFlushDwCmd(cmdBuffer, &miFlushDwParams));
    // but patch the protection bit off
    miFlushDw->ProtectedMemoryEnable = 0;

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS CpHwUnitVcs::AddMfxCryptoKeyExchange(
    PMOS_COMMAND_BUFFER                          cmdBuffer,
    PMHW_PAVP_MFX_CRYPTO_KEY_EXCHANGE_CMD_PARAMS params)
{
    MHW_CP_FUNCTION_ENTER;

    auto cmd = g_cInit_MFX_CRYPTO_KEY_EXCHANGE_CMD;

    // Populate the fields as requested by the caller.
    switch (params->KeyExchangeKeyUse)
    {
    case CRYPTO_TERMINATE_KEY:
        cmd.DW1.KeyUseIndicator = CRYPTO_TERMINATE_KEY;
        MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, &cmd, sizeof(cmd)));
        break;
    case CRYPTO_DECRYPT_AND_USE_NEW_KEY:
        cmd.DW1.KeyUseIndicator = CRYPTO_DECRYPT_AND_USE_NEW_KEY;
        if (params->KeyExchangeKeySelect & MHW_CRYPTO_DECRYPTION_KEY)
        {
            // DWORDLength is the number of DWORDs taken by the keys
            cmd.DW0.DWordLength            = KEY_DWORDS_PER_AES_BLOCK;
            cmd.DW1.DecryptionOrEncryption = CRYPTO_DECRYPT;
            MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, &cmd, sizeof(cmd)));
            MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, params->EncryptedDecryptionKey, sizeof(params->EncryptedDecryptionKey)));
        }
        if (params->KeyExchangeKeySelect & MHW_CRYPTO_ENCRYPTION_KEY)
        {
            // DWORDLength is the number of DWORDs taken by the keys
            cmd.DW0.DWordLength            = KEY_DWORDS_PER_AES_BLOCK;
            cmd.DW1.DecryptionOrEncryption = CRYPTO_ENCRYPT;
            MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, &cmd, sizeof(cmd)));
            // Populate the key blob.
            MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, params->EncryptedEncryptionKey, sizeof(params->EncryptedEncryptionKey)));
        }
        break;
    case CRYPTO_USE_NEW_FRESHNESS_VALUE:
        cmd.DW1.KeyUseIndicator = CRYPTO_USE_NEW_FRESHNESS_VALUE;
        if (params->KeyExchangeKeySelect == MHW_CRYPTO_DECRYPTION_KEY)
        {
            cmd.DW1.DecryptionOrEncryption = CRYPTO_DECRYPT;
        }
        else
        {
            cmd.DW1.DecryptionOrEncryption = CRYPTO_ENCRYPT;
        }
        MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, &cmd, sizeof(cmd)));
        break;
    default:
        return MOS_STATUS_INVALID_PARAMETER;
        break;
    }

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS CpHwUnitVcs::SaveEncryptionParams(PMHW_PAVP_ENCRYPTION_PARAMS pEncParams)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(pEncParams);

    //Key exchange command will be added when "pEncParams->bInsertKeyExinVcsProlog == true" on GEN11+
    //Key exchange command will be added when "pEncParams->bInsertKeyExinVcsProlog == true" and key blob not match with cached key on GEN8/GEN9/GEN10
    eStatus = MOS_SecureMemcpy(
        &m_VcsEncryptionParams,
        sizeof(MHW_PAVP_ENCRYPTION_PARAMS),
        pEncParams,
        sizeof(MHW_PAVP_ENCRYPTION_PARAMS));

    MHW_CP_FUNCTION_EXIT(eStatus);

    return eStatus;
}

// ---------- common GSC unit implement ----------
CpHwUnitGcs::CpHwUnitGcs(MOS_STATUS& eStatus, MOS_INTERFACE& osInterface) : CpHwUnit(osInterface)
{
    eStatus = MOS_STATUS_SUCCESS;
}

MOS_STATUS CpHwUnitGcs::AddProlog(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{

    MOS_UNUSED(osInterface);
    MOS_UNUSED(cmdBuffer);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS CpHwUnitGcs::AddEpilog(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{

    MOS_UNUSED(osInterface);
    MOS_UNUSED(cmdBuffer);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS CpHwUnitGcs::AddGscHeader()
{
    return MOS_STATUS_UNIMPLEMENTED;
}

MOS_STATUS CpHwUnitGcs::AddHeciHeader()
{
    return MOS_STATUS_UNIMPLEMENTED;
}


void MhwCpBase::IncrementCounter(
    uint32_t (&counter)[CODEC_DWORDS_PER_AES_BLOCK],
    uint32_t increment,
    uint32_t size)
{
    if (size == MHW_IV_SIZE_8)
    {
        *((uint64_t*)counter) += increment;
    }
    else if (size == MHW_IV_SIZE_16)
    {
        uint64_t curCounterLow = *((int64_t*)counter);
        *((uint64_t*)counter) += increment;
        if (*((uint64_t*)counter) < curCounterLow)
        {
            // carry
            *((uint64_t*)(counter+2)) += 1;
        }
    }
    else
    {
        MHW_ASSERTMESSAGE("IV size is not correct!");
        MT_ERR1(MT_CP_MHW_IV_SIZE, MT_CP_IV_SIZE, size);
    }
}

void MhwCpBase::CalcAesCbcSkipState(
                                         PMHW_PAVP_AES_PARAMS pParams,
                                         bool       *pbInitialTypeEncrypted,
                                         uint32_t   *pdwInitialSize,
                                         uint32_t   *pdwAesCbcIv,
                                         bool       *pbLastBlock,
                                         uint8_t    *frameBase)
{
    PMHW_PAVP_ENCRYPTION_PARAMS pEncryptionParams = GetEncryptionParams();

    uint32_t initclr;
    uint32_t initBytes;
    uint32_t handlePartialBlk;
    uint32_t slcOffset   = pParams->dwDataStartOffset - pEncryptionParams->dwPavpAesCbcSkipFrameOffset;
    uint32_t slcEndOffset = slcOffset + pParams->dwDataLength;
    uint32_t frameEndOffset = pEncryptionParams->dwPavpAesCbcSkipFrameEndOffset;

    uint32_t param_a = pEncryptionParams->dwPavpAesCbcSkipInitial;
    uint32_t param_b = pEncryptionParams->dwPavpAesCbcSkipEncrypted;
    uint32_t param_c = pEncryptionParams->dwPavpAesCbcSkipClear;

    handlePartialBlk = (slcEndOffset/16 == frameEndOffset/16) ? 1 : 0;

    frameBase += pEncryptionParams->dwPavpAesCbcSkipFrameOffset;

    //uint32_t iv_offset_value=0;
    if (slcOffset > param_a)
    {
        // Do the math here
        uint32_t blkSize      = param_b + param_c;
        uint32_t encSlcOffset = slcOffset - param_a;
        uint32_t blknum       = encSlcOffset / blkSize;
        uint32_t blkOffset    = encSlcOffset % blkSize;

        initclr      = blkOffset <= param_b ? 0 : 1;
        initBytes    = initclr ? blkSize - blkOffset : param_b - blkOffset;

        if (slcOffset/16 == frameEndOffset/16)
            initclr  = 1;

        if (encSlcOffset >= 16)
        {
            uint8_t* iv_offset;
            if (initclr)
                iv_offset = frameBase + param_a + blknum*blkSize + param_b - 16;
            else
            {
                if (blkOffset < 16)
                    iv_offset = frameBase + param_a + (blknum-1)*blkSize  + param_b - 16;
                else {
                    uint32_t num16byteblocks = blkOffset / 16;
                    iv_offset = frameBase + param_a + blknum*blkSize + (num16byteblocks-1)*16;
                }

            }
          //  iv_offset_value = iv_offset - frameBase;
            MOS_SecureMemcpy(pdwAesCbcIv, CODEC_DWORDS_PER_AES_BLOCK * sizeof(uint32_t),
                             iv_offset, CODEC_DWORDS_PER_AES_BLOCK * sizeof(uint32_t));
        }
    }
    else
    {
        initclr = 1;
        initBytes = param_a - slcOffset;
    }

    *pbInitialTypeEncrypted = initclr ? false : true;
    *pdwInitialSize = initBytes;
    *pbLastBlock = handlePartialBlk;
}

uint32_t MhwCpBase::GetSizeOfCmdIndirectCryptoState() const
{
    return m_sizeOfCmdIndirectCryptoState;
}

MOS_STATUS MhwCpBase::GetTranscryptKernelInfo(
    const uint32_t **transcryptMetaData,
    uint32_t     *transcryptMetaDataSize,
    const uint32_t                  **encryptedKernels,
    uint32_t                        *encryptedKernelsSize,
    MHW_PAVP_KERNEL_ENCRYPTION_TYPE encryptionType)
{
    MHW_CP_CHK_NULL(transcryptMetaData);
    MHW_CP_CHK_NULL(transcryptMetaDataSize);
    MHW_CP_CHK_NULL(encryptedKernels);
    MHW_CP_CHK_NULL(encryptedKernelsSize);

    return MOS_STATUS_PLATFORM_NOT_SUPPORTED;
}

MOS_STATUS MhwCpBase::InitCpCmdProps(
    PLATFORM                   platform,
    const MOS_HW_COMMAND       hwCommandType,
    MOS_CP_COMMAND_PROPERTIES* cpCmdProps,
    const uint32_t             forceDwordOffset)
{

    if (cpCmdProps == nullptr)
    {
        return MOS_STATUS_NULL_POINTER;
    }

    if(m_cpCmdPropsMap == nullptr)
    {
        return MOS_STATUS_UNKNOWN;
    }

    MOS_ZeroMemory(cpCmdProps, sizeof(MOS_CP_COMMAND_PROPERTIES));

    const auto it = m_cpCmdPropsMap->find(hwCommandType);
    if (it == m_cpCmdPropsMap->end())
    {
        // HW command does not have encrypted data bits in MOCS
        if (hwCommandType == MOS_MFX_CC_BASE_ADDR_STATE)
        {
            cpCmdProps->ForceEncryptionForNoMocs = true;
        }
        return MOS_STATUS_SUCCESS;
    }

    const auto& props            = it->second;
    cpCmdProps->HasDecryptBits   = props.hasDecryptBits;
    cpCmdProps->DecryptBitNumber = props.bitNumber;
    cpCmdProps->DecryptBitDwordOffset =
        (forceDwordOffset != 0) ? forceDwordOffset : props.dwordOffset;

    return MOS_STATUS_SUCCESS;
}


