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
//! \file     mhw_cp_hwcmd_g11_X.cpp
//! \brief    HW command initializers for all Gen11-based platforms
//!
#include "mhw_cp_hwcmd_g11_X.h"
#include "mhw_cp_hwcmd_common.h"

using namespace mhw_pavp_g11_X;

MFX_CRYPTO_COPY_CMD::MFX_CRYPTO_COPY_CMD()
{
    DW0.DWordLength              = OP_LENGTH(dwSize);
    DW0.CommandSubOpcodeB        = MEDIASUBOP_B_MFX_CRYPTO_COPY;
    DW0.CommandSubOpcodeA        = MEDIASUBOP_A_MFX_COMMON;
    DW0.CommandOpcode            = MEDIAOP_MEDIA_CRYPTO_MFX;    
    DW0.CommandPipeLine          = PIPE_MEDIA;                  
    DW0.CommandType              = INSTRUCTION_GFX;          

    DW1.IndirectDataLength       = 0;
    DW1.WiDiKeyInuse             = 0;
    DW1.UseIntialCounterValue    = 0;
    DW1.AesCounterFlavor         = MHW_PAVP_COUNTER_TYPE_C;
    DW1.SelectionForIndirectData = 0;

    DW2.SrcBaseOffset            = 0; 

    DW3.SrcBaseOffset            = 0;

    DW4.DstBaseOffset            = 0;

    DW5.DstBaseOffset            = 0;

    DW6.Counter                  = 0;

    DW7.InitializationValueDw1   = 0;

    DW8.InitializationValueDw2   = 0;

    DW9.InitializationValueDw3   = 0;

    DW10.InitialPacketLength     = 0;
    DW10.AesOperationType        = 0;
    DW10.StartPacketType         = 0;

    DW11.EncryptedBytesLength    = 0;

    DW12.ClearBytesLength        = 0;

    DW13.cryptoCopyCmdMode       = 0;
    DW13.lengthOfTable           = 0;
}