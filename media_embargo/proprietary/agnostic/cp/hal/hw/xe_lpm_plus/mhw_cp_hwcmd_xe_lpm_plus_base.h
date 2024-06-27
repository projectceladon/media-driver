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
//! \file     mhw_cp_hwcmd_xe_lpm_plus_base.h
//! \brief    CP command defintions for all Gen13-based platforms
//! \details  This file should not be included outside of mhw_cp_hwcmd_g13_X c files. As the
//!           commands should not be constructed outside of mhw_cp_hwcmd_g13_X.
//!
#ifndef __MHW_CP_HWCMD_G13_X_H__
#define __MHW_CP_HWCMD_G13_X_H__

#pragma once
#pragma pack(1)

#include <cstdint>
#include <cstddef>

class mhw_pavp_g13_X
{
public:
    // Internal Macros
    #define __CODEGEN_MAX(_a, _b) (((_a) > (_b)) ? (_a) : (_b))
    #define __CODEGEN_BITFIELD(l, h) (h) - (l) + 1
    #define __CODEGEN_OP_LENGTH_BIAS 2
    #define __CODEGEN_OP_LENGTH(x) (uint32_t)((__CODEGEN_MAX(x, __CODEGEN_OP_LENGTH_BIAS)) - __CODEGEN_OP_LENGTH_BIAS)

    struct GSC_HECI_CMD
    {
        // DW 0 GSC Command Header
        union
        {
            struct
            {
                uint32_t DwordLength                                    : __CODEGEN_BITFIELD(0 , 15);
                uint32_t Reserved                                       : __CODEGEN_BITFIELD(16, 21);
                uint32_t CommandSubType                                 : __CODEGEN_BITFIELD(22, 28);    // OpCode
                uint32_t CommandType                                    : __CODEGEN_BITFIELD(29, 31);    // OpCode
            };

            uint32_t Value;
        } DW0;

        union
        {
            struct
            {
                uint64_t InputBufferAddress                             : __CODEGEN_BITFIELD(0, 63);
            };
            uint32_t Value[2];;
        } DW1_2;

        union
        {
            struct
            {
                uint32_t InputBufferLength                              : __CODEGEN_BITFIELD(0, 31);
            };
            uint32_t Value;
        } DW3;

        union
        {
            struct
            {
                uint64_t OutputBufferAddress                            : __CODEGEN_BITFIELD(0, 63);
            };
            uint32_t Value[2];;
        } DW4_5;

        union
        {
            struct
            {
                uint32_t OutputBufferLength                             : __CODEGEN_BITFIELD(0, 31);
            };
            uint32_t Value;
        } DW6;

        union
        {
            struct
            {
                uint32_t Reserved                                       : __CODEGEN_BITFIELD(0, 31);
            };
            uint32_t Value;
        } DW7;

        //! \name Initializations
        //! \name Local enumerations

        enum COMMAND_TYPE
        {
             GSC_PIPE                                                           = 2, //!< No additional details
        };

        enum COMMAND_SUB_TYPE
        {
            GSC_HECI_CMD_PKT                                                    = 0, //!< No additional details
            GSC_FW_LOAD                                                         = 1, //!< No additional details
        };

        enum RESOURCE_OFFSET
        {
            SRC_DW_OFFSET                                                       = 1, //!< No additional details
            DST_DW_OFFSET                                                       = 4, //!< No additional details
        };

        //! \brief Explicit member initialization function
        GSC_HECI_CMD();

        static const size_t dwSize = 8;
        static const size_t byteSize = dwSize * sizeof(uint32_t);
    };
};

#pragma pack()

#endif // __MHW_CP_HWCMD_G13_X_H__
