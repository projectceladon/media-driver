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
//! \file     media_ddi_prot_content.h
//! \brief    Defines implementation class for DDI media protected content.
//!

#ifndef _MEDIA_DDI_PROT_CONTENT_H_
#define _MEDIA_DDI_PROT_CONTENT_H_

#include <stdint.h>
#include <va/va.h>
#include "media_ddi_prot.h"

#if defined(__has_include)
#if __has_include(<va/va_backend_prot.h>)
# include <va/va_backend_prot.h>
#endif //__has_include
#endif //defined(__has_include)

//!
//! \class  DdiMediaProtectedContent
//! \brief  Ddi media protected content
//!
class DdiMediaProtectedContent : public DdiMediaProtected
{
public:
    //!
    //! \brief Constructor
    //!
    DdiMediaProtectedContent() {};

    //!
    //! \brief Destructor
    //!
    virtual ~DdiMediaProtectedContent() {}

public:
    bool CheckEntrypointSupported(VAEntrypoint entrypoint) override;

    bool CheckAttribList(
        VAProfile profile,
        VAEntrypoint entrypoint,
        VAConfigAttrib* attrib,
        uint32_t numAttribs) override;

    uint64_t GetProtectedSurfaceTag(PDDI_MEDIA_CONTEXT media_ctx) override;

protected:

    VAStatus CreateProtectedSession(
        VADriverContextP        ctx,
        VAConfigID              config_id,
        VAProtectedSessionID    *protected_session) override;

    VAStatus DestroyProtectedSession(
        VADriverContextP        ctx,
        VAProtectedSessionID    protected_session) override;

    VAStatus AttachProtectedSession(
        VADriverContextP        ctx,
        VAContextID             context,
        VAProtectedSessionID    protected_session) override;

    VAStatus DetachProtectedSession(
        VADriverContextP    ctx,
        VAContextID         context) override;

    VAStatus ProtectedSessionExecute(
        VADriverContextP        ctx,
        VAProtectedSessionID    protected_session,
        VABufferID              data) override;

    VAStatus ProtectedSessionCreateBuffer(
        VADriverContextP        ctx,
        VAProtectedSessionID    protected_session,
        VABufferType            type,
        uint32_t                size,
        uint32_t                num_elements,
        void                    *data,
        VABufferID              *bufId) override;

private:
    void ReleasePVAContextFromHeap(
        PDDI_MEDIA_HEAP vaContextHeap,
        uint32_t vaContextID);

    MEDIA_CLASS_DEFINE_END(DdiMediaProtectedContent)
};

#endif /*  _MEDIA_DDI_PROT_CONTENT_H_ */
