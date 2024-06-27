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
//! \file     mhw_cp_hwcmd_common.h
//! \brief    CP command defintions for all platforms
//!

#ifndef __MHW_CP_HWCMD_COMMON_H__
#define __MHW_CP_HWCMD_COMMON_H__

#include "mhw_utilities.h"
#include "mos_os.h"
#include "mos_util_debug.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifndef SIZE32
#define SIZE32( x )         ((uint32_t)( sizeof(x) / sizeof(uint32_t) ))
#endif // SIZE32

#define _NAME_MERGE_(x, y)                      x ## y
#define _NAME_LABEL_(name, id)                  _NAME_MERGE_(name, id)
#define __CODEGEN_UNIQUE(name)                  _NAME_LABEL_(name, __LINE__)

#define __CODEGEN_MAX(_a, _b) (((_a) > (_b)) ? (_a) : (_b))
#define __CODEGEN_BITFIELD(l, h) (h) - (l) + 1
#define __CODEGEN_OP_LENGTH_BIAS 2
#define __CODEGEN_OP_LENGTH( x ) (uint32_t)((__CODEGEN_MAX(x, __CODEGEN_OP_LENGTH_BIAS)) - __CODEGEN_OP_LENGTH_BIAS)
#define WIDI_APP_ID 0x0f

#ifndef __MHW_PAVP_COUNTER_TYPE_PROTECTOR__
#define __MHW_PAVP_COUNTER_TYPE_PROTECTOR__
// PAVP counter type values understood by the hardware
typedef enum MHW_PAVP_COUNTER_TYPE_REC {
    MHW_PAVP_COUNTER_TYPE_C = 0x02   // 64-bit counter, 64-bit nonce
} MHW_PAVP_COUNTER_TYPE;
#endif

enum GFX_MEDIA_SUBOPCODE_B_CRYPTO_OPCODE
{
    MEDIASUBOP_B_MFX_CRYPTO_COPY_BASE_ADDR     = 0x0,
    MEDIASUBOP_B_MFX_CRYPTO_KEY_EXCHANGE_G5    = 0x1,
    MEDIASUBOP_B_MFX_CRYPTO_COPY               = 0x8,
    MEDIASUBOP_B_MFX_CRYPTO_KEY_EXCHANGE       = 0x9,
    MEDIASUBOP_B_MFX_CRYPTO_INLINE_STATUS_READ = 0xa
};

enum CRYPTO_INLINE_STATUS_READ_TYPE
{
    CRYPTO_INLINE_MEMORY_STATUS_READ           = 1,
    CRYPTO_INLINE_GET_FRESHNESS_READ           = 2,
    CRYPTO_INLINE_CONNECTION_STATE_READ        = 3,
    CRYPTO_INLINE_GET_WIDI_COUNTER_READ        = 4
};

enum CRYPTO_KEY_EXCHANGE_KEY_USE
{
    CRYPTO_TERMINATE_KEY                       = 0,
    CRYPTO_DECRYPT_AND_USE_NEW_KEY             = 2,
    CRYPTO_DECRYPT_AND_USE_NEW_FIRST_KEY       = 3,
    CRYPTO_USE_NEW_FRESHNESS_VALUE             = 4,
    CRYPTO_ENCRYPTION_OFF                      = 5
};

enum CRYPTO_KEY_EXCHANGE_DECRYPT_ENCRYPT
{
    CRYPTO_DECRYPT = 0,
    CRYPTO_ENCRYPT = 1
};

enum GFX_MEDIA_SUBOPCODE_A
{
    MEDIASUBOP_A_MFX_COMMON = 0x0,
};

enum GFX_MEDIA_OPCODE_MFX
{
    MEDIAOP_MEDIA_COMMON_MFX        = 0,
    MEDIAOP_MEDIA_CRYPTO_MFX        = 6
};

enum GFX_MEDIA_SUBOPCODE_A_HCP
{
    MEDIASUBOP_MEDIA_HCP_AES_STATE                  = 0x06,
    MEDIASUBOP_MEDIA_HCP_AES_IND_STATE              = 0x07
};

enum GFX_MEDIA_SUBOPCODE_A_AVP
{
    MEDIASUBOP_MEDIA_AVP_AES_STATE                  = 0x06,
    MEDIASUBOP_MEDIA_AVP_AES_IND_STATE              = 0x07
};

enum GFX_MEDIA_SUBOPCODE_A_VVCP
{
    MEDIASUBOP_MEDIA_VVCP_AES_STATE                  = 0x06,
    MEDIASUBOP_MEDIA_VVCP_AES_IND_STATE              = 0x07
};

enum GFX_OPCODE_CP
{
    GFXOP_AVP           = 0x3,      // AV1 Codec pipeline
    GFXOP_HCP           = 0x7,      // HEVC Codec Pipeline
    GFXOP_HUC           = 0xB,
    GFXOP_VVCP          = 0xF       // VVC Codec Pipeline
};

enum GFX_MEDIA_SUBOPCODE_B_COMM_COMM
{
    MEDIASUBOP_B_MFX_AES_STATE = 0x5
};

enum MI_OPCODE
{
    MI_SET_APP_ID = 0x0E
};

enum MEDIASTATE_CACHEABILITY_CONTROL
{
    MEDIASTATE_USE_GTT_SETTINGS         = 0,
};

enum GFX_MEDIA_SUBOPCODE_A_HUC
{
    MEDIASUBOP_MEDIA_HUC_AES_STATE                  = 0x06,
    MEDIASUBOP_MEDIA_HUC_AES_IND_STATE              = 0x07,
};

//-------below are command parameters to indicate protected bits in other commponents' commands, For MODS, any pavp field in open-sourced commands will be removed, so pavp need to maintain those bits internally.----------//

typedef struct _MFX_WAIT_PAVP
{
    // DW0
    uint32_t                                                                  :  __CODEGEN_BITFIELD( 0,  8)    ; //!< Reserved
    uint32_t                 PavpSyncControlFlag                              :  __CODEGEN_BITFIELD( 9,  9)    ; //!< Pavp Sync Control Flag
    uint32_t                                                                  :  __CODEGEN_BITFIELD(10, 31)    ; //!< Reserved
}MFX_WAIT_PAVP, *PMFX_WAIT_PAVP;

typedef struct _MI_FLUSH_DW_PAVP
{
    // DDW0
    uint32_t                                                                  : __CODEGEN_BITFIELD( 0,  5)    ; //!< Reserved
    uint32_t                 ContentProtectionAppId                           : __CODEGEN_BITFIELD( 6,  6)    ; //!< Content Protection AppId
    uint32_t                                                                  : __CODEGEN_BITFIELD( 7, 21)    ; //!< Reserved
    uint32_t                 ProtectedMemoryEnable                            : __CODEGEN_BITFIELD(22, 22)    ; //!< Protected memory Enable
    uint32_t                                                                  : __CODEGEN_BITFIELD(23, 31)    ; //!< Reserved
}MI_FLUSH_DW_PAVP, *PMI_FLUSH_DW_PAVP;

typedef struct _MFX_PIPE_MODE_SELECT_PAVP
{
    // DW0
    uint32_t                                                                  : __CODEGEN_BITFIELD( 0, 23)    ; //!< Reserved
    uint32_t                 DECEEnable                                       : __CODEGEN_BITFIELD(24, 24)    ; //!< DECE Enable
    uint32_t                                                                  : __CODEGEN_BITFIELD(25, 26)    ; //!< Reserved
    uint32_t                 AesCtrFlavor                                     : __CODEGEN_BITFIELD(27, 28)    ; //!< Aes Counter Flavor
    uint32_t                 AesOperationSelectionForTheIndirectSourceData    : __CODEGEN_BITFIELD(29, 29)    ; //!< Aes Operation Selection For The Indirect Source Data
    uint32_t                                                                  : __CODEGEN_BITFIELD(30, 30)    ; //!< Reserved
    uint32_t                 AesEnable                                        : __CODEGEN_BITFIELD(31, 31)    ; //!< Aes Enable
}MFX_PIPE_MODE_SELECT_PAVP, *PMFX_PIPE_MODE_SELECT_PAVP;

typedef struct _PIPE_CONTROL_PAVP
{
    /// DW1
    uint32_t                                                                  : __CODEGEN_BITFIELD( 0,  5)    ; ///< Reserved
    uint32_t                 ProtectedMemoryApplicationId                     : __CODEGEN_BITFIELD( 6,  6)    ; ///< Protected Memory Application Id
    uint32_t                                                                  : __CODEGEN_BITFIELD( 7, 21)    ; ///< Reserved
    uint32_t                 ProtectedMemoryEnable                            : __CODEGEN_BITFIELD(22, 22)    ; ///< Protected Memory Enable
    uint32_t                                                                  : __CODEGEN_BITFIELD(23, 26)    ; ///< Reserved
    uint32_t                 ProtectedMemoryDisable                           : __CODEGEN_BITFIELD(27, 27)    ; ///< Protected Memory Disable
    uint32_t                                                                  : __CODEGEN_BITFIELD(28, 31)    ; ///< Reserved
}PIPE_CONTROL_PAVP, *PPIPE_CONTROL_PAVP;

//-------above are command parameters to indicate protected bits in other commponents' commands, For MODS, any pavp field in open-sourced commands will be removed, so pavp need to maintain those bits internally.----------//


typedef struct _MFX_CRYPTO_COPY_BASE_ADDR_CMD_G8
{
    // DW 0
    union
    {
        struct
        {
            uint32_t DWordLength                             : 16 ;    // OP_LENGTH
            uint32_t CommandSubOpcodeB                       : 5  ;    // SUBOPCODE_B
            uint32_t CommandSubOpcodeA                       : 3  ;    // SUBOPCODE_A
            uint32_t CommandOpcode                           : 3  ;    // OPCODE
            uint32_t CommandPipeLine                         : 2  ;    // INSTRUCTION_PIPELINE
            uint32_t CommandType                             : 3  ;    // INSTRUCTION_TYPE
        };
        struct
        {
            uint32_t Value;
        };
    } DW0;

    // DW 1
    union
    {
        struct
        {
            uint32_t                                         : 12 ;
            uint32_t SrcAddr                                 : 20 ;
        };
        struct
        {
            uint32_t Value;
        };
    } DW1;

    // DW 2
    union
    {
        struct
        {
            uint32_t SrcAddr                                 : 16 ;
            uint32_t                                         : 16 ;
        };
        struct
        {
            uint32_t Value;
        };
    } DW2;

    // DW 3
    union
    {
        struct
        {
            uint32_t                                         : 12 ;
            uint32_t SrcAddrUpperBound                       : 20 ;
        };
        struct
        {
            uint32_t Value;
        };
    } DW3;

    // DW 4
    union
    {
        struct
        {
            uint32_t SrcAddrUpperBound                       : 16 ;
            uint32_t                                         : 16 ;
        };
        struct
        {
            uint32_t Value;
        };
    } DW4;

    // DW 5
    union
    {
        struct
        {
            uint32_t                                         : 12 ;
            uint32_t DstAddr                                 : 20 ;
        };
        struct
        {
            uint32_t Value;
        };
    } DW5;

    // DW 6
    union
    {
        struct
        {
            uint32_t DstAddr                                 : 16 ;
            uint32_t                                         : 16 ;
        };
        struct
        {
            uint32_t Value;
        };
    } DW6;
} MFX_CRYPTO_COPY_BASE_ADDR_CMD_G8, *PMFX_CRYPTO_COPY_BASE_ADDR_CMD_G8;

typedef struct _MFX_CRYPTO_COPY_CMD_G8
{
    union
    {
        struct
        {
            uint32_t DWordLength                             : 16 ;    // OP_LENGTH
            uint32_t CommandSubOpcodeB                       : 5  ;    // SUBOPCODE_B
            uint32_t CommandSubOpcodeA                       : 3  ;    // SUBOPCODE_A
            uint32_t CommandOpcode                           : 3  ;    // OPCODE
            uint32_t CommandPipeLine                         : 2  ;    // INSTRUCTION_PIPELINE
            uint32_t CommandType                             : 3  ;    // INSTRUCTION_TYPE
        };
        struct
        {
            uint32_t Value;
        };
    } DW0;

    // DW 1
    union
    {
        struct
        {
            uint32_t IndirectDataLength                      : 23 ;    // Indirect Data Length
            uint32_t                                         : 1  ;    // Reserved MBZ
            uint32_t UseIntialCounterValue                   : 1  ;    // AES counter intialization value from previous pass
            uint32_t AesCounterFlavor                        : 2  ;    // AES counter flavor for HDCP2
            uint32_t                                         : 1  ;    // Reserved MBZ
            uint32_t SelectionForIndirectData                : 4  ;    // Encryption/Authentication selection for the indirect source data
        };
        struct
        {
            uint32_t Value;
        };
    } DW1;

    // DW 2
    union
    {
        struct
        {
            uint32_t SrcBaseOffset                           : 32 ;
        };
        struct
        {
            uint32_t Value;
        };
    } DW2;

    // DW 3
    union
    {
        struct
        {
            uint32_t SrcBaseOffset                           : 16 ;    // In 16 bytes
            uint32_t                                         : 16 ;
        };
        struct
        {
            uint32_t Value;
        };
    } DW3;

    // DW 4
    union
    {
        struct
        {
            uint32_t DstBaseOffset                           : 32 ;
        };
        struct
        {
            uint32_t Value;
        };
    } DW4;

    // DW 5
    union
    {
        struct
        {
            uint32_t DstBaseOffset                           : 16 ;
            uint32_t                                         : 16 ;
        };
        struct
        {
            uint32_t Value;
        };
    } DW5;

    // DW 6
    union
    {
        struct
        {
            uint32_t Counter                                 : 32 ;    // Initial counter value for AES-128 counter mode decryption
        };
        struct
        {
            uint32_t Value;
        };
    } DW6;

    // DW 7
    union
    {
        struct
        {
            uint32_t InitializationValueDw1                  : 32 ;
        };
        struct
        {
            uint32_t Value;
        };
    } DW7;

    // DW 8
    union
    {
        struct
        {
            uint32_t InitializationValueDw2                  : 32 ;
        };
        struct
        {
            uint32_t Value;
        };
    } DW8;

    // DW 9
    union
    {
        struct
        {
            uint32_t InitializationValueDw3                  : 32 ;
        };
        struct
        {
            uint32_t Value;
        };
    } DW9;

    // DW 10
    union
    {
        struct
        {
            uint32_t InitialPacketLength                     : 16 ;    // Initial packet Length
            uint32_t                                         : 12 ;    // Reserved MBZ
            uint32_t AesOperationType                        : 2  ;    // AES operations input or output
            uint32_t                                         : 1  ;    // Reserved MBZ
            uint32_t StartPacketType                         : 1  ;    // Initial packet type clear or encrypted
        };
        struct
        {
            uint32_t Value;
        };
    } DW10;

    // DW 11
    union
    {
        struct
        {
            uint32_t EncryptedBytesLength                    : 23 ;    // Length of encrypted packet in bytes
            uint32_t                                         : 9  ;    // Reserved MBZ
        };
        struct
        {
            uint32_t Value;
        };
    } DW11;

    // DW 12
    union
    {
        struct
        {
            uint32_t ClearBytesLength                        : 23 ;    // Length of clear packet in bytes
            uint32_t                                         : 9  ;    // Reserved MBZ
        };
        struct
        {
            uint32_t Value;
        };
    } DW12;
} MFX_CRYPTO_COPY_CMD_G8, *PMFX_CRYPTO_COPY_CMD_G8;

typedef struct _MFX_CRYPTO_KEY_EXCHANGE_CMD
{
    union
    {
        struct
        {
            uint32_t DWordLength                             : 16 ;    // OP_LENGTH
            uint32_t CommandSubOpcodeB                       : 5  ;    // SUBOPCODE_B
            uint32_t CommandSubOpcodeA                       : 3  ;    // SUBOPCODE_A
            uint32_t CommandOpcode                           : 3  ;    // OPCODE
            uint32_t CommandPipeLine                         : 2  ;    // INSTRUCTION_PIPELINE
            uint32_t CommandType                             : 3  ;    // INSTRUCTION_TYPE
        };
        struct
        {
            uint32_t Value;
        };
    } DW0;

    // DW 1
    union
    {
        struct
        {
            uint32_t DecryptionOrEncryption                  : 1  ;    // For decrypt and use new key or freshness
            uint32_t                                         : 28 ;    // Reserved MBZ
            uint32_t KeyUseIndicator                         : 3  ;    // Function of Command
        };
        struct
        {
            uint32_t Value;
        };
    } DW1;

} MFX_CRYPTO_KEY_EXCHANGE_CMD, *PMFX_CRYPTO_KEY_EXCHANGE_CMD;

typedef struct _MFX_CRYPTO_INLINE_STATUS_READ_CMD_G8
{
    union
    {
        struct
        {
            uint32_t DWordLength                             : 16 ;    // OP_LENGTH
            uint32_t CommandSubOpcodeB                       : 5  ;    // SUBOPCODE_B
            uint32_t CommandSubOpcodeA                       : 3  ;    // SUBOPCODE_A
            uint32_t CommandOpcode                           : 3  ;    // OPCODE
            uint32_t CommandPipeLine : 2;    // INSTRUCTION_PIPELINE
            uint32_t CommandType : 3;    // INSTRUCTION_TYPE
        };
        struct
        {
            uint32_t Value;
        };
    } DW0;

    // DW 1
    union
    {
        struct
        {
            uint32_t InlineReadCmdType                       : 3  ;
            uint32_t                                         : 13 ;
            uint32_t KeyRefreshRandomValueType               : 1  ;
            uint32_t                                         : 15 ;
        };
        struct
        {
            uint32_t Value;
        };
    } DW1;

    // DW 2
    union
    {
        struct
        {
            uint32_t ReadDataStoreAddress                    : 32 ;
        };
        struct
        {
            uint32_t Value;
        };
    } DW2;

    // DW 3
    union
    {
        struct
        {
            uint32_t ReadDataStoreAddress                    : 16 ;
            uint32_t : 16;
        };
        struct
        {
            uint32_t Value;
        };
    } DW3;

    // DW 4
    union
    {
        struct
        {
            uint32_t Nonce                                   : 32 ;
        };
        struct
        {
            uint32_t Value;
        };
    } DW4;

    // DW 5
    union
    {
        struct
        {
            uint32_t                                         : 1  ;
            uint32_t EnableForNotifyInterrupt                : 1  ;
            uint32_t DestAddressType                         : 1  ;
            uint32_t GfxAddress                              : 29 ;
        };
        struct
        {
            uint32_t Value;
        };
    } DW5;

    // DW 6
    union
    {
        struct
        {
            uint32_t GfxAddress                              : 16 ;
            uint32_t                                         : 16 ;
        };
        struct
        {
            uint32_t Value;
        };
    } DW6;

    // DW 7
    union
    {
        struct
        {
            uint32_t ImmediateWriteData0                     : 32 ;
        };
        struct
        {
            uint32_t Value;
        };
    } DW7;

    // DW 8
    union
    {
        struct
        {
            uint32_t ImmediateWriteData1                     : 32 ;
        };
        struct
        {
            uint32_t Value;
        };
    } DW8;
} MFX_CRYPTO_INLINE_STATUS_READ_CMD_G8, *PMFX_CRYPTO_INLINE_STATUS_READ_CMD_G8;

typedef struct _MI_SET_APP_ID_CMD
{
    // DW 0
    union
    {
        struct
        {
            uint32_t ApplicationId                           : 7  ;
            uint32_t ApplicationType                         : 1  ;
            uint32_t SaveInhibit                             : 1  ;
            uint32_t RestoreInhibit                          : 1  ;
            uint32_t                                         : 13 ;
            uint32_t InstructionOpcode                        : 6  ;    // MI_OPCODE
            uint32_t InstructionType                         : 3  ;    // INSTRUCTION_TYPE
        };
        struct
        {
            uint32_t Value;
        };
    } DW0;
} MI_SET_APP_ID_CMD, *PMI_SET_APP_ID_CMD;

typedef struct _HCP_AES_STATE_CMD
{
    // DW 0
    union
    {
        struct
        {
            uint32_t DwordLength                             : BITFIELD_RANGE(0 , 11);    // U12
            uint32_t                                         : BITFIELD_RANGE(12, 15);
            uint32_t MediaInstructionCommand                 : BITFIELD_RANGE(16, 22);    // OpCode
            uint32_t MediaInstructionOpcode                  : BITFIELD_RANGE(23, 26);    // OpCode
            uint32_t PipelineType                            : BITFIELD_RANGE(27, 28);    // OpCode
            uint32_t CommandType                             : BITFIELD_RANGE(29, 31);    // OpCode
        };

        uint32_t Value;
    } DW0;

    // DW 1
    union
    {
        struct
        {
            uint32_t InitializationValue0                    : BITFIELD_RANGE(0 , 31);
        };

        uint32_t Value;
    } DW1;

    // DW 2
    union
    {
        struct
        {
            uint32_t InitializationValue1                    : BITFIELD_RANGE(0 , 31);
        };

        uint32_t Value;
    } DW2;

    // DW 3
    union
    {
        struct
        {
            uint32_t InitializationValue2                    : BITFIELD_RANGE(0 , 31);
        };

        uint32_t Value;
    } DW3;

    // DW 4
    union
    {
        struct
        {
            uint32_t InitializationValue3                    : BITFIELD_RANGE(0 , 31);
        };

        uint32_t Value;
    } DW4;

    // DW 5
    union
    {
        struct
        {
        uint32_t                                             : BITFIELD_RANGE(0 , 23);
        uint32_t MonotonicEncryptionCounterCheckEnable       : BITFIELD_BIT(24);    // U1
        uint32_t                                             : BITFIELD_RANGE(25, 31);
        };

        uint32_t Value;
    } DW5;

    // DW 6
    union
    {
        struct
        {
            uint32_t Reserved                                : BITFIELD_RANGE(0 , 31);
        };

        uint32_t Value;
    } DW6;

    // DW 7
    union
    {
        struct
        {
            uint32_t InitPktBytesLengthInBytes               : BITFIELD_RANGE(0 , 15);    // U16
            uint32_t                                         : BITFIELD_RANGE(16, 29);    // 
            uint32_t ProgrammedInitialCounterValue           : BITFIELD_BIT(30);    // Enable
            uint32_t InitialFairPlayOrDeceStartPacketType    : BITFIELD_BIT(31);    // Enable
        };

        uint32_t Value;
    } DW7;

    // DW 8
    union
    {
        struct
        {
            uint32_t EncryptedBytesLengthInBytes                : BITFIELD_RANGE(0 , 22);    // U23
            uint32_t                                            : BITFIELD_RANGE(23, 30);    // 
            uint32_t DetectAndHandlePartialBlockAtTheEndOfSlice : BITFIELD_BIT(31);    // Enable
        };

        uint32_t Value;
    } DW8;

    // uint32_t 9
    union
    {
        struct
        {
            uint32_t ClearBytesLengthInBytes                 : BITFIELD_RANGE(0 , 22);    // U23
        uint32_t                                             : BITFIELD_RANGE(23, 31);
        };

        uint32_t Value;
    } DW9;
} HCP_AES_STATE_CMD, *PHCP_AES_STATE_CMD;
C_ASSERT(SIZE32(HCP_AES_STATE_CMD) == 10);

typedef struct _AVP_AES_STATE_CMD
{
    // DW 0
    union
    {
        struct
        {
            uint32_t DwordLength                             : BITFIELD_RANGE(0 , 11);    // U12
            uint32_t                                         : BITFIELD_RANGE(12, 15);
            uint32_t MediaInstructionCommand                 : BITFIELD_RANGE(16, 22);    // OpCode
            uint32_t MediaInstructionOpcode                  : BITFIELD_RANGE(23, 26);    // OpCode
            uint32_t PipelineType                            : BITFIELD_RANGE(27, 28);    // OpCode
            uint32_t CommandType                             : BITFIELD_RANGE(29, 31);    // OpCode
        };

        uint32_t Value;
    } DW0;

    // DW 1
    union
    {
        struct
        {
            uint32_t InitializationValue0                    : BITFIELD_RANGE(0 , 31);
        };

        uint32_t Value;
    } DW1;

    // DW 2
    union
    {
        struct
        {
            uint32_t InitializationValue1                    : BITFIELD_RANGE(0 , 31);
        };

        uint32_t Value;
    } DW2;

    // DW 3
    union
    {
        struct
        {
            uint32_t InitializationValue2                    : BITFIELD_RANGE(0 , 31);
        };

        uint32_t Value;
    } DW3;

    // DW 4
    union
    {
        struct
        {
            uint32_t InitializationValue3                    : BITFIELD_RANGE(0 , 31);
        };

        uint32_t Value;
    } DW4;

    // DW 5
    union
    {
        struct
        {
        uint32_t                                             : BITFIELD_RANGE(0 , 23);
        uint32_t MonotonicEncryptionCounterCheckEnable       : BITFIELD_BIT(24);    // U1
        uint32_t                                             : BITFIELD_RANGE(25, 31);
        };

        uint32_t Value;
    } DW5;

    // DW 6
    union
    {
        struct
        {
            uint32_t Reserved                                : BITFIELD_RANGE(0 , 31);
        };

        uint32_t Value;
    } DW6;

    // DW 7
    union
    {
        struct
        {
            uint32_t InitPktBytesLengthInBytes               : BITFIELD_RANGE(0 , 15);    // U16
            uint32_t                                         : BITFIELD_RANGE(16, 29);    // 
            uint32_t ProgrammedInitialCounterValue           : BITFIELD_BIT(30);    // Enable
            uint32_t InitialFairPlayOrDeceStartPacketType    : BITFIELD_BIT(31);    // Enable
        };

        uint32_t Value;
    } DW7;

    // DW 8
    union
    {
        struct
        {
            uint32_t EncryptedBytesLengthInBytes                : BITFIELD_RANGE(0 , 22);    // U23
            uint32_t                                            : BITFIELD_RANGE(23, 30);    // 
            uint32_t DetectAndHandlePartialBlockAtTheEndOfSlice : BITFIELD_BIT(31);    // Enable
        };

        uint32_t Value;
    } DW8;

    // uint32_t 9
    union
    {
        struct
        {
            uint32_t ClearBytesLengthInBytes                 : BITFIELD_RANGE(0 , 22);    // U23
        uint32_t                                             : BITFIELD_RANGE(23, 31);
        };

        uint32_t Value;
    } DW9;
} AVP_AES_STATE_CMD, *PAVP_AES_STATE_CMD;
C_ASSERT(SIZE32(AVP_AES_STATE_CMD) == 10);

typedef struct _VVCP_AES_STATE_CMD
{
    // DW 0
    union
    {
        struct
        {
            uint32_t DwordLength                             : BITFIELD_RANGE(0 , 11);    // U12
            uint32_t                                         : BITFIELD_RANGE(12, 15);
            uint32_t MediaInstructionCommand                 : BITFIELD_RANGE(16, 21);    // OpCode
            uint32_t MediaInstructionOpcode                  : BITFIELD_RANGE(22, 26);    // OpCode
            uint32_t PipelineType                            : BITFIELD_RANGE(27, 28);    // OpCode
            uint32_t CommandType                             : BITFIELD_RANGE(29, 31);    // OpCode
        };

        uint32_t Value;
    } DW0;

    // DW 1
    union
    {
        struct
        {
            uint32_t InitializationValue0                    : BITFIELD_RANGE(0 , 31);
        };

        uint32_t Value;
    } DW1;

    // DW 2
    union
    {
        struct
        {
            uint32_t InitializationValue1                    : BITFIELD_RANGE(0 , 31);
        };

        uint32_t Value;
    } DW2;

    // DW 3
    union
    {
        struct
        {
            uint32_t InitializationValue2                    : BITFIELD_RANGE(0 , 31);
        };

        uint32_t Value;
    } DW3;

    // DW 4
    union
    {
        struct
        {
            uint32_t InitializationValue3                    : BITFIELD_RANGE(0 , 31);
        };

        uint32_t Value;
    } DW4;

    // DW 5
    union
    {
        struct
        {
        uint32_t                                             : BITFIELD_RANGE(0 , 23);
        uint32_t MonotonicEncryptionCounterCheckEnable       : BITFIELD_BIT(24);    // U1
        uint32_t                                             : BITFIELD_RANGE(25, 31);
        };

        uint32_t Value;
    } DW5;

    // DW 6
    union
    {
        struct
        {
            uint32_t Reserved                                : BITFIELD_RANGE(0 , 31);
        };

        uint32_t Value;
    } DW6;

    // DW 7
    union
    {
        struct
        {
            uint32_t InitPktBytesLengthInBytes               : BITFIELD_RANGE(0 , 15);    // U16
            uint32_t                                         : BITFIELD_RANGE(16, 29);    // 
            uint32_t ProgrammedInitialCounterValue           : BITFIELD_BIT(30);    // Enable
            uint32_t InitialFairPlayOrDeceStartPacketType    : BITFIELD_BIT(31);    // Enable
        };

        uint32_t Value;
    } DW7;

    // DW 8
    union
    {
        struct
        {
            uint32_t EncryptedBytesLengthInBytes                : BITFIELD_RANGE(0 , 22);    // U23
            uint32_t                                            : BITFIELD_RANGE(23, 30);    // 
            uint32_t DetectAndHandlePartialBlockAtTheEndOfSlice : BITFIELD_BIT(31);    // Enable
        };

        uint32_t Value;
    } DW8;

    // uint32_t 9
    union
    {
        struct
        {
            uint32_t ClearBytesLengthInBytes                 : BITFIELD_RANGE(0 , 22);    // U23
        uint32_t                                             : BITFIELD_RANGE(23, 31);
        };

        uint32_t Value;
    } DW9;
} VVCP_AES_STATE_CMD, *PVVCP_AES_STATE_CMD;
C_ASSERT(SIZE32(VVCP_AES_STATE_CMD) == 10);

typedef struct _MFX_AES_STATE_CMD_G8
{
    // DW 0
    union
    {
        struct
        {
            uint32_t DWordLength                             : 16 ;    // OP_LENGTH
            uint32_t CommandSubOpcodeB                       : 5  ;    // SUBOPCODE_B
            uint32_t CommandSubOpcodeA                       : 3  ;    // SUBOPCODE_A
            uint32_t CommandOpcode                           : 3  ;    // OPCODE
            uint32_t CommandPipeLine                         : 2  ;    // INSTRUCTION_PIPELINE
            uint32_t CommandType                             : 3  ;    // INSTRUCTION_TYPE
        };
        struct
        {
            uint32_t Value;
        };
    } DW0;
    // DW 1
    union
    {
        struct
        {
            uint32_t InitializationValue0;
        };
        struct
        {
            uint32_t Value;
        };
    } DW1;
    // DW 2
    union
    {
        struct
        {
            uint32_t InitializationValue1;
        };
        struct
        {
            uint32_t Value;
        };
    } DW2;
    // DW 3
    union
    {
        struct
        {
            uint32_t InitializationValue2;
        };
        struct
        {
            uint32_t Value;
        };
    } DW3;
    // DW 4
    union
    {
        struct
        {
            uint32_t InitializationValue3;
        };
        struct
        {
            uint32_t Value;
        };
    } DW4;
    // DW 5
    union
    {
        struct
        {
            uint32_t TotalPacketCount                        : 19 ;
            uint32_t                                         : 3  ;
            uint32_t SurfaceCacheabilityControl              : 2  ;
            uint32_t SurfaceGfdtReferencePicture             : 1  ;
            uint32_t EncryptedData                           : 1  ;
            uint32_t ArbitrationPriority                     : 2  ;
            uint32_t                                         : 1  ;
            uint32_t BitPosition                             : 3  ;
        };
        struct
        {
            uint32_t Value;
        };
    } DW5;
    // DW 6
    union
    {
        struct
        {
            uint32_t AACSBitVectorSurfaceStartAddress        : 32 ;
        };
        struct
        {
            uint32_t Value;
        };
    } DW6;
    // DW 7
    union
    {
        struct
        {
            uint32_t InitPacketBytesLength                   : 16 ;
            uint32_t                                         : 14 ;
            uint32_t IntialCounterValueUse                   : 1  ;
            uint32_t StartPacketType                         : 1  ;
        };
        struct
        {
            uint32_t Value;
        };
    } DW7;
    // DW 8
    union
    {
        struct
        {
            uint32_t EncryptedBytesLength                    : 23 ;
            uint32_t                                         : 9  ;
        };
        struct
        {
            uint32_t Value;
        };
    } DW8;
    // DW 9
    union
    {
        struct
        {
            uint32_t ClearBytesLength                        : 23 ;
            uint32_t                                         : 9  ;
        };
        struct
        {
            uint32_t Value;
        };
    } DW9;
    // DW 10
    union
    {
        struct
        {
            uint32_t AACSBitVectorSurfaceStartAddr           : 16 ;
            uint32_t                                         : 16 ;
        };
        struct
        {
            uint32_t Value;
        };
    } DW10;
    // DW 11
    union
    {
        struct
        {
            uint32_t Value;
        };
    } DW11;
} MFX_AES_STATE_CMD_G8, *PMFX_AES_STATE_CMD_G8;

typedef struct _MFX_AES_STATE_CMD_G9
{
    // DW 0
    union
    {
        struct
        {
            uint32_t DwordLength                             : BITFIELD_RANGE(0 , 11);   // OP_LENGTH
            uint32_t __CODEGEN_UNIQUE(Reserved)              : BITFIELD_RANGE(12, 15);   // RESERVED
            uint32_t SubOpcodeB                              : BITFIELD_RANGE(16, 20);   // SUBOPCODE_B
            uint32_t SubOpcodeA                              : BITFIELD_RANGE(21, 23);   // SUBOPCODE_A
            uint32_t MediaCommandOpcode                      : BITFIELD_RANGE(24, 26);   // OPCODE
            uint32_t Pipeline                                : BITFIELD_RANGE(27, 28);   // INSTRUCTION_PIPELINE
            uint32_t CommandType                             : BITFIELD_RANGE(29, 31);   // INSTRUCTION_TYPE
        };
        struct
        {
            uint32_t Value;
        };
    } DW0;

    // DW 1
    union
    {
        struct
        {
            uint32_t InitializationValue0                    : BITFIELD_RANGE(0 , 31);
        };
        struct
        {
            uint32_t Value;
        };
    } DW1;

    // DW 2
    union
    {
        struct
        {
            uint32_t InitializationValue1                    : BITFIELD_RANGE(0 , 31);
        };
        struct
        {
            uint32_t Value;
        };
    } DW2;

    // DW 3
    union
    {
        struct
        {
            uint32_t InitializationValue2                    : BITFIELD_RANGE(0 , 31);
        };
        struct
        {
            uint32_t Value;
        };
    } DW3;

    // DW 4
    union
    {
        struct
        {
            uint32_t InitializationValue3                    : BITFIELD_RANGE(0 , 31);
        };
        struct
        {
            uint32_t Value;
        };
    } DW4;

    // DW 5
    union
    {
        struct
        {
            uint32_t __CODEGEN_UNIQUE(Reserved)              : BITFIELD_RANGE(0 , 23);
            uint32_t MVcounterEncryption                     : BITFIELD_BIT(24);
            uint32_t __CODEGEN_UNIQUE(Reserved)              : BITFIELD_RANGE(25, 31);
        };
        struct
        {
            uint32_t Value;
        };
    } DW5;

    // DW 6
    union
    {
        struct
        {
            uint32_t __CODEGEN_UNIQUE(Reserved)              : BITFIELD_RANGE(0 , 31);
        };
        struct
        {
            uint32_t Value;
        };
    } DW6;

    // DW 7
    union
    {
        struct
        {
            uint32_t InitPacketBytesLength                   : BITFIELD_RANGE(0 , 15);
            uint32_t __CODEGEN_UNIQUE(Reserved)              : BITFIELD_RANGE(16, 29);
            uint32_t IntialCounterValueUse                   : BITFIELD_BIT(30);
            uint32_t StartPacketType                         : BITFIELD_BIT(31);
        };
        struct
        {
            uint32_t Value;
        };
    } DW7;

    // DW 8
    union
    {
        struct
        {
            uint32_t EncryptedBytesLength                    : BITFIELD_RANGE(0 , 22);
            uint32_t __CODEGEN_UNIQUE(Reserved)              : BITFIELD_RANGE(23, 30);
            uint32_t LastPartialBlockAdj                     : BITFIELD_BIT(31);
        };
        struct
        {
            uint32_t Value;
        };
    } DW8;

    // DW 9
    union
    {
        struct
        {
            uint32_t NumberByteInputBitstream                : BITFIELD_RANGE(0 , 22);
            uint32_t __CODEGEN_UNIQUE(Reserved)              : BITFIELD_RANGE(23, 31);
        };
        struct
        {
            uint32_t Value;
        };
    } DW9;

} MFX_AES_STATE_CMD_G9, *PMFX_AES_STATE_CMD_G9;

C_ASSERT(SIZE32(_MFX_AES_STATE_CMD_G9) == 10);

typedef struct _HUC_AES_STATE_CMD
{
    // DW 0
    union
    {
        struct
        {
            uint32_t DwordLength                             : BITFIELD_RANGE(0 , 11);   // U12
            uint32_t                                         : BITFIELD_RANGE(12, 15);   // 
            uint32_t MediaInstructionCommand                 : BITFIELD_RANGE(16, 22);   // OpCode
            uint32_t MediaInstructionOpcode                  : BITFIELD_RANGE(23, 26);   // OpCode
            uint32_t PipelineType                            : BITFIELD_RANGE(27, 28);   // OpCode
            uint32_t CommandType                             : BITFIELD_RANGE(29, 31);   // OpCode
        };

        uint32_t Value;
    } DW0;

    // DW 1
    union
    {
        struct
        {
            uint32_t InitializationValue0                    : BITFIELD_RANGE(0 , 31);
        };

        uint32_t Value;
    } DW1;

    // DW 2
    union
    {
        struct
        {
            uint32_t InitializationValue1                    : BITFIELD_RANGE(0 , 31);
        };

        uint32_t Value;
    } DW2;

    // DW 3
    union
    {
        struct
        {
            uint32_t InitializationValue2                    : BITFIELD_RANGE(0 , 31);
        };

        uint32_t Value;
    } DW3;

    // DW 4
    union
    {
        struct
        {
            uint32_t InitializationValue3                    : BITFIELD_RANGE(0 , 31);
        };

        uint32_t Value;
    } DW4;

    // DW 5
    union
    {
        struct
        {
        uint32_t                                             : BITFIELD_RANGE(0 , 23);
        uint32_t    MonotonicEncryptionCounterCheckEnable    : BITFIELD_BIT(24);   // U1
        uint32_t                                             : BITFIELD_RANGE(25, 31);
        };

        uint32_t Value;
    } DW5;

    // DW 6
    union
    {
        struct
        {
            uint32_t Reserved                                : BITFIELD_RANGE(0 , 31);
        };

        uint32_t Value;
    } DW6;

    // DW 7
    union
    {
        struct
        {
            uint32_t InitPktBytesLengthInBytes               : BITFIELD_RANGE(0 , 15);   // U16
            uint32_t                                         : BITFIELD_RANGE(16, 29);
            uint32_t ProgrammedInitialCounterValue           : BITFIELD_BIT(30);   // Enable
            uint32_t InitialFairPlayOrDeceStartPacketType    : BITFIELD_BIT(31);   // Enable
        };

        uint32_t Value;
    } DW7;

    // DW 8
    union
    {
        struct
        {
            uint32_t EncryptedBytesLengthInBytes                : BITFIELD_RANGE(0 , 22);   // U23
            uint32_t                                            : BITFIELD_RANGE(23, 30);
            uint32_t DetectAndHandlePartialBlockAtTheEndOfSlice : BITFIELD_BIT(31);   // Enable
        };

        uint32_t Value;
    } DW8;

    // DW 9
    union
    {
        struct
        {
            uint32_t ClearBytesLengthInBytes                 : BITFIELD_RANGE(0 , 22);   // U23
            uint32_t                                         : BITFIELD_RANGE(23, 31);
        };

        uint32_t Value;
    } DW9;
} HUC_AES_STATE_CMD, *PHUC_AES_STATE_CMD;
C_ASSERT(SIZE32(HUC_AES_STATE_CMD) == 10);

typedef struct _HUC_AES_IND_STATE_CMD
{
    // DW 0
    union
    {
        struct
        {
            uint32_t DwordLength                             : BITFIELD_RANGE(0 , 11);   // U12
        uint32_t                                             : BITFIELD_RANGE(12, 15);
            uint32_t MediaInstructionCommand                 : BITFIELD_RANGE(16, 22);   // OpCode
            uint32_t MediaInstructionOpcode                  : BITFIELD_RANGE(23, 26);   // OpCode
            uint32_t PipelineType                            : BITFIELD_RANGE(27, 28);   // OpCode
            uint32_t CommandType                             : BITFIELD_RANGE(29, 31);   // OpCode
        };

        uint32_t Value;
    } DW0;

    // DW 1_2
    struct
    {
        union
        {
            struct
            {
                uint32_t SliceIvPresent                      : BITFIELD_BIT(0);
                uint32_t                                     : BITFIELD_RANGE(1 , 5);
                uint32_t IndirectStateEntryAddress           : BITFIELD_RANGE(6 , 31);
            };

            uint32_t Value;
        } DW1;

        union
        {
            struct
            {
                uint32_t IndirectStateEntryAddress           : BITFIELD_RANGE(0 , 15);
                uint32_t                                     : BITFIELD_RANGE(16, 31);
            };

            uint32_t Value;
        } DW2;
    } QW1;

    // DW 3
    // Indirect State Entry Address Attributes
    union
    {
        struct
        {
            uint32_t Age                                     : BITFIELD_RANGE(0 , 1 );
            uint32_t EncryptedData                           : BITFIELD_BIT(2);
            uint32_t TargetCache                             : BITFIELD_RANGE(3 , 4 );
            uint32_t MemoryType                              : BITFIELD_RANGE(5 , 6 );
            uint32_t ArbitrationPriorityControl              : BITFIELD_RANGE(7 , 8 );
            uint32_t                                         : BITFIELD_RANGE(9 , 31);
        } Obj1;

        struct
        {
            uint32_t MemoryObjectControlState                : BITFIELD_RANGE(0 , 6 );
            uint32_t ArbitrationPriorityControl              : BITFIELD_RANGE(7 , 8 );
        uint32_t: BITFIELD_RANGE(9, 31);
        };

        uint32_t Value;
    } DW3;

    // DW 4
    union
    {
        struct
        {
            uint32_t NumberOfEntriesMinus1                   : BITFIELD_RANGE(0 , 31);   // U32
        };

        uint32_t Value;
    } DW4;

    // DW 5
    union
    {
        struct
        {
            uint32_t AESCtrOrIVDW0                           : BITFIELD_RANGE(0 , 31);
        };

        uint32_t Value;
    } DW5;

    // DW 6
    union
    {
        struct
        {
            uint32_t AESCtrOrIVDW1                           : BITFIELD_RANGE(0 , 31);
        };

        uint32_t Value;
    } DW6;

    // DW 7
    union
    {
        struct
        {
            uint32_t AESCtrOrIVDW2                           : BITFIELD_RANGE(0 , 31);
        };

        uint32_t Value;
    } DW7;

    // DW 8
    union
    {
        struct
        {
            uint32_t AESCtrOrIVDW3                           : BITFIELD_RANGE(0 , 31);
        };

        uint32_t Value;
    } DW8;
} HUC_AES_IND_STATE_CMD, *PHUC_AES_IND_STATE_CMD;
C_ASSERT(SIZE32(HUC_AES_IND_STATE_CMD) == 9);

typedef struct _HCP_AES_IND_STATE_CMD
{
    // DW 0
    union {
        struct
        {
            uint32_t DwordLength                             : BITFIELD_RANGE(0 , 11);   // U12
            uint32_t                                         : BITFIELD_RANGE(12, 15);
            uint32_t MediaInstructionCommand                 : BITFIELD_RANGE(16, 22);   // OpCode
            uint32_t MediaInstructionOpcode                  : BITFIELD_RANGE(23, 26);   // OpCode
            uint32_t PipelineType                            : BITFIELD_RANGE(27, 28);   // OpCode
            uint32_t CommandType                             : BITFIELD_RANGE(29, 31);   // OpCode
        };

        uint32_t Value;
    } DW0;

    // DW 1_2
    struct
    {
        union
        {
            struct
            {
                uint32_t SliceIvPresent                      : BITFIELD_BIT(0);
                uint32_t                                     : BITFIELD_RANGE(1 , 5 );
                uint32_t IndirectStateEntryAddress           : BITFIELD_RANGE(6 , 31);
            };

            uint32_t Value;
        } DW1;

        union
        {
            struct
            {
                uint32_t IndirectStateEntryAddress           : BITFIELD_RANGE(0 , 15);
                uint32_t: BITFIELD_RANGE(16, 31);
            };

            uint32_t Value;
        } DW2;
    } QW1;

    // DW 3
    // Indirect State Entry Address Attributes
    union
    {
        struct
        {
            uint32_t Age                                     : BITFIELD_RANGE(0 , 1 );
            uint32_t EncryptedData                           : BITFIELD_BIT(2);
            uint32_t TargetCache                             : BITFIELD_RANGE(3 , 4 );
            uint32_t MemoryType                              : BITFIELD_RANGE(5 , 6 );
            uint32_t ArbitrationPriorityControl              : BITFIELD_RANGE(7 , 8 );
            uint32_t                                         : BITFIELD_RANGE(9 , 31);
        } Obj1;

        struct
        {
            uint32_t MemoryObjectControlState                : BITFIELD_RANGE(0 , 6 );
            uint32_t ArbitrationPriorityControl              : BITFIELD_RANGE(7 , 8 );
        uint32_t                                             : BITFIELD_RANGE(9 , 31);
        };

        uint32_t Value;
    } DW3;

    // DW 4
    union
    {
        struct
        {
            uint32_t NumberOfEntriesMinus1                   : BITFIELD_RANGE(0 , 31);    // U32
        };

        uint32_t Value;
    } DW4;

    // DW 5
    union
    {
        struct
        {
            uint32_t AesCtrIv                                : BITFIELD_RANGE(0 , 31);    // U32
        };

        uint32_t Value;
    } DW5;

    // DW 6
    union
    {
        struct
        {
            uint32_t AesCtrIv                                : BITFIELD_RANGE(0 , 31);    // U32
        };

        uint32_t Value;
    } DW6;

    // DW 7
    union
    {
        struct
        {
            uint32_t AesCtrIv                                : BITFIELD_RANGE(0 , 31);    // U32
        };

        uint32_t Value;
    } DW7;

    // DW 8
    union
    {
        struct
        {
            uint32_t AesCtrIv                                : BITFIELD_RANGE(0 , 31);    // U32
        };

        uint32_t Value;
    } DW8;
} HCP_AES_IND_STATE_CMD, *PHCP_AES_IND_STATE_CMD;
C_ASSERT(SIZE32(_HCP_AES_IND_STATE_CMD) == 9);

typedef struct _AVP_AES_IND_STATE_CMD
{
    // DW 0
    union
    {
        struct
        {
            uint32_t DwordLength                             : BITFIELD_RANGE(0 , 11);   // U12
            uint32_t                                         : BITFIELD_RANGE(12, 15);
            uint32_t MediaInstructionCommand                 : BITFIELD_RANGE(16, 22);   // OpCode
            uint32_t MediaInstructionOpcode                  : BITFIELD_RANGE(23, 26);   // OpCode
            uint32_t PipelineType                            : BITFIELD_RANGE(27, 28);   // OpCode
            uint32_t CommandType                             : BITFIELD_RANGE(29, 31);   // OpCode
        };

        uint32_t Value;
    } DW0;

    // DW 1_2
    struct
    {
        union
        {
            struct
            {
                uint32_t SliceIvPresent                      : BITFIELD_BIT(0);
                uint32_t                                     : BITFIELD_RANGE(1 , 5 );
                uint32_t IndirectStateEntryAddress           : BITFIELD_RANGE(6 , 31);
            };

            uint32_t Value;
        } DW1;

        union
        {
            struct
            {
                uint32_t IndirectStateEntryAddress           : BITFIELD_RANGE(0 , 15);
                uint32_t                                     : BITFIELD_RANGE(16, 31);
            };

            uint32_t Value;
        } DW2;
    } QW1;

    // DW 3
    // Indirect State Entry Address Attributes
    union
    {
        struct
        {
            uint32_t Age                                     : BITFIELD_RANGE(0 , 1 );
            uint32_t EncryptedData                           : BITFIELD_BIT(2);
            uint32_t TargetCache                             : BITFIELD_RANGE(3 , 4 );
            uint32_t MemoryType                              : BITFIELD_RANGE(5 , 6 );
            uint32_t ArbitrationPriorityControl              : BITFIELD_RANGE(7 , 8 );
            uint32_t                                         : BITFIELD_RANGE(9 , 31);
        } Obj1;

        struct
        {
            uint32_t MemoryObjectControlState                : BITFIELD_RANGE(0 , 6 );
            uint32_t ArbitrationPriorityControl              : BITFIELD_RANGE(7 , 8 );
        uint32_t                                             : BITFIELD_RANGE(9 , 31);
        };

        uint32_t Value;
    } DW3;

    // DW 4
    union
    {
        struct
        {
            uint32_t NumberOfEntriesMinus1                   : BITFIELD_RANGE(0 , 31);    // U32
        };

        uint32_t Value;
    } DW4;

    // DW 5
    union
    {
        struct
        {
            uint32_t AesCtrIv                                : BITFIELD_RANGE(0 , 31);    // U32
        };

        uint32_t Value;
    } DW5;

    // DW 6
    union
    {
        struct
        {
            uint32_t AesCtrIv                                : BITFIELD_RANGE(0 , 31);    // U32
        };

        uint32_t Value;
    } DW6;

    // DW 7
    union
    {
        struct
        {
            uint32_t AesCtrIv                                : BITFIELD_RANGE(0 , 31);    // U32
        };

        uint32_t Value;
    } DW7;

    // DW 8
    union
    {
        struct
        {
            uint32_t AesCtrIv                                : BITFIELD_RANGE(0 , 31);    // U32
        };

        uint32_t Value;
    } DW8;
} AVP_AES_IND_STATE_CMD, *PAVP_AES_IND_STATE_CMD;
C_ASSERT(SIZE32(_AVP_AES_IND_STATE_CMD) == 9);

typedef struct _VVCP_AES_IND_STATE_CMD
{
    // DW 0
    union
    {
        struct
        {
            uint32_t DwordLength                             : BITFIELD_RANGE(0 , 11);   // U12
            uint32_t                                         : BITFIELD_RANGE(12, 15);
            uint32_t MediaInstructionCommand                 : BITFIELD_RANGE(16, 21);   // OpCode
            uint32_t MediaInstructionOpcode                  : BITFIELD_RANGE(22, 26);   // OpCode
            uint32_t PipelineType                            : BITFIELD_RANGE(27, 28);   // OpCode
            uint32_t CommandType                             : BITFIELD_RANGE(29, 31);   // OpCode
        };

        uint32_t Value;
    } DW0;

    // DW 1_2
    struct
    {
        union
        {
            struct
            {
                uint32_t SliceIvPresent                      : BITFIELD_BIT(0);
                uint32_t                                     : BITFIELD_RANGE(1 , 5 );
                uint32_t IndirectStateEntryAddress           : BITFIELD_RANGE(6 , 31);
            };

            uint32_t Value;
        } DW1;

        union
        {
            struct
            {
                uint32_t IndirectStateEntryAddress           : BITFIELD_RANGE(0 , 15);
                uint32_t                                     : BITFIELD_RANGE(16, 31);
            };

            uint32_t Value;
        } DW2;
    } QW1;

    // DW 3
    // Indirect State Entry Address Attributes
    union
    {
        struct
        {
            uint32_t Age                                     : BITFIELD_RANGE(0 , 1 );
            uint32_t EncryptedData                           : BITFIELD_BIT(2);
            uint32_t TargetCache                             : BITFIELD_RANGE(3 , 4 );
            uint32_t MemoryType                              : BITFIELD_RANGE(5 , 6 );
            uint32_t ArbitrationPriorityControl              : BITFIELD_RANGE(7 , 8 );
            uint32_t                                         : BITFIELD_RANGE(9 , 31);
        } Obj1;

        struct
        {
            uint32_t MemoryObjectControlState                : BITFIELD_RANGE(0 , 6 );
            uint32_t ArbitrationPriorityControl              : BITFIELD_RANGE(7 , 8 );
        uint32_t                                             : BITFIELD_RANGE(9 , 31);
        };

        uint32_t Value;
    } DW3;

    // DW 4
    union
    {
        struct
        {
            uint32_t NumberOfEntriesMinus1                   : BITFIELD_RANGE(0 , 31);    // U32
        };

        uint32_t Value;
    } DW4;

    // DW 5
    union
    {
        struct
        {
            uint32_t AesCtrIv0                                : BITFIELD_RANGE(0 , 31);    // U32
        };

        uint32_t Value;
    } DW5;

    // DW 6
    union
    {
        struct
        {
            uint32_t AesCtrIv1                                : BITFIELD_RANGE(0 , 31);    // U32
        };

        uint32_t Value;
    } DW6;

    // DW 7
    union
    {
        struct
        {
            uint32_t AesCtrIv2                                : BITFIELD_RANGE(0 , 31);    // U32
        };

        uint32_t Value;
    } DW7;

    // DW 8
    union
    {
        struct
        {
            uint32_t AesCtrIv3                                : BITFIELD_RANGE(0 , 31);    // U32
        };

        uint32_t Value;
    } DW8;
} VVCP_AES_IND_STATE_CMD, *PVVCP_AES_IND_STATE_CMD;
C_ASSERT(SIZE32(_VVCP_AES_IND_STATE_CMD) == 9);

typedef struct _HUC_INDIRECT_CRYPTO_STATE_G8LP
{
    // DW 0
    union
    {
        uint32_t AESCtrOrIVDW0;
    } DW0;

    // DW 1
    union
    {
        uint32_t AESCtrOrIVDW1;
    } DW1;

    // DW 2
    union
    {
        uint32_t AESCtrOrIVDW2;
    } DW2;

    // DW 3
    union
    {
        uint32_t AESCtrOrIVDW3;
    } DW3;

    // DW4
    union
    {
        struct
        {
            uint32_t FPClearBytesLength                      : 23 ;
            uint32_t : 9;
        };
        struct
        {
            uint32_t Value;
        };
    } DW4;

    // DW5
    union
    {
        struct
        {
            uint32_t FPEncryptedBytesLength                  : 23 ;
            uint32_t                                         : 9  ;
        };
        struct
        {
            uint32_t Value;
        };
    } DW5;

    // DW6
    union
    {
        struct
        {
            uint32_t InitByteLength                          : 16 ;
            uint32_t PartialBlockLength                      : 4  ;
            uint32_t : 9;
            uint32_t MonoCounterCheck                        : 1  ;
            uint32_t UseSuppliedIVorCTR                      : 1  ;
            uint32_t InitFPDECEPktType                       : 1  ;
        };
        struct
        {
            uint32_t Value;
        };
    } DW6;

    // DW7
    union
    {
        struct
        {
            uint32_t : 26;
            uint32_t AESCbcFlavor                            : 1  ;
            uint32_t AESCtrFlavor                            : 2  ;
            uint32_t AESMode                                 : 1  ;
            uint32_t                                         : 1  ;
            uint32_t AESEnable                               : 1  ;
        };
        struct
        {
            uint32_t Value;
        };
    } DW7;

    // DW8
    union
    {
        struct
        {
            uint32_t EndAddress                              : 29 ;
            uint32_t                                         : 3  ;
        };
        struct
        {
            uint32_t Value;
        };
    } DW8;

    // DW9
    union
    {
        struct
        {
            uint32_t StartAddress                            : 29 ;
            uint32_t                                         : 3  ;
        };
        struct
        {
            uint32_t Value;
        };
    } DW9;

    uint32_t            DW10;
    uint32_t            DW11;
    uint32_t            DW12;
    uint32_t            DW13;
    uint32_t            DW14;
    uint32_t            DW15;
} HUC_INDIRECT_CRYPTO_STATE_G8LP, *PHUC_INDIRECT_CRYPTO_STATE_G8LP;
C_ASSERT(SIZE32(HUC_INDIRECT_CRYPTO_STATE_G8LP) == 16);

extern const MFX_CRYPTO_COPY_BASE_ADDR_CMD_G8       g_cInit_MFX_CRYPTO_COPY_BASE_ADDR_CMD_G8;
extern const MFX_CRYPTO_COPY_CMD_G8                 g_cInit_MFX_CRYPTO_COPY_CMD_G8;
extern const MFX_CRYPTO_KEY_EXCHANGE_CMD            g_cInit_MFX_CRYPTO_KEY_EXCHANGE_CMD;
extern const MFX_CRYPTO_INLINE_STATUS_READ_CMD_G8   g_cInit_MFX_CRYPTO_INLINE_STATUS_READ_CMD_G8;
extern const MI_SET_APP_ID_CMD                      g_cInit_MI_SET_APP_ID_CMD;
extern const HCP_AES_STATE_CMD                      g_cInit_HCP_AES_STATE_CMD;
extern const VVCP_AES_STATE_CMD                     g_cInit_VVCP_AES_STATE_CMD;
extern const MFX_AES_STATE_CMD_G8                   g_cInit_MFX_AES_STATE_CMD_G8;
extern const MFX_AES_STATE_CMD_G9                   g_cInit_MFX_AES_STATE_CMD_G9;
extern const HUC_AES_STATE_CMD                      g_cInit_HUC_AES_STATE_CMD;
extern const HCP_AES_IND_STATE_CMD                  g_cInit_HCP_AES_IND_STATE_CMD;
extern const AVP_AES_IND_STATE_CMD                  g_cInit_AVP_AES_IND_STATE_CMD;
extern const HUC_AES_IND_STATE_CMD                  g_cInit_HUC_AES_IND_STATE_CMD;
extern const VVCP_AES_IND_STATE_CMD                 g_cInit_VVCP_AES_IND_STATE_CMD;

#ifdef __cplusplus
}
#endif // __cplusplus

//-----------PIPELINE_SELECT_G9 and PIPELINE_SELECT_G11 is needed in addprolog function------------//

//!
//! \brief PIPELINE_SELECT_G9
//! \details
//!     The PIPELINE_SELECT command is used to specify which GPE pipeline is to
//!     be considered the 'current'  active pipeline. Issuing
//!     3D-pipeline-specific commands when the Media pipeline is selected, or
//!     vice versa, is UNDEFINED.
//!     
//!     Issuing 3D-pipeline-specific commands when the GPGPU pipeline is
//!     selected, or vice versa, is UNDEFINED.
//!     
//!     Programming common non pipeline commands (e.g., STATE_BASE_ADDRESS) is
//!     allowed in all pipeline modes.
//!     
//!     Software must ensure all the write caches are flushed through a stalling
//!     PIPE_CONTROL command followed by another PIPE_CONTROL command to
//!     invalidate read only caches prior to programming MI_PIPELINE_SELECT
//!     command to change the Pipeline Select Mode. Example: ... Workload-3Dmode
//!     PIPE_CONTROL  (CS Stall, Depth Cache Flush Enable, Render Target Cache
//!     Flush Enable, DC Flush Enable) PIPE_CONTROL  (Constant Cache Invalidate,
//!     Texture Cache Invalidate, Instruction Cache Invalidate, State Cache
//!     invalidate) PIPELINE_SELECT ( GPGPU)
//!     
//!     Software must clear the COLOR_CALC_STATE Valid field in
//!     3DSTATE_CC_STATE_POINTERS command prior to send a PIPELINE_SELECT with
//!     Pipeline Select set to GPGPU.
//!     
//!     Render CS Only: SW must always program PIPE_CONTROL with CS Stall and
//!     Render Target Cache Flush Enable set prior to programming
//!     PIPELINE_SELECT command for GPGPU workloads i.e when pipeline mode is
//!     set to GPGPU. This is required to achieve better GPGPU preemption
//!     latencies for certain programming sequences. If programming PIPE_CONTROL
//!     has performance implications then preemption latencies can be trade off
//!     against performance by not implementing this programming note.
//!     
//!     Hardware Binding Tables are only supported for 3D workloads. Resource
//!     streamer must be enabled only for 3D workloads. Resource streamer must
//!     be disabled for Media and GPGPU workloads. Batch buffer containing both
//!     3D and GPGPU workloads must take care of disabling and enabling Resource
//!     Streamer appropriately while changing the PIPELINE_SELECT mode from 3D
//!     to GPGPU and vice versa. Resource streamer must be disabled using
//!     MI_RS_CONTROL command and Hardware Binding Tables must be disabled by
//!     programming 3DSTATE_BINDING_TABLE_POOL_ALLOC with "Binding Table Pool
//!     Enable" set  to disable (i.e. value '0'). Example below shows disabling
//!     and enabling of resource streamer in a batch buffer for 3D and GPGPU
//!     workloads. MI_BATCH_BUFFER_START (Resource Streamer Enabled)
//!     PIPELINE_SELECT (3D) 3DSTATE_BINDING_TABLE_POOL_ALLOC (Binding Table
//!     Pool Enabled) 3D WORKLAOD MI_RS_CONTROL (Disable Resource Streamer)
//!     3DSTATE_BINDING_TABLE_POOL_ALLOC (Binding Table Pool Disabled)
//!     PIPELINE_SELECT (GPGPU) GPGPU Workload 3DSTATE_BINDING_TABLE_POOL_ALLOC
//!     (Binding Table Pool Enabled) MI_RS_CONTROL (Enable Resource Streamer) 3D
//!     WORKLOAD MI_BATCH_BUFFER_END
//!     
struct PIPELINE_SELECT_CMD_G9
{
    union
    {
        struct
        {
            /// DW 0
            uint32_t                 PipelineSelection                                : __CODEGEN_BITFIELD( 0,  1)    ; ///< U2
            uint32_t                 RenderSliceCommonPowerGateEnable                 : __CODEGEN_BITFIELD( 2,  2)    ; ///< U1
            uint32_t                 RenderSamplerPowerGateEnable                     : __CODEGEN_BITFIELD( 3,  3)    ; ///< U1
            uint32_t                 MediaSamplerDopClockGateEnable                   : __CODEGEN_BITFIELD( 4,  4)    ; ///< U1
            uint32_t                 ForceMediaAwake                                  : __CODEGEN_BITFIELD( 5,  5)    ; ///< U1
            uint32_t                 Reserved_6                                       : __CODEGEN_BITFIELD( 6,  7)    ; ///< U1
            uint32_t                 MaskBits                                         : __CODEGEN_BITFIELD( 8, 15)    ; ///< U8
            uint32_t                 _3DCommandSubOpcode                              : __CODEGEN_BITFIELD(16, 23)    ; ///< U8
            uint32_t                 _3DCommandOpcode                                 : __CODEGEN_BITFIELD(24, 26)    ; ///< U3
            uint32_t                 CommandSubtype                                   : __CODEGEN_BITFIELD(27, 28)    ; ///< U2
            uint32_t                 CommandType                                      : __CODEGEN_BITFIELD(29, 31)    ; ///< U3
        };
        uint32_t                     Value;
    } DW0;

    //! \name Local enumerations

    enum PIPELINE_SELECTION
    {
        PIPELINE_SELECTION_3D                                            = 0, ///< 3D pipeline is selected
        PIPELINE_SELECTION_MEDIA                                         = 1, ///< Media pipeline is selected (Includes HD optical disc playback, HD video playback, and generic media workloads)
        PIPELINE_SELECTION_GPGPU                                         = 2, ///< GPGPU pipeline is selected
    };
    enum RENDER_SLICE_COMMON_POWER_GATE_ENABLE
    {
        RENDER_SLICE_COMMON_POWER_GATE_ENABLE_DISABLED                   = 0, ///< Command Streamer sends message to PM to disable render slice common Power Gating.
        RENDER_SLICE_COMMON_POWER_GATE_ENABLE_ENABLED                    = 1, ///< Command Streamer sends message to PM to enable render slice common Power Gating.
    };
    enum RENDER_SAMPLER_POWER_GATE_ENABLE
    {
        RENDER_SAMPLER_POWER_GATE_ENABLE_DISABLED                        = 0, ///< Command Streamer sends message to PM to disable render sampler Power Gating.
        RENDER_SAMPLER_POWER_GATE_ENABLE_ENABLED                         = 1, ///< Command Streamer sends message to PM to enable render sampler Power Gating.
    };
    enum MEDIA_SAMPLER_DOP_CLOCK_GATE_ENABLE
    {
        MEDIA_SAMPLER_DOP_CLOCK_GATE_ENABLE_DISABLED                     = 0, ///< Command Streamer sends message to PM to disable sampler DOP Clock Gating.
        MEDIA_SAMPLER_DOP_CLOCK_GATE_ENABLE_ENABLED                      = 1, ///< Command Streamer sends message to PM to enable media sampler DOP Clock Gating.
    };
    enum FORCE_MEDIA_AWAKE
    {
        FORCE_MEDIA_AWAKE_DISABLED                                       = 0, ///< Command streamer sends message to PM to disable force awake of media engine (next instructions do not require the media engine to be awake). Command streamer waits for acknowledge from PM before parsing the next command.
        FORCE_MEDIA_AWAKE_ENABLED                                        = 1, ///< Command streamer sends message to PM to force awake media engine (next instructions require media engine awake). Command streamer waits for acknowledge from PM before parsing the next command.
    };
    enum _3D_COMMAND_SUB_OPCODE
    {
        _3D_COMMAND_SUB_OPCODE_PIPELINE_SELECT                           = 4, ///< 
    };
    enum _3D_COMMAND_OPCODE
    {
        _3D_COMMAND_OPCODE_GFXPIPE_NONPIPELINED                          = 1, ///< 
    };
    enum COMMAND_SUBTYPE
    {
        COMMAND_SUBTYPE_GFXPIPE_SINGLE_DW                                = 1, ///< 
    };
    enum COMMAND_TYPE
    {
        COMMAND_TYPE_GFXPIPE                                             = 3, ///< 
    };

    //! \name Initializations

    //! \brief Explicit member initialization function
    PIPELINE_SELECT_CMD_G9();

    static const size_t dwSize = 1;
    static const size_t byteSize = 4;
};

//!
//! \brief PIPELINE_SELECT_G11
//! \details
//!     The PIPELINE_SELECT command is used to specify which GPE pipeline is to
//!     be considered the 'current'  active pipeline. Issuing
//!     3D-pipeline-specific commands when the Media pipeline is selected, or
//!     vice versa, is UNDEFINED.
//!     
//!     Issuing 3D-pipeline-specific commands when the GPGPU pipeline is
//!     selected, or vice versa, is UNDEFINED.
//!     
//!     Programming common non pipeline commands (e.g., STATE_BASE_ADDRESS) is
//!     allowed in all pipeline modes.
//!     
//!     Software must ensure all the write caches are flushed through a stalling
//!     PIPE_CONTROL command followed by another PIPE_CONTROL command to
//!     invalidate read only caches prior to programming MI_PIPELINE_SELECT
//!     command to change the Pipeline Select Mode. Example: ... Workload-3Dmode
//!     PIPE_CONTROL  (CS Stall, Depth Cache Flush Enable, Render Target Cache
//!     Flush Enable, DC Flush Enable) PIPE_CONTROL  (Constant Cache Invalidate,
//!     Texture Cache Invalidate, Instruction Cache Invalidate, State Cache
//!     invalidate) PIPELINE_SELECT ( GPGPU)
//!     
//!     Workaround
//!     https://vthsd.fm.intel.com/hsd/gen10lp/#bug_de/default.aspx?bug_de_id=1939080
//!     : This command must be followed by a PIPE_CONTROL with CS Stall bit
//!     set.,
//!     
struct PIPELINE_SELECT_CMD_G11
{
    union
    {
        struct
        {
            /// DW 0
            uint32_t                 PipelineSelection                                : __CODEGEN_BITFIELD( 0,  1)    ; ///< U2
            uint32_t                 RenderSliceCommonPowerGateEnable                 : __CODEGEN_BITFIELD( 2,  2)    ; ///< U1
            uint32_t                 RenderSamplerPowerGateEnable                     : __CODEGEN_BITFIELD( 3,  3)    ; ///< U1
            uint32_t                 MediaSamplerDopClockGateEnable                   : __CODEGEN_BITFIELD( 4,  4)    ; ///< U1
            uint32_t                 ForceMediaAwake                                  : __CODEGEN_BITFIELD( 5,  5)    ; ///< U1
            uint32_t                 MediaSamplerPowerClockGateDisable                : __CODEGEN_BITFIELD( 6,  6)    ; ///< U1
            uint32_t                 Reserved_7                                       : __CODEGEN_BITFIELD( 7,  7)    ; ///< U1
            uint32_t                 MaskBits                                         : __CODEGEN_BITFIELD( 8, 15)    ; ///< U8
            uint32_t                 _3DCommandSubOpcode                              : __CODEGEN_BITFIELD(16, 23)    ; ///< U8
            uint32_t                 _3DCommandOpcode                                 : __CODEGEN_BITFIELD(24, 26)    ; ///< U3
            uint32_t                 CommandSubtype                                   : __CODEGEN_BITFIELD(27, 28)    ; ///< U2
            uint32_t                 CommandType                                      : __CODEGEN_BITFIELD(29, 31)    ; ///< U3
        };
        uint32_t                     Value;
    } DW0;

    //! \name Local enumerations

    enum PIPELINE_SELECTION
    {
        PIPELINE_SELECTION_3D                                            = 0, ///< 3D pipeline is selected
        PIPELINE_SELECTION_MEDIA                                         = 1, ///< Media pipeline is selected (Includes HD optical disc playback, HD video playback, and generic media workloads)
        PIPELINE_SELECTION_GPGPU                                         = 2, ///< GPGPU pipeline is selected
    };
    enum RENDER_SLICE_COMMON_POWER_GATE_ENABLE
    {
        RENDER_SLICE_COMMON_POWER_GATE_ENABLE_DISABLED                   = 0, ///< Command Streamer sends message to PM to disable render slice common Power Gating.
        RENDER_SLICE_COMMON_POWER_GATE_ENABLE_ENABLED                    = 1, ///< Command Streamer sends message to PM to enable render slice common Power Gating.
    };
    enum RENDER_SAMPLER_POWER_GATE_ENABLE
    {
        RENDER_SAMPLER_POWER_GATE_ENABLE_DISABLED                        = 0, ///< Command Streamer sends message to PM to disable render sampler Power Gating.
        RENDER_SAMPLER_POWER_GATE_ENABLE_ENABLED                         = 1, ///< Command Streamer sends message to PM to enable render sampler Power Gating.
    };
    enum MEDIA_SAMPLER_DOP_CLOCK_GATE_ENABLE
    {
        MEDIA_SAMPLER_DOP_CLOCK_GATE_ENABLE_DISABLED                     = 0, ///< Command Streamer sends message to PM to disable sampler DOP Clock Gating.
        MEDIA_SAMPLER_DOP_CLOCK_GATE_ENABLE_ENABLED                      = 1, ///< Command Streamer sends message to PM to enable media sampler DOP Clock Gating.
    };
    enum FORCE_MEDIA_AWAKE
    {
        FORCE_MEDIA_AWAKE_DISABLED                                       = 0, ///< Command streamer sends message to PM to disable force awake of media engine (next instructions do not require the media engine to be awake). Command streamer waits for acknowledge from PM before parsing the next command.
        FORCE_MEDIA_AWAKE_ENABLED                                        = 1, ///< Command streamer sends message to PM to force awake media engine (next instructions require media engine awake). Command streamer waits for acknowledge from PM before parsing the next command.
    };
    enum _3D_COMMAND_SUB_OPCODE
    {
        _3D_COMMAND_SUB_OPCODE_PIPELINE_SELECT                           = 4, ///< 
    };
    enum _3D_COMMAND_OPCODE
    {
        _3D_COMMAND_OPCODE_GFXPIPE_NONPIPELINED                          = 1, ///< 
    };
    enum COMMAND_SUBTYPE
    {
        COMMAND_SUBTYPE_GFXPIPE_SINGLE_DW                                = 1, ///< 
    };
    enum COMMAND_TYPE
    {
        COMMAND_TYPE_GFXPIPE                                             = 3, ///< 
    };

    //! \name Initializations

    //! \brief Explicit member initialization function
    PIPELINE_SELECT_CMD_G11();

    static const size_t dwSize = 1;
    static const size_t byteSize = 4;
};
#endif
