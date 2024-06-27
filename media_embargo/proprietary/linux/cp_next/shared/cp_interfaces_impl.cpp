/*
* Copyright (c) 2020, Intel Corporation
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
//! \file     cp_interfaces_impl.h
//! \brief    The class implementation of CpInterfaces base class.
//!

#include "media_libva_cp.h"
#include "media_libva_caps_cp.h"
#include "codechal_secure_decode.h"
#include "decodecp.h"
#include "cp_interfaces_impl.h"

static bool isRegistered = CpInterfacesFactory::Register<CpInterfacesImpl>(CP_INTERFACE, true);

// embeded this information in binary for that can "grep cp_included" to check if this driver supports CP or not
__attribute__((visibility("default"))) bool cp_included() { return true; }

CodechalSecureDecodeInterface *CpInterfacesImpl::Create_SecureDecodeInterface(
    CodechalSetting *      codechalSettings,
    CodechalHwInterface *  hwInterfaceInput)
{
    return Create_SecureDecode(codechalSettings, hwInterfaceInput);
}

void CpInterfacesImpl::Delete_SecureDecodeInterface(CodechalSecureDecodeInterface *pInterface)
{
    MOS_Delete(pInterface);
}

MhwCpInterface *CpInterfacesImpl::Create_MhwCpInterface(PMOS_INTERFACE osInterface)
{
    return Create_MhwCp(osInterface);
}

void CpInterfacesImpl::Delete_MhwCpInterface(MhwCpInterface *pInterface)
{
    MOS_Delete(pInterface);
}

MosCpInterface *CpInterfacesImpl::Create_MosCpInterface(void* pvOsInterface)
{
    return Create_MosCp(pvOsInterface);
}

void CpInterfacesImpl::Delete_MosCpInterface(MosCpInterface *pInterface)
{
    MOS_Delete(pInterface);
}

DdiCpInterface *CpInterfacesImpl::Create_DdiCpInterface(MOS_CONTEXT& mosCtx)
{
    return Create_DdiCp(&mosCtx);
}

void CpInterfacesImpl::Delete_DdiCpInterface(DdiCpInterface *pInterface)
{
    MOS_Delete(pInterface);
}

MediaLibvaCapsCpInterface *CpInterfacesImpl::Create_MediaLibvaCapsCpInterface(
    DDI_MEDIA_CONTEXT *mediaCtx,
    MediaLibvaCaps *mediaCaps)
{
    return Create_MediaLibvaCapsCp(mediaCtx, mediaCaps);
}

void CpInterfacesImpl::Delete_MediaLibvaCapsCpInterface(MediaLibvaCapsCpInterface *pInterface)
{
    MOS_Delete(pInterface);
}

DecodeCpInterface *CpInterfacesImpl::Create_DecodeCpInterface(
    CodechalSetting *    codechalSettings,
    MhwCpInterface  *cpInterface,
    PMOS_INTERFACE   osInterface)
{
    return Create_DecodeCp(codechalSettings, dynamic_cast<MhwCpBase *>(cpInterface), cpInterface, osInterface);
}

void CpInterfacesImpl::Delete_DecodeCpInterface(DecodeCpInterface *pInterface)
{
    MOS_Delete(pInterface);
}
