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
//! \file     media_ddi_prot_content.cpp
//! \brief    The class implementation of DdiDecodeBase for all protected session
//!

#include "va_protected_content_private.h"
#include "media_libva_util.h"
#include "media_libva_decoder.h"
#include "media_libva_encoder.h"
#include "media_libva_vp.h"
#include "media_libva_cp.h"
#include "media_libva_caps_cp.h"
#include "cp_pavp_cryptosession_va.h"
#include "media_libva_cp.h"
#include "media_ddi_prot_content.h"

static bool isDefaultRegistered = DdiProtectedFactory::Register<DdiMediaProtectedContent>(DDI_PROTECTED_DEFAULT, true);
static bool isProtectedContentRegistered = DdiProtectedFactory::Register<DdiMediaProtectedContent>(DDI_PROTECTED_CONTENT, true);

bool DdiMediaProtectedContent::CheckEntrypointSupported(VAEntrypoint entrypoint)
{
#ifdef VA_DRIVER_VTABLE_PROT_VERSION
    if (entrypoint == (VAEntrypoint)VAEntrypointProtectedContent ||
        entrypoint == VAEntrypointProtectedTEEComm)
    {
        return true;
    }
#endif

    return false;
}

bool DdiMediaProtectedContent::CheckAttribList(
    VAProfile profile,
    VAEntrypoint entrypoint,
    VAConfigAttrib* attrib,
    uint32_t numAttribs)
{
#ifdef VA_DRIVER_VTABLE_PROT_VERSION
    if ((profile == VAProfileProtected) &&
        (entrypoint == (VAEntrypoint)VAEntrypointProtectedContent ||
         entrypoint == VAEntrypointProtectedTEEComm))
    {
        return true;
    }
#endif

    return false;
}

VAStatus DdiMediaProtectedContent::CreateProtectedSession(
    VADriverContextP        ctx,
    VAConfigID              config_id,
    VAProtectedSessionID    *protected_session)
{
    DDI_FUNCTION_ENTER();

    VAStatus va = VA_STATUS_SUCCESS;
#ifdef VA_DRIVER_VTABLE_PROT_VERSION
    PDDI_MEDIA_CONTEXT mediaCtx = nullptr;
    PDDI_MEDIA_VACONTEXT_HEAP_ELEMENT contextHeapElement = nullptr;
    DdiCp *pDdiCp = nullptr;
    DDI_CP_CONTEXT *ddiCpContext = nullptr;
    MediaLibvaCapsCp *cpCaps = nullptr;
    DDI_CP_CONFIG_ATTR cpConfigAttr;
    MOS_CONTEXT mosCtx = {};
    std::shared_ptr<CPavpCryptoSessionVA> pSession = nullptr;

    do
    {
        mediaCtx  = DdiMedia_GetMediaContext(ctx);
        DDI_CHK_NULL(mediaCtx, "nullptr mediaCtx", VA_STATUS_ERROR_INVALID_CONTEXT);
        DDI_CHK_NULL(mediaCtx->m_caps, "nullptr m_caps", VA_STATUS_ERROR_INVALID_CONTEXT);
        DDI_CHK_NULL(mediaCtx->pProtCtxHeap, "nullptr pProtCtxHeap", VA_STATUS_ERROR_INVALID_CONTEXT);

        cpCaps = dynamic_cast<MediaLibvaCapsCp*>(mediaCtx->m_caps->GetCpCaps());
        DDI_CHK_NULL(cpCaps, "nullptr cpCaps", VA_STATUS_ERROR_INVALID_CONTEXT);

        ddiCpContext = MOS_New(DDI_CP_CONTEXT);
        DDI_CHK_NULL(ddiCpContext, "nullptr ddiCpContext", VA_STATUS_ERROR_ALLOCATION_FAILED);

        va = cpCaps->GetCpConfigAttr(
                config_id + DDI_CP_GEN_CONFIG_ATTRIBUTES_BASE,
                &cpConfigAttr.profile,
                &cpConfigAttr.entrypoint,
                &cpConfigAttr.uiSessionMode,
                &cpConfigAttr.uiSessionType,
                &cpConfigAttr.uiCipherAlgo,
                &cpConfigAttr.uiCipherBlockSize,
                &cpConfigAttr.uiCipherMode,
                &cpConfigAttr.uiCipherSampleType,
                &cpConfigAttr.uiUsage);
        if (va != VA_STATUS_SUCCESS)
        {
            DDI_ASSERTMESSAGE("Invalide config_id!");
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, va);
            break;
        }
        DDI_VERBOSEMESSAGE("profile=%d, entrypoint=%d, mode=%d, type=%d",
            cpConfigAttr.profile,
            cpConfigAttr.entrypoint,
            cpConfigAttr.uiSessionMode,
            cpConfigAttr.uiSessionType);
        DDI_VERBOSEMESSAGE("algo=%d, size=%d, ciphermode=%d, sampletype=%d, usage=%d",
            cpConfigAttr.uiCipherAlgo,
            cpConfigAttr.uiCipherBlockSize,
            cpConfigAttr.uiCipherMode,
            cpConfigAttr.uiCipherSampleType,
            cpConfigAttr.uiUsage);

        mosCtx.bufmgr                = mediaCtx->pDrmBufMgr;
        mosCtx.m_gpuContextMgr       = mediaCtx->m_gpuContextMgr;
        mosCtx.m_cmdBufMgr           = mediaCtx->m_cmdBufMgr;
        mosCtx.fd                    = mediaCtx->fd;
        mosCtx.iDeviceId             = mediaCtx->iDeviceId;
        mosCtx.m_skuTable            = mediaCtx->SkuTable;
        mosCtx.m_waTable             = mediaCtx->WaTable;
        mosCtx.m_gtSystemInfo        = *mediaCtx->pGtSystemInfo;
        mosCtx.m_platform            = mediaCtx->platform;
        mosCtx.ppMediaMemDecompState = &mediaCtx->pMediaMemDecompState;
        mosCtx.pfnMemoryDecompress   = mediaCtx->pfnMemoryDecompress;
        mosCtx.pfnMediaMemoryCopy    = mediaCtx->pfnMediaMemoryCopy;
        mosCtx.pfnMediaMemoryCopy2D  = mediaCtx->pfnMediaMemoryCopy2D;
        mosCtx.ppMediaCopyState      = &mediaCtx->pMediaCopyState;
        mosCtx.m_auxTableMgr         = mediaCtx->m_auxTableMgr;
        mosCtx.pGmmClientContext     = mediaCtx->pGmmClientContext;
        mosCtx.m_osDeviceContext     = mediaCtx->m_osDeviceContext;
        mosCtx.m_apoMosEnabled       = mediaCtx->m_apoMosEnabled;
        mosCtx.m_userSettingPtr      = mediaCtx->m_userSettingPtr;
        if (MEDIA_IS_SKU(&mosCtx.m_skuTable, FtrProtectedGEMContextPatch))
        {
            mosCtx.m_protectedGEMContext = true; // always use protected GEM context for PXP path
        }

        pSession = std::make_shared<CPavpCryptoSessionVA>(ddiCpContext, cpConfigAttr);
        if (!pSession)
        {
            DDI_ASSERTMESSAGE("Create CPavpCryptoSessionVA fail! va=0x%x", va);
            va = VA_STATUS_ERROR_ALLOCATION_FAILED;
            MT_ERR2(MT_CP_SESSION_CREATE, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, va);
            break;
        }

        va = pSession->Setup(&mosCtx); //session init, inplay
        if (va != VA_STATUS_SUCCESS)
        {
            DDI_ASSERTMESSAGE("CPavpCryptoSessionVA Setup fail! va=0x%x", va);
            MT_ERR2(MT_CP_SESSION_INIT, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, va);
            break;
        }

        if (cpConfigAttr.entrypoint != VAEntrypointProtectedTEEComm)
        {
            pDdiCp = dynamic_cast<DdiCp*>(Create_DdiCp(&mosCtx));
            if (!pDdiCp)
            {
                DDI_ASSERTMESSAGE("nullptr pDdiCp");
                MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, VA_STATUS_ERROR_ALLOCATION_FAILED);
                MOS_Delete(ddiCpContext);
                return VA_STATUS_ERROR_ALLOCATION_FAILED;
            }
            pDdiCp->UpdateCpContext(pSession->GetCpTag(), cpConfigAttr);
            ddiCpContext->pCpDdiInterface = pDdiCp;
        }

        ddiCpContext->pDrvPrivate = pSession;
        ddiCpContext->pMediaCtx = mediaCtx;

        DdiMediaUtil_LockMutex(&mediaCtx->ProtMutex);
        contextHeapElement = DdiMediaUtil_AllocPVAContextFromHeap(mediaCtx->pProtCtxHeap);
        if (nullptr == contextHeapElement)
        {
            DdiMediaUtil_UnLockMutex(&mediaCtx->ProtMutex);
            va = VA_STATUS_ERROR_MAX_NUM_EXCEEDED;
            break;
        }

        // offset uiVaContextID to 0x8000000
        contextHeapElement->uiVaContextID += DDI_MEDIA_VACONTEXTID_OFFSET_PROT_CP;
        contextHeapElement->pVaContext = (void*) ddiCpContext;
        mediaCtx->uiNumProts++;
        *protected_session = (VAContextID)(contextHeapElement->uiVaContextID + DDI_MEDIA_VACONTEXTID_OFFSET_PROT);

        DdiMediaUtil_UnLockMutex(&mediaCtx->ProtMutex);
    } while (false);

    if (va != VA_STATUS_SUCCESS)
    {
        if (contextHeapElement)
        {
            DdiMediaUtil_ReleasePVAContextFromHeap(
                mediaCtx->pProtCtxHeap,
                contextHeapElement->uiVaContextID -= DDI_MEDIA_VACONTEXTID_OFFSET_PROT_CP
            );
        }
        Delete_DdiCp(pDdiCp);
        MOS_Delete(ddiCpContext);
    }
#endif

    DDI_FUNCTION_EXIT(va);
    return va;
}

VAStatus DdiMediaProtectedContent::DestroyProtectedSession(
    VADriverContextP        ctx,
    VAProtectedSessionID    protected_session)
{
    DDI_FUNCTION_ENTER();

    PDDI_MEDIA_CONTEXT mediaCtx  = DdiMedia_GetMediaContext(ctx);
    DDI_CHK_NULL(mediaCtx, "nullptr mediaCtx", VA_STATUS_ERROR_INVALID_CONTEXT);
    uint32_t ctxType;
    PDDI_CP_CONTEXT ddiCpContext = (PDDI_CP_CONTEXT) DdiMedia_GetContextFromProtectedSessionID(ctx, protected_session, &ctxType);
    DDI_CHK_NULL(ddiCpContext, "nullptr cpCtx", VA_STATUS_ERROR_INVALID_CONTEXT);

    if (ddiCpContext->pCpDdiInterface)
    {
        Delete_DdiCpInterface(ddiCpContext->pCpDdiInterface);
        ddiCpContext->pCpDdiInterface = nullptr;
    }

    ddiCpContext->pDrvPrivate = nullptr;

    // Free the context id from the context_heap earlier
    uint32_t cpIndex = (uint32_t)protected_session & DDI_MEDIA_MASK_VAPROTECTEDSESSION_ID;
    DdiMediaUtil_LockMutex(&mediaCtx->ProtMutex);
    ReleasePVAContextFromHeap(mediaCtx->pProtCtxHeap, cpIndex);
    mediaCtx->uiNumProts--;

    // when destroy cp session, find all the media context it attached before
    // and SetAttached(nullptr) for these media context
    PDDI_DECODE_CONTEXT decCtx = nullptr;
    PDDI_ENCODE_CONTEXT encCtx = nullptr;
    PDDI_VP_CONTEXT vpCtx = nullptr;
    DdiCp *pDdiCp = nullptr;
    if (!ddiCpContext->mapAttaching.empty())
    {
        for (auto itr = ddiCpContext->mapAttaching.begin(); itr != ddiCpContext->mapAttaching.end(); ++itr)
        {
            uint32_t ctxType = DDI_MEDIA_CONTEXT_TYPE_NONE;
            void *ctxPtr = DdiMedia_GetContextFromContextID(ctx, itr->first, &ctxType);
            if (ctxPtr == nullptr) continue;
            pDdiCp = nullptr;
            // context id may be same, but context pointer may be different
            // We need to find the exact original media context that it already
            // attached. If found, that means it is still valid (not destroyed)
            if (itr->second == ctxPtr)
            {
                switch (ctxType)
                {
                    case DDI_MEDIA_CONTEXT_TYPE_DECODER:
                        decCtx = DdiDecode_GetDecContextFromPVOID(ctxPtr);
                        pDdiCp = dynamic_cast<DdiCp*>(decCtx->pCpDdiInterface);
                        break;
                    case DDI_MEDIA_CONTEXT_TYPE_ENCODER:
                        encCtx = DdiEncode_GetEncContextFromPVOID(ctxPtr);
                        pDdiCp = dynamic_cast<DdiCp*>(encCtx->pCpDdiInterface);
                        break;
                    case DDI_MEDIA_CONTEXT_TYPE_VP:
                        vpCtx = (PDDI_VP_CONTEXT) ctxPtr;
                        pDdiCp = dynamic_cast<DdiCp*>(vpCtx->pCpDdiInterface);
                        break;
                    default:
                        DDI_ASSERTMESSAGE("DDI: unsupported context in DdiMediaProtectedContent::AttachProtectedSession.");
                        MT_ERR1(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__);
                        break;
                }
                if (pDdiCp)
                {
                    pDdiCp->SetAttached(nullptr);
                    pDdiCp->ResetCpContext();
                }
            }
        }
    }
    // erase all the key is cp session id in this map
    ddiCpContext->mapAttaching.clear();

    MOS_Delete(ddiCpContext);

    DdiMediaUtil_UnLockMutex(&mediaCtx->ProtMutex);

    DDI_FUNCTION_EXIT(VA_STATUS_SUCCESS);
    return VA_STATUS_SUCCESS;
}

VAStatus DdiMediaProtectedContent::AttachProtectedSession(
    VADriverContextP        ctx,
    VAContextID             context,
    VAProtectedSessionID    protected_session)
{
    DDI_FUNCTION_ENTER();

    VAStatus vaStatus = VA_STATUS_SUCCESS;
#ifdef VA_DRIVER_VTABLE_PROT_VERSION
    PDDI_MEDIA_CONTEXT mediaCtx  = DdiMedia_GetMediaContext(ctx);
    uint32_t ctxCpType = DDI_MEDIA_CONTEXT_TYPE_NONE;
    void     *ctxCpPtr = DdiMedia_GetContextFromProtectedSessionID(ctx, protected_session, &ctxCpType);
    uint32_t ctxType = DDI_MEDIA_CONTEXT_TYPE_NONE;
    void     *ctxPtr = DdiMedia_GetContextFromContextID(ctx, context, &ctxType);
    PDDI_CP_CONTEXT cpCtx = nullptr;
    PDDI_DECODE_CONTEXT decCtx = nullptr;
    PDDI_ENCODE_CONTEXT encCtx = nullptr;
    PDDI_VP_CONTEXT vpCtx = nullptr;
    DdiCp *pDdiCp = nullptr;

    DDI_CHK_NULL(mediaCtx, "nullptr mediaCtx", VA_STATUS_ERROR_INVALID_CONTEXT);
    DDI_CHK_NULL(ctxCpPtr, "nullptr ctxCpPtr", VA_STATUS_ERROR_INVALID_CONTEXT);
    DDI_CHK_NULL(ctxPtr, "nullptr ctxPtr", VA_STATUS_ERROR_INVALID_CONTEXT);

    if (ctxCpType != DDI_MEDIA_CONTEXT_TYPE_PROTECTED_CONTENT)
    {
        DDI_ASSERTMESSAGE("wrong cp_context type %d", ctxCpType);
        MT_ERR2(MT_ERR_INVALID_ARG, MT_CP_CTX_TYPE, ctxCpType, MT_FUNC_RET, VA_STATUS_ERROR_INVALID_CONTEXT);
        return VA_STATUS_ERROR_INVALID_CONTEXT;
    }

    do
    {
        cpCtx = DdiCp_GetCpContextFromPVOID(ctxCpPtr);
        CPavpCryptoSessionVA* cp_session = dynamic_cast<CPavpCryptoSessionVA*>(GetCryptoSession(cpCtx));
        DDI_CHK_NULL(cpCtx->pCpDdiInterface, "nullptr pCpDdiInterface", VA_STATUS_ERROR_INVALID_CONTEXT);
        DDI_CHK_NULL(cp_session, "cp_session", VA_STATUS_ERROR_INVALID_CONTEXT);

        switch (ctxType)
        {
            case DDI_MEDIA_CONTEXT_TYPE_DECODER:
                decCtx = DdiDecode_GetDecContextFromPVOID(ctxPtr);
                pDdiCp = dynamic_cast<DdiCp*>(decCtx->pCpDdiInterface);
                break;
            case DDI_MEDIA_CONTEXT_TYPE_ENCODER:
                encCtx = DdiEncode_GetEncContextFromPVOID(ctxPtr);
                pDdiCp = dynamic_cast<DdiCp*>(encCtx->pCpDdiInterface);
                break;
            case DDI_MEDIA_CONTEXT_TYPE_VP:
                vpCtx = (PDDI_VP_CONTEXT) ctxPtr;
                pDdiCp = dynamic_cast<DdiCp*>(vpCtx->pCpDdiInterface);
                break;
            default:
                DDI_ASSERTMESSAGE("DDI: unsupported context in DdiMediaProtectedContent::AttachProtectedSession.");
                MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_FUNC_RET, VA_STATUS_ERROR_INVALID_CONTEXT);
                return VA_STATUS_ERROR_INVALID_CONTEXT;
        }

        DDI_CHK_NULL(pDdiCp, "nullptr pDdiCp", VA_STATUS_ERROR_INVALID_CONTEXT);

        DdiMediaUtil_LockMutex(&mediaCtx->ProtMutex);
        if (pDdiCp->IsAttached())
        {
            DDI_ASSERTMESSAGE("Already attached, ctxType=%d, ctxPtr=%p", ctxType, ctxPtr);
            vaStatus  = VA_STATUS_ERROR_OPERATION_FAILED;
            MT_LOG3(MT_CP_ATTACH_SESSION, MT_NORMAL, MT_CP_CTX_TYPE, ctxType, MT_CP_CTX_PTR, (int64_t)ctxPtr, MT_FUNC_RET, vaStatus);
            DdiMediaUtil_UnLockMutex(&mediaCtx->ProtMutex);
            break;
        }

        pDdiCp->SetAttached(cp_session);
        pDdiCp->UpdateCpContext(cpCtx->pCpDdiInterface);

        // find if this media context is already in this map or not
        // if not, then add it into this map
        bool bFound = false;
        auto range = cpCtx->mapAttaching.equal_range(context);
        for (auto i = range.first; i != range.second; ++i)
        {
            if (i->second == ctxPtr)
            {
                bFound = true;
                break;
            }
        }
        if (!bFound)
        {
            cpCtx->mapAttaching.insert(std::make_pair(context, ctxPtr));
        }

        DdiMediaUtil_UnLockMutex(&mediaCtx->ProtMutex);

    } while (false);
#endif

    DDI_FUNCTION_EXIT(vaStatus);
    return vaStatus;
}

VAStatus DdiMediaProtectedContent::DetachProtectedSession(
    VADriverContextP    ctx,
    VAContextID         context)
{
    DDI_FUNCTION_ENTER();

    VAStatus vaStatus = VA_STATUS_SUCCESS;
    PDDI_MEDIA_CONTEXT mediaCtx  = DdiMedia_GetMediaContext(ctx);
    uint32_t ctxType = DDI_MEDIA_CONTEXT_TYPE_NONE;
    void *ctxPtr = DdiMedia_GetContextFromContextID(ctx, context, &ctxType);
    PDDI_DECODE_CONTEXT decCtx = nullptr;
    PDDI_ENCODE_CONTEXT encCtx = nullptr;
    PDDI_VP_CONTEXT vpCtx = nullptr;
    DdiCp *pDdiCp = nullptr;

    DDI_CHK_NULL(mediaCtx, "nullptr mediaCtx", VA_STATUS_ERROR_INVALID_CONTEXT);
    DDI_CHK_NULL(ctxPtr, "nullptr ctxPtr", VA_STATUS_ERROR_INVALID_CONTEXT);

    do
    {
        switch (ctxType)
        {
            case DDI_MEDIA_CONTEXT_TYPE_DECODER:
                decCtx = DdiDecode_GetDecContextFromPVOID(ctxPtr);
                pDdiCp = dynamic_cast<DdiCp*>(decCtx->pCpDdiInterface);
                break;
            case DDI_MEDIA_CONTEXT_TYPE_ENCODER:
                encCtx = DdiEncode_GetEncContextFromPVOID(ctxPtr);
                pDdiCp = dynamic_cast<DdiCp*>(encCtx->pCpDdiInterface);
                break;
            case DDI_MEDIA_CONTEXT_TYPE_VP:
                vpCtx = (PDDI_VP_CONTEXT) ctxPtr;
                pDdiCp = dynamic_cast<DdiCp*>(vpCtx->pCpDdiInterface);
                break;
            default:
                DDI_ASSERTMESSAGE("DDI: unsupported context in DdiCp::DetachProtectedSession.");
                MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_FUNC_RET, VA_STATUS_ERROR_INVALID_CONTEXT);
                return VA_STATUS_ERROR_INVALID_CONTEXT;
        }

        DDI_CHK_NULL(pDdiCp,      "nullptr pDdiCp",     VA_STATUS_ERROR_INVALID_CONTEXT);

        DdiMediaUtil_LockMutex(&mediaCtx->ProtMutex);
        if (!pDdiCp->IsAttached())
        {
            DDI_ASSERTMESSAGE("Not attached, ctxType=%d, ctxPtr=%p", ctxType, ctxPtr);
            vaStatus  = VA_STATUS_ERROR_OPERATION_FAILED;
            MT_LOG3(MT_CP_DETACH_SESSION, MT_NORMAL, MT_CP_CTX_TYPE, ctxType, MT_CP_CTX_PTR, (int64_t)ctxPtr, MT_FUNC_RET, vaStatus);
            DdiMediaUtil_UnLockMutex(&mediaCtx->ProtMutex);
            break;
        }

        pDdiCp->SetAttached(nullptr);
        pDdiCp->ResetCpContext();
        DdiMediaUtil_UnLockMutex(&mediaCtx->ProtMutex);

    } while (false);

    DDI_FUNCTION_EXIT(vaStatus);
    return vaStatus;
}

VAStatus DdiMediaProtectedContent::ProtectedSessionExecute(
    VADriverContextP        ctx,
    VAProtectedSessionID    protected_session,
    VABufferID              data)
{
    DDI_FUNCTION_ENTER();

    VAStatus vaStatus = VA_STATUS_SUCCESS;
#ifdef VA_DRIVER_VTABLE_PROT_VERSION
    uint32_t ctxType = DDI_MEDIA_CONTEXT_TYPE_NONE;
    void     *ctxPtr = DdiMedia_GetContextFromProtectedSessionID(ctx, protected_session, &ctxType);
    DDI_CHK_NULL(ctxPtr,    "nullptr ctxPtr",   VA_STATUS_ERROR_INVALID_CONTEXT);
    if (ctxType != DDI_MEDIA_CONTEXT_TYPE_PROTECTED_CONTENT)
    {
        DDI_ASSERTMESSAGE("wrong cp_context type %d", ctxType);
        MT_ERR2(MT_ERR_INVALID_ARG, MT_CP_CTX_TYPE, ctxType, MT_FUNC_RET, VA_STATUS_ERROR_INVALID_CONTEXT);
        return VA_STATUS_ERROR_INVALID_CONTEXT;
    }

    PDDI_CP_CONTEXT cpCtx = DdiCp_GetCpContextFromPVOID(ctxPtr);
    DDI_CHK_NULL(cpCtx,     "nullptr cpCtx",    VA_STATUS_ERROR_INVALID_CONTEXT);
    DDI_CHK_NULL(cpCtx->pMediaCtx,  "NULL cpCtx->pMediaCtx",    VA_STATUS_ERROR_INVALID_CONTEXT);

    CPavpCryptoSessionVA *session = GetCryptoSession(cpCtx);
    DDI_CHK_NULL(session,           "NULL session",             VA_STATUS_ERROR_INVALID_CONTEXT);

    DDI_MEDIA_BUFFER *buffer = DdiMedia_GetBufferFromVABufferID(cpCtx->pMediaCtx, data);
    DDI_CHK_NULL(buffer,            "NULL buf",                 VA_STATUS_ERROR_INVALID_BUFFER);
    DDI_CHK_NULL(buffer->pData,     "NULL buf->pData",          VA_STATUS_ERROR_INVALID_BUFFER);

    vaStatus = session->Execute(buffer);
#endif

    DDI_FUNCTION_EXIT(vaStatus);
    return vaStatus;
}

VAStatus DdiMediaProtectedContent::ProtectedSessionCreateBuffer(
    VADriverContextP        ctx,
    VAProtectedSessionID    protected_session,
    VABufferType            type,
    uint32_t                size,
    uint32_t                num_elements,
    void                    *data,
    VABufferID              *bufId)
{
    DDI_FUNCTION_ENTER();

    uint32_t ctxType = DDI_MEDIA_CONTEXT_TYPE_NONE;
    void     *ctxPtr = DdiMedia_GetContextFromProtectedSessionID(ctx, protected_session, &ctxType);
    DDI_CHK_NULL(ctxPtr,    "nullptr ctxPtr",   VA_STATUS_ERROR_INVALID_CONTEXT);
    if (ctxType != DDI_MEDIA_CONTEXT_TYPE_PROTECTED_CONTENT)
    {
        DDI_ASSERTMESSAGE("wrong cp_context type %d", ctxType);
        MT_ERR2(MT_ERR_INVALID_ARG, MT_CP_CTX_TYPE, ctxType, MT_FUNC_RET, VA_STATUS_ERROR_INVALID_CONTEXT);
        return VA_STATUS_ERROR_INVALID_CONTEXT;
    }

    PDDI_CP_CONTEXT cpCtx = DdiCp_GetCpContextFromPVOID(ctxPtr);
    DDI_CHK_NULL(cpCtx,             "NULL pCtx",                VA_STATUS_ERROR_INVALID_CONTEXT);
    DDI_CHK_NULL(cpCtx->pMediaCtx,  "NULL cpCtx->pMediaCtx",    VA_STATUS_ERROR_INVALID_CONTEXT);

    PDDI_MEDIA_BUFFER              buf = nullptr;
    VAStatus                       vaStatus = VA_STATUS_SUCCESS;
    PDDI_MEDIA_BUFFER_HEAP_ELEMENT bufferHeapElement;
    MOS_STATUS                     status = MOS_STATUS_SUCCESS;

    do
    {
        *bufId = VA_INVALID_ID;
        buf = (DDI_MEDIA_BUFFER *) MOS_AllocAndZeroMemory(sizeof(DDI_MEDIA_BUFFER));
        DDI_CHK_NULL(buf,           "NULL buf.", VA_STATUS_ERROR_ALLOCATION_FAILED);

        // allocate new buf and init
        buf->pMediaCtx     = cpCtx->pMediaCtx;
        buf->iSize         = size * num_elements;
        buf->uiNumElements = num_elements;
        buf->uiType        = type;
        buf->format        = Media_Format_CPU;
        buf->uiOffset      = 0;
        buf->pData         = (uint8_t*)MOS_AllocAndZeroMemory(size * num_elements);
        if (nullptr == buf->pData)
        {
            MOS_FreeMemAndSetNull(buf);
            return VA_STATUS_ERROR_ALLOCATION_FAILED;
        }

        bufferHeapElement  = DdiMediaUtil_AllocPMediaBufferFromHeap(cpCtx->pMediaCtx->pBufferHeap);
        if (nullptr == bufferHeapElement)
        {
            MOS_FreeMemAndSetNull(buf->pData);
            MOS_FreeMemAndSetNull(buf);
            DDI_ASSERTMESSAGE("Invalid buffer index.");
            vaStatus = VA_STATUS_ERROR_INVALID_BUFFER;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_FUNC_RET, vaStatus);
            break;
        }
        bufferHeapElement->pBuffer      = buf;
        bufferHeapElement->pCtx         = (void *) cpCtx;
        bufferHeapElement->uiCtxType    = DDI_MEDIA_CONTEXT_TYPE_PROTECTED;
        *bufId                          = bufferHeapElement->uiVaBufferID;
        cpCtx->pMediaCtx->uiNumBufs++;

        // if there is data from client, then dont need to copy data from client
        if (data)
        {
            // copy client data to new buf
            status = MOS_SecureMemcpy(buf->pData, size * num_elements, data, size * num_elements);
            if (status != MOS_STATUS_SUCCESS)
            {
                DDI_ASSERTMESSAGE("DDI: Failed to copy client data!");
                vaStatus = VA_STATUS_ERROR_MAX_NUM_EXCEEDED;
                MT_ERR2(MT_CP_MEM_COPY, MT_CODE_LINE, __LINE__, MT_FUNC_RET, vaStatus);
                break;
            }
        }
        else
        {
            // do nothing if there is no data from client
            vaStatus = VA_STATUS_SUCCESS;
        }

    } while (false);

    DDI_FUNCTION_EXIT(vaStatus);
    return vaStatus;
}

uint64_t DdiMediaProtectedContent::GetProtectedSurfaceTag(PDDI_MEDIA_CONTEXT media_ctx)
{
    DDI_FUNCTION_ENTER();

    uint32_t ret_r3ctxid = 0x0;
    uint32_t ret_tag = 0x0;
#ifdef VA_DRIVER_VTABLE_PROT_VERSION
    bool found = false;

    PDDI_MEDIA_VACONTEXT_HEAP_ELEMENT heap_elm = nullptr;
    DDI_CP_CONTEXT *ddi_cp_context = nullptr;

    DDI_CHK_NULL(media_ctx, "nullptr media_ctx", 0x0);


    auto &mutex = media_ctx->ProtMutex;
    auto &heap = media_ctx->pProtCtxHeap;
    if (nullptr == heap)
    {
        DDI_VERBOSEMESSAGE("heap is NULL");
        return 0x0;
    }

    for (auto i=0; i<heap->uiAllocatedHeapElements; i++)
    {
        DdiMediaUtil_LockMutex(&mutex);
        heap_elm = (PDDI_MEDIA_VACONTEXT_HEAP_ELEMENT) heap->pHeapBase;
        heap_elm += i;
        ddi_cp_context = reinterpret_cast<DDI_CP_CONTEXT*>(heap_elm->pVaContext);
        DdiMediaUtil_UnLockMutex(&mutex);

        if (ddi_cp_context == nullptr)
        {
            continue;
        }

        CPavpCryptoSessionVA* cp_session = dynamic_cast<CPavpCryptoSessionVA*>(GetCryptoSession(ddi_cp_context));
        if (cp_session == nullptr)
        {
            continue;
        }

        if (cp_session->GetSessionType() != PAVP_SESSION_TYPE_DECODE ||
            cp_session->IsMemoryEncryptionEnabled() == false)
        {
            continue;
        }

        found = true;
        ret_r3ctxid = heap_elm->uiVaContextID + DDI_MEDIA_VACONTEXTID_OFFSET_PROT;
        ret_tag = cp_session->GetCpTag();
        DDI_VERBOSEMESSAGE("GetProtectedSurfaceTag found=%d in index=%d, r3ctxid=0x%08x, tag=0x%08x", found, i, ret_r3ctxid, ret_tag);

        break;
    }

    if (!found)
    {
        DDI_VERBOSEMESSAGE("GetProtectedSurfaceTag not found");
    }
#endif

    DDI_FUNCTION_EXIT(ret_tag);
    return (uint64_t)ret_r3ctxid<<32 | ret_tag;
}

// This function is most same with DdiMediaUtil_ReleasePVAContextFromHeap but the last line
// which reset uiVaContextID to default value, the index vaContextID
void DdiMediaProtectedContent::ReleasePVAContextFromHeap(PDDI_MEDIA_HEAP vaContextHeap, uint32_t vaContextID)
{
    DDI_CHK_NULL(vaContextHeap, "nullptr vaContextHeap", );
    DDI_CHK_LESS(vaContextID, vaContextHeap->uiAllocatedHeapElements, "invalid context id", );
    PDDI_MEDIA_VACONTEXT_HEAP_ELEMENT vaContextHeapBase = (PDDI_MEDIA_VACONTEXT_HEAP_ELEMENT)vaContextHeap->pHeapBase;
    PDDI_MEDIA_VACONTEXT_HEAP_ELEMENT vaContextHeapElmt = &vaContextHeapBase[vaContextID];
    DDI_CHK_NULL(vaContextHeapElmt->pVaContext, "context is already released", );
    void *firstFree                        = vaContextHeap->pFirstFreeHeapElement;
    vaContextHeap->pFirstFreeHeapElement   = (void*)vaContextHeapElmt;
    vaContextHeapElmt->pNextFree           = (PDDI_MEDIA_VACONTEXT_HEAP_ELEMENT)firstFree;
    vaContextHeapElmt->pVaContext          = nullptr;
    vaContextHeapElmt->uiVaContextID       = vaContextID;
}
