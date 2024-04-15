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
//! \file     decode_cp_bitstream_m12.cpp
//! \brief    Defines the common interface for decode cps bitstream
//! \details  Defines the interface to handle the decode cp input bitstream .
//!
#include "decode_cp_bitstream_m12.h"
#include "decode_basic_feature.h"
#include "decode_pipeline.h"
#include "cp_streamout_interface.h"
#include "decode_huc_packet_creator_g12.h"

namespace decode {

DecodeStreamOutM12::DecodeStreamOutM12(DecodePipeline *pipeline, MediaTask *task, uint8_t numVdbox, CodechalHwInterface *hwInterface)
    : DecodeStreamOut(pipeline, task, numVdbox)
{
    m_decodecp = pipeline->GetDecodeCp();
    MOS_ZeroMemory(&m_streamInBuf, sizeof(m_streamInBuf));
    m_hwInterface = hwInterface;
}

MOS_STATUS DecodeStreamOutM12::Init(CodechalSetting &settings)
{
    DECODE_CHK_NULL(m_pipeline);

    DECODE_CHK_NULL(m_hwInterface);
    PMOS_INTERFACE osInterface = m_hwInterface->GetOsInterface();
    DECODE_CHK_NULL(osInterface);
    InitScalabilityPars(osInterface);

    m_allocator = m_pipeline->GetDecodeAllocator();
    DECODE_CHK_NULL(m_allocator);

    MediaFeatureManager* featureManager = m_pipeline->GetFeatureManager();
    DECODE_CHK_NULL(featureManager);
    m_basicFeature = dynamic_cast<DecodeBasicFeature*>(featureManager->GetFeature(FeatureIDs::basicFeature));
    DECODE_CHK_NULL(m_basicFeature);

    HucPacketCreatorG12 *hucPktCreator = dynamic_cast<HucPacketCreatorG12 *>(m_pipeline);
    DECODE_CHK_NULL(hucPktCreator);
    m_streamOutPkt = hucPktCreator->CreateStreamOutInterface(m_pipeline, m_task, m_hwInterface);
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

}
