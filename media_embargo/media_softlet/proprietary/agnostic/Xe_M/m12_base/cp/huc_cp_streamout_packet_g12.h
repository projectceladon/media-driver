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
//! \file     huc_cp_streamout_packet_g12.h
//! \brief    Defines the interface of huc cp streamout packet
//!

#ifndef __CODECHAL_HUC_CP_STREAMOUT_PACKET_G12_H__
#define __CODECHAL_HUC_CP_STREAMOUT_PACKET_G12_H__

#include "decode_huc_copy_packet_g12.h"
#include "media_pipeline.h"
#include "codechal_hw.h"
#include "cp_streamout_interface.h"
#include "decode_resource_array.h"

#define INDRECT_STATE_BUFFER_NUM 16

namespace decode
{
    class HucCpStreamOutPktG12 : public HucCopyPktG12, public CpStreamOutInterface
    {
    public:
        HucCpStreamOutPktG12(MediaPipeline *pipeline, MediaTask *task, CodechalHwInterface *hwInterface)
            : HucCopyPktG12(pipeline, task, hwInterface)
        {
        }

        virtual ~HucCpStreamOutPktG12();

        //!
        //! \brief  Get Packet Name
        //! \return std::string
        //!
        virtual std::string GetPacketName() override
        {
            return "HUC CP StreamOut G12";
        }

        virtual MOS_STATUS PushCopyParams(HucCopyParams &copyParams) override;

    protected:
        virtual void SetHucPipeModeSelectParameters(MHW_VDBOX_PIPE_MODE_SELECT_PARAMS &pipeModeSelectParams) override;
        virtual void SetIndObjParameters(MHW_VDBOX_IND_OBJ_BASE_ADDR_PARAMS &indObjParams) override;
        virtual void SetStreamObjectParameters(MHW_VDBOX_HUC_STREAM_OBJ_PARAMS &streamObjParams,
            CODEC_HEVC_SLICE_PARAMS &sliceParams) override;
        virtual MOS_STATUS AddHucIndObj(MOS_COMMAND_BUFFER &cmdBuffer) override;
        MOS_STATUS AllocateStreamOutResource();

    private:
        BufferArray             *m_hucIndStateBufArr  = nullptr;  //! IndState buffer
        std::vector<PMOS_BUFFER> m_hucIndStateBufList = {};
        uint32_t                 m_hucIndStateBufIdx  = 0;

    MEDIA_CLASS_DEFINE_END(decode__HucCpStreamOutPktG12)
    };
}

#endif
