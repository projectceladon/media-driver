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
//! \file     mhw_cp_hwcmd_common.c
//! \brief    hardware commands' initializers for content protection 
//!

#include "mhw_cp_hwcmd_common.h"

extern const MFX_CRYPTO_COPY_BASE_ADDR_CMD_G8 g_cInit_MFX_CRYPTO_COPY_BASE_ADDR_CMD_G8 =
{
    // DW 0
    {
        OP_LENGTH(SIZE32(MFX_CRYPTO_COPY_BASE_ADDR_CMD_G8)),   // DWordLength
        MEDIASUBOP_B_MFX_CRYPTO_COPY_BASE_ADDR,             // CommandSubOpcodeB
        MEDIASUBOP_A_MFX_COMMON,                            // CommandSubOpcodeA
        MEDIAOP_MEDIA_CRYPTO_MFX,                           // CommandOpcode
        PIPE_MEDIA,                                         // CommandPipeLine
        INSTRUCTION_GFX                                     // CommandType
    },

    // DW 1
    {
        0                                                   // SrcAddr
    },

    // DW 2
    {
        0                                                   // SrcAddr
    },

    // DW 3
    {
        0                                                   // SrcAddrUpperBound
    },

    // DW 4
    {
        0                                                   // SrcAddrUpperBound
    },

    // DW 5
    {
        0                                                   // DstAddr
    },

    // DW 6
    {
        0                                                   // DstAddr
    }
};

extern const MFX_CRYPTO_COPY_CMD_G8 g_cInit_MFX_CRYPTO_COPY_CMD_G8 =
{
    // DW 0
    {
        OP_LENGTH(SIZE32(MFX_CRYPTO_COPY_CMD_G8)),    // DWordLength
        MEDIASUBOP_B_MFX_CRYPTO_COPY,                   // CommandSubOpcodeB
        MEDIASUBOP_A_MFX_COMMON,                        // CommandSubOpcodeA
        MEDIAOP_MEDIA_CRYPTO_MFX,                       // CommandOpcode
        PIPE_MEDIA,                                     // CommandPipeLine
        INSTRUCTION_GFX                                 // CommandType
    },

    // DW 1
    {
        0,                                              // IndirectDataLength
        0,                                              // UseIntialCounterValue
        MHW_PAVP_COUNTER_TYPE_C,                         // AesCounterFlavor (Only counter type 'C' (128 bit counter) is supported)
        0                                               // SelectionForIndirectData
    },

    // DW 2
    {
        0                                               // SrcBaseOffset
    },

    // DW 3
    {
        0                                               // SrcBaseOffset
    },

    // DW 4
    {
        0                                               // DstBaseOffset
    },

    // DW 5
    {
        0                                               // DstBaseOffset
    },

    // DW 6
    {
        0                                               // Counter
    },

    // DW 7
    {
        0                                               // Initialization value 1
    },

    // DW 8
    {
        0                                               // Initialization value 2
    },

    // DW 9
    {
        0                                               // Initialization value 3
    },

    // DW 10
    {
        0,                                              // InitialPacketLength
        0,                                              // AesOperationType
        0                                               // StartPacketType
    },

    // DW 11
    {
        0                                               // EncryptedBytesLength
    },

    // DW 12
    {
        0                                               // ClearBytesLength
    }
};

extern const MFX_CRYPTO_KEY_EXCHANGE_CMD g_cInit_MFX_CRYPTO_KEY_EXCHANGE_CMD =
{
    // DW 0
    {
        OP_LENGTH(SIZE32(MFX_CRYPTO_KEY_EXCHANGE_CMD)),  // DWordLength
        MEDIASUBOP_B_MFX_CRYPTO_KEY_EXCHANGE,               // CommandSubOpcodeB
        MEDIASUBOP_A_MFX_COMMON,                            // CommandSubOpcodeA
        MEDIAOP_MEDIA_CRYPTO_MFX,                           // CommandOpcode
        PIPE_MEDIA,                                         // CommandPipeLine
        INSTRUCTION_GFX                                     // CommandType
    },

    // DW 1
    {
        0,                                                  // DecryptionOrEncryption
        0                                                   // KeyUseIndicator
    }
};

extern const MFX_CRYPTO_INLINE_STATUS_READ_CMD_G8 g_cInit_MFX_CRYPTO_INLINE_STATUS_READ_CMD_G8 =
{
    // DW 0
    {
        OP_LENGTH(SIZE32(MFX_CRYPTO_INLINE_STATUS_READ_CMD_G8)),  // DWordLength
        MEDIASUBOP_B_MFX_CRYPTO_INLINE_STATUS_READ,               // CommandSubOpcodeB
        MEDIASUBOP_A_MFX_COMMON,                                  // CommandSubOpcodeA
        MEDIAOP_MEDIA_CRYPTO_MFX,                                 // CommandOpcode
        PIPE_MEDIA,                                               // CommandPipeLine
        INSTRUCTION_GFX                                           // CommandType
    },

    // DW 1
    {
        0,                                                  // InlineReadCmdType
        0                                                   // KeyRefreshRandomValueType
    },

    // DW 2
    {
        0                                                   // ReadDataStoreAddress
    },

    // DW 3
    {
        0                                                   // ReadDataStoreAddress
    },

    // DW 4
    {
        0                                                   // Nonce
    },

    // DW 5
    {
        0,                                                  // EnableForNotifyInterrupt
        0,                                                  // DestAddressType
        0                                                   // GfxAddress
    },

    // DW 6
    {
        0                                                   // GfxAddress
    },

    // DW 7
    {
        0                                                   // ImmediateWriteData1
    },

    // DW 8
    {
        0                                                   // ImmediateWriteData1
    }
};

extern const MI_SET_APP_ID_CMD                   g_cInit_MI_SET_APP_ID_CMD =
{
    // DW 0
    {
        0,                                                  // ApplicationId
        0,                                                  // ApplicationType
        0,                                                  // SaveInhibit
        0,                                                  // RestoreInhibit
        MI_SET_APP_ID,                                      // InstructionOpcode
        INSTRUCTION_MI                                      // InstructionType
    }
};

extern const HCP_AES_STATE_CMD g_cInit_HCP_AES_STATE_CMD =
{
    // DW 0
    {
        {
            OP_LENGTH(SIZE32(HCP_AES_STATE_CMD)),              // DwordLength
            MEDIASUBOP_MEDIA_HCP_AES_STATE,                         // MediaInstructionCommand
            GFXOP_HCP,                                              // MediaInstructionOpcode
            PIPE_MEDIA,                                             // PipelineType
            INSTRUCTION_GFX                                         // CommandType
        }
    },

    // DW 1
    {
        { 0 }                                                   // InitializationValue0
    },

    // DW 2
    {
        { 0 }                                                   // InitializationValue1
    },

    // DW 3
    {
        { 0 }                                                   // InitializationValue2
    },

    // DW 4
    {
        { 0 }                                                   // InitializationValue3
    },


    // DW 5
    {
        { 0 }                                                   // 
    },

    // DW 6
    {
        { 0 }                                                  //
    },

    // DW 7
    {
        { 0, 0, 0 }                                                    //
    },

    // DW 8
    {
        { 0, 0 }                                                    // 
    },

    // DW 9
    {
        { 0 }                                                    // 
    }
};

extern const AVP_AES_STATE_CMD g_cInit_AVP_AES_STATE_CMD =
{
    // DW 0
    {
        {
            OP_LENGTH(SIZE32(AVP_AES_STATE_CMD)),              // DwordLength
            MEDIASUBOP_MEDIA_AVP_AES_STATE,                         // MediaInstructionCommand
            GFXOP_AVP,                                              // MediaInstructionOpcode
            PIPE_MEDIA,                                             // PipelineType
            INSTRUCTION_GFX                                         // CommandType
        }
    },

    // DW 1
    {
        { 0 }                                                   // InitializationValue0
    },

    // DW 2
    {
        { 0 }                                                   // InitializationValue1
    },

    // DW 3
    {
        { 0 }                                                   // InitializationValue2
    },

    // DW 4
    {
        { 0 }                                                   // InitializationValue3
    },


    // DW 5
    {
        { 0 }                                                   // 
    },

    // DW 6
    {
        { 0 }                                                  //
    },

    // DW 7
    {
        { 0, 0, 0 }                                                    //
    },

    // DW 8
    {
        { 0, 0 }                                                    // 
    },

    // DW 9
    {
        { 0 }                                                    // 
    }
};

extern const VVCP_AES_STATE_CMD g_cInit_VVCP_AES_STATE_CMD =
{
    // DW 0
    {
        {
            OP_LENGTH(SIZE32(VVCP_AES_STATE_CMD)),              // DwordLength
            MEDIASUBOP_MEDIA_VVCP_AES_STATE,                    // MediaInstructionCommand
            GFXOP_VVCP,                                         // MediaInstructionOpcode
            PIPE_MEDIA,                                         // PipelineType
            INSTRUCTION_GFX                                     // CommandType
        }
    },

    // DW 1
    {
        { 0 }                                                   // InitializationValue0
    },

    // DW 2
    {
        { 0 }                                                   // InitializationValue1
    },

    // DW 3
    {
        { 0 }                                                   // InitializationValue2
    },

    // DW 4
    {
        { 0 }                                                   // InitializationValue3
    },


    // DW 5
    {
        { 0 }                                                   // 
    },

    // DW 6
    {
        { 0 }                                                  //
    },

    // DW 7
    {
        { 
            0, 
            0,                                                  // Initial Fair play or DECE Start Packet Type
            0                                                   // BitField: InitPktBytes length in Bytes. 
        }                                                       //
    },

    // DW 8
    {
        { 
            0,                                                   // Detect and Handle Partial Block at the end of Slice
            0 
        }                                                        // EncryptedBytes length in Bytes.
    },

    // DW 9
    {
        { 0 }                                                    // ClearBytes length in Bytes 
    }
};

extern const VVCP_AES_IND_STATE_CMD g_cInit_VVCP_AES_IND_STATE_CMD =
{
    // DW 0
    {
        {
            OP_LENGTH(SIZE32(VVCP_AES_IND_STATE_CMD)),           // DwordLength
            MEDIASUBOP_MEDIA_VVCP_AES_IND_STATE,                 // MediaInstructionCommand
            GFXOP_VVCP,                                          // MediaInstructionOpcode
            PIPE_MEDIA,                                          // PipelineType
            INSTRUCTION_GFX                                      // CommandType
        }
    },

    // DW 1 and DW 2
    {
        { { 0, 0 } },{ { 0 } }
    },

    // DW 3
    {
        { 0, 0, 0, 0, 0 }                                        // Indirect State Entry Address Attributes
    },

    // DW 4
    {
        { 0 }                                                   // NumberOfEntriesMinus1
    },

    // DW 5
    {
        { 0 }                                                   // AesCtrIv0
    },


    // DW 6
    {
        { 0 }                                                   // AesCtrIv1
    },

    // DW 7
    {
        { 0 }                                                   // AesCtrIv2
    },

    // DW 8
    {
        { 0 }                                                   // AesCtrIv3
    }
};

extern const MFX_AES_STATE_CMD_G8 g_cInit_MFX_AES_STATE_CMD_G8 =
{
    // DW 0
    {
        OP_LENGTH(SIZE32(MFX_AES_STATE_CMD_G8)),            // DWordLength
        MEDIASUBOP_B_MFX_AES_STATE,                         // CommandSubOpcodeB
        MEDIASUBOP_A_MFX_COMMON,                            // CommandSubOpcodeA
        MEDIAOP_MEDIA_COMMON_MFX,                           // CommandOpcode
        PIPE_MEDIA,                                         // CommandPipeLine
        INSTRUCTION_GFX                                     // CommandType
    },

    // DW 1
    {
        0                                                   // InitializationValue0
    },

    // DW 2
    {
        0                                                   // InitializationValue1
    },

    // DW 3
    {
        0                                                   // InitializationValue2
    },

    // DW 4
    {
        0                                                   // InitializationValue3
    },

    // DW 5
    {
        0,                                                  // AACSTotalPacketCount
        MEDIASTATE_USE_GTT_SETTINGS,                        // AACSBitVectorSurfaceCacheCtrl
        0,                                                  // AACSBitVectorSurfaceGfdtRefPic
        0,                                                  // AACSBitVectorSurfaceEncData
        0,                                                  // AACSBitVectorSurfaceArbCtrl
        0                                                   // AACSFunction
    },

    // DW 6
    {
        0xFFFFFFFF                                          // AACSBitVectorSurfaceStartAddress
    },

    // DW 7
    {
        0,                                                  // InitPacketBytesLength
        0,                                                  // IntialCounterValueUse
        0,                                                  // StartPacketType
    },

    // DW 8
    {
        0                                                   // EncryptedBytesLength
    },

    // DW 9
    {
        0                                                   // ClearBytesLength
    },

    // DW 10
    {
        0                                                   // AACSBitVectorSurfaceStartAddr
    },

    // DW 11
    {
        0                                                   // MBZ - BDW
    }
};

extern const MFX_AES_STATE_CMD_G9 g_cInit_MFX_AES_STATE_CMD_G9 =
{
    // DW 0
    {
        {
            OP_LENGTH(SIZE32(MFX_AES_STATE_CMD_G9)),            // DWordLength
            0,                                                  // Reserved
            MEDIASUBOP_B_MFX_AES_STATE,                         // CommandSubOpcodeB
            MEDIASUBOP_A_MFX_COMMON,                            // CommandSubOpcodeA
            MEDIAOP_MEDIA_COMMON_MFX,                           // CommandOpcode
            PIPE_MEDIA,                                         // CommandPipeLine
            INSTRUCTION_GFX                                     // CommandType
        }
    },

    // DW 1
    {
        { 0 }                                                   // InitializationValue0
    },

    // DW 2
    {
        { 0 }                                                   // InitializationValue1
    },

    // DW 3
    {
        { 0 }                                                   // InitializationValue2
    },

    // DW 4
    {
        { 0 }                                                   // InitializationValue3
    },

    // DW 5
    {
        {
            0,                                                  // Reserved
            0,                                                  // MVcounterEncryption
            0,                                                  // Reserved
        }
    },

    // DW 6
    {
        { 0, }                                                   // Reserved
    },

    // DW 7
    {
        {
            0,                                                   // InitPacketBytesLength
            0,                                                   // Reserved
            0,                                                   // IntialCounterValueUse
            0,                                                   // StartPacketType
        }
    },

    // DW 8
    {
        {
            0,                                                  // EncryptedByteslength
            0,                                                  // Reserved
            0,                                                  // LastPartialBlockAdj
        }
    },

    // DW 9
    {
        {
            0,                                                   // NumberByteInputBitstream
            0
        }
    }
};

extern const HUC_AES_STATE_CMD g_cInit_HUC_AES_STATE_CMD =
{
    // DW 0
    {
        {
            OP_LENGTH(SIZE32(HUC_AES_STATE_CMD)),              // DwordLength
            MEDIASUBOP_MEDIA_HUC_AES_STATE,                         // MediaInstructionCommand
            GFXOP_HUC,                                              // MediaInstructionOpcode
            PIPE_MEDIA,                                             // PipelineType
            INSTRUCTION_GFX                                         // CommandType
        }
    },

    // DW 1
    {
        { 0 }                                                   // InitializationValue0
    },

    // DW 2
    {
        { 0 }                                                   // InitializationValue1
    },

    // DW 3
    {
        { 0 }                                                   // InitializationValue2
    },

    // DW 4
    {
        { 0 }                                                   // InitializationValue3
    },


    // DW 5
    {
        { 0 }                                                   // 
    },

    // DW 6
    {
        { 0 }                                                  //
    },

    // DW 7
    {
        { 0, 0, 0 }                                                    //
    },

    // DW 8
    {
        { 0, 0 }                                                    // 
    },

    // DW 9
    {
        { 0 }                                                    // 
    }
};

extern const HCP_AES_IND_STATE_CMD g_cInit_HCP_AES_IND_STATE_CMD =
{
    // DW 0
    {
        {
            OP_LENGTH(SIZE32(HCP_AES_IND_STATE_CMD)),              // DwordLength
            MEDIASUBOP_MEDIA_HCP_AES_IND_STATE,                         // MediaInstructionCommand
            GFXOP_HCP,                                                  // MediaInstructionOpcode
            PIPE_MEDIA,                                                 // PipelineType
            INSTRUCTION_GFX                                             // CommandType
        }
    },

    // DW 1 and DW 2
    {
        { { 0, 0 } },{ { 0 } }
    },

    // DW 3
    {
        { 0, 0, 0, 0, 0 }                                                   // Indirect State Entry Address Attributes
    },

    // DW 4
    {
        { 0 }                                                   // NumberOfEntriesMinus1
    },

    // DW 5
    {
        { 0 }                                                   // AesCtrIv
    },


    // DW 6
    {
        { 0 }                                                   // AesCtrIv
    },

    // DW 7
    {
        { 0 }                                                   // AesCtrIv
    },

    // DW 8
    {
        { 0 }                                                   // AesCtrIv
    }
};

extern const AVP_AES_IND_STATE_CMD g_cInit_AVP_AES_IND_STATE_CMD =
{
    // DW 0
    {
        {
            OP_LENGTH(SIZE32(AVP_AES_IND_STATE_CMD)),              // DwordLength
            MEDIASUBOP_MEDIA_AVP_AES_IND_STATE,                         // MediaInstructionCommand
            GFXOP_AVP,                                                  // MediaInstructionOpcode
            PIPE_MEDIA,                                                 // PipelineType
            INSTRUCTION_GFX                                             // CommandType
        }
    },

    // DW 1 and DW 2
    {
        { { 0, 0 } },{ { 0 } }
    },

    // DW 3
    {
        { 0, 0, 0, 0, 0 }                                                   // Indirect State Entry Address Attributes
    },

    // DW 4
    {
        { 0 }                                                   // NumberOfEntriesMinus1
    },

    // DW 5
    {
        { 0 }                                                   // AesCtrIv
    },


    // DW 6
    {
        { 0 }                                                   // AesCtrIv
    },

    // DW 7
    {
        { 0 }                                                   // AesCtrIv
    },

    // DW 8
    {
        { 0 }                                                   // AesCtrIv
    }
};

extern const HUC_AES_IND_STATE_CMD g_cInit_HUC_AES_IND_STATE_CMD =
{
    // DW 0
    {
        {
            OP_LENGTH(SIZE32(HUC_AES_IND_STATE_CMD)),       // DwordLength
            MEDIASUBOP_MEDIA_HUC_AES_IND_STATE,                  // MediaInstructionCommand
            GFXOP_HUC,                                           // MediaInstructionOpcode
            PIPE_MEDIA,                                          // PipelineType
            INSTRUCTION_GFX                                      // CommandType
        }
    },

    // DW 1 and DW 2
    {

        { { 0, 0 } },{ { 0 } }
    },

    // DW 3
    {
        { 0, 0, 0, 0, 0 }                                                   // Indirect State Entry Address Attributes
    },

    // DW 4
    {
        { 0 }                                                   // NumberOfEntriesMinus1
    },

    // DW 5
    {
        { 0 }                                                   // AESCtrOrIVDW0
    },

    // DW 6
    {
        { 0 }                                                   // AESCtrOrIVDW1
    },

    // DW 7
    {
        { 0 }                                                   // AESCtrOrIVDW2
    },

    // DW 8
    {
        { 0 }                                                   // AESCtrOrIVDW3
    }
};

//-----------PIPELINE_SELECT_G9 and PIPELINE_SELECT_G11 is needed in addprolog function------------//

PIPELINE_SELECT_CMD_G9::PIPELINE_SELECT_CMD_G9()
{
    DW0.Value                                        = 0;        
    DW0.PipelineSelection                            = PIPELINE_SELECTION_3D;
    DW0.RenderSliceCommonPowerGateEnable             = RENDER_SLICE_COMMON_POWER_GATE_ENABLE_DISABLED;
    DW0.RenderSamplerPowerGateEnable                 = RENDER_SAMPLER_POWER_GATE_ENABLE_DISABLED;
    DW0.MediaSamplerDopClockGateEnable               = MEDIA_SAMPLER_DOP_CLOCK_GATE_ENABLE_DISABLED;
    DW0.ForceMediaAwake                              = FORCE_MEDIA_AWAKE_DISABLED;
    DW0._3DCommandSubOpcode                          = _3D_COMMAND_SUB_OPCODE_PIPELINE_SELECT;
    DW0._3DCommandOpcode                             = _3D_COMMAND_OPCODE_GFXPIPE_NONPIPELINED;
    DW0.CommandSubtype                               = COMMAND_SUBTYPE_GFXPIPE_SINGLE_DW;
    DW0.CommandType                                  = COMMAND_TYPE_GFXPIPE;

}

PIPELINE_SELECT_CMD_G11::PIPELINE_SELECT_CMD_G11()
{
    DW0.Value                                        = 0;        
    DW0.PipelineSelection                            = PIPELINE_SELECTION_3D;
    DW0.RenderSliceCommonPowerGateEnable             = RENDER_SLICE_COMMON_POWER_GATE_ENABLE_DISABLED;
    DW0.RenderSamplerPowerGateEnable                 = RENDER_SAMPLER_POWER_GATE_ENABLE_DISABLED;
    DW0.MediaSamplerDopClockGateEnable               = MEDIA_SAMPLER_DOP_CLOCK_GATE_ENABLE_DISABLED;
    DW0.ForceMediaAwake                              = FORCE_MEDIA_AWAKE_DISABLED;
    DW0._3DCommandSubOpcode                          = _3D_COMMAND_SUB_OPCODE_PIPELINE_SELECT;
    DW0._3DCommandOpcode                             = _3D_COMMAND_OPCODE_GFXPIPE_NONPIPELINED;
    DW0.CommandSubtype                               = COMMAND_SUBTYPE_GFXPIPE_SINGLE_DW;
    DW0.CommandType                                  = COMMAND_TYPE_GFXPIPE;

}