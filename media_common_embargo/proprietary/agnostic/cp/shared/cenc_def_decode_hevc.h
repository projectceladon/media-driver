/*
// Copyright (C) 2018 Intel Corporation
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
//! \file      cenc_def_decode_hevc.h 
//! \brief     This modules declares structures, arrays and macros common for cenc decode hevc. 
//!

#ifndef __CENC_DEF_DECODE_HEVC_H__
#define __CENC_DEF_DECODE_HEVC_H__

//--------------------
// Start of H265/HEVC constants and data structs
//--------------------
#define     H265_MAX_NUM_SPS                16
#define     H265_MAX_NUM_PPS                64

#define     H265_MAX_NUM_TLAYER             8
#define     H265_MAX_NUM_SUB_LAYER          7
#define     H265_MAX_NUM_REF_PICS           16
#define     H265_MAX_NUM_LREF_PICS          33
#define     H265_MAX_NUM_REF_PICS_CUR       8

#define     H265_MAX_NUM_RPS_SETS           64

#define     H265_MAX_CPB_CNT                32
#define     H265_Max_TILE_COLS              20
#define     H265_Max_TILE_ROWS              22
#define     H265_MAX_SLICES                 200

#define     H265_SCALING_LIST_NUM           6
#define     H265_SCALING_LIST_NUM_32x32     2
#define     H265_SCALING_LIST_SIZE_4x4      16
#define     H265_SCALING_LIST_SIZE_8x8      64
#define     H265_SCALING_LIST_SIZE_16x16    64
#define     H265_SCALING_LIST_SIZE_32x32    64
#define     H265_SCALING_LIST_START_VALUE   8

#define     H265_INVALID_SPS                128
#define     H265_INVALID_PPS                128

typedef enum _h265_scaling_list_size
{
    H265_SCALING_LIST_4x4 = 0,
    H265_SCALING_LIST_8x8,
    H265_SCALING_LIST_16x16,
    H265_SCALING_LIST_32x32,
    H265_SCALING_LIST_SIZE_NUM
} h265_scaling_list_size;

typedef struct
{
    uint32_t      bit_rate_value_minus1[H265_MAX_CPB_CNT];
    uint32_t      cpb_size_value_minus1[H265_MAX_CPB_CNT];
    uint32_t      cpb_size_du_value_minus1[H265_MAX_CPB_CNT];
    uint32_t      bit_rate_du_value_minus1[H265_MAX_CPB_CNT];
    uint8_t       cbr_flag[H265_MAX_CPB_CNT];
} h265_hrd_sublayer_info;

typedef struct
{
    uint8_t       nal_hrd_parameters_present_flag;
    uint8_t       vcl_hrd_parameters_present_flag;
    uint8_t       hrd_sub_pic_cpb_params_present_flag;
    uint8_t       hrd_tick_divisor_minus2;

    uint8_t       hrd_du_cpb_removal_delay_increment_length_minus1;
    uint8_t       hrd_sub_pic_cpb_params_in_pic_timing_sei_flag;
    uint8_t       hrd_dpb_output_delay_du_length_minus1;
    uint8_t       hrd_bit_rate_scale;

    uint8_t       hrd_cpb_size_scale;
    uint8_t       hrd_cpb_size_du_scale;
    uint8_t       hrd_initial_cpb_removal_delay_length_minus1;
    uint8_t       hrd_au_cpb_removal_delay_length_minus1;

    uint8_t       hrd_dpb_output_delay_length_minus1;
    uint8_t       hrd_fixed_pic_rate_general_flag[H265_MAX_NUM_SUB_LAYER];
    uint8_t       hrd_fixed_pic_rate_within_cvs_flag[H265_MAX_NUM_SUB_LAYER];
    uint8_t       hrd_elemental_duration_in_tc_minus1[H265_MAX_NUM_SUB_LAYER];
    uint8_t       hrd_low_delay_hrd_flag[H265_MAX_NUM_SUB_LAYER];
    uint8_t       hrd_cpb_cnt_minus1[H265_MAX_NUM_SUB_LAYER];

    h265_hrd_sublayer_info hrd_sublayer_nal[H265_MAX_NUM_SUB_LAYER];
    h265_hrd_sublayer_info hrd_sublayer_vcl[H265_MAX_NUM_SUB_LAYER];
} h265_hrd_parameters;

typedef struct
{
    uint8_t       vui_aspect_ratio_info_present_flag;
    uint8_t       vui_aspect_ratio_idc;
    uint8_t       vui_overscan_info_present_flag;
    uint8_t       vui_overscan_appropriate_flag;

    uint16_t      vui_sar_width;
    uint16_t      vui_sar_height;

    uint8_t       vui_video_signal_type_present_flag;
    uint8_t       vui_video_format;
    uint8_t       vui_video_full_range_flag;
    uint8_t       vui_colour_description_present_flag;

    uint8_t       vui_colour_primaries;
    uint8_t       vui_transfer_characteristics;
    uint8_t       vui_matrix_coefficients;
    uint8_t       vui_chroma_loc_info_present_flag;

    uint32_t      vui_chroma_sample_loc_type_top_field;
    uint32_t      vui_chroma_sample_loc_type_bottom_field;

    uint8_t       vui_neutral_chroma_indication_flag;
    uint8_t       vui_field_seq_flag;
    uint8_t       vui_frame_field_info_present_flag;
    uint8_t       vui_default_display_window_flag;

    uint32_t      vui_def_disp_win_left_offset;
    uint32_t      vui_def_disp_win_right_offset;
    uint32_t      vui_def_disp_win_top_offset;
    uint32_t      vui_def_disp_win_bottom_offset;

    uint8_t       vui_timing_info_present_flag;
    uint8_t       vui_poc_proportional_to_timing_flag;
    uint8_t       vui_hrd_parameters_present_flag;
    uint8_t       vui_bitstream_restriction_flag;

    uint32_t      vui_num_units_in_tick;
    uint32_t      vui_time_scale;
    uint32_t      vui_num_ticks_poc_diff_one_minus1;

    uint8_t       vui_tiles_fixed_structure_flag;
    uint8_t       vui_motion_vectors_over_pic_boundaries_flag;
    uint8_t       vui_restricted_ref_pic_lists_flag;
    uint8_t       vui_padding_0;

    uint32_t      vui_min_spatial_segmentation_idc;
    uint32_t      vui_max_bytes_per_pic_denom;
    uint32_t      vui_max_bits_per_mincu_denom;
    uint32_t      vui_log2_max_mv_length_horizontal;
    uint32_t      vui_log2_max_mv_length_vertical;

    h265_hrd_parameters  vui_hrd_parameters;
} h265_vui_parameters;

typedef struct
{
    uint8_t       rps_inter_ref_pic_set_prediction_flag;
    uint8_t       rps_delta_rps_sign;
    uint8_t       rps_delta_idx_minus1;
    uint8_t       rps_num_total_pics;

    uint8_t       rps_num_negative_pics;
    uint8_t       rps_num_positive_pics;
    uint16_t      rps_abs_delta_rps_minus1;

    uint8_t       rps_used_by_curr_pic_flag[H265_MAX_NUM_REF_PICS];
    uint8_t       rps_use_delta_flag[H265_MAX_NUM_REF_PICS];

    int32_t       rps_delta_poc[H265_MAX_NUM_REF_PICS];
    //int32_t     rps_cur_poc[H265_MAX_NUM_REF_PICS];
    uint8_t       rps_is_used[H265_MAX_NUM_REF_PICS];

    //uint8_t     rps_refidc[H265_MAX_NUM_REF_PICS + 3];
    uint8_t       rps_num_refidc;

    //uint16_t    rps_delta_poc_s0_minus1[H265_MAX_NUM_REF_PICS];
    //uint16_t    rps_delta_poc_s1_minus1[H265_MAX_NUM_REF_PICS];

    //uint8_t     rps_used_by_curr_pic_s0_flag[H265_MAX_NUM_REF_PICS];
    //uint8_t     rps_used_by_curr_pic_s1_flag[H265_MAX_NUM_REF_PICS];

} h265_reference_picture_set;

typedef struct
{
    uint8_t       layer_profile_space;
    uint8_t       layer_tier_flag;
    uint8_t       layer_profile_idc;
    uint8_t       layer_level_idc;

    uint8_t       layer_progressive_source_flag;
    uint8_t       layer_interlaced_source_flag;
    uint8_t       layer_non_packed_constraint_flag;
    uint8_t       layer_frame_only_constraint_flag;

    uint8_t       layer_profile_compatibility_flag[32];
} h265_layer_profile_tier_level;

typedef struct
{
    h265_layer_profile_tier_level   general_ptl;
    h265_layer_profile_tier_level   sub_layer_ptl[H265_MAX_NUM_SUB_LAYER];
    uint8_t                         sub_layer_profile_present_flag[H265_MAX_NUM_SUB_LAYER];
    uint8_t                         sub_layer_level_present_flag[H265_MAX_NUM_SUB_LAYER];
} h265_profile_tier_level;

typedef struct
{
    uint8_t                       scaling_list_pred_mode_flag[H265_SCALING_LIST_SIZE_NUM][H265_SCALING_LIST_NUM];
    int32_t                       scaling_list_pred_matrix_id_delta[H265_SCALING_LIST_SIZE_NUM][H265_SCALING_LIST_NUM];

    uint8_t                       scaling_list_4x4[H265_SCALING_LIST_NUM][H265_SCALING_LIST_SIZE_4x4];
    uint8_t                       scaling_list_8x8[H265_SCALING_LIST_NUM][H265_SCALING_LIST_SIZE_8x8];
    uint8_t                       scaling_list_16x16[H265_SCALING_LIST_NUM][H265_SCALING_LIST_SIZE_16x16];
    uint8_t                       scaling_list_32x32[H265_SCALING_LIST_NUM_32x32][H265_SCALING_LIST_SIZE_32x32];

    uint8_t                       scaling_list_16x16_dc[H265_SCALING_LIST_NUM];
    uint8_t                       scaling_list_32x32_dc[H265_SCALING_LIST_NUM_32x32];
} h265_scaling_list;

typedef struct
{
    uint8_t                       ref_set_num_of_curr_before;
    uint8_t                       ref_set_num_of_curr_after;
    uint8_t                       ref_set_num_of_curr_lt;
    uint8_t                       ref_set_num_of_curr_total;

    uint8_t                       ref_set_num_of_foll_st;
    uint8_t                       ref_set_num_of_foll_lt;

    uint8_t                       ref_set_idx_curr_before[H265_MAX_NUM_REF_PICS_CUR];
    uint8_t                       ref_set_idx_curr_after[H265_MAX_NUM_REF_PICS_CUR];
    uint8_t                       ref_set_idx_curr_lt[H265_MAX_NUM_REF_PICS_CUR];

    uint8_t                       ref_set_deltapoc_msb_present_flag[H265_MAX_NUM_REF_PICS];
    int32_t                       ref_set_deltapoc_curr_before[H265_MAX_NUM_REF_PICS_CUR];
    int32_t                       ref_set_deltapoc_curr_after[H265_MAX_NUM_REF_PICS_CUR];
    int32_t                       ref_set_deltapoc_curr_lt[H265_MAX_NUM_REF_PICS_CUR];
    int32_t                       ref_set_deltapoc_curr_total[H265_MAX_NUM_REF_PICS_CUR];
    int32_t                       ref_set_deltapoc_foll_st[H265_MAX_NUM_REF_PICS];
    int32_t                       ref_set_deltapoc_foll_lt[H265_MAX_NUM_REF_PICS];
    uint8_t                       ref_set_is_lt_curr_total[H265_MAX_NUM_REF_PICS_CUR];

    uint8_t                       ref_list_idx_rps[2][H265_MAX_NUM_REF_PICS_CUR];
    uint8_t                       ref_list_idx[2][H265_MAX_NUM_REF_PICS];
    //int32_t                     ref_set_curr_poc[H265_MAX_NUM_REF_PICS_CUR];
} h265_ref_frames_t;

typedef struct
{
    uint8_t                       sei_bp_seq_parameter_set_id;
    uint8_t                       sei_bp_irap_cpb_params_present_flag;
    uint8_t                       sei_bp_concatenation_flag;
    //uint8_t                     sei_bp_concatenation_flag;

    uint32_t                      sei_bp_cpb_delay_offset;
    uint32_t                      sei_bp_dpb_delay_offset;
    uint32_t                      sei_bp_au_cpb_removal_delay_delta_minus1;

    uint32_t                      sei_bp_nal_initial_cpb_removal_delay[H265_MAX_CPB_CNT];
    uint32_t                      sei_bp_nal_initial_cpb_removal_offset[H265_MAX_CPB_CNT];
    uint32_t                      sei_bp_nal_initial_alt_cpb_removal_delay[H265_MAX_CPB_CNT];
    uint32_t                      sei_bp_nal_initial_alt_cpb_removal_offset[H265_MAX_CPB_CNT];

    uint32_t                      sei_bp_vcl_initial_cpb_removal_delay[H265_MAX_CPB_CNT];
    uint32_t                      sei_bp_vcl_initial_cpb_removal_offset[H265_MAX_CPB_CNT];
    uint32_t                      sei_bp_vcl_initial_alt_cpb_removal_delay[H265_MAX_CPB_CNT];
    uint32_t                      sei_bp_vcl_initial_alt_cpb_removal_offset[H265_MAX_CPB_CNT];
} h265_sei_buffering_period_t;

typedef struct
{
    uint8_t                       sei_pt_pic_struct;
    uint8_t                       sei_pt_source_scan_type;
    uint8_t                       sei_pt_duplicate_flag;
    uint8_t                       sei_pt_du_common_cpb_removal_delay_flag;
    //uint8_t                     sei_bp_concatenation_flag;

    uint32_t                      sei_pt_au_cpb_removal_delay_minus1;
    uint32_t                      sei_pt_pic_dpb_output_delay;
    uint32_t                      sei_pt_pic_dpb_output_du_delay;
    uint32_t                      sei_pt_num_decoding_units_minus1;

    uint32_t                      sei_pt_du_common_cpb_removal_delay_increment_minus1;
} h265_sei_pic_timing_t;

typedef struct
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
} h265_seq_param_set_t;

typedef struct
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
} h265_pic_param_set_t;

struct DecryptStatusHevc
{
    uint32_t                      status;         // MUST NOT BE MOVED as every codec assumes Status is first uint32_t in struct! Formatted as CODECHAL_STATUS.

    uint16_t                      statusReportFeebackNumber;
    uint16_t                      slicePicOrderCntLsb;

    uint8_t                       sliceType;
    uint8_t                       nalUnitType;
    uint8_t                       picOutputFlag;
    uint8_t                       noOutputOfPriorPicsFlag;

    uint8_t                       nuhTemporalId;
    uint8_t                       hasEOS;
    uint16_t                      padding;

    h265_seq_param_set_t          SeqParams;
    h265_pic_param_set_t          PicParams;
    h265_ref_frames_t             RefFrames;
    h265_sei_buffering_period_t   SeiBufferingPeriod;
    h265_sei_pic_timing_t         SeiPicTiming;
};

#endif  // __CENC_DEF_DECODE_HEVC_H__