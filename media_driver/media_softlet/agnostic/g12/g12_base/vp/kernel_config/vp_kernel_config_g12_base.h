/*
* Copyright (c) 2021, Intel Corporation
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
#ifndef __VP_KERNEL_CONFIG_G12_BASE_H__
#define __VP_KERNEL_CONFIG_G12_BASE_H__

#include "vp_kernel_config.h"
#include "media_class_trace.h"

namespace vp {

class VpKernelConfigG12_Base : public VpKernelConfig
{
public:
    VpKernelConfigG12_Base();
    virtual ~VpKernelConfigG12_Base();

private:
    void InitKernelParams();
MEDIA_CLASS_DEFINE_END(vp__VpKernelConfigG12_Base)
};
}
#endif // __VP_KERNEL_CONFIG_G12_BASE_H__
