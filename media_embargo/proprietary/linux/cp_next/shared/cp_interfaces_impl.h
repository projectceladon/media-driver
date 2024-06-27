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
//! \brief    Defines implementation class for CpInterfaces and CpInterfacesNext
//!

#ifndef _CP_INTERFACES_IMPL_H_
#define _CP_INTERFACES_IMPL_H_

#include <stdint.h>
#include "cp_interfaces.h"

//!
//! \class  CpInterfacesImpl
//! \brief  CpInterfaces implementation
//!
class CpInterfacesImpl : public CpInterfaces
{
public:
    //!
    //! \brief Constructor
    //!
    CpInterfacesImpl() {}

    //!
    //! \brief Destructor
    //!
    virtual ~CpInterfacesImpl() {}

    CodechalSecureDecodeInterface *Create_SecureDecodeInterface(
        CodechalSetting *      codechalSettings,
        CodechalHwInterface *  hwInterfaceInput) override;
    void Delete_SecureDecodeInterface(CodechalSecureDecodeInterface *pInterface) override;

    MhwCpInterface* Create_MhwCpInterface(PMOS_INTERFACE osInterface) override;
    void Delete_MhwCpInterface(MhwCpInterface *pInterface) override;

    MosCpInterface* Create_MosCpInterface(void* pvOsInterface) override;
    void Delete_MosCpInterface(MosCpInterface* pInterface) override;

    DdiCpInterface* Create_DdiCpInterface(MOS_CONTEXT& mosCtx) override;
    void Delete_DdiCpInterface(DdiCpInterface* pInterface) override;

    MediaLibvaCapsCpInterface* Create_MediaLibvaCapsCpInterface(
        DDI_MEDIA_CONTEXT *mediaCtx,
        MediaLibvaCaps *mediaCaps) override;
    void Delete_MediaLibvaCapsCpInterface(MediaLibvaCapsCpInterface* pInterface) override;

    DecodeCpInterface *Create_DecodeCpInterface(
        CodechalSetting *    codechalSettings,
        MhwCpInterface  *cpInterface,
        PMOS_INTERFACE   osInterface) override;
    void Delete_DecodeCpInterface(DecodeCpInterface *pInterface) override;

    MEDIA_CLASS_DEFINE_END(CpInterfacesImpl)
};

#endif /*  _CP_INTERFACES_IMPL_H_ */
