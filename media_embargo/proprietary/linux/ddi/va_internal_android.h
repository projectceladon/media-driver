/*
* Copyright (c) 2017, Intel Corporation
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
//! \file     va_internal_android.h
//! \brief    libva interface for android only, should be removed after android switch to libva2.0.1+
//!

#ifndef _VA_INTERNAL_ANDROID_H_
#define _VA_INTERNAL_ANDROID_H_

// Misc parameter for encoder
#define  VAEncMiscParameterTypeEncQuality -2

//Scene change parameter for ADI on Linux, if enabled, driver use spatial DI(Bob), instead of ADI. if not, use old behavior for ADI
//Input stream is TFF(set flags = 0), SRC0,1,2,3 are interlaced frame (top +bottom fields), DSTs are progressive frames
//30i->30p
//SRC0 -> BOBDI,  no reference, set flag = 0, output DST0
//SRC1 -> ADI, reference frame=SRC0, set flags = 0, call VP, output DST1
//SRC2 -> ADI, reference frame=SRC1, set flags = 0x0010(decimal 16), call VP, output DST2(T4)
//SRC3 -> ADI, reference frame=SRC2, set flags = 0, call VP, output DST3
//30i->60p
//SRC0 -> BOBDI, no reference, set flag = 0, output DST0
//SRC0 -> BOBDI, no reference, set flag =0x0002, output DST1

//SRC1 -> ADI, reference frame =SRC0, set flags = 0, call VP, output DST2
//SRC1 -> ADI, reference frame =SRC0, set flags = 0x0012(decimal18), call VP, output DST3(B3)

//SRC2 -> ADI, reference frame =SRC1, set flags =  0x0010(decimal 16), call VP, output DST4(T4)
//SRC2 -> ADI, reference frame =SRC1, set flags =  0x0002, call VP, output DST5

//SRC3 -> ADI, reference frame =SRC2, set flags =  0, call VP, output DST6
//SRC3 -> ADI, reference frame =SRC1, set flags = 0x0002, call VP, output DST7

#define VA_DEINTERLACING_SCD_ENABLE     0x0010

// Set MB partion mode mask and Half-pel/Quant-pel motion search
#define VAEncMiscParameterTypeSubMbPartPel - 17


#define MAX_NUM_TEMP_LAYERS       16

#ifndef VAConfigAttribFrameSizeToleranceSupport
#define VAConfigAttribFrameSizeToleranceSupport 33
#endif

// Indicate allocated buffer size is not enough for input or output
#define VA_STATUS_ERROR_NOT_ENOUGH_BUFFER       0x00000025

// Private per frame encoder controls, once set they will persist for all future frames
// till it is updated again.
typedef struct _VAEncMiscParameterEncQualtiy
{
    unsigned int target_usage; // Valid values 1-7 for AVC & MPEG2.
    union
    {
        struct
        {
            // Use raw frames for reference instead of reconstructed frames.
            unsigned int useRawPicForRef                    : 1;
            // Disables skip check for ENC.
            unsigned int skipCheckDisable                   : 1;
            // Indicates app will override default driver FTQ settings using FTQEnable.
            unsigned int FTQOverride                        : 1;
            // Enables/disables FTQ.
            unsigned int FTQEnable                          : 1;
            // Indicates the app will provide the Skip Threshold LUT to use when FTQ is
            // enabled (FTQSkipThresholdLUT), else default driver thresholds will be used.
            unsigned int FTQSkipThresholdLUTInput           : 1;
            // Indicates the app will provide the Skip Threshold LUT to use when FTQ is
            // disabled (NonFTQSkipThresholdLUT), else default driver thresholds will be used.
            unsigned int NonFTQSkipThresholdLUTInput        : 1;
            unsigned int ReservedBit                        : 1;
            // Control to enable the ENC mode decision algorithm to bias to fewer B Direct/Skip types.
            // Applies only to B frames, all other frames will ignore this setting.
            unsigned int directBiasAdjustmentEnable         : 1;
            // Enables global motion bias.
            unsigned int globalMotionBiasAdjustmentEnable   : 1;
            // MV cost scaling ratio for HME predictors.  It is used when
            // globalMotionBiasAdjustmentEnable == 1, else it is ignored.  Values are:
            //      0: set MV cost to be 0 for HME predictor.
            //      1: scale MV cost to be 1/2 of the default value for HME predictor.
            //      2: scale MV cost to be 1/4 of the default value for HME predictor.
            //      3: scale MV cost to be 1/8 of the default value for HME predictor.
            unsigned int HMEMVCostScalingFactor             : 2;
            //disable HME
            unsigned int HMEDisable                         : 1;
            //disable Super HME
            unsigned int SuperHMEDisable                    : 1;
            //disable Ultra HME
            unsigned int UltraHMEDisable                    : 1;
            // Force Disable the panic mode
            //      0: - force to enable the panic mode
            //      1: - force to disable the panic mode
            unsigned int PanicModeDisable              : 1;
            // Force RepartitionCheck
            //      0: DEFAULT - follow driver default settings.
            //      1: FORCE_ENABLE - enable this feature totally for all cases.
            //      2: FORCE_DISABLE - disable this feature totally for all cases.
            unsigned int ForceRepartitionCheck              : 2;

        };
        unsigned int encControls;
    };

    // Maps QP to skip thresholds when FTQ is enabled.  Valid range is 0-255.
    unsigned char FTQSkipThresholdLUT[52];
    // Maps QP to skip thresholds when FTQ is disabled.  Valid range is 0-65535.
    unsigned short NonFTQSkipThresholdLUT[52];

    unsigned int reserved[21];  // Reserved for future use.

} VAEncMiscParameterEncQuality;

/**
 *  \brief Custom Encoder Rounding Offset Control.
 *  Application may use this structure to set customized rounding
 *  offset parameters for quantization.
 *  Valid when \c VAConfigAttribCustomRoundingControl equals 1.
 */
typedef struct _VAEncMiscParameterCustomRoundingControl
{
    union {
        struct {
            /** \brief Enable customized rounding offset for intra blocks.
             *  If 0, default value would be taken by driver for intra
             *  rounding offset.
             */
            unsigned int    enable_custom_rouding_intra     : 1 ;

            /** \brief Intra rounding offset
             *  Ignored if \c enable_custom_rouding_intra equals 0.
             *  Value range [0..7] for AVC.
             */
            unsigned int    rounding_offset_intra           : 7;

            /** \brief Enable customized rounding offset for inter blocks.
             *  If 0, default value would be taken by driver for inter
             *  rounding offset.
             */
            unsigned int    enable_custom_rounding_inter    : 1 ;

            /** \brief Inter rounding offset
             *  Ignored if \c enable_custom_rouding_inter equals 0.
             *  Value range [0..7] for AVC.
             */
            unsigned int    rounding_offset_inter           : 7;

           /* Reserved */
            unsigned int    reserved                        :16;
        }  bits;
        unsigned int    value;
    }   rounding_offset_setting;
} VAEncMiscParameterCustomRoundingControl;

/** \brief Buffer type used for encoder rounding offset parameters. */
#define VAEncMiscParameterTypeCustomRoundingControl   -9


/**
* \defgroup api_enc_vp9 VP9 encoding API
*
* @{
*/

/**
 * \brief VP9 Encoding Information Buffer Structure
 *
 * This structure is used to convey information from encoder to application.
 * When VACodedBufferSegment.status takes value VA_CODED_BUF_STATUS_CODEC_SPECIFIC,
 * VACodedBufferSegment.buf should point to VAEncInformationBufferVP9.
 *
 */
typedef struct  _VAEncInformationBufferVP9
{
    /* suggest frame width for future frames */
    uint32_t    suggest_frame_width;
    /* suggest frame height for future frames */
    uint32_t    suggest_frame_height;

    union {
        struct {
            /* suggest this frame to be a golden frame */
            uint32_t    suggest_gf              : 1;

            /* suggest this frame to be a last frame */
            uint32_t    suggest_last            : 1;

            /* suggest this frame to be an altref frame */
            uint32_t    suggest_arf             : 1;

            uint32_t    reserved                : 29;
        } bits;
        uint32_t    value;
    } info_flags;

} VAEncInformationBufferVP9;

/**
 * \brief VP9 Encoding Sequence Parameter Buffer Structure
 *
 * This structure conveys sequence level parameters.
 *
 */
typedef struct  _VAEncSequenceParameterBufferVP9
{
    /** \brief Frame size note:
     *  Picture resolution may change frame by frame.
     *  Application needs to allocate surfaces and frame buffers based on
     *  max frame resolution in case resolution changes for later frames.
     *  The source and recon surfaces allocated should be 64x64(SB) aligned
     *  on both horizontal and vertical directions.
     *  But buffers on the surfaces need to be aligned to CU boundaries.
     */
    /* maximum frame width in pixels for the whole sequence */
    uint32_t    max_frame_width;

    /* maximum frame height in pixels for the whole sequence */
    uint32_t    max_frame_height;

    /* auto keyframe placement, non-zero means enable auto keyframe placement */
    uint32_t    kf_auto;

    /* keyframe minimum interval */
    uint32_t    kf_min_dist;

    /* keyframe maximum interval */
    uint32_t    kf_max_dist;


    /* RC related fields. RC modes are set with VAConfigAttribRateControl */
    /* For VP9, CBR implies HRD conformance and VBR implies no HRD conformance */

    /**
     * Initial bitrate set for this sequence in CBR or VBR modes.
     *
     * This field represents the initial bitrate value for this
     * sequence if CBR or VBR mode is used, i.e. if the encoder
     * pipeline was created with a #VAConfigAttribRateControl
     * attribute set to either \ref VA_RC_CBR or \ref VA_RC_VBR.
     *
     * The bitrate can be modified later on through
     * #VAEncMiscParameterRateControl buffers.
     */
    uint32_t    bits_per_second;

    /* Period between key frames */
    uint32_t    intra_period;

    /** \brief Reserved bytes for future use, must be zero */
    //uint32_t                va_reserved[VA_PADDING_LOW];
} VAEncSequenceParameterBufferVP9;


/**
 * \brief VP9 Encoding Picture Parameter Buffer Structure
 *
 * This structure conveys picture level parameters.
 *
 */
typedef struct  _VAEncPictureParameterBufferVP9
{
    /** VP9 encoder may support dynamic scaling function.
     *  If enabled (enable_dynamic_scaling is set), application may request
     *  GPU encodes picture with a different resolution from the raw source.
     *  GPU should handle the scaling process of source and
     *  all reference frames.
     */
    /* raw source frame width in pixels */
    uint32_t    frame_width_src;
    /* raw source frame height in pixels */
    uint32_t    frame_height_src;

    /* to be encoded frame width in pixels */
    uint32_t    frame_width_dst;
    /* to be encoded frame height in pixels */
    uint32_t    frame_height_dst;

    /* surface to store reconstructed frame, not used for enc only case */
    VASurfaceID reconstructed_frame;

    /** \brief reference frame buffers
     *  Each entry of the array specifies the surface index of the picture
     *  that is referred by current picture or will be referred by any future
     *  picture. The valid entries take value from 0 to 127, inclusive.
     *  Non-valid entries, those do not point to pictures which are referred
     *  by current picture or future pictures, should take value 0xFF.
     *  Other values are not allowed.
     *
     *  Application should update this array based on the refreshing
     *  information expected.
     */
    VASurfaceID reference_frames[8];

    /* buffer to store coded data */
    VABufferID  coded_buf;

    union {
        struct {
            /* force this frame to be a keyframe */
            uint32_t    force_kf                       : 1;

            /** \brief Indiates which frames to be used as reference.
             *  (Ref_frame_ctrl & 0x01) ? 1: last frame as reference frame, 0: not.
             *  (Ref_frame_ctrl & 0x02) ? 1: golden frame as reference frame, 0: not.
             *  (Ref_frame_ctrl & 0x04) ? 1: alt frame as reference frame, 0: not.
             *  L0 is for forward prediction.
             *  L1 is for backward prediction.
             */
            uint32_t    ref_frame_ctrl_l0              : 3;
            uint32_t    ref_frame_ctrl_l1              : 3;

            /** \brief Last Reference Frame index
             *  Specifies the index to RefFrameList[] which points to the LAST
             *  reference frame. It corresponds to active_ref_idx[0] in VP9 code.
             */
            uint32_t    ref_last_idx                   : 3;

            /** \brief Specifies the Sign Bias of the LAST reference frame.
             *  It corresponds to ref_frame_sign_bias[LAST_FRAME] in VP9 code.
             */
            uint32_t    ref_last_sign_bias             : 1;

            /** \brief GOLDEN Reference Frame index
             *  Specifies the index to RefFrameList[] which points to the Golden
             *  reference frame. It corresponds to active_ref_idx[1] in VP9 code.
             */
            uint32_t    ref_gf_idx                     : 3;

            /** \brief Specifies the Sign Bias of the GOLDEN reference frame.
             *  It corresponds to ref_frame_sign_bias[GOLDEN_FRAME] in VP9 code.
             */
            uint32_t    ref_gf_sign_bias               : 1;

            /** \brief Alternate Reference Frame index
             *  Specifies the index to RefFrameList[] which points to the Alternate
             *  reference frame. It corresponds to active_ref_idx[2] in VP9 code.
             */
            uint32_t    ref_arf_idx                    : 3;

            /** \brief Specifies the Sign Bias of the ALTERNATE reference frame.
             *  It corresponds to ref_frame_sign_bias[ALTREF_FRAME] in VP9 code.
             */
            uint32_t    ref_arf_sign_bias              : 1;

            /* The temporal id the frame belongs to */
            uint32_t    temporal_id                    : 8;

            uint32_t    reserved                       : 5;
        } bits;
        uint32_t value;
    } ref_flags;

    union {
        struct {
            /**
             * Indicates if the current frame is a key frame or not.
             * Corresponds to the same VP9 syntax element in frame tag.
             */
            uint32_t    frame_type                     : 1;

            /** \brief show_frame
             *  0: current frame is not for display
             *  1: current frame is for display
             */
            uint32_t    show_frame                     : 1;

            /**
             * The following fields correspond to the same VP9 syntax elements
             * in the frame header.
             */
            uint32_t    error_resilient_mode           : 1;

            /** \brief Indicate intra-only for inter pictures.
             *  Must be 0 for key frames.
             *  0: inter frame use both intra and inter blocks
             *  1: inter frame use only intra blocks.
             */
            uint32_t    intra_only                     : 1;

            /** \brief Indicate high precision mode for Motion Vector prediction
             *  0: normal mode
             *  1: high precision mode
             */
            uint32_t    allow_high_precision_mv        : 1;

            /** \brief Motion Compensation Filter type
             *  0: eight-tap  (only this mode is supported now.)
             *  1: eight-tap-smooth
             *  2: eight-tap-sharp
             *  3: bilinear
             *  4: switchable
             */
            uint32_t    mcomp_filter_type              : 3;
            uint32_t    frame_parallel_decoding_mode   : 1;
            uint32_t    reset_frame_context            : 2;
            uint32_t    refresh_frame_context          : 1;
            uint32_t    frame_context_idx              : 2;
            uint32_t    segmentation_enabled           : 1;

            /* corresponds to variable temporal_update in VP9 code.
             * Indicates whether Segment ID is from bitstream or from previous
             * frame.
             * 0: Segment ID from bitstream
             * 1: Segment ID from previous frame
             */
            uint32_t    segmentation_temporal_update   : 1;

            /* corresponds to variable update_mb_segmentation_map in VP9 code.
             * Indicates how hardware determines segmentation ID
             * 0: intra block - segment id is 0;
             *    inter block - segment id from previous frame
             * 1: intra block - segment id from bitstream (app or GPU decides)
             *    inter block - depends on segmentation_temporal_update
             */
            uint32_t    segmentation_update_map        : 1;

            /** \brief Specifies if the picture is coded in lossless mode.
             *
             *  lossless_mode = base_qindex == 0 && y_dc_delta_q == 0  \
             *                  && uv_dc_delta_q == 0 && uv_ac_delta_q == 0;
             *  Where base_qindex, y_dc_delta_q, uv_dc_delta_q and uv_ac_delta_q
             *  are all variables in VP9 code.
             *
             *  When enabled, tx_mode needs to be set to 4x4 only and all
             *  tu_size in CU record set to 4x4 for entire frame.
             *  Software also has to program such that final_qindex=0 and
             *  final_filter_level=0 following the Quant Scale and
             *  Filter Level Table in Segmentation State section.
             *  Hardware forces Hadamard Tx when this bit is set.
             *  When lossless_mode is on, BRC has to be turned off.
             *  0: normal mode
             *  1: lossless mode
             */
            uint32_t    lossless_mode                  : 1;

            /** \brief MV prediction mode. Corresponds to VP9 variable with same name.
             *  comp_prediction_mode = 0:        single prediction ony,
             *  comp_prediction_mode = 1:        compound prediction,
             *  comp_prediction_mode = 2:        hybrid prediction
             *
             *  Not mandatory. App may suggest the setting based on power or
             *  performance. Kernal may use it as a guildline and decide the proper
             *  setting on its own.
             */
            uint32_t    comp_prediction_mode           : 2;

            /** \brief Indicate how segmentation is specified
             *  0   application specifies segmentation partitioning and
             *      relevant parameters.
             *  1   GPU may decide on segmentation. If application already
             *      provides segmentation information, GPU may choose to
             *      honor it and further split into more levels if possible.
             */
            uint32_t    auto_segmentation              : 1;

            /** \brief Indicate super frame syntax should be inserted
             *  0   current frame is not encapsulated in super frame structure
             *  1   current fame is to be encapsulated in super frame structure.
             *      super frame index syntax will be inserted by encoder at
             *      the end of current frame.
             */
            uint32_t    super_frame_flag               : 1;

            uint32_t    reserved                       : 10;
        } bits;
        uint32_t    value;
    } pic_flags;

    /** \brief indicate which frames in DPB should be refreshed.
     *  same syntax and semantic as in VP9 code.
     */
    uint8_t     refresh_frame_flags;

    /** \brief Base Q index in the VP9 term.
     *  Added with per segment delta Q index to get Q index of Luma AC.
     */
    uint8_t     luma_ac_qindex;

    /**
     *  Q index delta from base Q index in the VP9 term for Luma DC.
     */
    int8_t      luma_dc_qindex_delta;

    /**
     *  Q index delta from base Q index in the VP9 term for Chroma AC.
     */
    int8_t      chroma_ac_qindex_delta;

    /**
     *  Q index delta from base Q index in the VP9 term for Chroma DC.
     */
    int8_t      chroma_dc_qindex_delta;

    /** \brief filter level
     *  Corresponds to the same VP9 syntax element in frame header.
     */
    uint8_t     filter_level;

    /**
     * Controls the deblocking filter sensitivity.
     * Corresponds to the same VP9 syntax element in frame header.
     */
    uint8_t     sharpness_level;

    /** \brief Loop filter level reference delta values.
     *  Contains a list of 4 delta values for reference frame based block-level
     *  loop filter adjustment.
     *  If no update, set to 0.
     *  value range [-63..63]
     */
    int8_t      ref_lf_delta[4];

    /** \brief Loop filter level mode delta values.
     *  Contains a list of 4 delta values for coding mode based MB-level loop
     *  filter adjustment.
     *  If no update, set to 0.
     *  value range [-63..63]
     */
    int8_t      mode_lf_delta[2];

    /**
     *  Offset from starting position of output bitstream in bits where
     *  ref_lf_delta[] should be inserted. This offset should cover any metadata
     *  ahead of uncompressed header in inserted bit stream buffer (the offset
     *  should be same as that for final output bitstream buffer).
     *
     *  In BRC mode, always insert ref_lf_delta[] (This implies uncompressed
     *  header should have mode_ref_delta_enabled=1 and mode_ref_delta_update=1).
     */
    uint16_t    bit_offset_ref_lf_delta;

    /**
     *  Offset from starting position of output bitstream in bits where
     *  mode_lf_delta[] should be inserted.
     *
     *  In BRC mode, always insert mode_lf_delta[] (This implies uncompressed
     *  header should have mode_ref_delta_enabled=1 and mode_ref_delta_update=1).
     */
    uint16_t    bit_offset_mode_lf_delta;

    /**
     *  Offset from starting position of output bitstream in bits where (loop)
     *  filter_level should be inserted.
     */
    uint16_t    bit_offset_lf_level;

    /**
     *  Offset from starting position of output bitstream in bits where
     *  Base Qindex should be inserted.
     */
    uint16_t    bit_offset_qindex;

    /**
     *  Offset from starting position of output bitstream in bits where
     *  First Partition Size should be inserted.
     */
    uint16_t    bit_offset_first_partition_size;

    /**
     *  Offset from starting position of output bitstream in bits where
     *  segmentation_enabled is located in bitstream. When auto_segmentation
     *  is enabled, GPU uses this offset to locate and update the
     *  segmentation related information.
     */
    uint16_t    bit_offset_segmentation;

    /** \brief length in bit of segmentation portion from the location
     *  in bit stream where segmentation_enabled syntax is coded.
     *  When auto_segmentation is enabled, GPU uses this bit size to locate
     *  and update the information after segmentation.
     */
    uint16_t    bit_size_segmentation;


    /** \brief log2 of number of tile rows
     *  Corresponds to the same VP9 syntax element in frame header.
     *  value range [0..2]
     */
    uint8_t     log2_tile_rows;

    /** \brief log2 of number of tile columns
     *  Corresponds to the same VP9 syntax element in frame header.
     *  value range [0..5]
     */
    uint8_t     log2_tile_columns;

    /** \brief indicate frame-skip happens
     *  Application may choose to drop/skip one or mulitple encoded frames or
     *  to-be-encoded frame due to various reasons such as insufficient
     *  bandwidth.
     *  Application uses the following three flags to inform GPU about frame-skip.
     *
     *  value range of skip_frame_flag: [0..2]
     *      0 - encode as normal, no skip;
     *      1 - one or more frames were skipped by application prior to the
     *          current frame. Encode the current frame as normal. The driver
     *          will pass the number_skip_frames and skip_frames_size
     *          to bit rate control for adjustment.
     *      2 - the current frame is to be skipped. Do not encode it but encrypt
     *          the packed header contents. This is for the secure encoding case
     *          where application generates a frame of all skipped blocks.
     *          The packed header will contain the skipped frame.
     */
    uint8_t     skip_frame_flag;

    /** \brief The number of frames skipped prior to the current frame.
     *  It includes only the skipped frames that were not counted before,
     *  and does not include the frame with skip_frame_flag == 2.
     *  Valid when skip_frame_flag = 1.
     */
    uint8_t     number_skip_frames;

    /** \brief When skip_frame_flag = 1, the size of the skipped frames in bits.
     *  It includes only the skipped frames that were not counted before,
     *  and does not include the frame size with skip_frame_flag = 2.
     *  When skip_frame_flag = 2, it is the size of the current skipped frame
     *  that is to be encrypted.
     */
    uint32_t    skip_frames_size;

    /** \brief Reserved bytes for future use, must be zero */
    //uint32_t    va_reserved[VA_PADDING_MEDIUM];
} VAEncPictureParameterBufferVP9;

typedef uint8_t VASegmentID;


/**
 * \brief VP9 Block Segmentation ID Buffer
 *
 * application provides buffer containing the initial segmentation id for each
 * block (8x8 in pixels), in raster scan order. Rate control may reassign it.
 * For an 640x480 video, the buffer has 6400 entries.
 * the value of each entry should be in the range [0..7], inclusive.
 * If segmentation is not enabled, application does not need to provide it.
 */
typedef struct _VAEncSegmentMapVP9
{
    /**
     * number of blocks in the frame.
     * It is also the number of entries of block_segment_id[];
     */
    uint32_t         block_num;
    /**
     * per Block Segmentation ID Buffer
     */
    VASegmentID*     block_segment_id;
} VAEncSegmentMapVP9;


/**
 * \brief Per segment parameters
 */
typedef struct _VAEncSegParamVP9
{
    union {
        struct {
            /** \brief Indicates if per segment reference frame indicator is enabled.
             *  Corresponding to variable feature_enabled when
             *  j == SEG_LVL_REF_FRAME in function setup_segmentation() VP9 code.
             */
            uint8_t     segment_reference_enabled       : 1;

            /** \brief Specifies per segment reference indication.
             *  0: reserved
             *  1: Last ref
             *  2: golden
             *  3: altref
             *  Value can be derived from variable data when
             *  j == SEG_LVL_REF_FRAME in function setup_segmentation() VP9 code.
             *  value range: [0..3]
             */
            uint8_t     segment_reference               : 2;

            /** \brief Indicates if per segment skip mode is enabled.
             *  Corresponding to variable feature_enabled when
             *  j == SEG_LVL_SKIP in function setup_segmentation() VP9 code.
             */
            uint8_t     segment_reference_skipped       : 1;

            uint8_t     reserved                        : 4;

        } bits;
        uint8_t value;
    } seg_flags;

    /** \brief Specifies per segment Loop Filter Delta.
     *  Must be 0 when segmentation_enabled == 0.
     *  value range: [-63..63]
     */
    int8_t      segment_lf_level_delta;

    /** \brief Specifies per segment QIndex Delta.
     *  Must be 0 when segmentation_enabled == 0.
     *  value range: [-255..255]
     */
    int16_t     segment_qindex_delta;

    /** \brief Reserved bytes for future use, must be zero */
    //uint32_t                va_reserved[VA_PADDING_LOW];
} VAEncSegParamVP9;

/**
 *  Structure to convey all segment related information.
 *  If segmentation is disabled, this data structure is still required.
 *  In this case, only seg_data[0] contains valid data.
 */
typedef struct _VAEncSegmentParameterBufferVP9
{
    /**
     *  Parameters for 8 segments.
     */
    VAEncSegParamVP9    seg_data[8];

    /** \brief Reserved bytes for future use, must be zero */
    //uint32_t                va_reserved[VA_PADDING_LOW];
} VAEncSegmentParameterBufferVP9;

/**
 * \brief VP9 Block Segmentation ID Buffer
 *
 * The application provides a buffer of VAEncMacroblockMapBufferType containing
 * the initial segmentation id for each 8x8 block, one byte each, in raster scan order.
 * Rate control may reassign it.  For example, a 640x480 video, the buffer has 4800 entries.
 * The value of each entry should be in the range [0..7], inclusive.
 * If segmentation is not enabled, the application does not need to provide it.
 */
typedef struct _VASegmentIdVP9
{
    /* buffer to store coded data */
    VABufferID  segment_id;

} VASegmentIdVP9;

/**
 * \brief MB partition modes and 1/2 1/4 motion search configuration  
 *
 * Specifies MB partition modes that are disabled. Specifies Half-pel
 * mode and Quarter-pel mode searching 
 */
typedef struct _VAEncMiscParameterSubMbPartPelH264
{
    bool disable_inter_sub_mb_partition;
    union {
        struct {
            unsigned int disable_16x16_inter_mb_partition        : 1;
            unsigned int disable_16x8_inter_mb_partition         : 1;
            unsigned int disable_8x16_inter_mb_partition         : 1;
            unsigned int disable_8x8_inter_mb_partition          : 1;            
            unsigned int disable_8x4_inter_mb_partition          : 1;
            unsigned int disable_4x8_inter_mb_partition          : 1;
            unsigned int disable_4x4_inter_mb_partition          : 1;
            unsigned int reserved                                : 1;
        } bits;
         unsigned char value;
    } inter_sub_mb_partition_mask;

    /**
     * \brief Precison of motion search
     * 0:Integer mode searching
     * 1:Half-pel mode searching
     * 2:Reserved
     * 3:Quarter-pel mode searching
     */
    bool enable_sub_pel_mode;
    unsigned char sub_pel_mode;
} VAEncMiscParameterSubMbPartPelH264;

/** \brief Attribute value for VAConfigAttribEncROI */
typedef union _VAConfigAttribValEncROIPrivate {
    struct {
        /** \brief The number of ROI regions supported, 0 if ROI is not supported. */
        unsigned int num_roi_regions        : 8;
        /** \brief Indicates if ROI priority indication is supported when
         * VAConfigAttribRateControl != VA_RC_CQP, else only ROI delta QP added on top of
         * the frame level QP is supported when VAConfigAttribRateControl == VA_RC_CQP.
         */
        unsigned int roi_rc_priority_support    : 1;
        /**
         * \brief A flag indicates whether ROI delta QP is supported
         *
         * \ref roi_rc_qp_delat_support equal to 1 specifies the underlying driver supports
         * ROI delta QP when VAConfigAttribRateControl != VA_RC_CQP, user can use \c roi_value
         * in #VAEncROI to set ROI delta QP. \ref roi_rc_qp_delat_support equal to 0 specifies
         * the underlying driver doesn't support ROI delta QP.
         *
         * User should ignore \ref roi_rc_qp_delat_support when VAConfigAttribRateControl == VA_RC_CQP
         * because ROI delta QP is always required when VAConfigAttribRateControl == VA_RC_CQP.
         */
        unsigned int roi_rc_qp_delat_support    : 1;
        unsigned int reserved                   : 22;
     } bits;
     unsigned int value;
} VAConfigAttribValEncROIPrivate;

typedef struct _VAEncMiscParameterBufferROIPrivate {
    /** \brief Number of ROIs being sent.*/
    unsigned int        num_roi;

    /** \brief Valid when VAConfigAttribRateControl != VA_RC_CQP, then the encoder's
     *  rate control will determine actual delta QPs.  Specifies the max/min allowed delta
     *  QPs. */
    char                max_delta_qp;
    char                min_delta_qp;

   /** \brief Pointer to a VAEncRoi array with num_roi elements.  It is relative to frame
     *  coordinates for the frame case and to field coordinates for the field case.*/
    VAEncROI            *roi;
    union {
        struct {
            /**
             * \brief An indication for roi value.
             *
             * \ref roi_value_is_qp_delta equal to 1 indicates \c roi_value in #VAEncROI should
             * be used as ROI delta QP. \ref roi_value_is_qp_delta equal to 0 indicates \c roi_value
             * in #VAEncROI should be used as ROI priority.
             *
             * \ref roi_value_is_qp_delta is only available when VAConfigAttribRateControl != VA_RC_CQP,
             * the setting must comply with \c roi_rc_priority_support and \c roi_rc_qp_delat_support in
             * #VAConfigAttribValEncROI. The underlying driver should ignore this field
             * when VAConfigAttribRateControl == VA_RC_CQP.
             */
            uint32_t  roi_value_is_qp_delta    : 1;
            uint32_t  reserved                 : 31;
        } bits;
        uint32_t value;
    } roi_flags;
} VAEncMiscParameterBufferROIPrivate;

#define VAEncMiscParameterTypeROIPrivate        -5



/*
 * Video Acceleration (VA) FEI API Specification
 *
 * Rev. 0.10
 * <kelvin.hu@intel.com>
 *
 * Revision History:
 * rev 0.10 (05/20/2015 Hu Kelvin) - Initial draft
 * rev 0.11 (05/25/2015 Hu Kelvin) - Added block8x8 support in VAStatsStatistics16x16Intel 
 * rev 0.12 (06/11/2015 Hu Kelvin) - Set search_window to 4 bits and replaced max_len_sp with search_path in VAEncMiscParameterFEIFrameControlH264Intel and VAStatsStatisticsParameter16x16Intel 
 * rev 0.13 (10/02/2015 Hu Kelvin) - Replaced VASurfaceID with VAPictureFEI in VAStatsStatistics16x16Intel for input, past/future_references, it allows indicate reference field type
 * rev 0.14 (12/01/2015 Hu Kelvin) - Added PreENC CONTENT_UPDATED 
 * rev 0.15 (02/25/2016 Hu Kelvin) - Added num_mv_predictors_l1 to VAEncMiscParameterFEIFrameControlH264Intel for performance optimization 
 * rev 0.16 (02/28/2016 Luo Ning)  - Added *past_ref_stat_buf and *future_ref_stat_buf to VAEncMiscParameterFEIFrameControlH264Intel for Preenc Stateless support
 * rev 0.17 (03/30/2016 Hu Kelvin) - Added VAStatsStatisticsBotFieldBufferTypeIntel buffer for independent Bottom field buffer 
 * rev 0.18 (05/04/2016 Zhang Carl)- Added FEI Multiple pass pack -- max frame size
 * rev 0.19 (05/12/2016 Hu Kelvin) - Changed VAStatsStatisticsBotFieldBufferTypeIntel to 1011 since BufferTypeIntel should start from 1000 
 */

#define VA_CONFIG_ATTRIB_FEI_INTERFACE_REV_INTEL  0x019 

/**
     * \brief FEI interface Revision attribute. Read-only.
     *
     * This attribute exposes a version number for both VAEntrypointStatistics entry and VAEntrypointEncFEIIntel entry
     * point. The attribute value is used by MSDK team to track va FEI interface change version
     */
#define VAConfigAttribFeiInterfaceRevIntel        1004

#ifndef VA_FEI_FUNCTION_ENC_PAK

    /**
     * \brief VAEntrypointEncFEIIntel
     *
     * The purpose of FEI (Flexible Encoding Infrastructure) is to allow applications to 
     * have more controls and trade off quality for speed with their own IPs. A pre-processing 
     * function for getting some statistics and motion vectors is added 
     * and some extra controls for Encode pipeline are provided. 
     * The application can optionally call the statistics function
     * to get motion vectors and statistics before calling encode function. 
     * The application can also optionally provide input to VME for extra 
     * encode control and get the output from VME. Application can chose to 
     * modify the VME output/PAK input during encoding, but the performance 
     * impact is significant.
     *
     * On top of the existing buffers for normal encode, there will be 
     * one extra input buffer (VAEncMiscParameterIntelFEIFrameControl) and 
     * three extra output buffers (VAIntelEncFEIMVBufferType, VAIntelEncFEIModeBufferType 
     * and VAIntelEncFEIDistortionBufferType) for VAEntrypointIntelEncFEI entry function. 
     * If separate PAK is set, two extra input buffers 
     * (VAIntelEncFEIMVBufferType, VAIntelEncFEIModeBufferType) are needed for PAK input. 
     *
     **/
#define VAEntrypointFEI                         1001  
    /**
     * \brief VAEntrypointStatisticsIntel
     *
     * Statistics, like variances, distortions, motion vectors can be obtained 
     * via this entry point. Checking whether Statistics is supported can be 
     * performed with vaQueryConfigEntrypoints() and the profile argument 
     * set to #VAProfileNone. If Statistics entry point is supported, 
     * then the list of returned entry-points will include #VAEntrypointIntelStatistics. 
     * Supported pixel format, maximum resolution and statistics specific attributes 
     * can be obtained via normal attribute query. 
     * One input buffer (VAIntelStatsStatisticsParameterBufferType) and one or two 
     * output buffers (VAIntelStatsStatisticsBufferType and VAIntelStatsMotionVectorBufferType) 
     * are needed for this entry point.
     *
     **/
#define VAEntrypointStats 1002

    /**
     * \brief Encode function type.
     *
     * This attribute conveys whether the driver supports different function types for encode. 
     * It can be ENC, PAK, or ENC + PAK. Currently it is for FEI entry point only. 
     * Default is ENC + PAK.
     */
#define VAConfigAttribFEIFunctionType   1001
    /**
     * \brief Maximum number of MV predictors. Read-only.
     *
     * This attribute determines the maximum number of MV predictors the driver 
     * can support to encode a single frame. 0 means no MV predictor is supported.
     */
#define VAConfigAttribFEIMVPredictors   1002
    /**
     * \brief Statistics attribute. Read-only.
     *
     * This attribute exposes a number of capabilities of the VAEntrypointStatistics entry 
     * point. The attribute value is partitioned into fields as defined in the 
     * VAConfigAttribValStatistics union.
     */
#define VAConfigAttribStats             1003

#define VAEncQPBufferType                         30
    /**
     * \brief Intel specific buffer types start at 1001
     */
#define VAEncFEIMVBufferType                 1001
#define VAEncFEIMBCodeBufferType               1002
#define VAEncFEIDistortionBufferType         1003
#define VAEncFEIMBControlBufferType          1004
#define VAEncFEIMVPredictorBufferType        1005
#define VAStatsStatisticsParameterBufferType 1006
#define VAStatsStatisticsBufferType          1007
#define VAStatsMVBufferType                  1008
#define VAStatsMVPredictorBufferType         1009


    /* Intel specific types start at 1001
       VAEntrypointEncFEIIntel */
#define VAEncMiscParameterTypeFEIFrameControl 1001

/**
 * \brief Intel specific attribute definitions
 */
/** @name Attribute values for VAConfigAttribEncFunctionTypeIntel
 *
 * The desired type should be passed to driver when creating the configuration. 
 * If VA_ENC_FUNCTION_ENC_PAK is set, VA_ENC_FUNCTION_ENC and VA_ENC_FUNCTION_PAK 
 * will be ignored if set also.  VA_ENC_FUNCTION_ENC and VA_ENC_FUNCTION_PAK operations 
 * shall be called separately if ENC and PAK (VA_ENC_FUNCTION_ENC | VA_ENC_FUNCTION_PAK) 
 * is set for configuration. VA_ENC_FUNCTION_ENC_PAK is recommended for best performance.
 * If only VA_ENC_FUNCTION_ENC is set, there will be no bitstream output. 
 * If VA_ENC_FUNCTION_ENC_PAK is not set and VA_ENC_FUNCTION_PAK is set, then two extra 
 * input buffers for PAK are needed: VAEncFEIMVBufferType and VAEncFEIModeBufferType.
 **/
/**@{*/
/** \brief ENC only is supported */
#define VA_FEI_FUNCTION_ENC         0x00000001
/** \brief PAK only is supported */
#define VA_FEI_FUNCTION_PAK                             0x00000002
/** \brief ENC_PAK is supported */
#define VA_FEI_FUNCTION_ENC_PAK                         0x00000004

/**@}*/

/** \brief Attribute value for VAConfigAttribStatisticsIntel */
typedef union _VAConfigAttribValStats {
    struct {
        /** \brief Max number of past reference frames that are supported. */
        unsigned int    max_num_past_references   : 4;
        /** \brief Max number of future reference frames that are supported. */
        unsigned int    max_num_future_references : 4;
        /** \brief Number of output surfaces that are supported */
        unsigned int    num_outputs               : 3;
        /** \brief Interlaced content is supported */
        unsigned int    interlaced                : 1;
        unsigned int    reserved                  : 20;
    } bits;
    unsigned int value;
} VAConfigAttribValStats;

/** \brief VAEncFEIMVBufferTypeIntel and VAStatsMotionVectorBufferTypeIntel. Motion vector buffer layout.
 * Motion vector output is per 4x4 block. For each 4x4 block there is a pair of past and future 
 * reference Motion Vectors as defined in VAMotionVectorIntel. Depending on Subblock partition, 
 * for the shape that is not 4x4, the MV is replicated so each 4x4 block has a pair of Motion Vectors. 
 * If only past reference is used, future MV should be ignored, and vice versa. 
 * The 16x16 block is in raster scan order, within the 16x16 block, each 4x4 block MV is ordered as below in memory. 
 * The buffer size shall be greater than or equal to the number of 16x16 blocks multiplied by (sizeof(VAMotionVector) * 16).
 *
 *                      16x16 Block        
 *        -----------------------------------------
 *        |    1    |    2    |    5    |    6    |
 *        -----------------------------------------
 *        |    3    |    4    |    7    |    8    |
 *        -----------------------------------------
 *        |    9    |    10   |    13   |    14   |
 *        -----------------------------------------
 *        |    11   |    12   |    15   |    16   |
 *        -----------------------------------------
 *
 **/

#endif  //end of #ifndef VA_ENC_FUNCTION_ENC_PAK_INTEL  

// Avoid 1010 since Android va.h has defined 1010 as VABufferTypeMax
#define VAStatsStatisticsBottomFieldBufferType  1011

#define VAEncMiscParameterTypeDirtyRect 1012

#define VAConfigAttribEncDirtyRect 500

typedef VAProcessingRateParams VAProcessingRateParameter;
typedef VAProcessingRateParamsEnc VAProcessingRateParameterEnc;
typedef VAProcessingRateParamsDec VAProcessingRateParameterDec;

typedef VAEncMiscParameterBufferDirtyROI VAEncMiscParameterBufferDirtyRect;

typedef VAEncSegmentParameterBufferVP9 VAEncMiscParameterTypeVP9PerSegmantParam;
//va_intel_fei.h
/**
 * \defgroup api_intel_fei Intel FEI (Flexible Encoding Infrastructure) encoding API
 *
 * @{
 */

/** \brief FEI frame level control buffer for H.264 */
typedef struct _VAEncMiscParameterFEIFrameControlH264 {
    unsigned int      function; /* one of the VAConfigAttribEncFunctionType values */   
    /** \brief MB (16x16) control input buffer. It is valid only when (mb_input | mb_size_ctrl) 
     * is set to 1. The data in this buffer correspond to the input source. 16x16 MB is in raster scan order, 
     * each MB control data structure is defined by VAEncFEIMBControlBufferH264. 
     * Buffer size shall not be less than the number of 16x16 blocks multiplied by 
     * sizeof(VAEncFEIMBControlBufferH264Intel). 
     * Note: if mb_qp is set, VAEncQpBufferH264 is expected.
     */
    VABufferID        mb_ctrl;
    /** \brief distortion output of MB ENC or ENC_PAK.
     * Each 16x16 block has one distortion data with VAEncFEIDistortionBufferH264Intel layout
     * Buffer size shall not be less than the number of 16x16 blocks multiplied by 
     * sizeof(VAEncFEIDistortionBufferH264Intel). 
     */
    VABufferID        distortion;
    /** \brief Motion Vectors data output of MB ENC.
     * Each 16x16 block has one Motion Vectors data with layout VAMotionVectorIntel 
     * Buffer size shall not be less than the number of 16x16 blocks multiplied by 
     * sizeof(VAMotionVectorIntel). 
     */
    VABufferID        mv_data;
    /** \brief MBCode data output of MB ENC.
     * Each 16x16 block has one MB Code data with layout VAEncFEIModeBufferH264Intel
     * Buffer size shall not be less than the number of 16x16 blocks multiplied by 
     * sizeof(VAEncFEIModeBufferH264Intel). 
     */
    VABufferID        mb_code_data;
    /** \brief Qp input buffer. It is valid only when mb_qp is set to 1. 
     * The data in this buffer correspond to the input source. 
     * One Qp per 16x16 block in raster scan order, each Qp is a signed char (8-bit) value. 
     **/
    VABufferID        qp;
    /** \brief MV predictor. It is valid only when mv_predictor_enable is set to 1. 
     * Each 16x16 block has one or more pair of motion vectors and the corresponding 
     * reference indexes as defined by VAEncMVPredictorBufferH264. 16x16 block is in raster scan order. 
     * Buffer size shall not be less than the number of 16x16 blocks multiplied by 
     * sizeof(VAEncMVPredictorBufferH264). */
    VABufferID        mv_predictor;

    /** \brief number of MV predictors. It must not be greater than maximum supported MV predictor. */
    unsigned int      num_mv_predictors_l0      : 16;
    unsigned int      num_mv_predictors_l1      : 16;

    /** \brief control parameters */
    unsigned int      search_path               : 8;     
    unsigned int      len_sp                    : 8;     
    unsigned int      reserved0                 : 16;

    unsigned int      sub_mb_part_mask          : 7;     
    unsigned int      intra_part_mask           : 5;     
    unsigned int      multi_pred_l0             : 1;     
    unsigned int      multi_pred_l1             : 1;     
    unsigned int      sub_pel_mode              : 2;     
    unsigned int      inter_sad                 : 2;
    unsigned int      intra_sad                 : 2;     
    unsigned int      distortion_type           : 1;     
    unsigned int      repartition_check_enable  : 1;     
    unsigned int      adaptive_search           : 1;     
    unsigned int      mv_predictor_enable       : 1;     
    unsigned int      mb_qp                     : 1;     
    unsigned int      mb_input                  : 1;     
    unsigned int      mb_size_ctrl              : 1;
    unsigned int      colocated_mb_distortion   : 1;
    unsigned int      reserved1                 : 4;

    unsigned int      ref_width                 : 8;     
    unsigned int      ref_height                : 8;
    unsigned int      search_window             : 4;     
    unsigned int      reserved2                 : 12;
    
    unsigned int      max_frame_size; 
    unsigned int      num_passes;     //number of QPs
    unsigned char     *delta_qp;      //list of detla QPs
    unsigned int      reserved3;      //add reserved , the structure size will be 72, 8byte alignment
} VAEncMiscParameterFEIFrameControlH264;


/** \brief FEI MB level control data structure */
typedef struct _VAEncFEIMBControlH264 {
    /** \brief when set, correposndent MB is coded as intra */
    unsigned int force_to_intra                : 1;     
    /** \brief when set, correposndent MB is coded as skip */
    unsigned int force_to_skip                 : 1;     
    unsigned int force_to_nonskip              : 1;     
    unsigned int enable_direct_bias_adjustment : 1;
    unsigned int enable_motion_bias_adjustment : 1;
    unsigned int ext_mv_cost_scaling_factor    : 3;
    unsigned int reserved0                     : 24;


    unsigned int reserved1;    

    unsigned int reserved2;    

    /** \brief when mb_size_ctrl is set, size here is used to budget accumulatively. Set to 0xFF if don't care. */
    unsigned int reserved3                     : 16;    
    unsigned int target_size_in_word           : 8;     
    unsigned int max_size_in_word              : 8;     
} VAEncFEIMBControlH264;


/** \brief Application can use this definition as reference to allocate the buffer 
 * based on MaxNumPredictor returned from attribute VAConfigAttribEncMVPredictorsIntel query. 
 **/
typedef struct _VAEncMVPredictorH264 {
    /** \brief Reference index corresponding to the entry of RefPicList0 & RefPicList1 in VAEncSliceParameterBufferH264. 
     * Note that RefPicList0 & RefPicList1 needs to be the same for all slices. 
     * ref_idx_l0_x : index to RefPicList0; ref_idx_l1_x : index to RefPicList1; x : 0 - MaxNumPredictor. 
     **/
    unsigned int ref_idx_l0_0 : 4;     
    unsigned int ref_idx_l1_0 : 4;     
    unsigned int ref_idx_l0_1 : 4;     
    unsigned int ref_idx_l1_1 : 4;     
    unsigned int ref_idx_l0_2 : 4;     
    unsigned int ref_idx_l1_2 : 4;     
    unsigned int ref_idx_l0_3 : 4;     
    unsigned int ref_idx_l1_3 : 4;     
    unsigned int reserved;
    /** \brief MV. MaxNumPredictor must be the returned value from attribute VAConfigAttribEncMVPredictors query. 
     * Even application doesn't use the maximum predictors, the VAEncMVPredictorH264 structure size 
     * has to be defined as maximum so each MB can be at a fixed location. 
     * Note that 0x8000 must be used for correspondent intra block. 
     **/
    struct _mv
    {
    /** \brief Motion vector corresponding to ref0x_index. 
     * mv0[0] is horizontal motion vector and mv0[1] is vertical motion vector. */
        short    mv0[2];
    /** \brief Motion vector corresponding to ref1x_index. 
     * mv1[0] is horizontal motion vector and mv1[1] is vertical motion vector. */
        short    mv1[2];
    } mv[4]; /* MaxNumPredictor is 4 */
} VAEncMVPredictorH264;


/** \brief FEI output */
/**
 * Motion vector output is per 4x4 block. For each 4x4 block there is a pair of Motion Vectors 
 * for RefPicList0 and RefPicList1 and each MV is 4 bytes including horizontal and vertical directions. 
 * Depending on Subblock partition, for the shape that is not 4x4, the MV is replicated 
 * so each 4x4 block has a pair of Motion Vectors. The 16x16 block has 32 Motion Vectors (128 bytes). 
 * 0x8000 is used for correspondent intra block. The 16x16 block is in raster scan order, 
 * within the 16x16 block, each 4x4 block MV is ordered as below in memory. 
 * The buffer size shall be greater than or equal to the number of 16x16 blocks multiplied by 128 bytes. 
 * Note that, when separate ENC and PAK is enabled, the exact layout of this buffer is needed for PAK input. 
 * App can reuse this buffer, or copy to a different buffer as PAK input. 
 *                      16x16 Block        
 *        -----------------------------------------
 *        |    1    |    2    |    5    |    6    |
 *        -----------------------------------------
 *        |    3    |    4    |    7    |    8    |
 *        -----------------------------------------
 *        |    9    |    10   |    13   |    14   |
 *        -----------------------------------------
 *        |    11   |    12   |    15   |    16   |
 *        -----------------------------------------
 **/

/** \brief VAEncFEIModeBufferIntel defines the data structure for VAEncFEIModeBufferTypeIntel per 16x16 MB block. 
 * The 16x16 block is in raster scan order. Buffer size shall not be less than the number of 16x16 blocks 
 * multiplied by sizeof(VAEncFEIModeBufferH264Intel). Note that, when separate ENC and PAK is enabled, 
 * the exact layout of this buffer is needed for PAK input. App can reuse this buffer, 
 * or copy to a different buffer as PAK input, reserved elements must not be modified when used as PAK input. 
 **/
typedef struct _VAEncFEIMBCodeBufferH264 {
    unsigned int    reserved1[3];

    //DWORD  3
    unsigned int    inter_mb_mode            : 2;
    unsigned int    mb_skip_flag             : 1;
    unsigned int    reserved00               : 1; 
    unsigned int    intra_mb_mode            : 2;
    unsigned int    reserved01               : 1; 
    unsigned int    field_mb_polarity_flag   : 1;
    unsigned int    mb_type                  : 5;
    unsigned int    intra_mb_flag            : 1;
    unsigned int    field_mb_flag            : 1;
    unsigned int    transform8x8_flag        : 1;
    unsigned int    reserved02               : 1; 
    unsigned int    dc_block_coded_cr_flag   : 1;
    unsigned int    dc_block_coded_cb_flag   : 1;
    unsigned int    dc_block_coded_y_flag    : 1;
    unsigned int    reserved03               : 12; 

    //DWORD 4
    unsigned int    horz_origin              : 8;
    unsigned int    vert_origin              : 8;
    unsigned int    cbp_y                    : 16;

    //DWORD 5
    unsigned int    cbp_cb                   : 16;
    unsigned int    cbp_cr                   : 16;

    //DWORD 6
    unsigned int    qp_prime_y               : 8;
    unsigned int    reserved30               : 17; 
    unsigned int    mb_skip_conv_disable     : 1;
    unsigned int    is_last_mb               : 1;
    unsigned int    enable_coefficient_clamp : 1; 
    unsigned int    direct8x8_pattern        : 4;

    //DWORD 7 8 and 9
    union 
    {
        /* Intra MBs */
        struct 
        {
            unsigned int   luma_intra_pred_modes0 : 16;
            unsigned int   luma_intra_pred_modes1 : 16;

            unsigned int   luma_intra_pred_modes2 : 16;
            unsigned int   luma_intra_pred_modes3 : 16;

            unsigned int   chroma_intra_pred_mode : 2;
            unsigned int   intra_pred_avail_flag  : 5;
            unsigned int   intra_pred_avail_flagF : 1;
            unsigned int   reserved60             : 24; 
        } intra_mb;

        /* Inter MBs */
        struct 
        {
            unsigned int   sub_mb_shapes          : 8; 
            unsigned int   sub_mb_pred_modes      : 8;
            unsigned int   reserved40             : 16; 
 
            unsigned int   ref_idx_l0_0           : 8; 
            unsigned int   ref_idx_l0_1           : 8; 
            unsigned int   ref_idx_l0_2           : 8; 
            unsigned int   ref_idx_l0_3           : 8; 

            unsigned int   ref_idx_l1_0           : 8; 
            unsigned int   ref_idx_l1_1           : 8; 
            unsigned int   ref_idx_l1_2           : 8; 
            unsigned int   ref_idx_l1_3           : 8; 
        } inter_mb;
    } mb_mode;

    //DWORD 10
    unsigned int   reserved70                : 16; 
    unsigned int   target_size_in_word       : 8;
    unsigned int   max_size_in_word          : 8; 

    //DWORD 11 12 13 14 and 15
    unsigned int   reserved2[4];
    unsigned int   batchbuffer_end;   // in cmodel, it is: is_last_mb ? BATCHBUFFER_END : 0;
} VAEncFEIMBCodeBufferH264;        // 16 DWORDs or 64 bytes

/** \brief VAEncFEIDistortionBufferIntel defines the data structure for 
 * VAEncFEIDistortionBufferType per 16x16 MB block. The 16x16 block is in raster scan order. 
 * Buffer size shall not be less than the number of 16x16 blocks multiple by sizeof(VAEncFEIDistortionBufferIntel). 
 **/
typedef struct _VAEncFEIDistortionH264 {
    /** \brief Inter-prediction-distortion associated with motion vector i (co-located with subblock_4x4_i). 
     * Its meaning is determined by sub-shape. It must be zero if the corresponding sub-shape is not chosen. 
     **/
    unsigned short  inter_distortion[16];
    unsigned int    best_inter_distortion     : 16;
    unsigned int    best_intra_distortion     : 16;
    unsigned int    colocated_mb_distortion   : 16;
    unsigned int    reserved                  : 16;
    unsigned int    reserved1[2];
} VAEncFEIDistortionH264;    // size is 48 bytes


//va_intel_statastics.h

typedef struct _VAPictureFEI
{
    VASurfaceID picture_id;
    /* 
     * see flags below. 
     */
    unsigned int flags;
} VAPictureFEI;
/* flags in VAPictureFEI could be one of the following */
#define VA_PICTURE_STATS_INVALID                   0xffffffff
#define VA_PICTURE_STATS_PROGRESSIVE               0x00000000
#define VA_PICTURE_STATS_TOP_FIELD                 0x00000001
#define VA_PICTURE_STATS_BOTTOM_FIELD              0x00000002
#define VA_PICTURE_STATS_CONTENT_UPDATED           0x00000010

/** \brief Motion Vector and Statistics frame level controls.
 * common part VAStatsStatisticsParameterBufferType for a MB or CTB
 **/
typedef struct _VAStatsStatisticsParameter
{
    /** \brief Source surface ID.  */
    VAPictureFEI    input;

    /** \brief Past reference surface ID pointer.  */
    VAPictureFEI    *past_references;

    /** \brief Past reference surface number  */
    uint32_t        num_past_references;

    /** \brief Statistics output for past reference surface.
     * Only enabling statistics output for past reference picture when *past_ref_stat_buf is a valid
     * VABufferID, it is needed in case app wants statistics data of both reference and current pictures
     * in very special use cases for better performance.
     * The output layout is defined by VAStatsStatisticsBufferType(for progressive and top field of
     * interlaced case) and VAStatsStatisticsBottomFieldBufferType(only for interlaced case), only
     * pixel_average_16x16/pixel_average_8x8 and variance_16x16/variance_8x8 data are valid.
     **/
    VABufferID      *past_ref_stat_buf;

    /** \brief Future reference surface ID pointer.  */
    VAPictureFEI    *future_references;

    /** \brief Future reference surface number  */
    uint32_t        num_future_references;

    /** \brief Statistics output for future reference surface.
     * Only enabling statistics output for future reference picture when *past_ref_stat_buf is a valid
     * VABufferID, it is needed in case app wants statistics data of both reference and current pictures
     * in very special use cases for better performance.
     * The output layout is defined by VAStatsStatisticsBufferType(for progressive and top field of
     * interlaced case) and VAStatsStatisticsBottomFieldBufferType(only for interlaced case), only
     * pixel_average_16x16/pixel_average_8x8 and variance_16x16/variance_8x8 data are valid.
     **/
    VABufferID      *future_ref_stat_buf;

    /** \brief ID of the output buffer.
     * The number of outputs is determined by below DisableMVOutput and DisableStatisticsOutput.
     * The output layout is defined by VAStatsMVBufferType, VAStatsStatisticsBufferType(for progressive and
     * top field of interlaced case) and VAStatsStatisticsBottomFieldBufferType(only for interlaced case).
     **/
    VABufferID      *outputs;

    /** \brief MV predictor. It is valid only when mv_predictor_ctrl is not 0.
     * Each block has a pair of Motion Vectors, one for past and one for future reference
     * as defined by VAMotionVector. The block is in raster scan order.
     * Buffer size shall not be less than the number of blocks multiplied by sizeof(VAMotionVector).
     **/
    VABufferID      mv_predictor;

    /** \brief QP input buffer. It is valid only when mb_qp is set to 1.
     * The data in this buffer correspond to the input source.
     * One QP per MB or CTB block in raster scan order, each QP is a signed char (8-bit) value.
     **/
    VABufferID      qp;
} VAStatsStatisticsParameter;

typedef struct _VAStatsStatisticsParameterH264
{
   VAStatsStatisticsParameter stats_params;

    uint32_t    frame_qp                    : 8;
    /** \brief maximum number of Search Units, valid range is [1, 63] */
    uint32_t    len_sp                      : 8;
    /** \brief motion search method definition
     * 0: default value, diamond search
     * 1: full search
     * 2: diamond search
     **/
    uint32_t    search_path                 : 8;
    uint32_t    reserved0                   : 8;

    uint32_t    sub_mb_part_mask            : 7;
    uint32_t    sub_pel_mode                : 2;
    uint32_t    inter_sad                   : 2;
    uint32_t    intra_sad                   : 2;
    uint32_t    adaptive_search             : 1;
    /** \brief indicate if future or/and past MV in mv_predictor buffer is valid.
     * 0: MV predictor disabled
     * 1: MV predictor enabled for past reference
     * 2: MV predictor enabled for future reference
     * 3: MV predictor enabled for both past and future references
     **/
    uint32_t    mv_predictor_ctrl           : 3;
    uint32_t    mb_qp                       : 1;
    uint32_t    ft_enable                   : 1;
    /** \brief luma intra mode partition mask
     * xxxx1: luma_intra_16x16 disabled
     * xxx1x: luma_intra_8x8 disabled
     * xx1xx: luma_intra_4x4 disabled
     * xx111: intra prediction is disabled
     **/
    uint32_t    intra_part_mask             : 5;
    uint32_t    reserved1                   : 8;

    /** \brief motion search window(ref_width * ref_height) */
    uint32_t    ref_width                   : 8;
    uint32_t    ref_height                  : 8;
    /** \brief predefined motion search windows. If selected, len_sp, window(ref_width * ref_eight)
     * and search_path setting are ignored.
     * 0: not use predefined search window
     * 1: Tiny, len_sp=4, 24x24 window and diamond search
     * 2: Small, len_sp=9, 28x28 window and diamond search
     * 3: Diamond, len_sp=16, 48x40 window and diamond search
     * 4: Large Diamond, len_sp=32, 48x40 window and diamond search
     * 5: Exhaustive, len_sp=48, 48x40 window and full search
     * 6: Extend Diamond, len_sp=16, 64x40 window and diamond search
     * 7: Extend Large Diamond, len_sp=32, 64x40 window and diamond search
     * 8: Extend Exhaustive, len_sp=48, 64x40 window and full search
     **/
    uint32_t    search_window               : 4;
    uint32_t    reserved2                   : 12;

    /** \brief MVOutput. When set to 1, MV output is NOT provided */
    uint32_t    disable_mv_output           : 1;
    /** \brief StatisticsOutput. When set to 1, Statistics output is NOT provided. */
    uint32_t    disable_statistics_output   : 1;
    /** \brief block 8x8 data enabling in statistics output */
    uint32_t    enable_8x8_statistics       : 1;
    uint32_t    reserved3                   : 29;
    uint32_t    reserved4[2];
} VAStatsStatisticsParameterH264;

#define VAEncFEICTBCmdBufferType                   53
#define VAEncFEICURecordBufferType                 54

#define VAConfigAttribFEICTBCmdSize                350   //VAConfigAttribTypeMax is 35 in current code, will back to be 35 after upstream
#define VAConfigAttribFEICuRecordSize              36

/** \brief FEI frame level control buffer for Hevc */
typedef struct _VAEncMiscParameterFEIFrameControlHEVC
{
    uint32_t      function; /* one of the VAConfigAttribFEIFunctionType values */
    /** \brief CTB control input buffer. It is valid only when per_ctb_input
     * is set to 1. The data in this buffer correspond to the input source. CTB is in raster scan order,
     * each CTB control data structure is defined by VAEncFEICTBControlHevc.
     * Buffer size shall not be less than the number of CTBs multiplied by
     * sizeof(VAEncFEICTBControlHevc).
     */
    VABufferID    ctb_ctrl;
    /** \brief CTB cmd per CTB data output of ENC.
     * Each CTB block has one CTB cmd data with layout VAEncFEICTBCmdHevc
     * Buffer size shall not be less than the number of CTBs multiplied by
     * sizeof(VAEncFEICTBCmdHevc).
     */
    VABufferID    ctb_cmd;
    /** \brief CU record data output of ENC.
     * Each CTB block has one CU record data with layout VAEncFEICURecordLongHevc for SKL and VAEncFEICURecordHevc for ICL+
     * Buffer size shall not be less than the number of CTB blocks multiplied by
     * sizeof(VAEncFEICURecordLongHevc) or sizeof(VAEncFEICURecordHevc).
     */
    VABufferID    cu_record;
    /** \brief distortion output of ENC or ENC_PAK.
     * Each CTB has one distortion data with VAEncFEIDistortionBufferHevc
     * Buffer size shall not be less than the number of CTBs multiplied by
     * sizeof(VAEncFEIDistortionBufferHevc).
     */
    VABufferID    distortion;
    /** \brief Qp input buffer. It is valid only when per_block_qp is set to 1.
     * The data in this buffer correspond to the input source.
     * One Qp per block block is in raster scan order, each Qp is a signed char (8-bit) value.
     **/
    VABufferID    qp;
    /** \brief MV predictor. It is valid only when mv_predictor_input is set to non-zero.
     * Each CTB block has one or more pair of motion vectors and the corresponding
     * reference indexes as defined by VAEncFEIMVPredictorHevc. 32x32 block is in raster scan order.
     * Buffer size shall not be less than the number of 16x16 blocks multiplied by
     * sizeof(VAEncFEIMVPredictorHevc). */
    VABufferID    mv_predictor;

    /** \brief number of MV predictors L0 and L1. the maximum number of motion vector predictor for a 16x16, 32x32 or
     * 64x64 block is four, it must not be greater than maximum supported MV predictor,
     * */
    uint32_t      num_mv_predictors_l0      : 16;
    uint32_t      num_mv_predictors_l1      : 16;

    /** \brief control parameters */
    uint32_t      search_path               : 8;
    uint32_t      len_sp                    : 8;
    uint32_t      reserved0                 : 16;

    /** \brief multi pred l0/1
     * 0000: no internal MV predictor will be used
     * 0001: spatial MV predictors
     * 0010: temporal MV predictors (ICL+)
     * 0100/1000: Reserved
     **/
    uint32_t      multi_pred_l0             : 4;
    uint32_t      multi_pred_l1             : 4;
    uint32_t      sub_pel_mode              : 2;
    uint32_t      adaptive_search           : 1;
    /** \brief mv_predictor_input
     * 000: MV predictor disabled
     * 001: MV predictor enabled per 16x16 block
     * 010: MV predictor enabled per 32x32 block
     * 011: MV predictor enabled per 64x64 block, valid only when 64x64 LCU is supported on ICL+.
     * 111: MV predictor enabled, block size can vary and is determined by BlockSize in motion vector predictor buffer
     * 100/101/110: Reserved
     **/
    uint32_t      mv_predictor_input        : 3;
    /** \brief enables per CTB qp for Gen9 or per CU qp for Gen11 */
    uint32_t      per_block_qp              : 1;
    uint32_t      per_ctb_input             : 1;
    uint32_t      colocated_ctb_distortion  : 1;
    /** brief specifies whether this CTB should be forced to split to remove Inter big LCU: do not check Inter 32x32
     * PUs. Every 32x32 LCU is split at least once. It can be used to improved performance.
     * 0: ENC determined block type
     * 1: Force to split
     **/
    uint32_t      force_lcu_split           : 1;
    /** \brief enables CU64x64 check */
    uint32_t      enable_cu64_check         : 1;    // for ICL+ only
    /** \brief enables CU64x64 asymmetric motion partition check */
    uint32_t      enable_cu64_amp_check     : 1;    // for ICL+ only
    /** \brief specifies if check the 64x64 merge candidate
     * 0: after skip check, also run merge for TU1
     * 1: only skip check for 64x64 for TU4
     Default: 0. This field is used by LCU64 bi-directional.
     **/
    uint32_t      cu64_skip_check_only      : 1;    // for ICL+ only
    uint32_t      reserved1                 : 11;

    uint32_t      ref_width                 : 8;
    uint32_t      ref_height                : 8;
    /** \brief search window similar for AVC gen9, value 9/10/11 are for 16/32/48 64x64 */
    uint32_t      search_window             : 8;    // similar for AVC gen9, value 9/10/11 are for 16/32/48 64x64
    /** \brief number of internal MV predictors for IME searches, value range is [1..6] */
    uint32_t      max_num_ime_search_center : 3;    // for ICL+ only
    uint32_t      reserved2                 : 5;

    /** \brief specifies number of splits that encoder could be run concurrently
     * 1: level 1, default value
     * 2: level 2
     * 4: level 3
     **/
    uint32_t      num_concurrent_enc_frame_partition : 8;
    uint32_t      reserved3                 : 24;

    /** \brief max frame size control with multi passes QP setting */
    uint32_t      max_frame_size;
    /** \brief number of passes, every pass has different QP */
    uint32_t      num_passes;
    /** \brief delta QP list for every pass */
    uint8_t       *delta_qp;

    uint32_t      reserved4[2];
} VAEncMiscParameterFEIFrameControlHEVC;

/** \brief Application can use this definition as reference to allocate the buffer
 * based on MaxNumPredictor returned from attribute VAConfigAttribFEIMVPredictors query.
 * this buffer allocation is always based on 16x16 block even block size is indicated as 32x32 or 64x64, and buffer
 * layout is always in 32x32 block raster scan order even block size is 16x16 or 64x64. If 32x32 block size is set,
 * only the data in the first 16x16 block (block 0) is used for 32x32 block. If 64x64 block size is set (valid only
 * when 64x64 LCU is supported), MV layout is still in 32x32 raster scan order, the same as 32x32 and the first 16x16
 * block within each 32x32 block needs to have intended MV data (four 32x32 blocks will have the same MV data in the
 * correspondent first 16x16 block). Data structure for each 16x16 block is defined as below (same as AVC except
 * BlockSize/Reserved bits).
 **/
typedef struct _VAEncFEIMVPredictorHevc
{
    /** \brief Feference index corresponding to the entry of RefPicList0 & RefPicList1 in slice header (final reference
     * list). Note that RefPicList0 & RefPicList1 needs to be the same for all slices.
     * Ref0xIndex – RefPicList0; Ref1xIndex – RefPicList1; x – 0 ~ MaxNumPredictor */
    struct {
        uint8_t   ref_idx_l0    : 4;
        uint8_t   ref_idx_l1    : 4;
    } ref_idx[4]; /* index is predictor number */
    /** \brief Valid only when MVPredictor is set to 011 for Hevc. Only valid in the first 16x16 block.
     * 00: MV predictor disabled for this 32x32 block
     * 01: MV predictor enabled per 16x16 block for this 32x32 block
     * 10: MV predictor enabled per 32x32 block, the rest of 16x16 block data within this 32x32 block are ignored
     * 11: Reserved */
    uint32_t block_size         : 2;
    uint32_t reserved           :30;

    VAMotionVector mv[4]; /* MaxNumPredictor is 4 */
} VAEncFEIMVPredictorHevc;    //40 bytes

/** \brief FEI CTB level control data structure */
typedef struct _VAEncFEICTBControlHevc
{
    // DWORD 0
    uint32_t    force_to_intra      : 1;
    uint32_t    force_to_inter      : 1;    // for ICL+ only
    uint32_t    force_to_skip       : 1;    // for ICL+ only
    /** \brief force all coeff to zero */
    uint32_t    force_to_zero_coeff : 1;    // for ICL+ only
    uint32_t    reserved0           : 28;
    // DWORD 1
    uint32_t    reserved1;
    // DWORD 2
    uint32_t    reserved2;
    // DWORD 3
    uint32_t    reserved3;
} VAEncFEICTBControlHevc;

/** \brief VAEncFEIDistortionHevc defines the data structure for VAEncFEIDistortionBufferType per CTB block.
 * It is output buffer of ENC and ENC_PAK modes, The CTB block is in raster scan order.
 * Buffer size shall not be less than the number of CTB blocks multiple by sizeof(VAEncFEIDistortionHevc).
 **/
typedef struct _VAEncFEIDistortionHevc
{
    uint32_t    best_distortion;
    uint32_t    colocated_ctb_distortion;
} VAEncFEIDistortionHevc;   // 8 bytes

#ifdef __cplusplus
extern "C" {
#endif

/** \brief Rate control parameters */
typedef struct _VAEncMiscParameterRateControlPrivate
{
    /* this is the maximum bit-rate to be constrained by the rate control implementation */
    unsigned int bits_per_second;
    /* this is the bit-rate the rate control is targeting, as a percentage of the maximum
     * bit-rate for example if target_percentage is 95 then the rate control will target
     * a bit-rate that is 95% of the maximum bit-rate
     */
    unsigned int target_percentage;
    /* windows size in milliseconds. For example if this is set to 500,
     * then the rate control will guarantee the target bit-rate over a 500 ms window
     */
    unsigned int window_size;
    /* initial_qp: initial QP for the first I frames
     * min_qp/max_qp: minimal and maximum QP frames
     * If set them to 0, encoder chooses the best QP according to rate control
     */
    unsigned int initial_qp;
    unsigned int min_qp;
    unsigned int max_qp;
    unsigned int basic_unit_size;
    union
    {
        struct
        {
            unsigned int reset : 1;
            unsigned int disable_frame_skip : 1; /* Disable frame skip in rate control mode */
            unsigned int disable_bit_stuffing : 1; /* Disable bit stuffing in rate control mode */
            unsigned int mb_rate_control : 4; /* Control VA_RC_MB 0: default, 1: enable, 2: disable, other: reserved*/
            /*
             * The temporal layer that the rate control parameters are specified for.
             */ 
            unsigned int temporal_id : 8; 
            unsigned int cfs_I_frames : 1; /* I frame also follows CFS */
            unsigned int enable_parallel_brc    : 1;
            unsigned int enable_dynamic_scaling : 1;
            /** \brief Frame Tolerance Mode
             *  Indicates the tolerance the application has to variations in the frame size.  
             *  For example, wireless display scenarios may require very steady bit rate to 
             *  reduce buffering time. It affects the rate control algorithm used, 
             *  but may or may not have an effect based on the combination of other BRC 
             *  parameters.  Only valid when the driver reports support for 
             *  #VAConfigAttribFrameSizeToleranceSupport.
             *
             *  equals 0    -- normal mode;
             *  equals 1    -- maps to sliding window;
             *  equals 2    -- maps to low delay mode;
             *  other       -- invalid.
             */
            unsigned int frame_tolerance_mode   : 2;
            /** \brief Enable fine-tuned qp adjustment at CQP mode. 
             *  With BRC modes, this flag should be set to 0.
             */
            unsigned int qp_adjustment          : 1;
            unsigned int reserved               : 11;
        } bits;
        unsigned int value;
    } rc_flags;
    unsigned int ICQ_quality_factor; /* Initial ICQ quality factor: 1-51. */
} VAEncMiscParameterRateControlPrivate;
typedef VAGenericID VAMFContextID;

#define VPG_EXT_VA_CREATE_MFECONTEXT  "DdiMedia_CreateMfeContext"
typedef VAStatus (*vaExtCreateMfeContext)(
    VADisplay           dpy,
    VAMFContextID      *mfe_context
);

#define VPG_EXT_VA_ADD_CONTEXT  "DdiMedia_AddContext"
typedef VAStatus (*vaExtAddContext)(
    VADisplay           dpy,
    VAContextID         context,
    VAMFContextID      mfe_context
);

#define VPG_EXT_VA_RELEASE_CONTEXT  "DdiMedia_ReleaseContext"
typedef VAStatus (*vaExtReleaseContext)(
    VADisplay           dpy,
    VAContextID         context,
    VAMFContextID      mfe_context
);

#define VPG_EXT_VA_MFE_SUBMIT  "DdiMedia_MfeSubmit"
typedef VAStatus (*vaExMfeSubmit)(
    VADisplay           dpy,
    VAMFContextID      mfe_context,
    VAContextID         *contexts,
    int                 num_contexts
);

/**@}*/

#ifdef __cplusplus
}
#endif

#ifndef VA_CODED_BUF_STATUS_BAD_BITSTREAM
#define VA_CODED_BUF_STATUS_BAD_BITSTREAM  0x8000
#endif

// Force MB's to be non skip for both ENC and PAK.  The width of the MB map
// Surface is (width of the Picture in MB unit) * 1 byte, multiple of 64 bytes.
/// The height is (height of the picture in MB unit). The picture is either
// frame or non-interleaved top or bottom field.  If the application provides this
// surface, it will override the "skipCheckDisable" setting in VAEncMiscParameterPrivate.
#define VAEncMacroblockDisableSkipMapBufferType     -7


#define VAConfigAttribCustomRoundingControl 0xfffffffe

/** \brief Sequence display extension data structure used for */
/*         VAEncMiscParameterTypeExtensionData buffer type. */
/*         the element definition in this structure has 1 : 1 correspondence */
/*         with the same element defined in sequence_display_extension() */
/*         from mpeg2 spec */
typedef struct _VAEncMiscParameterExtensionDataSeqDisplayMPEG2
{
    unsigned char extension_start_code_identifier;
    unsigned char video_format;
    unsigned char colour_description;
    unsigned char colour_primaries;
    unsigned char transfer_characteristics;
    unsigned char matrix_coefficients;
    unsigned short display_horizontal_size;
    unsigned short display_vertical_size;
} VAEncMiscParameterExtensionDataSeqDisplayMPEG2;
#define VAEncMiscParameterTypeExtensionData        -8
// Decode Streamout buffer type
#define VADecodeStreamoutBufferType     -10
 
#endif
