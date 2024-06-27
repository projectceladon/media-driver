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
//! \file      codechal_cenc_decode_avc_next.h 
//! \brief     Impelements the public interface for CodecHal CENC Decode 
//!
#ifndef __CODECHAL_CENC_DECODE_AVC_NEXT_H__
#define __CODECHAL_CENC_DECODE_AVC_NEXT_H__

#include "codechal_common.h"
#include "codechal_cenc_decode_next.h"
#include "cenc_def_decode_avc.h"

typedef enum
{
    AVC_MAX_FRAME_SIZE_IDC1_TO_1b = 99,
    AVC_MAX_FRAME_SIZE_IDC11_TO_2 = 396,
    AVC_MAX_FRAME_SIZE_IDC21 = 792,
    AVC_MAX_FRAME_SIZE_IDC22_TO_3 = 1620,
    AVC_MAX_FRAME_SIZE_IDC31 = 3600,
    AVC_MAX_FRAME_SIZE_IDC32 = 5120,
    AVC_MAX_FRAME_SIZE_IDC4_TO_41 = 8192,
    AVC_MAX_FRAME_SIZE_IDC42 = 8704,
    AVC_MAX_FRAME_SIZE_IDC5 = 22080,
    AVC_MAX_FRAME_SIZE_IDC51 = 36864
} AVC_MAX_FRAME_SIZE_IDC;


//!
//! \def CODECHAL_DECODE_MAX_SLICE_HDR_SET_SIZE
//! max slice header set size
//!
#define CODECHAL_DECODE_MAX_SLICE_HDR_SET_SIZE            256

//!
//! \def CODECHAL_DECODE_MAX_SLICE_HDR_LEN_SIZE
//! max slice header length size
//!
#define CODECHAL_DECODE_MAX_SLICE_HDR_LEN_SIZE            2

//!
//! \def CODECHAL_DECODE_MAX_SLICE_HDR_SET_BUF_SIZE
//! max slice header buf size
//!
#define CODECHAL_DECODE_MAX_SLICE_HDR_SET_BUF_SIZE        CODECHAL_DECODE_MAX_SLICE_HDR_SET_SIZE - CODECHAL_DECODE_MAX_SLICE_HDR_LEN_SIZE

//--------------------
// shared buffer between HuC kernel and Driver
//--------------------

typedef struct MOS_ALIGNED(MHW_CACHELINE_SIZE) _CODECHAL_DECRYPT_STATUS_REPORT_PIC_INFO_AVC
{
    uint16_t                      ui16StatusReportFeedbackNumber;
    uint8_t                       nal_ref_idc;
    uint8_t                       field_pic_flag;     // 1 top field, 2 bottom field, 3  frame

    uint32_t                      frame_num;
    uint32_t                      slice_type;         // 2&4 I, 0&3 P, 1 B
    uint32_t                      idr_flag;
    uint32_t                      idr_pic_id;

    int32_t                       pic_order_cnt_lsb;
    int32_t                       delta_pic_order_cnt_bottom;
    int32_t                       delta_pic_order_cnt[2];

    int32_t                       iCurrFieldOrderCnt[2];
} CODECHAL_DECRYPT_STATUS_REPORT_PIC_INFO_AVC, *PCODECHAL_DECRYPT_STATUS_REPORT_PIC_INFO_AVC;

typedef struct MOS_ALIGNED(MHW_CACHELINE_SIZE) _aligned_seq_param_set_t
{
    uint32_t                  is_updated;

    //Required for display section
    seq_param_set_disp_t      sps_disp;

    int32_t                   expectedDeltaPerPOCCycle;
    int32_t                   offset_for_non_ref_pic;                         // se(v), -2^31 to (2^31)-1, 32-bit integer
    int32_t                   offset_for_top_to_bottom_field;                 // se(v), -2^31 to (2^31)-1, 32-bit integer

                                                                              // IDC
    uint8_t                   profile_idc;                                    // u(8), 0x77 for MP
    uint8_t                   constraint_set_flags;                           // bit 0 to 3 for set0 to set3
    uint8_t                   level_idc;                                      // u(8)
    uint8_t                   seq_parameter_set_id;                           // ue(v), 0 to 31

    uint8_t                   pic_order_cnt_type;                             // ue(v), 0 to 2
    uint8_t                   log2_max_frame_num_minus4;                      // ue(v), 0 to 12
    uint8_t                   log2_max_pic_order_cnt_lsb_minus4;              // ue(v), 0 to 12
    uint8_t                   num_ref_frames_in_pic_order_cnt_cycle;          // ue(v), 0 to 255

    int32_t                   offset_for_ref_frame[MAX_NUM_REF_FRAMES_IN_PIC_ORDER_CNT_CYCLE];   // se(v), -2^31 to (2^31)-1, 32-bit integer
    uint8_t                   num_ref_frames;                                 // ue(v), 0 to 16,
    uint8_t                   gaps_in_frame_num_value_allowed_flag;           // u(1)
                                                                              // This is my addition, we should calculate this once and leave it with the sps
                                                                              // as opposed to calculating it each time in h264_hdr_decoding_POC()
    uint8_t                   delta_pic_order_always_zero_flag;               // u(1)
    uint8_t                   residual_colour_transform_flag;

    uint8_t                   bit_depth_luma_minus8;
    uint8_t                   bit_depth_chroma_minus8;
    uint8_t                   lossless_qpprime_y_zero_flag;
    uint8_t                   seq_scaling_matrix_present_flag;

    uint8_t                   seq_scaling_list_present_flag[MAX_PIC_LIST_NUM];   //0-7

                                                                                 // Combine the scaling matrix to word ( 24 + 32)
    uint8_t                   ScalingList4x4[6][16];
    uint8_t                   ScalingList8x8[2][64];
    uint8_t                   UseDefaultScalingMatrix4x4Flag[6];
    uint8_t                   UseDefaultScalingMatrix8x8Flag[6];
} aligned_seq_param_set_t;

// picture parameter set
typedef struct MOS_ALIGNED(MHW_CACHELINE_SIZE) _aligned_pic_param_set_t
{
    int32_t            pic_init_qp_minus26;                            // se(v), -26 to +25
    int32_t            pic_init_qs_minus26;                            // se(v), -26 to +25
    int32_t            chroma_qp_index_offset;                         // se(v), -12 to +12
    int32_t            second_chroma_qp_index_offset;

    uint32_t           pic_parameter_set_id;                           // ue(v), 0 to 255, restricted to 0 to 127 by MPD_CTRL_MAXPPS = 128

    uint8_t            _reserved_;
    uint8_t            seq_parameter_set_id;                           // ue(v), 0 to 31
    uint8_t            entropy_coding_mode_flag;                       // u(1)
    uint8_t            pic_order_present_flag;                         // u(1)

    uint8_t            num_slice_groups_minus1;                        // ue(v), shall be 0 for MP
                                                                       // Below are not relevant for main profile...
    uint8_t            slice_group_map_type;                           // ue(v), 0 to 6
    uint8_t            num_ref_idx_l0_active;                          // ue(v), 0 to 31
    uint8_t            num_ref_idx_l1_active;                          // ue(v), 0 to 31

    uint8_t            weighted_pred_flag;                             // u(1)
    uint8_t            weighted_bipred_idc;                            // u(2)
    uint8_t            deblocking_filter_control_present_flag;         // u(1)
    uint8_t            constrained_intra_pred_flag;                    // u(1)

    uint8_t            redundant_pic_cnt_present_flag;                 // u(1)
    uint8_t            transform_8x8_mode_flag;
    uint8_t            pic_scaling_matrix_present_flag;
    uint8_t            pps_status_flag;

    //Keep here with 32-bits aligned
    uint8_t            pic_scaling_list_present_flag[8];

    qm_matrix_set      pps_qm;

    uint8_t            ScalingList4x4[6][16];
    uint8_t            ScalingList8x8[2][64];
    uint8_t            UseDefaultScalingMatrix4x4Flag[6 + 2];
    uint8_t            UseDefaultScalingMatrix8x8Flag[6 + 2];
} aligned_pic_param_set_t;

//Buffer for one slice header data. uiSliceHeader[1] is the place holder for the variable length buffer
typedef struct _CODECHAL_DECRYPT_SLICE_HEADER_AVC
{
    uint16_t ui16SliceHeaderLen; // indicate the length of the following slice header in byte, it shall not be more than 1024
    uint8_t  ui8SliceHeader[1];  // slice header data, the last byte might contain some unused bits. They will be ignored
} CODECHAL_DECRYPT_SLICE_HEADER_AVC, *PCODECHAL_DECRYPT_SLICE_HEADER_AVC;

//Slice header set contains multiple slice header data
typedef struct MOS_ALIGNED(MHW_CACHELINE_SIZE)  _CODECHAL_DECRYPT_SLICE_HEADER_SET_AVC
{
    uint16_t     ui16NumHeaders;         // indicate the number of slice headers in the input sample
    uint8_t      ui8Data[CODECHAL_DECODE_MAX_SLICE_HDR_SET_BUF_SIZE]; // Slice header buffer
}CODECHAL_DECRYPT_SLICE_HEADER_SET_AVC, *PCODECHAL_DECRYPT_SLICE_HEADER_SET_AVC;

typedef h264_Dec_Ref_Pic_Marking_t  MOS_ALIGNED(MHW_CACHELINE_SIZE) aligned_h264_Dec_Ref_Pic_Marking_t;
typedef h264_SEI_buffering_period_t MOS_ALIGNED(MHW_CACHELINE_SIZE) aligned_h264_SEI_buffering_period_t;
typedef h264_SEI_pic_timing_t       MOS_ALIGNED(MHW_CACHELINE_SIZE) aligned_h264_SEI_pic_timing_t;
typedef h264_SEI_recovery_point_t   MOS_ALIGNED(MHW_CACHELINE_SIZE) aligned_h264_SEI_recovery_point_t;

typedef struct MOS_ALIGNED(MHW_CACHELINE_SIZE) _CODECHAL_DECRYPT_STATUS_REPORT_AVC
{
    CODECHAL_DECRYPT_STATUS_REPORT_PIC_INFO_AVC     PicInfo;

    aligned_seq_param_set_t                         SeqParams;
    aligned_pic_param_set_t                         PicParams;

    aligned_h264_Dec_Ref_Pic_Marking_t              RefPicMarking;

    aligned_h264_SEI_buffering_period_t             SeiBufferingPeriod;
    aligned_h264_SEI_pic_timing_t                   SeiPicTiming;
    aligned_h264_SEI_recovery_point_t               SeiRecoveryPoint;
    CODECHAL_DECRYPT_SLICE_HEADER_SET_AVC           SliceHeaderSet;
    uint8_t                                         ui8ReservedPerformance[512];
} CODECHAL_DECRYPT_STATUS_REPORT_AVC, *PCODECHAL_DECRYPT_STATUS_REPORT_AVC;

typedef struct MOS_ALIGNED(MHW_CACHELINE_SIZE) _CODECHAL_DECRYPT_POC_CALC_INFO
{
    // POC decoding
    int32_t     PicOrderCntMsb;
    int32_t     PrevPicOrderCntLsb;
    int32_t     PreviousFrameNum;
    int32_t     PreviousFrameNumOffset;
    int32_t     PrevPicPOC;
    uint32_t    last_has_mmco_5;
    uint32_t    cur_has_mmco_5;
    uint32_t    last_pic_bottom_field;

} CODECHAL_DECRYPT_POC_CALC_INFO, *PCODECHAL_DECRYPT_POC_CALC_INFO;

typedef struct MOS_ALIGNED(MHW_CACHELINE_SIZE) _CODECHAL_DECRYPT_ERROR_REPORT
{
    uint16_t    ui16ErrorReportFeedbackNumber;
    uint8_t     ui8ErrBufferFull;
}CODECHAL_DECRYPT_ERROR_REPORT, *PCODECHAL_DECRYPT_AVC_ERROR_REPORT;

typedef struct _CODECHAL_DECRYPT_SHARED_BUFFER_AVC
{
    CODECHAL_DECRYPT_STATUS_REPORT_AVC  StatusReport[8];

    // Standard specific parameters
    aligned_seq_param_set_t             SeqParams[CODEC_AVC_MAX_SPS_NUM];
    aligned_pic_param_set_t             PicParams[CODEC_AVC_MAX_PPS_NUM + 1];

    CODECHAL_DECRYPT_POC_CALC_INFO      PocCalcInfo;
    CODECHAL_DECRYPT_ERROR_REPORT       ErrorReport;

#ifdef CODECHAL_HUC_KERNEL_DEBUG
    CODECHAL_HUC_KERNEL_DEBUG_INFO      HucKernelDebugInfo[8];
#endif
} CODECHAL_DECRYPT_SHARED_BUFFER_AVC, *PCODECHAL_DECRYPT_SHARED_BUFFER_AVC;

class CodechalCencDecodeAvcNext : public CodechalCencDecodeNext
{
public:

    //!
    //! \brief  Constructor
    //! \param  [in] hwInterface
    //!         Hardware interface
    //!
    CodechalCencDecodeAvcNext();

    //!
    //! \brief  Destructor
    //!
    ~CodechalCencDecodeAvcNext();

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
    //! \brief  Parse Shared Buffer
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS ParseSharedBufParams(
        uint16_t                    bufIdx,
        uint16_t                    reportIdx,
        void                        *buffer);

    //!
    //! \brief  Initilize Shared Buffer
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS InitializeSharedBuf(bool initStatusReportOnly);

    //!
    //! \brief  Update Secure OptMode
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS UpdateSecureOptMode(PMHW_PAVP_ENCRYPTION_PARAMS params);

    //!
    //! \brief  Initilize Allocation
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS InitializeAllocation();

    MOS_STATUS SetDecodeParams(
        CodechalDecodeParams    *decodeParams,
        void                    *cpParam);

private:

    //!
    //! \brief  Get Max FS From LevelIdc
    //! \return int32_t
    //!
    int32_t GetMaxFSFromLevelIdc(
        int32_t     levelIdc);

    //!
    //! \brief  Get Max Supported Slices
    //! \return int32_t
    //!
    uint32_t GetMaxSupportedSlices(
        uint16_t    mbHeight);

    //!
    //! \brief  Get Max Bitstream
    //! \return int32_t
    //!
    uint32_t GetMaxBitstream(
        uint16_t mbBWidth,
        uint16_t mbHeight);

    //!
    //! \brief  Scaling List
    //! \return void
    //!
    void ScalingList(
        uint8_t   *inputScalingList,
        uint8_t   *scalingList,
        uint8_t   qmIdx);

    //!
    //! \brief  Scaling List FallbackA
    //! \return void
    //!
    void ScalingListFallbackA(
        PCODEC_AVC_IQ_MATRIX_PARAMS  avcQmParams,
        uint8_t                      qmIdx);

    //!
    //! \brief  Scaling List FallbackB
    //! \return void
    //!
    void ScalingListFallbackB(
        PCODEC_AVC_IQ_MATRIX_PARAMS     avcQmParams,
        uint8_t                         *inputScalingList,
        uint8_t                         imIdx);

    //!
    //! \brief  Verify Slice Header Set Params
    //! \return int32_t
    //!
    uint32_t VerifySliceHeaderSetParams(
        PCODECHAL_DECRYPT_SLICE_HEADER_SET_AVC     sliceHeaderSetBuf);

    //!
    //! \brief  Decide if the current profile is REXT profile
    //! \return bool
    //!
    bool IsFRextProfile(uint8_t profile);

    //!
    //! \brief  Pack scaling list
    //! \return MOS_STATUS
    //!
    MOS_STATUS PackScalingList(
        uint8_t  *scalingList,
        int      sizeOfScalingList,
        uint32_t bufSize,
        uint8_t  *accumulator,
        uint8_t  *accumulatorValidBits,
        uint8_t  *&outBitStreamBuf,
        uint32_t *byteWritten);

    //!
    //! \brief  Encode SPS
    //! \return MOS_STATUS
    //!
    MOS_STATUS EncodeSPS(
        const PCODECHAL_DECRYPT_STATUS_REPORT_AVC statusDriver,
        uint32_t bufSize,
        uint8_t  *&outBitStreamBuf,
        uint32_t &writtenBytes);

    //!
    //! \brief  Encode PPS
    //! \return MOS_STATUS
    //!
    MOS_STATUS EncodePPS(
        const PCODECHAL_DECRYPT_STATUS_REPORT_AVC statusDriver,
        uint32_t bufSize,
        uint8_t  *&outBitStreamBuf,
        uint32_t &writtenBytes);

    //!
    //! \brief  Encode buffering period SEI
    //! \return MOS_STATUS
    //!
    MOS_STATUS EncodeSeiBufferingPeriod(
        const PCODECHAL_DECRYPT_STATUS_REPORT_AVC statusDriver, 
        PCODECHAL_CENC_SEI                        seiBufPeriod);

    //!
    //! \brief  Encode pic timing SEI
    //! \return MOS_STATUS
    //!
    MOS_STATUS EncodeSeiPicTiming(
        const PCODECHAL_DECRYPT_STATUS_REPORT_AVC statusDriver,
        PCODECHAL_CENC_SEI                        seiPicTiming);

    //!
    //! \brief  Encode recovery point SEI
    //! \return MOS_STATUS
    //!
    MOS_STATUS EncodeSeiRecoveryPoint(
        const PCODECHAL_DECRYPT_STATUS_REPORT_AVC statusDriver,
        PCODECHAL_CENC_SEI                        seiRecPoint);

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
        const PCODECHAL_DECRYPT_STATUS_REPORT_AVC statusDriver,
        uint32_t bufSize,
        uint8_t  *&outBitStreamBuf,
        uint32_t &writtenBytes);

MEDIA_CLASS_DEFINE_END(CodechalCencDecodeAvcNext)
};
#endif  // __CODECHAL_CENC_DECODE_AVC_NEXT_H__