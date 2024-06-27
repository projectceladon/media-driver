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
//! \file      cenc_def_decode_vp9.h 
//! \brief     This modules declares structures, arrays and macros common for cenc decode vp9. 
//!

#ifndef __CENC_DEF_DECODE_VP9_H__
#define __CENC_DEF_DECODE_VP9_H__


struct DecryptStatusVp9
{
    uint32_t                      status;         // MUST NOT BE MOVED as every codec assumes Status is first uint32_t in struct! Formatted as CODECHAL_STATUS.

    uint16_t                      statusReportFeedbackNumber;

    uint16_t                      picture_width_minus1;
    uint16_t                      picture_height_minus1;
    uint16_t                      display_width_minus1;
    uint16_t                      display_height_minus1;

    uint8_t                       version;
    uint8_t                       show_existing_frame_flag;
    uint8_t                       frame_type;
    uint8_t                       frame_toshow_slot;
    uint8_t                       show_frame_flag;
    uint8_t                       error_resilient_mode;
    uint8_t                       color_space;
    uint8_t                       subsampling_x;
    uint8_t                       subsampling_y;
    uint8_t                       refresh_frame_flags;
    uint8_t                       display_different_size_flags;
    uint8_t                       frame_parallel_decoding_mode;
};

#endif  // __CENC_DEF_DECODE_VP9_H__