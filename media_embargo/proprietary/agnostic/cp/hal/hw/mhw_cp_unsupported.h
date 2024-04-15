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
//! \file     mhw_cp_unsupported.h
//! \brief    Impelements the unsupported platform's functionalities for content protection
//!

#include "mhw_cp.h"

class MhwCpUnsupported: public MhwCpBase
{
public:
    // ----- interface functions ------------------------------------------- //
    MhwCpUnsupported(MOS_STATUS& mos_status, MOS_INTERFACE& osInterfaces);
    ~MhwCpUnsupported() override;

    MOS_STATUS AddProlog(
        PMOS_INTERFACE      osInterface,
        PMOS_COMMAND_BUFFER cmdBuffer) override;

    MOS_STATUS AddEpilog(
        PMOS_INTERFACE      osInterface,
        PMOS_COMMAND_BUFFER cmdBuffer) override;

    MOS_STATUS AddMiSetAppId(
        PMOS_INTERFACE      osInterface,
        PMOS_COMMAND_BUFFER cmdBuffer,
        uint32_t            appId) override;

    bool IsHwCounterIncrement(PMOS_INTERFACE osInterface) override;

    MOS_STATUS AddMfxAesState(
        bool                 isDecodeInUse,
        PMOS_COMMAND_BUFFER  cmdBuffer,
        PMHW_BATCH_BUFFER    batchBuffer,
        PMHW_PAVP_AES_PARAMS params) override;

    MOS_STATUS AddMfxCryptoCopyBaseAddr(
        PMOS_COMMAND_BUFFER               cmdBuffer,
        PMHW_PAVP_CRYPTO_COPY_ADDR_PARAMS params) override;

    MOS_STATUS AddMfxCryptoInlineStatusRead(
        PMOS_INTERFACE                                     osInterface,
        PMOS_COMMAND_BUFFER                                cmdBuffer,
        PMHW_PAVP_MFX_CRYPTO_INLINE_STATUS_READ_CMD_PARAMS params) override;

    MOS_STATUS AddMfxCryptoCopy(
        PMOS_COMMAND_BUFFER          cmdBuffer,
        PMHW_PAVP_CRYPTO_COPY_PARAMS params) override;

    MOS_STATUS AddMfxCryptoKeyExchange(
        PMOS_COMMAND_BUFFER                          cmdBuffer,
        PMHW_PAVP_MFX_CRYPTO_KEY_EXCHANGE_CMD_PARAMS params) override;

    MOS_STATUS AddHcpAesState(
        bool                 isDecodeInUse,
        PMOS_COMMAND_BUFFER  cmdBuffer,
        PMHW_BATCH_BUFFER    batchBuffer,
        PMHW_PAVP_AES_PARAMS params) override;

    MOS_STATUS AddVvcpAesIndState(
        bool                 isDecodeInUse,
        PMOS_COMMAND_BUFFER  cmdBuffer,
        PMHW_BATCH_BUFFER    batchBuffer,
        PMHW_PAVP_AES_PARAMS params) override;

    MOS_STATUS AddHucAesState(
        PMOS_COMMAND_BUFFER  cmdBuffer,
        PMHW_PAVP_AES_PARAMS params) override;

    MOS_STATUS AddHucAesIndState(
        PMOS_COMMAND_BUFFER  cmdBuffer,
        PMHW_PAVP_AES_PARAMS params) override;

    MOS_STATUS SetHucIndirectCryptoState(
        PMHW_PAVP_AES_IND_PARAMS params) override;

    MOS_STATUS SetVvcpIndirectCryptoState(
        PMHW_PAVP_AES_IND_PARAMS params) override;

    MOS_STATUS SetProtectionSettingsForMiFlushDw(
        PMOS_INTERFACE osInterface,
        void           *pCmd) override;
    MOS_STATUS SetProtectionSettingsForMfxPipeModeSelect(uint32_t *data) override;
    MOS_STATUS SetProtectionSettingsForHcpPipeModeSelect(uint32_t *data, bool scalableEncode = false) override;
    MOS_STATUS SetProtectionSettingsForHucPipeModeSelect(uint32_t *data) override;
    MOS_STATUS SetProtectionSettingsForReservedPipeModeSelect(uint32_t *data) override;
    MOS_STATUS SetMfxProtectionState(
        bool                        isDecodeInUse,
        PMOS_COMMAND_BUFFER         cmdBuffer,
        PMHW_BATCH_BUFFER           batchBuffer,
        PMHW_CP_SLICE_INFO_PARAMS   sliceInfoParam) override;

    void GetCpStateLevelCmdSize(uint32_t& cmdSize, uint32_t& patchListSize) override;

    void GetCpSliceLevelCmdSize(uint32_t& cmdSize, uint32_t& patchListSize) override;
};
