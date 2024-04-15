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
//! \file     mhw_cp_hwcmd_next.h
//! \brief    CP command defintions for all Next-based platforms
//! \details  This file should not be included outside of MHW_CP c files. As the
//!           commands should not be constructed outside of MHW_CP.
//!
#ifndef __MHW_CP_HWCMD_NEXT_H__
#define __MHW_CP_HWCMD_NEXT_H__

#include "mhw_cp.h"

namespace mhw_pavp_next
{
    /*****************************************************************************\
    STRUCT: _MI_STORE_REGISTER_MEM_CMD
    PURPOSE: Defines MI_STORE_REGISTER_MEM GPU command
    VERSION:
    \*****************************************************************************/
    struct MFX_CRYPTO_COPY_CMD
    {
        union
        {
            struct
            {
                uint32_t DWordLength                : BITFIELD_RANGE(0, 15);   // OP_LENGTH
                uint32_t CommandSubOpcodeB          : BITFIELD_RANGE(16, 20);  // SUBOPCODE_B
                uint32_t CommandSubOpcodeA          : BITFIELD_RANGE(21, 23);  // SUBOPCODE_A
                uint32_t CommandOpcode              : BITFIELD_RANGE(24, 26);  // OPCODE
                uint32_t CommandPipeLine            : BITFIELD_RANGE(27, 28);  // INSTRUCTION_PIPELINE
                uint32_t CommandType                : BITFIELD_RANGE(29, 31);  // INSTRUCTION_TYPE
            };
            uint32_t Value;
        } DW0;

        // DW 1
        union
        {
            struct
            {
                uint32_t IndirectDataLength         : BITFIELD_RANGE(0, 22);   // Indirect Data Length
                uint32_t WiDiKeyInuse               : BITFIELD_BIT(23);        // Selects the WiDi session to be used for a Crypto Copy Encryption operation in Display Apps.
                uint32_t UseIntialCounterValue      : BITFIELD_BIT(24);        // AES counter intialization value from previous pass
                uint32_t AesCounterFlavor           : BITFIELD_RANGE(25, 26);  // AES counter flavor for HDCP2
                uint32_t Reserved_27                : BITFIELD_BIT(27);        // Reserved MBZ
                uint32_t SelectionForIndirectData   : BITFIELD_RANGE(28, 31);   // Encryption/Authentication selection for the indirect source data
            };
            uint32_t Value;
        } DW1;

        // DW 2
        union
        {
            struct
            {
                uint32_t SrcBaseOffset              : BITFIELD_RANGE(0, 31);
            };
            uint32_t Value;
        } DW2;

        // DW 3
        union
        {
            struct
            {
                uint32_t SrcBaseOffset              : BITFIELD_RANGE(0, 15);   // In 16 bytes
                uint32_t Reserved_16                : BITFIELD_RANGE(16, 31);  // Reserved
            };
            uint32_t Value;
        } DW3;

        // DW 4
        union
        {
            struct
            {
                uint32_t DstBaseOffset              : BITFIELD_RANGE(0, 31);
            };
            uint32_t Value;
        } DW4;

        // DW 5
        union
        {
            struct
            {
                uint32_t DstBaseOffset              : BITFIELD_RANGE(0, 15);   // In 16 bytes
                uint32_t Reserved_16                : BITFIELD_RANGE(16, 31);  // Reserved
            };
            uint32_t Value;
        } DW5;

        // DW 6
        union
        {
            struct
            {
                uint32_t Counter                    : BITFIELD_RANGE(0, 31);   // Initial counter value for AES-128 counter mode decryption
            };
            uint32_t Value;
        } DW6;

        // DW 7
        union
        {
            struct
            {
                uint32_t InitializationValueDw1     : BITFIELD_RANGE(0, 31);
            };
            uint32_t Value;
        } DW7;

        // DW 8
        union
        {
            struct
            {
                uint32_t InitializationValueDw2     : BITFIELD_RANGE(0, 31);
            };
            uint32_t Value;
        } DW8;

        // DW 9
        union
        {
            struct
            {
                uint32_t InitializationValueDw3     : BITFIELD_RANGE(0, 31);
            };
            uint32_t Value;
        } DW9;

        // DW 10
        union
        {
            struct
            {
                uint32_t InitialPacketLength        : BITFIELD_RANGE(0, 15);   // Initial packet Length
                uint32_t Reserved_16                : BITFIELD_RANGE(16, 27);  // Reserved MBZ
                uint32_t AesOperationType           : BITFIELD_RANGE(28, 29);  // AES operations input or output
                uint32_t Reserved_30                : BITFIELD_BIT(30);        // Reserved MBZ
                uint32_t StartPacketType            : BITFIELD_BIT(31);        // Initial packet type clear or encrypted
            };
            uint32_t Value;
        } DW10;

        // DW 11
        union
        {
            struct
            {
                uint32_t EncryptedBytesLength       : BITFIELD_RANGE(0, 22);    // Length of encrypted packet in bytes
                uint32_t Reserved_23                : BITFIELD_RANGE(23, 31);   // Reserved MBZ
            };
            uint32_t Value;
        } DW11;

        // DW 12
        union
        {
            struct
            {
                uint32_t ClearBytesLength           : BITFIELD_RANGE(0, 22);    // Length of clear packet in bytes
                uint32_t Reserved_23                : BITFIELD_RANGE(23, 31);   // Reserved MBZ
            };
            uint32_t Value;
        } DW12;

        // DW 13
        union
        {
            struct
            {
                uint32_t Reserved_0                 : BITFIELD_RANGE(0, 14);   // Reserved
                uint32_t cryptoCopyCmdMode          : BITFIELD_BIT(15);        // crypto copy cmd mode
                uint32_t lengthOfTable              : BITFIELD_RANGE(16, 31);  // length of table

            };
            uint32_t Value;
        } DW13;

        static const size_t dwSize = 14;
        static const size_t byteSize = dwSize * sizeof(uint32_t);
        MFX_CRYPTO_COPY_CMD();
    };
}

#endif