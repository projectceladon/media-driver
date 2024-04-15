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
//! \file     ddi_cp_caps.h
//! \brief    cp caps head file
//!

#ifndef __DDI_CP_CAPS_H__
#define __DDI_CP_CAPS_H__

#include "ddi_cp_caps_interface.h"
#include "ddi_cp_caps_data.h"
#include "media_capstable_specific.h"

#define REMOVE_CONFIG_ID_CP_OFFSET(x) (REMOVE_CONFIG_ID_OFFSET(x) - DDI_CP_GEN_CONFIG_ATTRIBUTES_BASE)
#define ADD_CONFIG_ID_CP_OFFSET(x) (ADD_CONFIG_ID_OFFSET(x) + DDI_CP_GEN_CONFIG_ATTRIBUTES_BASE)
class DdiCpCaps : public DdiCpCapsInterface
{
public:
    DdiCpCaps() {}

    virtual ~DdiCpCaps() {}

    VAStatus InitProfileMap(DDI_MEDIA_CONTEXT *mediaCtx, ProfileMap *profileMap) override;

    bool IsCpConfigId(VAConfigID configId) override;

    uint32_t GetCpConfigId(VAConfigID configId) override;
    
private:
    VAStatus UpdateSecureDecodeProfile(ProfileMap *profileMap);
    bool m_isEntryptSupported = false;

MEDIA_CLASS_DEFINE_END(DdiCpCaps)
};

#endif //__DDI_CP_CAPS_H__