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
//! \file      cp_pavp_cryptosession_va.h
//! \brief     libva(and its extension) crypto session header file
//!

#ifndef __CP_CRYPTOSESSION_VA_H__
#define __CP_CRYPTOSESSION_VA_H__

#include <vector>
#include <sstream>
#include <iostream>
#include "media_ddi_prot_content.h"
#include "cp_pavpdevice_new_sm.h"
#include "media_libva_cp_interface.h"
#include "media_libva_cp.h"

#ifdef VA_DRIVER_VTABLE_PROT_VERSION

#define CRYPTOSESSION_FUNCTION_ENTER    CP_FUNCTION_ENTER(MOS_CP_SUBCOMP_SECURESESSION)
#define CRYPTOSESSION_FUNCTION_EXIT(hr) CP_FUNCTION_EXIT(MOS_CP_SUBCOMP_SECURESESSION, hr)

#define CRYPTOSESSION_ASSERTMESSAGE(_message, ...) \
    CP_ASSERTMESSAGE(MOS_CP_SUBCOMP_SECURESESSION, _message, ##__VA_ARGS__)

#define CRYPTOSESSION_VERBOSEMESSAGE(_message, ...) \
    CP_VERBOSEMESSAGE(MOS_CP_SUBCOMP_SECURESESSION, _message, ##__VA_ARGS__)

#define CRYPTOSESSION_NORMALMESSAGE(_message, ...) \
    CP_NORMALMESSAGE(MOS_CP_SUBCOMP_SECURESESSION, _message, ##__VA_ARGS__)

#define CRYPTOSESSION_ASSERT(_expr) CP_ASSERT(MOS_CP_SUBCOMP_SECURESESSION, _expr)

// If pointer is nullptr, print the error message and return the specified value.
#define CRYPTOSESSION_CHK_NULL(_ptr, _str, _ret)                            \
    CRYPTOSESSION_CHK_CONDITION((nullptr == (_ptr)), _str, _ret)

// Check the condition, if true, print the error message
// and return the specified value, do nothing otherwise.
#define CRYPTOSESSION_CHK_CONDITION(condition, _str, _ret)                  \
    if (condition) {                                                        \
        CRYPTOSESSION_ASSERTMESSAGE(_str);                                  \
        return _ret;                                                        \
    }

class CPavpCryptoSessionVA : public CPavpDeviceNewSM
{
public:

    //!
    //! \brief    Class constructor
    //! \details  The constructor is minimal. All major initialization is done in the Init() function.
    //!
    //! \param    [in] medix_context
    //!           Pointer to DDI_MEDIA_CONTEXT
    //! \param    [in] attr
    //!           DDI_CP_CONFIG_ATTR
    //!
    CPavpCryptoSessionVA(PDDI_CP_CONTEXT cp_context, DDI_CP_CONFIG_ATTR attr);

    //!
    //! \brief    Class destructor
    //! \details  The destructor must be virtual, because a virtual table is created for this class instances.
    //!           If not, a derived class destructor would not be called when calling the base class destructor.
    //!
    virtual ~CPavpCryptoSessionVA();

    //!
    //! \brief    Setup crypto session
    //! \details  It sends needed buffers by the process (encode/decode/vp) to the driver
    //!
    //! \param    [in] context
    //!           Pointer to MOS_CONTEXT
    //!
    //! \return   VAStatus
    //!           VA_STATUS_SUCCESS if success, else fail reason
    //!
    VAStatus Setup(PAVP_DEVICE_CONTEXT context);

    //!
    //! \brief    Encrypts an uncompressed clear surface, to pass back to the application.
    //!
    //! \param    [in] src_resource
    //!           The source resource which contains the clear data.
    //! \param    [out] dst_Resource
    //!           The Destination resource. This resource will contain the encrypted data.
    //!           It should be allocated by the caller.
    //! \param    [in] width
    //!           The width of the surface in Bytes.
    //! \param    [in] height
    //!           The height of the surface in Bytes (pay attention that for NV12 the
    //!           height(Bytes) = 1.5*height(Pixel)).
    //!
    //! \return   VAStatus
    //!           VA_STATUS_SUCCESS if success, else fail reason
    //!
    VAStatus EncryptionBlt(
        uint8_t *src_resource,
        uint8_t *dst_Resource,
        uint32_t width,
        uint32_t height);

    //!
    //! \brief    Decrypts an uncompressed image and blts data to a protected surface.
    //!
    //! \param    [in] src_resource
    //!           The source resource which contains the AES encrypted data. The surface is an RGB surface.
    //! \param    [out] dst_Resource
    //!           The Destination resource. This resource will contain the serpent encrypted data. It
    //!           should be allocated by the caller.
    //! \param    [in] width
    //!           The width of the surface in Bytes (for RGB the width(Bytes) = 4*width(Pixel)).
    //! \param    [in] height
    //!           The height of the surface in Bytes (for RGB: height(Bytes) = 1*height(Pixel)).
    //! \param    [in] num_segment
    //!           The number of segments
    //! \param    [in] segments
    //!           Pointer of segments
    //!
    //! \return   VAStatus
    //!           VA_STATUS_SUCCESS if success, else fail reason
    //!
    VAStatus DecryptionBlt(
        uint8_t *src_resource,
        uint8_t *dst_Resource,
        uint32_t width,
        uint32_t height,
        uint32_t num_segment,
        VAEncryptionSegmentInfo *segments,
        uint32_t key_blob_size);

    //!
    //! \brief   TEE execution of this protected session
    //!
    //! \param   [in] buffer
    //!          DDI_MEDIA_BUFFER
    //!
    //! \return  VAStatus
    //!          VA_STATUS_SUCCESS if success, else fail reason
    //!
    VAStatus Execute(DDI_MEDIA_BUFFER *buffer);

    //!
    //! \brief   Return the config attribute
    //!
    //! \return  PDDI_CP_CONFIG_ATTR
    //!          Return the config attribute of this crypto session
    //!
    PDDI_CP_CONFIG_ATTR GetConfigAttr() { return &m_Attr; }

protected:
    PDDI_CP_CONTEXT     m_CpContext;
    DDI_CP_CONFIG_ATTR  m_Attr;

private:
    bool IsSessionAlive();

    VAStatus ProcessingGpuFunction(VAProtectedSessionExecuteBuffer *buff);

    VAStatus ProcessingTeeFunction(VAProtectedSessionExecuteBuffer *buff);

    //!
    //! \brief   Establishes PAVP HW session in GPU according to policy blob.
    //!
    //! \param   [in] buffer
    //!          DDI_MEDIA_BUFFER
    //!
    //! \return  VAStatus
    //!          VA_STATUS_SUCCESS if success, else fail reason
    //!
    VAStatus HwSessionUpdate(VAProtectedSessionExecuteBuffer *exec_buff);

    // 0x40000000~0x40000FFF belongs to GPU Function
    inline bool IsVAGpuFunctionID(uint32_t id)
    {
        return (id >= 0x40000000 && id < 0x40001000);
    }

    // 0x00000000~0x00000FFF belongs to TEE Function
    inline bool IsVATeeFunctionID(uint32_t id)
    {
        return (id < 0x1000);
    }

    MEDIA_CLASS_DEFINE_END(CPavpCryptoSessionVA)
};

static inline CPavpCryptoSessionVA* GetCryptoSession(PDDI_CP_CONTEXT cp_context)
{
    return static_cast<CPavpCryptoSessionVA*>(cp_context->pDrvPrivate.get());
}

#endif //#ifdef VA_DRIVER_VTABLE_PROT_VERSION

#endif // __CP_CRYPTOSESSION_VA_H__
