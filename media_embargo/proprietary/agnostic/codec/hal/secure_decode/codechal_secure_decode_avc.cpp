/*
// Copyright (C) 2017-2020 Intel Corporation
//
// Licensed under the Apache License,Version 2.0(the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
//!
//! \file     codechal_secure_decode_avc.cpp
//! \brief    Impelements the public interface for CodecHal Secure Decode 
//!

#include "codechal_secure_decode_avc.h"

CodechalSecureDecodeAvc::CodechalSecureDecodeAvc(CodechalHwInterface *hwInterface):CodechalSecureDecode(hwInterface)
{
}

CodechalSecureDecodeAvc::~CodechalSecureDecodeAvc()
{
}

MOS_STATUS CodechalSecureDecodeAvc::AllocateResource(void *state)
{
    CODECHAL_DECODE_FUNCTION_ENTER;

    CODECHAL_DECODE_CHK_NULL_RETURN(state);
    PCODECHAL_DECODE_AVC_STATE pAvcState = (PCODECHAL_DECODE_AVC_STATE)state;

    return AllocateStreamOutResource(pAvcState->m_dataSize);
}

MOS_STATUS CodechalSecureDecodeAvc::Execute(void *state)
{
    PCODECHAL_DECODE_AVC_STATE      pAvcState          = nullptr;
    MhwMiInterface*                 pCommonMiInterface = nullptr;
    PMHW_PAVP_ENCRYPTION_PARAMS     pEncryptionParams  = nullptr;
    bool                            bRenderingFlags    = false;
    MOS_COMMAND_BUFFER              CmdBuffer;
    MOS_STATUS                      eStatus = MOS_STATUS_SUCCESS;
    MhwCpBase                       *mhwCp = nullptr;

    if (!isStreamoutNeeded)
    {
        return MOS_STATUS_SUCCESS;
    }

    CODECHAL_DECODE_FUNCTION_ENTER;

    CODECHAL_DECODE_CHK_NULL(state);
    CODECHAL_DECODE_CHK_NULL(hwInterface);
    CODECHAL_DECODE_CHK_NULL(hwInterface->GetCpInterface());
    mhwCp = dynamic_cast<MhwCpBase*>(hwInterface->GetCpInterface());
    CODECHAL_DECODE_CHK_NULL(mhwCp);

    pAvcState          = (PCODECHAL_DECODE_AVC_STATE)state;
    pCommonMiInterface = hwInterface->GetMiInterface();
    pEncryptionParams  = mhwCp->GetEncryptionParams();
    bRenderingFlags    = pAvcState->GetVideoContextUsesNullHw();;

    CodechalHucStreamoutParams HucStreamOutParams;
    MOS_ZeroMemory(&HucStreamOutParams, sizeof(HucStreamOutParams));
    HucStreamOutParams.mode                       = CODECHAL_DECODE_MODE_AVCVLD;
    HucStreamOutParams.dataBuffer                 = &pAvcState->m_resDataBuffer;
    HucStreamOutParams.dataSize                   = pAvcState->m_dataSize;
    HucStreamOutParams.dataOffset                 = 0;
    HucStreamOutParams.streamOutObjectBuffer      = &hucStreamOutBuf[hucStreamOutBufIndex];
    HucStreamOutParams.streamOutObjectSize        = hucStreamOutBufSize[hucStreamOutBufIndex];
    HucStreamOutParams.streamOutObjectOffset      = hucStreamOutBufOffset;
    HucStreamOutParams.indStreamInLength          = pAvcState->m_dataSize;
    HucStreamOutParams.inputRelativeOffset        = 0;
    HucStreamOutParams.outputRelativeOffset       = hucStreamOutBufOffset;

    CODECHAL_DECODE_CHK_STATUS_RETURN(GetSegmentInfo(
        hwInterface->GetCpInterface(),
        HucStreamOutParams.dataBuffer));

    HucStreamOutParams.hucIndState = hucIndCryptoStateBuf[hucIndCryptoStateBufIndex];
    hucIndCryptoStateBufIndex      = (hucIndCryptoStateBufIndex + 1) % INDIRECT_CRYPTO_STATE_BUFFER_NUM;

    HucStreamOutParams.curIndEntriesNum  = 0;
    // On stripe mode the segments are remapped, so use remapped segment number for stream out
    HucStreamOutParams.curNumSegments    = (pEncryptionParams->dwBlocksStripeEncCount == 0) ?
        pEncryptionParams->dwSubSampleCount : sampleRemappingBlockNum;
    HucStreamOutParams.segmentInfo       = (void *)segmentInfo;

    CODECHAL_DECODE_CHK_STATUS_RETURN(osInterface->pfnGetCommandBuffer(osInterface, &CmdBuffer, 0));

    // Send command buffer header at the beginning (OS dependent)
    MHW_GENERIC_PROLOG_PARAMS GenericPrologParams;
    MOS_ZeroMemory(&GenericPrologParams, sizeof(GenericPrologParams));
    GenericPrologParams.pOsInterface        = osInterface;
    GenericPrologParams.pvMiInterface       = hwInterface->GetMiInterface();
    GenericPrologParams.bMmcEnabled         = m_mmcEnabled;
    CODECHAL_DECODE_CHK_STATUS_RETURN(Mhw_SendGenericPrologCmd(
        &CmdBuffer,
        &GenericPrologParams));

    CODECHAL_DECODE_CHK_STATUS_RETURN(PerformHucStreamOut(
        &HucStreamOutParams,
        &CmdBuffer));

    MHW_MI_FLUSH_DW_PARAMS FlushDwParams;
    MOS_ZeroMemory(&FlushDwParams, sizeof(FlushDwParams));
    CODECHAL_DECODE_CHK_STATUS_RETURN(pCommonMiInterface->AddMiFlushDwCmd(
        &CmdBuffer,
        &FlushDwParams));

    CODECHAL_DECODE_CHK_STATUS_RETURN(pCommonMiInterface->AddMiBatchBufferEnd(
        &CmdBuffer,
        nullptr));

    osInterface->pfnReturnCommandBuffer(osInterface, &CmdBuffer, 0);

    CODECHAL_DECODE_CHK_STATUS_RETURN(osInterface->pfnSubmitCommandBuffer(osInterface, &CmdBuffer, bRenderingFlags));

    // The bitstream is clear now, don't need PAVP flag anymore.
    mhwCp->SetHostEncryptMode(CP_TYPE_NONE);

finish:
    return eStatus;
}

MOS_STATUS CodechalSecureDecodeAvc::SetBitstreamBuffer(MHW_VDBOX_IND_OBJ_BASE_ADDR_PARAMS *indObjBaseAddrParams)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    if (!isStreamoutNeeded)
    {
        return MOS_STATUS_SUCCESS;
    }

    CODECHAL_DECODE_FUNCTION_ENTER;

    indObjBaseAddrParams->presDataBuffer = &hucStreamOutBuf[hucStreamOutBufIndex];
    indObjBaseAddrParams->dwDataSize     = hucStreamOutBufSize[hucStreamOutBufIndex];

    return eStatus;
}

