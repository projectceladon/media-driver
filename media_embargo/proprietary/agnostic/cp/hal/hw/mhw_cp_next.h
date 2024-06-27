/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2022
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
//! \file     mhw_cp_next.h
//! \brief    MHW interface for content protection
//! \details  Impelements the base functionalities for content protection
//!

#ifndef __MHW_CP_NEXT_H__
#define __MHW_CP_NEXT_H__

#include <array>
#include <list>
#include "mhw_cp.h"

#define MHW_PAVP_AUTHENTICATED_KERNEL_SIP_BIT       (1 << 16)

//CP Registers
#define MHW_PAVP_TRANSCODE_KCR_COUNTER_NONCE_REGISTER                          (0x32880)
#define MHW_PAVP_WIDI_KCR_COUNTER_NONCE_REGISTER                               (0x32860)

//Crypto Key exchange definitions have changed.
enum MHW_CRYPTO_KEY_EXCHANGE_KEY_USE
{
    MHW_CRYPTO_TERMINATE_KEY           = 0,
    MHW_CRYPTO_DECRYPT_AND_USE_NEW_KEY = 1,
    MHW_CRYPTO_ENCRYPTION_OFF          = 2
};

class MhwCpNextBase : public MhwCpBase
{
public:
    // ----- interface functions ------------------------------------------- //
    MhwCpNextBase(MOS_STATUS& mos_status, MOS_INTERFACE& osInterface);
    virtual ~MhwCpNextBase();

    // ----- static functions shared by several Gens. ------------------------------------------- //
    static MOS_STATUS AddMfxCryptoCopyBaseAddr(
        MhwCpBase*                        mhwCp,
        PMOS_INTERFACE                    osInterface,
        PMOS_COMMAND_BUFFER               cmdBuffer,
        PMHW_PAVP_CRYPTO_COPY_ADDR_PARAMS params);

    static MOS_STATUS AddMfxAesState(
        MhwCpBase*           mhwCp,
        PMOS_INTERFACE       osInterface,
        bool                 isDecodeInUse,
        PMOS_COMMAND_BUFFER  cmdBuffer,
        PMHW_BATCH_BUFFER    batchBuffer,
        PMHW_PAVP_AES_PARAMS params);

    static MOS_STATUS AddMfxCryptoCopy(
        MhwCpBase*                   mhwCp,
        PMOS_COMMAND_BUFFER          cmdBuffer,
        PMHW_PAVP_CRYPTO_COPY_PARAMS params);

    MEDIA_CLASS_DEFINE_END(MhwCpNextBase)
};

const std::map<MOS_HW_COMMAND, DecryptBitOffset> g_decryptBitOffsets{
    // HW Command ID                        Driver Id Bit Num, Driver ID DWORD Offset
    {MOS_MI_BATCH_BUFFER_START_RCS,         DecryptBitOffset()}, // No decrypt bits
    {MOS_MFX_PIPE_BUF_ADDR,                 DecryptBitOffset(0,  2)},
    {MOS_MFX_INDIRECT_OBJ_BASE_ADDR,        DecryptBitOffset(0,  2)},
    {MOS_MFX_BSP_BUF_BASE_ADDR,             DecryptBitOffset(0,  2)},
    {MOS_MFX_AVC_DIRECT_MODE,               DecryptBitOffset(0,  2)},
    {MOS_MFX_VC1_DIRECT_MODE,               DecryptBitOffset(0,  2)},
    {MOS_MFX_VP8_PIC,                       DecryptBitOffset(0,  2)},
    {MOS_MFX_DBK_OBJECT,                    DecryptBitOffset(0,  2)},
    {MOS_VDENC_PIPE_BUF_ADDR,               DecryptBitOffset(0,  2)},
    {MOS_HUC_DMEM,                          DecryptBitOffset(0,  2)},
    {MOS_HUC_VIRTUAL_ADDR,                  DecryptBitOffset(0,  2)},
    {MOS_HUC_IND_OBJ_BASE_ADDR,             DecryptBitOffset(0,  2)},
    {MOS_MI_CONDITIONAL_BATCH_BUFFER_END,   DecryptBitOffset(20, -2)},
    {MOS_MI_BATCH_BUFFER_START,             DecryptBitOffset(9,  -1)},
    {MOS_SURFACE_STATE,                     DecryptBitOffset(24, -7)},
    {MOS_SURFACE_STATE_ADV,                 DecryptBitOffset(0,  -1)},
    {MOS_STATE_BASE_ADDR,                   DecryptBitOffset(4,  0)},
    {MOS_VEBOX_STATE,                       DecryptBitOffset(25, -1)},
    {MOS_VEBOX_DI_IECP,                     DecryptBitOffset(0,  0)},
    {MOS_VEBOX_TILING_CONVERT,              DecryptBitOffset(0,  0)},
    {MOS_SFC_STATE,                         DecryptBitOffset(0,  2)},
};

#endif