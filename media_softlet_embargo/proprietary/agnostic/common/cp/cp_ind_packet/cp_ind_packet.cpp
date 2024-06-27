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
//! \file     cp_ind_packet.cpp
//! \brief    implement the functions for cp indirect state packet
//!

#include "cp_ind_packet.h"
#include "decode_pipeline.h"
#include "decodecp.h"


namespace decode
{
CpIndPkt::~CpIndPkt()
{
    if(m_cpIndStateBufArr)
    {
        m_allocator->Destroy(m_cpIndStateBufArr);
    }

    if (m_sliceHeaderSize)
    {
        MOS_FreeMemory(m_sliceHeaderSize);
        m_sliceHeaderSize = nullptr;
    }

    if (m_sliceDataSize)
    {
        MOS_FreeMemory(m_sliceDataSize);
        m_sliceDataSize = nullptr;
    }

    if (m_bsdDataStartOffset)
    {
        MOS_FreeMemory(m_bsdDataStartOffset);
        m_bsdDataStartOffset = nullptr;
    }
}

MOS_STATUS CpIndPkt::AllocateIndirectResource(uint32_t sliceIndex)
{
    DECODE_FUNC_CALL();

    DECODE_CP_CHK_NULL_RETURN(m_allocator);
    auto decodecp         = dynamic_cast<DecodeCp *>(m_decodecp);
    DECODE_CP_CHK_NULL_RETURN(decodecp);
    uint32_t stateBufSize = decodecp->GetIndStateSize(sliceIndex);

    if (m_cpIndStateBufArr == nullptr)
    {
        m_cpIndStateBufArr = m_allocator->AllocateBufferArray(
            stateBufSize, "CpIndState", m_indStateBufferNum, resourceInternalReadWriteCache, lockableVideoMem);
        DECODE_CP_CHK_NULL_RETURN(m_cpIndStateBufArr);
        auto &IndStateBuf = m_cpIndStateBufArr->Fetch();
        DECODE_CP_CHK_NULL_RETURN(IndStateBuf);
    }
    else
    {
        if (m_indStateBufferNum > m_cpIndStateBufArr->ArraySize())
        {
            uint32_t numberOfExtraBuffer = m_indStateBufferNum - m_cpIndStateBufArr->ArraySize();
            for (uint32_t i = 0; i < numberOfExtraBuffer; i++)
            {
                MOS_BUFFER *buf = m_allocator->AllocateBuffer(stateBufSize, "CpIndState", resourceInternalReadWriteCache, lockableVideoMem);
                DECODE_CP_CHK_NULL_RETURN(buf);
                m_cpIndStateBufArr->Push(buf);
            }
        }
        auto &IndStateBuf = m_cpIndStateBufArr->Fetch();
        DECODE_CP_CHK_NULL_RETURN(IndStateBuf);
        DECODE_CP_CHK_STATUS(m_allocator->Resize(IndStateBuf, stateBufSize, lockableVideoMem));
    }
    m_cpIndStateBuf = m_cpIndStateBufArr->Peek();
    return MOS_STATUS_SUCCESS;
}
MOS_STATUS CpIndPkt::Init()
{
    DECODE_FUNC_CALL();

    DECODE_CHK_NULL(m_featureManager);
    DECODE_CHK_NULL(m_osInterface);
    DECODE_CHK_NULL(m_pipeline);

    m_basicFeature = dynamic_cast<DecodeBasicFeature *>(m_featureManager->GetFeature(FeatureIDs::basicFeature));
    DECODE_CHK_NULL(m_basicFeature);
    m_allocator    = m_pipeline->GetDecodeAllocator();
    m_decodecp     = m_pipeline->GetDecodeCp();
    DECODE_CHK_NULL(m_allocator);

    return MOS_STATUS_SUCCESS;
}
MOS_STATUS CpIndPkt::Prepare()
{
    DECODE_FUNC_CALL();
    auto          decodecp        = dynamic_cast<DecodeCp *>(m_decodecp);
    DECODE_CHK_NULL(decodecp);
    DECODE_CHK_NULL(m_basicFeature);
    PMOS_RESOURCE bitstreamBuffer = &(m_basicFeature->m_resDataBuffer.OsResource);
    DECODE_CHK_NULL(bitstreamBuffer);
    if (!decodecp->IsAesIndSubPacketNeeded())
        return MOS_STATUS_SUCCESS;
    DECODE_CP_CHK_STATUS(PrepareSliceInfo());
    DECODE_CP_CHK_STATUS(decodecp->AllocateDecodeResource());
    DECODE_CP_CHK_STATUS(decodecp->CalculateSliceSegmentInfo(bitstreamBuffer));
    DECODE_CP_CHK_STATUS(decodecp->UpdateSliceSegmentInfo());
    m_indStateBufferNum = decodecp->GetIndStateNum();
    DECODE_CP_CHK_COND_RETURN(m_indStateBufferNum == 0, "Indirect state buffer number can't be zero!");
    
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS CpIndPkt::CalculateCommandSize(
    uint32_t &commandBufferSize,
    uint32_t &requestedPatchListSize)
{
    // This function is not really called due to current decode slice level code already added cp command size. 
    // TODO implement this function if needed.
    DECODE_FUNC_CALL();
    return MOS_STATUS_SUCCESS;
}

}  // namespace decode
