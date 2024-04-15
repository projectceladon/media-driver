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
//! \file     ddi_cp_functions.cpp
//! \brief    ddi cp functions implementaion.
//!
#include "ddi_cp_functions.h"
#include "media_libva_util_next.h"
#include "media_libva_caps_next.h"
#include "va_protected_content_private.h"
#include "ddi_cp_caps.h"
#include "cp_pavp_cryptosession_va_next.h"
#include "ddi_libva_encoder_specific.h"
#include "ddi_libva_decoder_specific.h"
#include "ddi_cp_next.h"
#include "ddi_vp_functions.h"

#if VA_CHECK_VERSION(1,11,0)
VAStatus DdiCpFunctions::CreateProtectedSession(
    VADriverContextP      ctx,
    VAConfigID            configId,
    VAProtectedSessionID  *protectedSession)
{
    DDI_CP_FUNC_ENTER;

    DDI_CHK_NULL(protectedSession, "nullptr protected_session",    VA_STATUS_ERROR_INVALID_PARAMETER);

    VAStatus vaStatus = VA_STATUS_SUCCESS;
#ifdef VA_DRIVER_VTABLE_PROT_VERSION
    PDDI_MEDIA_CONTEXT                        mediaCtx           = nullptr;
    PDDI_MEDIA_VACONTEXT_HEAP_ELEMENT         contextHeapElement = nullptr;
    DdiCpNext                                *pDdiCp             = nullptr;
    DDI_CP_CONTEXT_NEXT                      *ddiCpContext       = nullptr;
    DDI_CP_CONFIG_ATTR                        cpConfigAttr;
    MOS_CONTEXT                               mosCtx   = {};
    std::shared_ptr<CPavpCryptoSessionVANext> pSession = nullptr;
    DdiCpCapsInterface                       *cpCaps   = nullptr;

    do
    {
        mediaCtx  = GetMediaContext(ctx);
        DDI_CP_CHK_NULL(mediaCtx,               "nullptr mediaCtx",     VA_STATUS_ERROR_INVALID_CONTEXT);
        DDI_CP_CHK_NULL(mediaCtx->m_capsNext,   "nullptr caps",         VA_STATUS_ERROR_INVALID_CONTEXT);
        DDI_CP_CHK_NULL(mediaCtx->pProtCtxHeap, "nullptr pProtCtxHeap", VA_STATUS_ERROR_INVALID_CONTEXT);

        ConfigList* configList = mediaCtx->m_capsNext->GetConfigList();
        DDI_CP_CHK_NULL(configList, "Get configList failed", VA_STATUS_ERROR_INVALID_PARAMETER);

        cpCaps = CreateDdiCpCaps();
        DDI_CP_CHK_NULL(cpCaps,     "nullptr cpCaps", VA_STATUS_ERROR_INVALID_PARAMETER);

        ddiCpContext = MOS_New(DDI_CP_CONTEXT_NEXT);
        DDI_CP_CHK_NULL(ddiCpContext, "nullptr ddiCpContext", VA_STATUS_ERROR_ALLOCATION_FAILED);

        uint32_t index = cpCaps->GetCpConfigId(configId);
        if(!(cpCaps->IsCpConfigId(configId)) || (index >= configList->size()) )
        {
            DDI_CP_ASSERTMESSAGE("Invalid cp config Id");
            vaStatus = VA_STATUS_ERROR_INVALID_CONFIG;
            break;
        }

        ConfigLinux configData = configList->at(index);
        cpConfigAttr.profile       = configData.profile;
        cpConfigAttr.entrypoint    = configData.entrypoint;
        cpConfigAttr.uiSessionMode = configData.componentData.data.sessionMode;
        cpConfigAttr.uiSessionType = configData.componentData.data.sessionType;
        cpConfigAttr.uiCipherAlgo  = configData.componentData.data.algorithm;
        cpConfigAttr.uiCipherMode  = configData.componentData.data.counterMode;
        cpConfigAttr.uiCipherBlockSize  = configData.componentData.data.blockSize;
        cpConfigAttr.uiCipherSampleType = configData.componentData.data.sampleType;
        cpConfigAttr.uiUsage       = configData.componentData.data.usage;

        DDI_CP_VERBOSEMESSAGE("profile=%d, entrypoint=%d, mode=%d, type=%d",
            cpConfigAttr.profile,
            cpConfigAttr.entrypoint,
            cpConfigAttr.uiSessionMode,
            cpConfigAttr.uiSessionType);
        DDI_CP_VERBOSEMESSAGE("algo=%d, size=%d, ciphermode=%d, sampletype=%d, usage=%d",
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
        mosCtx.pPerfData             = &ddiCpContext->perfData;
        if (MEDIA_IS_SKU(&mosCtx.m_skuTable, FtrProtectedGEMContextPatch))
        {
            mosCtx.m_protectedGEMContext = true; // always use protected GEM context for PXP path
        }

        pSession = std::make_shared<CPavpCryptoSessionVANext>(ddiCpContext, cpConfigAttr);
        if (!pSession)
        {
            DDI_CP_ASSERTMESSAGE("Create CPavpCryptoSessionVANext fail! va=0x%x", vaStatus);
            vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
            MT_ERR2(MT_CP_SESSION_CREATE, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, vaStatus);
            break;
        }

        vaStatus = pSession->Setup(&mosCtx); //session init, inplay
        if (vaStatus != VA_STATUS_SUCCESS)
        {
            DDI_CP_ASSERTMESSAGE("CPavpCryptoSessionVANext Setup fail! va=0x%x", vaStatus);
            MT_ERR2(MT_CP_SESSION_INIT, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, vaStatus);
            break;
        }

        if (cpConfigAttr.entrypoint != VAEntrypointProtectedTEEComm)
        {
            pDdiCp = dynamic_cast<DdiCpNext*>(CreateDdiCpNext(&mosCtx));
            if (!pDdiCp)
            {
                DDI_CP_ASSERTMESSAGE("nullptr pDdiCp");
                MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, VA_STATUS_ERROR_ALLOCATION_FAILED);
                MOS_Delete(ddiCpContext);
                return VA_STATUS_ERROR_ALLOCATION_FAILED;
            }
            pDdiCp->UpdateCpContext(pSession->GetCpTag(), cpConfigAttr);
            ddiCpContext->pCpDdiInterface = pDdiCp;
        }

        ddiCpContext->pDrvPrivate = pSession;
        ddiCpContext->pMediaCtx = mediaCtx;

        MosUtilities::MosLockMutex(&mediaCtx->ProtMutex);
        contextHeapElement = MediaLibvaUtilNext::DdiAllocPVAContextFromHeap(mediaCtx->pProtCtxHeap);
        if (nullptr == contextHeapElement)
        {
            MosUtilities::MosUnlockMutex(&mediaCtx->ProtMutex);
            vaStatus = VA_STATUS_ERROR_MAX_NUM_EXCEEDED;
            break;
        }

        // offset uiVaContextID to 0x8000000
        contextHeapElement->uiVaContextID += DDI_MEDIA_VACONTEXTID_OFFSET_PROT_CP;
        contextHeapElement->pVaContext = (void*) ddiCpContext;
        mediaCtx->uiNumProts++;
        *protectedSession = (VAContextID)(contextHeapElement->uiVaContextID + DDI_MEDIA_SOFTLET_VACONTEXTID_CP_OFFSET);

        MosUtilities::MosUnlockMutex(&mediaCtx->ProtMutex);
    } while (false);

    if (vaStatus != VA_STATUS_SUCCESS)
    {
        if (contextHeapElement)
        {
            MediaLibvaUtilNext::DdiReleasePVAContextFromHeap(
                mediaCtx->pProtCtxHeap,
                contextHeapElement->uiVaContextID -= DDI_MEDIA_VACONTEXTID_OFFSET_PROT_CP
            );
        }
        MOS_Delete(pDdiCp);
        MOS_Delete(ddiCpContext);
    }

    if(cpCaps)
    {
        MOS_Delete(cpCaps);
    }
#endif
    return vaStatus;
}

VAStatus DdiCpFunctions::DestroyProtectedSession(
    VADriverContextP      ctx,
    VAProtectedSessionID  protectedSession)
{
    DDI_CP_FUNC_ENTER;

    DDI_CP_CHK_NULL(ctx, "nullptr driverCtx", VA_STATUS_ERROR_INVALID_CONTEXT);
    PDDI_MEDIA_CONTEXT mediaCtx = GetMediaContext(ctx);
    DDI_CP_CHK_NULL(mediaCtx, "nullptr mediaCtx", VA_STATUS_ERROR_INVALID_CONTEXT);

    uint32_t             ctxType      = DDI_MEDIA_CONTEXT_TYPE_NONE;
    PDDI_CP_CONTEXT_NEXT ddiCpContext = (PDDI_CP_CONTEXT_NEXT)GetContextFromProtectedSessionID(ctx, protectedSession, &ctxType);
    DDI_CP_CHK_NULL(ddiCpContext, "nullptr cpCtx", VA_STATUS_ERROR_INVALID_CONTEXT);

    if (ddiCpContext->pCpDdiInterface)
    {
        MOS_Delete(ddiCpContext->pCpDdiInterface);
    }

    ddiCpContext->pDrvPrivate = nullptr;

    // Free the context id from the context_heap earlier
    uint32_t cpIndex = (uint32_t)protectedSession & DDI_MEDIA_MASK_VAPROTECTEDSESSION_ID;
    MosUtilities::MosLockMutex(&mediaCtx->ProtMutex);
    ReleasePVAContextFromHeap(mediaCtx->pProtCtxHeap, cpIndex);
    mediaCtx->uiNumProts--;

    // when destroy cp session, find all the media context it attached before
    // and SetAttached(nullptr) for these media context
    decode::PDDI_DECODE_CONTEXT decCtx = nullptr;
    encode::PDDI_ENCODE_CONTEXT encCtx = nullptr;
    PDDI_VP_CONTEXT             vpCtx  = nullptr;
    DdiCpNext                  *pDdiCp = nullptr;
    if (!ddiCpContext->mapAttaching.empty())
    {
        for (auto itr = ddiCpContext->mapAttaching.begin(); itr != ddiCpContext->mapAttaching.end(); ++itr)
        {
            ctxType      = DDI_MEDIA_CONTEXT_TYPE_NONE;
            void *ctxPtr = MediaLibvaCommonNext::GetContextFromContextID(ctx, itr->first, &ctxType);
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
                        decCtx = (decode::PDDI_DECODE_CONTEXT)(ctxPtr);
                        pDdiCp = dynamic_cast<DdiCpNext*>(decCtx->pCpDdiInterfaceNext);
                        break;
                    case DDI_MEDIA_CONTEXT_TYPE_ENCODER:
                        encCtx = (encode::PDDI_ENCODE_CONTEXT)(ctxPtr);
                        pDdiCp = dynamic_cast<DdiCpNext*>(encCtx->pCpDdiInterfaceNext);
                        break;
                    case DDI_MEDIA_CONTEXT_TYPE_VP:
                        vpCtx  = (PDDI_VP_CONTEXT)(ctxPtr);
                        pDdiCp = dynamic_cast<DdiCpNext*>(vpCtx->pCpDdiInterfaceNext);
                        break;
                    default:
                        DDI_CP_ASSERTMESSAGE("DDI: unsupported context in DdiMediaProtectedContent::AttachProtectedSession.");
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

    MosUtilities::MosUnlockMutex(&mediaCtx->ProtMutex);
    return VA_STATUS_SUCCESS;
}

VAStatus DdiCpFunctions::AttachProtectedSession(
    VADriverContextP      ctx,
    VAContextID           context,
    VAProtectedSessionID  protectedSession)
{
    DDI_CP_FUNC_ENTER;

    VAStatus vaStatus = VA_STATUS_SUCCESS;
#ifdef VA_DRIVER_VTABLE_PROT_VERSION
    DDI_CP_CHK_NULL(ctx,      "nullptr driverCtx", VA_STATUS_ERROR_INVALID_CONTEXT);
    PDDI_MEDIA_CONTEXT mediaCtx  = GetMediaContext(ctx);

    uint32_t ctxCpType = DDI_MEDIA_CONTEXT_TYPE_NONE;
    void     *ctxCpPtr = GetContextFromProtectedSessionID(ctx, protectedSession, &ctxCpType);
    uint32_t ctxType   = DDI_MEDIA_CONTEXT_TYPE_NONE;
    void     *ctxPtr   = MediaLibvaCommonNext::GetContextFromContextID(ctx, context, &ctxType);

    PDDI_CP_CONTEXT_NEXT        cpCtx  = nullptr;
    decode::PDDI_DECODE_CONTEXT decCtx = nullptr;
    encode::PDDI_ENCODE_CONTEXT encCtx = nullptr;
    PDDI_VP_CONTEXT             vpCtx  = nullptr;
    DdiCpNext                  *pDdiCp = nullptr;

    DDI_CP_CHK_NULL(mediaCtx, "nullptr mediaCtx", VA_STATUS_ERROR_INVALID_CONTEXT);
    DDI_CP_CHK_NULL(ctxCpPtr, "nullptr ctxCpPtr", VA_STATUS_ERROR_INVALID_CONTEXT);
    DDI_CP_CHK_NULL(ctxPtr,   "nullptr ctxPtr",   VA_STATUS_ERROR_INVALID_CONTEXT);

    if (ctxCpType != DDI_MEDIA_CONTEXT_TYPE_PROTECTED_CONTENT)
    {
        DDI_CP_ASSERTMESSAGE("wrong cp_context type %d", ctxCpType);
        MT_ERR2(MT_ERR_INVALID_ARG, MT_CP_CTX_TYPE, ctxCpType, MT_FUNC_RET, VA_STATUS_ERROR_INVALID_CONTEXT);
        return VA_STATUS_ERROR_INVALID_CONTEXT;
    }

    do
    {
        cpCtx = GetCpContextNextFromPVOID(ctxCpPtr);
        CPavpCryptoSessionVANext *cpSession = dynamic_cast<CPavpCryptoSessionVANext*>(GetCryptoSession(cpCtx));
        DDI_CP_CHK_NULL(cpCtx->pCpDdiInterface, "nullptr pCpDdiInterface", VA_STATUS_ERROR_INVALID_CONTEXT);
        DDI_CP_CHK_NULL(cpSession,              "nullptr cp Session",      VA_STATUS_ERROR_INVALID_CONTEXT);

        switch (ctxType)
        {
            case DDI_MEDIA_CONTEXT_TYPE_DECODER:
                decCtx = (decode::PDDI_DECODE_CONTEXT)(ctxPtr);
                pDdiCp = dynamic_cast<DdiCpNext*>(decCtx->pCpDdiInterfaceNext);
                break;
            case DDI_MEDIA_CONTEXT_TYPE_ENCODER:
                encCtx = (encode::PDDI_ENCODE_CONTEXT)(ctxPtr);
                pDdiCp = dynamic_cast<DdiCpNext*>(encCtx->pCpDdiInterfaceNext);
                break;
            case DDI_MEDIA_CONTEXT_TYPE_VP:
                vpCtx  = (PDDI_VP_CONTEXT)(ctxPtr);
                pDdiCp = dynamic_cast<DdiCpNext*>(vpCtx->pCpDdiInterfaceNext);
                break;
            default:
                DDI_CP_ASSERTMESSAGE("DDI: unsupported context in DdiMediaProtectedContent::AttachProtectedSession.");
                MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_FUNC_RET, VA_STATUS_ERROR_INVALID_CONTEXT);
                return VA_STATUS_ERROR_INVALID_CONTEXT;
        }

        DDI_CP_CHK_NULL(pDdiCp, "nullptr pDdiCp", VA_STATUS_ERROR_INVALID_CONTEXT);

        MosUtilities::MosLockMutex(&mediaCtx->ProtMutex);
        if (pDdiCp->IsAttached())
        {
            DDI_CP_ASSERTMESSAGE("Already attached, ctxType=%d, ctxPtr=%p", ctxType, ctxPtr);
            vaStatus = VA_STATUS_ERROR_OPERATION_FAILED;
            MT_LOG3(MT_CP_ATTACH_SESSION, MT_NORMAL, MT_CP_CTX_TYPE, ctxType, MT_CP_CTX_PTR, (int64_t)ctxPtr, MT_FUNC_RET, vaStatus);
            MosUtilities::MosUnlockMutex(&mediaCtx->ProtMutex);
            break;
        }

        pDdiCp->SetAttached(cpSession);
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

        MosUtilities::MosUnlockMutex(&mediaCtx->ProtMutex);

    } while (false);
#endif

    return vaStatus;
}

VAStatus DdiCpFunctions::DetachProtectedSession(
    VADriverContextP  ctx,
    VAContextID       context)
{
    DDI_CP_FUNC_ENTER;

    VAStatus           vaStatus  = VA_STATUS_SUCCESS;
    DDI_CP_CHK_CONDITION(context == 0, "VAContextID is 0", vaStatus);
    
    DDI_CP_CHK_NULL(ctx,      "nullptr driverCtx", VA_STATUS_ERROR_INVALID_CONTEXT);
    PDDI_MEDIA_CONTEXT mediaCtx  = GetMediaContext(ctx);
    DDI_CP_CHK_NULL(mediaCtx, "nullptr mediaCtx", VA_STATUS_ERROR_INVALID_CONTEXT);

    uint32_t                    ctxType = DDI_MEDIA_CONTEXT_TYPE_NONE;
    void                       *ctxPtr  = MediaLibvaCommonNext::GetContextFromContextID(ctx, context, &ctxType);
    decode::PDDI_DECODE_CONTEXT decCtx  = nullptr;
    encode::PDDI_ENCODE_CONTEXT encCtx  = nullptr;
    PDDI_VP_CONTEXT             vpCtx   = nullptr;
    DdiCpNext                  *pDdiCp  = nullptr;
    DDI_CP_CHK_NULL(ctxPtr,   "nullptr ctxPtr",   VA_STATUS_ERROR_INVALID_CONTEXT);

    do
    {
        switch (ctxType)
        {
            case DDI_MEDIA_CONTEXT_TYPE_DECODER:
                decCtx = (decode::PDDI_DECODE_CONTEXT)ctxPtr;
                pDdiCp = dynamic_cast<DdiCpNext*>(decCtx->pCpDdiInterfaceNext);
                break;
            case DDI_MEDIA_CONTEXT_TYPE_ENCODER:
                encCtx = (encode::PDDI_ENCODE_CONTEXT)ctxPtr;
                pDdiCp = dynamic_cast<DdiCpNext*>(encCtx->pCpDdiInterfaceNext);
                break;
            case DDI_MEDIA_CONTEXT_TYPE_VP:
                vpCtx  = (PDDI_VP_CONTEXT)(ctxPtr);
                pDdiCp = dynamic_cast<DdiCpNext*>(vpCtx->pCpDdiInterfaceNext);
                break;
            default:
                DDI_CP_ASSERTMESSAGE("DDI: unsupported context in DdiCpNext::DetachProtectedSession.");
                MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_FUNC_RET, VA_STATUS_ERROR_INVALID_CONTEXT);
                return VA_STATUS_ERROR_INVALID_CONTEXT;
        }

        DDI_CP_CHK_NULL(pDdiCp,      "nullptr pDdiCp",     VA_STATUS_ERROR_INVALID_CONTEXT);

        MosUtilities::MosLockMutex(&mediaCtx->ProtMutex);
        if (!pDdiCp->IsAttached())
        {
            DDI_CP_ASSERTMESSAGE("Not attached, ctxType=%d, ctxPtr=%p", ctxType, ctxPtr);
            vaStatus  = VA_STATUS_ERROR_OPERATION_FAILED;
            MT_LOG3(MT_CP_DETACH_SESSION, MT_NORMAL, MT_CP_CTX_TYPE, ctxType, MT_CP_CTX_PTR, (int64_t)ctxPtr, MT_FUNC_RET, vaStatus);
            MosUtilities::MosUnlockMutex(&mediaCtx->ProtMutex);
            break;
        }

        pDdiCp->SetAttached(nullptr);
        pDdiCp->ResetCpContext();
        MosUtilities::MosUnlockMutex(&mediaCtx->ProtMutex);

    } while (false);

    return vaStatus;
}

VAStatus DdiCpFunctions::ProtectedSessionExecute(
    VADriverContextP      ctx,
    VAProtectedSessionID  protectedSession,
    VABufferID            data)
{
    DDI_CP_FUNC_ENTER;

    VAStatus vaStatus = VA_STATUS_SUCCESS;
#ifdef VA_DRIVER_VTABLE_PROT_VERSION
    DDI_CP_CHK_NULL(ctx,      "nullptr driverCtx", VA_STATUS_ERROR_INVALID_CONTEXT);
    uint32_t ctxType  = DDI_MEDIA_CONTEXT_TYPE_NONE;
    void     *ctxPtr  = GetContextFromProtectedSessionID(ctx, protectedSession, &ctxType);
    DDI_CP_CHK_NULL(ctxPtr,    "nullptr ctxPtr",   VA_STATUS_ERROR_INVALID_CONTEXT);

    if (ctxType != DDI_MEDIA_CONTEXT_TYPE_PROTECTED_CONTENT)
    {
        DDI_CP_ASSERTMESSAGE("wrong cp_context type %d", ctxType);
        MT_ERR2(MT_ERR_INVALID_ARG, MT_CP_CTX_TYPE, ctxType, MT_FUNC_RET, VA_STATUS_ERROR_INVALID_CONTEXT);
        return VA_STATUS_ERROR_INVALID_CONTEXT;
    }

    PDDI_CP_CONTEXT_NEXT cpCtx = GetCpContextNextFromPVOID(ctxPtr);
    DDI_CP_CHK_NULL(cpCtx,     "nullptr cpCtx",    VA_STATUS_ERROR_INVALID_CONTEXT);
    DDI_CP_CHK_NULL(cpCtx->pMediaCtx,  "NULL cpCtx->pMediaCtx",    VA_STATUS_ERROR_INVALID_CONTEXT);

    CPavpCryptoSessionVANext *session = GetCryptoSession(cpCtx);
    DDI_CP_CHK_NULL(session,           "NULL session",             VA_STATUS_ERROR_INVALID_CONTEXT);

    DDI_MEDIA_BUFFER *buffer = MediaLibvaCommonNext::GetBufferFromVABufferID(cpCtx->pMediaCtx, data);
    DDI_CP_CHK_NULL(buffer,            "NULL buf",                 VA_STATUS_ERROR_INVALID_BUFFER);
    DDI_CP_CHK_NULL(buffer->pData,     "NULL buf->pData",          VA_STATUS_ERROR_INVALID_BUFFER);

    vaStatus = session->Execute(buffer);
#endif

    return vaStatus;
}
#endif

VAStatus DdiCpFunctions::CreateBuffer(
    VADriverContextP  ctx,
    VAContextID       context,
    VABufferType      type,
    uint32_t          size,
    uint32_t          elementsNum,
    void              *data,
    VABufferID        *bufId)
{
    DDI_CP_FUNC_ENTER;

    DDI_CP_CHK_NULL(ctx,      "nullptr driverCtx", VA_STATUS_ERROR_INVALID_CONTEXT);
    uint32_t ctxType = DDI_MEDIA_CONTEXT_TYPE_NONE;
    void     *ctxPtr = GetContextFromProtectedSessionID(ctx, VAProtectedSessionID(context), &ctxType);

    DDI_CP_CHK_NULL(ctxPtr,    "nullptr ctxPtr",   VA_STATUS_ERROR_INVALID_CONTEXT);
    if (ctxType != DDI_MEDIA_CONTEXT_TYPE_PROTECTED_CONTENT)
    {
        DDI_CP_ASSERTMESSAGE("wrong cp_context type %d", ctxType);
        MT_ERR2(MT_ERR_INVALID_ARG, MT_CP_CTX_TYPE, ctxType, MT_FUNC_RET, VA_STATUS_ERROR_INVALID_CONTEXT);
        return VA_STATUS_ERROR_INVALID_CONTEXT;
    }

    PDDI_CP_CONTEXT_NEXT cpCtx = GetCpContextNextFromPVOID(ctxPtr); // Refactor later
    DDI_CP_CHK_NULL(cpCtx,             "NULL pCtx",                VA_STATUS_ERROR_INVALID_CONTEXT);
    DDI_CP_CHK_NULL(cpCtx->pMediaCtx,  "NULL cpCtx->pMediaCtx",    VA_STATUS_ERROR_INVALID_CONTEXT);

    PDDI_MEDIA_BUFFER              buf = nullptr;
    VAStatus                       vaStatus = VA_STATUS_SUCCESS;
    PDDI_MEDIA_BUFFER_HEAP_ELEMENT bufferHeapElement;
    MOS_STATUS                     status = MOS_STATUS_SUCCESS;

    do
    {
        *bufId = VA_INVALID_ID;
        buf = (DDI_MEDIA_BUFFER *)MOS_AllocAndZeroMemory(sizeof(DDI_MEDIA_BUFFER));
        DDI_CP_CHK_NULL(buf,           "NULL buf.", VA_STATUS_ERROR_ALLOCATION_FAILED);

        // allocate new buf and init
        buf->pMediaCtx     = cpCtx->pMediaCtx;
        buf->iSize         = size * elementsNum;
        buf->uiNumElements = elementsNum;
        buf->uiType        = type;
        buf->format        = Media_Format_CPU;
        buf->uiOffset      = 0;
        buf->pData         = (uint8_t*)MOS_AllocAndZeroMemory(size * elementsNum);
        if (nullptr == buf->pData)
        {
            MOS_FreeMemAndSetNull(buf);
            return VA_STATUS_ERROR_ALLOCATION_FAILED;
        }

        bufferHeapElement  = MediaLibvaUtilNext::AllocPMediaBufferFromHeap(cpCtx->pMediaCtx->pBufferHeap);
        if (nullptr == bufferHeapElement)
        {
            MOS_FreeMemAndSetNull(buf->pData);
            MOS_FreeMemAndSetNull(buf);
            DDI_CP_ASSERTMESSAGE("Invalid buffer index.");
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
            status = MOS_SecureMemcpy(buf->pData, size * elementsNum, data, size * elementsNum);
            if (status != MOS_STATUS_SUCCESS)
            {
                DDI_CP_ASSERTMESSAGE("DDI: Failed to copy client data!");
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

    return vaStatus;
}

void* DdiCpFunctions::GetContextFromProtectedSessionID(
    VADriverContextP     ctx,
    VAProtectedSessionID vaID,
    uint32_t             *ctxType)
{
    DDI_CP_FUNC_ENTER;

    PDDI_MEDIA_CONTEXT   mediaCtx = nullptr;
    uint32_t             heapIndex = 0, protIndex = 0;
    void                 *pSession = nullptr;

    DDI_CP_CHK_NULL(ctx,     "nullptr ctx",     nullptr);
    DDI_CP_CHK_NULL(ctxType, "nullptr ctxType", nullptr);

    mediaCtx  = GetMediaContext(ctx);
    protIndex = vaID & DDI_MEDIA_MASK_VACONTEXTID;
    heapIndex = vaID & DDI_MEDIA_MASK_VAPROTECTEDSESSION_ID;

    if ((vaID & DDI_MEDIA_MASK_VACONTEXT_TYPE) != DDI_MEDIA_SOFTLET_VACONTEXTID_CP_OFFSET)
    {
        DDI_CP_ASSERTMESSAGE("Invalid protected session: 0x%x", vaID);
        *ctxType = DDI_MEDIA_CONTEXT_TYPE_NONE;
        return nullptr;
    }

    // 0        ~ 0x8000000: LP context
    // 0x8000000~0x10000000: CP context
    if (protIndex < DDI_MEDIA_VACONTEXTID_OFFSET_PROT_CP)
    {
        DDI_VERBOSEMESSAGE("LP protected session detected: 0x%x", vaID);
        *ctxType = DDI_MEDIA_CONTEXT_TYPE_PROTECTED_LINK;
        return MediaLibvaCommonNext::GetVaContextFromHeap(mediaCtx->pProtCtxHeap, heapIndex, &mediaCtx->ProtMutex);
    }

    DDI_VERBOSEMESSAGE("CP protected session detected: 0x%x", vaID);
    *ctxType = DDI_MEDIA_CONTEXT_TYPE_PROTECTED_CONTENT;
    return MediaLibvaCommonNext::GetVaContextFromHeap(mediaCtx->pProtCtxHeap, heapIndex, &mediaCtx->ProtMutex);
}

void DdiCpFunctions::ReleasePVAContextFromHeap(PDDI_MEDIA_HEAP vaContextHeap, uint32_t vaContextID)
{
    DDI_CP_CHK_NULL(vaContextHeap, "nullptr vaContextHeap", );
    DDI_CHK_LESS(vaContextID, vaContextHeap->uiAllocatedHeapElements, "invalid context id", );

    PDDI_MEDIA_VACONTEXT_HEAP_ELEMENT vaContextHeapBase = (PDDI_MEDIA_VACONTEXT_HEAP_ELEMENT)vaContextHeap->pHeapBase;
    DDI_CP_CHK_NULL(vaContextHeapBase, "nullptr vaContextHeapBase", );

    PDDI_MEDIA_VACONTEXT_HEAP_ELEMENT vaContextHeapElmt = &vaContextHeapBase[vaContextID];
    DDI_CP_CHK_NULL(vaContextHeapElmt,             "Invalid vaContextHeapElmt", );
    DDI_CP_CHK_NULL(vaContextHeapElmt->pVaContext, "context is already released", );
    
    void *firstFree                        = vaContextHeap->pFirstFreeHeapElement;
    vaContextHeap->pFirstFreeHeapElement   = (void*)vaContextHeapElmt;
    vaContextHeapElmt->pNextFree           = (PDDI_MEDIA_VACONTEXT_HEAP_ELEMENT)firstFree;
    vaContextHeapElmt->pVaContext          = nullptr;
    vaContextHeapElmt->uiVaContextID       = vaContextID;
}

VAStatus DdiCpFunctions::CreateConfig (
    VADriverContextP  ctx,
    VAProfile         profile,
    VAEntrypoint      entrypoint,
    VAConfigAttrib    *attribList,
    int32_t           attribsNum,
    VAConfigID        *configId)
{
    DDI_CP_FUNC_ENTER;
    
    VAStatus status = VA_STATUS_SUCCESS;

    DDI_CP_CHK_NULL(ctx,      "nullptr ctx",      VA_STATUS_ERROR_INVALID_CONTEXT);
    DDI_CP_CHK_NULL(configId, "nullptr configId", VA_STATUS_ERROR_INVALID_PARAMETER);

    PDDI_MEDIA_CONTEXT mediaCtx = GetMediaContext(ctx);
    DDI_CP_CHK_NULL(mediaCtx,             "nullptr mediaCtx", VA_STATUS_ERROR_INVALID_CONTEXT);
    DDI_CP_CHK_NULL(mediaCtx->m_capsNext, "nullptr m_caps",   VA_STATUS_ERROR_INVALID_PARAMETER);

    /* CP No need to check common config*/

    // Get all config list
    auto configList = mediaCtx->m_capsNext->GetConfigList();
    DDI_CP_CHK_NULL(configList, "Get configList failed", VA_STATUS_ERROR_INVALID_PARAMETER);
    
    // Init input attrib list
    uint32_t cpSessionMode = VA_PC_SESSION_MODE_NONE;
    uint32_t cpSessionType = VA_PC_SESSION_TYPE_NONE;
    uint32_t cipherAlgo = VA_PC_CIPHER_AES;
    uint32_t cipherBlockSize = VA_PC_BLOCK_SIZE_128;
    uint32_t cipherMode = VA_PC_CIPHER_MODE_CTR;
    uint32_t cipherSampleType = VA_PC_SAMPLE_TYPE_FULLSAMPLE;
    uint32_t cpUsage = VA_PC_USAGE_DEFAULT;

    for(int i = 0; i < attribsNum; i++)
    {
        if(static_cast<int>(attribList[i].type) == VAConfigAttribProtectedContentSessionMode)
        {
            cpSessionMode = attribList[i].value;
        }

        else if(static_cast<int>(attribList[i].type) == VAConfigAttribProtectedContentSessionType)
        {
            cpSessionType = attribList[i].value;
        }

        else if(attribList[i].type == VAConfigAttribProtectedContentCipherAlgorithm)
        {
            cipherAlgo = attribList[i].value;
        }

        else if(attribList[i].type == VAConfigAttribProtectedContentCipherBlockSize)
        {
            cipherBlockSize = attribList[i].value;
        }

        else if(attribList[i].type == VAConfigAttribProtectedContentCipherMode)
        {
            cipherMode = attribList[i].value;
        }

        else if(attribList[i].type == VAConfigAttribProtectedContentCipherSampleType)
        {
            cipherSampleType = attribList[i].value;
        }
        
        else if(attribList[i].type == VAConfigAttribProtectedContentUsage)
        {
            cpUsage = attribList[i].value;
        }
    }

    if (entrypoint == VAEntrypointProtectedTEEComm)
    {
        cipherAlgo       = 0;
        cipherBlockSize  = 0;
        cipherMode       = 0;
        cipherSampleType = 0;
        cpUsage          = 0;
    }

    DdiCpCapsInterface *cpCaps = CreateDdiCpCaps();
    DDI_CP_CHK_NULL(cpCaps,     "nullptr cpCaps", VA_STATUS_ERROR_INVALID_PARAMETER);

    do
    {
        status = VA_STATUS_ERROR_ATTR_NOT_SUPPORTED;
        *configId = 0xFFFFFFFF;

        // Compare and get config
        for(int i = 0; i < configList->size(); i++)
        {
            if((configList->at(i).profile == profile) && (configList->at(i).entrypoint == entrypoint))
            {
                if( (configList->at(i).componentData.data.sessionMode == cpSessionMode)   &&
                    (configList->at(i).componentData.data.sessionType == cpSessionType)   &&
                    (configList->at(i).componentData.data.algorithm == cipherAlgo)        &&
                    (configList->at(i).componentData.data.blockSize == cipherBlockSize)   &&
                    (configList->at(i).componentData.data.counterMode == cipherMode)      &&
                    (configList->at(i).componentData.data.sampleType == cipherSampleType) &&
                    (configList->at(i).componentData.data.usage == cpUsage))
                {
                    uint32_t curConfigID = ADD_CONFIG_ID_CP_OFFSET(i);

                    if(!cpCaps->IsCpConfigId(curConfigID))
                    {
                        *configId = 0xFFFFFFFF;
                        DDI_CODEC_ASSERTMESSAGE("DDI: Invalid configID.");
                        status = VA_STATUS_ERROR_ATTR_NOT_SUPPORTED;
                        break;
                    }

                    *configId = curConfigID;
                    status = VA_STATUS_SUCCESS;
                    break;
                }
            }
        }

    } while (false);
    
    MOS_Delete(cpCaps);
    return status;
}
