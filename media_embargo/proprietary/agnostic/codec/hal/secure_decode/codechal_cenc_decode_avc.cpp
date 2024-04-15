/*
// Copyright (C) 2017-2018 Intel Corporation
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
//! \file      codechal_cenc_decode_avc.cpp
//! \brief     Impelements the public interface for CodecHal CENC Decode 
//!

#include "codechal_cenc_decode_avc.h"
#include "codechal_utilities.h"
#include "mhw_cp_hwcmd_common.h"

CodechalCencDecodeAvc::CodechalCencDecodeAvc():CodechalCencDecode()
{

}

CodechalCencDecodeAvc::~CodechalCencDecodeAvc()
{
    CodecHalFreeDataList((CODEC_AVC_PIC_PARAMS **)m_picParams, CENC_NUM_DECRYPT_SHARED_BUFFERS);
    CodecHalFreeDataList((CODEC_AVC_IQ_MATRIX_PARAMS **)m_qmParams, CENC_NUM_DECRYPT_SHARED_BUFFERS);
}

MOS_STATUS CodechalCencDecodeAvc::Initialize(
    PMOS_CONTEXT                osContext,
    CodechalSetting             *settings)
{
    uint32_t   dwPatchListSize = 0;
    MOS_STATUS eStatus         = MOS_STATUS_SUCCESS;

    CENC_DECODE_FUNCTION_ENTER;   

    CENC_DECODE_CHK_NULL(osContext);
    CENC_DECODE_CHK_NULL(settings);

    CodechalCencDecode::Initialize(
        osContext,
        settings
    );

    // 2nd level batch buffer size for AVC slice-level and VP9 second level decode
    CENC_DECODE_CHK_STATUS(m_mfxInterface->GetMfxPrimitiveCommandsDataSize(
        settings->mode,
        (uint32_t *)&m_standardSlcSizeNeeded,
        (uint32_t *)&dwPatchListSize,
        true));

    if (!(GFX_IS_PRODUCT(m_platform, IGFX_CANNONLAKE) ||
            GFX_IS_PRODUCT(m_platform, IGFX_COFFEELAKE) ||
            GFX_IS_PRODUCT(m_platform, IGFX_KABYLAKE)))
    {
        m_enableAesNativeSupport = false;
    }

    CENC_DECODE_VERBOSEMESSAGE("dwStandardSlcSizeNeeded = %d.", m_standardSlcSizeNeeded);
    CENC_DECODE_VERBOSEMESSAGE("bEnableAesNativeSupport = %d.", m_enableAesNativeSupport);

    if (m_enableAesNativeSupport)
    {
        m_standardSlcSizeNeeded += 2 * (sizeof(MFX_AES_STATE_CMD_G9) + sizeof(uint32_t));
    }

    // Allocate the resources required to perform decryption
    CENC_DECODE_CHK_STATUS(AllocateResources());

    return eStatus;
}

MOS_STATUS CodechalCencDecodeAvc::AllocateDecodeResource(
    uint8_t bufIdx)
{
    CENC_DECODE_FUNCTION_ENTER;

    m_bitstreamSize = MOS_ALIGN_CEIL(m_picSize, CODECHAL_PAGE_SIZE);
    // set 2nd level batch buffer size according to number of slices
    m_sliceLevelBBSize = m_standardSlcSizeNeeded * m_numSlices;

    return CodechalCencDecode::AllocateDecodeResource(bufIdx);
}

MOS_STATUS CodechalCencDecodeAvc::SetDecodeParams(
    CodechalDecodeParams *decodeParams,
    void                 *cpParam)
{
    uint8_t                 cencBufIdx, idx;
    PCODEC_AVC_PIC_PARAMS   appPicParams, driverPicParams;
    CODECHAL_CENC_HEAP_INFO *heapInfoTmp;
    MOS_STATUS              eStatus = MOS_STATUS_SUCCESS;

    CENC_DECODE_FUNCTION_ENTER;

    if (dwFrameNum > 0)
    {
        if (!m_serialMode)
        {
            CENC_DECODE_CHK_NULL(m_picParams);
            CENC_DECODE_CHK_NULL(m_qmParams);

            appPicParams = (PCODEC_AVC_PIC_PARAMS)decodeParams->m_picParams;

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
                CENC_DECODE_VERBOSEMESSAGE("Status report number %x for decrypt not found in array!", decodeParams->m_cencDecodeStatusReportNum);
                eStatus = MOS_STATUS_INVALID_PARAMETER;
                return eStatus;
            }

            if (!m_pr30Enabled)
            {
                CENC_DECODE_CHK_NULL(m_picParams[cencBufIdx]);
                CENC_DECODE_CHK_NULL(m_qmParams[cencBufIdx]);

                driverPicParams = (PCODEC_AVC_PIC_PARAMS)m_picParams[cencBufIdx];

                driverPicParams->CurrPic = appPicParams->CurrPic;

                for (idx = 0; idx < 16; idx++)
                {
                    driverPicParams->RefFrameList[idx] = appPicParams->RefFrameList[idx];
                    driverPicParams->FrameNumList[idx] = appPicParams->FrameNumList[idx];
                    driverPicParams->FieldOrderCntList[idx][0] = appPicParams->FieldOrderCntList[idx][0];
                    driverPicParams->FieldOrderCntList[idx][1] = appPicParams->FieldOrderCntList[idx][1];
                }
                driverPicParams->StatusReportFeedbackNumber = appPicParams->StatusReportFeedbackNumber;
                driverPicParams->NonExistingFrameFlags = appPicParams->NonExistingFrameFlags;
                driverPicParams->UsedForReferenceFlags = appPicParams->UsedForReferenceFlags;

                MOS_SecureMemcpy(
                    decodeParams->m_picParams,
                    sizeof(CODEC_AVC_PIC_PARAMS),
                    driverPicParams,
                    sizeof(CODEC_AVC_PIC_PARAMS));
                MOS_SecureMemcpy(
                    decodeParams->m_iqMatrixBuffer,
                    sizeof(CODEC_AVC_IQ_MATRIX_PARAMS),
                    m_qmParams[cencBufIdx],
                    sizeof(CODEC_AVC_IQ_MATRIX_PARAMS));
            }

            heapInfoTmp = &m_heapInfo[cencBufIdx];

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

            decodeParams->m_numSlices = 1;  // is the needed?

            //pAvcState->m_avcSliceParams = nullptr;
            decodeParams->m_picIdRemappingInUse = false;
            m_shareBuf.secondLvlBbBlock = &heapInfoTmp->SecondLvlBbBlock;
            m_shareBuf.trackerId = heapInfoTmp->u32TrackerId;
            m_shareBuf.bufIdx = cencBufIdx;
            m_shareBuf.checkStatusRequired = m_pr30Enabled;
            decodeParams->m_cencBuf = &m_shareBuf;

            MOS_SecureMemcpy(
                cpParam,
                sizeof(MHW_PAVP_ENCRYPTION_PARAMS),
                &m_savedEncryptionParams[cencBufIdx],
                sizeof(MHW_PAVP_ENCRYPTION_PARAMS));

            //This parameter is set for normal decoder
            //Since CENC decoder did key exchange during PR PPED process, set bInsertKeyExinVcsProlog to false to skip key exchange
            static_cast<PMHW_PAVP_ENCRYPTION_PARAMS>(cpParam)->bInsertKeyExinVcsProlog = false;
        }
        else
        {
            decodeParams->m_bFullFrameData = true;
        }
    }

    return eStatus;
}

int32_t CodechalCencDecodeAvc::GetMaxFSFromLevelIdc(
    int32_t     levelIdc)
{
    int32_t     frameSize = 0;

    switch (levelIdc)
    {
    case CODEC_AVC_LEVEL_1:
        frameSize = AVC_MAX_FRAME_SIZE_IDC1_TO_1b;
        break;
    case CODEC_AVC_LEVEL_11:
    case CODEC_AVC_LEVEL_12:
    case CODEC_AVC_LEVEL_13:
    case CODEC_AVC_LEVEL_2:
        frameSize = AVC_MAX_FRAME_SIZE_IDC11_TO_2;
        break;
    case CODEC_AVC_LEVEL_21:
        frameSize = AVC_MAX_FRAME_SIZE_IDC21;
        break;
    case CODEC_AVC_LEVEL_22:
    case CODEC_AVC_LEVEL_3:
        frameSize = AVC_MAX_FRAME_SIZE_IDC22_TO_3;
        break;
    case CODEC_AVC_LEVEL_31:
        frameSize = AVC_MAX_FRAME_SIZE_IDC31;
        break;
    case CODEC_AVC_LEVEL_32:
        frameSize = AVC_MAX_FRAME_SIZE_IDC32;
        break;
    case CODEC_AVC_LEVEL_4:
    case CODEC_AVC_LEVEL_41:
        frameSize = AVC_MAX_FRAME_SIZE_IDC4_TO_41;
        break;
    case CODEC_AVC_LEVEL_42:
        frameSize = AVC_MAX_FRAME_SIZE_IDC42;
        break;
    case CODEC_AVC_LEVEL_5:
        frameSize = AVC_MAX_FRAME_SIZE_IDC5;
        break;
    case CODEC_AVC_LEVEL_51:
    default:
        frameSize = AVC_MAX_FRAME_SIZE_IDC51;
        break;
    }
    return frameSize;
}

uint32_t CodechalCencDecodeAvc::GetMaxSupportedSlices(
    uint16_t    mbHeight)
{
    uint32_t     maxSlices = 0;

    //Support 4 slices per MB row
    maxSlices = (uint32_t)mbHeight * 4;

    return maxSlices;
}

uint32_t CodechalCencDecodeAvc::GetMaxBitstream(
    uint16_t mbWidth,
    uint16_t mbHeight)
{
    uint32_t     maxBitstream = 0;

    //Compress ratio is 3 pratically, bitrate should be 16*16*3/2/3,
    maxBitstream = (uint32_t)mbWidth * mbHeight * 128;

    return maxBitstream;
}

void CodechalCencDecodeAvc::ScalingList(
    uint8_t   *inputScalingList,
    uint8_t   *scalingList,
    uint8_t   qmIdx)
{
    uint8_t   idx;

    CENC_DECODE_ASSERT(scalingList);

    if (qmIdx < 6)
    {
        for (idx = 0; idx < 16; idx++)
        {
            scalingList[CODEC_AVC_Qmatrix_scan_4x4[idx]] = inputScalingList[idx];
        }
    }
    else
    {
        // 8x8 block
        for (idx = 0; idx < 64; idx++)
        {
            scalingList[CODEC_AVC_Qmatrix_scan_8x8[idx]] = inputScalingList[idx];
        }
    }
}

void CodechalCencDecodeAvc::ScalingListFallbackA(
    PCODEC_AVC_IQ_MATRIX_PARAMS  avcQmParams,
    uint8_t                         qmIdx)
{
    uint8_t   idx;

    CENC_DECODE_ASSERT(avcQmParams);

    if (qmIdx < 6)
    {
        for (idx = 0; idx < 16; idx++)
        {
            if (qmIdx == 0)
            {
                avcQmParams->ScalingList4x4[qmIdx][CODEC_AVC_Qmatrix_scan_4x4[idx]] = CODEC_AVC_Default_4x4_Intra[idx];
            }
            else if (qmIdx == 3)
            {
                avcQmParams->ScalingList4x4[qmIdx][CODEC_AVC_Qmatrix_scan_4x4[idx]] = CODEC_AVC_Default_4x4_Inter[idx];
            }
            else
            {
                avcQmParams->ScalingList4x4[qmIdx][idx] =
                    avcQmParams->ScalingList4x4[qmIdx - 1][idx];
            }
        }
    }
    else
    {
        // 8x8 block
        for (idx = 0; idx < 64; idx++)
        {
            if (qmIdx < 7)
            {
                avcQmParams->ScalingList8x8[qmIdx - 6][CODEC_AVC_Qmatrix_scan_8x8[idx]] = CODEC_AVC_Default_8x8_Intra[idx];
            }
            else
            {
                avcQmParams->ScalingList8x8[qmIdx - 6][CODEC_AVC_Qmatrix_scan_8x8[idx]] = CODEC_AVC_Default_8x8_Inter[idx];
            }
        }
    }

}

void CodechalCencDecodeAvc::ScalingListFallbackB(
    PCODEC_AVC_IQ_MATRIX_PARAMS  avcQmParams,
    uint8_t                         *inputScalingList,
    uint8_t                         qmIdx)
{
    uint8_t   idx;

    CENC_DECODE_ASSERT(avcQmParams);

    if (qmIdx < 6)
    {

        if ((qmIdx == 0 || qmIdx == 3) && inputScalingList != nullptr)
        {
            ScalingList(
                inputScalingList,
                &(avcQmParams->ScalingList4x4[qmIdx][0]),
                qmIdx);
        }
        else
        {
            for (idx = 0; idx < 16; idx++)
            {
                if (qmIdx == 0 || qmIdx == 3)
                {
                    avcQmParams->ScalingList4x4[qmIdx][idx] = 16;
                }
                else
                {
                    avcQmParams->ScalingList4x4[qmIdx][idx] = avcQmParams->ScalingList4x4[qmIdx - 1][idx];
                }
            }
        }
    }
    else
    {
        if (inputScalingList == nullptr)
        {
            // 8x8 block
            for (idx = 0; idx < 64; idx++)
            {
                avcQmParams->ScalingList8x8[qmIdx - 6][idx] = 16;
            }
        }
        else
        {
            ScalingList(
                inputScalingList,
                &(avcQmParams->ScalingList8x8[qmIdx - 6][0]),
                qmIdx
            );
        }
    }
}

uint32_t CodechalCencDecodeAvc::VerifySliceHeaderSetParams(
    PCODECHAL_DECRYPT_SLICE_HEADER_SET_AVC     sliceHeaderSetBuf)
{
    PCODECHAL_DECRYPT_SLICE_HEADER_AVC  sliceHeader;
    uint32_t                            sliceHeaderSetLength = 0;

    if (sliceHeaderSetBuf)
    {
        sliceHeader = (PCODECHAL_DECRYPT_SLICE_HEADER_AVC)&sliceHeaderSetBuf->ui8Data;
        sliceHeaderSetLength = sizeof(sliceHeaderSetBuf->ui16NumHeaders);

        //Only pass one slice header data
        if (sliceHeaderSetBuf->ui16NumHeaders && sliceHeaderSetLength <= CODECHAL_DECODE_MAX_SLICE_HDR_SET_SIZE)
        {
            sliceHeaderSetLength += sizeof(sliceHeader->ui16SliceHeaderLen) + sliceHeader->ui16SliceHeaderLen;
        }
        CENC_DECODE_VERBOSEMESSAGE("Slice NumHeader = %d, BufLength = %d.", sliceHeaderSetBuf->ui16NumHeaders, sliceHeaderSetLength);
    }
    return sliceHeaderSetLength;
}

MOS_STATUS CodechalCencDecodeAvc::InitializeSharedBuf(
    bool                        initStatusReportOnly)
{
    MOS_STATUS                          eStatus = MOS_STATUS_SUCCESS;
    PCODECHAL_DECRYPT_SHARED_BUFFER_AVC statusBuf;
    PCODECHAL_DECRYPT_STATUS_REPORT_AVC statusDriver;
    uint32_t                            idx;

    CodechalResLock shareBuffer(m_osInterface, &m_resHucSharedBuf);
    statusBuf = (PCODECHAL_DECRYPT_SHARED_BUFFER_AVC)shareBuffer.Lock(CodechalResLock::writeOnly);
    CENC_DECODE_CHK_NULL(statusBuf);

    if (initStatusReportOnly)
    {
        idx = m_currSharedBufIdx;
        statusDriver =
            (PCODECHAL_DECRYPT_STATUS_REPORT_AVC)&statusBuf->StatusReport[idx % CODECHAL_NUM_DECRYPT_STATUS_BUFFERS];

        // Save current status report number
        statusDriver->PicInfo.ui16StatusReportFeedbackNumber = m_statusReportNumbers[idx];

        // Invalidate SPS/PPS in status report for current index
        statusDriver->SeqParams.seq_parameter_set_id = CODEC_AVC_MAX_SPS_NUM + 1;
        statusDriver->PicParams.pic_parameter_set_id = CODEC_AVC_MAX_PPS_NUM + 1;
    }
    else
    {
        for (idx = 0; idx <= CODEC_AVC_MAX_PPS_NUM; idx++)
        {
            if (idx < CODEC_AVC_MAX_SPS_NUM)
            {
                statusBuf->SeqParams[idx].seq_parameter_set_id = CODEC_AVC_MAX_SPS_NUM + 1;
            }
            statusBuf->PicParams[idx].pic_parameter_set_id = CODEC_AVC_MAX_PPS_NUM + 1;
        }
    }
    MOS_ZeroMemory(&statusBuf->ErrorReport, sizeof(CODECHAL_DECRYPT_ERROR_REPORT));

    return eStatus;
}

MOS_STATUS CodechalCencDecodeAvc::UpdateSecureOptMode(
    PMHW_PAVP_ENCRYPTION_PARAMS params)
{
    MOS_STATUS  eStatus = MOS_STATUS_SUCCESS;

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

MOS_STATUS CodechalCencDecodeAvc::InitializeAllocation()
{
    MOS_STATUS  eStatus = MOS_STATUS_SUCCESS;
    uint32_t    dwHeightInMb;

    CENC_DECODE_FUNCTION_ENTER;

    //Support 4 slices per MB row
    dwHeightInMb = CODEC_MAX_DECRYPT_HEIGHT >> 4;
    m_numSlices = dwHeightInMb * 4;

    // shared buffer size
    m_sharedBufSize = sizeof(CODECHAL_DECRYPT_SHARED_BUFFER_AVC);
    m_standardReportSizeApp = sizeof(DecryptStatusAvc);

    // allocate the Pic Params and QM Params buffer
    CENC_DECODE_CHK_STATUS(CodecHalAllocateDataList(
        (CODEC_AVC_PIC_PARAMS **)m_picParams,
        CENC_NUM_DECRYPT_SHARED_BUFFERS));

    CENC_DECODE_CHK_STATUS(CodecHalAllocateDataList(
        (CODEC_AVC_IQ_MATRIX_PARAMS **)m_qmParams,
        CENC_NUM_DECRYPT_SHARED_BUFFERS));

    return eStatus;
}

bool CodechalCencDecodeAvc::IsFRextProfile(uint8_t profile)
{
    return (profile == CODEC_AVC_HIGH_PROFILE       || 
            profile == CODEC_AVC_HIGH10_PROFILE     || 
            profile == CODEC_AVC_HIGH422_PROFILE    || 
            profile == CODEC_AVC_HIGH444_PROFILE    || 
            profile == CODEC_AVC_CAVLC444_INTRA_PROFILE);
}


MOS_STATUS CodechalCencDecodeAvc::PackScalingList(
    uint8_t *scalingList, 
    int sizeOfScalingList, 
    uint32_t bufSize,
    uint8_t* accumulator, 
    uint8_t* accumulatorValidBits,
    uint8_t* & outBitStreamBuf, 
    uint32_t* byteWritten)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    uint8_t lastScale, nextScale, j;
    int8_t  deltaScale;

    lastScale = nextScale = 8;

    for (j = 0; j < sizeOfScalingList; j++)
    {
        if (nextScale != 0)
        {
            deltaScale = (int8_t)(scalingList[j] - lastScale);
            ENCODE_EXP_GOLOMB(SIGNED_VALUE(deltaScale), SizeOfInBits(deltaScale), bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten);
            nextScale = scalingList[j];
        }
        lastScale = (nextScale == 0) ? lastScale : nextScale;
    }
    return eStatus;
}

MOS_STATUS CodechalCencDecodeAvc::EncodeSPS(
    const PCODECHAL_DECRYPT_STATUS_REPORT_AVC statusDriver, 
    uint32_t bufSize, 
    uint8_t* &outBitStreamBuf, 
    uint32_t &writtenBytes)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    if (statusDriver == nullptr || outBitStreamBuf == nullptr)
    {
        CENC_DECODE_ASSERTMESSAGE("Invalid input parameters");
        return MOS_STATUS_NULL_POINTER;
    }

    aligned_seq_param_set_t* spsHeader = &statusDriver->SeqParams;
    uint8_t accumulator = 0;
    uint8_t accumulatorValidBits = 0;
    uint8_t *pAccumulator = &accumulator;
    uint8_t *pAccumulatorValidBits = &accumulatorValidBits;
    uint32_t *pWrittenBytes = &writtenBytes;

    // SPS starts with 00, 00, 01, 27
    WRITE_BITS( 0, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, false);
    WRITE_BITS( 0, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, false);
    WRITE_BITS( 1, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, false);
    WRITE_BITS(0x27, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, false);
    // write profile_idc - u(8)
    WRITE_BITS(spsHeader->profile_idc, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // write constraint_setX_flag (0 <= X <= 5) - u(1), reserved_zero_2bits - u(2)
    // according to comments, the spsHeader->constraint_set_flags field is bit 0 to 3 for set0 to set3
    WRITE_BITS(static_cast<uint32_t>(spsHeader->constraint_set_flags & 1), 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    WRITE_BITS(static_cast<uint32_t>((spsHeader->constraint_set_flags >> 1) & 1), 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    WRITE_BITS(static_cast<uint32_t>((spsHeader->constraint_set_flags >> 2) & 1), 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    WRITE_BITS(static_cast<uint32_t>((spsHeader->constraint_set_flags >> 3) & 1), 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    WRITE_BITS(0, 4, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // write level_idc - u(8)
    WRITE_BITS(spsHeader->level_idc, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // write seq_parameter_set_id - ue(v)
    ENCODE_EXP_GOLOMB(spsHeader->seq_parameter_set_id, SizeOfInBits(spsHeader->seq_parameter_set_id),
        bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);

    if (spsHeader->profile_idc == CODEC_AVC_HIGH_PROFILE           || 
        spsHeader->profile_idc == CODEC_AVC_HIGH10_PROFILE         || 
        spsHeader->profile_idc == CODEC_AVC_HIGH422_PROFILE        || 
        spsHeader->profile_idc == CODEC_AVC_HIGH444_PROFILE        || 
        spsHeader->profile_idc == CODEC_AVC_CAVLC444_INTRA_PROFILE || 
        spsHeader->profile_idc == CODEC_AVC_SCALABLE_BASE_PROFILE  || 
        spsHeader->profile_idc == CODEC_AVC_SCALABLE_HIGH_PROFILE)
    {
        // write chroma_format_idc - ue(v)
        ENCODE_EXP_GOLOMB(spsHeader->sps_disp.chroma_format_idc, SizeOfInBits(spsHeader->sps_disp.chroma_format_idc), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
        if (spsHeader->sps_disp.chroma_format_idc == 3)
        {
            // write separate_colour_plane_flag - u(1)
            // separate_colour_plane_flag is not in aligned_seq_param_set_t
            WRITE_BITS(0, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        }

        // write bit_depth_luma_minus8 - ue(v)
        ENCODE_EXP_GOLOMB(spsHeader->bit_depth_luma_minus8, SizeOfInBits(spsHeader->bit_depth_luma_minus8),
            bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
        // write bit_depth_chroma_minus8 - ue(v)
        ENCODE_EXP_GOLOMB(spsHeader->bit_depth_chroma_minus8, SizeOfInBits(spsHeader->bit_depth_chroma_minus8),
            bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);

        // write qpprime_y_zero_transform_bypass_flag - u(1)
        // cannot find qpprime_y_zero_transform_bypass_flag, using lossless_qpprime_y_zero_flag
        WRITE_BITS(spsHeader->lossless_qpprime_y_zero_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        // write seq_scaling_matrix_present_flag - u(1)
        WRITE_BITS(spsHeader->seq_scaling_matrix_present_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);

        if (spsHeader->seq_scaling_matrix_present_flag)
        {
            for (uint8_t cnt = 0; cnt < ((spsHeader->sps_disp.chroma_format_idc != 3) ? 8U : 12U); cnt++)
            {
                if (cnt < COUNTOF(spsHeader->seq_scaling_list_present_flag))
                {
                    // write seq_scaling_list_present_flag[i] - u(1)
                    WRITE_BITS( spsHeader->seq_scaling_list_present_flag[cnt], 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
                    if (spsHeader->seq_scaling_list_present_flag[cnt])
                    {
                        if (cnt < 6)
                        {
                            eStatus = PackScalingList(spsHeader->ScalingList4x4[cnt], 16, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
                            if (eStatus != MOS_STATUS_SUCCESS)
                            {
                                return eStatus;
                            }
                        }
                        else
                        {
                            eStatus = PackScalingList(spsHeader->ScalingList8x8[cnt - 6], 64, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
                            if (eStatus != MOS_STATUS_SUCCESS)
                            {
                                return eStatus;
                            }
                        }
                    }
                }
                else
                {
                    WRITE_BITS(0, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
                }
            }
        }
    }

    // write log2_max_frame_num_minus4 - ue(v)
    ENCODE_EXP_GOLOMB(spsHeader->log2_max_frame_num_minus4, SizeOfInBits(spsHeader->log2_max_frame_num_minus4),
        bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    // write pic_order_cnt_type - ue(v)
    ENCODE_EXP_GOLOMB(spsHeader->pic_order_cnt_type, SizeOfInBits(spsHeader->pic_order_cnt_type),
        bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);

    if (spsHeader->pic_order_cnt_type == 0)
    {
        // write log2_max_pic_order_cnt_lsb_minus4 - ue(v)
        ENCODE_EXP_GOLOMB(spsHeader->log2_max_pic_order_cnt_lsb_minus4, SizeOfInBits(spsHeader->log2_max_pic_order_cnt_lsb_minus4),
            bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    }
    else if (spsHeader->pic_order_cnt_type == 1)
    {
        // write delta_pic_order_always_zero_flag - u(1)
        WRITE_BITS(spsHeader->delta_pic_order_always_zero_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        // write offset_for_non_ref_pic - se(v)
        ENCODE_EXP_GOLOMB(SIGNED_VALUE(spsHeader->offset_for_non_ref_pic), SizeOfInBits(spsHeader->offset_for_non_ref_pic),
            bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
        // write offset_for_top_to_bottom_field - se(v)
        ENCODE_EXP_GOLOMB(SIGNED_VALUE(spsHeader->offset_for_top_to_bottom_field), SizeOfInBits(spsHeader->offset_for_top_to_bottom_field),
            bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
        // write num_ref_frames_in_pic_order_cnt_cycle - ue(v)
        ENCODE_EXP_GOLOMB(spsHeader->num_ref_frames_in_pic_order_cnt_cycle, SizeOfInBits(spsHeader->num_ref_frames_in_pic_order_cnt_cycle),
            bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);

        for (uint32_t cnt = 0; cnt < spsHeader->num_ref_frames_in_pic_order_cnt_cycle; cnt++)
        {
            // write offset_for_ref_frame[i] - se(v)
            ENCODE_EXP_GOLOMB(SIGNED_VALUE(spsHeader->offset_for_ref_frame[cnt]), SizeOfInBits(spsHeader->offset_for_ref_frame[0]),
                bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
        }
    }

    // write max_num_ref_frames - ue(v)
    // using num_ref_frames in aligned_seq_param_set_t
    ENCODE_EXP_GOLOMB(spsHeader->num_ref_frames, SizeOfInBits(spsHeader->num_ref_frames), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    // write gaps_in_frame_num_value_allowed_flag - u(1)
    WRITE_BITS(spsHeader->gaps_in_frame_num_value_allowed_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // write pic_width_in_mbs_minus1 - ue(v)
    ENCODE_EXP_GOLOMB(spsHeader->sps_disp.pic_width_in_mbs_minus1, SizeOfInBits(spsHeader->sps_disp.pic_width_in_mbs_minus1),
        bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    // write pic_height_in_map_units_minus1 - ue(v)
    ENCODE_EXP_GOLOMB(spsHeader->sps_disp.pic_height_in_map_units_minus1, SizeOfInBits(spsHeader->sps_disp.pic_height_in_map_units_minus1),
        bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    // write frame_mbs_only_flag - u(1)
    WRITE_BITS(spsHeader->sps_disp.frame_mbs_only_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);

    if (spsHeader->sps_disp.frame_mbs_only_flag == 0)
    {
        // write mb_adaptive_frame_field_flag - u(1)
        WRITE_BITS(spsHeader->sps_disp.mb_adaptive_frame_field_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    }

    // write direct_8x8_inference_flag - u(1)
    WRITE_BITS(spsHeader->sps_disp.direct_8x8_inference_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // write frame_cropping_flag - u(1)
    WRITE_BITS(spsHeader->sps_disp.frame_cropping_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);

    if (spsHeader->sps_disp.frame_cropping_flag)
    {
        // write frame_crop_left_offset - ue(v)
        ENCODE_EXP_GOLOMB(spsHeader->sps_disp.frame_crop_rect_left_offset, SizeOfInBits(spsHeader->sps_disp.frame_crop_rect_left_offset),
            bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
        // write frame_crop_right_offset - ue(v)
        ENCODE_EXP_GOLOMB(spsHeader->sps_disp.frame_crop_rect_right_offset, SizeOfInBits(spsHeader->sps_disp.frame_crop_rect_right_offset),
            bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
        // write frame_crop_top_offset - ue(v)
        ENCODE_EXP_GOLOMB(spsHeader->sps_disp.frame_crop_rect_top_offset, SizeOfInBits(spsHeader->sps_disp.frame_crop_rect_top_offset),
            bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
        // write frame_crop_bottom_offset - ue(v)
        ENCODE_EXP_GOLOMB(spsHeader->sps_disp.frame_crop_rect_bottom_offset, SizeOfInBits(spsHeader->sps_disp.frame_crop_rect_bottom_offset),
            bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    }

    // write vui_parameters_present_flag - u(1)
    WRITE_BITS(spsHeader->sps_disp.vui_parameters_present_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);

    if (spsHeader->sps_disp.vui_parameters_present_flag)
    {
        // vui_parameters( ) 
        // write aspect_ratio_info_present_flag - u(1)
        WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.aspect_ratio_info_present_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);

        if (spsHeader->sps_disp.vui_seq_parameters.aspect_ratio_info_present_flag)
        {
            // write aspect_ratio_idc - u(8)
            WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.aspect_ratio_idc, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);

            if (spsHeader->sps_disp.vui_seq_parameters.aspect_ratio_idc == 255 /*Extended_SAR*/)
            {
                // write sar_width - u(16)
                WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.sar_width, 16, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
                // write sar_height - u(16)
                WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.sar_height, 16, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
            }
        }

        // write overscan_info_present_flag - u(1)
        // overscan_info_present_flag is not in aligned_seq_param_set_t
        WRITE_BITS(0, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        // write video_signal_type_present_flag - u(1)
        WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.video_signal_type_present_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);

        if (spsHeader->sps_disp.vui_seq_parameters.video_signal_type_present_flag)
        {
            // write video_format - u(3)
            WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.video_format, 3, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
            // write video_full_range_flag - u(1)
            // video_full_range_flag is not in aligned_seq_param_set_t
            WRITE_BITS(0, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
            // write colour_description_present_flag - u(1)
            WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.colour_description_present_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);

            if (spsHeader->sps_disp.vui_seq_parameters.colour_description_present_flag)
            {
                // write colour_primaries - u(8)
                WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.colour_primaries, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
                // write transfer_characteristics - u(8)
                WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.transfer_characteristics, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
                // write matrix_coefficients - u(8)
                WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.matrix_coefficients, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
            }
        }
        // write chroma_loc_info_present_flag - u(1)
        // chroma_loc_info_present_flag is not in aligned_seq_param_set_t
        WRITE_BITS(0, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);

        // write timing_info_present_flag - u(1)
        WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.timing_info_present_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);

        if (spsHeader->sps_disp.vui_seq_parameters.timing_info_present_flag)
        {
            // write num_units_in_tick - u(32)
            WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.num_units_in_tick, 32, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
            // write time_scale - u(32)
            WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.time_scale, 32, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
            // write fixed_frame_rate_flag - u(1)
            WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.fixed_frame_rate_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        }

        // write nal_hrd_parameters_present_flag - u(1)
        WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.nal_hrd_parameters_present_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);

        if (spsHeader->sps_disp.vui_seq_parameters.nal_hrd_parameters_present_flag)
        {
            // hrd_parameters( ) for NAL
            // write cpb_cnt_minus1 - ue(v)
            ENCODE_EXP_GOLOMB(spsHeader->sps_disp.vui_seq_parameters.NalHrd.info.cpb_cnt_minus1, SizeOfInBits(spsHeader->sps_disp.vui_seq_parameters.NalHrd.info.cpb_cnt_minus1),
                bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
            // write bit_rate_scale - u(4)
            WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.NalHrd.info.bit_rate_scale, 4, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
            // write cpb_size_scale - u(4)
            WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.NalHrd.info.cpb_size_scale, 4, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);

            for (uint8_t SchedSelIdx = 0; spsHeader->sps_disp.vui_seq_parameters.NalHrd.info.cpb_cnt_minus1 < MAX_CPB_CNT && SchedSelIdx <= spsHeader->sps_disp.vui_seq_parameters.NalHrd.info.cpb_cnt_minus1; SchedSelIdx++)
            {
                // write bit_rate_value_minus1 - ue(v)
                ENCODE_EXP_GOLOMB(spsHeader->sps_disp.vui_seq_parameters.NalHrd.item[SchedSelIdx].bit_rate_value_minus1, SizeOfInBits(spsHeader->sps_disp.vui_seq_parameters.NalHrd.item[SchedSelIdx].bit_rate_value_minus1),
                    bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
                // write cpb_size_value_minus1 - ue(v)
                ENCODE_EXP_GOLOMB(spsHeader->sps_disp.vui_seq_parameters.NalHrd.item[SchedSelIdx].cpb_size_value_minus1, SizeOfInBits(spsHeader->sps_disp.vui_seq_parameters.NalHrd.item[SchedSelIdx].cpb_size_value_minus1),
                    bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
                // write cbr_flag - u(1)
                WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.NalHrd.item[SchedSelIdx].cbr_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
            }

            // write initial_cpb_removal_delay_length_minus1 - u(5)
            WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.NalHrd.info.initial_cpb_removal_delay_length_minus1, 5, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
            // write cpb_removal_delay_length_minus1 - u(5)
            WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.NalHrd.info.cpb_removal_delay_length_minus1, 5, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
            // write dpb_output_delay_length_minus1 - u(5)
            WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.NalHrd.info.dpb_output_delay_length_minus1, 5, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
            // write time_offset_length - u(5)
            WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.NalHrd.info.time_offset_length, 5, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        }

        // write vcl_hrd_parameters_present_flag - u(1)
        WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.vcl_hrd_parameters_present_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);

        if (spsHeader->sps_disp.vui_seq_parameters.vcl_hrd_parameters_present_flag)
        {
            // hrd_parameters( ) for VCL
            // write cpb_cnt_minus1 - ue(v)
            ENCODE_EXP_GOLOMB(spsHeader->sps_disp.vui_seq_parameters.VclHrd.info.cpb_cnt_minus1, SizeOfInBits(spsHeader->sps_disp.vui_seq_parameters.VclHrd.info.cpb_cnt_minus1),
                bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
            // write bit_rate_scale - u(4)
            WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.VclHrd.info.bit_rate_scale, 4, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
            // write cpb_size_scale - u(4)
            WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.VclHrd.info.cpb_size_scale, 4, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);

            for (uint8_t SchedSelIdx = 0; spsHeader->sps_disp.vui_seq_parameters.VclHrd.info.cpb_cnt_minus1 < MAX_CPB_CNT && SchedSelIdx <= spsHeader->sps_disp.vui_seq_parameters.VclHrd.info.cpb_cnt_minus1; SchedSelIdx++)
            {
                // write bit_rate_value_minus1 - ue(v)
                ENCODE_EXP_GOLOMB(spsHeader->sps_disp.vui_seq_parameters.VclHrd.item[SchedSelIdx].bit_rate_value_minus1, SizeOfInBits(spsHeader->sps_disp.vui_seq_parameters.VclHrd.item[SchedSelIdx].bit_rate_value_minus1),
                    bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
                // write cpb_size_value_minus1 - ue(v)
                ENCODE_EXP_GOLOMB(spsHeader->sps_disp.vui_seq_parameters.VclHrd.item[SchedSelIdx].cpb_size_value_minus1, SizeOfInBits(spsHeader->sps_disp.vui_seq_parameters.VclHrd.item[SchedSelIdx].cpb_size_value_minus1),
                    bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
                // write cbr_flag - u(1)
                WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.VclHrd.item[SchedSelIdx].cbr_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
            }

            // write initial_cpb_removal_delay_length_minus1 - u(5)
            WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.VclHrd.info.initial_cpb_removal_delay_length_minus1, 5, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
            // write cpb_removal_delay_length_minus1 - u(5)
            WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.VclHrd.info.cpb_removal_delay_length_minus1, 5, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
            // write dpb_output_delay_length_minus1 - u(5)
            WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.VclHrd.info.dpb_output_delay_length_minus1, 5, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
            // write time_offset_length - u(5)
            WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.VclHrd.info.time_offset_length, 5, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        }

        if (spsHeader->sps_disp.vui_seq_parameters.nal_hrd_parameters_present_flag || spsHeader->sps_disp.vui_seq_parameters.vcl_hrd_parameters_present_flag)
        {
            // write low_delay_hrd_flag - u(1)
            WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.low_delay_hrd_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        }

        // write pic_struct_present_flag - u(1)
        WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.pic_struct_present_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        // write bitstream_restriction_flag - u(1)
        WRITE_BITS(spsHeader->sps_disp.vui_seq_parameters.bitstream_restriction_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);

        if (spsHeader->sps_disp.vui_seq_parameters.bitstream_restriction_flag)
        {
            // write motion_vectors_over_pic_boundaries_flag - u(1)
            // motion_vectors_over_pic_boundaries_flag is not in aligned_seq_param_set_t
            WRITE_BITS(1, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
            // write max_bytes_per_pic_denom - ue(v)
            // max_bytes_per_pic_denom is not in aligned_seq_param_set_t
            ENCODE_EXP_GOLOMB(2, SizeOfInBits(uint8_t), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
            // write max_bits_per_mb_denom - ue(v)
            // max_bits_per_mb_denom is not in aligned_seq_param_set_t
            ENCODE_EXP_GOLOMB(1, SizeOfInBits(uint8_t), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
            // write log2_max_mv_length_horizontal - ue(v)
            // log2_max_mv_length_horizontal is not in aligned_seq_param_set_t
            ENCODE_EXP_GOLOMB(13, SizeOfInBits(uint8_t), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
            // write log2_max_mv_length_vertical - ue(v)
            // log2_max_mv_length_vertical is not in aligned_seq_param_set_t
            ENCODE_EXP_GOLOMB(11, SizeOfInBits(uint8_t), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
            // write max_num_reorder_frames - ue(v)
            // max_num_reorder_frames is not in aligned_seq_param_set_t
            ENCODE_EXP_GOLOMB(0, SizeOfInBits(uint8_t), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
            // write max_dec_frame_buffering - ue(v)
            // max_dec_frame_buffering is not in aligned_seq_param_set_t
            ENCODE_EXP_GOLOMB(1, SizeOfInBits(uint8_t), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
        }
    }

    // flush the accumulator
    if (accumulatorValidBits != 0)
    {
        // it seems that to flush accumulator, we need to 1 bit 1, with the rest 0
        WRITE_BITS(1, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        if (accumulatorValidBits != 0)
        {
            WRITE_BITS(0, SizeOfInBits(uint8_t) - accumulatorValidBits, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        }
    }

    return eStatus;
}


MOS_STATUS CodechalCencDecodeAvc::EncodePPS(
    const PCODECHAL_DECRYPT_STATUS_REPORT_AVC statusDriver, 
    uint32_t bufSize, 
    uint8_t* &outBitStreamBuf, 
    uint32_t &writtenBytes)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    if (statusDriver == nullptr || outBitStreamBuf == nullptr)
    {
        CENC_DECODE_ASSERTMESSAGE("Invalid input parameters");
        return MOS_STATUS_NULL_POINTER;
    }

    aligned_pic_param_set_t* ppsHeader = &statusDriver->PicParams;
    uint8_t accumulator = 0;
    uint8_t accumulatorValidBits = 0;
    uint8_t *pAccumulator = &accumulator;
    uint8_t *pAccumulatorValidBits = &accumulatorValidBits;
    uint32_t *pWrittenBytes = &writtenBytes;

    // PPS starts with 00, 00, 01, 28
    WRITE_BITS( 0, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, false);
    WRITE_BITS( 0, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, false);
    WRITE_BITS( 1, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, false);
    WRITE_BITS(0x28, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, false);
    // write pic_parameter_set_id - ue(v)
    ENCODE_EXP_GOLOMB(ppsHeader->pic_parameter_set_id, SizeOfInBits(ppsHeader->pic_parameter_set_id),
        bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    // write seq_parameter_set_id - ue(v)
    ENCODE_EXP_GOLOMB(ppsHeader->seq_parameter_set_id, SizeOfInBits(ppsHeader->seq_parameter_set_id),
        bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    // write entropy_coding_mode_flag - u(1)
    WRITE_BITS(ppsHeader->entropy_coding_mode_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // write bottom_field_pic_order_in_frame_present_flag - u(1)
    // bottom_field_pic_order_in_frame_present_flag is not in aligned_pic_param_set_t
    WRITE_BITS(0, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // write num_slice_groups_minus1 - ue(v)
    ENCODE_EXP_GOLOMB(ppsHeader->num_slice_groups_minus1, SizeOfInBits(ppsHeader->num_slice_groups_minus1),
        bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);

    if (ppsHeader->num_slice_groups_minus1 > 0)
    {
        // write slice_group_map_type - ue(v)
        ENCODE_EXP_GOLOMB(ppsHeader->slice_group_map_type, SizeOfInBits(ppsHeader->slice_group_map_type),
            bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);

        switch (ppsHeader->slice_group_map_type)
        {
            case 0:
            {
                for (uint8_t cGroup = 0; cGroup <= ppsHeader->num_slice_groups_minus1; cGroup++)
                {
                    // write run_length_minus1 - ue(v)
                    // run_length_minus1 is not in aligned_pic_param_set_t
                    ENCODE_EXP_GOLOMB(0, SizeOfInBits(uint8_t), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
                }
                break;
            }
            case 2:
            {
                for (uint8_t cGroup = 0; cGroup < ppsHeader->num_slice_groups_minus1; cGroup++)
                {
                    // write top_left - ue(v)
                    // top_left is not in aligned_pic_param_set_t
                    ENCODE_EXP_GOLOMB(0, SizeOfInBits(uint8_t), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
                    // write bottom_right - ue(v)
                    // bottom_right is not in aligned_pic_param_set_t
                    ENCODE_EXP_GOLOMB(0, SizeOfInBits(uint8_t), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
                }
                break;
            }
            case 3:
            case 4:
            case 5:
            {
                // write slice_group_change_direction_flag - u(1)
                // bottom_field_pic_order_in_frame_present_flag is not in aligned_pic_param_set_t
                WRITE_BITS(0, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
                // write slice_group_change_rate_minus1 - ue(v)
                // slice_group_change_rate_minus1 is not in aligned_pic_param_set_t
                ENCODE_EXP_GOLOMB(0, SizeOfInBits(uint8_t /*ppsHeader->slice_group_change_rate_minus1*/),
                  bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
                break;
            }
            case 6:
            {
                // write pic_size_in_map_units_minus1 - ue(v)
                // pic_size_in_map_units_minus1 is not in aligned_pic_param_set_t
                ENCODE_EXP_GOLOMB(0, SizeOfInBits(uint8_t), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
                break;
            }
            default:
                // if ppsHeader->slice_group_map_type is 1 or bigger than 6, there is nothing to do
                break;
        }
    }

    // write num_ref_idx_l0_default_active_minus1 - ue(v)
    ENCODE_EXP_GOLOMB(ppsHeader->num_ref_idx_l0_active, SizeOfInBits(ppsHeader->num_ref_idx_l0_active),
        bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    // write num_ref_idx_l1_default_active_minus1 - ue(v)
    ENCODE_EXP_GOLOMB(ppsHeader->num_ref_idx_l1_active, SizeOfInBits(ppsHeader->num_ref_idx_l1_active),
        bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    // write weighted_pred_flag - u(1)
    WRITE_BITS(ppsHeader->weighted_pred_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // write weighted_bipred_idc - u(2)
    WRITE_BITS(ppsHeader->weighted_bipred_idc, 2, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // write pic_init_qp_minus26 - se(v)
    ENCODE_EXP_GOLOMB(SIGNED_VALUE(ppsHeader->pic_init_qp_minus26), SizeOfInBits(ppsHeader->pic_init_qp_minus26),
        bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    // write pic_init_qs_minus26 - se(v)
    ENCODE_EXP_GOLOMB(SIGNED_VALUE(ppsHeader->pic_init_qs_minus26), SizeOfInBits(ppsHeader->pic_init_qs_minus26),
        bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    // write chroma_qp_index_offset - se(v)
    if (IsFRextProfile(statusDriver->SeqParams.profile_idc))
    {
        ENCODE_EXP_GOLOMB(0, SizeOfInBits(int32_t), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    }
    else
    {
        ENCODE_EXP_GOLOMB(SIGNED_VALUE(ppsHeader->chroma_qp_index_offset), SizeOfInBits(ppsHeader->chroma_qp_index_offset),
            bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    }
    // write deblocking_filter_control_present_flag - u(1)
    WRITE_BITS(ppsHeader->deblocking_filter_control_present_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // write constrained_intra_pred_flag - u(1)
    WRITE_BITS(ppsHeader->constrained_intra_pred_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // write redundant_pic_cnt_present_flag - u(1)
    WRITE_BITS(ppsHeader->redundant_pic_cnt_present_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);

    if (IsFRextProfile(statusDriver->SeqParams.profile_idc))
    {
        // write transform_8x8_mode_flag - u(1)
        WRITE_BITS(ppsHeader->transform_8x8_mode_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        // write pic_scaling_matrix_present_flag - u(1)
        WRITE_BITS(ppsHeader->pic_scaling_matrix_present_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);

        if (ppsHeader->pic_scaling_matrix_present_flag)
        {
            for (uint8_t cIndex = 0; ppsHeader->transform_8x8_mode_flag <= 1 && cIndex < 6 + (2 * ppsHeader->transform_8x8_mode_flag); cIndex++)
            {
                // write pic_scaling_list_present_flag - u(1)
                WRITE_BITS(ppsHeader->pic_scaling_list_present_flag[cIndex], 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
                if (ppsHeader->pic_scaling_list_present_flag[cIndex])
                {
                    if (cIndex < 6)
                    {
                        eStatus = PackScalingList(ppsHeader->ScalingList4x4[cIndex], 16, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
                        if (eStatus != MOS_STATUS_SUCCESS)
                        {
                            return eStatus;
                        }
                    }
                    else
                    {
                        eStatus = PackScalingList(ppsHeader->ScalingList8x8[cIndex - 6], 64, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
                        if (eStatus != MOS_STATUS_SUCCESS)
                        {
                            return eStatus;
                        }
                    }
                }
            }
        }
        // write second_chroma_qp_index_offset - u(1)
        WRITE_BITS(static_cast<uint32_t>(ppsHeader->second_chroma_qp_index_offset), 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    }

    WRITE_BITS(1, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    if (accumulatorValidBits != 0)
    {
        WRITE_BITS(0, SizeOfInBits(uint8_t) - accumulatorValidBits, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    }

    return eStatus;
}

MOS_STATUS CodechalCencDecodeAvc::EncodeSeiBufferingPeriod(
    const PCODECHAL_DECRYPT_STATUS_REPORT_AVC statusDriver, 
    PCODECHAL_CENC_SEI seiBufPeriod)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    if (statusDriver == nullptr || seiBufPeriod == nullptr)
    {
        CENC_DECODE_ASSERTMESSAGE("Invalid input parameters");
        return MOS_STATUS_NULL_POINTER;
    }

    uint8_t* outBitStreamBuf = seiBufPeriod->data;
    uint32_t bufSize = sizeof(seiBufPeriod->data);
    uint32_t SchedSelIdx = 0;
    uint32_t byteWritten = 0;
    uint8_t accumulator = 0;
    uint8_t accumulatorValidBits = 0;
    uint8_t *pAccumulator = &accumulator;
    uint8_t *pAccumulatorValidBits = &accumulatorValidBits;
    uint32_t *pWrittenBytes = &byteWritten;
    aligned_seq_param_set_t *spsHeader = &statusDriver->SeqParams;

    // write seq_param_set_id - ue(v)
    ENCODE_EXP_GOLOMB(statusDriver->SeiBufferingPeriod.seq_param_set_id, SizeOfInBits(statusDriver->SeiBufferingPeriod.seq_param_set_id), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);

    if (spsHeader->sps_disp.vui_seq_parameters.nal_hrd_parameters_present_flag)
    {
        for (SchedSelIdx = 0; spsHeader->sps_disp.vui_seq_parameters.NalHrd.info.cpb_cnt_minus1 < MAX_CPB_CNT && SchedSelIdx <= spsHeader->sps_disp.vui_seq_parameters.NalHrd.info.cpb_cnt_minus1; SchedSelIdx++)
        {
            // write initial_cpb_removal_delay_nal[SchedSelIdx], u(spsHeader->sps_disp.vui_seq_parameters.NalHrd.info.initial_cpb_removal_delay_length_minus1 + 1)
            WRITE_BITS(statusDriver->SeiBufferingPeriod.initial_cpb_removal_delay_nal[SchedSelIdx], spsHeader->sps_disp.vui_seq_parameters.NalHrd.info.initial_cpb_removal_delay_length_minus1 + 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
            // write initial_cpb_removal_delay_offset_nal[SchedSelIdx], u(spsHeader->sps_disp.vui_seq_parameters.NalHrd.info.initial_cpb_removal_delay_length_minus1 + 1)
            WRITE_BITS(statusDriver->SeiBufferingPeriod.initial_cpb_removal_delay_offset_nal[SchedSelIdx], spsHeader->sps_disp.vui_seq_parameters.NalHrd.info.initial_cpb_removal_delay_length_minus1 + 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        }
    }

    if (spsHeader->sps_disp.vui_seq_parameters.vcl_hrd_parameters_present_flag)
    {
        for (SchedSelIdx = 0; spsHeader->sps_disp.vui_seq_parameters.VclHrd.info.cpb_cnt_minus1 < MAX_CPB_CNT && SchedSelIdx <= spsHeader->sps_disp.vui_seq_parameters.VclHrd.info.cpb_cnt_minus1; SchedSelIdx++)
        {
            // write initial_cpb_removal_delay_vcl[SchedSelIdx], u(spsHeader->sps_disp.vui_seq_parameters.VclHrd.info.initial_cpb_removal_delay_length_minus1 + 1)
            WRITE_BITS(statusDriver->SeiBufferingPeriod.initial_cpb_removal_delay_vcl[SchedSelIdx], spsHeader->sps_disp.vui_seq_parameters.VclHrd.info.initial_cpb_removal_delay_length_minus1 + 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
            // write initial_cpb_removal_delay_offset_vcl[SchedSelIdx], u(spsHeader->sps_disp.vui_seq_parameters.VclHrd.info.initial_cpb_removal_delay_length_minus1 + 1)
            WRITE_BITS(statusDriver->SeiBufferingPeriod.initial_cpb_removal_delay_offset_vcl[SchedSelIdx], spsHeader->sps_disp.vui_seq_parameters.VclHrd.info.initial_cpb_removal_delay_length_minus1 + 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        }
    }

    // flush the accumulator
    if (accumulatorValidBits != 0)
    {
        // TODO: it seems that to flush accumulator, we need to 1 bit 1, with the rest 0
        WRITE_BITS(1, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        if (accumulatorValidBits != 0)
        {
            WRITE_BITS(0, SizeOfInBits(uint8_t) - accumulatorValidBits, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        }
    }

    seiBufPeriod->payloadSize = byteWritten;

    return eStatus;
}

MOS_STATUS CodechalCencDecodeAvc::EncodeSeiPicTiming(
    const PCODECHAL_DECRYPT_STATUS_REPORT_AVC statusDriver, 
    PCODECHAL_CENC_SEI seiPicTiming)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    if (statusDriver == nullptr || seiPicTiming == nullptr)
    {
        CENC_DECODE_ASSERTMESSAGE("Invalid input parameters");
        return MOS_STATUS_NULL_POINTER;
    }

    uint8_t* outBitStreamBuf = seiPicTiming->data;
    uint32_t bufSize = sizeof(seiPicTiming->data);
    uint32_t byteWritten = 0;
    uint8_t accumulator = 0;
    uint8_t accumulatorValidBits = 0;
    uint8_t *pAccumulator = &accumulator;
    uint8_t *pAccumulatorValidBits = &accumulatorValidBits;
    uint32_t *pWrittenBytes = &byteWritten;
    h264_SEI_pic_timing_t *pDriverSeiPicTiming = &statusDriver->SeiPicTiming;
    aligned_seq_param_set_t *spsHeader = &statusDriver->SeqParams;

    // CpbDpbDelaysPresentFlag can also be set "by some means not specified in this Recommendation | International Standard"
    bool CpbDpbDelaysPresentFlag = (bool) (spsHeader->sps_disp.vui_parameters_present_flag && 
                                           ((spsHeader->sps_disp.vui_seq_parameters.nal_hrd_parameters_present_flag != 0) || 
                                            (spsHeader->sps_disp.vui_seq_parameters.vcl_hrd_parameters_present_flag != 0)));
    h264_hrd_param_set_t *hrd = nullptr;

    if (spsHeader->sps_disp.vui_seq_parameters.vcl_hrd_parameters_present_flag)
    {
        hrd = &(spsHeader->sps_disp.vui_seq_parameters.VclHrd);
    }
    else if (spsHeader->sps_disp.vui_seq_parameters.nal_hrd_parameters_present_flag)
    {
        hrd = &(spsHeader->sps_disp.vui_seq_parameters.NalHrd);
    }

    if (CpbDpbDelaysPresentFlag)
    {
        // write cpb_removal_delay, u(hrd->info.cpb_removal_delay_length_minus1 + 1)
        WRITE_BITS(pDriverSeiPicTiming->cpb_removal_delay, hrd->info.cpb_removal_delay_length_minus1 + 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        // write dpb_output_delay, u(hrd->info.dpb_output_delay_length_minus1  + 1)
        WRITE_BITS(pDriverSeiPicTiming->dpb_output_delay, hrd->info.dpb_output_delay_length_minus1  + 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    }
    
    if (spsHeader->sps_disp.vui_seq_parameters.pic_struct_present_flag)
    {
        // write pic_struct, u(4)
        WRITE_BITS(pDriverSeiPicTiming->pic_struct, 4, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    }

    // flush the accumulator
    if (accumulatorValidBits != 0)
    {
        // TODO: it seems that to flush accumulator, we need to 1 bit 1, with the rest 0
        WRITE_BITS(1, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        if (accumulatorValidBits != 0)
        {
            WRITE_BITS(0, SizeOfInBits(uint8_t) - accumulatorValidBits, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        }
    }

    seiPicTiming->payloadSize = byteWritten;

    return eStatus;
}

MOS_STATUS CodechalCencDecodeAvc::EncodeSeiRecoveryPoint(
    const PCODECHAL_DECRYPT_STATUS_REPORT_AVC statusDriver, 
    PCODECHAL_CENC_SEI seiRecPoint)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    if (statusDriver == nullptr || seiRecPoint == nullptr)
    {
        CENC_DECODE_ASSERTMESSAGE("Invalid input parameters");
        return MOS_STATUS_NULL_POINTER;
    }

    uint8_t* outBitStreamBuf = seiRecPoint->data;
    uint32_t bufSize = sizeof(seiRecPoint->data);
    uint32_t byteWritten = 0;
    uint8_t accumulator = 0;
    uint8_t accumulatorValidBits = 0;
    uint8_t *pAccumulator = &accumulator;
    uint8_t *pAccumulatorValidBits = &accumulatorValidBits;
    uint32_t *pWrittenBytes = &byteWritten;

    // write recovery_frame_cnt - ue(v)
    ENCODE_EXP_GOLOMB(statusDriver->SeiRecoveryPoint.recovery_frame_cnt, SizeOfInBits(statusDriver->SeiRecoveryPoint.recovery_frame_cnt), bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes);
    // write exact_match_flag - u(1)
    WRITE_BITS(statusDriver->SeiRecoveryPoint.exact_match_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // write broken_link_flag - u(1)
    WRITE_BITS(statusDriver->SeiRecoveryPoint.broken_link_flag, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
    // write changing_slice_group_idc - u(2)
    WRITE_BITS(statusDriver->SeiRecoveryPoint.changing_slice_group_idc, 2, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);

    // flush the accumulator
    if (accumulatorValidBits != 0)
    {
        // TODO: it seems that to flush accumulator, we need to 1 bit 1, with the rest 0
        WRITE_BITS(1, 1, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        if (accumulatorValidBits != 0)
        {
            WRITE_BITS(0, SizeOfInBits(uint8_t) - accumulatorValidBits, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, true);
        }
    }

    seiRecPoint->payloadSize = byteWritten;

    return eStatus;
}

MOS_STATUS CodechalCencDecodeAvc::EncodeSeiWriteMessage(
    PCODECHAL_CENC_SEI sei, 
    const uint8_t* payload, 
    const uint32_t payloadSize, 
    const uint8_t payloadType)
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

MOS_STATUS CodechalCencDecodeAvc::EncodeSEI(
    const PCODECHAL_DECRYPT_STATUS_REPORT_AVC statusDriver, 
    uint32_t bufSize, 
    uint8_t* &outBitStreamBuf, 
    uint32_t &writtenBytes)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    if (statusDriver == nullptr || outBitStreamBuf == nullptr)
    {
        CENC_DECODE_ASSERTMESSAGE("Invalid input parameters");
        return MOS_STATUS_NULL_POINTER;
    }

    uint8_t accumulator = 0;
    uint8_t accumulatorValidBits = 0;
    uint8_t *pAccumulator = &accumulator;
    uint8_t *pAccumulatorValidBits = &accumulatorValidBits;
    uint32_t *pWrittenBytes = &writtenBytes;

    CODECHAL_CENC_SEI payload = {0};
    CODECHAL_CENC_SEI sei = {0};

    // SEI starts with 00, 00, 01, 06
    WRITE_BITS( 0, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, false);
    WRITE_BITS( 0, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, false);
    WRITE_BITS( 1, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, false);
    WRITE_BITS(0x06, 8, bufSize, pAccumulator, pAccumulatorValidBits, outBitStreamBuf, pWrittenBytes, false);

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
    eStatus = EncodeSeiRecoveryPoint(statusDriver, &payload);
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
    eStatus = EncodeSeiWriteMessage(&sei, payload.data, payload.payloadSize, 6);  //recovery point payload type is 6
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

    return  eStatus;
}


