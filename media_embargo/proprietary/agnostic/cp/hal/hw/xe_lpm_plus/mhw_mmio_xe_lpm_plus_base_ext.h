/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2020
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
//! \file     mhw_mmio_xe_lpm_plus_base_ext.h
//! \brief    Defines common MMIO register access definitions for gen13.
//! \details
//!           This file is embargoed.
//!
#ifndef __MHW_MMIO_GEN13_EXT_H__
#define __MHW_MMIO_GEN13_EXT_H__

//CP Registers
#define MHW_PAVP_TRANSCODE_KCR_COUNTER_NONCE_REGISTER_G13                          (0x386880)
#define MHW_PAVP_WIDI_KCR_COUNTER_NONCE_REGISTER_G13                               (0x386860)

#define MHW_PAVP_VCR_DEBUG_STATUS_2_REGISTER_G13                                   (0x3860E8)
#define MHW_PAVP_KCR_DEBUG_STATUS_1_REGISTER_G13                                   (0x3860F4)
#define MHW_PAVP_ISO_DEC_SESSION_IN_PLAY_REGISTER_G13                              (0x386670)
#define MHW_PAVP_SESSION_IN_PLAY_REGISTER_G13                                      (0x386260)

#define MHW_PAVP_FWSTS_REGISTER_BASE_G13            (0x116000)
#define MHW_PAVP_FWSTS1_REGISTER_G13                (MHW_PAVP_FWSTS_REGISTER_BASE_G13 + 0xC40)

#endif  //__MHW_MMIO_G13_EXT_H__