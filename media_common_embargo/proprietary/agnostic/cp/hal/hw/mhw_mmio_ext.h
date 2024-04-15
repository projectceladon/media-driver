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
//! \file     mhw_mmio_ext.h
//! \brief    Defines common MMIO register access definitions used in the gen9/10/11.
//! \details  
//!           This file is embargoed.
//!
#ifndef __MHW_MMIO_EXT_H__
#define __MHW_MMIO_EXT_H__


// CP registers
#define MHW_PAVP_VCR_DEBUG_STATUS_2_REGISTER            (0x320E8)
#define MHW_PAVP_KCR_DEBUG_STATUS_1_REGISTER            (0x320F4)
#define MHW_PAVP_ISO_DEC_SESSION_IN_PLAY_REGISTER       (0x32670)
#define MHW_PAVP_SESSION_IN_PLAY_REGISTER               (0x32260)
#define MHW_PAVP_PLANE_ENABLE_MASKED_BIT                (1 << 26)
#define MHW_PAVP_UNSOL_ATTACK_MASKED_BIT                (1 << 31)
#define MHW_PAVP_ISOLATED_DECODE_KEYS_VALID_MASKED_BIT  (1 << 7)
#define MHW_PAVP_SERPENT_HEAVY_KEYS_VALID_MASKED_BIT    (1 << 5)

#define MHW_PAVP_GSC_MODE_MASKED_BITS                   (0xF)
#define MHW_PAVP_HUC_AUTHENTICATION_MASKED_BIT          (1 << 19)

#define MHW_REGISTER_INVALID                            (0x00000)
#define MHW_INVALID_SESSION_ID                          (0xFF)

#define MHW_RCS_GENERAL_PURPOSE_REGISTER_BASE           (0x02600)
#define MHW_RCS_GENERAL_PURPOSE_REGISTER(index)         (MHW_RCS_GENERAL_PURPOSE_REGISTER_BASE + 8 * (index))
#define MHW_RCS_GENERAL_PURPOSE_REGISTER_LO(index)      (MHW_RCS_GENERAL_PURPOSE_REGISTER_BASE + 8 * (index))
#define MHW_RCS_GENERAL_PURPOSE_REGISTER_HI(index)      (MHW_RCS_GENERAL_PURPOSE_REGISTER_BASE + 8 * (index) + 4)

//MMC
//                                                       Ctrl,    rBuffer0, rBuffer0Ex, rCache0, rCache0Ex, rCfg0,   rExCfg0, wBuffer0, wBuffer0Ex, wCache0, wCache0Ex, wCfg0,   wExCfg0
#define G_PRV_MMC_REG_SET_VDBOX                         {0x15300, 0x15100,  0x15700,    0x15180, 0x15780,   0x15000, 0x15400, 0x15200,  0x15800,    0x15280, 0x15880,   0x15080, 0x15500 }
#define G_PRV_MMC_REG_SET_VDBOX2                        {0x1A300, 0x1A100,  0x1A700,    0x1A180, 0x1A780,   0x1A000, 0x1A400, 0x1A200,  0x1A800,    0x1A280, 0x1A880,   0x1A080, 0x1A500 }
#define G_PRV_MMC_REG_SET_VEBOX                         {0x1B300, 0x1B100,  0x1B700,    0x1B180, 0x1B780,   0x1B000, 0x1B400, 0x1B200,  0x1B800,    0x1B280, 0x1B880,   0x1B080, 0x1B500 }

// AVP
#define AVP_AVP_BITSTREAM_BYTECOUNT_TILE_WITH_HEADER_AWM_REG0   0x1C2B48
 
#endif