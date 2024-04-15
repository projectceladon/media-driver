/*
* Copyright (c) 2019-2020, Intel Corporation
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
//! \file     codechal_setting_ext.h
//! \brief    Defines class CodechalSetting
//!
#ifndef __CODECHAL_SETTING_EXT_H__
#define __CODECHAL_SETTING_EXT_H__

#include "codechal_setting.h"

typedef struct _MHW_PAVP_ENCRYPTION_PARAMS *PMHW_PAVP_ENCRYPTION_PARAMS;

//!
//! \enum   CodechalDecodePkEncType
//! \brief  enum constant for decode encryption type on Windows porting kit
//!
enum CodechalDecodePkEncType
{
    CodechalDecodePkNone = 0x00,    //!< Clear mode decode
    CodechalDecodePk3Cenc,          //!< HEVC decode PR3.0 and Google AVC widevine on Windows
    CodechalDecodePk4Cenc,          //!< HEVC,AVC and VP9 CENC decode on PR4.0
    CodechalDecodePk4Cbcs,          //!< AVC CBCS decode on PR4.0
};

//!
//! \class  CodechalSettingExt 
//! \brief  Settings used to finalize the creation of the CodecHal device 
//!
class CodechalSettingExt: public CodechalSetting
{
public:
    // Cenc Decode for AVC decode on PR3.0.
    bool isCencInUse = false;           //!< Flag to indicate the PR3.0 DDI is in use (as opposed to Intel defined DDI for HuC-based DRM).

    //!
    //! \brief    Return the indicate if cenc advance is used or not
    //!
    virtual bool CheckCencAdvance();

MEDIA_CLASS_DEFINE_END(CodechalSettingExt)
};

#endif
