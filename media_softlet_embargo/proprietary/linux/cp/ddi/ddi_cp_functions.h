/*
* Copyright (c) 2021-2022, Intel Corporation
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
//! \file     ddi_cp_functions.h
//! \brief    ddi cp functions head file
//!

#ifndef __DDI_CP_FUNCTIONS_H__
#define __DDI_CP_FUNCTIONS_H__

#include <stdint.h>
#include <va/va.h>
#include <va/va_backend.h>
#include <va/va_version.h>
#include "ddi_media_functions.h"
#include "media_class_trace.h"
#include "media_libva_common.h"

#define DDI_MEDIA_VACONTEXTID_OFFSET_PROT_LP        0
#define DDI_MEDIA_VACONTEXTID_OFFSET_PROT_CP        0x8000000

#define DDI_MEDIA_MASK_VAPROTECTEDSESSION_ID        0x7FFFFFF
#define DDI_MEDIA_MASK_VAPROTECTEDSESSION_TYPE      0x8000000

#define DDI_MEDIA_CONTEXT_TYPE_PROTECTED_LINK       1
#define DDI_MEDIA_CONTEXT_TYPE_PROTECTED_CONTENT    2
class DdiCpFunctions :public DdiMediaFunctions
{
public:

    virtual ~DdiCpFunctions() override{};

#if VA_CHECK_VERSION(1,11,0)
    //!
    //! \brief   Create protected session
    //!
    //! \param   [in] ctx
    //!          Pointer to VA driver context
    //! \param   [in] configId
    //!          VA configuration ID
    //! \param   [out] protectedSession
    //!          VA protected session ID
    //!
    //! \return  VAStatus
    //!     VA_STATUS_SUCCESS if success, else fail reason
    //!
    virtual VAStatus CreateProtectedSession (
        VADriverContextP      ctx,
        VAConfigID            configId,
        VAProtectedSessionID  *protectedSession
    ) override;

    //!
    //! \brief   Destroy protected session
    //!
    //! \param   [in] ctx
    //!          Pointer to VA driver context
    //! \param   [in] protectedSession
    //!          VA protected session ID
    //!
    //! \return  VAStatus
    //!     VA_STATUS_SUCCESS if success, else fail reason
    //!
    virtual VAStatus DestroyProtectedSession (
        VADriverContextP      ctx,
        VAProtectedSessionID  protectedSession
    ) override;

    //!
    //! \brief   Attach protected session to display or context
    //!
    //! \param   [in] ctx
    //!          Pointer to VA driver context
    //! \param   [in] context
    //!          VA context ID to be attached if not 0.
    //! \param   [in] protectedSession
    //!          VA protected session ID
    //!
    //! \return  VAStatus
    //!     VA_STATUS_SUCCESS if success, else fail reason
    //!
    virtual VAStatus AttachProtectedSession (
        VADriverContextP      ctx,
        VAContextID           context,
        VAProtectedSessionID  protectedSession
    ) override;

    //!
    //! \brief   Detach protected session from display or context
    //!
    //! \param   [in] ctx
    //!          Pointer to VA driver context
    //! \param   [in] context
    //!          VA context ID to be Detached if not 0.
    //!
    //! \return  VAStatus
    //!     VA_STATUS_SUCCESS if success, else fail reason
    //!
    virtual VAStatus DetachProtectedSession (
        VADriverContextP  ctx,
        VAContextID       context
    ) override;

    //!
    //! \brief   TEE execution for the particular protected session
    //!
    //! \param   [in] ctx
    //!          Pointer to VA driver context
    //! \param   [in] protectedSession
    //!          VA protected session ID
    //! \param   [in] data
    //!          VA buffer ID
    //!
    //! \return  VAStatus
    //!     VA_STATUS_SUCCESS if success, else fail reason
    //!
    virtual VAStatus ProtectedSessionExecute (
        VADriverContextP      ctx,
        VAProtectedSessionID  protectedSession,
        VABufferID            data
    ) override;
#endif
    //!
    //! \brief  Create buffer
    //!
    //! \param  [in] ctx
    //!         Pointer to VA driver context
    //! \param  [in] context
    //!         VA context id
    //! \param  [in] type
    //!         VA buffer type
    //! \param  [in] size
    //!         Buffer size
    //! \param  [out] elementsNum
    //!         Number of elements
    //! \param  [in] data
    //!         Buffer data
    //! \param  [out] bufId
    //!         VA buffer id
    //!
    //! \return VAStatus
    //!     VA_STATUS_SUCCESS if success, else fail reason
    //!
    virtual VAStatus CreateBuffer (
        VADriverContextP  ctx,
        VAContextID       context,
        VABufferType      type,
        uint32_t          size,
        uint32_t          elementsNum,
        void              *data,
        VABufferID        *bufId
    ) override;

    //!
    //! \brief  Create a configuration for the encode/decode/vp pipeline
    //! \details    it passes in the attribute list that specifies the attributes it cares
    //!             about, with the rest taking default values.
    //!
    //! \param  [in] ctx
    //!         Pointer to VA driver context
    //! \param  [in] profile
    //!         VA profile of configuration
    //! \param  [in] entrypoint
    //!         VA entrypoint of configuration
    //! \param  [out] attribList
    //!         VA attrib list
    //! \param  [out] attribsNum
    //!         Number of attribs
    //! \param  [out] configId
    //!         VA config id
    //!
    //! \return VAStatus
    //!     VA_STATUS_SUCCESS if success, else fail reason
    //!
    virtual VAStatus CreateConfig (
        VADriverContextP  ctx,
        VAProfile         profile,
        VAEntrypoint      entrypoint,
        VAConfigAttrib    *attribList,
        int32_t           attribsNum,
        VAConfigID        *configId
    ) override;

private:
    void* GetContextFromProtectedSessionID(
        VADriverContextP     ctx,
        VAProtectedSessionID vaID,
        uint32_t             *ctxType);

    void ReleasePVAContextFromHeap(PDDI_MEDIA_HEAP vaContextHeap, uint32_t vaContextID);

MEDIA_CLASS_DEFINE_END(DdiCpFunctions)
};

#endif //__DDI_CP_FUNCTIONS_H__