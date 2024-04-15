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
//! \file      cp_pavp_cryptosession_va_next.cpp
//! \brief     libva(and its extension) crypto session implementation
//!

// TODO: Clean up HRESULT in this class when CPavpDevice clean done

#include "va_protected_content_private.h"
#include "media_libva_util_next.h"
#include "ddi_libva_decoder_specific.h"
#include "ddi_cp_next.h"
#include "cp_os_services.h"
#include "pavp_oem_policy_api.h"
#include "cp_pavp_cryptosession_va_next.h"

#ifdef VA_DRIVER_VTABLE_PROT_VERSION

CPavpCryptoSessionVANext::CPavpCryptoSessionVANext(
    PDDI_CP_CONTEXT_NEXT cpContext,
    DDI_CP_CONFIG_ATTR   attr)
    : CPavpDeviceNewSM()
{
    DDI_CP_FUNC_ENTER;

    m_CpContext = cpContext;
    m_Attr      = attr;
}

VAStatus CPavpCryptoSessionVANext::Setup(PAVP_DEVICE_CONTEXT context)
{
    DDI_CP_FUNC_ENTER;

    VAStatus                 va = VA_STATUS_SUCCESS;
    HRESULT                  hr = S_OK;
    PAVP_SESSION_MODE        session_mode;
    PAVP_SESSION_TYPE        session_type;
    PAVP_ENCRYPTION_TYPE     encryption_type;
    PAVP_CRYPTO_SESSION_TYPE crypto_session_type;

    DDI_CP_CHK_NULL(m_CpContext, "Null m_CpContext", VA_STATUS_ERROR_INVALID_PARAMETER);

    do
    {
        session_mode        = DdiCpUtils::_GetSessionMode(m_Attr.uiSessionMode);
        session_type        = DdiCpUtils::_GetSessionType(m_Attr.uiSessionType);
        encryption_type     = DdiCpUtils::_GetEncryptionType(m_Attr.uiCipherBlockSize, m_Attr.uiCipherMode);
        crypto_session_type = DdiCpUtils::_GetCryptoSessionType(m_Attr.entrypoint, m_Attr.uiSessionMode, m_Attr.uiSessionType, m_Attr.uiUsage);

        m_eDrmType = DdiCpUtils::_GetDrmType(m_Attr.uiUsage);

        // Set the session type and crypto type in use.
        hr = SetEncryptionType(encryption_type);
        if (FAILED(hr))
        {
            DDI_CP_ASSERTMESSAGE("Call to SetEncryptionType() Failed. Error code = 0x%x", hr);
            MT_ERR2(MT_CP_ENCRYPT_TYPE_SET, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        hr = SetSessionType(session_type);
        if (FAILED(hr))
        {
            DDI_CP_ASSERTMESSAGE("Call to SetSessionType() Failed. Error code = 0x%x", hr);
            MT_ERR2(MT_CP_SESSION_TYPE_SET, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        hr = SetSessionMode(session_mode);
        if (FAILED(hr))
        {
            DDI_CP_ASSERTMESSAGE("Call to SetSessionMode() Failed. Error code = 0x%x", hr);
            MT_ERR2(MT_CP_SESSION_MODE_SET, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        hr = Init(context);
        if (FAILED(hr))
        {
            DDI_CP_ASSERTMESSAGE("Call to Init() Failed. Error code = 0x%x", hr);
            MT_ERR2(MT_CP_SESSION_INIT, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

#if (_DEBUG || _RELEASE_INTERNAL)
        if (crypto_session_type == PAVP_CRYPTO_SESSION_TYPE_PAVP_MECOMM_FW_APP ||
            crypto_session_type == PAVP_CRYPTO_SESSION_TYPE_WIDI_MECOMM_FW_APP)
        {
            // even though MECOMM doesn't create actual hw session, we think it's completed and alive after this call.
            SetHwSessionAlive();
        }
        else
#endif
        if (crypto_session_type == PAVP_CRYPTO_SESSION_TYPE_PAVP_MECOMM)
        {
            // even though MECOMM doesn't create actual hw session, we think it's completed and alive after this call.
            SetHwSessionAlive();
        }
        else if (crypto_session_type == PAVP_CRYPTO_SESSION_TYPE_NONE)
        {
            // Do nothing for PAVP_CRYPTO_SESSION_TYPE_NONE (it could be audio or video session later)
            // because we postpone the creation of actual hw session in HwSessionUpdate()
        }
        else
        {
            hr = InitHwSession();
            if (FAILED(hr))
            {
                DDI_CP_ASSERTMESSAGE("Failed to InitHwSession() hr=0x%x", hr);
                MT_ERR2(MT_CP_SESSION_INIT, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }
            SetHwSessionAlive();
        }

    } while (false);

    if (hr != S_OK)
    {
        va = VA_STATUS_ERROR_OPERATION_FAILED;
    }

    return va;
}

CPavpCryptoSessionVANext::~CPavpCryptoSessionVANext()
{
    DDI_CP_FUNC_ENTER;
}

VAStatus CPavpCryptoSessionVANext::EncryptionBlt(
    uint8_t *srcResource,
    uint8_t *dstResource,
    uint32_t width,
    uint32_t height)
{
    DDI_CP_FUNC_ENTER;

    VAStatus                va                  = VA_STATUS_SUCCESS;
    HRESULT                 hr                  = S_OK;
    MOS_STATUS              eStatus             = MOS_STATUS_UNKNOWN;
    uint32_t                size                = width * height;
    bool                    bDstLocked          = false;
    PAVP_ENCRYPT_BLT_PARAMS encryptionBltParams = {0, 0, 0, {0}};
    uint8_t                *pResourceData       = nullptr;

    do
    {
        // Validate input parameters.
        if (srcResource == nullptr ||
            dstResource == nullptr ||
            width == 0 ||  // Cannot be negative.
            height == 0)   // Cannot be negative.
        {
            DDI_CP_ASSERTMESSAGE("Invalid parameter.");
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, VA_STATUS_ERROR_INVALID_PARAMETER);
            return VA_STATUS_ERROR_INVALID_PARAMETER;
        }

        // Verify that the needed MOS interface is valid.
        if (m_pOsServices == nullptr ||
            m_pOsServices->m_pOsInterface == nullptr ||
            m_pOsServices->m_pOsInterface->pfnUnlockResource == nullptr)
        {
            DDI_CP_ASSERTMESSAGE("MOS interface is invalid.");
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, VA_STATUS_ERROR_OPERATION_FAILED);
            return VA_STATUS_ERROR_OPERATION_FAILED;
        }

        // Allocate the source resource.
        hr = m_pOsServices->AllocateLinearResource(
            MOS_GFXRES_BUFFER,
            size,
            1,
            "PAVPEncryptionBltSrc",
            Format_Buffer,
            encryptionBltParams.SrcResource);
        if (FAILED(hr))
        {
            DDI_CP_ASSERTMESSAGE("Allocation of the source resource failed. Error = 0x%x.", hr);
            va = VA_STATUS_ERROR_ALLOCATION_FAILED;
            MT_ERR2(MT_CP_RESOURCE_ALLOC, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        // Allocate the destination resource.
        hr = m_pOsServices->AllocateLinearResource(
            MOS_GFXRES_BUFFER,
            size,
            1,
            "PAVPEncryptionBltDst",
            Format_Buffer,
            encryptionBltParams.DstResource);
        if (FAILED(hr))
        {
            DDI_CP_ASSERTMESSAGE("Allocation of the destination resource failed. Error = 0x%x.", hr);
            va = VA_STATUS_ERROR_ALLOCATION_FAILED;
            MT_ERR2(MT_CP_RESOURCE_ALLOC, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, VA_STATUS_ERROR_ALLOCATION_FAILED);
            break;
        }

        // Fill in the src resource with the input buffer.
        hr = m_pOsServices->CopyToLinearBufferResource(
            *(encryptionBltParams.SrcResource),
            srcResource,
            size);
        if (FAILED(hr))
        {
            DDI_CP_ASSERTMESSAGE("Failed to copy memory to SrcResource");
            hr = E_OUTOFMEMORY;
            MT_ERR2(MT_CP_MEM_COPY, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        if (encryptionBltParams.DstResource == nullptr ||
            encryptionBltParams.DstResource->pGmmResInfo == nullptr)
        {
            DDI_CP_ASSERTMESSAGE("Invalid pGmmResInfo.");
            va = VA_STATUS_ERROR_OPERATION_FAILED;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, VA_STATUS_ERROR_OPERATION_FAILED);
            break;
        }

        encryptionBltParams.dwDstResourceSize = size;

        hr = DoEncryptionBlt(encryptionBltParams);
        if (FAILED(hr))
        {
            DDI_CP_ASSERTMESSAGE("DoEncryptionBlt failed. Error = 0x%x.", hr);
            va = VA_STATUS_ERROR_OPERATION_FAILED;
            MT_ERR2(MT_CP_ENCRYPTION_BLT_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        // Fill in the output buffer with the output data.
        pResourceData = static_cast<uint8_t *>(
            m_pOsServices->LockResource(*(encryptionBltParams.DstResource), CPavpOsServices::LockType::Read));
        if (pResourceData == nullptr)
        {
            DDI_CP_ASSERTMESSAGE("Error locking the destination resource.");
            va = VA_STATUS_ERROR_OPERATION_FAILED;
            MT_ERR2(MT_CP_RESOURCE_LOCK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, VA_STATUS_ERROR_OPERATION_FAILED);
            break;
        }
        bDstLocked = true;

        eStatus = MOS_SecureMemcpy(
            dstResource,
            size,
            pResourceData,
            size);
        if (eStatus != MOS_STATUS_SUCCESS)
        {
            DDI_CP_ASSERTMESSAGE("MOS_SecureMemcpy failed with error = %d.", eStatus);
            va = VA_STATUS_ERROR_OPERATION_FAILED;
            MT_ERR2(MT_CP_MEM_COPY, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, eStatus);
            break;
        }

    } while (false);

    if (m_pOsServices != nullptr &&
        m_pOsServices->m_pOsInterface != nullptr &&
        m_pOsServices->m_pOsInterface->pfnUnlockResource != nullptr)
    {
        // Unlock the locked resources.
        if (bDstLocked)
        {
            m_pOsServices->m_pOsInterface->pfnUnlockResource(
                m_pOsServices->m_pOsInterface,
                encryptionBltParams.DstResource);
        }

        // Free the allocated resources.
        if (encryptionBltParams.SrcResource != nullptr &&
            !Mos_ResourceIsNull(encryptionBltParams.SrcResource))
        {
            m_pOsServices->FreeResource(encryptionBltParams.SrcResource);
        }

        if (encryptionBltParams.DstResource != nullptr &&
            !Mos_ResourceIsNull(encryptionBltParams.DstResource))
        {
            m_pOsServices->FreeResource(encryptionBltParams.DstResource);
        }
    }

    return va;
}

VAStatus CPavpCryptoSessionVANext::DecryptionBlt(
    uint8_t                 *srcResource,
    uint8_t                 *dstResource,
    uint32_t                 width,
    uint32_t                 height,
    uint32_t                 numSegment,
    VAEncryptionSegmentInfo *segments,
    uint32_t                 keyBlobSize)
{
    DDI_CP_FUNC_ENTER;

    VAStatus                va                  = VA_STATUS_SUCCESS;
    HRESULT                 hr                  = S_OK;
    MOS_STATUS              eStatus             = MOS_STATUS_UNKNOWN;
    uint8_t                *pResourceData       = nullptr;
    bool                    bDstLocked          = false;
    uint32_t                size                = width * height;
    uint32_t                size_aligned        = 0;
    PAVP_DECRYPT_BLT_PARAMS DecryptionBltParams = {0, 0, 0, {}, 0, 0, 0, 0};

    do
    {
        // Validate input parameters.
        if (srcResource == nullptr ||
            dstResource == nullptr ||
            width == 0 ||       // Cannot be negative.
            height == 0 ||      // Cannot be negative.
            numSegment == 0 ||  // We need at least 1 segment
            segments == nullptr)
        {
            DDI_CP_ASSERTMESSAGE("Invalid parameter.");
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, VA_STATUS_ERROR_INVALID_PARAMETER);
            return VA_STATUS_ERROR_INVALID_PARAMETER;
        }

        if (numSegment > 1)
        {
            // currently only implement 1 segment
            return VA_STATUS_ERROR_UNIMPLEMENTED;
        }

        if (keyBlobSize == 0)
        {
            keyBlobSize = 16;
        }

        MOS_SecureMemcpy(DecryptionBltParams.pIV,
            sizeof(DecryptionBltParams.pIV),
            segments->aes_cbc_iv_or_ctr,
            keyBlobSize);

        // Verify that the needed MOS interface is valid.
        if (m_pOsServices == nullptr ||
            m_pOsServices->m_pOsInterface == nullptr ||
            m_pOsServices->m_pOsInterface->pfnUnlockResource == nullptr)
        {
            DDI_CP_ASSERTMESSAGE("MOS interface is invalid.");
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, VA_STATUS_ERROR_INVALID_PARAMETER);
            return VA_STATUS_ERROR_INVALID_PARAMETER;
        }

        if (GetEncryptionType() == PAVP_ENCRYPTION_AES128_CBC)
        {
            DecryptionBltParams.dwAesMode = CRYPTO_COPY_AES_MODE_CBC_SKIP;

            // find if there is a partial block at the end
            size_aligned = size & ~(AES_BLOCK_SIZE - 1);
            if (size != size_aligned)
            {
                // CBC mode should just copy a partial block at the end.
                eStatus = MOS_SecureMemcpy(
                    dstResource + size_aligned,
                    size - size_aligned,
                    srcResource + size_aligned,
                    size - size_aligned);
                if (eStatus != MOS_STATUS_SUCCESS)
                {
                    DDI_CP_ASSERTMESSAGE("MOS_SecureMemcpy failed with error = %d.", eStatus);
                    va = VA_STATUS_ERROR_OPERATION_FAILED;
                    MT_ERR2(MT_CP_MEM_COPY, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, eStatus);
                    break;
                }
            }
            if (size_aligned == 0)
            {
                // if size aligned to AES_BLOCK_SIZE is 0 which means the input size is less than one
                // block and already handled in partial block in previous statement
                va = VA_STATUS_SUCCESS;
                break;
            }
            size = size_aligned;

            if (segments->segment_length == 0)
            {
                // CBC should go this case for that CBC mode setting is in DW10.AesOperationType
                // only be set when bPartialDecryption is TRUE
                DecryptionBltParams.bPartialDecryption = TRUE;
                DecryptionBltParams.uiClearBytes       = 0;
                DecryptionBltParams.uiEncryptedBytes   = size;
            }
        }
        else
        {
            DecryptionBltParams.dwAesMode = CRYPTO_COPY_AES_MODE_CTR;

            // The GPU-CP DDI provides the counter in big endian, but the hardware expects little endian.
            // But of course it's backwards for CBC, so don't swap in that case.
            CPavpDevice::CounterSwapEndian(DecryptionBltParams.pIV);
        }

        if (segments->segment_length > 0)
        {
            uint32_t clear_bytes   = segments->init_byte_length;
            uint32_t encrypt_bytes = segments->segment_length - segments->init_byte_length;

            if (encrypt_bytes == 0 && clear_bytes > 0)
            {
                va = VA_STATUS_ERROR_INVALID_PARAMETER;
                break;
            }

            DecryptionBltParams.bPartialDecryption = TRUE;
            DecryptionBltParams.uiClearBytes       = clear_bytes;
            DecryptionBltParams.uiEncryptedBytes   = encrypt_bytes;
        }

        // Allocate the source resource.
        hr = m_pOsServices->AllocateLinearResource(
            MOS_GFXRES_BUFFER,
            size,
            1,
            "PAVPDecryptoinBltSrc",
            Format_Buffer,
            DecryptionBltParams.SrcResource);
        if (FAILED(hr))
        {
            DDI_CP_ASSERTMESSAGE("Allocation of the source resource failed. Error = 0x%x.", hr);
            va = VA_STATUS_ERROR_ALLOCATION_FAILED;
            MT_ERR2(MT_CP_RESOURCE_ALLOC, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        // Allocate the destination resource.
        hr = m_pOsServices->AllocateLinearResource(
            MOS_GFXRES_BUFFER,
            size,
            1,
            "PAVPDecryptoinBltDst",
            Format_Buffer,
            DecryptionBltParams.DstResource);
        if (FAILED(hr))
        {
            DDI_CP_ASSERTMESSAGE("Allocation of the destination resource failed. Error = 0x%x.", hr);
            va = VA_STATUS_ERROR_ALLOCATION_FAILED;
            MT_ERR2(MT_CP_RESOURCE_ALLOC, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        hr = m_pOsServices->CopyToLinearBufferResource(
            *(DecryptionBltParams.SrcResource),
            srcResource,
            size);
        if (FAILED(hr))
        {
            DDI_CP_ASSERTMESSAGE("Failed to copy memory to SrcResource");
            hr = E_OUTOFMEMORY;
            MT_ERR2(MT_CP_MEM_COPY, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        if (DecryptionBltParams.DstResource == nullptr ||
            DecryptionBltParams.DstResource->pGmmResInfo == nullptr)
        {
            DDI_CP_ASSERTMESSAGE("DstResource is nullptr.");
            va = VA_STATUS_ERROR_OPERATION_FAILED;
            MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, VA_STATUS_ERROR_OPERATION_FAILED);
            break;
        }

        DecryptionBltParams.dwSrcResourceSize = size;

        hr = DoDecryptionBlt(DecryptionBltParams);
        if (FAILED(hr))
        {
            DDI_CP_ASSERTMESSAGE("DoDecryptionBlt failed. Error = 0x%x.", hr);
            va = VA_STATUS_ERROR_OPERATION_FAILED;
            MT_ERR2(MT_CP_FUNC_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        // Fill in the output buffer with the output data.
        pResourceData = static_cast<uint8_t *>(
            m_pOsServices->LockResource(*(DecryptionBltParams.DstResource), CPavpOsServices::LockType::Read));
        if (pResourceData == nullptr)
        {
            DDI_CP_ASSERTMESSAGE("Error locking the destination resource.");
            va = VA_STATUS_ERROR_OPERATION_FAILED;
            MT_ERR2(MT_CP_RESOURCE_LOCK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, va);
            break;
        }
        bDstLocked = true;

        eStatus = MOS_SecureMemcpy(
            dstResource,
            size,
            pResourceData,
            size);
        if (eStatus != MOS_STATUS_SUCCESS)
        {
            DDI_CP_ASSERTMESSAGE("MOS_SecureMemcpy failed with error = %d.", eStatus);
            va = VA_STATUS_ERROR_OPERATION_FAILED;
            MT_ERR2(MT_CP_MEM_COPY, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, eStatus);
            break;
        }

    } while (false);

    if (m_pOsServices != nullptr &&
        m_pOsServices->m_pOsInterface != nullptr &&
        m_pOsServices->m_pOsInterface->pfnUnlockResource != nullptr)
    {
        // Unlock the locked resources.
        if (bDstLocked)
        {
            m_pOsServices->m_pOsInterface->pfnUnlockResource(
                m_pOsServices->m_pOsInterface,
                DecryptionBltParams.DstResource);
        }

        // Free the allocated resources.
        if (DecryptionBltParams.SrcResource != nullptr &&
            !Mos_ResourceIsNull(DecryptionBltParams.SrcResource))
        {
            m_pOsServices->FreeResource(DecryptionBltParams.SrcResource);
        }

        if (DecryptionBltParams.DstResource != nullptr &&
            !Mos_ResourceIsNull(DecryptionBltParams.DstResource))
        {
            m_pOsServices->FreeResource(DecryptionBltParams.DstResource);
        }
    }

    return va;
}

VAStatus CPavpCryptoSessionVANext::Execute(DDI_MEDIA_BUFFER *buffer)
{
    DDI_CP_FUNC_ENTER;

    VAStatus                         va        = VA_STATUS_SUCCESS;
    VAProtectedSessionExecuteBuffer *exec_buff = nullptr;

    MOS_TraceEventExt(EVENT_PPED_FW, EVENT_TYPE_START, nullptr, 0, nullptr, 0);

    do
    {
        DDI_CP_CHK_NULL(buffer, "Null buffer", VA_STATUS_ERROR_INVALID_BUFFER);
        DDI_CP_CHK_NULL(buffer->pData, "Null buffer->pData", VA_STATUS_ERROR_INVALID_BUFFER);

        exec_buff = reinterpret_cast<VAProtectedSessionExecuteBuffer *>(buffer->pData);

        if (IsVAGpuFunctionID(exec_buff->function_id))
        {
            if (m_Attr.entrypoint == VAEntrypointProtectedContent)
            {
                va = ProcessingGpuFunction(exec_buff);
            }
            else
            {
                va = VA_STATUS_ERROR_OPERATION_FAILED;
            }
        }
        else if (IsVATeeFunctionID(exec_buff->function_id))
        {
            va = ProcessingTeeFunction(exec_buff);
        }
        else
        {
            DDI_CP_ASSERTMESSAGE("Unknown function_id %x", exec_buff->function_id);
            va = VA_STATUS_ERROR_INVALID_PARAMETER;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, va);
        }

    } while (false);

    MOS_TraceEventExt(EVENT_PPED_FW, EVENT_TYPE_END, &va, sizeof(va), nullptr, 0);

    return va;
}

bool CPavpCryptoSessionVANext::IsSessionAlive()
{
    DDI_CP_FUNC_ENTER;

    HRESULT hr              = S_OK;
    BOOL    bIsSessionAlive = FALSE;

    hr = IsSessionInitialized(bIsSessionAlive);
    if (FAILED(hr))
    {
        DDI_CP_ASSERTMESSAGE("IsSessionInitialized failed. Error = 0x%x.", hr);
        MT_ERR2(MT_CP_SESSION_INIT, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
    }

    return bIsSessionAlive ? true : false;
}

VAStatus CPavpCryptoSessionVANext::ProcessingGpuFunction(VAProtectedSessionExecuteBuffer *exec_buff)
{
    DDI_CP_FUNC_ENTER;

    VAStatus va = VA_STATUS_SUCCESS;
    HRESULT  hr = S_OK;

    DDI_CP_CHK_NULL(exec_buff, "exec_buff is nullptr", VA_STATUS_ERROR_INVALID_PARAMETER);

    do
    {
        switch (exec_buff->function_id)
        {
        case VA_TEE_EXEC_GPU_FUNCID_ENCRYPTION_BLT: {
            DDI_CP_VERBOSEMESSAGE("'Encryption Blt' propietary function requested.");

            if (!IsSessionAlive())
            {
                DDI_CP_ASSERTMESSAGE("Session not alive");
                va = VA_STATUS_ERROR_OPERATION_FAILED;
                MT_ERR2(MT_CP_SESSION_NOT_ALIVE, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, va);
                break;
            }

            VA_PROTECTED_BLT_PARAMS *pEncryptionBltParams =
                static_cast<VA_PROTECTED_BLT_PARAMS *>(exec_buff->input.data);

            hr = EncryptionBlt(
                pEncryptionBltParams->src_resource,
                pEncryptionBltParams->dst_resource,
                pEncryptionBltParams->width,
                pEncryptionBltParams->height);

            if (FAILED(hr))
            {
                DDI_CP_ASSERTMESSAGE("Failed to execute the function 0x%x. Error code = 0x%x", exec_buff->function_id, hr);
                va = VA_STATUS_ERROR_OPERATION_FAILED;
                MT_ERR2(MT_CP_FUNC_FAIL, MT_CP_FUNC_ID, exec_buff->function_id, MT_ERROR_CODE, hr);
            }

            break;
        }

        case VA_TEE_EXEC_GPU_FUNCID_DECRYPTION_BLT: {
            DDI_CP_VERBOSEMESSAGE("'Decryption Blt' propietary function requested.");

            if (!IsSessionAlive())
            {
                DDI_CP_ASSERTMESSAGE("Session not alive");
                va = VA_STATUS_ERROR_OPERATION_FAILED;
                MT_ERR2(MT_CP_SESSION_NOT_ALIVE, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, va);
                break;
            }

            uint32_t                 iv[4];
            uint32_t                 iv_length  = 0;
            uint32_t                 numSegment = 0;
            VAEncryptionSegmentInfo *segments   = nullptr;
            VA_PROTECTED_BLT_PARAMS *pDecryptionBltParams =
                static_cast<VA_PROTECTED_BLT_PARAMS *>(exec_buff->input.data);
            VAEncryptionParameters *enc_params = pDecryptionBltParams->enc_params;

            DDI_CP_CHK_NULL(pDecryptionBltParams, "pDecryptionBltParams is nullptr", VA_STATUS_ERROR_INVALID_PARAMETER);
            DDI_CP_CHK_NULL(enc_params, "enc_params is nullptr", VA_STATUS_ERROR_INVALID_PARAMETER);

            if (enc_params->num_segments > 0)
            {
                DDI_CP_CHK_NULL(enc_params->segment_info, "segment_info is nullptr", VA_STATUS_ERROR_INVALID_PARAMETER);

                numSegment = enc_params->num_segments;
                segments   = enc_params->segment_info;
            }

            hr = DecryptionBlt(
                pDecryptionBltParams->src_resource,
                pDecryptionBltParams->dst_resource,
                pDecryptionBltParams->width,
                pDecryptionBltParams->height,
                numSegment,
                segments,
                enc_params->key_blob_size);

            if (FAILED(hr))
            {
                DDI_CP_ASSERTMESSAGE("Failed to execute the function 0x%x. Error code = 0x%x", exec_buff->function_id, hr);
                va = VA_STATUS_ERROR_OPERATION_FAILED;
                MT_ERR2(MT_CP_FUNC_FAIL, MT_CP_FUNC_ID, exec_buff->function_id, MT_ERROR_CODE, hr);
            }

            break;
        }

        case VA_TEE_EXEC_TEE_FUNCID_HW_UPDATE: {
            DDI_CP_VERBOSEMESSAGE("'HW Update' propietary function requested.");

            va = HwSessionUpdate(exec_buff);
            if (va)
            {
                DDI_CP_ASSERTMESSAGE("VA_TEE_EXEC_TEE_FUNCID_HW_UPDATE fail %d", va);
                MT_ERR2(MT_CP_FUNC_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, va);
                break;
            }

            if (m_Attr.entrypoint != VAEntrypointProtectedTEEComm)
            {
                DdiCpNext *pDdiCp = dynamic_cast<DdiCpNext *>(m_CpContext->pCpDdiInterface);
                if (!pDdiCp)
                {
                    DDI_CP_ASSERTMESSAGE("Can't get DdiCpNext*");
                    va = VA_STATUS_ERROR_OPERATION_FAILED;
                    MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, va);
                    break;
                }

                pDdiCp->UpdateCpContext(GetCpTag(), m_Attr);
            }
            break;
        }

        case VA_TEE_EXEC_GPU_FUNCID_SET_STREAM_KEY: {
            DDI_CP_VERBOSEMESSAGE("'Set Stream Key' propietary function requested.");

            if (!IsSessionAlive())
            {
                DDI_CP_ASSERTMESSAGE("Session not alive");
                va = VA_STATUS_ERROR_OPERATION_FAILED;
                MT_ERR2(MT_CP_SESSION_NOT_ALIVE, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, va);
                break;
            }

            PAVP_SET_STREAM_KEY_PARAMS *pSetStreamKeyParams =
                static_cast<PAVP_SET_STREAM_KEY_PARAMS *>(exec_buff->input.data);

            if (pSetStreamKeyParams == nullptr ||
                exec_buff->input.data_size < sizeof(PAVP_SET_STREAM_KEY_PARAMS))
            {
                DDI_CP_ASSERTMESSAGE("Invalid set stream key parameters.");
                va = VA_STATUS_ERROR_INVALID_PARAMETER;
                MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, va);
                break;
            }

            // Linux DDI layer will only get KB based blob for setstreamkey so set the translation as false
            hr = SetStreamKey(*pSetStreamKeyParams, false);
            if (FAILED(hr))
            {
                DDI_CP_ASSERTMESSAGE("Failed to execute the function 0x%x. Error code = 0x%x", exec_buff->function_id, hr);
                va = VA_STATUS_ERROR_OPERATION_FAILED;
                MT_ERR2(MT_CP_FUNC_FAIL, MT_CP_FUNC_ID, exec_buff->function_id, MT_ERROR_CODE, hr);
            }

            break;
        }

        case VA_TEE_EXEC_GPU_FUNCID_GET_SESSION_ID: {
            DDI_CP_VERBOSEMESSAGE("'Get Session ID' proprietary function requested.");

            // Validate the input struct.
            if (exec_buff->output.data == nullptr ||
                exec_buff->output.data_size < sizeof(uint8_t))
            {
                DDI_CP_ASSERTMESSAGE("Invalid get session ID parameters.");
                va = VA_STATUS_ERROR_INVALID_PARAMETER;
                MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, va);
                break;
            }

            memset(exec_buff->output.data, 0, exec_buff->output.data_size);
            *reinterpret_cast<uint8_t *>(exec_buff->output.data) = GetSessionId();

            break;
        }

        case VA_TEE_EXEC_GPU_FUNCID_RESERVE_SESSION_SLOT: {
            DDI_CP_VERBOSEMESSAGE("'Reserve session slot' proprietary function requested.");

            // Validate the input struct.
            if (exec_buff->output.data == nullptr ||
                exec_buff->output.data_size < sizeof(uint8_t))
            {
                DDI_CP_ASSERTMESSAGE("Invalid reserve session slot parameters.");
                va = VA_STATUS_ERROR_INVALID_PARAMETER;
                MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, va);
                break;
            }

            uint32_t appId = 0xff;
            HRESULT  hr    = ReserveSessionSlot(&appId);
            if (hr != S_OK)
            {
                va = VA_STATUS_ERROR_INVALID_PARAMETER;
                break;
            }

            memset(exec_buff->output.data, 0, exec_buff->output.data_size);
            *reinterpret_cast<uint8_t *>(exec_buff->output.data) = appId;

            break;
        }

        case VA_TEE_EXEC_GPU_FUNCID_GET_SESSION_HANDLE: {
            DDI_CP_VERBOSEMESSAGE("'Get Session Handle' proprietary function requested.");

            // Validate the input struct.
            if (exec_buff->output.data == nullptr ||
                exec_buff->output.data_size < sizeof(unsigned long))
            {
                DDI_CP_ASSERTMESSAGE("Invalid get session handle parameters.");
                va = VA_STATUS_ERROR_INVALID_PARAMETER;
                MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, va);
                break;
            }

            memset(exec_buff->output.data, 0, exec_buff->output.data_size);
            *reinterpret_cast<unsigned long *>(exec_buff->output.data) = (unsigned long)this;

            break;
        }

        case VA_TEE_EXEC_GPU_FUNCID_IS_SESSION_ALIVE: {
            BOOL bIsSessionAlive = FALSE;

            DDI_CP_VERBOSEMESSAGE("'Is Session Alive' proprietary function requested.");

            // Validate the input struct.
            if (exec_buff->output.data == nullptr ||
                exec_buff->output.data_size != sizeof(uint8_t))
            {
                DDI_CP_ASSERTMESSAGE("Invalid Is Session Alive parameters.");
                va = VA_STATUS_ERROR_INVALID_PARAMETER;
                MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, va);
                break;
            }

            // set output to 0
            memset(exec_buff->output.data, 0, exec_buff->output.data_size);

            hr = IsSessionInitialized(bIsSessionAlive);
            if (FAILED(hr))
            {
                DDI_CP_ASSERTMESSAGE("IsSessionInitialized failed. Error = 0x%x.", hr);
                va = VA_STATUS_ERROR_OPERATION_FAILED;
                MT_ERR2(MT_CP_SESSION_INIT, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }

            *reinterpret_cast<uint8_t *>(exec_buff->output.data) = (uint8_t)bIsSessionAlive;

            break;
        }

        default:
            DDI_CP_ASSERTMESSAGE("Invalid function requested: 0x08%x.", exec_buff->function_id);
            va = VA_STATUS_ERROR_INVALID_PARAMETER;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CP_FUNC_ID, exec_buff->function_id, MT_ERROR_CODE, va);
            break;
        }

    } while (false);

    if (va != VA_STATUS_SUCCESS)
    {
        DDI_CP_ASSERTMESSAGE("Failed to execute the function 0x%x. Error code = 0x%x", exec_buff->function_id, va);
        MT_ERR2(MT_CP_FUNC_FAIL, MT_CP_FUNC_ID, exec_buff->function_id, MT_ERROR_CODE, va);
    }

    return va;
}

VAStatus CPavpCryptoSessionVANext::ProcessingTeeFunction(VAProtectedSessionExecuteBuffer *exec_buff)
{
    DDI_CP_FUNC_ENTER;

    VAStatus va = VA_STATUS_SUCCESS;
    HRESULT  hr = S_OK;

    DDI_CP_CHK_NULL(exec_buff, "exec_buff is nullptr", VA_STATUS_ERROR_INVALID_PARAMETER);

    do
    {
        switch (exec_buff->function_id)
        {
        case VA_TEE_EXECUTE_FUNCTION_ID_PASS_THROUGH: {
            DDI_CP_VERBOSEMESSAGE("'ME Pass Through' propietary function requested.");

            uint8_t vTag = 0;
#if VA_CHECK_VERSION(1, 19, 0)
            vTag = exec_buff->vtag;
#endif
            // Simply pass the received parameters to ME.
            hr = AccessME(
                static_cast<PBYTE>(exec_buff->input.data),
                exec_buff->input.data_size,
                static_cast<PBYTE>(exec_buff->output.data),
                exec_buff->output.data_size,
                false, //not a widi usecase
                vTag);

            if (FAILED(hr))
            {
                DDI_CP_ASSERTMESSAGE("Failed to execute the function 0x%x. Error code = 0x%x", exec_buff->function_id, hr);
                va = VA_STATUS_ERROR_OPERATION_FAILED;
                MT_ERR2(MT_CP_FUNC_FAIL, MT_CP_FUNC_ID, exec_buff->function_id, MT_ERROR_CODE, hr);
            }

            break;
        }

        default:
            DDI_CP_ASSERTMESSAGE("Invalid function requested: 0x08%x.", exec_buff->function_id);
            va = VA_STATUS_ERROR_INVALID_PARAMETER;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CP_FUNC_ID, exec_buff->function_id, MT_ERROR_CODE, va);
            break;
        }

    } while (false);

    if (va != VA_STATUS_SUCCESS)
    {
        DDI_CP_ASSERTMESSAGE("Failed to execute the function 0x%x. Error code = 0x%x", exec_buff->function_id, va);
        MT_ERR2(MT_CP_FUNC_FAIL, MT_CP_FUNC_ID, exec_buff->function_id, MT_ERROR_CODE, va);
    }

    return va;
}

VAStatus CPavpCryptoSessionVANext::HwSessionUpdate(VAProtectedSessionExecuteBuffer *exec_buff)
{
    DDI_CP_FUNC_ENTER;

    VAStatus                  va                    = VA_STATUS_SUCCESS;
    HRESULT                   hr                    = E_FAIL;
    OEMCrypto_PolicyBlobInfo *oemcrypto_policy_info = nullptr;
    PAVP_ExtendedSessionId   *extended_session_id   = nullptr;
    intel_oem_policy_t       *oem_policy            = nullptr;
    BOOL                      bIsSessionInitialized = FALSE;

    do
    {
        DDI_CP_CHK_NULL(exec_buff, "Null exec_buff", VA_STATUS_ERROR_INVALID_BUFFER);

        if (PAVP_DRM_TYPE_SECURE_COMPUTE == m_eDrmType)
        {
            DDI_CP_VERBOSEMESSAGE("HwSessionUpdate in secure_compute");
            if (exec_buff->input.data_size < sizeof(PAVPStoreKeyBlobInputBuff) ||
                exec_buff->input.data == nullptr)
            {
                DDI_CP_ASSERTMESSAGE("Invalid input parameter.");
                va = VA_STATUS_ERROR_INVALID_PARAMETER;
                MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, va);
                break;
            }

            if (exec_buff->output.data_size < sizeof(PavpInit43OutBuff) ||
                exec_buff->output.data == nullptr)
            {
                DDI_CP_ASSERTMESSAGE("Invalid output parameter.");
                va = VA_STATUS_ERROR_INVALID_PARAMETER;
                MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, va);
                break;
            }

            MOS_STATUS eStatus       = MOS_STATUS_UNKNOWN;
            uint8_t   *pbPrivateData = reinterpret_cast<uint8_t *>(exec_buff->output.data);

            memset(pbPrivateData, 0, sizeof(PavpInit43OutBuff));

            hr = StoreKeyBlob(exec_buff->input.data);
            if (FAILED(hr))
            {
                DDI_CP_ASSERTMESSAGE("StoreKeyBlob failed. Error = 0x%x.", hr);
                va = VA_STATUS_ERROR_OPERATION_FAILED;
                MT_ERR2(MT_CP_FUNC_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }

            hr = InitHwTranscodeSession();
            if (FAILED(hr))
            {
                DDI_CP_ASSERTMESSAGE("Call to InitHwTranscodeSession() Failed. Error code = 0x%x", hr);
                va = VA_STATUS_ERROR_OPERATION_FAILED;
                MT_ERR2(MT_CP_SESSION_INIT, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }
            m_bHwSessionIsAlive = true;

            eStatus = MOS_SecureMemcpy(pbPrivateData,
                sizeof(PavpInit43OutBuff),
                &m_pRootTrustHal->m_ResponseForINIT43,
                sizeof(PavpInit43OutBuff));
            if (MOS_STATUS_SUCCESS != eStatus)
            {
                DDI_CP_ASSERTMESSAGE("MOS_SecureMemcpy failed with error = %d.", eStatus);
                va = VA_STATUS_ERROR_OPERATION_FAILED;
                break;
            }
        }
        else
        {
            if (exec_buff->input.data_size < sizeof(OEMCrypto_PolicyBlobInfo) ||
                exec_buff->input.data == nullptr)
            {
                DDI_CP_ASSERTMESSAGE("Invalid parameter.");
                va = VA_STATUS_ERROR_INVALID_PARAMETER;
                MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, va);
                break;
            }

            oemcrypto_policy_info = reinterpret_cast<OEMCrypto_PolicyBlobInfo *>(exec_buff->input.data);
            oem_policy            = reinterpret_cast<intel_oem_policy_t *>(oemcrypto_policy_info->policy_blob_info);

            // if output data is not nullptr, need to check the max_data_size if valid or not
            if (exec_buff->output.data != nullptr &&
                exec_buff->output.max_data_size < sizeof(PAVP_ExtendedSessionId))
            {
                DDI_CP_ASSERTMESSAGE("Invalid parameter. max_data_size is less than %ld",
                    sizeof(PAVP_ExtendedSessionId));
                va = VA_STATUS_ERROR_INVALID_PARAMETER;
                MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, va);
                break;
            }
            extended_session_id = reinterpret_cast<PAVP_ExtendedSessionId *>(exec_buff->output.data);

            // Init hardware session if IsSessionInitialized() returns failure or outputs false, then return the current AppID as is.
            hr = IsSessionInitialized(bIsSessionInitialized);
            if (FAILED(hr))
            {
                DDI_CP_ASSERTMESSAGE("IsSessionInitialized failed. Error = 0x%x.", hr);
                va = VA_STATUS_ERROR_OPERATION_FAILED;
                MT_ERR2(MT_CP_SESSION_INIT, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }

            if (FALSE == bIsSessionInitialized)
            {
                DDI_CP_VERBOSEMESSAGE("oem_policy: drm_type=%d version=%d pavp_type=%d pavp_mode=%d content_type=%d",
                    oem_policy->drm_type,
                    oem_policy->version,
                    oem_policy->pavp_type,
                    oem_policy->pavp_mode,
                    oem_policy->content_type);

                if (oem_policy->drm_type == PAVP_DRM_TYPE_WIDEVINE)
                {
                    if (oem_policy->content_type == PAVP_SESSION_CONTENT_TYPE_VIDEO &&
                        oem_policy->pavp_type == PAVP_SESSION_TYPE_DECODE)
                    {
                        if (oem_policy->pavp_mode != PAVP_SESSION_MODE_HEAVY)
                        {
                            DDI_CP_ASSERTMESSAGE("pavp_mode set to %d", oem_policy->pavp_mode);
                            MT_LOG1(MT_CP_SESSION_MODE_SET, MT_NORMAL, MT_CP_SESSION_MODE, oem_policy->pavp_mode);
                        }

                        SetSessionType(PAVP_SESSION_TYPE_DECODE);
                        SetSessionMode(static_cast<PAVP_SESSION_MODE>(oem_policy->pavp_mode));
                    }
                    else if (oem_policy->content_type == PAVP_SESSION_CONTENT_TYPE_AUDIO &&
                             oem_policy->pavp_type == PAVP_SESSION_TYPE_DECODE &&
                             oem_policy->pavp_mode == PAVP_SESSION_MODE_LITE &&
                             oem_policy->version >= PAVP_OEM_POLICY_BLOB_VERSION_V1)
                    {
                        SetSessionType(PAVP_SESSION_TYPE_DECODE);
                        SetSessionMode(PAVP_SESSION_MODE_LITE);
                    }
                    else
                    {
                        DDI_CP_ASSERTMESSAGE("Unsupported content_type=%u, pavp_type=%u",
                            oem_policy->content_type,
                            oem_policy->pavp_type);
                        va = VA_STATUS_ERROR_INVALID_PARAMETER;
                        MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, va);
                        break;
                    }
                }
                else
                {
                    DDI_CP_ASSERTMESSAGE("Unknown DRM type: %u.", oem_policy->drm_type);
                    va = VA_STATUS_ERROR_INVALID_PARAMETER;
                    MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, va);
                    break;
                }

                hr = InitHwSession();
                if (FAILED(hr))
                {
                    DDI_CP_ASSERTMESSAGE("Call to InitHwSession() Failed. Error code = 0x%x", hr);
                    va = VA_STATUS_ERROR_OPERATION_FAILED;
                    MT_ERR2(MT_CP_SESSION_INIT, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                    break;
                }
            }

            if (extended_session_id)
            {
                extended_session_id->session_id   = GetSessionId();  // returns PAVP_INVALID_SESSION_ID if no session was created
                extended_session_id->drm_type     = oem_policy->drm_type;
                extended_session_id->content_type = oem_policy->content_type;
                extended_session_id->reserved     = 0;
                exec_buff->output.data_size       = sizeof(*extended_session_id);
            }
        }

    } while (false);

    return va;
}

#endif  //#ifdef VA_DRIVER_VTABLE_PROT_VERSION
