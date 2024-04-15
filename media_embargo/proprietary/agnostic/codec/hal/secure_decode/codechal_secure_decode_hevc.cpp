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
//! \file     codechal_secure_decode_hevc.cpp
//! \brief    Impelements the public interface for CodecHal Secure Decode 
//!

#include "codechal_secure_decode_hevc.h"
#include "codechal_decode_singlepipe_virtualengine.h"

CodechalSecureDecodeHevc::CodechalSecureDecodeHevc(CodechalHwInterface *hwInterface):CodechalSecureDecode(hwInterface)
{
}

CodechalSecureDecodeHevc::~CodechalSecureDecodeHevc()
{
}

MOS_STATUS CodechalSecureDecodeHevc::AllocateResource(void *state)
{
    CODECHAL_DECODE_FUNCTION_ENTER;

    CODECHAL_DECODE_CHK_NULL_RETURN(state);
    PCODECHAL_DECODE_HEVC_STATE pHevcState = (PCODECHAL_DECODE_HEVC_STATE)state;

    return AllocateStreamOutResource(
        pHevcState->m_copyDataBufferInUse ? pHevcState->m_copyDataBufferSize : pHevcState->m_dataSize);
}

MOS_STATUS CodechalSecureDecodeHevc::Execute(void *state)
{
    PCODECHAL_DECODE_HEVC_STATE     pHevcState          = nullptr;
    MhwMiInterface*                 pCommonMiInterface  = nullptr;
    PMHW_PAVP_ENCRYPTION_PARAMS     pEncryptionParams   = nullptr;
    PMOS_CMD_BUF_ATTRI_VE          pAttriVe           = nullptr;
    bool                            bRenderingFlags     = false;
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

    pHevcState          = (PCODECHAL_DECODE_HEVC_STATE)state;
    pCommonMiInterface  = hwInterface->GetMiInterface();
    pEncryptionParams   = mhwCp->GetEncryptionParams();
    bRenderingFlags     = pHevcState->GetVideoContextUsesNullHw();;

    CodechalHucStreamoutParams HucStreamOutParams;
    MOS_ZeroMemory(&HucStreamOutParams, sizeof(HucStreamOutParams));
    HucStreamOutParams.mode                       = CODECHAL_DECODE_MODE_HEVCVLD;
    HucStreamOutParams.dataBuffer                 = pHevcState->m_copyDataBufferInUse ? &pHevcState->m_resCopyDataBuffer : &pHevcState->m_resDataBuffer;
    HucStreamOutParams.dataSize                   = pHevcState->m_copyDataBufferInUse ? pHevcState->m_copyDataBufferSize : pHevcState->m_dataSize;
    HucStreamOutParams.dataOffset                 = 0;
    HucStreamOutParams.streamOutObjectBuffer      = &hucStreamOutBuf[hucStreamOutBufIndex];
    HucStreamOutParams.streamOutObjectSize        = hucStreamOutBufSize[hucStreamOutBufIndex];
    HucStreamOutParams.streamOutObjectOffset      = 0;
    HucStreamOutParams.indStreamInLength          = pHevcState->m_copyDataBufferInUse ? pHevcState->m_copyDataBufferSize : pHevcState->m_dataSize;
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

    if (MOS_VE_SUPPORTED(osInterface))
    {
        pAttriVe = (PMOS_CMD_BUF_ATTRI_VE)CmdBuffer.Attributes.pAttriVe;
        if(pAttriVe)
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

MOS_STATUS CodechalSecureDecodeHevc::SetBitstreamBuffer(MHW_VDBOX_IND_OBJ_BASE_ADDR_PARAMS *indObjBaseAddrParams)
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

MOS_STATUS CodechalSecureDecodeHevc::SetHevcHucDmemS2LBss(
    void     *state,
    void     *hucPicBss,
    void     *hucSliceBss)
{
    PCODECHAL_DECODE_HEVC_STATE     hevcState;
    PHUC_HEVC_S2L_PIC_BSS           hucHevcS2LPicBss;
    PHUC_HEVC_S2L_PIC_BSS_EXT       hucHevcS2LPicBssExt;
    PHUC_HEVC_S2L_SLICE_BSS         hucHevcS2LSliceBss;
    PHUC_HEVC_S2L_SLICE_BSS_EXT     hucHevcS2LSliceBssExt;
    PMHW_PAVP_ENCRYPTION_PARAMS     pEncryptionParams = nullptr;
    MHW_PAVP_AES_PARAMS             AESParams;
    MOS_STATUS                      eStatus = MOS_STATUS_SUCCESS;
    MhwCpBase                       *mhwCp = nullptr;

    CODECHAL_DECODE_FUNCTION_ENTER;

    CODECHAL_DECODE_CHK_NULL_RETURN(state);
    CODECHAL_DECODE_CHK_NULL_RETURN(hucPicBss);
    CODECHAL_DECODE_CHK_NULL_RETURN(hucSliceBss);
    CODECHAL_DECODE_CHK_NULL_RETURN(hwInterface->GetCpInterface());

    hevcState             = (PCODECHAL_DECODE_HEVC_STATE)state;
    hucHevcS2LPicBss      = (PHUC_HEVC_S2L_PIC_BSS)hucPicBss;
    hucHevcS2LPicBssExt   = (PHUC_HEVC_S2L_PIC_BSS_EXT)(&hucHevcS2LPicBss->reserve);
    hucHevcS2LSliceBss    = (PHUC_HEVC_S2L_SLICE_BSS)hucSliceBss;
    mhwCp = dynamic_cast<MhwCpBase*>(hwInterface->GetCpInterface());
    CODECHAL_DECODE_CHK_NULL_RETURN(mhwCp);

    pEncryptionParams = mhwCp->GetEncryptionParams();
    hucHevcS2LPicBssExt->AESEnable = osInterface->osCpInterface->IsCpEnabled() ? (pEncryptionParams->HostEncryptMode == CP_TYPE_BASIC) : 0;
    //doesn't use hcp indirect aes cmd to send AES State as per Slice state, because no performance benefit
    hucHevcS2LPicBssExt->IsPlayReady3_0InUse        = false;
    hucHevcS2LPicBssExt->IsPlayReady3_0HcpSupported = false;

    if (hucHevcS2LPicBssExt->AESEnable && !hucHevcS2LPicBssExt->IsPlayReady3_0InUse)
    {
        hucHevcS2LPicBssExt->IsCTRMode                 = (pEncryptionParams->PavpEncryptionType == CP_SECURITY_AES128_CBC) ? 0 : 1;
        hucHevcS2LPicBssExt->CTRFlavor                 = 2;
        hucHevcS2LPicBssExt->IsFairPlay                = (pEncryptionParams->PavpEncryptionType == CP_SECURITY_DECE_AES128_CTR) ? 1 : 0;
        hucHevcS2LPicBssExt->InitialFairPlayEnabled    = 0;
        hucHevcS2LPicBssExt->InitPktBytes              = 0;
        hucHevcS2LPicBssExt->EncryptedBytes            = 0;
        hucHevcS2LPicBssExt->ClearBytes                = 0;

        MOS_ZeroMemory(&AESParams, sizeof(AESParams));
        AESParams.presDataBuffer = &hevcState->m_resDataBuffer;

        MOS_ZeroMemory(InitialCounterPerSlice, sizeof(InitialCounterPerSlice));
    }

    for (uint32_t i = 0; i < hevcState->m_numSlices; i++)
    {
        hucHevcS2LSliceBssExt = (PHUC_HEVC_S2L_SLICE_BSS_EXT)(&hucHevcS2LSliceBss->reserve);

        if (hucHevcS2LPicBssExt->AESEnable && !hucHevcS2LPicBssExt->IsPlayReady3_0InUse)
        {
            AESParams.dwDataLength          = hevcState->m_hevcSliceParams[i].slice_data_size;
            AESParams.dwDataStartOffset     = hevcState->m_hevcSliceParams[i].slice_data_offset;
            AESParams.bShortFormatInUse     = 1;
            AESParams.dwSliceIndex          = i;
            AESParams.bLastPass             = 0;
            AESParams.dwTotalBytesConsumed  = 0;

            uint32_t dwAesCtr[4] = { 0 };
            CODECHAL_DECODE_CHK_STATUS_RETURN(GetSliceInitValue(
                &AESParams,
                dwAesCtr));

            hucHevcS2LSliceBssExt->InitializationCounterValue0 = InitialCounterPerSlice[i][0] = dwAesCtr[0];
            hucHevcS2LSliceBssExt->InitializationCounterValue1 = InitialCounterPerSlice[i][1] = dwAesCtr[1];
            hucHevcS2LSliceBssExt->InitializationCounterValue2 = InitialCounterPerSlice[i][2] = dwAesCtr[2];
            hucHevcS2LSliceBssExt->InitializationCounterValue3 = InitialCounterPerSlice[i][3] = dwAesCtr[3];
        }

        hucHevcS2LSliceBss++;
    }
    return eStatus;
}

MOS_STATUS CodechalSecureDecodeHevc::AddHucSecureState(
    void                            *state,
    void                            *sliceState,
    PMOS_COMMAND_BUFFER             cmdBuffer)
{
    PCODECHAL_DECODE_HEVC_STATE         pHevcState;
    PMHW_VDBOX_HEVC_SLICE_STATE         pHevcSliceState;
    MHW_PAVP_AES_PARAMS                 AESParams;
    MHW_PAVP_AES_IND_PARAMS             IndAesParams;
    MhwCpBase                            *pCpInterface;
    PCODEC_HEVC_SLICE_PARAMS            pSlc;
    MOS_STATUS                          eStatus = MOS_STATUS_SUCCESS;

    CODECHAL_DECODE_FUNCTION_ENTER;

    CODECHAL_DECODE_CHK_NULL(state);
    CODECHAL_DECODE_CHK_NULL(sliceState);
    CODECHAL_DECODE_CHK_NULL(cmdBuffer);
    CODECHAL_DECODE_CHK_NULL(hwInterface);
    CODECHAL_DECODE_CHK_NULL(hwInterface->GetCpInterface());

    pHevcState      = (PCODECHAL_DECODE_HEVC_STATE)state;
    pHevcSliceState = (PMHW_VDBOX_HEVC_SLICE_STATE)sliceState;

    pCpInterface = dynamic_cast<MhwCpBase*>(hwInterface->GetCpInterface());
    CODECHAL_DECODE_CHK_NULL(pCpInterface);
    if (pCpInterface->GetHostEncryptMode() == CP_TYPE_BASIC)
    {
        pSlc = pHevcSliceState->pHevcSliceParams;

        if (!isStreamoutNeeded)
        {
            MOS_ZeroMemory(&AESParams, sizeof(AESParams));
            AESParams.presDataBuffer        = pHevcSliceState->presDataBuffer;
            pCpInterface->SetCpAesCounter(InitialCounterPerSlice[pHevcSliceState->dwSliceIndex]);
            AESParams.dwDataLength          = pSlc->slice_data_size;
            AESParams.dwDataStartOffset     = pSlc->slice_data_offset;
            AESParams.bShortFormatInUse     = 1;
            AESParams.dwSliceIndex          = pHevcSliceState->dwSliceIndex;
            AESParams.bLastPass             = 0;
            AESParams.dwTotalBytesConsumed  = 0;

            CODECHAL_DECODE_CHK_STATUS(pCpInterface->AddHucAesState(cmdBuffer, &AESParams));
        }
    }

finish:
    return eStatus;
}

MOS_STATUS CodechalSecureDecodeHevc::SetHucStreamObj(
    PMHW_VDBOX_HUC_STREAM_OBJ_PARAMS hucStreamObjParams)
{
    MOS_STATUS    eStatus = MOS_STATUS_SUCCESS;

    CODECHAL_DECODE_FUNCTION_ENTER;

    MhwCpBase* mhwCp = dynamic_cast<MhwCpBase*>(hwInterface->GetCpInterface());

    if (MEDIA_IS_WA(hwInterface->GetWaTable(), WaHuCStreamOutOffsetIsStreamInOffsetForPAVP) &&
        mhwCp != nullptr && (mhwCp->GetHostEncryptMode() == CP_TYPE_BASIC))
    {
        hucStreamObjParams->dwIndStreamOutStartAddrOffset = hucStreamObjParams->dwIndStreamInStartAddrOffset;
    }

    return eStatus;
}

MOS_STATUS CodechalSecureDecodeHevc::AddHcpSecureState(
    PMOS_COMMAND_BUFFER             cmdBuffer,
    void                            *sliceState)
{
    PMHW_VDBOX_HEVC_SLICE_STATE         pHevcSliceState;
    MHW_PAVP_AES_PARAMS                 AESParams;
    PCODEC_HEVC_SLICE_PARAMS            pSlc = nullptr;
    MOS_STATUS                          eStatus = MOS_STATUS_SUCCESS;
    MhwCpBase                           *mhwCp = nullptr;


    CODECHAL_DECODE_CHK_NULL(hwInterface);
    CODECHAL_DECODE_CHK_NULL(hwInterface->GetCpInterface());
    CODECHAL_DECODE_CHK_NULL(sliceState);

    mhwCp = dynamic_cast<MhwCpBase*>(hwInterface->GetCpInterface());
    CODECHAL_DECODE_CHK_NULL(mhwCp);

    pHevcSliceState = (PMHW_VDBOX_HEVC_SLICE_STATE)sliceState;

    if (mhwCp->GetHostEncryptMode() == CP_TYPE_BASIC)
    {
        pSlc = pHevcSliceState->pHevcSliceParams;

        MOS_ZeroMemory(&AESParams, sizeof(AESParams));
        AESParams.presDataBuffer = pHevcSliceState->presDataBuffer;
        AESParams.dwDataLength = pSlc->slice_data_size;
        AESParams.dwDataStartOffset = pSlc->slice_data_offset + pHevcSliceState->dwOffset;
        AESParams.bShortFormatInUse = 0;
        AESParams.dwSliceIndex = pHevcSliceState->dwSliceIndex;
        AESParams.bLastPass = 0;
        AESParams.dwTotalBytesConsumed = 0;

        CODECHAL_DECODE_CHK_STATUS(mhwCp->AddHcpAesState(
            true,
            cmdBuffer,
            nullptr,
            &AESParams));
    }

finish:
    return eStatus;
}

MOS_STATUS CodechalSecureDecodeHevc::GetSliceInitValue(
    PMHW_PAVP_AES_PARAMS            params,
    uint32_t                        *value)
{
    MhwCpBase                        *pCpInterface = nullptr;
    uint32_t                        dwAesCtr[4] = { 0,0,0,0 };
    uint32_t                        dwCounterIncrement = 0;
    uint32_t                        dwNalUnitLength = 0;
    uint32_t                        dwNumClearBytes = 0;
    uint32_t                        dwFixedNumberOfClearBytesInSlice = MHW_PAVP_DECE_NUM_CLEAR_BYTES_IN_SLICE;
    uint32_t                        dwZeroPadding = 0;
    uint32_t                        *pdwFirstEncryptedOword = nullptr;
    uint32_t                        *pdwLastEncryptedOword = nullptr;
    MOS_LOCK_PARAMS                 LockFlags;
    MOS_STATUS                      eStatus = MOS_STATUS_SUCCESS;

    CODECHAL_DECODE_FUNCTION_ENTER;

    CODECHAL_DECODE_CHK_NULL(hwInterface);
    CODECHAL_DECODE_CHK_NULL(hwInterface->GetCpInterface());
    CODECHAL_DECODE_CHK_NULL(osInterface);
    CODECHAL_DECODE_CHK_NULL(params);
    CODECHAL_DECODE_CHK_NULL(value);

    pCpInterface = dynamic_cast<MhwCpBase*>(hwInterface->GetCpInterface());
    CODECHAL_DECODE_CHK_NULL(pCpInterface);

    eStatus = MOS_SecureMemcpy(dwAesCtr,
        CODEC_DWORDS_PER_AES_BLOCK * sizeof(uint32_t),
        pCpInterface->GetCpAesCounter(),
        CODEC_DWORDS_PER_AES_BLOCK * sizeof(uint32_t));

    if (eStatus != MOS_STATUS_SUCCESS)
    {
        CODECHAL_DECODE_ASSERTMESSAGE("%s: Failed to copy memory\n", __FUNCTION__);
        goto finish;
    }

    if (pCpInterface->GetCpEncryptionType() == CP_SECURITY_DECE_AES128_CTR)
    {
        // Fair Play
        dwNalUnitLength = (params->bShortFormatInUse) ? MOS_NAL_UNIT_STARTCODE_LENGTH : MOS_NAL_UNIT_LENGTH;
        dwZeroPadding = params->dwDataStartOffset - params->dwTotalBytesConsumed;
        dwNumClearBytes = dwFixedNumberOfClearBytesInSlice - dwZeroPadding;

        if (params->dwDataLength == MHW_PAVP_DECE_NUM_CLEAR_BYTES_IN_SLICE)
        {
            // Reset counter to zero
            dwAesCtr[0] = dwAesCtr[1] = dwAesCtr[2] = dwAesCtr[3] = 0;

            // No increment as well
            dwCounterIncrement = 0;
        }
        else
        {
            dwCounterIncrement = (params->dwTotalBytesConsumed >> 4) - (params->dwSliceIndex << 3);
        }
    }
    else if (pCpInterface->GetCpEncryptionType() == CP_SECURITY_AES128_CBC)
    {
        // CBC
        CODECHAL_DECODE_CHK_NULL(params->presDataBuffer);

        pdwLastEncryptedOword = nullptr;
        MOS_ZeroMemory(&LockFlags, sizeof(MOS_LOCK_PARAMS));
        LockFlags.ReadOnly = 1;
        pdwFirstEncryptedOword = (uint32_t *)osInterface->pfnLockResource(
            osInterface,
            params->presDataBuffer,
            &LockFlags);

        CODECHAL_DECODE_CHK_NULL(pdwFirstEncryptedOword);

        pdwLastEncryptedOword = pdwFirstEncryptedOword + ((params->dwDataStartOffset >> 4) * 4);
        if (pdwLastEncryptedOword != pdwFirstEncryptedOword)
        {
            pdwLastEncryptedOword -= KEY_DWORDS_PER_AES_BLOCK;  // The block size of AES-128 is 128 bits, so the subtrahend is 4.
            dwAesCtr[0] = pdwLastEncryptedOword[0];
            dwAesCtr[1] = pdwLastEncryptedOword[1];
            dwAesCtr[2] = pdwLastEncryptedOword[2];
            dwAesCtr[3] = pdwLastEncryptedOword[3];
        }

        osInterface->pfnUnlockResource(osInterface, params->presDataBuffer);

        dwCounterIncrement = 0;
    }
    else
    {
        // Standard AES CTR mode
        dwCounterIncrement = params->dwDataStartOffset >> 4;
    }

    // Decode increments the counter before adding the AES state to the command buffer
    pCpInterface->IncrementCounter(
        dwAesCtr,
        dwCounterIncrement);

    eStatus = MOS_SecureMemcpy(value, sizeof(dwAesCtr), dwAesCtr, sizeof(dwAesCtr));

    if (eStatus != MOS_STATUS_SUCCESS)
    {
        CODECHAL_DECODE_ASSERTMESSAGE("%s: Failed to copy memory\n", __FUNCTION__);
        goto finish;
    }

finish:
    return eStatus;
}
