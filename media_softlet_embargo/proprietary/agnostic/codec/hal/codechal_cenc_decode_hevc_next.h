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
//! \file      codechal_cenc_decode_hevc_next.h 
//! \brief     Impelements the public interface for CodecHal CENC Decode 
//!
#ifndef __CODECHAL_CENC_DECODE_HEVC_NEXT_H__
#define __CODECHAL_CENC_DECODE_HEVC_NEXT_H__

#include "codechal_common.h"
#include "codechal_cenc_decode_next.h"
#include "cenc_def_decode_hevc.h"

//--------------------
// for HuC DRM usage
//--------------------

typedef struct  MOS_ALIGNED(MHW_CACHELINE_SIZE) _aligned_h265_seq_param_set_t
{
    uint8_t                       sps_video_parameter_set_id;
    uint8_t                       sps_max_sub_layers_minus1;
    uint8_t                       sps_temporal_id_nesting_flag;
    uint8_t                       sps_separate_colour_plane_flag;

    h265_profile_tier_level       sps_profile_tier_level;

    uint8_t                       sps_seq_parameter_set_id;
    uint8_t                       sps_chroma_format_idc;
    uint32_t                      sps_pic_width_in_luma_samples;
    uint32_t                      sps_pic_height_in_luma_samples;

    uint8_t                       sps_conformance_window_flag;
    uint8_t                       sps_sub_layer_ordering_info_present_flag;
    uint8_t                       sps_scaling_list_enabled_flag;
    uint8_t                       sps_scaling_list_data_present_flag;

    uint32_t                      sps_conf_win_left_offset;
    uint32_t                      sps_conf_win_right_offset;
    uint32_t                      sps_conf_win_top_offset;
    uint32_t                      sps_conf_win_bottom_offset;

    uint8_t                       sps_bit_depth_luma_minus8;
    uint8_t                       sps_bit_depth_chroma_minus8;
    uint8_t                       sps_log2_max_pic_order_cnt_lsb_minus4;

    uint32_t                      sps_max_dec_pic_buffering_minus1[H265_MAX_NUM_TLAYER];
    uint32_t                      sps_max_num_reorder_pics[H265_MAX_NUM_TLAYER];
    uint32_t                      sps_max_latency_increase[H265_MAX_NUM_TLAYER];

    uint32_t                      sps_log2_min_luma_coding_block_size_minus3;
    uint32_t                      sps_log2_diff_max_min_luma_coding_block_size;
    uint32_t                      sps_log2_min_transform_block_size_minus2;
    uint32_t                      sps_log2_diff_max_min_transform_block_size;

    uint32_t                      sps_max_transform_hierarchy_depth_inter;
    uint32_t                      sps_max_transform_hierarchy_depth_intra;

    uint8_t                       sps_amp_enabled_flag;
    uint8_t                       sps_sample_adaptive_offset_enabled_flag;
    uint8_t                       sps_long_term_ref_pics_present_flag;
    uint8_t                       sps_temporal_mvp_enabled_flag;

    uint8_t                       sps_pcm_enabled_flag;
    uint8_t                       sps_pcm_sample_bit_depth_luma_minus1;
    uint8_t                       sps_pcm_sample_bit_depth_chroma_minus1;
    uint8_t                       sps_pcm_loop_filter_disable_flag;
    uint32_t                      sps_log2_min_pcm_luma_coding_block_size_minus3;
    uint32_t                      sps_log2_diff_max_min_pcm_luma_coding_block_size;

    uint8_t                       sps_num_short_term_ref_pic_sets;
    uint8_t                       sps_num_long_term_ref_pics_sps;

    h265_reference_picture_set    sps_short_term_ref_pic_set[H265_MAX_NUM_RPS_SETS];

    h265_scaling_list             sps_scaling_list;

    uint8_t                       sps_strong_intra_smoothing_enabled_flag;
    uint8_t                       sps_vui_parameters_present_flag;
    uint8_t                       sps_extension_flag;
    uint8_t                       sps_extension_data_flag;

    uint16_t                      sps_lt_ref_pic_poc_lsb_sps[H265_MAX_NUM_LREF_PICS];
    uint8_t                       sps_used_by_curr_pic_lt_sps_flag[H265_MAX_NUM_LREF_PICS];

    h265_vui_parameters           sps_vui;
} aligned_h265_seq_param_set_t;

typedef struct  MOS_ALIGNED(MHW_CACHELINE_SIZE) _aligned_h265_pic_param_set_t
{
    uint8_t                       pps_pic_parameter_set_id;
    uint8_t                       pps_seq_parameter_set_id;
    uint8_t                       pps_dependent_slice_segments_enabled_flag;
    uint8_t                       pps_output_flag_present_flag;

    uint8_t                       pps_num_extra_slice_header_bits;
    uint8_t                       pps_sign_data_hiding_flag;
    uint8_t                       pps_cabac_init_present_flag;
    uint8_t                       pps_num_ref_idx_l0_default_active_minus1;

    uint8_t                       pps_num_ref_idx_l1_default_active_minus1;
    uint8_t                       pps_constrained_intra_pred_flag;
    uint8_t                       pps_transform_skip_enabled_flag;
    uint8_t                       pps_cu_qp_delta_enabled_flag;

    int32_t                       pps_init_qp_minus26;
    uint32_t                      pps_diff_cu_qp_delta_depth;
    int32_t                       pps_cb_qp_offset;
    int32_t                       pps_cr_qp_offset;

    uint8_t                       pps_slice_chroma_qp_offsets_present_flag;
    uint8_t                       pps_weighted_pred_flag;
    uint8_t                       pps_weighted_bipred_flag;
    uint8_t                       pps_transquant_bypass_enabled_flag;

    uint8_t                       pps_tiles_enabled_flag;
    uint8_t                       pps_entropy_coding_sync_enabled_flag;
    uint8_t                       pps_uniform_spacing_flag;
    uint8_t                       pps_loop_filter_across_tiles_enabled_flag;

    uint8_t                       pps_num_tile_columns_minus1;
    uint8_t                       pps_num_tile_rows_minus1;
    uint8_t                       pps_loop_filter_across_slices_enabled_flag;
    uint8_t                       pps_df_control_present_flag;

    uint8_t                       pps_deblocking_filter_override_enabled_flag;
    uint8_t                       pps_disable_deblocking_filter_flag;
    int8_t                        pps_beta_offset_div2;
    int8_t                        pps_tc_offset_div2;

    uint16_t                      pps_column_width_minus1[H265_Max_TILE_COLS - 1];
    uint16_t                      pps_row_height_minus1[H265_Max_TILE_ROWS - 1];

    uint8_t                       pps_scaling_list_data_present_flag;
    uint8_t                       pps_lists_modification_present_flag;
    uint8_t                       pps_slice_segment_header_extension_present_flag;
    uint8_t                       pps_extension_flag;

    uint32_t                      pps_log2_parallel_merge_level_minus2;

    h265_scaling_list             pps_scaling_list;
} aligned_h265_pic_param_set_t;

typedef h265_ref_frames_t MOS_ALIGNED(MHW_CACHELINE_SIZE)              aligned_h265_ref_frames_t;
typedef h265_sei_buffering_period_t MOS_ALIGNED(MHW_CACHELINE_SIZE)    aligned_h265_sei_buffering_period_t;
typedef h265_sei_pic_timing_t MOS_ALIGNED(MHW_CACHELINE_SIZE)          aligned_h265_sei_pic_timing_t;

typedef struct MOS_ALIGNED(MHW_CACHELINE_SIZE) _CODECHAL_HUC_HEVC_PIC_INFO
{
    uint16_t                      ui16StatusReportFeebackNumber;
    uint16_t                      ui16SlicePicOrderCntLsb;

    uint8_t                       ui8SliceType;
    uint8_t                       ui8NalUnitType;
    uint8_t                       ui8PicOutputFlag;
    uint8_t                       ui8NoOutputOfPriorPicsFlag;

    uint8_t                       ui8NuhTemporalId;
    uint8_t                       ui8HasEOS;
    uint16_t                      ui16Padding;

    int32_t                       iCurrPicOrderCntVal;
    uint32_t                      uiLastSliceEndPos;

    uint8_t                       ui8SeparateColourPlaneFlag;
    uint8_t                       ui8ColourPlaneId;
} CODECHAL_HUC_HEVC_PIC_INFO, *PCODECHAL_HUC_HEVC_PIC_INFO;

typedef struct MOS_ALIGNED(MHW_CACHELINE_SIZE) _HUC_POC_HEVC_INFO
{
    int32_t                       i32PrevPoc;
    uint32_t                      uiValidRefFrameNumberPlusOne;
    int32_t                       iRefSetPoc[H265_MAX_NUM_REF_PICS + 1];
} HUC_POC_HEVC_INFO, *PHUC_POC_HEVC_INFO;

// HEVC decrypt status report
typedef struct MOS_ALIGNED(MHW_CACHELINE_SIZE) _CODECHAL_DECRYPT_STATUS_REPORT_HEVC
{
    CODECHAL_HUC_HEVC_PIC_INFO          PicInfo;

    aligned_h265_seq_param_set_t        SeqParams;
    aligned_h265_pic_param_set_t        PicParams;
    aligned_h265_ref_frames_t           RefFrames;
    aligned_h265_sei_buffering_period_t SeiBufferingPeriod;
    aligned_h265_sei_pic_timing_t       SeiPicTiming;
} CODECHAL_DECRYPT_STATUS_REPORT_HEVC, *PCODECHAL_DECRYPT_STATUS_REPORT_HEVC;

// shared buffer between HuC kernel and Driver
typedef struct MOS_ALIGNED(MHW_CACHELINE_SIZE) _CODECHAL_DECRYPT_SHARED_BUFFER_HEVC
{
    CODECHAL_DECRYPT_STATUS_REPORT_HEVC StatusReport[8];

    HUC_POC_HEVC_INFO                   PocInfo;
    aligned_h265_seq_param_set_t        SeqParams[H265_MAX_NUM_SPS];
    aligned_h265_pic_param_set_t        PicParams[H265_MAX_NUM_PPS];

#ifdef CODECHAL_HUC_KERNEL_DEBUG
    CODECHAL_HUC_KERNEL_DEBUG_INFO      HucKernelDebugInfo[8];
#endif
} CODECHAL_DECRYPT_SHARED_BUFFER_HEVC, *PCODECHAL_DECRYPT_SHARED_BUFFER_HEVC;

class CodechalCencDecodeHevcNext : public CodechalCencDecodeNext
{
public:

    //!
    //! \brief  Constructor
    //! \param  [in] hwInterface
    //!         Hardware interface
    //!
    CodechalCencDecodeHevcNext();

    //!
    //! \brief  Destructor
    //!
    ~CodechalCencDecodeHevcNext();

    //!
    //! \brief  Initilize
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS Initialize(
        PMOS_CONTEXT                osContext,
        CodechalSetting             *settings
    );

    //!
    //! \brief  AllocateDecodeResource
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS AllocateDecodeResource(
        uint8_t bufIdx);

    //!
    //! \brief  Initilize Allocation
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS InitializeAllocation();

    //!
    //! \brief  Initilize Shared Buffer
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS InitializeSharedBuf(bool initStatusReportOnly);

    //!
    //! \brief  Parse Shared Buffer
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS ParseSharedBufParams(
        uint16_t                   bufIdx,
        uint16_t                   reportIdx,
        void                       *buffer);

    //!
    //! \brief  Update Secure OptMode
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS UpdateSecureOptMode(PMHW_PAVP_ENCRYPTION_PARAMS params);

    MOS_STATUS SetDecodeParams(
        CodechalDecodeParams    *decodeParams,
        void                    *cpParam);

private:

    //!
    //! \brief  Scaling List Sps Pps
    //! \return void
    //!
    void ScalingListSpsPps(
        PCODECHAL_DECRYPT_STATUS_REPORT_HEVC    satusDriver,
        PCODECHAL_HEVC_IQ_MATRIX_PARAMS         hevcQmParams);

    //!
    //! \brief  Scaling List Default
    //! \return void
    //!
    void ScalingListDefault(PCODECHAL_HEVC_IQ_MATRIX_PARAMS  hevcQmParams);

    //!
    //! \brief  Scaling List Flat
    //! \return void
    //!
    void ScalingListFlat(PCODECHAL_HEVC_IQ_MATRIX_PARAMS  hevcQmParams);

    //!
    //! \brief  Pack profile tier
    //! \return MOS_STATUS
    //!
    MOS_STATUS PackProfileTier(
        const h265_layer_profile_tier_level *profileTilerLevel,
        uint32_t bufSize,
        uint8_t  *accumulator,
        uint8_t  *accumulatorValidBits,
        uint8_t  *&outBitStreamBuf,
        uint32_t *byteWritten);

    //!
    //! \brief  Pack profile tier level
    //! \return MOS_STATUS
    //!
    MOS_STATUS PackPTL(
        const h265_profile_tier_level *profileTilerLevel,
        uint32_t maxNumSubLayersMinus1,
        uint32_t bufSize,
        uint8_t  *accumulator,
        uint8_t  *accumulatorValidBits,
        uint8_t  *&outBitStreamBuf,
        uint32_t *byteWritten);

    //!
    //! \brief  Pack scaling matrix
    //! \return MOS_STATUS
    //!
    MOS_STATUS PackScalingMatrix(
        uint8_t       sizeNum,
        uint8_t       matrixSize,
        const uint8_t *matrix,
        const uint8_t *dc,
        uint32_t      bufSize,
        uint8_t       *accumulator,
        uint8_t       *accumulatorValidBits,
        uint8_t       *&outBitStreamBuf,
        uint32_t      *byteWritten);

    //!
    //! \brief  Pack scaling list
    //! \return MOS_STATUS
    //!
    MOS_STATUS PackScalingList(
        const h265_scaling_list *scalingList,
        uint32_t                bufSize,
        uint8_t                 *accumulator,
        uint8_t                 *accumulatorValidBits,
        uint8_t                 *&outBitStreamBuf,
        uint32_t                *byteWritten);

    //!
    //! \brief  Pack short term ref pic set
    //! \return MOS_STATUS
    //!
    MOS_STATUS PackShortTermRefPicSet(
        const h265_reference_picture_set *rps,
        uint32_t                         index,
        uint32_t                         numOfRps,
        uint32_t                         bufSize,
        uint8_t                          *accumulator,
        uint8_t                          *accumulatorValidBits,
        uint8_t                          *&outBitStreamBuf,
        uint32_t                         *byteWritten);

    //!
    //! \brief  Pack HRD param
    //! \return MOS_STATUS
    //!
    MOS_STATUS PackHrdParameters(
        const h265_hrd_parameters *hrd,
        uint32_t                  maxNumSubLayersMinus1,
        uint32_t                  bufSize,
        uint8_t                   *accumulator,
        uint8_t                   *accumulatorValidBits,
        uint8_t                   *&outBitStreamBuf,
        uint32_t                  *byteWritten);

    //!
    //! \brief  Pack VUI
    //! \return MOS_STATUS
    //!
    MOS_STATUS PackVUI(
        const h265_vui_parameters *vui,
        uint32_t                  maxNumSubLayersMinus1,
        uint32_t                  bufSize,
        uint8_t                   *accumulator,
        uint8_t                   *accumulatorValidBits,
        uint8_t                   *&outBitStreamBuf,
        uint32_t                  *byteWritten);

    //!
    //! \brief  Encode SPS
    //! \return MOS_STATUS
    //!
    MOS_STATUS EncodeSPS(
        const PCODECHAL_DECRYPT_STATUS_REPORT_HEVC statusDriver,
        uint32_t                                   bufSize,
        uint8_t                                    *&outBitStreamBuf,
        uint32_t                                   &writtenBytes);

    //!
    //! \brief  Encode PPS
    //! \return MOS_STATUS
    //!
    MOS_STATUS EncodePPS(
        const PCODECHAL_DECRYPT_STATUS_REPORT_HEVC statusDriver,
        uint32_t                                   bufSize,
        uint8_t                                    *&outBitStreamBuf,
        uint32_t                                   &writtenBytes);

    //!
    //! \brief  Encode buffering period SEI
    //! \return MOS_STATUS;
    //!
    MOS_STATUS EncodeSeiBufferingPeriod(
        const PCODECHAL_DECRYPT_STATUS_REPORT_HEVC statusDriver,
        PCODECHAL_CENC_SEI                         seiBufPeriod);

    //!
    //! \brief  Encode pic timing SEI
    //! \return MOS_STATUS
    //!
    MOS_STATUS EncodeSeiPicTiming(
        const PCODECHAL_DECRYPT_STATUS_REPORT_HEVC statusDriver, 
        PCODECHAL_CENC_SEI                         seiPicTiming);

    //!
    //! \brief  Write SEI bitstream to one buffer
    //! \return MOS_STATUS
    //!
    MOS_STATUS EncodeSeiWriteMessage(
        PCODECHAL_CENC_SEI sei,
        const uint8_t      *payload,
        const uint32_t     payloadSize,
        const uint8_t      payloadType);

    //!
    //! \brief  Encode SEI
    //! \return MOS_STATUS
    //!
    MOS_STATUS EncodeSEI(
        const PCODECHAL_DECRYPT_STATUS_REPORT_HEVC statusDriver,
        uint32_t                                   bufSize,
        uint8_t                                    *&outBitStreamBuf,
        uint32_t                                   &writtenBytes);

MEDIA_CLASS_DEFINE_END(CodechalCencDecodeHevcNext)
};

#endif  // __CODECHAL_CENC_DECODE_HEVC_NEXT_H__
