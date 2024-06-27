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
//! \file     cp_debug_ext.cpp
//! \brief
//! \details  This file contains the Implementation of extention functions for
//!           cp parameter dumper
//!
#include "cp_debug_ext.h"
#include "mos_utilities.h"

CpOcaDumper::CpOcaDumper()
{
    m_pCpOcaParam = MOS_New(CP_OCA_PARAM);
    m_pCpInputMsg = MOS_New(CP_OCA_IO_MESSAGE);
    m_pCpOutputMsg = MOS_New(CP_OCA_IO_MESSAGE);
    m_SurfInfoCnt = 0;
}

CpOcaDumper::~CpOcaDumper()
{
    if (m_pCpOcaParam)
    {
        MOS_Delete(m_pCpOcaParam);
        m_pCpOcaParam = nullptr;
    }

    if (m_pCpInputMsg)
    {
        MOS_Delete(m_pCpInputMsg);
    }

    if (m_pCpOutputMsg)
    {
        MOS_Delete(m_pCpOutputMsg);
    }
}

void CpOcaDumper::SetCpContext(
    uint32_t value)
{
    MOS_OS_CHK_NULL_NO_STATUS_RETURN(m_pCpOcaParam)

    m_pCpOcaParam->cpContext = value;
}

void CpOcaDumper::AddProtectedSurfInfo(
    uint64_t addr,
    uint32_t cptag,
    CpSurfExtInfo info)
{
    MOS_OS_CHK_NULL_NO_STATUS_RETURN(m_pCpOcaParam)

    if (m_SurfInfoCnt < MAX_SURF_COUNT)
    {
        for (uint32_t i = 0; i < m_SurfInfoCnt; i++)
        {
            if (m_pCpOcaParam->surf[i].uGfxAddr == addr)
                return;
        }
        m_pCpOcaParam->surf[m_SurfInfoCnt].uGfxAddr = addr;
        m_pCpOcaParam->surf[m_SurfInfoCnt].uCpTag = cptag;
        m_pCpOcaParam->surf[m_SurfInfoCnt].extInfo.value = info.value;
        m_SurfInfoCnt++;
    }
}

void CpOcaDumper::ClearCpParam()
{
    MOS_OS_CHK_NULL_NO_STATUS_RETURN(m_pCpOcaParam)

    if (m_SurfInfoCnt == 0)
        return;

    for (uint32_t i = 0; i < m_SurfInfoCnt; i++)
    {
        m_pCpOcaParam->surf[i] = {};
    }
    m_SurfInfoCnt = 0;
}

void CpOcaDumper::SetIoMessage(CP_IO_MESSAGE_TYPE type, uint64_t addr, uint32_t size, uint8_t *data)
{
    MOS_OS_CHK_NULL_NO_STATUS_RETURN(m_pCpInputMsg);

    uint32_t copySize = size < MAX_DUMP_MSG_BUF_SIZE ? size : MAX_DUMP_MSG_BUF_SIZE;

    switch (type)
    {
    case CP_IO_MESSAGE_TYPE_INPUT:
        m_pCpInputMsg->Header.type     = type;
        m_pCpInputMsg->Header.uGfxAddr = addr;
        m_pCpInputMsg->Header.size     = copySize;
        
        if (data != nullptr && size != 0)
        {
            MOS_SecureMemcpy(m_pCpInputMsg->data, copySize, data, copySize);
        }

        break;
    case CP_IO_MESSAGE_TYPE_OUTPUT:
        m_pCpOutputMsg->Header.type     = type;
        m_pCpOutputMsg->Header.uGfxAddr = addr;
        m_pCpOutputMsg->Header.size     = copySize;

        if (data != nullptr && size != 0)
        {
            MOS_SecureMemcpy(m_pCpOutputMsg->data, copySize, data, copySize);
        }

        break;
    default:
        break;
    }
}
