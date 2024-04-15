/*
// Copyright (C) 2011-2020 Intel Corporation
//
// Licensed under the Apache License,Version 2.0(the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
//!
//! \file     cp_debug_ext.h
//! \brief    Definition of extention structures and functions for debugging CP
//! \details  This file contains the extension definition of structures and functions
//!           for cp parameter dumper
//!
#ifndef __CP_DEBUG_EXT_H__
#define __CP_DEBUG_EXT_H__

#include <string>

//-------------------------------------------------------------------------------
// CP OCA DUMPER
//-------------------------------------------------------------------------------

#define MAX_SURF_COUNT 20
#define MAX_DUMP_MSG_BUF_SIZE 1024

// Extended surface info struct
typedef struct _CpSurfExtInfo
{
    union
    {
        uint32_t value;
        struct
        {
            uint32_t HwCmdType : 8;           //!< Surface is used by which kind of command
            uint32_t RC : 4;                  //!< Surface is RenderCompressed
            uint32_t MC : 4;                  //!< Surface is MediaCompressed
            uint32_t output : 4;               //!< Surface CpTag will be set
            uint32_t input : 4;               //!< Surface CpTag already set
            uint32_t : 8;                     //!< Reserved for future use.
        };
    };
} CpSurfExtInfo;

struct CP_OCA_PARAM
{
    uint32_t cpContext = 0;
    struct SurfInfo
    {
        uint64_t uGfxAddr;
        uint32_t uCpTag;
        CpSurfExtInfo extInfo;
    } surf[MAX_SURF_COUNT] = {};
};

enum CP_IO_MESSAGE_TYPE
{
    CP_IO_MESSAGE_TYPE_INPUT,
    CP_IO_MESSAGE_TYPE_OUTPUT
};

struct CP_OCA_IO_MESSAGE_HEADER
{
    uint64_t           timestamp;
    CP_IO_MESSAGE_TYPE type;
    uint64_t           uGfxAddr;
    uint32_t           size;
};

struct CP_OCA_IO_MESSAGE
{
    CP_OCA_IO_MESSAGE_HEADER Header;
    uint8_t            data[MAX_DUMP_MSG_BUF_SIZE];
};

class CpOcaDumper
{
public:
    CpOcaDumper();
    virtual ~CpOcaDumper();

    void SetCpContext(uint32_t value);
    void AddProtectedSurfInfo(uint64_t addr, uint32_t cptag, CpSurfExtInfo info);
    void ClearCpParam();
    void SetIoMessage(CP_IO_MESSAGE_TYPE type, uint64_t addr, uint32_t size, uint8_t* data);

    CP_OCA_PARAM* GetCpParam()
    {
        return m_pCpOcaParam;
    }

    CP_OCA_IO_MESSAGE* GetCpIoMsg(CP_IO_MESSAGE_TYPE type)
    {
        return type == CP_IO_MESSAGE_TYPE_INPUT ? m_pCpInputMsg : m_pCpOutputMsg;
    }

private:
    CP_OCA_PARAM *m_pCpOcaParam = nullptr;
    CP_OCA_IO_MESSAGE *m_pCpInputMsg = nullptr;
    CP_OCA_IO_MESSAGE *m_pCpOutputMsg = nullptr;
    uint32_t m_SurfInfoCnt;
};

#endif // __CP_DEBUG_EXT_H__
