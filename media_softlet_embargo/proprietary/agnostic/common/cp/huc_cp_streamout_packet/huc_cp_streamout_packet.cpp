/*
* Copyright (c) 2020, Intel Corporation
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
* OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*/
//!
//! \file     huc_cp_streamout_packet.cpp
//! \brief    implement the functions for huc cp streamout packet
//!

#include "huc_cp_streamout_packet.h"
#include "decodecp.h"

namespace decode
{
HucCpStreamOutPkt::~HucCpStreamOutPkt()
{
    if(m_hucIndStateBufArr)
    {
        m_allocator->Destroy(m_hucIndStateBufArr);
    }
}

MOS_STATUS HucCpStreamOutPkt::AddCmd_HUC_PIPE_MODE_SELECT(MOS_COMMAND_BUFFER &cmdBuffer)
{
    DECODE_FUNC_CALL();
    //for gen 11+, we need to add MFX wait for both KIN and VRT before and after HUC Pipemode select...
    auto &miMfxWaitParams               = m_miItf->MHW_GETPAR_F(MFX_WAIT)();
    miMfxWaitParams                     = {};
    miMfxWaitParams.iStallVdboxPipeline = true;
    DECODE_CP_CHK_STATUS(m_miItf->MHW_ADDCMD_F(MFX_WAIT)(&cmdBuffer));

    auto &par                      = m_hucItf->MHW_GETPAR_F(HUC_PIPE_MODE_SELECT)();
    par                            = {};
    par.mediaSoftResetCounterValue = 2400;
    par.streamOutEnabled           = true;
    par.disableProtectionSetting   = true;

    m_decodecp->SetHucPipeModeSelectParameter(par.disableProtectionSetting);

    m_hucItf->MHW_ADDCMD_F(HUC_PIPE_MODE_SELECT)(&cmdBuffer);
    miMfxWaitParams                     = {};
    miMfxWaitParams.iStallVdboxPipeline = true;
    DECODE_CP_CHK_STATUS(m_miItf->MHW_ADDCMD_F(MFX_WAIT)(&cmdBuffer));

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS HucCpStreamOutPkt::AddHucIndState(MOS_COMMAND_BUFFER &cmdBuffer)
{
    DECODE_CP_CHK_STATUS(m_decodecp->AddHucIndState(&cmdBuffer, &(m_hucIndStateBuf->OsResource)));
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS HucCpStreamOutPkt::PushCopyParams(HucCopyParams &copyParams)
{
    auto decodecp = dynamic_cast<DecodeCp*>(m_decodecp);
    DECODE_CP_CHK_NULL_RETURN(decodecp);
    DECODE_CP_CHK_STATUS(decodecp->AllocateStreamOutResource());
    DECODE_CP_CHK_STATUS(decodecp->CalculateSegmentInfo(copyParams.srcBuffer));
    DECODE_CP_CHK_STATUS(AllocateStreamOutResource());
    decodecp->SetCopyLength(copyParams.copyLength);

    DECODE_CP_CHK_STATUS(HucCopyPkt::PushCopyParams(copyParams));
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS HucCpStreamOutPkt::AllocateStreamOutResource(){
    DECODE_CP_CHK_NULL_RETURN(m_allocator);
    auto     decodecp     = dynamic_cast<DecodeCp *>(m_decodecp);
    DECODE_CP_CHK_NULL_RETURN(decodecp);
    uint32_t stateBufSize = decodecp->GetIndStateSize();

    if (m_hucIndStateBufArr == nullptr)
    {
        m_hucIndStateBufArr = m_allocator->AllocateBufferArray(
            stateBufSize, "HucIndState", INDRECT_STATE_BUFFER_NUM, resourceInternalReadWriteCache, lockableVideoMem);
        DECODE_CP_CHK_NULL_RETURN(m_hucIndStateBufArr);
        auto &hucIndStateBuf = m_hucIndStateBufArr->Fetch();
        DECODE_CP_CHK_NULL_RETURN(hucIndStateBuf);
    }
    else
    {
        auto &hucIndStateBuf = m_hucIndStateBufArr->Fetch();
        DECODE_CP_CHK_NULL_RETURN(hucIndStateBuf);
        DECODE_CP_CHK_STATUS(m_allocator->Resize(hucIndStateBuf, stateBufSize, lockableVideoMem));
    }
    m_hucIndStateBuf = m_hucIndStateBufArr->Peek();
    return MOS_STATUS_SUCCESS;
}
}  // namespace decode
