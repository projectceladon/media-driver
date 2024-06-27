/*
* Copyright (c) 2021-2022, Intel Corporation
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
//! \file     huc_cp_streamout_packet_g12.cpp
//! \brief    implement the functions for huc cp streamout packet
//!

#include "huc_cp_streamout_packet_g12.h"
#include "decodecp.h"

namespace decode
{
    HucCpStreamOutPktG12::~HucCpStreamOutPktG12()
    {
        m_hucIndStateBufList.clear();
        if (m_hucIndStateBufArr)
        {
            m_allocator->Destroy(m_hucIndStateBufArr);
        }
    }

    void HucCpStreamOutPktG12::SetHucPipeModeSelectParameters(MHW_VDBOX_PIPE_MODE_SELECT_PARAMS &pipeModeSelectParams)
    {
        DECODE_CP_FUNCTION_ENTER;
        HucCopyPktG12::SetHucPipeModeSelectParameters(pipeModeSelectParams);
        m_decodecp->SetHucPipeModeSelectParameter(pipeModeSelectParams.disableProtectionSetting);
    }

    MOS_STATUS HucCpStreamOutPktG12::AddHucIndObj(MOS_COMMAND_BUFFER &cmdBuffer)
    {
        DECODE_CP_FUNCTION_ENTER;

        DECODE_CP_CHK_STATUS(HucCopyPktG12::AddHucIndObj(cmdBuffer));

        if (m_hucIndStateBufIdx < m_hucIndStateBufList.size())
        {
            DECODE_CP_CHK_STATUS(m_decodecp->AddHucIndState(&cmdBuffer, &(m_hucIndStateBufList[m_hucIndStateBufIdx]->OsResource)));
            m_hucIndStateBufIdx++;
        }

        return MOS_STATUS_SUCCESS;
    }

    void HucCpStreamOutPktG12::SetIndObjParameters(MHW_VDBOX_IND_OBJ_BASE_ADDR_PARAMS &indObjParams)
    {
        DECODE_CP_FUNCTION_ENTER;

        HucCopyParams &copyParams = m_copyParamsList.at(m_copyParamsIdx);

        uint32_t dataSize       = copyParams.copyLength;
        uint32_t dataOffset     = MOS_ALIGN_FLOOR(copyParams.srcOffset, MOS_PAGE_SIZE);
        uint32_t relativeOffset = copyParams.srcOffset - dataOffset;

        // Enlarge the stream in/out data size to avoid upper bound hit assert in HuC
        dataSize += relativeOffset;

        // pass bit-stream buffer by Ind Obj Addr command
        indObjParams.presDataBuffer            = copyParams.srcBuffer;
        indObjParams.dwDataSize                = MOS_ALIGN_CEIL(dataSize, MOS_PAGE_SIZE);
        indObjParams.dwDataOffset              = dataOffset;
        indObjParams.presStreamOutObjectBuffer = copyParams.destBuffer;
        indObjParams.dwStreamOutObjectSize     = MOS_ALIGN_CEIL(dataSize, MOS_PAGE_SIZE);
        indObjParams.dwStreamOutObjectOffset   = dataOffset;
    }

    void HucCpStreamOutPktG12::SetStreamObjectParameters(MHW_VDBOX_HUC_STREAM_OBJ_PARAMS &streamObjParams,
        CODEC_HEVC_SLICE_PARAMS &sliceParams)
    {
        DECODE_CP_FUNCTION_ENTER;

        HucCopyParams &copyParams = m_copyParamsList.at(m_copyParamsIdx);

        uint32_t dataOffset     = MOS_ALIGN_FLOOR(copyParams.srcOffset, MOS_PAGE_SIZE);
        uint32_t relativeOffset = copyParams.srcOffset - dataOffset;

        // set stream object with stream out enabled
        streamObjParams.dwIndStreamInLength           = copyParams.copyLength + relativeOffset;
        streamObjParams.dwIndStreamInStartAddrOffset  = 0;
        streamObjParams.dwIndStreamOutStartAddrOffset = 0;
        streamObjParams.bHucProcessing                = true;
        streamObjParams.bStreamInEnable               = true;
        streamObjParams.bStreamOutEnable              = true;
    }

    MOS_STATUS HucCpStreamOutPktG12::PushCopyParams(HucCopyParams &copyParams)
    {
        DECODE_CP_FUNCTION_ENTER;

        auto decodecp = dynamic_cast<DecodeCp *>(m_decodecp);
        DECODE_CP_CHK_NULL_RETURN(decodecp);
        DECODE_CP_CHK_STATUS(decodecp->AllocateStreamOutResource());
        DECODE_CP_CHK_STATUS(decodecp->CalculateSegmentInfo(copyParams.srcBuffer));
        DECODE_CP_CHK_STATUS(AllocateStreamOutResource());

        uint32_t hucCopyNum = decodecp->GetIndStateNum();
        uint32_t streamOutLength = copyParams.copyLength;
        decodecp->SetCopyLength(streamOutLength);

        while (hucCopyNum > 0)
        {
            copyParams.copyLength = decodecp->FetchCopyLength(hucCopyNum - 1);
            copyParams.srcOffset  = streamOutLength - copyParams.copyLength;
            copyParams.destOffset = streamOutLength - copyParams.copyLength;

            streamOutLength -= copyParams.copyLength;
            hucCopyNum--;

            DECODE_CP_CHK_STATUS(HucCopyPktG12::PushCopyParams(copyParams));
        }

        return MOS_STATUS_SUCCESS;
    }

    MOS_STATUS HucCpStreamOutPktG12::AllocateStreamOutResource()
    {
        DECODE_CP_FUNCTION_ENTER;

        DECODE_CP_CHK_NULL_RETURN(m_allocator);
        auto     decodecp     = dynamic_cast<DecodeCp *>(m_decodecp);
        DECODE_CP_CHK_NULL_RETURN(decodecp);
        uint32_t stateBufSize = decodecp->GetIndStateSize();
        uint32_t stateBufNum  = decodecp->GetIndStateNum();

        m_hucIndStateBufList.resize(stateBufNum);
        m_hucIndStateBufIdx = 0;

        for (uint32_t i = 0; i < stateBufNum; i++)
        {
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
            m_hucIndStateBufList[i] = m_hucIndStateBufArr->Peek();
        }
        return MOS_STATUS_SUCCESS;
    }
}  // namespace decode

