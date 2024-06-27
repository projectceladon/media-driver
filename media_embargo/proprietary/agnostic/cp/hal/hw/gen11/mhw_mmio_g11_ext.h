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
//! \file     mhw_mmio_gen11_ext.h
//! \brief    Defines common MMIO register access definitions for gen11.
//! \details  
//!           This file is embargoed.
//!
#ifndef __MHW_MMIO_GEN11_EXT_H__
#define __MHW_MMIO_GEN11_EXT_H__

//CP Registers
#define MHW_PAVP_TRANSCODE_KCR_COUNTER_NONCE_REGISTER_G11                          (0x32880)
#define MHW_PAVP_WIDI_KCR_COUNTER_NONCE_REGISTER_G11                               (0x32860)
//Defined register as VEBox0 MMIO and it will be converted to relative offset before sent to h/w
#define VPHAL_VEBOX_WATCHDOG_COUNT_CONTROL_REG_OFFSET_G11                          0x1CA178
#define VPHAL_VEBOX_WATCHDOG_COUNT_THRESHOLD_REG_OFFSET_G11                        0x1CA17C

 
#endif