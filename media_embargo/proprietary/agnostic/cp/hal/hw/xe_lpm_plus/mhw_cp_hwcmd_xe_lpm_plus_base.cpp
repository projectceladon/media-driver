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
//! \file     mhw_cp_hwcmd_xe_lpm_plus_base.cpp
//! \brief    HW command initializers for all Gen13-based platforms
//!
#include "mhw_cp_hwcmd_xe_lpm_plus_base.h"

mhw_pavp_g13_X::GSC_HECI_CMD::GSC_HECI_CMD()
{
    DW0.Value                               = 0;
    DW0.DwordLength                         = __CODEGEN_OP_LENGTH(dwSize);
    DW0.CommandType                         = GSC_PIPE;
    DW0.CommandSubType                      = GSC_HECI_CMD_PKT;
    DW1_2.Value[0]                          = 0;
    DW1_2.Value[1]                          = 0;
    DW3.Value                               = 0;
    DW4_5.Value[0]                          = 0;
    DW4_5.Value[1]                          = 0;
    DW6.Value                               = 0;
    DW7.Value                               = 0;
};