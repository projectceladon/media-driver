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
//! \file     codechal_cenc_decode_vp9.h
//! \brief    Impelements the public interface for CodecHal CENC Decode 
//!
#ifndef __CODECHAL_CENC_DECODE_VP9_H__
#define __CODECHAL_CENC_DECODE_VP9_H__

#include "codechal.h"
#include "codechal_cenc_decode.h"
#include "cenc_def_decode_vp9.h"

typedef struct _CODECHAL_DECODE_VP9_PIC_INFO
{
    uint16_t                              ui16StatusReportFeedbackNumber;
    uint16_t                              picture_width_minus1;
    uint16_t                              picture_height_minus1;
    uint16_t                              display_width_minus1;       //if display_different_size==1
    uint16_t                              display_height_minus1;      //if display_different_size==1

    uint8_t                               version;
    uint8_t                               show_existing_frame_flag;
    uint8_t                               frame_type;
    uint8_t                               frame_toshow_slot;
    uint8_t                               show_frame_flag;
    uint8_t                               error_resilient_mode;
    uint8_t                               color_space;
    uint8_t                               subsampling_x;
    uint8_t                               subsampling_y;
    uint8_t                               refresh_frame_flags;
    uint8_t                               display_different_size_flag;
    uint8_t                               frame_parallel_decoding_mode;
    uint8_t                               ten_or_twelve_bit;
    uint8_t                               color_range;
    uint8_t                               intra_only;
    uint8_t                               ref_frame_idx[3];
    uint8_t                               ref_frame_sign_bias[3];
} MOS_ALIGNED(MHW_CACHELINE_SIZE)  CODECHAL_DECODE_VP9_PIC_INFO, *PCODECHAL_DECODE_VP9_PIC_INFO;

typedef struct _CODECHAL_DECRYPT_STATUS_REPORT_VP9
{
    CODECHAL_DECODE_VP9_PIC_INFO        PicInfo;
    int8_t                              i8ReservedPerformance[512];
} CODECHAL_DECRYPT_STATUS_REPORT_VP9, *PCODECHAL_DECRYPT_STATUS_REPORT_VP9;

typedef struct
{
    // for context refresh
    uint8_t                               intra_only;
    uint8_t                               reset_frame_context;
    int8_t                                frame_context_idx;
} CODECHAL_VP9_FRAME_CONTEXT_MANAGE_INFO;

typedef struct
{
    uint16_t                              width;
    uint16_t                              height;
    uint8_t                               frame_type;
    uint8_t                               intra_only;
    uint8_t                               show_frame_flag;
} CODECHAL_DECODE_VP9_IMMLAST_FRAME_INFO;

typedef struct
{
    uint16_t                              width;
    uint16_t                              height;
} CODECHAL_DECODE_VP9_FRAME_SIZE;

typedef struct
{
    int8_t                                ref_deltas[CODEC_VP9_MAX_REF_LF_DELTAS];      // 0 = Intra, Last, GF, ARF
    int8_t                                mode_deltas[CODEC_VP9_MAX_MODE_LF_DELTAS];    // 0 = ZERO_MV, MV
} CODECHAL_DECODE_VP9_LOOP_FILTER_DELTAS;

typedef struct
{
    uint8_t                               enabled;
    uint8_t                               abs_delta;
    uint8_t                               segment_update_map;
    uint8_t                               segment_update_data;
    uint8_t                               segment_temporal_delta;
    uint8_t                               segment_tree_probs[CODECHAL_VP9_SEG_TREE_PROBS];
    uint8_t                               segment_pred_probs[CODEC_VP9_PREDICTION_PROBS];
    int16_t                               feature_data[CODEC_VP9_MAX_SEGMENTS][CODECHAL_DECODE_VP9_SEG_LVL_MAX];
    uint8_t                               feature_mask[CODEC_VP9_MAX_SEGMENTS];
} CODECHAL_DECODE_VP9_SEGMENT_FEATURE;

typedef struct _CODECHAL_DECRYPT_SHARED_BUFFER_VP9
{
    CODECHAL_DECRYPT_STATUS_REPORT_VP9         StatusReport[8];

    // Standard specific parameters
    CODECHAL_VP9_FRAME_CONTEXT_MANAGE_INFO     MOS_ALIGNED(MHW_CACHELINE_SIZE) FrameContextManageInfo;
    uint8_t                                    ActiveRefSlot[CODECHAL_MAX_CUR_NUM_REF_FRAME_VP9];
    uint8_t                                    RefFrameSignBias[CODECHAL_MAX_CUR_NUM_REF_FRAME_VP9];
    CODECHAL_DECODE_VP9_IMMLAST_FRAME_INFO     MOS_ALIGNED(MHW_CACHELINE_SIZE) ImmLastFrameInfo;
    int8_t                                     RefFrameMap[CODEC_VP9_NUM_REF_FRAMES];
    int32_t                                    FrameRefBufCnt[CODECHAL_VP9_NUM_DPB_BUFFERS];
    CODECHAL_DECODE_VP9_FRAME_SIZE             FrameSize[CODECHAL_VP9_NUM_DPB_BUFFERS];
    CODECHAL_DECODE_VP9_LOOP_FILTER_DELTAS     LoopFilterDeltas;
    CODECHAL_DECODE_VP9_SEGMENT_FEATURE        SegmentFeature;
    uint8_t                                    ui8toAlign[MHW_CACHELINE_SIZE];

#ifdef CODECHAL_HUC_KERNEL_DEBUG
    CODECHAL_HUC_KERNEL_DEBUG_INFO             HucKernelDebugInfo[8];
#endif
} CODECHAL_DECRYPT_SHARED_BUFFER_VP9, *PCODECHAL_DECRYPT_SHARED_BUFFER_VP9;

typedef struct _CODECHAL_DECODE_VP9_DPB_PARAMS
{
    uint8_t    ui8RefreshFrameFlag;
    uint8_t    ActiveRefSlot[CODECHAL_MAX_CUR_NUM_REF_FRAME_VP9];
    uint8_t    RefFrameSignBias[CODECHAL_MAX_CUR_NUM_REF_FRAME_VP9];
} CODECHAL_DECODE_VP9_DPB_PARAMS, *PCODECHAL_DECODE_VP9_DPB_PARAMS;

//!
//! \class VP9 Decode Decrypt 
//! \brief This class defines the member fields, functions etc used by Vp9 Decode Decrypt.
//!
class CodechalCencDecodeVp9 : public CodechalCencDecode
{
public:

    //!
    //! \brief  Constructor
    //! \param  [in] hwInterface
    //!         Hardware interface
    //!
    CodechalCencDecodeVp9();

    //!
    //! \brief  Destructor
    //!
    ~CodechalCencDecodeVp9();

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
    uint8_t m_refFrameMap[CODEC_VP9_NUM_REF_FRAMES]={};  //!< Reference frame map for DPB management
};

#endif  // __CODECHAL_CENC_DECODE_VP9_H__

