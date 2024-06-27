/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2017
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
//! \file     mhw_cp_unsupported.cpp
//! \brief    Impelements the unsupported platform's functionalities for content protection
//!

#include "mhw_cp_unsupported.h"

static void unsupported()
{
    MOS_ASSERTMESSAGE(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW, "This platform has no PAVP support.");
    MT_ERR2(MT_CP_MHW_UNSUPPORTED, MT_COMPONENT, MOS_COMPONENT_CP, MT_SUB_COMPONENT, MOS_CP_SUBCOMP_MHW);
}

MhwCpUnsupported::MhwCpUnsupported(MOS_STATUS& mos_status, MOS_INTERFACE& osInterface) : MhwCpBase(osInterface)
{
    mos_status = MOS_STATUS_SUCCESS;
}

MhwCpUnsupported::~MhwCpUnsupported()
{
}

MOS_STATUS MhwCpUnsupported::AddProlog(
    PMOS_INTERFACE,
    PMOS_COMMAND_BUFFER)
{
    unsupported();
    return MOS_STATUS_UNKNOWN;
}

MOS_STATUS MhwCpUnsupported::AddEpilog(
    PMOS_INTERFACE,
    PMOS_COMMAND_BUFFER)
{
    unsupported();
    return MOS_STATUS_UNKNOWN;
}

MOS_STATUS MhwCpUnsupported::AddMiSetAppId(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer,
    uint32_t            appId)
{
    unsupported();
    return MOS_STATUS_UNKNOWN;
}

bool MhwCpUnsupported::IsHwCounterIncrement(PMOS_INTERFACE osInterface)
{
    unsupported();
    return false;
}

MOS_STATUS MhwCpUnsupported::AddMfxAesState(
    bool,
    PMOS_COMMAND_BUFFER,
    PMHW_BATCH_BUFFER,
    PMHW_PAVP_AES_PARAMS)
{
    unsupported();
    return MOS_STATUS_UNKNOWN;
}

MOS_STATUS MhwCpUnsupported::AddMfxCryptoCopyBaseAddr(
    PMOS_COMMAND_BUFFER,
    PMHW_PAVP_CRYPTO_COPY_ADDR_PARAMS)
{
    unsupported();
    return MOS_STATUS_UNKNOWN;
}

MOS_STATUS MhwCpUnsupported::AddMfxCryptoInlineStatusRead(
    PMOS_INTERFACE,
    PMOS_COMMAND_BUFFER,
    PMHW_PAVP_MFX_CRYPTO_INLINE_STATUS_READ_CMD_PARAMS)
{
    unsupported();
    return MOS_STATUS_UNKNOWN;
}

MOS_STATUS MhwCpUnsupported::AddMfxCryptoCopy(
    PMOS_COMMAND_BUFFER,
    PMHW_PAVP_CRYPTO_COPY_PARAMS)
{
    unsupported();
    return MOS_STATUS_UNKNOWN;
}

MOS_STATUS MhwCpUnsupported::AddMfxCryptoKeyExchange(
    PMOS_COMMAND_BUFFER                          cmdBuffer,
    PMHW_PAVP_MFX_CRYPTO_KEY_EXCHANGE_CMD_PARAMS pParams)
{
    unsupported();
    return MOS_STATUS_UNKNOWN;
}

MOS_STATUS MhwCpUnsupported::AddHcpAesState(
    bool,
    PMOS_COMMAND_BUFFER,
    PMHW_BATCH_BUFFER,
    PMHW_PAVP_AES_PARAMS)
{
    unsupported();
    return MOS_STATUS_UNKNOWN;
}

MOS_STATUS MhwCpUnsupported::AddVvcpAesIndState(
    bool,
    PMOS_COMMAND_BUFFER,
    PMHW_BATCH_BUFFER,
    PMHW_PAVP_AES_PARAMS)
{
    unsupported();
    return MOS_STATUS_UNKNOWN;
}

MOS_STATUS MhwCpUnsupported::AddHucAesState(
    PMOS_COMMAND_BUFFER,
    PMHW_PAVP_AES_PARAMS)
{
    unsupported();
    return MOS_STATUS_UNKNOWN;
}

MOS_STATUS MhwCpUnsupported::AddHucAesIndState(
    PMOS_COMMAND_BUFFER,
    PMHW_PAVP_AES_PARAMS)
{
    unsupported();
    return MOS_STATUS_UNKNOWN;
}

MOS_STATUS MhwCpUnsupported::SetHucIndirectCryptoState(
    PMHW_PAVP_AES_IND_PARAMS)
{
    unsupported();
    return MOS_STATUS_UNKNOWN;
}

MOS_STATUS MhwCpUnsupported::SetVvcpIndirectCryptoState(
    PMHW_PAVP_AES_IND_PARAMS)
{
    unsupported();
    return MOS_STATUS_UNKNOWN;
}

MOS_STATUS MhwCpUnsupported::SetProtectionSettingsForMiFlushDw(
    PMOS_INTERFACE,
    void *)
{
    unsupported();
    return MOS_STATUS_UNKNOWN;
}

MOS_STATUS MhwCpUnsupported::SetProtectionSettingsForMfxPipeModeSelect(
    uint32_t*)
{
    unsupported();
    return MOS_STATUS_UNKNOWN;
}

MOS_STATUS MhwCpUnsupported::SetProtectionSettingsForHcpPipeModeSelect(
    uint32_t*,
    bool)
{
    unsupported();
    return MOS_STATUS_UNKNOWN;
}

MOS_STATUS MhwCpUnsupported::SetProtectionSettingsForHucPipeModeSelect(
    uint32_t*)
{
    unsupported();
    return MOS_STATUS_UNKNOWN;
}

MOS_STATUS MhwCpUnsupported::SetProtectionSettingsForReservedPipeModeSelect(
        uint32_t *)
{
    unsupported();
    return MOS_STATUS_UNKNOWN;
}

MOS_STATUS MhwCpUnsupported::SetMfxProtectionState(
    bool,
    PMOS_COMMAND_BUFFER,
    PMHW_BATCH_BUFFER,
    PMHW_CP_SLICE_INFO_PARAMS)
{
    unsupported();
    return MOS_STATUS_UNKNOWN;
}

void MhwCpUnsupported::GetCpStateLevelCmdSize(uint32_t& cmdSize, uint32_t& patchListSize)
{
    unsupported();
    cmdSize = 0;
    patchListSize = 0;
    return;
}

void MhwCpUnsupported::GetCpSliceLevelCmdSize(uint32_t& cmdSize, uint32_t& patchListSize)
{
    unsupported();
    cmdSize = 0;
    patchListSize = 0;
    return;
}
