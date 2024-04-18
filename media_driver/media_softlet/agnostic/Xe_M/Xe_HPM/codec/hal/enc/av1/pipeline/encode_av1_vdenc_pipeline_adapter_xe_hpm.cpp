/*
* Copyright (c) 2019, Intel Corporation
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
//! \file     encode_av1_vdenc_pipeline_adapter_xe_hpm.cpp
//! \brief    Defines the interface to adapt to av1 vdenc encode pipeline Xe_HPM
//!

#include "encode_av1_vdenc_pipeline_adapter_xe_hpm.h"

EncodeAv1VdencPipelineAdapterXe_Hpm::EncodeAv1VdencPipelineAdapterXe_Hpm(
    CodechalHwInterfaceNext     *hwInterface,
    CodechalDebugInterface  *debugInterface)
    : EncodeAv1VdencPipelineAdapterXe_M_Base(hwInterface, debugInterface)
{
}

MOS_STATUS EncodeAv1VdencPipelineAdapterXe_Hpm::Allocate(CodechalSetting *codecHalSettings)
{
    ENCODE_FUNC_CALL();

    m_encoder = std::make_shared<encode::Av1VdencPipelineXe_Hpm>(m_hwInterface, m_debugInterface);
    ENCODE_CHK_NULL_RETURN(m_encoder);

    return m_encoder->Init(codecHalSettings);
}
MOS_STATUS EncodeAv1VdencPipelineAdapterXe_Hpm::ResolveMetaData(PMOS_RESOURCE pInput, PMOS_RESOURCE pOutput)
{
    return m_encoder->ExecuteResolveMetaData(pInput, pOutput);
}
