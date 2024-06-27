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
//! \file     ddi_cp_next.cpp
//! \brief    DDI interface for content protection parameters and operations.
//! \details  Other components will call this interface for content protection operations directly
//!

#include "ddi_cp_next.h"
#include "codechal_setting.h"
#include "ddi_decode_base_specific.h"
#include "media_libva_util_next.h"
#include "ddi_libva_encoder_specific.h"
#include "mhw_cp.h"
#include "mhw_cp_hwcmd_common.h"
#include "cp_pavp_cryptosession_va_next.h"
#include "va_protected_content_private.h"
#include "ddi_libva_decoder_specific.h"

#define DDI_ENCODE_AES_COUNTER_INCREASE_STEP 0xc000;

static const bool registeredDdiCpNextImp = DdiCpFactory::Register<DdiCpNext>(DdiCpCreateType::CreateDdiCpImp);

DdiCpInterfaceNext *CreateDdiCpNext(MOS_CONTEXT* mosCtx)
{
    DDI_CP_FUNC_ENTER;
    
    DDI_CP_CHK_NULL(mosCtx, "mosCtx is nullptr", nullptr);
    DdiCpInterfaceNext *ddiCp = nullptr;
    ddiCp = DdiCpFactory::Create(DdiCpCreateType::CreateDdiCpImp, *mosCtx);
    if(ddiCp == nullptr)
    {
        DDI_CP_ASSERTMESSAGE("Create Cp instance failed.");
    }
    
    return ddiCp;
}

DdiCpNext::DdiCpNext(MOS_CONTEXT& mosCtx):DdiCpInterfaceNext(mosCtx)
{
    DDI_CP_FUNC_ENTER;

    isHDCP2Enabled = false;
    isPAVPEnabled  = false;
    m_ddiCpContext.value  = 0;
    m_isCencProcessing = false;
    m_isSecureDecoderEnabled = false;
    m_attachedCpSession = nullptr;
    m_numSegmentsInBuffer = 0;
    ecbInputEnabled = false;
    rIV = 0;
    cpInstanceCount = 0;
    streamCtr = 0;
    
    m_encryptionParams[DECODE_ENCRYPTION_PARAMS_INDEX].PavpEncryptionType  = CP_SECURITY_NONE;
    m_encryptionParams[DECODE_ENCRYPTION_PARAMS_INDEX].HostEncryptMode     = CP_TYPE_NONE;
    m_encryptionParams[CENC_ENCRYPTION_PARAMS_INDEX].PavpEncryptionType  = CP_SECURITY_NONE;
    m_encryptionParams[CENC_ENCRYPTION_PARAMS_INDEX].HostEncryptMode     = CP_TYPE_NONE;

    MOS_ZeroMemory(&m_cencParams, sizeof(CODECHAL_CENC_PARAMS));
}

DdiCpNext::~DdiCpNext()
{
    DDI_CP_FUNC_ENTER;

    // Destroy cenc Decode
    if ( nullptr != m_cencDecoder)
    {
        MOS_Delete(m_cencDecoder);
        m_cencDecoder = nullptr;
    }

    if(m_cencParams.pSegmentInfo)
    {
        MOS_FreeMemory(m_cencParams.pSegmentInfo);
        m_cencParams.pSegmentInfo = nullptr;
    }

    if(!m_subSampleMappingBlocks.empty())
    {
        m_subSampleMappingBlocks.clear();
    }
}

//----decoder related functions------------------------------------------
void DdiCpNext::SetCpParams(uint32_t encryptionType, CodechalSetting *setting)
{
    DDI_CP_FUNC_ENTER;

    switch (encryptionType)
    {
    case VA_ENCRYPTION_TYPE_FULLSAMPLE_CTR:
    case VA_ENCRYPTION_TYPE_FULLSAMPLE_CBC:
    case VA_ENCRYPTION_TYPE_SUBSAMPLE_CTR:
    case VA_ENCRYPTION_TYPE_SUBSAMPLE_CBC:
        m_isSecureDecoderEnabled = true;
        break;
    default:
        break;
    }

    if (setting)
    {
        // In secure AVC fullsample, we will set shortFormatInUse to true in DdiCpNext::SetDecodeParams
        // But when SetCpParams is getting called in decode context creation, Chrome browser does not
        // the the stream is fullsample or subsample, therefore we set shortFormatInUse to true to
        // let m_standardDecodeSizeNeeded being the larger one. m_standardDecodeSizeNeeded is
        // calculated in CodechalDecodeAvc::AllocateStandard()
        if (m_isSecureDecoderEnabled && setting->standard == CODECHAL_AVC)
        {
            setting->shortFormatInUse = false;
        }

        setting->cpParams = &m_encryptionParams[DECODE_ENCRYPTION_PARAMS_INDEX];
    }
}

void DdiCpNext::SetCryptoKeyExParams(uint32_t encryptionType, uint32_t *EncryptedDecryptKey)
{
    DDI_CP_FUNC_ENTER;

    if (EncryptedDecryptKey)
    {
        uint32_t idx = 0;
        //Get encryption type from DDI parameter and set encryption parameter which was registed to MHW
        if (encryptionType == VA_ENCRYPTION_TYPE_FULLSAMPLE_CBC ||
            encryptionType == VA_ENCRYPTION_TYPE_CENC_CTR_LENGTH ||
            encryptionType == VA_ENCRYPTION_TYPE_FULLSAMPLE_CTR)
        {
            idx = CENC_ENCRYPTION_PARAMS_INDEX;
        }
        else if (encryptionType == VA_ENCRYPTION_TYPE_SUBSAMPLE_CTR ||
                 encryptionType == VA_ENCRYPTION_TYPE_SUBSAMPLE_CBC)
        {
            idx = DECODE_ENCRYPTION_PARAMS_INDEX;
            m_encryptionParams[idx].bInsertKeyExinVcsProlog = true;
        }
        else
        {
            return;
        }

        switch (encryptionType)
        {
        case VA_ENCRYPTION_TYPE_FULLSAMPLE_CBC:
        case VA_ENCRYPTION_TYPE_SUBSAMPLE_CBC:
            m_encryptionParams[idx].HostEncryptMode = CENC_TYPE_CBC;
            m_encryptionParams[idx].PavpEncryptionType = CP_SECURITY_AES128_CBC;
            break;
        case VA_ENCRYPTION_TYPE_CENC_CTR_LENGTH:
            m_encryptionParams[idx].HostEncryptMode = CENC_TYPE_CTR_LENGTH;
            m_encryptionParams[idx].PavpEncryptionType = CP_SECURITY_AES128_CTR;
            break;
        case VA_ENCRYPTION_TYPE_FULLSAMPLE_CTR:
        case VA_ENCRYPTION_TYPE_SUBSAMPLE_CTR:
            m_encryptionParams[idx].HostEncryptMode = CENC_TYPE_CTR;
            m_encryptionParams[idx].PavpEncryptionType = CP_SECURITY_AES128_CTR;
            break;
        default:
            break;
        }

        m_encryptionParams[idx].dwIVSize = MHW_IV_SIZE_8; //8 bytes IV and 8 Bytes Counter
        m_encryptionParams[idx].CryptoKeyExParams.KeyExchangeKeyUse = CRYPTO_DECRYPT_AND_USE_NEW_KEY;
        m_encryptionParams[idx].CryptoKeyExParams.KeyExchangeKeySelect |= MHW_CRYPTO_DECRYPTION_KEY;

        auto *pPavpDevice = static_cast<CPavpDevice *>(m_attachedCpSession);

        if(pPavpDevice->IsGSK())
        {
            if (pPavpDevice->SetCryptoKeyExParams(&m_encryptionParams[idx]) != S_OK)
            {
                DDI_CP_ASSERTMESSAGE("SetCryptoKeyExParams failed");
            }
        }
        else
        {
            MOS_SecureMemcpy(m_encryptionParams[idx].CryptoKeyExParams.EncryptedDecryptionKey,
                             CODEC_DWORDS_PER_AES_BLOCK * sizeof(uint32_t),
                             EncryptedDecryptKey,
                             CODEC_DWORDS_PER_AES_BLOCK * sizeof(uint32_t));
        }

        if(m_cencDecoder)
        {
            //Programming note for ICL+: All VCS commands must include an inline key exchange command at the beginning if Kb-based key refresh is in use
            m_encryptionParams[idx].bInsertKeyExinVcsProlog = true;
        }
    }
}

VAStatus DdiCpNext::ParseEncryptionParamsforDecrypt(void *params)
{
    DDI_CP_FUNC_ENTER;

    auto pDecryptInputParams = (VAEncryptionParameters *)params;

    DDI_CP_CHK_NULL(pDecryptInputParams,"NULL pDecryptInputParams in ParseEncryptionParamsforDecrypt!", VA_STATUS_ERROR_INVALID_CONTEXT);

    m_cencParams.ui16StatusReportFeedbackNumber = pDecryptInputParams->status_report_index;
    m_cencParams.dwNumSegments = pDecryptInputParams->num_segments;

    DDI_CP_NORMALMESSAGE("num_segments=%d", m_cencParams.dwNumSegments);

    if (m_cencParams.dwNumSegments > 0)
    {
        DDI_CP_CHK_NULL(pDecryptInputParams->segment_info,"NULL pDecryptInputParams->segment_info!", VA_STATUS_ERROR_INVALID_CONTEXT);

        if(nullptr == m_cencParams.pSegmentInfo) //allocate new buffer
        {
            m_cencParams.pSegmentInfo = (PMHW_SEGMENT_INFO)MOS_AllocAndZeroMemory(m_cencParams.dwNumSegments * sizeof(MHW_SEGMENT_INFO));
            DDI_CP_CHK_NULL(m_cencParams.pSegmentInfo, "Allocate CENC segment buffer Failed!", VA_STATUS_ERROR_INVALID_CONTEXT);
            m_numSegmentsInBuffer = m_cencParams.dwNumSegments;
        }
        else if(m_cencParams.dwNumSegments > m_numSegmentsInBuffer) //resize buffer for more segments
        {
            m_cencParams.pSegmentInfo = (PMHW_SEGMENT_INFO)MOS_ReallocMemory(m_cencParams.pSegmentInfo, m_cencParams.dwNumSegments * sizeof(MHW_SEGMENT_INFO));
            DDI_CP_CHK_NULL(m_cencParams.pSegmentInfo, "Reallocate CENC segment buffer Failed!", VA_STATUS_ERROR_INVALID_CONTEXT);
            m_numSegmentsInBuffer = m_cencParams.dwNumSegments;
        }

        for (uint32_t i = 0; i < pDecryptInputParams->num_segments; ++i)
        {
            MHW_SEGMENT_INFO&        CencSegmentInfo       = ((PMHW_SEGMENT_INFO)(m_cencParams.pSegmentInfo))[i];
            VAEncryptionSegmentInfo& EncryptionSegmentInfo = pDecryptInputParams->segment_info[i];

            CencSegmentInfo.dwSegmentStartOffset         = EncryptionSegmentInfo.segment_start_offset;
            CencSegmentInfo.dwSegmentLength              = EncryptionSegmentInfo.segment_length;
            CencSegmentInfo.dwPartialAesBlockSizeInBytes = EncryptionSegmentInfo.partial_aes_block_size;
            CencSegmentInfo.dwInitByteLength             = EncryptionSegmentInfo.init_byte_length;

            DDI_CP_NORMALMESSAGE("  init=%d, off=%d, partial=%d, length=%d",
                CencSegmentInfo.dwInitByteLength,
                CencSegmentInfo.dwSegmentStartOffset,
                CencSegmentInfo.dwPartialAesBlockSizeInBytes,
                CencSegmentInfo.dwSegmentLength);

            if (pDecryptInputParams->key_blob_size == 0)
            {
                pDecryptInputParams->key_blob_size = 16;
            }
            auto eStatus = MOS_SecureMemcpy(CencSegmentInfo.dwAesCbcIvOrCtr, sizeof(CencSegmentInfo.dwAesCbcIvOrCtr), EncryptionSegmentInfo.aes_cbc_iv_or_ctr, pDecryptInputParams->key_blob_size);
            DDI_CHK_CONDITION((eStatus != MOS_STATUS_SUCCESS), "DDI:Failed to copy AesCbcIvorCtr in Segment info!", VA_STATUS_ERROR_OPERATION_FAILED);

            if (pDecryptInputParams->encryption_type == VA_ENCRYPTION_TYPE_FULLSAMPLE_CTR)
            {
                CPavpDevice::CounterSwapEndian(CencSegmentInfo.dwAesCbcIvOrCtr);
            }
        }

        SetCryptoKeyExParams(pDecryptInputParams->encryption_type, (uint32_t *) pDecryptInputParams->wrapped_decrypt_blob);
    }
    m_cencParams.ui8SizeOfLength = pDecryptInputParams->size_of_length;

    m_isCencProcessing = true;

    return VA_STATUS_SUCCESS;
}

VAStatus DdiCpNext::ParseEncryptionParamsforDecode(void  *params)
{
    DDI_CP_FUNC_ENTER;

    VAStatus vaStatus = VA_STATUS_SUCCESS;

    DDI_CP_CHK_NULL(params, "NULL params in DdiCpNext::ParseEncryptionParamsforDecode!", VA_STATUS_ERROR_INVALID_CONTEXT);

    auto cipherData = (VAEncryptionParameters *)params;

    m_encryptionParams[DECODE_ENCRYPTION_PARAMS_INDEX].HostEncryptMode = CP_TYPE_BASIC;

    //Sub sample setting for AES CTR. And CBC subsample may be added in future.
    if (cipherData->segment_info && cipherData->num_segments > 0 && cipherData->encryption_type == VA_ENCRYPTION_TYPE_SUBSAMPLE_CTR)
    {
        m_encryptionParams[DECODE_ENCRYPTION_PARAMS_INDEX].PavpEncryptionType     = CP_SECURITY_DECE_AES128_CTR; //AES CTR DECE IECE

        if (!m_subSampleMappingBlocks.empty())
            m_subSampleMappingBlocks.clear();

        //map segment info to subsample mapping block
        MHW_SUB_SAMPLE_MAPPING_BLOCK block;
        for (int i = 0; i < cipherData->num_segments; i++)
        {
            block.ClearSize = cipherData->segment_info[i].init_byte_length;
            block.EncryptedSize = cipherData->segment_info[i].segment_length - cipherData->segment_info[i].init_byte_length;
            m_subSampleMappingBlocks.push_back(block);
        }

        //save sub sample block info to encryption parameter
        if (!m_subSampleMappingBlocks.empty())
        {
            m_encryptionParams[DECODE_ENCRYPTION_PARAMS_INDEX].pSubSampleMappingBlock = m_subSampleMappingBlocks.data();
            m_encryptionParams[DECODE_ENCRYPTION_PARAMS_INDEX].dwSubSampleCount = m_subSampleMappingBlocks.size();
        }

        //copy first segment IV to encryption parameter
        if (cipherData->key_blob_size == 0)
        {
            cipherData->key_blob_size = 16;
        }
        auto eStatus = MOS_SecureMemcpy(m_encryptionParams[DECODE_ENCRYPTION_PARAMS_INDEX].dwPavpAesCounter,
            sizeof(m_encryptionParams[DECODE_ENCRYPTION_PARAMS_INDEX].dwPavpAesCounter),
            cipherData->segment_info->aes_cbc_iv_or_ctr,
            cipherData->key_blob_size);
        DDI_CHK_CONDITION((eStatus != MOS_STATUS_SUCCESS), "DDI:Failed to copy AesCbcIvorCtr in Segment info!", VA_STATUS_ERROR_OPERATION_FAILED);

        // cipherData->encryption_type == VA_ENCRYPTION_TYPE_SUBSAMPLE_CTR is true
        CPavpDevice::CounterSwapEndian(m_encryptionParams[DECODE_ENCRYPTION_PARAMS_INDEX].dwPavpAesCounter);
    }

    m_encryptionParams[DECODE_ENCRYPTION_PARAMS_INDEX].bInsertKeyExinVcsProlog = true;
    m_encryptionParams[DECODE_ENCRYPTION_PARAMS_INDEX].dwIVSize = MHW_IV_SIZE_8; //8 bytes IV and 8 Bytes Counter
    m_encryptionParams[DECODE_ENCRYPTION_PARAMS_INDEX].CryptoKeyExParams.KeyExchangeKeyUse = CRYPTO_DECRYPT_AND_USE_NEW_KEY;
    m_encryptionParams[DECODE_ENCRYPTION_PARAMS_INDEX].CryptoKeyExParams.KeyExchangeKeySelect |= MHW_CRYPTO_DECRYPTION_KEY;
    MOS_SecureMemcpy(m_encryptionParams[DECODE_ENCRYPTION_PARAMS_INDEX].CryptoKeyExParams.EncryptedDecryptionKey,
        CODEC_DWORDS_PER_AES_BLOCK * sizeof(uint32_t),
        (uint32_t *)cipherData->wrapped_decrypt_blob,
        CODEC_DWORDS_PER_AES_BLOCK * sizeof(uint32_t));

    return vaStatus;
}

VAStatus DdiCpNext::ParseEncryptionParamsforHUCDecode(void  *params)
{
    DDI_CP_FUNC_ENTER;

    VAStatus vaStatus = VA_STATUS_SUCCESS;
    bool is_stripe = false;

    DDI_CP_CHK_NULL(params, "NULL params in DdiCpNext::ParseEncryptionParamsforHUCDecode!", VA_STATUS_ERROR_INVALID_CONTEXT);

    auto cipherData = (VAEncryptionParameters *)params;

    if (cipherData->segment_info && cipherData->num_segments > 0)
    {
        if(!m_subSampleMappingBlocks.empty())
            m_subSampleMappingBlocks.clear();

        DDI_CP_NORMALMESSAGE("num_segments=%d, stripe_enc=%d, stripe_clr=%d",
            cipherData->num_segments,
            cipherData->blocks_stripe_encrypted,
            cipherData->blocks_stripe_clear);

        // strip enabled when blocks_stripe_encrypted is non-zero
        if (cipherData->blocks_stripe_encrypted > 0)
        {
            m_encryptionParams[DECODE_ENCRYPTION_PARAMS_INDEX].dwBlocksStripeEncCount = cipherData->blocks_stripe_encrypted;
            m_encryptionParams[DECODE_ENCRYPTION_PARAMS_INDEX].dwBlocksStripeClearCount = cipherData->blocks_stripe_clear;
            is_stripe = true;
        }

        //map segment info to subsample mapping block
        MHW_SUB_SAMPLE_MAPPING_BLOCK block;
        for (int i = 0; i < cipherData->num_segments; i++)
        {
            block.ClearSize = cipherData->segment_info[i].init_byte_length;
            block.EncryptedSize = cipherData->segment_info[i].segment_length - cipherData->segment_info[i].init_byte_length;
            if ((cipherData->encryption_type == VA_ENCRYPTION_TYPE_SUBSAMPLE_CBC) && (is_stripe == false))
            {
                // If there is a partial block at the end in non-striping CBC, we skip the partial block
                block.EncryptedSize = MOS_ALIGN_FLOOR(block.EncryptedSize, 16);
            }
            m_subSampleMappingBlocks.push_back(block);

            DDI_CP_NORMALMESSAGE("  init=%d, off=%d, partial=%d, length=%d, clr_s=%d, enc_s=%d",
                cipherData->segment_info[i].init_byte_length,
                cipherData->segment_info[i].segment_start_offset,
                cipherData->segment_info[i].partial_aes_block_size,
                cipherData->segment_info[i].segment_length,
                block.ClearSize,
                block.EncryptedSize);
        }

        //save sub sample block info to encryption parameter
        if (!m_subSampleMappingBlocks.empty())
        {
            m_encryptionParams[DECODE_ENCRYPTION_PARAMS_INDEX].pSubSampleMappingBlock = m_subSampleMappingBlocks.data();
            m_encryptionParams[DECODE_ENCRYPTION_PARAMS_INDEX].dwSubSampleCount = m_subSampleMappingBlocks.size();
        }

        //copy first segment IV to encryption parameter
        if (cipherData->key_blob_size == 0)
        {
            cipherData->key_blob_size = 16;
        }
        auto eStatus = MOS_SecureMemcpy(m_encryptionParams[DECODE_ENCRYPTION_PARAMS_INDEX].dwPavpAesCounter,
            sizeof(m_encryptionParams[DECODE_ENCRYPTION_PARAMS_INDEX].dwPavpAesCounter),
            cipherData->segment_info->aes_cbc_iv_or_ctr,
            cipherData->key_blob_size);
        DDI_CHK_CONDITION((eStatus != MOS_STATUS_SUCCESS), "DDI:Failed to copy AesCbcIvorCtr in Segment info!", VA_STATUS_ERROR_OPERATION_FAILED);

        if (cipherData->encryption_type == VA_ENCRYPTION_TYPE_SUBSAMPLE_CTR)
        {
            CPavpDevice::CounterSwapEndian(m_encryptionParams[DECODE_ENCRYPTION_PARAMS_INDEX].dwPavpAesCounter);
        }
    }

    SetCryptoKeyExParams(cipherData->encryption_type, (uint32_t *)cipherData->wrapped_decrypt_blob);
    return vaStatus;
}

VAStatus DdiCpNext::ParseEncryptionParamsforEncode(void  *params)
{
    DDI_CP_FUNC_ENTER;

    VAStatus vaStatus = VA_STATUS_SUCCESS;

    DDI_CP_CHK_NULL(params, "NULL params in DdiCpNext::ParseEncryptionParamsforEncode!", VA_STATUS_ERROR_INVALID_CONTEXT);

    auto cipherData = (VAEncryptionParameters *)params;

    m_encryptionParams[ENCODE_ENCRYPTION_PARAMS_INDEX].HostEncryptMode = CP_TYPE_BASIC;
    m_encryptionParams[ENCODE_ENCRYPTION_PARAMS_INDEX].PavpEncryptionType = CP_SECURITY_BASIC;

    m_encryptionParams[ENCODE_ENCRYPTION_PARAMS_INDEX].bInsertKeyExinVcsProlog = true;
    m_encryptionParams[ENCODE_ENCRYPTION_PARAMS_INDEX].dwIVSize = MHW_IV_SIZE_8;
    m_encryptionParams[ENCODE_ENCRYPTION_PARAMS_INDEX].CryptoKeyExParams.KeyExchangeKeyUse = CRYPTO_DECRYPT_AND_USE_NEW_KEY;
    m_encryptionParams[ENCODE_ENCRYPTION_PARAMS_INDEX].CryptoKeyExParams.KeyExchangeKeySelect |= MHW_CRYPTO_ENCRYPTION_KEY;

    auto *pPavpDevice = static_cast<CPavpDevice *>(m_attachedCpSession);

    if(pPavpDevice->IsGSK())
    {
        if (pPavpDevice->SetCryptoKeyExParams(&m_encryptionParams[ENCODE_ENCRYPTION_PARAMS_INDEX]) != S_OK)
        {
            DDI_CP_ASSERTMESSAGE("ParseEncryptionParamsforEncode failed");
        }
    }
    else
    {
        MOS_SecureMemcpy(m_encryptionParams[ENCODE_ENCRYPTION_PARAMS_INDEX].CryptoKeyExParams.EncryptedEncryptionKey,
                         CODEC_DWORDS_PER_AES_BLOCK * sizeof(uint32_t),
                         (uint32_t *)cipherData->wrapped_encrypt_blob,
                         CODEC_DWORDS_PER_AES_BLOCK * sizeof(uint32_t));
    }

    return vaStatus;
}

VAStatus DdiCpNext::EndPictureCenc (
    VADriverContextP vaDrvCtx,
    VAContextID      contextId)
{
    DDI_CP_FUNC_ENTER;

    VAStatus                          va;

    DDI_CP_CHK_NULL(vaDrvCtx,"NULL vaDrvCtx in DdiCpNext::EndPictureCenc!", VA_STATUS_ERROR_INVALID_CONTEXT);

    // assume the VAContextID is decoder ID
    uint32_t                         ctxType;
    // assume the VAContextID is decoder ID
    auto decCtx     = (decode::PDDI_DECODE_CONTEXT)MediaLibvaCommonNext::GetContextFromContextID(vaDrvCtx, contextId, &ctxType);
    DDI_CP_CHK_NULL(decCtx,"Null decCtx in DdiCpNext::EndPictureCenc",VA_STATUS_ERROR_INVALID_CONTEXT);

    m_cencParams.presDataBuffer = &decCtx->BufMgr.resBitstreamBuffer;
    auto status = m_cencDecoder->Execute(&m_cencParams);

    if (status == MOS_STATUS_SUCCESS)
    {
        va = VA_STATUS_SUCCESS;
    }
    else if (status == MOS_STATUS_CLIENT_AR_NO_SPACE)
    {
        va = VA_STATUS_ERROR_SURFACE_BUSY;
    }
    else
    {
        va = VA_STATUS_ERROR_OPERATION_FAILED;
    }

    if (va == VA_STATUS_SUCCESS)
    {
        // query cenc status buf, the buf is set in vaBeginPicture
        if (decCtx->RTtbl.pCurrentRT && decCtx->RTtbl.pCurrentRT->pData)
        {
            auto cencStatus = (VACencStatusBuf *) decCtx->RTtbl.pCurrentRT->pData;
            auto decstatus = m_cencDecoder->GetStatusReport(cencStatus, 1);

            if (decstatus != MOS_STATUS_SUCCESS)
            {
                va = VA_STATUS_ERROR_DECODING_ERROR;
                DDI_CP_ASSERTMESSAGE("cencDecoder GetStatusReport error %d", va);
                MT_ERR2(MT_ERR_MOS_STATUS_CHECK, MT_MOS_STATUS, decstatus, MT_ERROR_CODE, va);
            }

            if (cencStatus->status == VA_ENCRYPTION_STATUS_SUCCESSFUL)
            {
                // Do nothing
            }
            else if (cencStatus->status == VA_ENCRYPTION_STATUS_INCOMPLETE)
            {
                // Report success to App with error code, skip the hang frame
                DDI_CP_ASSERTMESSAGE("cencDecoder GetStatusReport incomplete");
                MT_ERR2(MT_CP_CENC_STATUS_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, cencStatus->status);
            }
            else if (cencStatus->status == VA_ENCRYPTION_STATUS_BUFFER_FULL)
            {
                // Return success to App for not breaking execution but enlarge the buffer and try again
                DDI_CP_ASSERTMESSAGE("cencDecoder GetStatusReport buffer full");
                MT_ERR2(MT_CP_CENC_STATUS_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, cencStatus->status);
            }
            else if (cencStatus->status == VA_ENCRYPTION_STATUS_ERROR)
            {
                va = VA_STATUS_ERROR_DECODING_ERROR;
                DDI_CP_ASSERTMESSAGE("cencDecoder decrypt status error %d", va);
                MT_ERR2(MT_CP_CENC_STATUS_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, va);
            }
        }
    }

    UMD_ATRACE_END;
    return va;
}

VAStatus DdiCpNext::RenderCencPicture (
    VADriverContextP      vaDrvctx,
    VAContextID           contextId,
    DDI_MEDIA_BUFFER      *buf,
    void                  *data)
{
    DDI_CP_FUNC_ENTER;

    int32_t                       index;
    VAStatus                      status = VA_STATUS_SUCCESS;
    decode::PDDI_DECODE_CONTEXT   decCtx = nullptr;
    encode::PDDI_ENCODE_CONTEXT   encCtx = nullptr;

    DDI_CP_CHK_NULL(vaDrvctx,"nullptr vaDrvctx in DdiCpNext::RenderDecryptPicture!", VA_STATUS_ERROR_INVALID_CONTEXT);

    uint32_t                         ctxType;
    auto ctx     = MediaLibvaCommonNext::GetContextFromContextID(vaDrvctx, contextId, &ctxType);

    DDI_CP_CHK_NULL(ctx,   "nullptr ctx",   VA_STATUS_ERROR_INVALID_CONTEXT);
    DDI_CP_CHK_NULL(buf,   "nullptr buf",   VA_STATUS_ERROR_INVALID_PARAMETER);

    switch(ctxType)
    {
    case DDI_MEDIA_CONTEXT_TYPE_DECODER:
        decCtx = reinterpret_cast<decode::PDDI_DECODE_CONTEXT>(ctx);
        break;
    case DDI_MEDIA_CONTEXT_TYPE_ENCODER:
        encCtx = reinterpret_cast<encode::PDDI_ENCODE_CONTEXT>(ctx);
        break;
    default:
        return VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE;
    }

    switch ((int32_t)buf->uiType)
    {
    case VAProtectedSliceDataBufferType:
    {
        if(decCtx)
        {
            index = decCtx->m_ddiDecodeNext->GetBitstreamBufIndexFromBuffer(&decCtx->BufMgr, buf);
            if(index == DDI_CODEC_INVALID_BUFFER_INDEX)
            {
                return VA_STATUS_ERROR_INVALID_BUFFER;
                break;
            }

            DdiMedia_MediaBufferToMosResource(decCtx->BufMgr.pBitStreamBuffObject[index], &decCtx->BufMgr.resBitstreamBuffer);
            m_cencParams.dwDataBufferSize = buf->iSize;
        }
        else
        {
            status = VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE;
        }
        break;
    }
    case VACencStatusParameterBufferType:
    {
        if(decCtx)
        {
            decCtx->DecodeParams.m_cencDecodeStatusReportNum = ((VACencStatusParameters *)data)->status_report_index_feedback;
            decCtx->DecodeParams.m_numSlices                 = 1;
        }
        else
        {
            status = VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE;
        }
        break;
    }
    case VAEncryptionParameterBufferType:
    {
        auto cipherData = (VAEncryptionParameters *)data;
        if (VA_ENCRYPTION_TYPE_SUBSAMPLE_CTR == cipherData->encryption_type ||
            VA_ENCRYPTION_TYPE_SUBSAMPLE_CBC == cipherData->encryption_type)
        {
            if(DDI_MEDIA_CONTEXT_TYPE_DECODER == ctxType)
            {
                if(m_isSecureDecoderEnabled)
                {
                    //secure decode with HUC streamout/S2L
                    status = ParseEncryptionParamsforHUCDecode(data);
                }
                else
                {
                    //secure decode w/o HUC
                    status = ParseEncryptionParamsforDecode(data);
                }
            }
            else
            {
                status = ParseEncryptionParamsforEncode(data);
            }
        }
        else if (VA_ENCRYPTION_TYPE_FULLSAMPLE_CBC  == cipherData->encryption_type ||
                 VA_ENCRYPTION_TYPE_CENC_CTR_LENGTH == cipherData->encryption_type ||
                 VA_ENCRYPTION_TYPE_FULLSAMPLE_CTR  == cipherData->encryption_type ||
                 VA_ENCRYPTION_TYPE_NONE            == cipherData->encryption_type) //TODO: change to adapt lite mode or clear cenc cases
        {
            status = ParseEncryptionParamsforDecrypt(data);
        }
        else
        {
            status = VA_STATUS_ERROR_OPERATION_FAILED;
            DDI_CP_ASSERTMESSAGE("Not Supported Yet");
            MT_ERR2(MT_CP_DDI_FUNC_UNSUPPORTED, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, status);
        }
        break;
    }
    default:
        status = VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE;
    }

    return status;

}

VAStatus DdiCpNext::CreateBuffer(
    VABufferType             type,
    DDI_MEDIA_BUFFER*        buffer,
    uint32_t                 size,
    uint32_t                 num_elements)
{
    DDI_CP_FUNC_ENTER;

    DDI_CP_CHK_NULL(buffer,"Null buffer",VA_STATUS_ERROR_INVALID_CONTEXT);

#ifdef VA_DRIVER_VTABLE_PROT_VERSION
    switch ((int32_t)type)
    {
    case VAEncryptionParameterBufferType:
        buffer->pData = (uint8_t*)MOS_AllocAndZeroMemory(size * num_elements);
        buffer->format = Media_Format_CPU;
        break;
    case VACencStatusParameterBufferType:
        buffer->pData = (uint8_t*)MOS_AllocAndZeroMemory(sizeof(VACencStatusParameters));
        buffer->format = Media_Format_CPU;
        break;
    case VAProtectedSessionExecuteBufferType:
        buffer->pData = (uint8_t*)MOS_AllocAndZeroMemory(size * num_elements);
        buffer->format = Media_Format_CPU;
    break;
    default:
        return VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE;
    }
#endif //#ifdef VA_DRIVER_VTABLE_PROT_VERSION

    return VA_STATUS_SUCCESS;
}

//----encoder related functions------------------------------------------
VAStatus DdiCpNext::InitHdcp2Buffer(DDI_CODEC_COM_BUFFER_MGR* bufMgr)
{
    DDI_CP_FUNC_ENTER;

    DDI_CP_CHK_NULL(bufMgr,"Null bufMgr",VA_STATUS_ERROR_INVALID_CONTEXT);

    if (IsHdcp2Enabled() || isPAVPEnabled)
    {
        VACodedBufferSegment *pHDCPCodedBufferSegment = (VACodedBufferSegment *)MOS_AllocAndZeroMemory(sizeof(VACodedBufferSegment));
        if (pHDCPCodedBufferSegment == nullptr)
        {
            return VA_STATUS_ERROR_ALLOCATION_FAILED;
        }

        bufMgr->pHDCP2ParameterBuffer = (VAHDCP2EncryptionStatusBuffer *)MOS_AllocAndZeroMemory(sizeof(VAHDCP2EncryptionStatusBuffer));
        if (bufMgr->pHDCP2ParameterBuffer == nullptr)
        {
            MOS_FreeMemory(pHDCPCodedBufferSegment);
            return VA_STATUS_ERROR_ALLOCATION_FAILED;
        }
        // Report to app HDCP2 session active
        ((VAHDCP2EncryptionStatusBuffer *)(bufMgr->pHDCP2ParameterBuffer))->bEncrypted = true;

        pHDCPCodedBufferSegment->buf  = bufMgr->pHDCP2ParameterBuffer;
        pHDCPCodedBufferSegment->size = sizeof(VAHDCP2EncryptionStatusBuffer);
        pHDCPCodedBufferSegment->status |= HDCP2_CODED_BUF_FLAG;
        bufMgr->pCodedBufferSegment->next = pHDCPCodedBufferSegment;
    }

    return VA_STATUS_SUCCESS;
}

VAStatus DdiCpNext::StatusReportForHdcp2Buffer(
    DDI_CODEC_COM_BUFFER_MGR*       bufMgr,
    void*               encodeStatusReport)
{
    DDI_CP_FUNC_ENTER;

    DDI_CP_CHK_NULL(bufMgr,"Null bufMgr",VA_STATUS_ERROR_INVALID_CONTEXT);
    DDI_CP_CHK_NULL(encodeStatusReport,"Null info",VA_STATUS_ERROR_INVALID_CONTEXT);

    if (IsHdcp2Enabled() || isPAVPEnabled)
    {
        VACodedBufferSegment *hdCpCodedBufferSegment = nullptr;

        hdCpCodedBufferSegment = (VACodedBufferSegment *)bufMgr->pCodedBufferSegment->next;
        DDI_CP_CHK_NULL(hdCpCodedBufferSegment, "Null hdCpCodedBufferSegment", VA_STATUS_ERROR_INVALID_PARAMETER);

        // App will get correct AES counter values by the order in which they are added
        if (bufMgr->pHDCP2ParameterBuffer)
        {
            VAHDCP2EncryptionStatusBuffer *pHDCP2ParameterBuffer= (VAHDCP2EncryptionStatusBuffer*)bufMgr->pHDCP2ParameterBuffer;
            if (encodeStatusReport)
            {
                EncodeStatusReport *pencodeStatusReport = reinterpret_cast<EncodeStatusReport*>(encodeStatusReport);
                MOS_SecureMemcpy(
                    &pHDCP2ParameterBuffer->counter,
                    sizeof(pencodeStatusReport->HWCounterValue.IV),
                    &pencodeStatusReport->HWCounterValue.IV,
                    sizeof(pencodeStatusReport->HWCounterValue.IV));
                MOS_SecureMemcpy(&pHDCP2ParameterBuffer->counter[2],
                    sizeof(pencodeStatusReport->HWCounterValue.Count),
                    &pencodeStatusReport->HWCounterValue.Count,
                    sizeof(pencodeStatusReport->HWCounterValue.Count));
                DDI_CP_NORMALMESSAGE("GetFrom: AES counter = %lld", pencodeStatusReport->HWCounterValue.Count);
            }
        }

        if (IsHdcpTearDown())
        {
            hdCpCodedBufferSegment->status |= VA_CODED_BUF_STATUS_HW_TEAR_DOWN;
            DDI_CP_NORMALMESSAGE("HDCP2 tear down happen! status=0x%x ", hdCpCodedBufferSegment->status);
        }
        else  // tear down didn't happen
        {
            hdCpCodedBufferSegment->status &= (~VA_CODED_BUF_STATUS_HW_TEAR_DOWN);
        }
    }

    return VA_STATUS_SUCCESS;
}

void DdiCpNext::FreeHdcp2Buffer(DDI_CODEC_COM_BUFFER_MGR* bufMgr)
{
    DDI_CP_FUNC_ENTER;

    DDI_CP_CHK_NULL(bufMgr,"Null bufMgr", );

    if (IsHdcp2Enabled() || isPAVPEnabled)
    {
        if (bufMgr->pCodedBufferSegment->next == bufMgr->pCodedBufferSegmentForStatusReport)
        {
            MOS_FreeMemory(bufMgr->pCodedBufferSegment->next);
            bufMgr->pCodedBufferSegment->next = nullptr;
            if (bufMgr->pCodedBufferSegmentForStatusReport != nullptr)
                bufMgr->pCodedBufferSegmentForStatusReport = nullptr;
        }
        else
        {
            MOS_FreeMemory(bufMgr->pCodedBufferSegment->next);
            bufMgr->pCodedBufferSegment->next = nullptr;
        }
        MOS_FreeMemory(bufMgr->pHDCP2ParameterBuffer);
        bufMgr->pHDCP2ParameterBuffer = nullptr;
    }
}

MOS_STATUS DdiCpNext::StoreCounterToStatusReport(
    encode::PDDI_ENCODE_STATUS_REPORT_INFO info)
{
    DDI_CP_FUNC_ENTER;

    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    do
    {
        if (info == nullptr)
        {
            eStatus = MOS_STATUS_INVALID_PARAMETER;
            break;
        }

        info->uiInputCtr[0] = m_encryptionParams[ENCODE_ENCRYPTION_PARAMS_INDEX].dwPavpAesCounter[0];
        info->uiInputCtr[1] = m_encryptionParams[ENCODE_ENCRYPTION_PARAMS_INDEX].dwPavpAesCounter[1];
        info->uiInputCtr[2] = m_encryptionParams[ENCODE_ENCRYPTION_PARAMS_INDEX].dwPavpAesCounter[2];
        info->uiInputCtr[3] = m_encryptionParams[ENCODE_ENCRYPTION_PARAMS_INDEX].dwPavpAesCounter[3];
        DDI_CP_NORMALMESSAGE("AddTo: AES counter[0] = %d", m_encryptionParams[ENCODE_ENCRYPTION_PARAMS_INDEX].dwPavpAesCounter[0]);
    } while (false);

    return eStatus;
}

bool DdiCpNext::IsHdcpTearDown()
{
    DDI_CP_FUNC_ENTER;

    return false;
}

void DdiCpNext::SetInputResourceEncryption(PMOS_INTERFACE osInterface, PMOS_RESOURCE resource)
{
    DDI_CP_FUNC_ENTER;

    DDI_CP_CHK_NULL(osInterface, "Null pOSInterface",);
    DDI_CP_CHK_NULL(resource, "Null pResource",);
    DDI_CP_CHK_NULL(osInterface->osCpInterface, "Null osCpInterface", );

    if (ecbInputEnabled)
    {
        osInterface->osCpInterface->SetResourceEncryption(resource, true);
    }
}

// WiDi encode is not POR for Linux
//HDCP interfaces are not yet defined
VAStatus DdiCpNext::ParseCpParamsForEncode()
{
    DDI_CP_FUNC_ENTER;

    return VA_STATUS_SUCCESS;
}

void DdiCpNext::SetCpFlags(int32_t flag)
{
    DDI_CP_FUNC_ENTER;

    isHDCP2Enabled  = flag & VA_HDCP_ENABLED;
    ecbInputEnabled = flag & VA_ECB_INPUT_ENABLED;
    isPAVPEnabled   = flag & VA_PAVP_ENABLED;
}

bool DdiCpNext::IsHdcp2Enabled()
{
    DDI_CP_FUNC_ENTER;

    return isHDCP2Enabled;
}

VAStatus DdiCpNext::CreateCencDecode(
    CodechalDebugInterface      *debugInterface,
    PMOS_CONTEXT                osContext,
    CodechalSetting *           settings)
{
    DDI_CP_FUNC_ENTER;

    if (settings && settings->secureMode)
    {
        CodechalCencDecodeNext::Create((CODECHAL_STANDARD)settings->standard, &m_cencDecoder);
        if (nullptr == m_cencDecoder)
        {
            DDI_CP_ASSERTMESSAGE("Failure in CreateCencDecode create.\n");
            MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, VA_STATUS_ERROR_ALLOCATION_FAILED);
            return VA_STATUS_ERROR_ALLOCATION_FAILED;
        }

        settings->cpParams = &m_encryptionParams[CENC_ENCRYPTION_PARAMS_INDEX];
        if (m_cencDecoder->Initialize(osContext, settings) != MOS_STATUS_SUCCESS)
        {
            DDI_CP_ASSERTMESSAGE("Failure in CreateCencDecode create.\n");
            MT_ERR2(MT_CP_CENC_DECODE_CREATE, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, VA_STATUS_ERROR_ALLOCATION_FAILED);
            return VA_STATUS_ERROR_ALLOCATION_FAILED;
        }
    }

    return VA_STATUS_SUCCESS;
}

VAStatus DdiCpNext::SetDecodeParams(
    decode::DDI_DECODE_CONTEXT *ddiDecodeContext,
    CodechalSetting *setting
)
{
    DDI_CP_FUNC_ENTER;

    DDI_CHK_NULL(ddiDecodeContext, "nullptr ddiDecodeContext", VA_STATUS_ERROR_INVALID_PARAMETER);
    DDI_CHK_NULL(setting, "nullptr setting", VA_STATUS_ERROR_INVALID_PARAMETER);

    if (setting->secureMode)
    {
        bool shortFormatInUse   = false;
        bool updateDecodeFormat = false;

        if (setting->standard == CODECHAL_AVC)
        {
            // Reset setting->shortFormatInUse and pCodechal->m_shortFormatInUse
            // to ddiDecodeContext->bShortFormatInUse. setting->shortFormatInUse is
            // set to false in DdiCp::SetCpParams().
            shortFormatInUse = ddiDecodeContext->bShortFormatInUse;
            setting->shortFormatInUse = shortFormatInUse;
            updateDecodeFormat = true;
        }

        // fullsample
        if (m_cencDecoder && m_cencParams.dwDataBufferSize)
        {
            m_cencDecoder->SetDecodeParams(&(ddiDecodeContext->DecodeParams), &m_encryptionParams[DECODE_ENCRYPTION_PARAMS_INDEX]);

            // this happens after fullsample huc decrypt, before decoding. Therefore we can clear
            // dwDataBufferSize and dwNumSegments variables for next frame
            m_cencParams.dwDataBufferSize = 0;
            m_cencParams.dwNumSegments = 0;

            // In AVC fullsample, shortFormatInUse needs to set to true.
            if (setting->standard == CODECHAL_AVC)
            {
                shortFormatInUse = true;
                updateDecodeFormat = true;
            }
        }

        if(updateDecodeFormat)
        {
            //update decode format in decoder hal
            if(ddiDecodeContext->pCodecHal->IsApogeiosEnabled())
            {
                DecodePipelineAdapter *pAdapter = dynamic_cast<DecodePipelineAdapter*>(ddiDecodeContext->pCodecHal);
                DDI_CHK_NULL(pAdapter, "pAdapter is nullptr, failed to update decode format", VA_STATUS_ERROR_INVALID_PARAMETER);
                pAdapter->SetDecodeFormat(shortFormatInUse);
            }
            else
            {   
                DDI_CP_ASSERTMESSAGE("Only softlet supportted");
            }
        }
    }

    return VA_STATUS_SUCCESS;
}

VAStatus DdiCpNext::EndPicture(
    VADriverContextP    ctx,
    VAContextID         context
)
{
    DDI_CP_FUNC_ENTER;

    DDI_CP_CHK_NULL( ctx, "nullptr ctx", VA_STATUS_ERROR_INVALID_CONTEXT );

    uint32_t  ctxType;
    decode::PDDI_DECODE_CONTEXT decCtx  = (decode::PDDI_DECODE_CONTEXT)MediaLibvaCommonNext::GetContextFromContextID(ctx, context, &ctxType);
    DDI_CP_CHK_NULL(decCtx, "nullptr decCtx", VA_STATUS_ERROR_INVALID_CONTEXT);

    if (decCtx)
    {
        PDDI_MEDIA_CONTEXT mediaCtx = DdiMedia_GetMediaContext(ctx);
        DDI_CP_CHK_NULL(decCtx->m_ddiDecodeNext, "nullptr m_ddiDecodeNext", VA_STATUS_ERROR_INVALID_SURFACE);
        DDI_CHK_RET(decCtx->m_ddiDecodeNext->DecodeCombineBitstream(mediaCtx), "Fail to combine bitstream");
        DDI_CODEC_COM_BUFFER_MGR *bufMgr = &(decCtx->BufMgr);
        bufMgr->dwNumSliceData    = 0;
        bufMgr->dwNumSliceControl = 0;
        DDI_CP_CHK_NULL(decCtx->pCpDdiInterfaceNext, "nullptr pCpDdiInterfaceNext", VA_STATUS_ERROR_INVALID_SURFACE);
        DdiCpNext* ddiCp = dynamic_cast<DdiCpNext*>(decCtx->pCpDdiInterfaceNext);
        DDI_CP_CHK_NULL(ddiCp, "failed to cast to DdiCpNext", VA_STATUS_ERROR_INVALID_VALUE);
        DDI_CHK_RET(ddiCp->EndPictureCenc(ctx, context), "Fail to end picture process for cenc query call");

        m_isCencProcessing = false;
    }

    return VA_STATUS_SUCCESS;
}

VAStatus DdiCpNext::IsAttachedSessionAlive()
{
    DDI_CP_FUNC_ENTER;

    VAStatus va = VA_STATUS_SUCCESS;

#ifdef VA_DRIVER_VTABLE_PROT_VERSION
    do
    {
        if (m_attachedCpSession)
        {
            HRESULT hr = S_OK;
            BOOL bIsSessionAlive = FALSE;

            hr = m_attachedCpSession->IsSessionInitialized(bIsSessionAlive);
            if (FAILED(hr))
            {
                DDI_CP_ASSERTMESSAGE("IsSessionInitialized failed. Error = 0x%x.", hr);
                va = VA_STATUS_ERROR_OPERATION_FAILED;
                MT_ERR2(MT_CP_SESSION_INIT, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }

            if (!bIsSessionAlive)
            {
                va = VA_STATUS_ERROR_OPERATION_FAILED;
            }
        }
    } while (false);
#endif

    return va;
}

bool DdiCpNext::IsCencProcessing()
{
    DDI_CP_FUNC_ENTER;

    return m_isCencProcessing;
}

void DdiCpNext::UpdateCpContext(uint32_t value, DDI_CP_CONFIG_ATTR &cpConfigAttr)
{
    DDI_CP_FUNC_ENTER;

    uint32_t index = DECODE_ENCRYPTION_PARAMS_INDEX;

    DDI_CP_VERBOSEMESSAGE("value=0x%08x", value);

    m_ddiCpContext.value = value;

    if (cpConfigAttr.uiSessionType == VA_PC_SESSION_TYPE_TRANSCODE ||
        cpConfigAttr.uiUsage == VA_PC_USAGE_WIDI_STREAM)
    {
        index = ENCODE_ENCRYPTION_PARAMS_INDEX;
        cpInstanceCount = m_ddiCpContext.instanceId;
    }

    m_encryptionParams[index].CpTag = value;
    m_encryptionParams[CENC_ENCRYPTION_PARAMS_INDEX].CpTag = value;

    switch (cpConfigAttr.uiCipherMode)
    {
        case VA_PC_CIPHER_MODE_CBC:
            m_encryptionParams[index].HostEncryptMode = CP_TYPE_BASIC;
            m_encryptionParams[index].PavpEncryptionType = CP_SECURITY_AES128_CBC;
            m_encryptionParams[CENC_ENCRYPTION_PARAMS_INDEX].HostEncryptMode = CENC_TYPE_CBC;
            m_encryptionParams[CENC_ENCRYPTION_PARAMS_INDEX].PavpEncryptionType = CP_SECURITY_AES128_CBC;
            ///TODO: encode parameters
            break;
        case VA_PC_CIPHER_MODE_CTR:
            m_encryptionParams[index].HostEncryptMode = CP_TYPE_BASIC;
            m_encryptionParams[index].PavpEncryptionType = CP_SECURITY_AES128_CTR;
            m_encryptionParams[CENC_ENCRYPTION_PARAMS_INDEX].HostEncryptMode = CENC_TYPE_CTR;
            m_encryptionParams[CENC_ENCRYPTION_PARAMS_INDEX].PavpEncryptionType = CP_SECURITY_AES128_CTR;
            // encode parameters
            if (cpConfigAttr.uiSessionType == VA_PC_SESSION_TYPE_TRANSCODE)
            {
                m_encryptionParams[ENCODE_ENCRYPTION_PARAMS_INDEX].uiCounterIncrement   = DDI_ENCODE_AES_COUNTER_INCREASE_STEP;
                memset(m_encryptionParams[ENCODE_ENCRYPTION_PARAMS_INDEX].dwPavpAesCounter, 0x00, CODEC_DWORDS_PER_AES_BLOCK*sizeof(uint32_t));
            }
            break;
    }
}

void DdiCpNext::UpdateCpContext(DdiCpInterfaceNext *cpInterface)
{
    DDI_CP_FUNC_ENTER;

    if (cpInterface == nullptr)
        return;

    DdiCpNext *ddiCp = dynamic_cast<DdiCpNext*>(cpInterface);
    if (ddiCp == nullptr)
        return;

    m_ddiCpContext.value = ddiCp->m_ddiCpContext.value;
    MOS_SecureMemcpy(m_encryptionParams, sizeof(m_encryptionParams), ddiCp->m_encryptionParams, sizeof(m_encryptionParams));
}

void DdiCpNext::ResetCpContext(void)
{
    DDI_CP_FUNC_ENTER;

    m_ddiCpContext.value = 0;
    MOS_ZeroMemory(m_encryptionParams, sizeof(m_encryptionParams));
}

bool DdiCpNext::IsAttached()
{
    DDI_CP_FUNC_ENTER;

    return m_attachedCpSession ? true : false;
}

void DdiCpNext::SetAttached(CPavpCryptoSessionVANext *cpSession)
{
    DDI_CP_FUNC_ENTER;

    m_attachedCpSession = cpSession;
}
