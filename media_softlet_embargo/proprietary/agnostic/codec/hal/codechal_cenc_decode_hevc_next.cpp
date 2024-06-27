/*
// Copyright (C) 2022 Intel Corporation
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
//! \file      codechal_cenc_decode_hevc_next.cpp 
//! \brief     Impelements the public interface for CodecHal CENC Decode 
//!

#include "codechal_cenc_decode_hevc_next.h"
#include "codechal_utilities.h"
#include "mhw_cp_hwcmd_common.h"
#include "mhw_vdbox_hcp_hwcmd_g12_X.h"

CodechalCencDecodeHevcNext::CodechalCencDecodeHevcNext() : CodechalCencDecodeNext()
{

}

CodechalCencDecodeHevcNext::~CodechalCencDecodeHevcNext()
{
    CodecHalFreeDataList((CODEC_HEVC_PIC_PARAMS **)m_picParams, CENC_NUM_DECRYPT_SHARED_BUFFERS);
    CodecHalFreeDataList((CODECHAL_HEVC_IQ_MATRIX_PARAMS **)m_qmParams, CENC_NUM_DECRYPT_SHARED_BUFFERS);
}

MOS_STATUS CodechalCencDecodeHevcNext::Initialize(
    PMOS_CONTEXT                osContext,
    CodechalSetting             *settings)
{
    uint32_t   patchListSize = 0;
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    CENC_DECODE_FUNCTION_ENTER;

    CENC_DECODE_CHK_NULL(osContext);
    CENC_DECODE_CHK_NULL(settings);

    CodechalCencDecodeNext::Initialize(
        osContext,
        settings
    );

    // 2nd level batch buffer size for slice-level decode 
    CENC_DECODE_CHK_STATUS(m_hwInterface->GetHcpPrimitiveCommandSize(
        settings->mode, (uint32_t *)&m_standardSlcSizeNeeded, (uint32_t *)&patchListSize, true));
    
    CENC_DECODE_VERBOSEMESSAGE("dwStandardSlcSizeNeeded = %d.", m_standardSlcSizeNeeded);
    CENC_DECODE_VERBOSEMESSAGE("bEnableAesNativeSupport = %d.", m_enableAesNativeSupport);

    // add HCP_AES_IND_STATE_CMD
    m_standardSlcSizeNeeded += sizeof(HCP_AES_IND_STATE_CMD);
    // align to huc hevc fw, add dummy HCP_REF_IDX_STATE_CMD
    if (MEDIA_IS_WA(m_waTable, WaDummyReference) && !m_osInterface->bSimIsActive)
    {
        m_standardSlcSizeNeeded += sizeof(mhw_vdbox_hcp_g12_X::HCP_REF_IDX_STATE_CMD);
    }

    // Allocate the resources required to perform decryption
    CENC_DECODE_CHK_STATUS(AllocateResources());

    return eStatus;
}

MOS_STATUS CodechalCencDecodeHevcNext::AllocateDecodeResource(
    uint8_t bufIdx)
{
    CENC_DECODE_FUNCTION_ENTER;

    m_bitstreamSize = MOS_ALIGN_CEIL(m_picSize, CODECHAL_PAGE_SIZE);
    // set 2nd level batch buffer size according to number of slices
    m_sliceLevelBBSize = m_standardSlcSizeNeeded * m_numSlices;
    return CodechalCencDecodeNext::AllocateDecodeResource(bufIdx);
}

MOS_STATUS CodechalCencDecodeHevcNext::InitializeAllocation()
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    CENC_DECODE_FUNCTION_ENTER;

    // so far our HW supports up to level 6
    m_numSlices = CODECHAL_HEVC_MAX_NUM_SLICES_LVL_6;

    // shared buffer size
    m_sharedBufSize = sizeof(CODECHAL_DECRYPT_SHARED_BUFFER_HEVC);
    m_standardReportSizeApp = sizeof(DecryptStatusHevc);

    // allocate the Pic Params and QM Params buffer
    CENC_DECODE_CHK_STATUS(CodecHalAllocateDataList(
        (CODEC_HEVC_PIC_PARAMS **)m_picParams,
        CENC_NUM_DECRYPT_SHARED_BUFFERS));

    CENC_DECODE_CHK_STATUS(CodecHalAllocateDataList(
        (CODECHAL_HEVC_IQ_MATRIX_PARAMS **)m_qmParams,
        CENC_NUM_DECRYPT_SHARED_BUFFERS));

    return eStatus;
}

MOS_STATUS CodechalCencDecodeHevcNext::InitializeSharedBuf(
    bool                        initStatusReportOnly)
{
    MOS_STATUS                              eStatus = MOS_STATUS_SUCCESS;
    PCODECHAL_DECRYPT_SHARED_BUFFER_HEVC    statusBuf;
    uint32_t                                idx;

    CodechalResLock shareBuffer(m_osInterface, &m_resHucSharedBuf);
    statusBuf = (PCODECHAL_DECRYPT_SHARED_BUFFER_HEVC)shareBuffer.Lock(CodechalResLock::writeOnly);
    CENC_DECODE_CHK_NULL(statusBuf);

    if (initStatusReportOnly)
    {
        return eStatus;
    }
    else
    {
        for (idx = 0; idx < H265_MAX_NUM_PPS; idx++)
        {
            if (idx < H265_MAX_NUM_SPS)
            {
                statusBuf->SeqParams[idx].sps_seq_parameter_set_id = 0xFF;
            }
            statusBuf->PicParams[idx].pps_pic_parameter_set_id = 0xFF;
        }
    }

    return eStatus;
}

MOS_STATUS CodechalCencDecodeHevcNext::UpdateSecureOptMode(
    PMHW_PAVP_ENCRYPTION_PARAMS params)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    CENC_DECODE_FUNCTION_ENTER;

    CENC_DECODE_CHK_NULL(params);

    if (params->CpTag)
    {
        if (m_dashMode)
        {
            m_aesFromHuC = true;
            m_noStreamOutOpt = true;
        }
        else
        {
            m_aesFromHuC = false;
            m_noStreamOutOpt = false;
        }
    }
    else
    {
        m_aesFromHuC = false;
        m_noStreamOutOpt = false;
    }

    return eStatus;
}

MOS_STATUS CodechalCencDecodeHevcNext::SetDecodeParams(
    CodechalDecodeParams *decodeParams,
    void                 *cpParam)
{
    uint8_t                     cencBufIdx, idx, frameIdx, refTotalIdx, refIdx;
    PCODEC_HEVC_PIC_PARAMS      appPicParams, driverPicParams;
    CODECHAL_CENC_HEAP_INFO     *heapInfoTmp;
    MOS_STATUS                  eStatus = MOS_STATUS_SUCCESS;

    CENC_DECODE_FUNCTION_ENTER;

    CENC_DECODE_CHK_NULL(decodeParams);
    CENC_DECODE_CHK_NULL(m_picParams);
    CENC_DECODE_CHK_NULL(m_qmParams);

    appPicParams = (PCODEC_HEVC_PIC_PARAMS)decodeParams->m_picParams;

    for (cencBufIdx = 0; cencBufIdx < CENC_NUM_DECRYPT_SHARED_BUFFERS; cencBufIdx++)
    {
        if (m_statusReportNumbers[cencBufIdx] == decodeParams->m_cencDecodeStatusReportNum)
        {
            CENC_DECODE_VERBOSEMESSAGE("ucCencBufIdx=%x, m_cencDecodeStatusReportNum = %x!", cencBufIdx, decodeParams->m_cencDecodeStatusReportNum);
            break;
        }
    }

    if (cencBufIdx == CENC_NUM_DECRYPT_SHARED_BUFFERS)
    {
        CENC_DECODE_ASSERTMESSAGE("Status report number for decrypt not found in array!");
        eStatus = MOS_STATUS_INVALID_PARAMETER;
        return eStatus;
    }


    CENC_DECODE_CHK_NULL(m_picParams[cencBufIdx]);
    CENC_DECODE_CHK_NULL(m_qmParams[cencBufIdx]);

    driverPicParams = (PCODEC_HEVC_PIC_PARAMS)m_picParams[cencBufIdx];
    driverPicParams->CurrPic = appPicParams->CurrPic;

    for (idx = 0; idx < CODECHAL_MAX_CUR_NUM_REF_FRAME_HEVC; idx++)
    {
        driverPicParams->RefPicSetLtCurr[idx] = appPicParams->RefPicSetLtCurr[idx];
        driverPicParams->RefPicSetStCurrAfter[idx] = appPicParams->RefPicSetStCurrAfter[idx];
        driverPicParams->RefPicSetStCurrBefore[idx] = appPicParams->RefPicSetStCurrBefore[idx];
    }

    // Need to add non cur ref frames into RefFrameList since MVbuffer management depends on it.
    for (idx = 0; idx < CODEC_MAX_NUM_REF_FRAME_HEVC; idx++)
    {
        driverPicParams->RefFrameList[idx] = appPicParams->RefFrameList[idx];
        driverPicParams->PicOrderCntValList[idx] = appPicParams->PicOrderCntValList[idx];
    }

    

    driverPicParams->RefFieldPicFlag = appPicParams->RefFieldPicFlag;
    driverPicParams->RefBottomFieldFlag = appPicParams->RefBottomFieldFlag;
    driverPicParams->StatusReportFeedbackNumber = appPicParams->StatusReportFeedbackNumber;

    MOS_SecureMemcpy(
        decodeParams->m_picParams,
        sizeof(CODEC_HEVC_PIC_PARAMS),
        driverPicParams,
        sizeof(CODEC_HEVC_PIC_PARAMS));
    MOS_SecureMemcpy(
        decodeParams->m_iqMatrixBuffer,
        sizeof(CODECHAL_HEVC_IQ_MATRIX_PARAMS),
        m_qmParams[cencBufIdx],
        sizeof(CODECHAL_HEVC_IQ_MATRIX_PARAMS));

    //hevcState->dwDataSize        = dwStreamOutBufSize[ucCencBufIdx != 0];
    decodeParams->m_dataSize = driverPicParams->dwLastSliceEndPos;
    decodeParams->m_numSlices = 0; // is the needed?
    
    //hevcState->m_hevcSliceParams = nullptr;

    heapInfoTmp =&m_heapInfo[cencBufIdx];
    if (m_noStreamOutOpt)
    {
        decodeParams->m_dataSize = heapInfoTmp->u32OriginalBitstreamSize;
        decodeParams->m_dataBuffer = &heapInfoTmp->resOriginalBitstream;
        decodeParams->m_dataOffset = 0;
    }
    else
    {
        decodeParams->m_dataSize = heapInfoTmp->BitstreamBlock.GetSize();
        MOS_RESOURCE *resHeap = nullptr;
        CENC_DECODE_CHK_NULL(resHeap = heapInfoTmp->BitstreamBlock.GetResource());
        decodeParams->m_dataBuffer = resHeap;
        decodeParams->m_dataOffset = heapInfoTmp->BitstreamBlock.GetOffset();
    }
    CENC_DECODE_VERBOSEMESSAGE("Decode dwLastSliceEndPos %d.", decodeParams->m_dataSize);

    m_shareBuf.secondLvlBbBlock = &heapInfoTmp->SecondLvlBbBlock;
    m_shareBuf.trackerId = heapInfoTmp->u32TrackerId;
    m_shareBuf.bufIdx = cencBufIdx;
    decodeParams->m_cencBuf = &m_shareBuf;

    MOS_SecureMemcpy(
        cpParam,
        sizeof(MHW_PAVP_ENCRYPTION_PARAMS),
        &m_savedEncryptionParams[cencBufIdx],
        sizeof(MHW_PAVP_ENCRYPTION_PARAMS));

    return eStatus;
}

void CodechalCencDecodeHevcNext::ScalingListSpsPps(
    PCODECHAL_DECRYPT_STATUS_REPORT_HEVC    statusDriver,
    PCODECHAL_HEVC_IQ_MATRIX_PARAMS         hevcQmParams)
{
    uint8_t   idx = 0, idxSize = 0, idxMatrix = 0, idxRefMatrix = 0, matrixSize = 0;
    uint8_t   scalingListMatrixSize[H265_SCALING_LIST_SIZE_NUM] = { 16, 64, 64, 64 };
    uint8_t   scalingListSize[H265_SCALING_LIST_SIZE_NUM] = { 6, 6, 6, 2 };
    h265_scaling_list                  *hevcScalingList;
    uint8_t                            *hevcDstMatrix;
    const uint8_t                      *hevcSrcMatrix;
    bool                               loadDiag = false;

    CENC_DECODE_ASSERT(statusDriver);
    CENC_DECODE_ASSERT(hevcQmParams);

    if (0 != statusDriver->PicParams.pps_scaling_list_data_present_flag)
    {
        hevcScalingList = &(statusDriver->PicParams.pps_scaling_list);
    }
    else
    {
        hevcScalingList = &(statusDriver->SeqParams.sps_scaling_list);
    }

    memset(&(hevcQmParams->ucScalingListDCCoefSizeID2[0]),
        0x10, (6 * sizeof(uint8_t)));

    memset(&(hevcQmParams->ucScalingListDCCoefSizeID3[0]),
        0x10, (2 * sizeof(uint8_t)));

    for (idxSize = 0; idxSize < H265_SCALING_LIST_SIZE_NUM; idxSize++)
    {
        matrixSize = scalingListMatrixSize[idxSize];

        for (idxMatrix = 0; idxMatrix < scalingListSize[idxSize]; idxMatrix++)
        {
            loadDiag = false;
            hevcDstMatrix = nullptr;
            hevcSrcMatrix = nullptr;

            switch (idxSize)
            {
            case H265_SCALING_LIST_4x4:
            {
                hevcDstMatrix = &(hevcQmParams->ucScalingLists0[idxMatrix][0]);

                if (0 == hevcScalingList->scaling_list_pred_mode_flag[idxSize][idxMatrix])
                {
                    if (0 == hevcScalingList->scaling_list_pred_matrix_id_delta[idxSize][idxMatrix])
                    {
                        hevcSrcMatrix = &(CODECHAL_DECODE_HEVC_Default_4x4[0]);
                    }
                    else
                    {
                        idxRefMatrix = idxMatrix - (hevcScalingList->scaling_list_pred_matrix_id_delta[idxSize][idxMatrix]);
                        hevcSrcMatrix = &(hevcQmParams->ucScalingLists0[idxRefMatrix][0]);
                    }

                }
                else
                {
                    hevcSrcMatrix = &(hevcScalingList->scaling_list_4x4[idxMatrix][0]);
                    loadDiag = true;
                }
            }
            break;
            case H265_SCALING_LIST_8x8:
            {
                hevcDstMatrix = &(hevcQmParams->ucScalingLists1[idxMatrix][0]);

                if (0 == hevcScalingList->scaling_list_pred_mode_flag[idxSize][idxMatrix])
                {
                    if (0 == hevcScalingList->scaling_list_pred_matrix_id_delta[idxSize][idxMatrix])
                    {
                        if (idxMatrix < 3)
                        {
                            hevcSrcMatrix = &(CODECHAL_DECODE_HEVC_Default_8x8_Intra[0]);
                        }
                        else
                        {
                            hevcSrcMatrix = &(CODECHAL_DECODE_HEVC_Default_8x8_Inter[0]);
                        }
                    }
                    else
                    {
                        idxRefMatrix = idxMatrix - (hevcScalingList->scaling_list_pred_matrix_id_delta[idxSize][idxMatrix]);
                        hevcSrcMatrix = &(hevcQmParams->ucScalingLists1[idxRefMatrix][0]);
                    }
                }
                else
                {
                    hevcSrcMatrix = &(hevcScalingList->scaling_list_8x8[idxMatrix][0]);
                    loadDiag = true;
                }
            }
            break;
            case H265_SCALING_LIST_16x16:
            {
                hevcDstMatrix = &(hevcQmParams->ucScalingLists2[idxMatrix][0]);

                if (0 == hevcScalingList->scaling_list_pred_mode_flag[idxSize][idxMatrix])
                {
                    if (0 == hevcScalingList->scaling_list_pred_matrix_id_delta[idxSize][idxMatrix])
                    {
                        if (idxMatrix < 3)
                        {
                            hevcSrcMatrix = &(CODECHAL_DECODE_HEVC_Default_8x8_Intra[0]);
                        }
                        else
                        {
                            hevcSrcMatrix = &(CODECHAL_DECODE_HEVC_Default_8x8_Inter[0]);
                        }
                    }
                    else
                    {
                        idxRefMatrix = idxMatrix - (hevcScalingList->scaling_list_pred_matrix_id_delta[idxSize][idxMatrix]);
                        hevcSrcMatrix = &(hevcQmParams->ucScalingLists2[idxRefMatrix][0]);

                        hevcQmParams->ucScalingListDCCoefSizeID2[idxMatrix] = hevcQmParams->ucScalingListDCCoefSizeID2[idxRefMatrix];
                    }
                }
                else
                {
                    hevcSrcMatrix = &(hevcScalingList->scaling_list_16x16[idxMatrix][0]);
                    loadDiag = true;

                    hevcQmParams->ucScalingListDCCoefSizeID2[idxMatrix] = hevcScalingList->scaling_list_16x16_dc[idxMatrix];
                }
            }
            break;
            case H265_SCALING_LIST_32x32:
            {
                hevcDstMatrix = &(hevcQmParams->ucScalingLists3[idxMatrix][0]);

                if (0 == hevcScalingList->scaling_list_pred_mode_flag[idxSize][idxMatrix])
                {
                    if (0 == hevcScalingList->scaling_list_pred_matrix_id_delta[idxSize][idxMatrix])
                    {
                        if (idxMatrix < 1)
                        {
                            hevcSrcMatrix = &(CODECHAL_DECODE_HEVC_Default_8x8_Intra[0]);
                        }
                        else
                        {
                            hevcSrcMatrix = &(CODECHAL_DECODE_HEVC_Default_8x8_Inter[0]);
                        }
                    }
                    else
                    {
                        idxRefMatrix = idxMatrix - (hevcScalingList->scaling_list_pred_matrix_id_delta[idxSize][idxMatrix]);
                        hevcSrcMatrix = &(hevcQmParams->ucScalingLists3[idxRefMatrix][0]);

                        hevcQmParams->ucScalingListDCCoefSizeID3[idxMatrix] = hevcQmParams->ucScalingListDCCoefSizeID3[idxRefMatrix];
                    }
                }
                else
                {
                    hevcSrcMatrix = &(hevcScalingList->scaling_list_32x32[idxMatrix][0]);
                    loadDiag = true;

                    hevcQmParams->ucScalingListDCCoefSizeID3[idxMatrix] = hevcScalingList->scaling_list_32x32_dc[idxMatrix];
                }
            }
            break;
            default:
            {
                CENC_DECODE_ASSERTMESSAGE("%s: Wrong Matrix Size\n", __FUNCTION__);
            }
            }

            if ((nullptr != hevcDstMatrix) && (nullptr != hevcSrcMatrix))
            {

                if (H265_SCALING_LIST_4x4 != idxSize)
                {
                    if (loadDiag)
                    {
                        for (idx = 0; idx < 64; idx++)
                        {
                            hevcDstMatrix[CODECHAL_DECODE_HEVC_Qmatrix_Scan_8x8[idx]] = hevcSrcMatrix[idx];
                        }
                    }
                    else
                    {
                        for (idx = 0; idx < 64; idx++)
                        {
                            hevcDstMatrix[idx] = hevcSrcMatrix[idx];
                        }
                    }
                }
                else
                {
                    if (loadDiag)
                    {
                        for (idx = 0; idx < 16; idx++)
                        {
                            hevcDstMatrix[CODECHAL_DECODE_HEVC_Qmatrix_Scan_4x4[idx]] = hevcSrcMatrix[idx];
                        }
                    }
                    else
                    {
                        MOS_SecureMemcpy(&(hevcDstMatrix[0]), sizeof(CODECHAL_DECODE_HEVC_Default_4x4),
                            &(hevcSrcMatrix[0]), sizeof(CODECHAL_DECODE_HEVC_Default_4x4));
                    }
                }
            }
            else
            {
                CENC_DECODE_VERBOSEMESSAGE("Scaling Matrix %d Failed", idxMatrix);
            }
        }
    }

    return;
}

void CodechalCencDecodeHevcNext::ScalingListDefault(
    PCODECHAL_HEVC_IQ_MATRIX_PARAMS  hevcQmParams)
{
    uint8_t   idx;
    uint8_t   *hevcDstMatrix;

    CENC_DECODE_ASSERT(hevcQmParams);

    for (idx = 0; idx < 6; idx++)
    {
        hevcDstMatrix = &(hevcQmParams->ucScalingLists0[idx][0]);
        MOS_SecureMemcpy(hevcDstMatrix, sizeof(hevcQmParams->ucScalingLists0[idx]),
            &(CODECHAL_DECODE_HEVC_Default_4x4[0]), sizeof(CODECHAL_DECODE_HEVC_Default_4x4));

        if (idx < 3)
        {
            hevcDstMatrix = &(hevcQmParams->ucScalingLists1[idx][0]);
            MOS_SecureMemcpy(hevcDstMatrix, sizeof(hevcQmParams->ucScalingLists1[idx]),
                &(CODECHAL_DECODE_HEVC_Default_8x8_Intra[0]), sizeof(CODECHAL_DECODE_HEVC_Default_8x8_Intra));

            hevcDstMatrix = &(hevcQmParams->ucScalingLists2[idx][0]);
            MOS_SecureMemcpy(hevcDstMatrix, sizeof(hevcQmParams->ucScalingLists2[idx]),
                &(CODECHAL_DECODE_HEVC_Default_8x8_Intra[0]), sizeof(CODECHAL_DECODE_HEVC_Default_8x8_Intra));
        }
        else
        {
            hevcDstMatrix = &(hevcQmParams->ucScalingLists1[idx][0]);
            MOS_SecureMemcpy(hevcDstMatrix, sizeof(hevcQmParams->ucScalingLists1[idx]),
                &(CODECHAL_DECODE_HEVC_Default_8x8_Inter[0]), sizeof(CODECHAL_DECODE_HEVC_Default_8x8_Inter));

            hevcDstMatrix = &(hevcQmParams->ucScalingLists2[idx][0]);
            MOS_SecureMemcpy(hevcDstMatrix, sizeof(hevcQmParams->ucScalingLists2[idx]),
                &(CODECHAL_DECODE_HEVC_Default_8x8_Inter[0]), sizeof(CODECHAL_DECODE_HEVC_Default_8x8_Inter));
        }
    }

    MOS_SecureMemcpy(&(hevcQmParams->ucScalingLists3[0][0]), sizeof(hevcQmParams->ucScalingLists3[0]),
        &(CODECHAL_DECODE_HEVC_Default_8x8_Intra[0]), sizeof(CODECHAL_DECODE_HEVC_Default_8x8_Intra));

    MOS_SecureMemcpy(&(hevcQmParams->ucScalingLists3[1][0]), sizeof(hevcQmParams->ucScalingLists3[1]),
        &(CODECHAL_DECODE_HEVC_Default_8x8_Inter[0]), sizeof(CODECHAL_DECODE_HEVC_Default_8x8_Inter));

    memset(&(hevcQmParams->ucScalingListDCCoefSizeID2[0]),
        0x10, (6 * sizeof(uint8_t)));

    memset(&(hevcQmParams->ucScalingListDCCoefSizeID3[0]),
        0x10, (2 * sizeof(uint8_t)));

    return;
}

void CodechalCencDecodeHevcNext::ScalingListFlat(
    PCODECHAL_HEVC_IQ_MATRIX_PARAMS  hevcQmParams)
{
    uint8_t i;

    CENC_DECODE_ASSERT(hevcQmParams);

    for (i = 0; i < 6; i++)
    {
        memset(&(hevcQmParams->ucScalingLists0[i][0]),
            0x10, (sizeof(hevcQmParams->ucScalingLists0[i])));

        memset(&(hevcQmParams->ucScalingLists1[i][0]),
            0x10, (sizeof(hevcQmParams->ucScalingLists1[i])));

        memset(&(hevcQmParams->ucScalingLists2[i][0]),
            0x10, (sizeof(hevcQmParams->ucScalingLists2[i])));

        if (i < 2)
        {
            memset(&(hevcQmParams->ucScalingLists3[i][0]),
                0x10, (sizeof(hevcQmParams->ucScalingLists3[i])));
        }
    }

    memset(&(hevcQmParams->ucScalingListDCCoefSizeID2[0]),
        0x10, (6 * sizeof(uint8_t)));

    memset(&(hevcQmParams->ucScalingListDCCoefSizeID3[0]),
        0x10, (2 * sizeof(uint8_t)));

}

MOS_STATUS CodechalCencDecodeHevcNext::PackProfileTier(
    const h265_layer_profile_tier_level *profileTilerLevel,
        uint32_t bufSize,
        uint8_t  *accumulator,
        uint8_t  *accumulatorValidBits,
        uint8_t  *&outBitStreamBuf,
        uint32_t *byteWritten)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    // profile_space - u(2)
    WRITE_BITS(profileTilerLevel->layer_profile_space, 2, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
    // tier_flag - u(1)
    WRITE_BITS(profileTilerLevel->layer_tier_flag, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
    // profile_idc - u(5)
    WRITE_BITS(profileTilerLevel->layer_profile_idc, 5, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
    
    for (uint32_t i = 0; i < 32; i++)
    {
        // profile_compatibility_flag[i] - u(1)
        WRITE_BITS(profileTilerLevel->layer_profile_compatibility_flag[i], 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
    }
    // progressive_source_flag - u(1)
    WRITE_BITS(profileTilerLevel->layer_progressive_source_flag, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
    // interlaced_source_flag - u(1)
    WRITE_BITS(profileTilerLevel->layer_interlaced_source_flag, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
    // non_packed_constraint_flag - u(1)
    WRITE_BITS(profileTilerLevel->layer_non_packed_constraint_flag, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
    // frame_only_constraint_flag - u(1)
    WRITE_BITS(profileTilerLevel->layer_frame_only_constraint_flag, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
    // reserved 44 bits - u(44)
    WRITE_BITS(0, 32, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
    WRITE_BITS(0, 12, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);

    return eStatus;
}

MOS_STATUS CodechalCencDecodeHevcNext::PackPTL(
    const h265_profile_tier_level *profileTilerLevel,
    uint32_t                       maxNumSubLayersMinus1,
    uint32_t                       bufSize,
    uint8_t                        *accumulator,
    uint8_t                        *accumulatorValidBits,
    uint8_t                        *&outBitStreamBuf,
    uint32_t                       *byteWritten)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    eStatus = PackProfileTier(&profileTilerLevel->general_ptl, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
    if (eStatus != MOS_STATUS_SUCCESS)
    {
        return eStatus;
    }
    // general_level_idc - u(8)
    WRITE_BITS(profileTilerLevel->general_ptl.layer_level_idc, 8, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);

    for (uint32_t i = 0; maxNumSubLayersMinus1 <= H265_MAX_NUM_SUB_LAYER && i < maxNumSubLayersMinus1; i++)
    {
        // sub_layer_profile_present_flag[i] - u(1)
        WRITE_BITS(profileTilerLevel->sub_layer_profile_present_flag[i], 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
        // sub_layer_level_present_flag[i] - u(1)
        WRITE_BITS(profileTilerLevel->sub_layer_level_present_flag[i], 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
    }

    if (maxNumSubLayersMinus1 > 0)
    {
        for (uint32_t i = maxNumSubLayersMinus1; i < 8; i++)
        {
            // reserved_zero_2bits - u(2)
            WRITE_BITS(0, 2, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
        }
    }

    for(uint32_t i = 0; maxNumSubLayersMinus1 <= H265_MAX_NUM_SUB_LAYER && i < maxNumSubLayersMinus1; i++)
    {
        if (profileTilerLevel->sub_layer_profile_present_flag[i])
        {
            eStatus = PackProfileTier(&profileTilerLevel->sub_layer_ptl[i], bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
            if (eStatus != MOS_STATUS_SUCCESS)
            {
                return eStatus;
            }
        }
        if (profileTilerLevel->sub_layer_level_present_flag[i])
        {
            // sub_layer_level_idc[i] - u(8)
            WRITE_BITS(profileTilerLevel->sub_layer_ptl[i].layer_level_idc, 8, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
        }
    }

    return eStatus;
}

MOS_STATUS CodechalCencDecodeHevcNext::PackScalingMatrix(
    uint8_t        sizeNum,
    uint8_t        matrixSize,
    const uint8_t  *matrix,
    const uint8_t  *dc,
    uint32_t       bufSize,
    uint8_t        *accumulator,
    uint8_t        *accumulatorValidBits,
    uint8_t        *&outBitStreamBuf,
    uint32_t       *byteWritten)
{
    MOS_STATUS  eStatus = MOS_STATUS_SUCCESS;
    uint32_t    i = 0;
    int32_t     value = 0;
    int32_t     nextCoef = H265_SCALING_LIST_START_VALUE;

    if(sizeNum >= H265_SCALING_LIST_16x16)
    {
        value = (int32_t)(*dc - 8);
        ENCODE_EXP_GOLOMB(SIGNED_VALUE(value), SizeOfInBits(value), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
        nextCoef = (int32_t)*dc;
    }

    for(i = 0; i < matrixSize; i++)
    {
        value = (int32_t)(matrix[i] - nextCoef);
        if(value > 127)
        {
            value = value - 256;
        }
        if(value < -128)
        {
            value = value + 256;
        }
        ENCODE_EXP_GOLOMB(SIGNED_VALUE(value), SizeOfInBits(value), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
        nextCoef = matrix[i];
    }

    return eStatus;
}

MOS_STATUS CodechalCencDecodeHevcNext::PackScalingList(
    const h265_scaling_list  *scalingList,
    uint32_t                 bufSize,
    uint8_t                  *accumulator,
    uint8_t                  *accumulatorValidBits,
    uint8_t                  *&outBitStreamBuf,
    uint32_t                 *byteWritten)
{
    MOS_STATUS      eStatus = MOS_STATUS_SUCCESS;
    uint32_t        scalinglistmatrixsize[H265_SCALING_LIST_SIZE_NUM]   = {16,64,64,64};
    uint32_t        scalinglistsize[H265_SCALING_LIST_SIZE_NUM]         = {6,6,6,2};

    uint32_t        sizenum = 0, matrixnum = 0;
    uint8_t         matrixsize =0, dcreserved = 0;
    const uint8_t   *pMatrix = nullptr, *pDc = nullptr;

    for(sizenum = 0; sizenum < H265_SCALING_LIST_SIZE_NUM; sizenum++)
    {
        matrixsize = static_cast<uint8_t>(scalinglistmatrixsize[sizenum]);
        
        for(matrixnum = 0; matrixnum < scalinglistsize[sizenum]; matrixnum++)
        {
            switch(sizenum)
            {
                case H265_SCALING_LIST_4x4:
                {
                    pMatrix = &(scalingList->scaling_list_4x4[matrixnum][0]);
                    pDc     = &dcreserved;
                }
                break;
                case H265_SCALING_LIST_8x8:
                {
                    pMatrix = &(scalingList->scaling_list_8x8[matrixnum][0]);
                    pDc     = &dcreserved;
                }
                break;
                case H265_SCALING_LIST_16x16:
                {
                    pMatrix = &(scalingList->scaling_list_16x16[matrixnum][0]);
                    pDc     = &(scalingList->scaling_list_16x16_dc[matrixnum]);
                }
                break;
                case H265_SCALING_LIST_32x32:
                {
                    pMatrix = &(scalingList->scaling_list_32x32[matrixnum][0]);
                    pDc     = &(scalingList->scaling_list_32x32_dc[matrixnum]);
                }
                break;
                default:
                    CENC_DECODE_VERBOSEMESSAGE("Invalid scaling list type!");
                    return eStatus;
            }
            // scaling_list_pred_mode_flag - u(1)
            WRITE_BITS(scalingList->scaling_list_pred_mode_flag[sizenum][matrixnum], 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);

            if(scalingList->scaling_list_pred_mode_flag[sizenum][matrixnum] == false)
            {
                // scaling_list_pred_matrix_id_delta[sizenum][matrixnum], uvlc
                ENCODE_EXP_GOLOMB(scalingList->scaling_list_pred_matrix_id_delta[sizenum][matrixnum], SizeOfInBits(scalingList->scaling_list_pred_matrix_id_delta[sizenum][matrixnum]), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
            }
            else
            {
                eStatus = PackScalingMatrix(static_cast<uint8_t>(sizenum), matrixsize, pMatrix, pDc, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
                if (eStatus != MOS_STATUS_SUCCESS)
                {
                    return eStatus;
                }
            }
        }
    }

    return eStatus;
}

MOS_STATUS CodechalCencDecodeHevcNext::PackShortTermRefPicSet(
    const h265_reference_picture_set  *rps,
    uint32_t                          index,
    uint32_t                          numOfRps,
    uint32_t                          bufSize,
    uint8_t                           *accumulator,
    uint8_t                           *accumulatorValidBits,
    uint8_t                           *&outBitStreamBuf,
    uint32_t                          *byteWritten)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    if (index > 0)
    {
        // inter_ref_pic_set_prediction_flag - u(1)
        WRITE_BITS(rps->rps_inter_ref_pic_set_prediction_flag, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
    }

    if (rps->rps_inter_ref_pic_set_prediction_flag)
    {
        if (index == numOfRps)
        {
            // rps_delta_idx_minus1 - uvlc
            ENCODE_EXP_GOLOMB(rps->rps_delta_idx_minus1, SizeOfInBits(rps->rps_delta_idx_minus1), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
        }
        // delta_rps_sign - u(1)
        WRITE_BITS(rps->rps_delta_rps_sign, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
        // abs_delta_rps_minus1 - uvlc
        ENCODE_EXP_GOLOMB(rps->rps_abs_delta_rps_minus1, SizeOfInBits(rps->rps_abs_delta_rps_minus1), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
        for (uint32_t i = 0; i < rps->rps_num_refidc; i++)
        {
            WRITE_BITS(rps->rps_used_by_curr_pic_flag[i] == 1 ? 1 : 0, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
            if (rps->rps_used_by_curr_pic_flag[i] == 0)
            {
                WRITE_BITS(rps->rps_use_delta_flag[i] == 1 ? 1 : 0, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
            }
        }
    }
    else
    {
        // num_negative_pics - uvlc
        ENCODE_EXP_GOLOMB(rps->rps_num_negative_pics, SizeOfInBits(rps->rps_num_negative_pics), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
        // num_positive_pics - uvlc
        ENCODE_EXP_GOLOMB(rps->rps_num_positive_pics, SizeOfInBits(rps->rps_num_positive_pics), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);

        int32_t prevPoc = 0;
        int32_t value = 0;
        for (uint32_t i = 0; i < rps->rps_num_negative_pics; i++)
        {
            // delta_poc_s0_minus1 - uvlc
            value = prevPoc - rps->rps_delta_poc[i] - 1;
            ENCODE_EXP_GOLOMB(value, SizeOfInBits(value), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
            prevPoc = rps->rps_delta_poc[i];
            // used_by_curr_pic_s0_flag - u(1)
            WRITE_BITS(rps->rps_is_used[i], 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
        }
        prevPoc = 0;
        for (uint32_t i = rps->rps_num_negative_pics; i < rps->rps_num_total_pics; i++)
        {
            value = rps->rps_delta_poc[i] - prevPoc - 1;
            // delta_poc_s1_minus1 - uvlc
            ENCODE_EXP_GOLOMB(value, SizeOfInBits(value), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
            prevPoc = rps->rps_delta_poc[i];
            // used_by_curr_pic_s1_flag - u(1)
            WRITE_BITS(rps->rps_is_used[i], 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
        }
    }

    return eStatus;
}

MOS_STATUS CodechalCencDecodeHevcNext::PackHrdParameters(
    const h265_hrd_parameters  *hrd,
    uint32_t                   maxNumSubLayersMinus1,
    uint32_t                   bufSize,
    uint8_t                    *accumulator,
    uint8_t                    *accumulatorValidBits,
    uint8_t                    *&outBitStreamBuf,
    uint32_t                   *byteWritten)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    // nal_hrd_parameters_present_flag - u(1)
    WRITE_BITS(hrd->nal_hrd_parameters_present_flag, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
    // vcl_hrd_parameters_present_flag - u(1)
    WRITE_BITS(hrd->vcl_hrd_parameters_present_flag, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);

    if(hrd->nal_hrd_parameters_present_flag || hrd->vcl_hrd_parameters_present_flag)
    {
        // sub_pic_hrd_params_present_flag - u(1)
        WRITE_BITS(hrd->hrd_sub_pic_cpb_params_present_flag, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
        if(hrd->hrd_sub_pic_cpb_params_present_flag)
        {
            // tick_divisor_minus2 - u(8)
            WRITE_BITS(hrd->hrd_tick_divisor_minus2, 8, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
            // du_cpb_removal_delay_increment_length_minus1 - u(5)
            WRITE_BITS(hrd->hrd_du_cpb_removal_delay_increment_length_minus1, 5, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
            // sub_pic_cpb_params_in_pic_timing_sei_flag - u(1)
            WRITE_BITS(hrd->hrd_sub_pic_cpb_params_in_pic_timing_sei_flag, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
            // dpb_output_delay_du_length_minus1 - u(5)
            WRITE_BITS(hrd->hrd_dpb_output_delay_du_length_minus1, 5, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
        }
        // bit_rate_scale - u(4)
        WRITE_BITS(hrd->hrd_bit_rate_scale, 4, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
        // cpb_size_scale - u(4)
        WRITE_BITS(hrd->hrd_cpb_size_scale, 4, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
        if(hrd->hrd_sub_pic_cpb_params_present_flag)
        {
            // du_cpb_size_scale - u(4)
            WRITE_BITS(hrd->hrd_cpb_size_du_scale, 4, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
        }
        // initial_cpb_removal_delay_length_minus1 - u(5)
        WRITE_BITS(hrd->hrd_initial_cpb_removal_delay_length_minus1, 5, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
        // au_cpb_removal_delay_length_minus1 - u(5)
        WRITE_BITS(hrd->hrd_au_cpb_removal_delay_length_minus1, 5, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
        // dpb_output_delay_length_minus1 - u(5)
        WRITE_BITS(hrd->hrd_dpb_output_delay_length_minus1, 5, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
    }

    for(uint32_t i = 0; maxNumSubLayersMinus1 < H265_MAX_NUM_SUB_LAYER && i <= maxNumSubLayersMinus1; i++)
    {
        // fixed_pic_rate_general_flag[i] - u(1)
        WRITE_BITS(hrd->hrd_fixed_pic_rate_general_flag[i], 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
        bool fixedPixRateWithinCvsFlag = true;
        if(!hrd->hrd_fixed_pic_rate_general_flag[i])
        {
            fixedPixRateWithinCvsFlag = hrd->hrd_fixed_pic_rate_within_cvs_flag[i];
            WRITE_BITS(fixedPixRateWithinCvsFlag, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
        }
        if(fixedPixRateWithinCvsFlag)
        {
            // elemental_duration_in_tc_minus1[i] - uvlc
            ENCODE_EXP_GOLOMB(hrd->hrd_elemental_duration_in_tc_minus1[i], SizeOfInBits(hrd->hrd_elemental_duration_in_tc_minus1[i]), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
        }
        else
        {
            // low_delay_hrd_flag[i] - u(1)
            WRITE_BITS(hrd->hrd_low_delay_hrd_flag[i], 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
        }
        if (!hrd->hrd_low_delay_hrd_flag[i])
        {
            // cpb_cnt_minus1 - uvlc
            ENCODE_EXP_GOLOMB(hrd->hrd_cpb_cnt_minus1[i], SizeOfInBits(hrd->hrd_cpb_cnt_minus1[i]), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
        }

        if(hrd->nal_hrd_parameters_present_flag)
        {
            for (uint32_t j = 0; hrd->hrd_cpb_cnt_minus1[i] < H265_MAX_CPB_CNT && j <= hrd->hrd_cpb_cnt_minus1[i]; j++)
            {
                // bit_rate_value_minus1 - uvlc
                ENCODE_EXP_GOLOMB(hrd->hrd_sublayer_nal[i].bit_rate_value_minus1[j], SizeOfInBits(hrd->hrd_sublayer_nal[i].bit_rate_value_minus1[j]), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
                // cpb_size_value_minus1 - uvlc
                ENCODE_EXP_GOLOMB(hrd->hrd_sublayer_nal[i].cpb_size_value_minus1[j], SizeOfInBits(hrd->hrd_sublayer_nal[i].cpb_size_value_minus1[j]), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
                if(hrd->hrd_sub_pic_cpb_params_present_flag)
                {
                    // cpb_size_du_value_minus1 - uvlc
                    ENCODE_EXP_GOLOMB(hrd->hrd_sublayer_nal[i].cpb_size_du_value_minus1[j], SizeOfInBits(hrd->hrd_sublayer_nal[i].cpb_size_du_value_minus1[j]), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
                    // bit_rate_du_value_minus1 - uvlc
                    ENCODE_EXP_GOLOMB(hrd->hrd_sublayer_nal[i].bit_rate_du_value_minus1[j], SizeOfInBits(hrd->hrd_sublayer_nal[i].bit_rate_du_value_minus1[j]), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
                }
                // cbr_flag - u(1)
                WRITE_BITS(hrd->hrd_sublayer_nal[i].cbr_flag[j], 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
            }
        }
        if (hrd->vcl_hrd_parameters_present_flag)
        {
            for (uint32_t j = 0; hrd->hrd_cpb_cnt_minus1[i] < H265_MAX_CPB_CNT && j <= hrd->hrd_cpb_cnt_minus1[i]; j++)
            {
                // bit_rate_value_minus1 - uvlc
                ENCODE_EXP_GOLOMB(hrd->hrd_sublayer_vcl[i].bit_rate_value_minus1[j], SizeOfInBits(hrd->hrd_sublayer_vcl[i].bit_rate_value_minus1[j]), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
                // cpb_size_value_minus1 - uvlc
                ENCODE_EXP_GOLOMB(hrd->hrd_sublayer_vcl[i].cpb_size_value_minus1[j], SizeOfInBits(hrd->hrd_sublayer_vcl[i].cpb_size_value_minus1[j]), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
                if(hrd->hrd_sub_pic_cpb_params_present_flag)
                {
                    // cpb_size_du_value_minus1 - uvlc
                    ENCODE_EXP_GOLOMB(hrd->hrd_sublayer_vcl[i].cpb_size_du_value_minus1[j], SizeOfInBits(hrd->hrd_sublayer_vcl[i].cpb_size_du_value_minus1[j]), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
                    // bit_rate_du_value_minus1 - uvlc
                    ENCODE_EXP_GOLOMB(hrd->hrd_sublayer_vcl[i].bit_rate_du_value_minus1[j], SizeOfInBits(hrd->hrd_sublayer_vcl[i].bit_rate_du_value_minus1[j]), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
                }
                // cbr_flag - u(1)
                WRITE_BITS(hrd->hrd_sublayer_vcl[i].cbr_flag[j], 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
            }
        }
    }

    return eStatus;
}

MOS_STATUS CodechalCencDecodeHevcNext::PackVUI(
    const h265_vui_parameters  *vui,
    uint32_t                   maxNumSubLayersMinus1,
    uint32_t                   bufSize,
    uint8_t                    *accumulator,
    uint8_t                    *accumulatorValidBits,
    uint8_t                    *&outBitStreamBuf,
    uint32_t                   *byteWritten)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    // aspect_ratio_info_present_flag - u(1)
    WRITE_BITS(vui->vui_aspect_ratio_info_present_flag, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
    if (vui->vui_aspect_ratio_info_present_flag)
    {
        // aspect_ratio_idc - u(8)
        WRITE_BITS(vui->vui_aspect_ratio_idc, 8, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
        if (vui->vui_aspect_ratio_idc == 255)
        {
            // sar_width - u(16)
            WRITE_BITS(vui->vui_sar_width, 16, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
            // sar_height - u(16)
            WRITE_BITS(vui->vui_sar_height, 16, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
        }
    }
    // overscan_info_present_flag - u(1)
    WRITE_BITS(vui->vui_overscan_info_present_flag, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
    if (vui->vui_overscan_info_present_flag)
    {
        // overscan_appropriate_flag - u(1)
        WRITE_BITS(vui->vui_overscan_appropriate_flag, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
    }
    // video_signal_type_present_flag - u(1)
    WRITE_BITS(vui->vui_video_signal_type_present_flag, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
    if (vui->vui_video_signal_type_present_flag)
    {
        // video_format - u(3)
        WRITE_BITS(vui->vui_video_format, 3, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
        // video_full_range_flag - u(1)
        WRITE_BITS(vui->vui_video_full_range_flag, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
        // colour_description_present_flag - u(1)
        WRITE_BITS(vui->vui_colour_description_present_flag, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
        if (vui->vui_colour_description_present_flag)
        {
            // colour_primaries - u(8)
            WRITE_BITS(vui->vui_colour_primaries, 8, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
            // transfer_characteristics - u(8)
            WRITE_BITS(vui->vui_transfer_characteristics, 8, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
            // matrix_coeffs - u(8)
            WRITE_BITS(vui->vui_matrix_coefficients, 8, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
        }
    }
    // chroma_loc_info_present_flag - u(1)
    WRITE_BITS(vui->vui_chroma_loc_info_present_flag, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
    if (vui->vui_chroma_loc_info_present_flag)
    {
        // chroma_sample_loc_type_top_field - uvlc
        ENCODE_EXP_GOLOMB(vui->vui_chroma_sample_loc_type_top_field, SizeOfInBits(vui->vui_chroma_sample_loc_type_top_field), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
        // chroma_sample_loc_type_bottom_field - uvlc
        ENCODE_EXP_GOLOMB(vui->vui_chroma_sample_loc_type_bottom_field, SizeOfInBits(vui->vui_chroma_sample_loc_type_bottom_field), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
    }
    // neutral_chroma_indication_flag - u(1)
    WRITE_BITS(vui->vui_neutral_chroma_indication_flag, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
    // field_seq_flag - u(1)
    WRITE_BITS(vui->vui_field_seq_flag, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
    // frame_field_info_present_flag - u(1)
    WRITE_BITS(vui->vui_frame_field_info_present_flag, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
    // default_display_window_flag - u(1)
    WRITE_BITS(vui->vui_default_display_window_flag, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);

    if(vui->vui_default_display_window_flag)
    {
        // def_disp_win_left_offset - uvlc
        ENCODE_EXP_GOLOMB(vui->vui_def_disp_win_left_offset, SizeOfInBits(vui->vui_def_disp_win_left_offset), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
        // def_disp_win_right_offset - uvlc
        ENCODE_EXP_GOLOMB(vui->vui_def_disp_win_right_offset, SizeOfInBits(vui->vui_def_disp_win_right_offset), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
        // def_disp_win_top_offset - uvlc
        ENCODE_EXP_GOLOMB(vui->vui_def_disp_win_top_offset, SizeOfInBits(vui->vui_def_disp_win_top_offset), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
        // def_disp_win_bottom_offset - uvlc
        ENCODE_EXP_GOLOMB(vui->vui_def_disp_win_bottom_offset, SizeOfInBits(vui->vui_def_disp_win_bottom_offset), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
    }

    // vui_timing_info_present_flag - u(1)
    WRITE_BITS(vui->vui_timing_info_present_flag, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
    if(vui->vui_timing_info_present_flag)
    {
        // vui_num_units_in_tick - u(32)
        WRITE_BITS(vui->vui_num_units_in_tick, 32, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
        // vui_time_scale - u(32)
        WRITE_BITS(vui->vui_time_scale, 32, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
        // vui_poc_proportional_to_timing_flag - u(1)
        WRITE_BITS(vui->vui_poc_proportional_to_timing_flag, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
        if(vui->vui_poc_proportional_to_timing_flag)
        {
            // vui_num_ticks_poc_diff_one_minus1 - uvlc
            ENCODE_EXP_GOLOMB(vui->vui_num_ticks_poc_diff_one_minus1, SizeOfInBits(vui->vui_num_ticks_poc_diff_one_minus1), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
        }
        // vui_hrd_parameters_present_flag - u(1)
        WRITE_BITS(vui->vui_hrd_parameters_present_flag, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
        if(vui->vui_hrd_parameters_present_flag)
        {
            eStatus = PackHrdParameters(&vui->vui_hrd_parameters, maxNumSubLayersMinus1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
            if (eStatus != MOS_STATUS_SUCCESS)
            {
                return eStatus;
            }
        }
    }
    // bitstream_restriction_flag - u(1)
    WRITE_BITS(vui->vui_bitstream_restriction_flag, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
    if (vui->vui_bitstream_restriction_flag)
    {
        // tiles_fixed_structure_flag - u(1)
        WRITE_BITS(vui->vui_tiles_fixed_structure_flag, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
        // motion_vectors_over_pic_boundaries_flag - u(1)
        WRITE_BITS(vui->vui_motion_vectors_over_pic_boundaries_flag, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
        // restricted_ref_pic_lists_flag - u(1)
        WRITE_BITS(vui->vui_restricted_ref_pic_lists_flag, 1, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten, true);
        // min_spatial_segmentation_idc - uvlc
        ENCODE_EXP_GOLOMB(vui->vui_min_spatial_segmentation_idc, SizeOfInBits(vui->vui_min_spatial_segmentation_idc), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
        // max_bytes_per_pic_denom - uvlc
        ENCODE_EXP_GOLOMB(vui->vui_max_bytes_per_pic_denom, SizeOfInBits(vui->vui_max_bytes_per_pic_denom), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
        // max_bits_per_min_cu_denom - uvlc
        ENCODE_EXP_GOLOMB(vui->vui_max_bits_per_mincu_denom, SizeOfInBits(vui->vui_max_bits_per_mincu_denom), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
        // log2_max_mv_length_horizontal - uvlc
        ENCODE_EXP_GOLOMB(vui->vui_log2_max_mv_length_horizontal, SizeOfInBits(vui->vui_log2_max_mv_length_horizontal), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
        // log2_max_mv_length_vertical - uvlc
        ENCODE_EXP_GOLOMB(vui->vui_log2_max_mv_length_vertical, SizeOfInBits(vui->vui_log2_max_mv_length_vertical), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
    }

    return eStatus;
}

MOS_STATUS CodechalCencDecodeHevcNext::EncodeSPS(
    const PCODECHAL_DECRYPT_STATUS_REPORT_HEVC statusDriver,
    uint32_t                                   bufSize,
    uint8_t                                    *&outBitStreamBuf,
    uint32_t                                   &writtenBytes)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    if (statusDriver == nullptr || outBitStreamBuf == nullptr)
    {
        CENC_DECODE_ASSERTMESSAGE("Invalid input parameters");
        return MOS_STATUS_NULL_POINTER;
    }

    aligned_h265_seq_param_set_t *spsHeader = &statusDriver->SeqParams;
    uint8_t  accumulator = 0;
    uint8_t  accumulatorValidBits = 0;
    uint8_t  *pAccumulator = &accumulator;
    uint8_t  *pAccumulatorValidBits = &accumulatorValidBits;
    uint32_t *pWrittenBytes = &writtenBytes;

    // SPS starts with 00, 00, 01, 42
    WRITE_BITS( 0, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, false);
    WRITE_BITS( 0, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, false);
    WRITE_BITS( 1, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, false);
    WRITE_BITS(0x42, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, false);
    WRITE_BITS( 1, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, false);

    // sps_video_parameter_set_id - u(4)
    WRITE_BITS(spsHeader->sps_video_parameter_set_id, 4, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // sps_max_sub_layers_minus1 - u(3)
    WRITE_BITS(spsHeader->sps_max_sub_layers_minus1, 3, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // sps_temporal_id_nesting_flag - u(1)
    WRITE_BITS(spsHeader->sps_temporal_id_nesting_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // pack Profile Tiler Level
    eStatus = PackPTL(&spsHeader->sps_profile_tier_level, spsHeader->sps_max_sub_layers_minus1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    if (eStatus != MOS_STATUS_SUCCESS)
    {
        return eStatus;
    }
    // sps_seq_parameter_set_id - uvlc
    ENCODE_EXP_GOLOMB(spsHeader->sps_seq_parameter_set_id, SizeOfInBits(spsHeader->sps_seq_parameter_set_id), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    // chroma_format_idc - uvlc
    ENCODE_EXP_GOLOMB(spsHeader->sps_chroma_format_idc, SizeOfInBits(spsHeader->sps_chroma_format_idc), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    if (spsHeader->sps_chroma_format_idc == 3) // CHROMA_444
    {
        WRITE_BITS(spsHeader->sps_separate_colour_plane_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    }
    // pic_width_in_luma_samples -uvlc
    ENCODE_EXP_GOLOMB(spsHeader->sps_pic_width_in_luma_samples, SizeOfInBits(spsHeader->sps_pic_width_in_luma_samples), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    // pic_height_in_luma_samples - uvlc
    ENCODE_EXP_GOLOMB(spsHeader->sps_pic_height_in_luma_samples, SizeOfInBits(spsHeader->sps_pic_height_in_luma_samples), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    // conformance_window_flag - u(1)
    WRITE_BITS(spsHeader->sps_conformance_window_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    if (spsHeader->sps_conformance_window_flag)
    {
        // conf_win_left_offset - uvlc
        ENCODE_EXP_GOLOMB(spsHeader->sps_conf_win_left_offset, SizeOfInBits(spsHeader->sps_conf_win_left_offset), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
        // conf_win_right_offset - uvlc
        ENCODE_EXP_GOLOMB(spsHeader->sps_conf_win_right_offset, SizeOfInBits(spsHeader->sps_conf_win_right_offset), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
        // conf_win_top_offset - uvlc
        ENCODE_EXP_GOLOMB(spsHeader->sps_conf_win_top_offset, SizeOfInBits(spsHeader->sps_conf_win_top_offset), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
        // conf_win_bottom_offset - uvlc
        ENCODE_EXP_GOLOMB(spsHeader->sps_conf_win_bottom_offset, SizeOfInBits(spsHeader->sps_conf_win_bottom_offset), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    }

    // bit_depth_luma_minus8 - uvlc
    ENCODE_EXP_GOLOMB(spsHeader->sps_bit_depth_luma_minus8, SizeOfInBits(spsHeader->sps_bit_depth_luma_minus8), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    // bit_depth_chroma_minus8 - uvlc
    ENCODE_EXP_GOLOMB(spsHeader->sps_bit_depth_chroma_minus8, SizeOfInBits(spsHeader->sps_bit_depth_chroma_minus8), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    // log2_max_pic_order_cnt_lsb_minus4 - uvlc
    ENCODE_EXP_GOLOMB(spsHeader->sps_log2_max_pic_order_cnt_lsb_minus4, SizeOfInBits(spsHeader->sps_log2_max_pic_order_cnt_lsb_minus4), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);

    // sps_sub_layer_ordering_info_present_flag - u(1)
    const bool subLayerOrderingInfoPresentFlag = true;
    WRITE_BITS(subLayerOrderingInfoPresentFlag/*spsHeader->sps_sub_layer_ordering_info_present_flag*/, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true); // TODO: why not use the value from HuC?
    for(uint32_t i = 0; spsHeader->sps_max_sub_layers_minus1 < H265_MAX_NUM_TLAYER && i <= spsHeader->sps_max_sub_layers_minus1; i++)
    {
        // sps_max_dec_pic_buffering_minus1[i] - uvlc
        ENCODE_EXP_GOLOMB(spsHeader->sps_max_dec_pic_buffering_minus1[i], SizeOfInBits(spsHeader->sps_max_dec_pic_buffering_minus1[i]), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
        // sps_max_num_reorder_pics[i] - uvlc
        ENCODE_EXP_GOLOMB(spsHeader->sps_max_num_reorder_pics[i], SizeOfInBits(spsHeader->sps_max_num_reorder_pics[i]), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
        // sps_max_latency_increase_plus1[i] - uvlc
        ENCODE_EXP_GOLOMB(spsHeader->sps_max_latency_increase[i], SizeOfInBits(spsHeader->sps_max_latency_increase[i]), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
        if (!subLayerOrderingInfoPresentFlag)
        {
            break;
        }
    }

    // log2_min_luma_coding_block_size_minus3 - uvlc
    ENCODE_EXP_GOLOMB(spsHeader->sps_log2_min_luma_coding_block_size_minus3, SizeOfInBits(spsHeader->sps_log2_min_luma_coding_block_size_minus3), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    // log2_diff_max_min_luma_coding_block_size - uvlc
    ENCODE_EXP_GOLOMB(spsHeader->sps_log2_diff_max_min_luma_coding_block_size, SizeOfInBits(spsHeader->sps_log2_diff_max_min_luma_coding_block_size), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    // log2_min_luma_transform_block_size_minus2 - uvlc
    ENCODE_EXP_GOLOMB(spsHeader->sps_log2_min_transform_block_size_minus2, SizeOfInBits(spsHeader->sps_log2_min_transform_block_size_minus2), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    // log2_diff_max_min_luma_transform_block_size - uvlc
    ENCODE_EXP_GOLOMB(spsHeader->sps_log2_diff_max_min_transform_block_size, SizeOfInBits(spsHeader->sps_log2_diff_max_min_transform_block_size), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    // max_transform_hierarchy_depth_inter - uvlc
    ENCODE_EXP_GOLOMB(spsHeader->sps_max_transform_hierarchy_depth_inter, SizeOfInBits(spsHeader->sps_max_transform_hierarchy_depth_inter), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    // max_transform_hierarchy_depth_intra - uvlc
    ENCODE_EXP_GOLOMB(spsHeader->sps_max_transform_hierarchy_depth_intra, SizeOfInBits(spsHeader->sps_max_transform_hierarchy_depth_intra), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    // scaling_list_enabled_flag - u(1)
    WRITE_BITS(spsHeader->sps_scaling_list_enabled_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true); // TODO: why not use the value from HuC?

    if (spsHeader->sps_scaling_list_enabled_flag)
    {
        // sps_scaling_list_data_present_flag - u(1)
        WRITE_BITS(spsHeader->sps_scaling_list_data_present_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        if (spsHeader->sps_scaling_list_data_present_flag)
        {
            eStatus = PackScalingList(&spsHeader->sps_scaling_list, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
            if (eStatus != MOS_STATUS_SUCCESS)
            {
                return eStatus;
            }
        }
    }
    // amp_enabled_flag - u(1)
    WRITE_BITS(spsHeader->sps_amp_enabled_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // sample_adaptive_offset_enabled_flag - u(1)
    WRITE_BITS(spsHeader->sps_sample_adaptive_offset_enabled_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // pcm_enabled_flag - u(1)
    WRITE_BITS(spsHeader->sps_pcm_enabled_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);

    if(spsHeader->sps_pcm_enabled_flag)
    {
        // pcm_sample_bit_depth_luma_minus1 - u(4)
        WRITE_BITS(spsHeader->sps_pcm_sample_bit_depth_luma_minus1, 4, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        // pcm_sample_bit_depth_chroma_minus1 - u(4)
        WRITE_BITS(spsHeader->sps_pcm_sample_bit_depth_chroma_minus1, 4, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        // log2_min_pcm_luma_coding_block_size_minus3 - uvlc
        ENCODE_EXP_GOLOMB(spsHeader->sps_log2_min_pcm_luma_coding_block_size_minus3, SizeOfInBits(spsHeader->sps_log2_min_pcm_luma_coding_block_size_minus3), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
        // log2_diff_max_min_pcm_luma_coding_block_size - uvlc
        ENCODE_EXP_GOLOMB(spsHeader->sps_log2_diff_max_min_pcm_luma_coding_block_size, SizeOfInBits(spsHeader->sps_log2_diff_max_min_pcm_luma_coding_block_size), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
        // pcm_loop_filter_disable_flag - u(1)
        WRITE_BITS(spsHeader->sps_pcm_loop_filter_disable_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    }

    // sps_num_short_term_ref_pic_sets - uvlc
    ENCODE_EXP_GOLOMB(spsHeader->sps_num_short_term_ref_pic_sets, SizeOfInBits(spsHeader->sps_num_short_term_ref_pic_sets), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    for(uint32_t i = 0; i < spsHeader->sps_num_short_term_ref_pic_sets; i++)
    {
        eStatus = PackShortTermRefPicSet(&spsHeader->sps_short_term_ref_pic_set[i], i, 
            spsHeader->sps_num_short_term_ref_pic_sets, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
        if (eStatus != MOS_STATUS_SUCCESS)
        {
            return eStatus;
        }
    }
    // long_term_ref_pics_present_flag - u(1)
    WRITE_BITS(spsHeader->sps_long_term_ref_pics_present_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);

    if (spsHeader->sps_long_term_ref_pics_present_flag)
    {
        // num_long_term_ref_pics_sps - uvlc
        ENCODE_EXP_GOLOMB(spsHeader->sps_num_long_term_ref_pics_sps, SizeOfInBits(spsHeader->sps_num_long_term_ref_pics_sps), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
        for (uint32_t i = 0; i < spsHeader->sps_num_long_term_ref_pics_sps; i++)
        {
            // lt_ref_pic_poc_lsb_sps[i] - u(sps_log2_max_pic_order_cnt_lsb_minus4 + 4)
            WRITE_BITS(spsHeader->sps_lt_ref_pic_poc_lsb_sps[i], spsHeader->sps_log2_max_pic_order_cnt_lsb_minus4 + 4, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
            // used_by_curr_pic_lt_sps_flag[i] - u(1)
            WRITE_BITS(spsHeader->sps_used_by_curr_pic_lt_sps_flag[i], 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        }
    }
    // sps_temporal_mvp_enabled_flag - u(1)
    WRITE_BITS(spsHeader->sps_temporal_mvp_enabled_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // strong_intra_smoothing_enable_flag - u(1)
    WRITE_BITS(spsHeader->sps_strong_intra_smoothing_enabled_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // vui_parameters_present_flag - u(1)
    WRITE_BITS(spsHeader->sps_vui_parameters_present_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);

    if (spsHeader->sps_vui_parameters_present_flag)
    {
        eStatus = PackVUI(&spsHeader->sps_vui, spsHeader->sps_max_sub_layers_minus1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
        if (eStatus != MOS_STATUS_SUCCESS)
        {
            return eStatus;
        }
    }

    // sps_extension_flag - u(1)
    // doesn't support extension
    WRITE_BITS(0, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);

    // rbsp trailing bits
    WRITE_BITS(1, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    if (accumulatorValidBits != 0)
    {
        WRITE_BITS(0, SizeOfInBits(uint8_t) - accumulatorValidBits, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    }

    return eStatus;
}

MOS_STATUS CodechalCencDecodeHevcNext::EncodePPS(
    const PCODECHAL_DECRYPT_STATUS_REPORT_HEVC statusDriver,
    uint32_t                                   bufSize,
    uint8_t                                    *&outBitStreamBuf,
    uint32_t                                   &writtenBytes)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    if (statusDriver == nullptr || outBitStreamBuf == nullptr)
    {
        CENC_DECODE_ASSERTMESSAGE("Invalid input parameters");
        return MOS_STATUS_NULL_POINTER;;
    }

    aligned_h265_pic_param_set_t  *ppsHeader             = &statusDriver->PicParams;
    uint32_t                      byteWritten            = 0;
    uint8_t                       accumulator            = 0;
    uint8_t                       accumulatorValidBits   = 0;
    uint8_t                       *pAccumulator          = &accumulator;
    uint8_t                       *pAccumulatorValidBits = &accumulatorValidBits;
    uint32_t                      *pWrittenBytes         = &writtenBytes;

    // PPS starts with 00, 00, 01, 44
    WRITE_BITS( 0, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, false);
    WRITE_BITS( 0, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, false);
    WRITE_BITS( 1, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, false);
    WRITE_BITS(0x44, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, false);
    WRITE_BITS( 1, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, false);

    // pps_pic_parameter_set_id - uvlc
    ENCODE_EXP_GOLOMB(ppsHeader->pps_pic_parameter_set_id, SizeOfInBits(ppsHeader->pps_pic_parameter_set_id), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    // pps_seq_parameter_set_id - uvlc
    ENCODE_EXP_GOLOMB(ppsHeader->pps_seq_parameter_set_id, SizeOfInBits(ppsHeader->pps_seq_parameter_set_id), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    // dependent_slice_segments_enabled_flag - u(1)
    WRITE_BITS(ppsHeader->pps_dependent_slice_segments_enabled_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // output_flag_present_flag - u(1)
    WRITE_BITS(ppsHeader->pps_output_flag_present_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // num_extra_slice_header_bits - u(3)
    WRITE_BITS(ppsHeader->pps_num_extra_slice_header_bits, 3, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // sign_data_hiding_enabled_flag - (1)
    WRITE_BITS(ppsHeader->pps_sign_data_hiding_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // cabac_init_present_flag - u(1)
    WRITE_BITS(ppsHeader->pps_cabac_init_present_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // num_ref_idx_l0_default_active_minus1 - uvlc
    ENCODE_EXP_GOLOMB(ppsHeader->pps_num_ref_idx_l0_default_active_minus1, SizeOfInBits(ppsHeader->pps_num_ref_idx_l0_default_active_minus1), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    // num_ref_idx_l1_default_active_minus1 - uvlc
    ENCODE_EXP_GOLOMB(ppsHeader->pps_num_ref_idx_l1_default_active_minus1, SizeOfInBits(ppsHeader->pps_num_ref_idx_l1_default_active_minus1), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    // init_qp_minus26 - svlc
    ENCODE_EXP_GOLOMB(SIGNED_VALUE(ppsHeader->pps_init_qp_minus26), SizeOfInBits(ppsHeader->pps_init_qp_minus26), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    // constrained_intra_pred_flag - u(1)
    WRITE_BITS(ppsHeader->pps_constrained_intra_pred_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // transform_skip_enabled_flag - u(1)
    WRITE_BITS(ppsHeader->pps_transform_skip_enabled_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // cu_qp_delta_enabled_flag - u(1)
    WRITE_BITS(ppsHeader->pps_cu_qp_delta_enabled_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    if (ppsHeader->pps_cu_qp_delta_enabled_flag)
    {
        // diff_cu_qp_delta_depth - uvlc
        ENCODE_EXP_GOLOMB(ppsHeader->pps_diff_cu_qp_delta_depth, SizeOfInBits(ppsHeader->pps_diff_cu_qp_delta_depth), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    }
    // pps_cb_qp_offset - svlc
    ENCODE_EXP_GOLOMB(SIGNED_VALUE(ppsHeader->pps_cb_qp_offset), SizeOfInBits(ppsHeader->pps_cb_qp_offset), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    // pps_cr_qp_offset - svlc
    ENCODE_EXP_GOLOMB(SIGNED_VALUE(ppsHeader->pps_cr_qp_offset), SizeOfInBits(ppsHeader->pps_cr_qp_offset), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    // pps_slice_chroma_qp_offsets_present_flag - u(1)
    WRITE_BITS(ppsHeader->pps_slice_chroma_qp_offsets_present_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // weighted_pred_flag - u(1)
    WRITE_BITS(ppsHeader->pps_weighted_pred_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // weighted_bipred_flag - u(1)
    WRITE_BITS(ppsHeader->pps_weighted_bipred_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // transquant_bypass_enabled_flag - u(1)
    WRITE_BITS(ppsHeader->pps_transquant_bypass_enabled_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // tiles_enabled_flag - u(1)
    WRITE_BITS(ppsHeader->pps_tiles_enabled_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // entropy_coding_sync_enabled_flag - u(1)
    WRITE_BITS(ppsHeader->pps_entropy_coding_sync_enabled_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    if (ppsHeader->pps_tiles_enabled_flag)
    {
        // num_tile_columns_minus1 - uvlc
        ENCODE_EXP_GOLOMB(ppsHeader->pps_num_tile_columns_minus1, SizeOfInBits(ppsHeader->pps_num_tile_columns_minus1), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
        // num_tile_rows_minus1 - uvlc
        ENCODE_EXP_GOLOMB(ppsHeader->pps_num_tile_rows_minus1, SizeOfInBits(ppsHeader->pps_num_tile_rows_minus1), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
        // uniform_spacing_flag - u(1)
        WRITE_BITS(ppsHeader->pps_uniform_spacing_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        if (!ppsHeader->pps_uniform_spacing_flag)
        {
            for (uint32_t i = 0; i < ppsHeader->pps_num_tile_columns_minus1; i++)
            {
                // column_width_minus1 - uvlc
                ENCODE_EXP_GOLOMB(ppsHeader->pps_column_width_minus1[i], SizeOfInBits(ppsHeader->pps_column_width_minus1[i]), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
            }
            for (uint32_t i = 0; i < ppsHeader->pps_num_tile_rows_minus1; i++)
            {
                // row_height_minus1 - uvlc
                ENCODE_EXP_GOLOMB(ppsHeader->pps_row_height_minus1[i], SizeOfInBits(ppsHeader->pps_row_height_minus1[i]), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
            }
        }
        // loop_filter_across_tiles_enabled_flag - u(1)
        WRITE_BITS(ppsHeader->pps_loop_filter_across_tiles_enabled_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    }
    // pps_loop_filter_across_slices_enabled_flag - u(1)
    WRITE_BITS(ppsHeader->pps_loop_filter_across_slices_enabled_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // deblocking_filter_control_present_flag - u(1)
    WRITE_BITS(ppsHeader->pps_df_control_present_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    if (ppsHeader->pps_df_control_present_flag)
    {
        // deblocking_filter_override_enabled_flag - u(1)
        WRITE_BITS(ppsHeader->pps_deblocking_filter_override_enabled_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        // pps_deblocking_filter_disabled_flag - u(1)
        WRITE_BITS(ppsHeader->pps_disable_deblocking_filter_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        if (!ppsHeader->pps_disable_deblocking_filter_flag)
        {
            // pps_beta_offset_div2 - u(1)
            ENCODE_EXP_GOLOMB(SIGNED_VALUE(ppsHeader->pps_beta_offset_div2), SizeOfInBits(ppsHeader->pps_beta_offset_div2), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
            // pps_tc_offset_div2 - u(1)
            ENCODE_EXP_GOLOMB(SIGNED_VALUE(ppsHeader->pps_tc_offset_div2), SizeOfInBits(ppsHeader->pps_tc_offset_div2), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
        }
    }

    // pps_scaling_list_data_present_flag - u(1)
    WRITE_BITS(ppsHeader->pps_scaling_list_data_present_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    if (ppsHeader->pps_scaling_list_data_present_flag)
    {
        eStatus = PackScalingList(&ppsHeader->pps_scaling_list, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
        if (eStatus != MOS_STATUS_SUCCESS)
        {
            return eStatus;
        }
    }
    // lists_modification_present_flag - u(1)
    WRITE_BITS(ppsHeader->pps_lists_modification_present_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // log2_parallel_merge_level_minus2 - uvlc
    ENCODE_EXP_GOLOMB(ppsHeader->pps_log2_parallel_merge_level_minus2, SizeOfInBits(ppsHeader->pps_log2_parallel_merge_level_minus2), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    // slice_segment_header_extension_present_flag - u(1)
    WRITE_BITS(ppsHeader->pps_slice_segment_header_extension_present_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // pps_extension_flag - u(1)
    // doesn't support extension
    WRITE_BITS(0, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);

    // rbsp trailing bits
    WRITE_BITS(1, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    if (accumulatorValidBits != 0)
    {
        WRITE_BITS(0, SizeOfInBits(uint8_t) - accumulatorValidBits, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    }

    return eStatus;
}

MOS_STATUS CodechalCencDecodeHevcNext::EncodeSeiBufferingPeriod(
    const PCODECHAL_DECRYPT_STATUS_REPORT_HEVC statusDriver,
    PCODECHAL_CENC_SEI                         seiBufPeriod)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;
    if (statusDriver == nullptr || seiBufPeriod == nullptr)
    {
        CENC_DECODE_ASSERTMESSAGE("Invalid input parameters");
        return MOS_STATUS_NULL_POINTER;
    }

    uint8_t                             *outBitStreamBuf       = seiBufPeriod->data;
    uint32_t                            bufSize                = sizeof(seiBufPeriod->data);
    uint32_t                            byteWritten            = 0;
    uint8_t                             accumulator            = 0;
    uint8_t                             accumulatorValidBits   = 0;
    uint8_t                             *pAccumulator          = &accumulator;
    uint8_t                             *pAccumulatorValidBits = &accumulatorValidBits;
    uint32_t                            *pWrittenBytes         = &byteWritten;
    aligned_h265_sei_buffering_period_t *bufPeriod             = &statusDriver->SeiBufferingPeriod;
    aligned_h265_seq_param_set_t        *spsHeader             = &statusDriver->SeqParams;
    h265_vui_parameters                 *vui                   = &spsHeader->sps_vui;
    h265_hrd_parameters                 *hrd                   = &vui->vui_hrd_parameters;

    // bp_seq_parameter_set_id - uvlc
    ENCODE_EXP_GOLOMB(bufPeriod->sei_bp_seq_parameter_set_id, SizeOfInBits(bufPeriod->sei_bp_seq_parameter_set_id), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    if (!hrd->hrd_sub_pic_cpb_params_present_flag)
    {
        // irap_cpb_params_present_flag - u(1)
        WRITE_BITS(bufPeriod->sei_bp_irap_cpb_params_present_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    }
    if (bufPeriod->sei_bp_irap_cpb_params_present_flag)
    {
        // cpb_delay_offset - u(hrd->hrd_au_cpb_removal_delay_length_minus1 + 1)
        WRITE_BITS(bufPeriod->sei_bp_cpb_delay_offset, hrd->hrd_au_cpb_removal_delay_length_minus1 + 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        // dpb_delay_offset - u(hrd->hrd_dpb_output_delay_length_minus1 + 1)
        WRITE_BITS(bufPeriod->sei_bp_dpb_delay_offset, hrd->hrd_dpb_output_delay_length_minus1 + 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    }
    // concatenation_flag - u(1)
    WRITE_BITS(bufPeriod->sei_bp_concatenation_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // au_cpb_removal_delay_delta_minus1 - u(hrd->hrd_au_cpb_removal_delay_length_minus1 + 1)
    WRITE_BITS(bufPeriod->sei_bp_au_cpb_removal_delay_delta_minus1, hrd->hrd_au_cpb_removal_delay_length_minus1 + 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);

    if (hrd->nal_hrd_parameters_present_flag)
    {
        for (uint8_t i = 0; hrd->hrd_cpb_cnt_minus1[0] < H265_MAX_CPB_CNT && i < hrd->hrd_cpb_cnt_minus1[0] + 1; i++)
        {
            // initial_cpb_removal_delay - u(hrd->hrd_initial_cpb_removal_delay_length_minus1 + 1)
            WRITE_BITS(bufPeriod->sei_bp_nal_initial_cpb_removal_delay[i], hrd->hrd_initial_cpb_removal_delay_length_minus1 + 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
            // initial_cpb_removal_delay_offset - u()
            WRITE_BITS(bufPeriod->sei_bp_nal_initial_cpb_removal_offset[i], hrd->hrd_initial_cpb_removal_delay_length_minus1 + 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
            if (hrd->hrd_sub_pic_cpb_params_present_flag || bufPeriod->sei_bp_irap_cpb_params_present_flag)
            {
                // initial_alt_cpb_removal_delay - u(hrd->hrd_initial_cpb_removal_delay_length_minus1 + 1)
                WRITE_BITS(bufPeriod->sei_bp_nal_initial_alt_cpb_removal_delay[i], hrd->hrd_initial_cpb_removal_delay_length_minus1 + 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
                // initial_alt_cpb_removal_delay_offset - u(hrd->hrd_initial_cpb_removal_delay_length_minus1 + 1)
                WRITE_BITS(bufPeriod->sei_bp_nal_initial_alt_cpb_removal_offset[i], hrd->hrd_initial_cpb_removal_delay_length_minus1 + 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
            }
        }
    }

    if (hrd->vcl_hrd_parameters_present_flag)
    {
        for (uint8_t i = 0; hrd->hrd_cpb_cnt_minus1[0] < H265_MAX_CPB_CNT && i < hrd->hrd_cpb_cnt_minus1[0] + 1; i++)
        {
            // initial_cpb_removal_delay - u(hrd->hrd_initial_cpb_removal_delay_length_minus1 + 1)
            WRITE_BITS(bufPeriod->sei_bp_vcl_initial_cpb_removal_delay[i], hrd->hrd_initial_cpb_removal_delay_length_minus1 + 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
            // initial_cpb_removal_delay_offset - u()
            WRITE_BITS(bufPeriod->sei_bp_vcl_initial_cpb_removal_offset[i], hrd->hrd_initial_cpb_removal_delay_length_minus1 + 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
            if (hrd->hrd_sub_pic_cpb_params_present_flag || bufPeriod->sei_bp_irap_cpb_params_present_flag)
            {
                // initial_alt_cpb_removal_delay - u(hrd->hrd_initial_cpb_removal_delay_length_minus1 + 1)
                WRITE_BITS(bufPeriod->sei_bp_vcl_initial_alt_cpb_removal_delay[i], hrd->hrd_initial_cpb_removal_delay_length_minus1 + 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
                // initial_alt_cpb_removal_delay_offset - u(hrd->hrd_initial_cpb_removal_delay_length_minus1 + 1)
                WRITE_BITS(bufPeriod->sei_bp_vcl_initial_alt_cpb_removal_offset[i], hrd->hrd_initial_cpb_removal_delay_length_minus1 + 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
            }
        }
    }

    if (accumulatorValidBits != 0)
    {
        // payload_bit_equal_to_one
        WRITE_BITS(1, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        if (accumulatorValidBits != 0)
        {
            // payload_bit_equal_to_zero
            WRITE_BITS(0, SizeOfInBits(uint8_t) - accumulatorValidBits, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        }
    }

    seiBufPeriod->payloadSize = byteWritten;

    return eStatus;
}

MOS_STATUS CodechalCencDecodeHevcNext::EncodeSeiPicTiming(
    const PCODECHAL_DECRYPT_STATUS_REPORT_HEVC statusDriver,
    PCODECHAL_CENC_SEI                         seiPicTiming)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    if (statusDriver == nullptr || seiPicTiming == nullptr)
    {
        CENC_DECODE_ASSERTMESSAGE("Invalid input parameters");
        return MOS_STATUS_NULL_POINTER;
    }

    uint8_t                       *outBitStreamBuf       = seiPicTiming->data;
    uint32_t                      bufSize                = sizeof(seiPicTiming->data);
    uint32_t                      byteWritten            = 0;
    uint8_t                       accumulator            = 0;
    uint8_t                       accumulatorValidBits   = 0;
    uint8_t                       *pAccumulator          = &accumulator;
    uint8_t                       *pAccumulatorValidBits = &accumulatorValidBits;
    uint32_t                      *pWrittenBytes         = &byteWritten;
    aligned_h265_sei_pic_timing_t *picTiming             = &statusDriver->SeiPicTiming;
    aligned_h265_seq_param_set_t  *spsHeader             = &statusDriver->SeqParams;
    h265_vui_parameters           *vui                   = &spsHeader->sps_vui;
    h265_hrd_parameters           *hrd                   = &vui->vui_hrd_parameters;

    if (vui->vui_frame_field_info_present_flag)
    {

        // pic_struct - u(4)
        WRITE_BITS(picTiming->sei_pt_pic_struct, 4, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        // source_scan_type - u(2)
        WRITE_BITS(picTiming->sei_pt_source_scan_type, 2, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        // duplicate_flag - u(1)
        WRITE_BITS(picTiming->sei_pt_duplicate_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    }

    if (hrd->nal_hrd_parameters_present_flag || hrd->vcl_hrd_parameters_present_flag)
    {
        // au_cpb_removal_delay_minus1 - u(hrd->hrd_au_cpb_removal_delay_length_minus1 + 1)
        WRITE_BITS(picTiming->sei_pt_au_cpb_removal_delay_minus1, hrd->hrd_au_cpb_removal_delay_length_minus1 + 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        // pic_dpb_output_delay - u(hrd->hrd_dpb_output_delay_length_minus1 + 1)
        WRITE_BITS(picTiming->sei_pt_pic_dpb_output_delay, hrd->hrd_dpb_output_delay_length_minus1 + 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        if (hrd->hrd_sub_pic_cpb_params_present_flag)
        {
            // pic_dpb_output_du_delay - u(hrd->hrd_dpb_output_delay_du_length_minus1 + 1)
            WRITE_BITS(picTiming->sei_pt_pic_dpb_output_du_delay, hrd->hrd_dpb_output_delay_du_length_minus1 + 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        }
        if (hrd->hrd_sub_pic_cpb_params_present_flag && hrd->hrd_sub_pic_cpb_params_in_pic_timing_sei_flag)
        {
            // num_decoding_units_minus1 - uvlc
            ENCODE_EXP_GOLOMB(picTiming->sei_pt_num_decoding_units_minus1, SizeOfInBits(picTiming->sei_pt_num_decoding_units_minus1), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
            // du_common_cpb_removal_delay_flag - u(1)
            WRITE_BITS(picTiming->sei_pt_du_common_cpb_removal_delay_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
            if (picTiming->sei_pt_du_common_cpb_removal_delay_flag)
            {
                // du_common_cpb_removal_delay_minus1 - u(hrd->hrd_du_cpb_removal_delay_increment_length_minus1 + 1)
                WRITE_BITS(picTiming->sei_pt_du_common_cpb_removal_delay_increment_minus1, hrd->hrd_du_cpb_removal_delay_increment_length_minus1 + 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
            }
            for (uint32_t i = 0; i <= picTiming->sei_pt_num_decoding_units_minus1; i++)
            {
                // num_nalus_in_du_minus1[i] - uvlc
                // num_nalus_in_du_minus1[i] not in aligned_h265_sei_pic_timing_t
                ENCODE_EXP_GOLOMB(0, SizeOfInBits(uint32_t), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
                if (!picTiming->sei_pt_du_common_cpb_removal_delay_flag && i < picTiming->sei_pt_num_decoding_units_minus1)
                {
                    // du_cpb_removal_delay_minus1[i] - u(hrd->hrd_du_cpb_removal_delay_increment_length_minus1 + 1)
                    // du_cpb_removal_delay_minus1[i] not in aligned_h265_sei_pic_timing_t
                    WRITE_BITS(0, hrd->hrd_du_cpb_removal_delay_increment_length_minus1 + 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
                }
            }
        }
    }

    if (accumulatorValidBits != 0)
    {
        // payload_bit_equal_to_one
        WRITE_BITS(1, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        if (accumulatorValidBits != 0)
        {
            // payload_bit_equal_to_zero
            WRITE_BITS(0, SizeOfInBits(uint8_t) - accumulatorValidBits, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        }
    }

    seiPicTiming->payloadSize = byteWritten;

    return eStatus;
}

MOS_STATUS CodechalCencDecodeHevcNext::EncodeSeiWriteMessage(
    PCODECHAL_CENC_SEI sei,
    const uint8_t      *payload,
    const uint32_t     payloadSize,
    const uint8_t      payloadType)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    if (payload == nullptr || payloadType >= 46)
    {
        CENC_DECODE_ASSERTMESSAGE("Invalid input parameters");
        return MOS_STATUS_INVALID_PARAMETER;
    }

    if (payloadSize == 0)
    {
        return eStatus;
    }

    // Add extra 3 bytes for type, size, and extra 0xff packing
    if ((sei->payloadSize + payloadSize + 3) > CENC_SEI_INTERNAL_BUFFER_SIZE)
    {
        return MOS_STATUS_NOT_ENOUGH_BUFFER;
    }

    uint32_t size = payloadSize;
    uint32_t offset = sei->payloadSize;

    sei->data[offset++] = (uint8_t)payloadType;

    while (size > 254)
    {
        sei->data[offset++] = 0xFF;
        size = size - 255;
    }
    sei->data[offset++] = (uint8_t)size;

    MOS_SecureMemcpy(sei->data + offset, payloadSize, payload, payloadSize);
    offset += payloadSize;

    sei->payloadSize = offset;

    return eStatus;
}

MOS_STATUS CodechalCencDecodeHevcNext::EncodeSEI(
    const PCODECHAL_DECRYPT_STATUS_REPORT_HEVC statusDriver,
    uint32_t                                   bufSize,
    uint8_t                                    *&outBitStreamBuf,
    uint32_t                                   &writtenBytes)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    if (statusDriver == nullptr || outBitStreamBuf == nullptr)
    {
        CENC_DECODE_ASSERTMESSAGE("Invalid input parameters");
        return MOS_STATUS_NULL_POINTER;
    }

    uint32_t byteWritten = 0;
    uint8_t  accumulator = 0;
    uint8_t  accumulatorValidBits = 0;
    uint8_t  *pAccumulator = &accumulator;
    uint8_t  *pAccumulatorValidBits = &accumulatorValidBits;
    uint32_t *pWrittenBytes = &writtenBytes;

    CODECHAL_CENC_SEI payload = {0};
    CODECHAL_CENC_SEI sei = {0};

    // SEI starts with 00, 00, 01, 06
    WRITE_BITS( 0, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, false);
    WRITE_BITS( 0, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, false);
    WRITE_BITS( 1, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, false);
    WRITE_BITS(0x4e, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, false);
    WRITE_BITS( 1, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, false);

    eStatus = EncodeSeiBufferingPeriod(statusDriver, &payload);
    if (eStatus != MOS_STATUS_SUCCESS)
    {
        if (eStatus == MOS_STATUS_NOT_ENOUGH_BUFFER)
        {
            // SEI payload will be packed into internal buffer at first, not packed to output bitstream buffer directly
            CENC_DECODE_ASSERTMESSAGE("SEI internal payload buffer is not big enough!");
            return MOS_STATUS_UNKNOWN;
        }
        return eStatus;
    }
    eStatus = EncodeSeiWriteMessage(&sei, payload.data, payload.payloadSize, 0);  //buffering period payload type is 0
    if (eStatus != MOS_STATUS_SUCCESS)
    {
        if (eStatus == MOS_STATUS_NOT_ENOUGH_BUFFER)
        {
            // SEI payload will be packed into internal buffer at first, not packed to output bitstream buffer directly
            CENC_DECODE_ASSERTMESSAGE("SEI internal payload buffer is not big enough!");
            return MOS_STATUS_UNKNOWN;
        }
        return eStatus;
    }

    MOS_ZeroMemory(&payload, sizeof(CODECHAL_CENC_SEI));
    eStatus = EncodeSeiPicTiming(statusDriver, &payload);
    if (eStatus != MOS_STATUS_SUCCESS)
    {
        if (eStatus == MOS_STATUS_NOT_ENOUGH_BUFFER)
        {
            // SEI payload will be packed into internal buffer at first, not packed to output bitstream buffer directly
            CENC_DECODE_ASSERTMESSAGE("SEI internal payload buffer is not big enough!");
            return MOS_STATUS_UNKNOWN;
        }
        return eStatus;
    }
    eStatus = EncodeSeiWriteMessage(&sei, payload.data, payload.payloadSize, 1);  //pic timing payload type is 1
    if (eStatus != MOS_STATUS_SUCCESS)
    {
        if (eStatus == MOS_STATUS_NOT_ENOUGH_BUFFER)
        {
            // SEI payload will be packed into internal buffer at first, not packed to output bitstream buffer directly
            CENC_DECODE_ASSERTMESSAGE("SEI internal payload buffer is not big enough!");
            return MOS_STATUS_UNKNOWN;
        }
        return eStatus;
    }

    if (sei.payloadSize + writtenBytes > bufSize)
    {
        return MOS_STATUS_NOT_ENOUGH_BUFFER;
    }
    MOS_SecureMemcpy(outBitStreamBuf, sei.payloadSize, sei.data, sei.payloadSize);

    // rbsp trailing bits
    outBitStreamBuf += sei.payloadSize;
    WRITE_BITS(1, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    if (accumulatorValidBits != 0)
    {
        WRITE_BITS(0, SizeOfInBits(uint8_t) - accumulatorValidBits, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    }

    return eStatus;
}

