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
//! \file     huc_cp_streamout_packet.h
//! \brief    Defines the interface of huc cp streamout packet
//!

#ifndef __CODECHAL_HUC_CP_STREAMOUT_PACKET_H__
#define __CODECHAL_HUC_CP_STREAMOUT_PACKET_H__

#include "decode_huc_copy_packet.h"
#include "media_pipeline.h"
#include "codec_hw_next.h"
#include "cp_streamout_interface.h"
#include "decode_resource_array.h"

#define INDRECT_STATE_BUFFER_NUM 16

namespace decode
{
class HucCpStreamOutPkt : public HucCopyPkt, public CpStreamOutInterface
{
public:
    HucCpStreamOutPkt(MediaPipeline *pipeline, MediaTask *task, CodechalHwInterfaceNext *hwInterface)
        : HucCopyPkt(pipeline, task, hwInterface)
    {
    }

    virtual ~HucCpStreamOutPkt();

    //!
    //! \brief  Get Packet Name
    //! \return std::string
    //!
    virtual std::string GetPacketName() override
    {
        return "HUC CP StreamOut";
    }

    virtual MOS_STATUS PushCopyParams(HucCopyParams &copyParams) override;

protected:
    virtual MOS_STATUS AddCmd_HUC_PIPE_MODE_SELECT(MOS_COMMAND_BUFFER &cmdBuffer) override;
    virtual MOS_STATUS AddHucIndState(MOS_COMMAND_BUFFER &cmdBuffer) override;
    MOS_STATUS AllocateStreamOutResource();

private:
    BufferArray *        m_hucIndStateBufArr = nullptr;  //! IndState buffer
    PMOS_BUFFER          m_hucIndStateBuf    = nullptr;

MEDIA_CLASS_DEFINE_END(decode__HucCpStreamOutPkt)
};
}

#endif
