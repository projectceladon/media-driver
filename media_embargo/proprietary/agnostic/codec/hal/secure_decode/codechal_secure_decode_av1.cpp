/*
// Copyright (C) 2019-2020 Intel Corporation
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
//! \file     codechal_secure_decode_av1.cpp
//! \brief    Impelements the public interface for CodecHal Secure Decode 
//!

#include "codechal_decoder.h"
#include "codechal_secure_decode_av1.h"

CodechalSecureDecodeAv1::CodechalSecureDecodeAv1(CodechalHwInterface *hwInterface):
    CodechalSecureDecode(hwInterface),
    m_uiTempCdfBufIndex(0)
{
    for (uint32_t i = 0; i < av1DefaultCdfTableNum; i++)
    {
        MOS_ZeroMemory(&m_resTempCdfTableBuf[i], sizeof(MOS_RESOURCE));
    }
}

CodechalSecureDecodeAv1::~CodechalSecureDecodeAv1()
{
    for (uint32_t i = 0; i < av1DefaultCdfTableNum; i++)
    {
        if (!Mos_ResourceIsNull(&m_resTempCdfTableBuf[i]))
        {
            osInterface->pfnFreeResource(osInterface, &m_resTempCdfTableBuf[i]);
        }
    }
}

MOS_STATUS CodechalSecureDecodeAv1::AllocateResource(void *state)
{
    CODECHAL_DECODE_FUNCTION_ENTER;

    CODECHAL_DECODE_CHK_NULL_RETURN(state);
    CodechalDecodeAv1State* pAv1State = (CodechalDecodeAv1State*)state;
    MOS_STATUS              eStatus   = MOS_STATUS_SUCCESS;

    eStatus = AllocateStreamOutResource(
        pAv1State->m_copyDataBufferInUse ? pAv1State->m_copyDataBufferSize : pAv1State->m_dataSize);

    if (isStreamoutNeeded)
    {
        CODECHAL_DECODE_CHK_NULL_RETURN(osInterface);

        MOS_ALLOC_GFXRES_PARAMS allocParams;
        MOS_ZeroMemory(&allocParams, sizeof(MOS_ALLOC_GFXRES_PARAMS));
        allocParams.Type     = MOS_GFXRES_BUFFER;
        allocParams.TileType = MOS_TILE_LINEAR;
        allocParams.Format   = Format_Buffer;
        allocParams.dwBytes  = MOS_ALIGN_CEIL(pAv1State->m_cdfMaxNumBytes, CODECHAL_PAGE_SIZE);
        allocParams.pBufName = "Av1CdfTablesTempBuffer";

        for (uint32_t i = 0; i < av1DefaultCdfTableNum; i++)
        {
            CODECHAL_DECODE_CHK_STATUS_MESSAGE_RETURN(osInterface->pfnAllocateResource(
                osInterface,
                &allocParams,
                &m_resTempCdfTableBuf[i]),
                "Failed to allocate %s.",
                allocParams.pBufName);

            CodechalResLock cdfTableLock(osInterface, &m_resTempCdfTableBuf[i]);
            auto data = (uint8_t *)cdfTableLock.Lock(CodechalResLock::writeOnly);
            CODECHAL_DECODE_CHK_NULL_RETURN(data);
            memset(data, 0, pAv1State->m_cdfMaxNumBytes);
        }
    }

    return eStatus;
}

MOS_STATUS CodechalSecureDecodeAv1::Execute(void *state)
{
    CodechalDecodeAv1State          *pAv1State          = nullptr;
    MhwMiInterface*                 pCommonMiInterface  = nullptr;
    PMHW_PAVP_ENCRYPTION_PARAMS     pEncryptionParams   = nullptr;
    PMOS_CMD_BUF_ATTRI_VE           pAttriVe            = nullptr;
    MOS_STATUS                      eStatus             = MOS_STATUS_SUCCESS;
    MhwCpBase                       *mhwCp              = nullptr;
    bool                            bRenderingFlags     = false;
    MOS_COMMAND_BUFFER              CmdBuffer;

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

    pAv1State           = (CodechalDecodeAv1State*)state;
    pCommonMiInterface  = hwInterface->GetMiInterface();
    pEncryptionParams   = mhwCp->GetEncryptionParams();
    bRenderingFlags     = pAv1State->GetVideoContextUsesNullHw();;

    CodechalHucStreamoutParams HucStreamOutParams;
    MOS_ZeroMemory(&HucStreamOutParams, sizeof(HucStreamOutParams));
    HucStreamOutParams.mode                       = CODECHAL_DECODE_MODE_AV1VLD;
    HucStreamOutParams.dataBuffer                 = pAv1State->m_copyDataBufferInUse ? &pAv1State->m_copyDataBuffer : &pAv1State->m_dataBuffer;
    HucStreamOutParams.dataSize                   = pAv1State->m_copyDataBufferInUse ? pAv1State->m_copyDataBufferSize : pAv1State->m_dataSize;
    HucStreamOutParams.dataOffset                 = 0;
    HucStreamOutParams.streamOutObjectBuffer      = &hucStreamOutBuf[hucStreamOutBufIndex];
    HucStreamOutParams.streamOutObjectSize        = hucStreamOutBufSize[hucStreamOutBufIndex];
    HucStreamOutParams.streamOutObjectOffset      = 0;
    HucStreamOutParams.indStreamInLength          = pAv1State->m_copyDataBufferInUse ? pAv1State->m_copyDataBufferSize : pAv1State->m_dataSize;
    HucStreamOutParams.inputRelativeOffset        = 0;
    HucStreamOutParams.outputRelativeOffset       = 0;

    CODECHAL_DECODE_CHK_STATUS_RETURN(GetSegmentInfo(
        hwInterface->GetCpInterface(),
        HucStreamOutParams.dataBuffer));

    HucStreamOutParams.hucIndState = hucIndCryptoStateBuf[hucIndCryptoStateBufIndex];
    hucIndCryptoStateBufIndex      = (hucIndCryptoStateBufIndex + 1) % INDIRECT_CRYPTO_STATE_BUFFER_NUM;

    HucStreamOutParams.curIndEntriesNum   = 0;
    // On stripe mode the segments are remapped, so use remapped segment number for stream out
    HucStreamOutParams.curNumSegments     = (pEncryptionParams->dwBlocksStripeEncCount == 0) ?
        pEncryptionParams->dwSubSampleCount : sampleRemappingBlockNum;
    HucStreamOutParams.segmentInfo        = (void *)segmentInfo;

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

    if (MOS_VE_SUPPORTED(osInterface) && CmdBuffer.Attributes.pAttriVe)
    {
        pAttriVe = (PMOS_CMD_BUF_ATTRI_VE)CmdBuffer.Attributes.pAttriVe;
        pAttriVe->bUseVirtualEngineHint = true;
        MOS_ZeroMemory(&pAttriVe->VEngineHintParams, sizeof(pAttriVe->VEngineHintParams));
        pAttriVe->VEngineHintParams.NeedSyncWithPrevious = true;
    }

    CODECHAL_DECODE_CHK_STATUS_RETURN(osInterface->pfnSubmitCommandBuffer(osInterface, &CmdBuffer, bRenderingFlags));

    // The bitstream is clear now, don't need PAVP flag anymore.
    mhwCp->SetHostEncryptMode(CP_TYPE_NONE);

finish:
    return eStatus;
}

MOS_STATUS CodechalSecureDecodeAv1::SetBitstreamBuffer(MHW_VDBOX_IND_OBJ_BASE_ADDR_PARAMS *indObjBaseAddrParams)
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

MOS_STATUS CodechalSecureDecodeAv1::ProtectDefaultCdfTableBuffer(
    void *state,
    uint32_t bufIndex,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    MOS_STATUS                      eStatus             = MOS_STATUS_SUCCESS;
    CodechalDecodeAv1State*         pAv1State           = nullptr;
    MhwMiInterface*                 pCommonMiInterface  = nullptr;
    CodechalHucStreamoutParams      HucStreamOutParams;
    MHW_MI_FLUSH_DW_PARAMS          FlushDwParams;

    if (!isStreamoutNeeded)
    {
        return eStatus;
    }

    CODECHAL_DECODE_FUNCTION_ENTER;
    CODECHAL_DECODE_CHK_NULL_RETURN(state);
    CODECHAL_DECODE_CHK_NULL_RETURN(cmdBuffer);

    pAv1State           = (CodechalDecodeAv1State*)state;
    pCommonMiInterface  = hwInterface->GetMiInterface();

    osInterface->pfnSetPerfTag(
        osInterface,
        (uint16_t)(((CODECHAL_DECODE_MODE_AV1VLD << 4) & 0xF0) | COPY_TYPE));
    osInterface->pfnResetPerfBufferID(osInterface);

    MOS_ZeroMemory(&HucStreamOutParams, sizeof(HucStreamOutParams));

    // Ind Obj Addr command
    HucStreamOutParams.dataBuffer                 = &m_resTempCdfTableBuf[m_uiTempCdfBufIndex - 1];
    HucStreamOutParams.dataSize                   = pAv1State->m_cdfMaxNumBytes;
    HucStreamOutParams.dataOffset                 = 0;
    HucStreamOutParams.streamOutObjectBuffer      = &pAv1State->m_cdfTablesBuffer[pAv1State->m_defaultCdfbufIdx + bufIndex - 3];
    HucStreamOutParams.streamOutObjectSize        = pAv1State->m_cdfMaxNumBytes;
    HucStreamOutParams.streamOutObjectOffset      = 0;

    // Stream object params
    HucStreamOutParams.indStreamInLength          = pAv1State->m_cdfMaxNumBytes;
    HucStreamOutParams.inputRelativeOffset        = 0;
    HucStreamOutParams.outputRelativeOffset       = 0;

    CODECHAL_DECODE_CHK_STATUS_RETURN(PerformHucStreamOut(
        &HucStreamOutParams,
        cmdBuffer));

    MOS_ZeroMemory(&FlushDwParams, sizeof(FlushDwParams));
    CODECHAL_DECODE_CHK_STATUS_RETURN(pCommonMiInterface->AddMiFlushDwCmd(
        cmdBuffer,
        &FlushDwParams));

    return eStatus;
}