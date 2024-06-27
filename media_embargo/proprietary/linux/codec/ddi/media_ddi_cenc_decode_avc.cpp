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
//! \file     media_ddi_cenc_decode_avc.cpp
//! \brief    The class implementation of DdiDecodeAVC  for Avc decode
//!
#include <va/va.h>
#include "va_protected_content_private.h"
#include "codechal_cenc_decode_avc.h"

MOS_STATUS CodechalCencDecodeAvc::ParseSharedBufParams(
    uint16_t                    bufIdx,
    uint16_t                    reportIdx,
    void                        *buffer)
{
    MOS_STATUS                          eStatus = MOS_STATUS_SUCCESS;
    PCODECHAL_DECRYPT_SHARED_BUFFER_AVC pSharedBuf;
    PCODECHAL_DECRYPT_STATUS_REPORT_AVC pStatusDriver;
    VACencStatusBuf                     *pStatusApp;
    VACencSliceParameterBufferH264      *pSliceParam;
    PCODEC_AVC_PIC_PARAMS               pAvcPicParams;
    PCODEC_AVC_IQ_MATRIX_PARAMS         pAvcQmParams;
    uint8_t                             frame_mbs_only_flag;
    uint8_t                             ucIdx;
    size_t                              PPSCopySize;
    uint16_t                            usMBHeight;
    uint8_t                             *pBitStreamBuf = nullptr;
    uint32_t                            bytesWritten = 0;

    CENC_DECODE_FUNCTION_ENTER;

    CENC_DECODE_CHK_NULL(m_picParams);
    CENC_DECODE_CHK_NULL(m_qmParams);
    CENC_DECODE_CHK_NULL(buffer);

    CodechalResLock shareBuffer(m_osInterface, &m_resHucSharedBuf);
    pSharedBuf = (PCODECHAL_DECRYPT_SHARED_BUFFER_AVC)shareBuffer.Lock(CodechalResLock::readOnly);
    CENC_DECODE_CHK_NULL(pSharedBuf);

    pStatusDriver = (PCODECHAL_DECRYPT_STATUS_REPORT_AVC)&pSharedBuf->StatusReport[bufIdx % CODECHAL_NUM_DECRYPT_STATUS_BUFFERS];
    pStatusApp = (VACencStatusBuf*)buffer;
    pSliceParam = (VACencSliceParameterBufferH264*)pStatusApp->slice_buf;
    pAvcPicParams = (PCODEC_AVC_PIC_PARAMS)m_picParams[bufIdx];
    pAvcQmParams = (PCODEC_AVC_IQ_MATRIX_PARAMS)m_qmParams[bufIdx];
    CENC_DECODE_VERBOSEMESSAGE("Parse buffer %d.", bufIdx);

    if (pSliceParam == nullptr || pStatusApp->buf == nullptr)
    {
        CENC_DECODE_ASSERTMESSAGE("VACencStatusBuf's buf or slice_buf is NULL.");
        return MOS_STATUS_NULL_POINTER;
    }

    if (pStatusApp->slice_buf_type != VaCencSliceBufParamter || 
        pStatusApp->slice_buf_size != sizeof(VACencSliceParameterBufferH264))
    {
        CENC_DECODE_ASSERTMESSAGE("VACencStatusBuf's slice_buf_type or slice_buf_size is not correct.");
        return MOS_STATUS_INVALID_PARAMETER;
    }

    if (pStatusDriver->SeqParams.seq_parameter_set_id == CODEC_AVC_MAX_SPS_NUM + 1 &&
        pStatusDriver->PicParams.pic_parameter_set_id == CODEC_AVC_MAX_PPS_NUM + 1)
    {
        // No slices were parsed by HuC FW, so there will be no data in the status report
        pStatusApp->status = VA_ENCRYPTION_STATUS_ERROR;
        pStatusApp->status_report_index_feedback = pStatusDriver->PicInfo.ui16StatusReportFeedbackNumber;
        return eStatus;
    }

    frame_mbs_only_flag = pStatusDriver->SeqParams.sps_disp.frame_mbs_only_flag;
    usMBHeight = (pStatusDriver->SeqParams.sps_disp.pic_height_in_map_units_minus1 + 1) << uint16_t(frame_mbs_only_flag == 0);

    // calculate max number of slices according to profile level
    m_numSlices =
        GetMaxSupportedSlices(usMBHeight);
    m_picSize =
        GetMaxBitstream((pStatusDriver->SeqParams.sps_disp.pic_width_in_mbs_minus1 + 1), usMBHeight);
    CENC_DECODE_VERBOSEMESSAGE("level = %d, max slice = %d. max bitstream = %d", pStatusDriver->SeqParams.level_idc, m_numSlices, m_picSize);
    CENC_DECODE_VERBOSEMESSAGE("AVC decrypt status buffer size = %d.", sizeof(CODECHAL_DECRYPT_STATUS_REPORT_AVC));

    // allocate Decrypt state internal resources needed by Decode
    CENC_DECODE_CHK_STATUS(AllocateDecodeResource((uint8_t)bufIdx));

    CENC_DECODE_VERBOSEMESSAGE("ErrorReportFeedbackNumber = %d, ErrBufferFull = %d.",
        pSharedBuf->ErrorReport.ui16ErrorReportFeedbackNumber, pSharedBuf->ErrorReport.ui8ErrBufferFull);

    // populate params from share buffer into App Status Report buffer
    pStatusApp->status = VA_ENCRYPTION_STATUS_SUCCESSFUL;
    pStatusApp->status_report_index_feedback = pStatusDriver->PicInfo.ui16StatusReportFeedbackNumber;

    // uncompressed slice header
    pSliceParam->nal_ref_idc = pStatusDriver->PicInfo.nal_ref_idc;
    pSliceParam->idr_pic_flag = pStatusDriver->PicInfo.idr_flag;
    pSliceParam->slice_type = pStatusDriver->PicInfo.slice_type;
    pSliceParam->field_frame_flag = pStatusDriver->PicInfo.field_pic_flag;
    CENC_DECODE_COND_ASSERTMESSAGE((pSliceParam->field_frame_flag != 3), "ERROR - interlaced field is not supported by HuC-based DRM");
    pSliceParam->frame_number = pStatusDriver->PicInfo.frame_num;
    if (pStatusDriver->PicInfo.slice_type > SLICE_I)
    {
        CENC_DECODE_ASSERTMESSAGE("ERROR, not a valice slice type %d", pStatusDriver->PicInfo.slice_type);
    }

    pSliceParam->idr_pic_id= pStatusDriver->PicInfo.idr_pic_id;
    pSliceParam->pic_order_cnt_lsb = pStatusDriver->PicInfo.pic_order_cnt_lsb;
    pSliceParam->delta_pic_order_cnt_bottom = pStatusDriver->PicInfo.delta_pic_order_cnt_bottom;
    pSliceParam->delta_pic_order_cnt[0] = pStatusDriver->PicInfo.delta_pic_order_cnt[0];
    pSliceParam->delta_pic_order_cnt[1] = pStatusDriver->PicInfo.delta_pic_order_cnt[1];

    // Ref Pic Marking
    pSliceParam->ref_pic_fields.bits.no_output_of_prior_pics_flag = pStatusDriver->RefPicMarking.no_output_of_prior_pics_flag;
    pSliceParam->ref_pic_fields.bits.long_term_reference_flag = pStatusDriver->RefPicMarking.long_term_reference_flag;
    pSliceParam->ref_pic_fields.bits.adaptive_ref_pic_marking_mode_flag = pStatusDriver->RefPicMarking.adaptive_ref_pic_marking_mode_flag;
    pSliceParam->ref_pic_fields.bits.dec_ref_pic_marking_count = pStatusDriver->RefPicMarking.dec_ref_pic_marking_count;
    MOS_SecureMemcpy(pSliceParam->memory_management_control_operation,
        sizeof(pStatusDriver->RefPicMarking.memory_management_control_operation),
        pStatusDriver->RefPicMarking.memory_management_control_operation,
        sizeof(pStatusDriver->RefPicMarking.memory_management_control_operation));
    MOS_SecureMemcpy(pSliceParam->difference_of_pic_nums_minus1,
        sizeof(pStatusDriver->RefPicMarking.difference_of_pic_num_minus1),
        pStatusDriver->RefPicMarking.difference_of_pic_num_minus1,
        sizeof(pStatusDriver->RefPicMarking.difference_of_pic_num_minus1));
    MOS_SecureMemcpy(pSliceParam->long_term_pic_num,
        sizeof(pStatusDriver->RefPicMarking.long_term_pic_num),
        pStatusDriver->RefPicMarking.long_term_pic_num,
        sizeof(pStatusDriver->RefPicMarking.long_term_pic_num));

    for (ucIdx = 0; ucIdx < NUM_MMCO_OPERATIONS; ucIdx++)
    {
        pSliceParam->max_long_term_frame_idx_plus1[ucIdx] = pStatusDriver->RefPicMarking.max_long_term_frame_idx_plus1[ucIdx];
        pSliceParam->long_term_frame_idx[ucIdx] = pStatusDriver->RefPicMarking.long_term_frame_idx[ucIdx];
    }

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

    for (ucIdx = 0; ucIdx < 8; ucIdx++)
    {
        if (ucIdx < 6)
        {
            if (pStatusDriver->PicParams.pic_scaling_matrix_present_flag)
            {
                if (pStatusDriver->PicParams.pic_scaling_list_present_flag[ucIdx])
                {
                    if (pStatusDriver->PicParams.UseDefaultScalingMatrix4x4Flag[ucIdx])
                    {
                        ScalingListFallbackA(pAvcQmParams, ucIdx);

                    }
                    else
                    {
                        ScalingList(
                            &(pStatusDriver->PicParams.ScalingList4x4[ucIdx][0]),
                            &(pAvcQmParams->ScalingList4x4[ucIdx][0]),
                            ucIdx);
                    }
                }
                else if (pStatusDriver->SeqParams.seq_scaling_matrix_present_flag)
                {
                    // FallbackB if pic_scaling_list_present_flag[ucIdx] is to use sequence level scaling list
                    if (pStatusDriver->SeqParams.seq_scaling_list_present_flag[ucIdx])
                    {
                        if (pStatusDriver->SeqParams.UseDefaultScalingMatrix4x4Flag[ucIdx])
                        {

                            ScalingListFallbackA(pAvcQmParams, ucIdx);
                        }
                        else
                        {
                            ScalingListFallbackB(
                                pAvcQmParams,
                                &(pStatusDriver->SeqParams.ScalingList4x4[ucIdx][0]),
                                ucIdx);
                        }

                    }
                    else
                    {
                        ScalingListFallbackA(pAvcQmParams, ucIdx);
                    }
                }
                else
                {
                    ScalingListFallbackA(pAvcQmParams, ucIdx);
                }

            }
            else if (pStatusDriver->SeqParams.seq_scaling_matrix_present_flag)
            {
                if (pStatusDriver->SeqParams.seq_scaling_list_present_flag[ucIdx])
                {
                    if (pStatusDriver->SeqParams.UseDefaultScalingMatrix4x4Flag[ucIdx])
                    {
                        ScalingListFallbackA(pAvcQmParams, ucIdx);
                    }
                    else
                    {
                        ScalingList(
                            &(pStatusDriver->SeqParams.ScalingList4x4[ucIdx][0]),
                            &(pAvcQmParams->ScalingList4x4[ucIdx][0]),
                            ucIdx);
                    }
                }
                else
                {
                    ScalingListFallbackA(pAvcQmParams, ucIdx);
                }
            }
            else
            {
                ScalingListFallbackB(pAvcQmParams, nullptr, ucIdx);
            }
        }
        else
        {
            if (pStatusDriver->PicParams.pic_scaling_matrix_present_flag)
            {
                if (pStatusDriver->PicParams.pic_scaling_list_present_flag[ucIdx])
                {
                    if (pStatusDriver->PicParams.UseDefaultScalingMatrix8x8Flag[ucIdx - 6])
                    {
                        ScalingListFallbackA(pAvcQmParams, ucIdx);
                    }
                    else
                    {
                        ScalingList(
                            &(pStatusDriver->PicParams.ScalingList8x8[ucIdx - 6][0]),
                            &(pAvcQmParams->ScalingList8x8[ucIdx - 6][0]),
                            ucIdx);
                    }
                }
                else if (pStatusDriver->SeqParams.seq_scaling_matrix_present_flag)
                {
                    // FallbackB if pic_scaling_list_present_flag[ucIdx] is to use sequence level scaling list
                    if (pStatusDriver->SeqParams.seq_scaling_list_present_flag[ucIdx])
                    {
                        if (pStatusDriver->SeqParams.UseDefaultScalingMatrix8x8Flag[ucIdx - 6])
                        {
                            ScalingListFallbackA(pAvcQmParams, ucIdx);
                        }
                        else
                        {
                            ScalingListFallbackB(
                                pAvcQmParams,
                                &(pStatusDriver->SeqParams.ScalingList8x8[ucIdx - 6][0]),
                                ucIdx);
                        }
                    }
                    else
                    {
                        ScalingListFallbackA(pAvcQmParams, ucIdx);
                    }
                }
                else
                {
                    ScalingListFallbackA(pAvcQmParams, ucIdx);
                }

            }
            else if (pStatusDriver->SeqParams.seq_scaling_matrix_present_flag)
            {
                if (pStatusDriver->SeqParams.seq_scaling_list_present_flag[ucIdx])
                {
                    if (pStatusDriver->SeqParams.UseDefaultScalingMatrix8x8Flag[ucIdx - 6])
                    {
                        ScalingListFallbackA(pAvcQmParams, ucIdx);
                    }
                    else
                    {
                        ScalingList(
                            &(pStatusDriver->SeqParams.ScalingList8x8[ucIdx - 6][0]),
                            &(pAvcQmParams->ScalingList8x8[ucIdx - 6][0]),
                            ucIdx);
                    }
                }
                else
                {
                    ScalingListFallbackA(pAvcQmParams, ucIdx);
                }
            }
            else
            {
                ScalingListFallbackB(pAvcQmParams, nullptr, ucIdx);
            }
        }
    }

    CENC_DECODE_VERBOSEMESSAGE("Verifying Pic Param parsing.");

    // populdate params from what HuC returned to driver
    pAvcPicParams->CurrFieldOrderCnt[0] = pStatusDriver->PicInfo.iCurrFieldOrderCnt[0];
    pAvcPicParams->CurrFieldOrderCnt[1] = pStatusDriver->PicInfo.iCurrFieldOrderCnt[1];
    pAvcPicParams->pic_width_in_mbs_minus1 = pStatusDriver->SeqParams.sps_disp.pic_width_in_mbs_minus1;
    pAvcPicParams->pic_height_in_mbs_minus1 = usMBHeight - 1;
    pAvcPicParams->bit_depth_luma_minus8 = pStatusDriver->SeqParams.bit_depth_luma_minus8;
    pAvcPicParams->bit_depth_chroma_minus8 = pStatusDriver->SeqParams.bit_depth_chroma_minus8;
    pAvcPicParams->num_ref_frames = pStatusDriver->SeqParams.num_ref_frames;
    // Seq fields
    pAvcPicParams->seq_fields.chroma_format_idc = pStatusDriver->SeqParams.sps_disp.chroma_format_idc;
    pAvcPicParams->seq_fields.residual_colour_transform_flag = pStatusDriver->SeqParams.residual_colour_transform_flag;
    pAvcPicParams->seq_fields.frame_mbs_only_flag = frame_mbs_only_flag;
    pAvcPicParams->seq_fields.mb_adaptive_frame_field_flag = pStatusDriver->SeqParams.sps_disp.mb_adaptive_frame_field_flag;
    pAvcPicParams->seq_fields.direct_8x8_inference_flag = pStatusDriver->SeqParams.sps_disp.direct_8x8_inference_flag;
    pAvcPicParams->seq_fields.log2_max_frame_num_minus4 = pStatusDriver->SeqParams.log2_max_frame_num_minus4;
    pAvcPicParams->seq_fields.pic_order_cnt_type = pStatusDriver->SeqParams.pic_order_cnt_type;
    pAvcPicParams->seq_fields.log2_max_pic_order_cnt_lsb_minus4 = pStatusDriver->SeqParams.log2_max_pic_order_cnt_lsb_minus4;
    pAvcPicParams->seq_fields.delta_pic_order_always_zero_flag = pStatusDriver->SeqParams.delta_pic_order_always_zero_flag;
    // Slice and QP
    pAvcPicParams->num_slice_groups_minus1 = pStatusDriver->PicParams.num_slice_groups_minus1;
    pAvcPicParams->slice_group_map_type = pStatusDriver->PicParams.slice_group_map_type;
    // This is for FMO/ASO, currently not supported by HW, force to 0
    pAvcPicParams->slice_group_change_rate_minus1 = 0;
    pAvcPicParams->pic_init_qp_minus26 = (char)pStatusDriver->PicParams.pic_init_qp_minus26;
    pAvcPicParams->chroma_qp_index_offset = (char)pStatusDriver->PicParams.chroma_qp_index_offset;
    pAvcPicParams->second_chroma_qp_index_offset = (char)pStatusDriver->PicParams.second_chroma_qp_index_offset;
    // Pic fields
    pAvcPicParams->pic_fields.entropy_coding_mode_flag = pStatusDriver->PicParams.entropy_coding_mode_flag;
    pAvcPicParams->pic_fields.weighted_pred_flag = pStatusDriver->PicParams.weighted_pred_flag;
    pAvcPicParams->pic_fields.weighted_bipred_idc = pStatusDriver->PicParams.weighted_bipred_idc;
    pAvcPicParams->pic_fields.transform_8x8_mode_flag = pStatusDriver->PicParams.transform_8x8_mode_flag;
    pAvcPicParams->pic_fields.field_pic_flag = pStatusDriver->PicInfo.field_pic_flag != 3;
    pAvcPicParams->pic_fields.constrained_intra_pred_flag = pStatusDriver->PicParams.constrained_intra_pred_flag;
    pAvcPicParams->pic_fields.pic_order_present_flag = pStatusDriver->PicParams.pic_order_present_flag;
    pAvcPicParams->pic_fields.deblocking_filter_control_present_flag = pStatusDriver->PicParams.deblocking_filter_control_present_flag;
    pAvcPicParams->pic_fields.redundant_pic_cnt_present_flag = pStatusDriver->PicParams.redundant_pic_cnt_present_flag;
    pAvcPicParams->pic_fields.reference_pic_flag = pStatusDriver->PicInfo.nal_ref_idc != 0;
    pAvcPicParams->pic_fields.IntraPicFlag =
        (pStatusDriver->PicInfo.slice_type == 2 || pStatusDriver->PicInfo.slice_type == 4);  // intra pred for I-frame only
    pAvcPicParams->num_ref_idx_l0_active_minus1 = pStatusDriver->PicParams.num_ref_idx_l0_active -
        (pStatusDriver->PicParams.num_ref_idx_l0_active > 0);
    pAvcPicParams->num_ref_idx_l1_active_minus1 = pStatusDriver->PicParams.num_ref_idx_l1_active -
        (pStatusDriver->PicParams.num_ref_idx_l1_active > 0);
    pAvcPicParams->frame_num = (uint16_t)pStatusDriver->PicInfo.frame_num;

    return eStatus;
}

