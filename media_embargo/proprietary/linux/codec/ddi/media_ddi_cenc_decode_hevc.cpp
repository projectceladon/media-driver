/*
* Copyright (c) 2018, Intel Corporation
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
//! \file     media_ddi_cenc_decode_hevc.cpp
//! \brief    The class implementation of DdiDecodeHevc for Hevc decode
//!

#include <va/va.h>
#include "va_protected_content_private.h"
#include "codechal_cenc_decode_hevc.h"

MOS_STATUS CodechalCencDecodeHevc::ParseSharedBufParams(
    uint16_t                    bufIdx,
    uint16_t                    reportIdx,
    void                        *buffer)
{
    MOS_STATUS                              eStatus = MOS_STATUS_SUCCESS;
    PCODECHAL_DECRYPT_SHARED_BUFFER_HEVC    pSharedBuf;
    PCODECHAL_DECRYPT_STATUS_REPORT_HEVC    pStatusDriver;
    VACencStatusBuf                         *pStatusApp;
    VACencSliceParameterBufferHEVC          *pSliceParam;
    PCODEC_HEVC_PIC_PARAMS                  pHevcPicParams;
    PCODECHAL_HEVC_IQ_MATRIX_PARAMS         pHevcQmParams;
    uint32_t                                dwMinCbs;
    uint8_t                                 *pBitStreamBuf = nullptr;
    uint32_t                                bytesWritten = 0;

    CENC_DECODE_FUNCTION_ENTER;

    CENC_DECODE_CHK_NULL(m_picParams);
    CENC_DECODE_CHK_NULL(m_qmParams);

    CodechalResLock shareBuffer(m_osInterface, &m_resHucSharedBuf);
    pSharedBuf = (PCODECHAL_DECRYPT_SHARED_BUFFER_HEVC)shareBuffer.Lock(CodechalResLock::readOnly);
    CENC_DECODE_CHK_NULL(pSharedBuf);

    pStatusDriver = (PCODECHAL_DECRYPT_STATUS_REPORT_HEVC)&pSharedBuf->StatusReport[bufIdx % CODECHAL_NUM_DECRYPT_STATUS_BUFFERS];
    pHevcPicParams = (PCODEC_HEVC_PIC_PARAMS)m_picParams[bufIdx];
    pHevcQmParams = (PCODECHAL_HEVC_IQ_MATRIX_PARAMS)m_qmParams[bufIdx];
    CENC_DECODE_VERBOSEMESSAGE("Parse buffer %d.", bufIdx);

    dwMinCbs = 1 << (pStatusDriver->SeqParams.sps_log2_min_luma_coding_block_size_minus3 + 3);

    m_picSize = pStatusDriver->PicInfo.uiLastSliceEndPos;
    CENC_DECODE_VERBOSEMESSAGE("HEVC decrypt status buffer size = %d.", sizeof(CODECHAL_DECRYPT_STATUS_REPORT_HEVC));
    // allocate Decrypt state internal resources needed by Decode
    CENC_DECODE_CHK_STATUS(AllocateDecodeResource((uint8_t)bufIdx));

    // populate params from share buffer into App Status Report buffer
    CENC_DECODE_CHK_NULL(buffer);
    pStatusApp = (VACencStatusBuf*)buffer;
    pSliceParam = (VACencSliceParameterBufferHEVC*)pStatusApp->slice_buf;

    if (pSliceParam == nullptr || pStatusApp->buf == nullptr)
    {
        CENC_DECODE_ASSERTMESSAGE("VACencStatusBuf's buf or slice_buf is NULL.");
        return MOS_STATUS_NULL_POINTER;
    }

    if (pStatusApp->slice_buf_type != VaCencSliceBufParamter || 
        pStatusApp->slice_buf_size != sizeof(VACencSliceParameterBufferHEVC))
    {
        CENC_DECODE_ASSERTMESSAGE("VACencStatusBuf's slice_buf_type or slice_buf_size is not correct.");
        return MOS_STATUS_INVALID_PARAMETER;
    }
    
    pStatusApp->status = VA_ENCRYPTION_STATUS_SUCCESSFUL;

    // Ref Pic, SEI, etc
    pStatusApp->status_report_index_feedback = pStatusDriver->PicInfo.ui16StatusReportFeebackNumber;
    pSliceParam->slice_pic_order_cnt_lsb = pStatusDriver->PicInfo.ui16SlicePicOrderCntLsb;
    pSliceParam->slice_type = pStatusDriver->PicInfo.ui8SliceType;
    pSliceParam->nal_unit_type = pStatusDriver->PicInfo.ui8NalUnitType;
    pSliceParam->nuh_temporal_id = pStatusDriver->PicInfo.ui8NuhTemporalId;
    pSliceParam->has_eos_or_eob = pStatusDriver->PicInfo.ui8HasEOS;
    CENC_DECODE_VERBOSEMESSAGE("ui8HasEOS %d", pStatusDriver->PicInfo.ui8HasEOS);
    pSliceParam->slice_fields.bits.pic_output_flag = pStatusDriver->PicInfo.ui8PicOutputFlag;
    pSliceParam->slice_fields.bits.no_output_of_prior_pics_flag = pStatusDriver->PicInfo.ui8NoOutputOfPriorPicsFlag;
    pSliceParam->slice_fields.bits.colour_plane_id = pStatusDriver->PicInfo.ui8ColourPlaneId;

    pSliceParam->num_of_curr_before = pStatusDriver->RefFrames.ref_set_num_of_curr_before;
    pSliceParam->num_of_curr_after = pStatusDriver->RefFrames.ref_set_num_of_curr_after;
    pSliceParam->num_of_curr_total = pStatusDriver->RefFrames.ref_set_num_of_curr_total;
    pSliceParam->num_of_foll_st = pStatusDriver->RefFrames.ref_set_num_of_foll_st;
    pSliceParam->num_of_curr_lt = pStatusDriver->RefFrames.ref_set_num_of_curr_lt;
    pSliceParam->num_of_foll_lt = pStatusDriver->RefFrames.ref_set_num_of_foll_lt;
    MOS_SecureMemcpy(pSliceParam->delta_poc_curr_before,
        sizeof(pSliceParam->delta_poc_curr_before),
        pStatusDriver->RefFrames.ref_set_deltapoc_curr_before,
        sizeof(pSliceParam->delta_poc_curr_before));
    MOS_SecureMemcpy(pSliceParam->delta_poc_curr_after,
        sizeof(pSliceParam->delta_poc_curr_after),
        pStatusDriver->RefFrames.ref_set_deltapoc_curr_after,
        sizeof(pSliceParam->delta_poc_curr_after));
    MOS_SecureMemcpy(pSliceParam->delta_poc_curr_total,
        sizeof(pSliceParam->delta_poc_curr_total),
        pStatusDriver->RefFrames.ref_set_deltapoc_curr_total,
        sizeof(pSliceParam->delta_poc_curr_total));
    MOS_SecureMemcpy(pSliceParam->delta_poc_foll_st,
        sizeof(pSliceParam->delta_poc_foll_st),
        pStatusDriver->RefFrames.ref_set_deltapoc_foll_st,
        sizeof(pSliceParam->delta_poc_foll_st));
    MOS_SecureMemcpy(pSliceParam->delta_poc_curr_lt,
        sizeof(pSliceParam->delta_poc_curr_lt),
        pStatusDriver->RefFrames.ref_set_deltapoc_curr_lt,
        sizeof(pSliceParam->delta_poc_curr_lt));
    MOS_SecureMemcpy(pSliceParam->delta_poc_foll_lt,
        sizeof(pSliceParam->delta_poc_foll_lt),
        pStatusDriver->RefFrames.ref_set_deltapoc_foll_lt,
        sizeof(pSliceParam->delta_poc_foll_lt));
    MOS_SecureMemcpy(pSliceParam->delta_poc_msb_present_flag,
        sizeof(pSliceParam->delta_poc_msb_present_flag),
        pStatusDriver->RefFrames.ref_set_deltapoc_msb_present_flag,
        sizeof(pSliceParam->delta_poc_msb_present_flag));
    MOS_SecureMemcpy(pSliceParam->is_lt_curr_total,
        sizeof(pSliceParam->is_lt_curr_total),
        pStatusDriver->RefFrames.ref_set_is_lt_curr_total,
        sizeof(pSliceParam->is_lt_curr_total));
    MOS_SecureMemcpy(pSliceParam->ref_list_idx,
        sizeof(pSliceParam->ref_list_idx),
        pStatusDriver->RefFrames.ref_list_idx,
        sizeof(pSliceParam->ref_list_idx));

    // SPS
    pBitStreamBuf = (uint8_t*)pStatusApp->buf;
    eStatus = EncodeSPS(pStatusDriver, pStatusApp->buf_size, pBitStreamBuf, bytesWritten);
    if (eStatus != MOS_STATUS_SUCCESS)
    {
        if (eStatus == MOS_STATUS_NOT_ENOUGH_BUFFER)
        {
            pStatusApp->status = VA_ENCRYPTION_STATUS_BUFFER_FULL;
            CENC_DECODE_ASSERTMESSAGE("Bitstream buffer full!");
            return MOS_STATUS_SUCCESS;  //Return success for App to not break execution but enlarge the buffer and query again
        }
        return eStatus;
    }
    // PPS
    eStatus = EncodePPS(pStatusDriver, pStatusApp->buf_size, pBitStreamBuf, bytesWritten);
    if (eStatus != MOS_STATUS_SUCCESS)
    {
        if (eStatus == MOS_STATUS_NOT_ENOUGH_BUFFER)
        {
            pStatusApp->status = VA_ENCRYPTION_STATUS_BUFFER_FULL;
            CENC_DECODE_ASSERTMESSAGE("Bitstream buffer full!");
            return MOS_STATUS_SUCCESS;  //Return success for App to not break execution but enlarge the buffer and query again
        }
        return eStatus;
    }
    // SEI: BufferingPeriod/PicTiming/RecoveryPoint
    eStatus = EncodeSEI(pStatusDriver, pStatusApp->buf_size, pBitStreamBuf, bytesWritten);
    if (eStatus != MOS_STATUS_SUCCESS)
    {
        if (eStatus == MOS_STATUS_NOT_ENOUGH_BUFFER)
        {
            pStatusApp->status = VA_ENCRYPTION_STATUS_BUFFER_FULL;
            CENC_DECODE_ASSERTMESSAGE("Bitstream buffer full!");
            return MOS_STATUS_SUCCESS;  //Return success for App to not break execution but enlarge the buffer and query again
        }
        return eStatus;
    }

    //load QM
    if (pStatusDriver->SeqParams.sps_scaling_list_enabled_flag)
    {
        if ((pStatusDriver->PicParams.pps_scaling_list_data_present_flag) || (pStatusDriver->SeqParams.sps_scaling_list_data_present_flag))
        {
            ScalingListSpsPps(pStatusDriver, pHevcQmParams);
        }
        else
        {
            ScalingListDefault(pHevcQmParams);
        }
    }
    else
    {
        ScalingListFlat(pHevcQmParams);
    }

    CENC_DECODE_VERBOSEMESSAGE("Verifying Pic Param parsing.");

    // populate params from what HuC returned to driver
    pHevcPicParams->PicWidthInMinCbsY = (uint16_t)((pStatusDriver->SeqParams.sps_pic_width_in_luma_samples + dwMinCbs - 1) / dwMinCbs);
    pHevcPicParams->PicHeightInMinCbsY = (uint16_t)((pStatusDriver->SeqParams.sps_pic_height_in_luma_samples + dwMinCbs - 1) / dwMinCbs);

    pHevcPicParams->chroma_format_idc = pStatusDriver->SeqParams.sps_chroma_format_idc;
    pHevcPicParams->separate_colour_plane_flag = pStatusDriver->SeqParams.sps_separate_colour_plane_flag;
    pHevcPicParams->bit_depth_luma_minus8 = pStatusDriver->SeqParams.sps_bit_depth_luma_minus8;
    pHevcPicParams->bit_depth_chroma_minus8 = pStatusDriver->SeqParams.sps_bit_depth_chroma_minus8;
    pHevcPicParams->log2_max_pic_order_cnt_lsb_minus4 = pStatusDriver->SeqParams.sps_log2_max_pic_order_cnt_lsb_minus4;

    //pHevcPicParams->sps_max_dec_pic_buffering_minus1            = pStatusDriver->SeqParams.sps_max_dec_pic_buffering_minus1;
    pHevcPicParams->log2_min_luma_coding_block_size_minus3 = (uint8_t)pStatusDriver->SeqParams.sps_log2_min_luma_coding_block_size_minus3;
    pHevcPicParams->log2_diff_max_min_luma_coding_block_size = (uint8_t)pStatusDriver->SeqParams.sps_log2_diff_max_min_luma_coding_block_size;
    pHevcPicParams->log2_min_transform_block_size_minus2 = (uint8_t)pStatusDriver->SeqParams.sps_log2_min_transform_block_size_minus2;
    pHevcPicParams->log2_diff_max_min_transform_block_size = (uint8_t)pStatusDriver->SeqParams.sps_log2_diff_max_min_transform_block_size;
    pHevcPicParams->max_transform_hierarchy_depth_inter = (uint8_t)pStatusDriver->SeqParams.sps_max_transform_hierarchy_depth_inter;
    pHevcPicParams->max_transform_hierarchy_depth_intra = (uint8_t)pStatusDriver->SeqParams.sps_max_transform_hierarchy_depth_intra;
    pHevcPicParams->num_short_term_ref_pic_sets = pStatusDriver->SeqParams.sps_num_short_term_ref_pic_sets;
    pHevcPicParams->num_long_term_ref_pic_sps = pStatusDriver->SeqParams.sps_num_long_term_ref_pics_sps;
    pHevcPicParams->num_ref_idx_l0_default_active_minus1 = pStatusDriver->PicParams.pps_num_ref_idx_l0_default_active_minus1;
    pHevcPicParams->num_ref_idx_l1_default_active_minus1 = pStatusDriver->PicParams.pps_num_ref_idx_l1_default_active_minus1;
    pHevcPicParams->init_qp_minus26 = (char)pStatusDriver->PicParams.pps_init_qp_minus26;

    pHevcPicParams->scaling_list_enabled_flag = pStatusDriver->SeqParams.sps_scaling_list_enabled_flag;
    pHevcPicParams->amp_enabled_flag = pStatusDriver->SeqParams.sps_amp_enabled_flag;
    pHevcPicParams->sample_adaptive_offset_enabled_flag = pStatusDriver->SeqParams.sps_sample_adaptive_offset_enabled_flag;
    pHevcPicParams->pcm_enabled_flag = pStatusDriver->SeqParams.sps_pcm_enabled_flag;
    pHevcPicParams->pcm_sample_bit_depth_luma_minus1 = pStatusDriver->SeqParams.sps_pcm_sample_bit_depth_luma_minus1;
    pHevcPicParams->pcm_sample_bit_depth_chroma_minus1 = pStatusDriver->SeqParams.sps_pcm_sample_bit_depth_chroma_minus1;
    pHevcPicParams->log2_min_pcm_luma_coding_block_size_minus3 = pStatusDriver->SeqParams.sps_log2_min_pcm_luma_coding_block_size_minus3;
    pHevcPicParams->log2_diff_max_min_pcm_luma_coding_block_size = pStatusDriver->SeqParams.sps_log2_diff_max_min_pcm_luma_coding_block_size;
    pHevcPicParams->pcm_loop_filter_disabled_flag = pStatusDriver->SeqParams.sps_pcm_loop_filter_disable_flag;
    pHevcPicParams->long_term_ref_pics_present_flag = pStatusDriver->SeqParams.sps_long_term_ref_pics_present_flag;
    pHevcPicParams->sps_temporal_mvp_enabled_flag = pStatusDriver->SeqParams.sps_temporal_mvp_enabled_flag;
    pHevcPicParams->strong_intra_smoothing_enabled_flag = pStatusDriver->SeqParams.sps_strong_intra_smoothing_enabled_flag;
    pHevcPicParams->dependent_slice_segments_enabled_flag = pStatusDriver->PicParams.pps_dependent_slice_segments_enabled_flag;
    pHevcPicParams->output_flag_present_flag = pStatusDriver->PicParams.pps_output_flag_present_flag;
    pHevcPicParams->num_extra_slice_header_bits = pStatusDriver->PicParams.pps_num_extra_slice_header_bits;
    pHevcPicParams->sign_data_hiding_enabled_flag = pStatusDriver->PicParams.pps_sign_data_hiding_flag;
    pHevcPicParams->cabac_init_present_flag = pStatusDriver->PicParams.pps_cabac_init_present_flag;

    pHevcPicParams->constrained_intra_pred_flag = pStatusDriver->PicParams.pps_constrained_intra_pred_flag;
    pHevcPicParams->transform_skip_enabled_flag = pStatusDriver->PicParams.pps_transform_skip_enabled_flag;
    pHevcPicParams->cu_qp_delta_enabled_flag = pStatusDriver->PicParams.pps_cu_qp_delta_enabled_flag;
    pHevcPicParams->pps_slice_chroma_qp_offsets_present_flag = pStatusDriver->PicParams.pps_slice_chroma_qp_offsets_present_flag;
    pHevcPicParams->weighted_pred_flag = pStatusDriver->PicParams.pps_weighted_pred_flag;
    pHevcPicParams->weighted_bipred_flag = pStatusDriver->PicParams.pps_weighted_bipred_flag;
    pHevcPicParams->transquant_bypass_enabled_flag = pStatusDriver->PicParams.pps_transquant_bypass_enabled_flag;
    pHevcPicParams->tiles_enabled_flag = pStatusDriver->PicParams.pps_tiles_enabled_flag;
    pHevcPicParams->entropy_coding_sync_enabled_flag = pStatusDriver->PicParams.pps_entropy_coding_sync_enabled_flag;
    pHevcPicParams->uniform_spacing_flag = pStatusDriver->PicParams.pps_uniform_spacing_flag;
    pHevcPicParams->loop_filter_across_tiles_enabled_flag = pStatusDriver->PicParams.pps_loop_filter_across_tiles_enabled_flag;
    pHevcPicParams->pps_loop_filter_across_slices_enabled_flag = pStatusDriver->PicParams.pps_loop_filter_across_slices_enabled_flag;
    pHevcPicParams->deblocking_filter_override_enabled_flag = pStatusDriver->PicParams.pps_deblocking_filter_override_enabled_flag;
    pHevcPicParams->pps_deblocking_filter_disabled_flag = pStatusDriver->PicParams.pps_disable_deblocking_filter_flag;
    pHevcPicParams->lists_modification_present_flag = pStatusDriver->PicParams.pps_lists_modification_present_flag;
    pHevcPicParams->slice_segment_header_extension_present_flag = pStatusDriver->PicParams.pps_slice_segment_header_extension_present_flag;

    // DHXTODO: per HEVC owner, below params are not used by driver so far
    // pHevcPicParams->NoPicReorderingFlag
    // pHevcPicParams->NoBiPredFlag
    // pHevcPicParams->IrapPicFlag
    // pHevcPicParams->IdrPicFlag
    // pHevcPicParams->wNumBitsForShortTermRPSInSlice
    // pHevcPicParams->CurrPicOrderCntVal       // coming from DDI now in ParseHevcPicParams()

    // IntraPicFlag is mainly used to calculate perf tag in the future, current code which related to IntraPicFlag for other usage will be removed.
    //pHevcPicParams->IntraPicFlag                = pStatusDriver->PicParams.irap;  // derive inside UMD

    pHevcPicParams->pps_cb_qp_offset = (char)pStatusDriver->PicParams.pps_cb_qp_offset;
    pHevcPicParams->pps_cr_qp_offset = (char)pStatusDriver->PicParams.pps_cr_qp_offset;
    pHevcPicParams->num_tile_columns_minus1 = pStatusDriver->PicParams.pps_num_tile_columns_minus1;
    pHevcPicParams->num_tile_rows_minus1 = pStatusDriver->PicParams.pps_num_tile_rows_minus1;

    MOS_SecureMemcpy(&pHevcPicParams->column_width_minus1[0], sizeof(uint16_t) * (HEVC_NUM_MAX_TILE_COLUMN - 1),
        &pStatusDriver->PicParams.pps_column_width_minus1[0], sizeof(uint16_t) * (HEVC_NUM_MAX_TILE_COLUMN - 1));

    MOS_SecureMemcpy(&pHevcPicParams->row_height_minus1[0], sizeof(uint16_t) * (HEVC_NUM_MAX_TILE_ROW - 1),
        &pStatusDriver->PicParams.pps_row_height_minus1[0], sizeof(uint16_t) * (HEVC_NUM_MAX_TILE_ROW - 1));

    pHevcPicParams->diff_cu_qp_delta_depth = (uint8_t)pStatusDriver->PicParams.pps_diff_cu_qp_delta_depth;
    pHevcPicParams->pps_beta_offset_div2 = pStatusDriver->PicParams.pps_beta_offset_div2;
    pHevcPicParams->pps_tc_offset_div2 = pStatusDriver->PicParams.pps_tc_offset_div2;
    pHevcPicParams->log2_parallel_merge_level_minus2 = (uint8_t)pStatusDriver->PicParams.pps_log2_parallel_merge_level_minus2;
    pHevcPicParams->CurrPicOrderCntVal = pStatusDriver->PicInfo.iCurrPicOrderCntVal;
    pHevcPicParams->dwLastSliceEndPos = pStatusDriver->PicInfo.uiLastSliceEndPos;

    return eStatus;
}

