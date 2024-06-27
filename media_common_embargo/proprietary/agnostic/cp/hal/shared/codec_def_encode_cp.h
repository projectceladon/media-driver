/*
// Copyright (C) 2017-2022 Intel Corporation
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
//! \file     codec_def_encode_cp.h
//! \brief    Defines decode CP types and macros shared by CodecHal, MHW, and DDI layer for encode.
//! \details  All codec_def_decode may include this file which should not contain any DDI specific code.
//!
#ifndef __CODEC_DEF_ENCODE_CP_H__
#define __CODEC_DEF_ENCODE_CP_H__

typedef enum _CODEC_INPUT_TYPE
{
    ETYPE_DRM_NONE    = 0,
    ETYPE_DRM_SECURE  = 1,
    ETYPE_DRM_UNKNOWN
} CODEC_INPUT_TYPE, ENCODE_INPUT_TYPE;

#endif
