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
//! \file     decode_cp_bitstream.cpp
//! \brief    Defines the common interface for decode cps bitstream
//! \details  Defines the interface to handle the decode cp input bitstream .
//!
#include "decode_cp_bitstream.h"
#include "decode_basic_feature.h"
#include "decode_pipeline.h"
#include "cp_streamout_interface.h"
#include "decode_huc_packet_creator_base.h"

namespace decode {

DecodeStreamOut::DecodeStreamOut(DecodePipeline *pipeline, MediaTask *task, uint8_t numVdbox)
    : DecodeSubPipeline(pipeline, task, numVdbox)
{
    m_decodecp = pipeline->GetDecodeCp();
    MOS_ZeroMemory(&m_streamInBuf, sizeof(m_streamInBuf));
}

DecodeStreamOut::~DecodeStreamOut()
{
    if (m_allocator)
    {
        m_allocator->Destroy(m_streamOutBufArr);
    }
}

MOS_STATUS DecodeStreamOut::Init(CodechalSetting& settings)
{
    DECODE_CHK_NULL(m_pipeline);

    CodechalHwInterfaceNext* hwInterface = m_pipeline->GetHwInterface();
    DECODE_CHK_NULL(hwInterface);
    PMOS_INTERFACE osInterface = hwInterface->GetOsInterface();
    DECODE_CHK_NULL(osInterface);
    InitScalabilityPars(osInterface);

    m_allocator = m_pipeline->GetDecodeAllocator();
    DECODE_CHK_NULL(m_allocator);

    MediaFeatureManager* featureManager = m_pipeline->GetFeatureManager();
    DECODE_CHK_NULL(featureManager);
    m_basicFeature = dynamic_cast<DecodeBasicFeature*>(featureManager->GetFeature(FeatureIDs::basicFeature));
    DECODE_CHK_NULL(m_basicFeature);

    HucPacketCreatorBase *hucPktCreator = dynamic_cast<HucPacketCreatorBase *>(m_pipeline);
    DECODE_CHK_NULL(hucPktCreator);
    m_streamOutPkt = hucPktCreator->CreateStreamOutInterface(m_pipeline, m_task, hwInterface);
    DECODE_CHK_NULL(m_streamOutPkt);

    MediaPacket *packet = dynamic_cast<MediaPacket *>(m_streamOutPkt);
    if (packet)
    {
        DECODE_CHK_STATUS(RegisterPacket(DecodePacketId(m_pipeline, hucCpStreamOutPacketId), *packet));
        DECODE_CHK_STATUS(packet->Init());
    }

    MOS_ZeroMemory(&m_streamInBuf, sizeof(m_streamInBuf));

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS DecodeStreamOut::Prepare(DecodePipelineParams& params)
{
    if (!m_decodecp)
    {
        return MOS_STATUS_SUCCESS;
    }
    if ((params.m_pipeMode != decodePipeModeProcess
        || !m_pipeline->IsCompleteBitstream()  // if codec bitstream is not ready
        || !m_decodecp->IsStreamOutNeeded()))  //if hucStreamOut not needed
    {
        return MOS_STATUS_SUCCESS;
    }

    if (!m_decodecp->IsStreamOutEnabled())
    {
        return MOS_STATUS_PLATFORM_NOT_SUPPORTED;
    }

    DECODE_CHK_NULL(params.m_params);
    CodechalDecodeParams *decodeParams = params.m_params;

    DECODE_CHK_STATUS(Begin()); // reset activePkts
    DECODE_CHK_STATUS(Append(*decodeParams));

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS DecodeStreamOut::Begin()
{
    DECODE_CHK_STATUS(DecodeSubPipeline::Reset());
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS DecodeStreamOut::AllocateStreamOutResource()
{
    DECODE_CHK_NULL(m_allocator);

    uint32_t allocSize = MOS_ALIGN_CEIL(m_basicFeature->m_dataSize, CODECHAL_CACHELINE_SIZE);

    if (m_streamOutBufArr == nullptr)
    {
        m_streamOutBufArr = m_allocator->AllocateBufferArray(
            allocSize, "streamoutStream", STREAM_OUT_BUFFER_NUM, resourceInternalReadWriteCache, notLockableVideoMem);
        DECODE_CHK_NULL(m_streamOutBufArr);
        auto& streamOutBuf = m_streamOutBufArr->Fetch();
        DECODE_CHK_NULL(streamOutBuf);
    }
    else
    {
        auto& streamOutBuf = m_streamOutBufArr->Fetch();
        DECODE_CHK_NULL(streamOutBuf);
        DECODE_CHK_STATUS(m_allocator->Resize(streamOutBuf, allocSize, notLockableVideoMem));
    }
    m_streamOutBuf = m_streamOutBufArr->Peek();

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS DecodeStreamOut::Append(const CodechalDecodeParams &decodeParams)
{
    m_streamInBuf = m_basicFeature->m_resDataBuffer;
    DECODE_CHK_STATUS(AllocateStreamOutResource());
    m_basicFeature->m_resDataBuffer = *m_streamOutBuf;
    m_basicFeature->m_dataOffset = 0;
    DECODE_CHK_STATUS(ActivatePacket(DecodePacketId(m_pipeline, hucCpStreamOutPacketId), true, 0, 0));
    AddNewSegment(m_streamInBuf.OsResource, m_basicFeature->m_dataSize);

    return MOS_STATUS_SUCCESS;
}

void DecodeStreamOut::AddNewSegment(MOS_RESOURCE &resource, uint32_t size)
{
    HucCopyPktItf::HucCopyParams copyParams;
    copyParams.srcBuffer    = &resource;
    copyParams.srcOffset    = 0;
    copyParams.destBuffer   = &(m_streamOutBuf->OsResource);
    copyParams.destOffset   = 0;
    copyParams.copyLength   = size;
    if(m_streamOutPkt)
    {
        m_streamOutPkt->PushCopyParams(copyParams);
    }
}

MediaFunction DecodeStreamOut::GetMediaFunction()
{
    return VdboxDecodeWaFunc;
}

void DecodeStreamOut::InitScalabilityPars(PMOS_INTERFACE osInterface)
{
    m_decodeScalabilityPars.disableScalability = true;
    m_decodeScalabilityPars.disableRealTile = true;
    m_decodeScalabilityPars.enableVE = MOS_VE_SUPPORTED(osInterface);
    m_decodeScalabilityPars.numVdbox = m_numVdbox;
}

}
