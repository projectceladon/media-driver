/*
* Copyright (c) 2022, Intel Corporation
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
//! \file     cp_ind_packet.h.h
//! \brief    Defines the interface of cp indirect state packet
//!

#ifndef __CODECHAL_CP_IND_PACKET_H__
#define __CODECHAL_CP_IND_PACKET_H__

#include "decode_basic_feature.h"
#include "media_pipeline.h"
#include "decode_resource_array.h"
#include "decode_sub_packet.h"
#include "mhw_cp.h"

namespace decode
{

#define ENCRYPTION_CBC_BLOCK_SIZE 16

class CpIndPkt : public DecodeSubPacket
{
public:
    CpIndPkt(DecodePipeline *pipeline, CodechalHwInterfaceNext *hwInterface)
        : DecodeSubPacket(pipeline, hwInterface)
    {
        if (m_hwInterface != nullptr)
        {
            m_cpInterface = hwInterface->GetCpInterface();
            m_mhwCp       = dynamic_cast<MhwCpBase *>(hwInterface->GetCpInterface());
        }
    }

    virtual ~CpIndPkt();

    //!
    //! \brief  Initialize the media packet, allocate required resources
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    virtual MOS_STATUS Init() override;

    //!
    //! \brief  Prepare interal parameters, should be invoked for each frame
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    virtual MOS_STATUS Prepare() override;

    //!
    //! \brief  Execute indirect state packet
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    virtual MOS_STATUS Execute(
        MOS_COMMAND_BUFFER                 &cmdBuffer,
        uint32_t                           sliceIdx) = 0;

    //!
    //! \brief  Get Segment Number for Each Slice
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    virtual MOS_STATUS CalculateSliceSegmentNum() = 0;

    //!
    //! \brief  Update Slice Info From Decode Feature
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    virtual MOS_STATUS PrepareSliceInfo() = 0;

    //!
    //! \brief  Calculate Command Size
    //!
    //! \param  [in, out] commandBufferSize
    //!         requested size
    //! \param  [in, out] requestedPatchListSize
    //!         requested size
    //! \return MOS_STATUS
    //!         status
    //!
    MOS_STATUS CalculateCommandSize(
        uint32_t &commandBufferSize,
        uint32_t &requestedPatchListSize) override;

protected:
    MOS_STATUS AllocateIndirectResource(uint32_t sliceIndex);

protected:
    BufferArray *           m_cpIndStateBufArr   = nullptr;  //! IndState buffer array
    PMOS_BUFFER             m_cpIndStateBuf      = nullptr;
    DecodeAllocator *       m_allocator          = nullptr;
    DecodeBasicFeature *    m_basicFeature       = nullptr;
    MhwCpInterface *        m_cpInterface        = nullptr;
    MhwCpBase *             m_mhwCp              = nullptr;
    uint32_t *              m_sliceSegmentNum    = nullptr;  //! Segment number per Slice
    uint32_t               *m_sliceHeaderSize    = nullptr;
    uint32_t               *m_sliceDataSize      = nullptr;
    uint32_t               *m_bsdDataStartOffset = nullptr;
    uint32_t                m_slicesNum          = 0;
    uint32_t                m_indStateBufferNum  = 0;

MEDIA_CLASS_DEFINE_END(decode__CpIndPkt)
};

typedef CpFactory<CpIndPkt, DecodePipeline *, CodechalHwInterfaceNext *> CpIndPktFactory;

}  // namespace decode

#endif
