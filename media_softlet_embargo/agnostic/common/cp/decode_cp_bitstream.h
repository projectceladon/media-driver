/*
* Copyright (c) 2019-2021, Intel Corporation
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
//! \file     decode_cp_bitstream.h
//! \brief    Defines the common interface for decode cp bitstream
//! \details  Defines the interface to handle the decode cp bitstream
//!

#ifndef __DECODE_CP_BITSTREAM_H__
#define __DECODE_CP_BITSTREAM_H__

#include "decode_packet_id.h"
#include "decode_resource_array.h"
#include "decode_sub_pipeline.h"
#include "codec_def_decode.h"
#include "media_feature_manager.h"
#include "decodecp_interface.h"
#include "decode_basic_feature.h"
#include "decode_huc_copy_packet_itf.h"

#define STREAM_OUT_BUFFER_NUM 16

namespace decode
{
class DecodeStreamOut : public DecodeSubPipeline
{
public:
    //!
    //! \brief  Decode input bitstream constructor
    //!
    DecodeStreamOut(DecodePipeline *pipeline, MediaTask *task, uint8_t numVdbox);

    //!
    //! \brief  Decode input bitstream destructor
    //!
    virtual ~DecodeStreamOut();

    //!
    //! \brief  Initialize the bitstream context
    //!
    //! \param  [in] settings
    //!         Reference to the Codechal settings
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    virtual MOS_STATUS Init(CodechalSetting &settings) override;

    //!
    //! \brief  Prepare interal parameters
    //! \param  [in] params
    //!         Reference to decode pipeline parameters
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    virtual MOS_STATUS Prepare(DecodePipelineParams &params) override;

    //!
    //! \brief  Get media function for context switch
    //! \return MediaFunction
    //!         Return the media function
    //!
    MediaFunction GetMediaFunction() override;

protected:
    //!
    //! \brief  Reset the bitstream context for each frame
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS Begin();

    //!
    //! \brief  Append the new bits
    //! \param  [in] decodeParams
    //!         Decode parameters
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS Append(const CodechalDecodeParams &decodeParams);

    //!
    //! \brief  Add new segment to segment list
    //! \param  [in] resource
    //!         Resource of current segment
    //! \param  [in] size
    //!         Size of current segment
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    void AddNewSegment(MOS_RESOURCE &resource, uint32_t size);

    //!
    //! \brief  Initialize scalability parameters
    //!
    virtual void InitScalabilityPars(PMOS_INTERFACE osInterface) override;

    //!
    //! \brief  Allocate resource that StreamOut needed
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS AllocateStreamOutResource();

protected:
    DecodeBasicFeature  *m_basicFeature = nullptr;  //!< Decode basic feature
    DecodeAllocator     *m_allocator    = nullptr;  //!< Resource allocator

    MOS_BUFFER          m_streamInBuf;              //!< Decode input bitstream

    HucCopyPktItf       *m_streamOutPkt     = nullptr;  //!< Bitstream streamOut packet
    BufferArray         *m_streamOutBufArr  = nullptr;  //!< streamOut's dest Resource
    PMOS_BUFFER         m_streamOutBuf      = nullptr;
    DecodeCpInterface   *m_decodecp         = nullptr;

MEDIA_CLASS_DEFINE_END(decode__DecodeStreamOut)
};
}  // namespace decode
#endif
