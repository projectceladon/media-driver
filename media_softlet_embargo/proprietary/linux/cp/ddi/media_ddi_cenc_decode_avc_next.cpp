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
//! \file     media_ddi_cenc_decode_avc_next.cpp
//! \brief    The class implementation of CencDecode DDI part  for Avc decode
//!

#include <va/va.h>
#include "va_protected_content_private.h"
#include "codechal_cenc_decode_avc_next.h"

MOS_STATUS CodechalCencDecodeAvcNext::ParseSharedBufParams(
    uint16_t bufIdx,
    uint16_t reportIdx,
    void *   buffer)
{
    MOS_STATUS                          eStatus = MOS_STATUS_SUCCESS;
    PCODECHAL_DECRYPT_SHARED_BUFFER_AVC sharedBuf;
    PCODECHAL_DECRYPT_STATUS_REPORT_AVC statusDriver;
    VACencStatusBuf                     *statusApp;
    VACencSliceParameterBufferH264      *sliceParam;
    PCODEC_AVC_PIC_PARAMS               avcPicParams;
    PCODEC_AVC_IQ_MATRIX_PARAMS         avcQmParams;
    uint8_t                             frame_mbs_only_flag;
    uint8_t                             idx;
    size_t                              ppsCopySize;
    uint16_t                            mbHeight;
    uint8_t                             *pBitStreamBuf = nullptr;
    uint32_t                            bytesWritten = 0;

    CENC_DECODE_FUNCTION_ENTER;

    CENC_DECODE_CHK_NULL(m_hucSharedBuf);
    CENC_DECODE_CHK_NULL(m_picParams);
    CENC_DECODE_CHK_NULL(m_qmParams);
    CENC_DECODE_CHK_NULL(buffer);

    CodechalResLock shareBuffer(m_osInterface, &m_resHucSharedBuf);
    sharedBuf = (PCODECHAL_DECRYPT_SHARED_BUFFER_AVC)shareBuffer.Lock(CodechalResLock::readOnly);
    CENC_DECODE_CHK_NULL(sharedBuf);

    statusDriver = (PCODECHAL_DECRYPT_STATUS_REPORT_AVC)&sharedBuf->StatusReport[bufIdx % CODECHAL_NUM_DECRYPT_STATUS_BUFFERS];
    statusApp    = (VACencStatusBuf*)buffer;
    sliceParam = (VACencSliceParameterBufferH264*)statusApp->slice_buf;
    avcPicParams = (PCODEC_AVC_PIC_PARAMS)m_picParams[bufIdx];
    avcQmParams  = (PCODEC_AVC_IQ_MATRIX_PARAMS)m_qmParams[bufIdx];
    CENC_DECODE_VERBOSEMESSAGE("Parse buffer %d.", bufIdx);

    if (sliceParam == nullptr || statusApp->buf == nullptr)
    {
        CENC_DECODE_ASSERTMESSAGE("VACencStatusBuf's buf or slice_buf is NULL.");
        return MOS_STATUS_NULL_POINTER;
    }

    if (statusApp->slice_buf_type != VaCencSliceBufParamter || 
        statusApp->slice_buf_size != sizeof(VACencSliceParameterBufferH264))
    {
        CENC_DECODE_ASSERTMESSAGE("VACencStatusBuf's slice_buf_type or slice_buf_size is not correct.");
        return MOS_STATUS_INVALID_PARAMETER;
    }

    MOS_TraceDumpExt("HucDecryptOutputAVC", bufIdx, statusDriver, sizeof(*statusDriver));

    if (statusDriver->SeqParams.seq_parameter_set_id == CODEC_AVC_MAX_SPS_NUM + 1 &&
        statusDriver->PicParams.pic_parameter_set_id == CODEC_AVC_MAX_PPS_NUM + 1)
    {
        // No slices were parsed by HuC FW, so there will be no data in the status report
        statusApp->status                     = VA_ENCRYPTION_STATUS_ERROR;
        statusApp->status_report_index_feedback = statusDriver->PicInfo.ui16StatusReportFeedbackNumber;
        return eStatus;
    }

    frame_mbs_only_flag = statusDriver->SeqParams.sps_disp.frame_mbs_only_flag;
    mbHeight            = (statusDriver->SeqParams.sps_disp.pic_height_in_map_units_minus1 + 1) << uint16_t(frame_mbs_only_flag == 0);

    // calculate max number of slices according to profile level
    m_numSlices =
        GetMaxSupportedSlices(mbHeight);
    m_picSize =
        GetMaxBitstream((statusDriver->SeqParams.sps_disp.pic_width_in_mbs_minus1 + 1), mbHeight);
    CENC_DECODE_VERBOSEMESSAGE("level = %d, max slice = %d. max bitstream = %d", statusDriver->SeqParams.level_idc, m_numSlices, m_picSize);
    CENC_DECODE_VERBOSEMESSAGE("AVC decrypt status buffer size = %d.", sizeof(CODECHAL_DECRYPT_STATUS_REPORT_AVC));

    // allocate Decrypt state internal resources needed by Decode
    CENC_DECODE_CHK_STATUS(AllocateDecodeResource((uint8_t)bufIdx));

    CENC_DECODE_VERBOSEMESSAGE("ErrorReportFeedbackNumber = %d, ErrBufferFull = %d.",
        sharedBuf->ErrorReport.ui16ErrorReportFeedbackNumber,
        sharedBuf->ErrorReport.ui8ErrBufferFull);

    // populate params from share buffer into App Status Report buffer
    statusApp->status = VA_ENCRYPTION_STATUS_SUCCESSFUL;
    statusApp->status_report_index_feedback = statusDriver->PicInfo.ui16StatusReportFeedbackNumber;


    // uncompressed slice header
    sliceParam->nal_ref_idc = statusDriver->PicInfo.nal_ref_idc;
    sliceParam->idr_pic_flag = statusDriver->PicInfo.idr_flag;
    sliceParam->slice_type = statusDriver->PicInfo.slice_type;
    sliceParam->field_frame_flag = statusDriver->PicInfo.field_pic_flag;
    CENC_DECODE_COND_ASSERTMESSAGE((sliceParam->field_frame_flag != 3), "ERROR - interlaced field is not supported by HuC-based DRM");
    sliceParam->frame_number = statusDriver->PicInfo.frame_num;
    if (statusDriver->PicInfo.slice_type > SLICE_I)
    {
        CENC_DECODE_ASSERTMESSAGE("ERROR, not a valice slice type %d", statusDriver->PicInfo.slice_type);
    }

    sliceParam->idr_pic_id= statusDriver->PicInfo.idr_pic_id;
    sliceParam->pic_order_cnt_lsb = statusDriver->PicInfo.pic_order_cnt_lsb;
    sliceParam->delta_pic_order_cnt_bottom = statusDriver->PicInfo.delta_pic_order_cnt_bottom;
    sliceParam->delta_pic_order_cnt[0] = statusDriver->PicInfo.delta_pic_order_cnt[0];
    sliceParam->delta_pic_order_cnt[1] = statusDriver->PicInfo.delta_pic_order_cnt[1];

    // Ref Pic Marking
    sliceParam->ref_pic_fields.bits.no_output_of_prior_pics_flag = statusDriver->RefPicMarking.no_output_of_prior_pics_flag;
    sliceParam->ref_pic_fields.bits.long_term_reference_flag = statusDriver->RefPicMarking.long_term_reference_flag;
    sliceParam->ref_pic_fields.bits.adaptive_ref_pic_marking_mode_flag = statusDriver->RefPicMarking.adaptive_ref_pic_marking_mode_flag;
    sliceParam->ref_pic_fields.bits.dec_ref_pic_marking_count = statusDriver->RefPicMarking.dec_ref_pic_marking_count;
    MOS_SecureMemcpy(sliceParam->memory_management_control_operation,
        sizeof(statusDriver->RefPicMarking.memory_management_control_operation),
        statusDriver->RefPicMarking.memory_management_control_operation,
        sizeof(statusDriver->RefPicMarking.memory_management_control_operation));
    MOS_SecureMemcpy(sliceParam->difference_of_pic_nums_minus1,
        sizeof(statusDriver->RefPicMarking.difference_of_pic_num_minus1),
        statusDriver->RefPicMarking.difference_of_pic_num_minus1,
        sizeof(statusDriver->RefPicMarking.difference_of_pic_num_minus1));
    MOS_SecureMemcpy(sliceParam->long_term_pic_num,
        sizeof(statusDriver->RefPicMarking.long_term_pic_num),
        statusDriver->RefPicMarking.long_term_pic_num,
        sizeof(statusDriver->RefPicMarking.long_term_pic_num));

    for (idx = 0; idx < NUM_MMCO_OPERATIONS; idx++)
    {
        sliceParam->max_long_term_frame_idx_plus1[idx] = statusDriver->RefPicMarking.max_long_term_frame_idx_plus1[idx];
        sliceParam->long_term_frame_idx[idx] = statusDriver->RefPicMarking.long_term_frame_idx[idx];
    }

    // SPS
    pBitStreamBuf = (uint8_t*)statusApp->buf;
    eStatus = EncodeSPS(statusDriver, statusApp->buf_size, pBitStreamBuf, bytesWritten);
    if (eStatus != MOS_STATUS_SUCCESS)
    {
        if (eStatus == MOS_STATUS_NOT_ENOUGH_BUFFER)
        {
            statusApp->status = VA_ENCRYPTION_STATUS_BUFFER_FULL;
            CENC_DECODE_ASSERTMESSAGE("Bitstream buffer full!");
            return MOS_STATUS_SUCCESS;  //Return success for App to not break execution but enlarge the buffer and query again
        }
        return eStatus;
    }
    // PPS
    eStatus = EncodePPS(statusDriver, statusApp->buf_size, pBitStreamBuf, bytesWritten);
    if (eStatus != MOS_STATUS_SUCCESS)
    {
        if (eStatus == MOS_STATUS_NOT_ENOUGH_BUFFER)
        {
            statusApp->status = VA_ENCRYPTION_STATUS_BUFFER_FULL;
            CENC_DECODE_ASSERTMESSAGE("Bitstream buffer full!");
            return MOS_STATUS_SUCCESS;  //Return success for App to not break execution but enlarge the buffer and query again
        }
        return eStatus;
    }
    // SEI: BufferingPeriod/PicTiming/RecoveryPoint
    eStatus = EncodeSEI(statusDriver, statusApp->buf_size, pBitStreamBuf, bytesWritten);
    if (eStatus != MOS_STATUS_SUCCESS)
    {
        if (eStatus == MOS_STATUS_NOT_ENOUGH_BUFFER)
        {
            statusApp->status = VA_ENCRYPTION_STATUS_BUFFER_FULL;
            CENC_DECODE_ASSERTMESSAGE("Bitstream buffer full!");
            return MOS_STATUS_SUCCESS;  //Return success for App to not break execution but enlarge the buffer and query again
        }
        return eStatus;
    }

    for (idx = 0; idx < 8; idx++)
    {
        if (idx < 6)
        {
            if (statusDriver->PicParams.pic_scaling_matrix_present_flag)
            {
                if (statusDriver->PicParams.pic_scaling_list_present_flag[idx])
                {
                    if (statusDriver->PicParams.UseDefaultScalingMatrix4x4Flag[idx])
                    {
                        ScalingListFallbackA(avcQmParams, idx);
                    }
                    else
                    {
                        ScalingList(
                            &(statusDriver->PicParams.ScalingList4x4[idx][0]),
                            &(avcQmParams->ScalingList4x4[idx][0]),
                            idx);
                    }
                }
                else if (statusDriver->SeqParams.seq_scaling_matrix_present_flag)
                {
                    // FallbackB if pic_scaling_list_present_flag[idx] is to use sequence level scaling list
                    if (statusDriver->SeqParams.seq_scaling_list_present_flag[idx])
                    {
                        if (statusDriver->SeqParams.UseDefaultScalingMatrix4x4Flag[idx])
                        {
                            ScalingListFallbackA(avcQmParams, idx);
                        }
                        else
                        {
                            ScalingListFallbackB(
                                avcQmParams,
                                &(statusDriver->SeqParams.ScalingList4x4[idx][0]),
                                idx);
                        }
                    }
                    else
                    {
                        ScalingListFallbackA(avcQmParams, idx);
                    }
                }
                else
                {
                    ScalingListFallbackA(avcQmParams, idx);
                }
            }
            else if (statusDriver->SeqParams.seq_scaling_matrix_present_flag)
            {
                if (statusDriver->SeqParams.seq_scaling_list_present_flag[idx])
                {
                    if (statusDriver->SeqParams.UseDefaultScalingMatrix4x4Flag[idx])
                    {
                        ScalingListFallbackA(avcQmParams, idx);
                    }
                    else
                    {
                        ScalingList(
                            &(statusDriver->SeqParams.ScalingList4x4[idx][0]),
                            &(avcQmParams->ScalingList4x4[idx][0]),
                            idx);
                    }
                }
                else
                {
                    ScalingListFallbackA(avcQmParams, idx);
                }
            }
            else
            {
                ScalingListFallbackB(avcQmParams, nullptr, idx);
            }
        }
        else
        {
            if (statusDriver->PicParams.pic_scaling_matrix_present_flag)
            {
                if (statusDriver->PicParams.pic_scaling_list_present_flag[idx])
                {
                    if (statusDriver->PicParams.UseDefaultScalingMatrix8x8Flag[idx - 6])
                    {
                        ScalingListFallbackA(avcQmParams, idx);
                    }
                    else
                    {
                        ScalingList(
                            &(statusDriver->PicParams.ScalingList8x8[idx - 6][0]),
                            &(avcQmParams->ScalingList8x8[idx - 6][0]),
                            idx);
                    }
                }
                else if (statusDriver->SeqParams.seq_scaling_matrix_present_flag)
                {
                    // FallbackB if pic_scaling_list_present_flag[idx] is to use sequence level scaling list
                    if (statusDriver->SeqParams.seq_scaling_list_present_flag[idx])
                    {
                        if (statusDriver->SeqParams.UseDefaultScalingMatrix8x8Flag[idx - 6])
                        {
                            ScalingListFallbackA(avcQmParams, idx);
                        }
                        else
                        {
                            ScalingListFallbackB(
                                avcQmParams,
                                &(statusDriver->SeqParams.ScalingList8x8[idx - 6][0]),
                                idx);
                        }
                    }
                    else
                    {
                        ScalingListFallbackA(avcQmParams, idx);
                    }
                }
                else
                {
                    ScalingListFallbackA(avcQmParams, idx);
                }
            }
            else if (statusDriver->SeqParams.seq_scaling_matrix_present_flag)
            {
                if (statusDriver->SeqParams.seq_scaling_list_present_flag[idx])
                {
                    if (statusDriver->SeqParams.UseDefaultScalingMatrix8x8Flag[idx - 6])
                    {
                        ScalingListFallbackA(avcQmParams, idx);
                    }
                    else
                    {
                        ScalingList(
                            &(statusDriver->SeqParams.ScalingList8x8[idx - 6][0]),
                            &(avcQmParams->ScalingList8x8[idx - 6][0]),
                            idx);
                    }
                }
                else
                {
                    ScalingListFallbackA(avcQmParams, idx);
                }
            }
            else
            {
                ScalingListFallbackB(avcQmParams, nullptr, idx);
            }
        }
    }

    CENC_DECODE_VERBOSEMESSAGE("Verifying Pic Param parsing.");

    // populdate params from what HuC returned to driver
    avcPicParams->CurrFieldOrderCnt[0]     = statusDriver->PicInfo.iCurrFieldOrderCnt[0];
    avcPicParams->CurrFieldOrderCnt[1]     = statusDriver->PicInfo.iCurrFieldOrderCnt[1];
    avcPicParams->pic_width_in_mbs_minus1  = statusDriver->SeqParams.sps_disp.pic_width_in_mbs_minus1;
    avcPicParams->pic_height_in_mbs_minus1 = mbHeight - 1;
    avcPicParams->bit_depth_luma_minus8    = statusDriver->SeqParams.bit_depth_luma_minus8;
    avcPicParams->bit_depth_chroma_minus8  = statusDriver->SeqParams.bit_depth_chroma_minus8;
    avcPicParams->num_ref_frames           = statusDriver->SeqParams.num_ref_frames;
    // Seq fields
    avcPicParams->seq_fields.chroma_format_idc                 = statusDriver->SeqParams.sps_disp.chroma_format_idc;
    avcPicParams->seq_fields.residual_colour_transform_flag    = statusDriver->SeqParams.residual_colour_transform_flag;
    avcPicParams->seq_fields.frame_mbs_only_flag               = frame_mbs_only_flag;
    avcPicParams->seq_fields.mb_adaptive_frame_field_flag      = statusDriver->SeqParams.sps_disp.mb_adaptive_frame_field_flag;
    avcPicParams->seq_fields.direct_8x8_inference_flag         = statusDriver->SeqParams.sps_disp.direct_8x8_inference_flag;
    avcPicParams->seq_fields.log2_max_frame_num_minus4         = statusDriver->SeqParams.log2_max_frame_num_minus4;
    avcPicParams->seq_fields.pic_order_cnt_type                = statusDriver->SeqParams.pic_order_cnt_type;
    avcPicParams->seq_fields.log2_max_pic_order_cnt_lsb_minus4 = statusDriver->SeqParams.log2_max_pic_order_cnt_lsb_minus4;
    avcPicParams->seq_fields.delta_pic_order_always_zero_flag  = statusDriver->SeqParams.delta_pic_order_always_zero_flag;
    // Slice and QP
    avcPicParams->num_slice_groups_minus1 = statusDriver->PicParams.num_slice_groups_minus1;
    avcPicParams->slice_group_map_type    = statusDriver->PicParams.slice_group_map_type;
    // This is for FMO/ASO, currently not supported by HW, force to 0
    avcPicParams->slice_group_change_rate_minus1 = 0;
    avcPicParams->pic_init_qp_minus26            = (char)statusDriver->PicParams.pic_init_qp_minus26;
    avcPicParams->chroma_qp_index_offset         = (char)statusDriver->PicParams.chroma_qp_index_offset;
    avcPicParams->second_chroma_qp_index_offset  = (char)statusDriver->PicParams.second_chroma_qp_index_offset;
    // Pic fields
    avcPicParams->pic_fields.entropy_coding_mode_flag               = statusDriver->PicParams.entropy_coding_mode_flag;
    avcPicParams->pic_fields.weighted_pred_flag                     = statusDriver->PicParams.weighted_pred_flag;
    avcPicParams->pic_fields.weighted_bipred_idc                    = statusDriver->PicParams.weighted_bipred_idc;
    avcPicParams->pic_fields.transform_8x8_mode_flag                = statusDriver->PicParams.transform_8x8_mode_flag;
    avcPicParams->pic_fields.field_pic_flag                         = statusDriver->PicInfo.field_pic_flag != 3;
    avcPicParams->pic_fields.constrained_intra_pred_flag            = statusDriver->PicParams.constrained_intra_pred_flag;
    avcPicParams->pic_fields.pic_order_present_flag                 = statusDriver->PicParams.pic_order_present_flag;
    avcPicParams->pic_fields.deblocking_filter_control_present_flag = statusDriver->PicParams.deblocking_filter_control_present_flag;
    avcPicParams->pic_fields.redundant_pic_cnt_present_flag         = statusDriver->PicParams.redundant_pic_cnt_present_flag;
    avcPicParams->pic_fields.reference_pic_flag                     = statusDriver->PicInfo.nal_ref_idc != 0;
    avcPicParams->pic_fields.IntraPicFlag =
        (statusDriver->PicInfo.slice_type == 2 || statusDriver->PicInfo.slice_type == 4);  // intra pred for I-frame only
    avcPicParams->num_ref_idx_l0_active_minus1 = statusDriver->PicParams.num_ref_idx_l0_active -
                                                 (statusDriver->PicParams.num_ref_idx_l0_active > 0);
    avcPicParams->num_ref_idx_l1_active_minus1 = statusDriver->PicParams.num_ref_idx_l1_active -
                                                 (statusDriver->PicParams.num_ref_idx_l1_active > 0);
    avcPicParams->frame_num = (uint16_t)statusDriver->PicInfo.frame_num;

    return eStatus;
}
