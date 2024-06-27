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
//! \file     codechal_secure_decode_vp9.cpp
//! \brief    Impelements the public interface for CodecHal Secure Decode 
//!

#include "codechal_decoder.h"
#include "codechal_secure_decode_vp9.h"
#include "codechal_decode_singlepipe_virtualengine.h"

#define VDBOX_HUC_VP9_PAVP_DECODE_KERNEL_DESCRIPTOR 6

CodechalSecureDecodeVp9::CodechalSecureDecodeVp9(CodechalHwInterface* hwInterface) :CodechalSecureDecode(hwInterface)
{
}

CodechalSecureDecodeVp9::~CodechalSecureDecodeVp9()
{
}

MOS_STATUS CodechalSecureDecodeVp9::ResetVP9SegIdBufferWithHuc(
    void                    *state,
    PMOS_COMMAND_BUFFER     cmdBuffer)
{
    CodechalDecodeVp9*              pVp9State          = nullptr;
    MhwMiInterface*                 pCommonMiInterface = nullptr;
    uint32_t                        dwBufSize          = 0;
    MOS_STATUS                      eStatus            = MOS_STATUS_SUCCESS;

    CODECHAL_DECODE_FUNCTION_ENTER;

    CODECHAL_DECODE_CHK_NULL(state);
    CODECHAL_DECODE_CHK_NULL(cmdBuffer);

    pVp9State = (CodechalDecodeVp9*)state;
    pCommonMiInterface = hwInterface->GetMiInterface();

    osInterface->pfnSetPerfTag(
        osInterface,
        (uint16_t)(((pVp9State->GetMode() << 4) & 0xF0) | COPY_TYPE));
    osInterface->pfnResetPerfBufferID(osInterface);

    dwBufSize =
        pVp9State->m_allocatedWidthInSb *pVp9State->m_allocatedHeightInSb * CODECHAL_CACHELINE_SIZE; // 16 byte aligned

    CODECHAL_DECODE_CHK_STATUS_RETURN(pVp9State->HucCopy(
        cmdBuffer,                         // pCmdBuffer
        &pVp9State->m_resSegmentIdBuffReset, // presSrc
        &pVp9State->m_resVp9SegmentIdBuffer, // presDst
        dwBufSize,                         // u32CopyLength
        0,                                 // u32CopyInputOffset
        0));                               // u32CopyOutputOffset

    MHW_MI_FLUSH_DW_PARAMS FlushDwParams;
    MOS_ZeroMemory(&FlushDwParams, sizeof(FlushDwParams));
    CODECHAL_DECODE_CHK_STATUS_RETURN(pCommonMiInterface->AddMiFlushDwCmd(
        cmdBuffer,
        &FlushDwParams));

finish:
    return eStatus;
}

MOS_STATUS CodechalSecureDecodeVp9::UpdateVP9ProbBufferWithHuc(
    bool                    isFullUpdate,
    void                    *state,
    PMOS_COMMAND_BUFFER     cmdBuffer)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    CODECHAL_DECODE_FUNCTION_ENTER;

    CODECHAL_DECODE_CHK_NULL(state);
    CODECHAL_DECODE_CHK_NULL(cmdBuffer);

    if (isFullUpdate)
    {
        CODECHAL_DECODE_CHK_STATUS(ProbBufFullUpdate(state, cmdBuffer));
    }
    else
    {
        CODECHAL_DECODE_CHK_STATUS(ProbBufPartialUpdate(state, cmdBuffer));
    }

finish:
    return eStatus;
}

MOS_STATUS CodechalSecureDecodeVp9::AddHcpSecureState(
    PMOS_COMMAND_BUFFER             cmdBuffer,
    void                            *state)
{
    CodechalDecodeVp9*                      pVp9State;
    PCODEC_VP9_PIC_PARAMS                   pVp9PicParams;
    MHW_PAVP_AES_PARAMS                     AesParams;
    MhwCpBase                                *pCpInterface = nullptr;
    MOS_STATUS                              eStatus = MOS_STATUS_SUCCESS;

    CODECHAL_DECODE_CHK_NULL(cmdBuffer);
    CODECHAL_DECODE_CHK_NULL(hwInterface);
    CODECHAL_DECODE_CHK_NULL(hwInterface->GetCpInterface());
    CODECHAL_DECODE_CHK_NULL(state);

    pVp9State = (CodechalDecodeVp9*)state;
    pVp9PicParams = pVp9State->m_vp9PicParams;
    pCpInterface = dynamic_cast<MhwCpBase*>(hwInterface->GetCpInterface());
    CODECHAL_DECODE_CHK_NULL(pCpInterface);
    if (pCpInterface->GetHostEncryptMode() == CP_TYPE_BASIC)
    {
        CODECHAL_DECODE_VERBOSEMESSAGE("AES command CP_TYPE_BASIC enabled, Start Offset %d", pVp9PicParams->UncompressedHeaderLengthInBytes);
        MOS_ZeroMemory(&AesParams, sizeof(AesParams));
        AesParams.presDataBuffer = &pVp9State->m_resDataBuffer;
        AesParams.dwDataLength = pVp9PicParams->BSBytesInBuffer - pVp9PicParams->UncompressedHeaderLengthInBytes;
        AesParams.dwDataStartOffset = pVp9PicParams->UncompressedHeaderLengthInBytes;
        AesParams.bShortFormatInUse = 0;
        AesParams.bLastPass = 0;
        AesParams.dwTotalBytesConsumed = 0;

        CODECHAL_DECODE_CHK_STATUS(pCpInterface->AddHcpAesState(
            true,
            cmdBuffer,
            nullptr,
            &AesParams));
    }

finish:
    return eStatus;
}

MOS_STATUS CodechalSecureDecodeVp9::AllocateResource(void *state)
{
    CODECHAL_DECODE_FUNCTION_ENTER;

    CODECHAL_DECODE_CHK_NULL_RETURN(state);
    CodechalDecodeVp9 *pVp9State = (CodechalDecodeVp9 *)state;

    return AllocateStreamOutResource(pVp9State->m_dataSize);
}

MOS_STATUS CodechalSecureDecodeVp9::Execute(void *state)
{
    CodechalDecodeVp9*              pVp9State = nullptr;
    MhwMiInterface*                 pCommonMiInterface = nullptr;
    PMHW_PAVP_ENCRYPTION_PARAMS     pEncryptionParams = nullptr;
    PMOS_CMD_BUF_ATTRI_VE          pAttriVe = nullptr;
    bool                            bRenderingFlags = false;
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

    pVp9State = (CodechalDecodeVp9*)state;
    pCommonMiInterface = hwInterface->GetMiInterface();

    mhwCp = dynamic_cast<MhwCpBase*>(hwInterface->GetCpInterface());
    CODECHAL_DECODE_CHK_NULL(mhwCp);

    pEncryptionParams = mhwCp->GetEncryptionParams();
    bRenderingFlags = pVp9State->GetVideoContextUsesNullHw();;

    CodechalHucStreamoutParams HucStreamOutParams;
    MOS_ZeroMemory(&HucStreamOutParams, sizeof(HucStreamOutParams));
    HucStreamOutParams.mode = CODECHAL_DECODE_MODE_HEVCVLD;
    HucStreamOutParams.dataBuffer = pVp9State->m_copyDataBufferInUse ? &pVp9State->m_resCopyDataBuffer : &pVp9State->m_resDataBuffer;
    HucStreamOutParams.dataSize = pVp9State->m_copyDataBufferInUse ? pVp9State->m_copyDataBufferSize : pVp9State->m_dataSize;
    HucStreamOutParams.dataOffset = 0;
    HucStreamOutParams.streamOutObjectBuffer = &hucStreamOutBuf[hucStreamOutBufIndex];
    HucStreamOutParams.streamOutObjectSize = hucStreamOutBufSize[hucStreamOutBufIndex];
    HucStreamOutParams.streamOutObjectOffset = 0;
    HucStreamOutParams.indStreamInLength = pVp9State->m_copyDataBufferInUse ? pVp9State->m_copyDataBufferSize : pVp9State->m_dataSize;
    HucStreamOutParams.inputRelativeOffset = 0;
    HucStreamOutParams.outputRelativeOffset = 0;

    CODECHAL_DECODE_CHK_STATUS_RETURN(GetSegmentInfo(
        hwInterface->GetCpInterface(),
        HucStreamOutParams.dataBuffer));

    HucStreamOutParams.hucIndState = hucIndCryptoStateBuf[hucIndCryptoStateBufIndex];
    hucIndCryptoStateBufIndex      = (hucIndCryptoStateBufIndex + 1) % INDIRECT_CRYPTO_STATE_BUFFER_NUM;

    HucStreamOutParams.curIndEntriesNum = 0;
    // On stripe mode the segments are remapped, so use remapped segment number for stream out
    HucStreamOutParams.curNumSegments = (pEncryptionParams->dwBlocksStripeEncCount == 0) ?
        pEncryptionParams->dwSubSampleCount : sampleRemappingBlockNum;
    HucStreamOutParams.segmentInfo = (void *)segmentInfo;

    CODECHAL_DECODE_CHK_STATUS_RETURN(osInterface->pfnGetCommandBuffer(osInterface, &CmdBuffer, 0));

    // Send command buffer header at the beginning (OS dependent)
    MHW_GENERIC_PROLOG_PARAMS GenericPrologParams;
    MOS_ZeroMemory(&GenericPrologParams, sizeof(GenericPrologParams));
    GenericPrologParams.pOsInterface = osInterface;
    GenericPrologParams.pvMiInterface = hwInterface->GetMiInterface();
    GenericPrologParams.bMmcEnabled = m_mmcEnabled;
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

    if (MOS_VE_SUPPORTED(osInterface))
    {
        pAttriVe = (PMOS_CMD_BUF_ATTRI_VE)CmdBuffer.Attributes.pAttriVe;
        if (pAttriVe)
        {
            pAttriVe->bUseVirtualEngineHint = true;
            MOS_ZeroMemory(&pAttriVe->VEngineHintParams, sizeof(pAttriVe->VEngineHintParams));
            pAttriVe->VEngineHintParams.NeedSyncWithPrevious = true;
        }
    }

    CODECHAL_DECODE_CHK_STATUS_RETURN(osInterface->pfnSubmitCommandBuffer(osInterface, &CmdBuffer, bRenderingFlags));

    // The bitstream is clear now, don't need PAVP flag anymore.
    mhwCp->SetHostEncryptMode(CP_TYPE_NONE);

finish:
    return eStatus;
}

MOS_STATUS CodechalSecureDecodeVp9::SetBitstreamBuffer(MHW_VDBOX_IND_OBJ_BASE_ADDR_PARAMS *indObjBaseAddrParams)
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

MOS_STATUS CodechalSecureDecodeVp9::ProbBufFullUpdate(
    void                        *state,
    PMOS_COMMAND_BUFFER         cmdBuffer)
{
    CodechalDecodeVp9*              pVp9State           = nullptr;
    MhwMiInterface*                 pCommonMiInterface  = nullptr;
    uint32_t                        dwBufSize           = CODEC_VP9_PROB_MAX_NUM_ELEM;
    CodechalHucStreamoutParams      HucStreamOutParams;
    MOS_STATUS                      eStatus = MOS_STATUS_SUCCESS;

    CODECHAL_DECODE_FUNCTION_ENTER;

    CODECHAL_DECODE_CHK_NULL_RETURN(state);
    CODECHAL_DECODE_CHK_NULL_RETURN(hwInterface);

    pVp9State           = (CodechalDecodeVp9*)state;
    pCommonMiInterface  = hwInterface->GetMiInterface();

    osInterface->pfnSetPerfTag(
        osInterface,
        (uint16_t)(((CODECHAL_DECODE_MODE_VP9VLD << 4) & 0xF0) | COPY_TYPE));
    osInterface->pfnResetPerfBufferID(osInterface);

    CodechalResLock ResourceLock(osInterface, &pVp9State->m_resVp9ProbBuffer[CODEC_VP9_NUM_CONTEXTS]);
    auto data = (uint8_t*)ResourceLock.Lock(CodechalResLock::writeOnly);
    CODECHAL_DECODE_CHK_NULL_RETURN(data);

    CODECHAL_DECODE_CHK_STATUS_RETURN(pVp9State->ContextBufferInit(
        data,
        pVp9State->m_probUpdateFlags.bResetKeyDefault ? true : false));

    CODECHAL_DECODE_CHK_STATUS_RETURN(MOS_SecureMemcpy(
        (data + CODEC_VP9_SEG_PROB_OFFSET),
        7,
        pVp9State->m_probUpdateFlags.SegTreeProbs, 7));

    CODECHAL_DECODE_CHK_STATUS_RETURN(MOS_SecureMemcpy(
        (data + CODEC_VP9_SEG_PROB_OFFSET + 7),
        3,
        pVp9State->m_probUpdateFlags.SegPredProbs, 3));

    MOS_ZeroMemory(&HucStreamOutParams, sizeof(HucStreamOutParams));

    // Ind Obj Addr command
    HucStreamOutParams.dataBuffer                 = &pVp9State->m_resVp9ProbBuffer[CODEC_VP9_NUM_CONTEXTS];
    HucStreamOutParams.dataSize                   = dwBufSize;
    HucStreamOutParams.dataOffset                 = 0;
    HucStreamOutParams.streamOutObjectBuffer      = &pVp9State->m_resVp9ProbBuffer[pVp9State->m_frameCtxIdx];
    HucStreamOutParams.streamOutObjectSize        = dwBufSize;
    HucStreamOutParams.streamOutObjectOffset      = 0;

    // Stream object params
    HucStreamOutParams.indStreamInLength          = dwBufSize;
    HucStreamOutParams.inputRelativeOffset        = 0;
    HucStreamOutParams.outputRelativeOffset       = 0;

    CODECHAL_DECODE_CHK_STATUS_RETURN(PerformHucStreamOut(
        &HucStreamOutParams,
        cmdBuffer));

    MHW_MI_FLUSH_DW_PARAMS FlushDwParams;
    MOS_ZeroMemory(&FlushDwParams, sizeof(FlushDwParams));
    CODECHAL_DECODE_CHK_STATUS_RETURN(pCommonMiInterface->AddMiFlushDwCmd(
        cmdBuffer,
        &FlushDwParams));

    return eStatus;
}

MOS_STATUS CodechalSecureDecodeVp9::ProbBufPartialUpdate(
    void                    *state,
    PMOS_COMMAND_BUFFER     cmdBuffer)
{
    CodechalDecodeVp9*                              pVp9State            = nullptr;
    MhwMiInterface*                                 pCommonMiInterface   = nullptr;
    MmioRegistersHuc                                *pMmioRegisters      = nullptr;
    PCODECHAL_DECODE_VP9_PROB_UPDATE                pProbUpdate          = nullptr;
    CodechalDecodeStatusBuffer                      *DecodeStatusBuf     = nullptr;
    uint32_t                                        dwStatusBufferOffset = 0;
    MHW_VDBOX_PIPE_MODE_SELECT_PARAMS               PipeModeSelectParams;
    MHW_VDBOX_HUC_IMEM_STATE_PARAMS                 ImemParams;
    MHW_VDBOX_HUC_DMEM_STATE_PARAMS                 DmemParams;
    MHW_VDBOX_HUC_VIRTUAL_ADDR_PARAMS               VirtualAddrParams;
    MHW_MI_STORE_REGISTER_MEM_PARAMS                StoreRegParams;
    MHW_MI_STORE_DATA_PARAMS                        StoreDataParams;
    MHW_VDBOX_VD_PIPE_FLUSH_PARAMS                  VDPipeFlushParams;
    MHW_MI_FLUSH_DW_PARAMS                          FlushDwParams;
    MOS_STATUS                                      eStatus = MOS_STATUS_SUCCESS;

    CODECHAL_DECODE_FUNCTION_ENTER;

    CODECHAL_DECODE_CHK_NULL(state);
    CODECHAL_DECODE_CHK_NULL(cmdBuffer);
    CODECHAL_DECODE_CHK_NULL(hwInterface);
    CODECHAL_DECODE_CHK_NULL(hwInterface->GetMiInterface());

    pVp9State           = (CodechalDecodeVp9*)state;
    pCommonMiInterface  = hwInterface->GetMiInterface();
    pProbUpdate         = &pVp9State->m_probUpdateFlags;

    CODECHAL_DECODE_CHK_COND((pVp9State->GetVdboxIndex() > hwInterface->GetMfxInterface()->GetMaxVdboxIndex()), "ERROR - vdbox index exceed the maximum");
    pMmioRegisters = hwInterface->GetHucInterface()->GetMmioRegisters(pVp9State->GetVdboxIndex());

    // Load kernel from WOPCM into L2 storage RAM
    MOS_ZeroMemory(&ImemParams, sizeof(ImemParams));
    ImemParams.dwKernelDescriptor = VDBOX_HUC_VP9_PAVP_DECODE_KERNEL_DESCRIPTOR;
    CODECHAL_DECODE_CHK_STATUS(hwInterface->GetHucInterface()->AddHucImemStateCmd(cmdBuffer, &ImemParams));

    // Pipe mode select
    PipeModeSelectParams.dwMediaSoftResetCounterValue = 2400;
    CODECHAL_DECODE_CHK_STATUS(hwInterface->GetHucInterface()->AddHucPipeModeSelectCmd(cmdBuffer, &PipeModeSelectParams));

    // Set HuC DMEM param
    CODECHAL_DECODE_CHK_STATUS(SetHucDmemParams(pVp9State, osInterface, pProbUpdate));
    MOS_ZeroMemory(&DmemParams, sizeof(DmemParams));
    DmemParams.presHucDataSource = &pVp9State->m_resDmemBuffer;
    DmemParams.dwDataLength      = pVp9State->m_dmemBufferSize;
    DmemParams.dwDmemOffset      = HUC_DMEM_OFFSET_RTOS_GEMS;
    CODECHAL_DECODE_CHK_STATUS(hwInterface->GetHucInterface()->AddHucDmemStateCmd(cmdBuffer, &DmemParams));

    // Pass prob buffer to be updated and interprob save buffer by Virtual Addr command
    MOS_ZeroMemory(&VirtualAddrParams, sizeof(VirtualAddrParams));
    VirtualAddrParams.regionParams[3].presRegion = &pVp9State->m_resVp9ProbBuffer[pVp9State->m_frameCtxIdx];
    VirtualAddrParams.regionParams[3].isWritable = true;
    VirtualAddrParams.regionParams[4].presRegion = &pVp9State->m_resInterProbSaveBuffer;
    VirtualAddrParams.regionParams[4].isWritable = true;
#ifdef CODECHAL_HUC_KERNEL_DEBUG
    VirtualAddrParams.regionParams[15].presRegion = &pVp9State->resHucSharedBuffer;
    VirtualAddrParams.regionParams[15].isWritable = false;
#endif
    CODECHAL_DECODE_CHK_STATUS(hwInterface->GetHucInterface()->AddHucVirtualAddrStateCmd(cmdBuffer, &VirtualAddrParams));

    DecodeStatusBuf = pVp9State->GetDecodeStatusBuf();
    dwStatusBufferOffset = (DecodeStatusBuf->m_currIndex * sizeof(CodechalDecodeStatus)) +
        DecodeStatusBuf->m_storeDataOffset +
        sizeof(uint32_t) * 2;

    if (pVp9State->IsStatusQueryReportingEnabled())
    {
        // Write HUC_STATUS2 mask
        MOS_ZeroMemory(&StoreDataParams, sizeof(StoreDataParams));
        StoreDataParams.pOsResource      = &DecodeStatusBuf->m_statusBuffer;
        StoreDataParams.dwResourceOffset = dwStatusBufferOffset + DecodeStatusBuf->m_hucErrorStatus2MaskOffset;
        StoreDataParams.dwValue          = hwInterface->GetHucInterface()->GetHucStatus2ImemLoadedMask();
        CODECHAL_DECODE_CHK_STATUS(pCommonMiInterface->AddMiStoreDataImmCmd(cmdBuffer, &StoreDataParams));

        // Store HUC_STATUS2 register
        MOS_ZeroMemory(&StoreRegParams, sizeof(StoreRegParams));
        StoreRegParams.presStoreBuffer  = &DecodeStatusBuf->m_statusBuffer;
        StoreRegParams.dwOffset         = dwStatusBufferOffset + DecodeStatusBuf->m_hucErrorStatus2RegOffset;
        StoreRegParams.dwRegister       = pMmioRegisters->hucStatus2RegOffset;
        CODECHAL_DECODE_CHK_STATUS(pCommonMiInterface->AddMiStoreRegisterMemCmd(cmdBuffer, &StoreRegParams));
    }

    CODECHAL_DECODE_CHK_STATUS(hwInterface->GetHucInterface()->AddHucStartCmd(cmdBuffer, true));

    CODECHAL_DECODE_CHK_STATUS(hwInterface->AddHucDummyStreamOut(cmdBuffer));

    MOS_ZeroMemory(&VDPipeFlushParams, sizeof(VDPipeFlushParams));
    VDPipeFlushParams.Flags.bWaitDoneHEVC           = 1;
    VDPipeFlushParams.Flags.bFlushHEVC              = 1;
    VDPipeFlushParams.Flags.bWaitDoneVDCmdMsgParser = 1;
    CODECHAL_DECODE_CHK_STATUS(hwInterface->GetVdencInterface()->AddVdPipelineFlushCmd(cmdBuffer, &VDPipeFlushParams));

    MOS_ZeroMemory(&FlushDwParams, sizeof(FlushDwParams));
    CODECHAL_DECODE_CHK_STATUS(pCommonMiInterface->AddMiFlushDwCmd(cmdBuffer, &FlushDwParams));

finish:
    return eStatus;
}

MOS_STATUS CodechalSecureDecodeVp9::SetHucDmemParams(
    CodechalDecodeVp9*                 vp9State,
    PMOS_INTERFACE                     osInterface,
    PCODECHAL_DECODE_VP9_PROB_UPDATE   probUpdate)
{
    MOS_LOCK_PARAMS                     LockFlagsWriteOnly;
    PCODECHAL_DECODE_VP9_PROB_UPDATE    pDmemBuf = nullptr;
    MOS_STATUS                          eStatus  = MOS_STATUS_SUCCESS ;

    CODECHAL_DECODE_FUNCTION_ENTER;

    CODECHAL_DECODE_CHK_NULL(vp9State);
    CODECHAL_DECODE_CHK_NULL(osInterface);
    CODECHAL_DECODE_CHK_NULL(probUpdate);

    MOS_ZeroMemory(&LockFlagsWriteOnly, sizeof(MOS_LOCK_PARAMS));
    LockFlagsWriteOnly.WriteOnly = 1;

    pDmemBuf = (PCODECHAL_DECODE_VP9_PROB_UPDATE)osInterface->pfnLockResource(
        osInterface,
        &vp9State->m_resDmemBuffer,
        &LockFlagsWriteOnly);
    CODECHAL_DECODE_CHK_NULL(pDmemBuf);

    pDmemBuf->bSegProbCopy     = probUpdate->bSegProbCopy;
    pDmemBuf->bProbReset       = probUpdate->bProbReset;
    pDmemBuf->bProbRestore     = probUpdate->bProbRestore;
    pDmemBuf->bProbSave        = probUpdate->bProbSave;
    pDmemBuf->bResetFull       = probUpdate->bResetFull;
    pDmemBuf->bResetKeyDefault = probUpdate->bResetKeyDefault;
    CODECHAL_DECODE_CHK_STATUS(MOS_SecureMemcpy(pDmemBuf->SegTreeProbs, 7, probUpdate->SegTreeProbs, 7));
    CODECHAL_DECODE_CHK_STATUS(MOS_SecureMemcpy(pDmemBuf->SegPredProbs, 3, probUpdate->SegPredProbs, 3));

    CODECHAL_DECODE_CHK_STATUS(osInterface->pfnUnlockResource(
        osInterface,
        &vp9State->m_resDmemBuffer));

finish:
    if (eStatus != MOS_STATUS_SUCCESS && osInterface != nullptr && vp9State != nullptr)
    {
        osInterface->pfnUnlockResource(osInterface, &vp9State->m_resDmemBuffer);
    }

    return eStatus;
}
