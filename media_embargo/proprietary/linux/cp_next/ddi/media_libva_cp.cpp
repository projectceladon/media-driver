/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2015-2020
Intel Corporation All Rights Reserved.

The source code contained or described herein and all documents related to the
source code ("Material") are owned by Intel Corporation or its suppliers or
licensors. Title to the Material remains with Intel Corporation or its suppliers
and licensors. The Material contains trade secrets and proprietary and confidential
information of Intel or its suppliers and licensors. The Material is protected by
worldwide copyright and trade secret laws and treaty provisions. No part of the
Material may be used, copied, reproduced, modified, published, uploaded, posted,
transmitted, distributed, or disclosed in any way without Intel's prior express
written permission.

No license under any patent, copyright, trade secret or other intellectual
property right is granted to or conferred upon you by disclosure or delivery
of the Materials, either expressly, by implication, inducement, estoppel
or otherwise. Any license under such intellectual property rights must be
express and approved by Intel in writing.

======================= end_copyright_notice ==================================*/
//!
//! \file     media_libva_cp.cpp
//! \brief    DDI interface for content protection parameters and operations.
//! \details  Other components will call this interface for content protection operations directly
//!

#include "media_libva_cp.h"
#include "media_libva_decoder.h"
#include "media_libva_encoder.h"
#include "media_libva_util.h"
#include "media_libva_vp.h"
#include "media_libva_caps.h"
#include "media_libva_caps_cp.h"
#include "media_ddi_decode_base.h"
#include "codechal_decode_avc.h"
#include "codechal_encoder_base.h"
#include "codechal_setting.h"
#include "va_protected_content_private.h"
#include "mhw_cp.h"
#include "mhw_cp_hwcmd_common.h"
#include "mos_pxp_sm.h"
#include "cp_pavp_cryptosession_va.h"

#define DDI_ENCODE_AES_COUNTER_INCREASE_STEP 0xc000;

DdiCpInterface* Create_DdiCp(MOS_CONTEXT* pMosCtx)
{
    return MOS_New(DdiCp, *pMosCtx);
}

void Delete_DdiCp(DdiCpInterface* pDdiCpInterface)
{
    MOS_Delete(pDdiCpInterface);
}

DdiCp::DdiCp(MOS_CONTEXT& mosCtx):DdiCpInterface(mosCtx)
{
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

DdiCp::~DdiCp()
{
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
void DdiCp::SetCpParams(uint32_t encryptionType, CodechalSetting *setting)
{
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
        // In secure AVC fullsample, we will set shortFormatInUse to true in DdiCp::SetDecodeParams
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

void DdiCp::SetCryptoKeyExParams(uint32_t encryptionType, uint32_t *EncryptedDecryptKey)
{
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
            m_encryptionParams[idx].bInsertKeyExinVcsProlog = TRUE;
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
                DDI_ASSERTMESSAGE("SetCryptoKeyExParams failed");
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
            PMOS_INTERFACE osinterface = m_cencDecoder->GetOsInterface();
            if(osinterface)
            {
                PLATFORM platform;
                osinterface->pfnGetPlatform(osinterface, &platform);

                //WA: Add platform check to avoid key exchange command on GEN11- platforms.
                //Because codec command does not work with key exchang command on GEN11- Linux. Will remove platform check after fix command buffer issue.
                //Programming note for ICL+: All VCS commands must include an inline key exchange command at the beginning if Kb-based key refresh is in use
                if(platform.eProductFamily >= IGFX_ICELAKE)
                    m_encryptionParams[idx].bInsertKeyExinVcsProlog = TRUE;
            }
        }
    }
}

VAStatus DdiCp::ParseEncryptionParamsforDecrypt(void *params)
{
    auto pDecryptInputParams = (VAEncryptionParameters *)params;

    DDI_CHK_NULL(pDecryptInputParams,"NULL pDecryptInputParams in ParseEncryptionParamsforDecrypt!", VA_STATUS_ERROR_INVALID_CONTEXT);

    m_cencParams.ui16StatusReportFeedbackNumber = pDecryptInputParams->status_report_index;
    m_cencParams.dwNumSegments = pDecryptInputParams->num_segments;

    DDI_NORMALMESSAGE("num_segments=%d", m_cencParams.dwNumSegments);

    if (m_cencParams.dwNumSegments > 0)
    {
        DDI_CHK_NULL(pDecryptInputParams->segment_info,"NULL pDecryptInputParams->segment_info!", VA_STATUS_ERROR_INVALID_CONTEXT);

        if(nullptr == m_cencParams.pSegmentInfo) //allocate new buffer
        {
            m_cencParams.pSegmentInfo = (PMHW_SEGMENT_INFO)MOS_AllocAndZeroMemory(m_cencParams.dwNumSegments * sizeof(MHW_SEGMENT_INFO));
            DDI_CHK_NULL(m_cencParams.pSegmentInfo, "Allocate CENC segment buffer Failed!", VA_STATUS_ERROR_INVALID_CONTEXT);
            m_numSegmentsInBuffer = m_cencParams.dwNumSegments;
        }
        else if(m_cencParams.dwNumSegments > m_numSegmentsInBuffer) //resize buffer for more segments
        {
            m_cencParams.pSegmentInfo = (PMHW_SEGMENT_INFO)MOS_ReallocMemory(m_cencParams.pSegmentInfo, m_cencParams.dwNumSegments * sizeof(MHW_SEGMENT_INFO));
            DDI_CHK_NULL(m_cencParams.pSegmentInfo, "Reallocate CENC segment buffer Failed!", VA_STATUS_ERROR_INVALID_CONTEXT);
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

            DDI_NORMALMESSAGE("  init=%d, off=%d, partial=%d, length=%d",
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

VAStatus DdiCp::ParseEncryptionParamsforDecode(void  *params)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    DDI_CHK_NULL(params, "NULL params in DdiCp::ParseEncryptionParamsforDecode!", VA_STATUS_ERROR_INVALID_CONTEXT);

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

    m_encryptionParams[DECODE_ENCRYPTION_PARAMS_INDEX].bInsertKeyExinVcsProlog = TRUE;
    m_encryptionParams[DECODE_ENCRYPTION_PARAMS_INDEX].dwIVSize = MHW_IV_SIZE_8; //8 bytes IV and 8 Bytes Counter
    m_encryptionParams[DECODE_ENCRYPTION_PARAMS_INDEX].CryptoKeyExParams.KeyExchangeKeyUse = CRYPTO_DECRYPT_AND_USE_NEW_KEY;
    m_encryptionParams[DECODE_ENCRYPTION_PARAMS_INDEX].CryptoKeyExParams.KeyExchangeKeySelect |= MHW_CRYPTO_DECRYPTION_KEY;
    MOS_SecureMemcpy(m_encryptionParams[DECODE_ENCRYPTION_PARAMS_INDEX].CryptoKeyExParams.EncryptedDecryptionKey,
        CODEC_DWORDS_PER_AES_BLOCK * sizeof(uint32_t),
        (uint32_t *)cipherData->wrapped_decrypt_blob,
        CODEC_DWORDS_PER_AES_BLOCK * sizeof(uint32_t));

    return vaStatus;
}

VAStatus DdiCp::ParseEncryptionParamsforHUCDecode(void  *params)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    bool is_stripe = false;

    DDI_CHK_NULL(params, "NULL params in DdiCp::ParseEncryptionParamsforHUCDecode!", VA_STATUS_ERROR_INVALID_CONTEXT);

    auto cipherData = (VAEncryptionParameters *)params;

    if (cipherData->segment_info && cipherData->num_segments > 0)
    {
        if(!m_subSampleMappingBlocks.empty())
            m_subSampleMappingBlocks.clear();

        DDI_NORMALMESSAGE("num_segments=%d, stripe_enc=%d, stripe_clr=%d",
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

            DDI_NORMALMESSAGE("  init=%d, off=%d, partial=%d, length=%d, clr_s=%d, enc_s=%d",
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

VAStatus DdiCp::ParseEncryptionParamsforEncode(void  *params)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    DDI_CHK_NULL(params, "NULL params in DdiCp::ParseEncryptionParamsforEncode!", VA_STATUS_ERROR_INVALID_CONTEXT);

    auto cipherData = (VAEncryptionParameters *)params;

    m_encryptionParams[ENCODE_ENCRYPTION_PARAMS_INDEX].HostEncryptMode = CP_TYPE_BASIC;
    m_encryptionParams[ENCODE_ENCRYPTION_PARAMS_INDEX].PavpEncryptionType = CP_SECURITY_BASIC;

    m_encryptionParams[ENCODE_ENCRYPTION_PARAMS_INDEX].bInsertKeyExinVcsProlog = TRUE;
    m_encryptionParams[ENCODE_ENCRYPTION_PARAMS_INDEX].dwIVSize = MHW_IV_SIZE_8;
    m_encryptionParams[ENCODE_ENCRYPTION_PARAMS_INDEX].CryptoKeyExParams.KeyExchangeKeyUse = CRYPTO_DECRYPT_AND_USE_NEW_KEY;
    m_encryptionParams[ENCODE_ENCRYPTION_PARAMS_INDEX].CryptoKeyExParams.KeyExchangeKeySelect |= MHW_CRYPTO_ENCRYPTION_KEY;

    auto *pPavpDevice = static_cast<CPavpDevice *>(m_attachedCpSession);

    if(pPavpDevice->IsGSK())
    {
        if (pPavpDevice->SetCryptoKeyExParams(&m_encryptionParams[ENCODE_ENCRYPTION_PARAMS_INDEX]) != S_OK)
        {
            DDI_ASSERTMESSAGE("ParseEncryptionParamsforEncode failed");
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

VAStatus DdiCp::EndPictureCenc (
    VADriverContextP vaDrvCtx,
    VAContextID      contextId)
{
    VAStatus                          va;

    UMD_ATRACE_BEGIN(__FUNCTION__);

    DDI_CHK_NULL(vaDrvCtx,"NULL vaDrvCtx in DdiCp::EndPictureCenc!", VA_STATUS_ERROR_INVALID_CONTEXT);

    // assume the VAContextID is decoder ID
    uint32_t                         ctxType;
    // assume the VAContextID is decoder ID
    auto decCtx     = (PDDI_DECODE_CONTEXT)DdiMedia_GetContextFromContextID(vaDrvCtx, contextId, &ctxType);
    DDI_CHK_NULL(decCtx,"Null decCtx in DdiCp::EndPictureCenc",VA_STATUS_ERROR_INVALID_CONTEXT);

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
                DDI_ASSERTMESSAGE("cencDecoder GetStatusReport error %d", va);
                MT_ERR2(MT_ERR_MOS_STATUS_CHECK, MT_MOS_STATUS, decstatus, MT_ERROR_CODE, va);
            }

            if (cencStatus->status == VA_ENCRYPTION_STATUS_SUCCESSFUL)
            {
                // Do nothing
            }
            else if (cencStatus->status == VA_ENCRYPTION_STATUS_INCOMPLETE)
            {
                // Report success to App with error code, skip the hang frame
                DDI_ASSERTMESSAGE("cencDecoder GetStatusReport incomplete");
                MT_ERR2(MT_CP_CENC_STATUS_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, cencStatus->status);
            }
            else if (cencStatus->status == VA_ENCRYPTION_STATUS_BUFFER_FULL)
            {
                // Return success to App for not breaking execution but enlarge the buffer and try again
                DDI_ASSERTMESSAGE("cencDecoder GetStatusReport buffer full");
                MT_ERR2(MT_CP_CENC_STATUS_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, cencStatus->status);
            }
            else if (cencStatus->status == VA_ENCRYPTION_STATUS_ERROR)
            {
                va = VA_STATUS_ERROR_DECODING_ERROR;
                DDI_ASSERTMESSAGE("cencDecoder decrypt status error %d", va);
                MT_ERR2(MT_CP_CENC_STATUS_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, va);
            }
        }
    }

    UMD_ATRACE_END;
    return va;
}

VAStatus DdiCp::RenderCencPicture (
    VADriverContextP      vaDrvctx,
    VAContextID           contextId,
    DDI_MEDIA_BUFFER      *buf,
    void                  *data)
{
    int32_t                       index;
    VAStatus                      status = VA_STATUS_SUCCESS;
    PDDI_DECODE_CONTEXT           decCtx = nullptr;
    PDDI_ENCODE_CONTEXT           encCtx = nullptr;

    DDI_CHK_NULL(vaDrvctx,"nullptr vaDrvctx in DdiCp::RenderDecryptPicture!", VA_STATUS_ERROR_INVALID_CONTEXT);

    uint32_t                         ctxType;
    auto ctx     = DdiMedia_GetContextFromContextID(vaDrvctx, contextId, &ctxType);

    DDI_CHK_NULL(ctx,   "nullptr ctx",   VA_STATUS_ERROR_INVALID_CONTEXT);
    DDI_CHK_NULL(buf,   "nullptr buf",   VA_STATUS_ERROR_INVALID_PARAMETER);

    switch(ctxType)
    {
    case DDI_MEDIA_CONTEXT_TYPE_DECODER:
        decCtx = reinterpret_cast<PDDI_DECODE_CONTEXT>(ctx);
        break;
    case DDI_MEDIA_CONTEXT_TYPE_ENCODER:
        encCtx = reinterpret_cast<PDDI_ENCODE_CONTEXT>(ctx);
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
            index = decCtx->m_ddiDecode->GetBitstreamBufIndexFromBuffer(&decCtx->BufMgr, buf);
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
            DDI_ASSERTMESSAGE("Not Supported Yet");
            MT_ERR2(MT_CP_DDI_FUNC_UNSUPPORTED, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, status);
        }
        break;
    }
    default:
        status = VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE;
    }

    return status;

}

VAStatus DdiCp::CreateBuffer(
    VABufferType             type,
    DDI_MEDIA_BUFFER*        buffer,
    uint32_t                 size,
    uint32_t                 num_elements)
{
    DDI_CHK_NULL(buffer,"Null buffer",VA_STATUS_ERROR_INVALID_CONTEXT);

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
VAStatus DdiCp::InitHdcp2Buffer(DDI_CODEC_COM_BUFFER_MGR* bufMgr)
{
    DDI_CHK_NULL(bufMgr,"Null bufMgr",VA_STATUS_ERROR_INVALID_CONTEXT);

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

VAStatus DdiCp::StatusReportForHdcp2Buffer(
    DDI_CODEC_COM_BUFFER_MGR*       bufMgr,
    void*               encodeStatusReport)
{
    DDI_CHK_NULL(bufMgr,"Null bufMgr",VA_STATUS_ERROR_INVALID_CONTEXT);
    DDI_CHK_NULL(encodeStatusReport,"Null info",VA_STATUS_ERROR_INVALID_CONTEXT);

    if (IsHdcp2Enabled() || isPAVPEnabled)
    {
        VACodedBufferSegment *hdCpCodedBufferSegment = nullptr;

        hdCpCodedBufferSegment = (VACodedBufferSegment *)bufMgr->pCodedBufferSegment->next;
        DDI_CHK_NULL(hdCpCodedBufferSegment, "Null hdCpCodedBufferSegment", VA_STATUS_ERROR_INVALID_PARAMETER);

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
                DDI_NORMALMESSAGE("GetFrom: AES counter = %lld", pencodeStatusReport->HWCounterValue.Count);
            }
        }

        if (IsHdcpTearDown())
        {
            hdCpCodedBufferSegment->status |= VA_CODED_BUF_STATUS_HW_TEAR_DOWN;
            DDI_NORMALMESSAGE("HDCP2 tear down happen! status=0x%x ", hdCpCodedBufferSegment->status);
        }
        else  // tear down didn't happen
        {
            hdCpCodedBufferSegment->status &= (~VA_CODED_BUF_STATUS_HW_TEAR_DOWN);
        }
    }

    return VA_STATUS_SUCCESS;
}

void DdiCp::FreeHdcp2Buffer(DDI_CODEC_COM_BUFFER_MGR* bufMgr)
{
    DDI_CHK_NULL(bufMgr,"Null bufMgr", );

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

MOS_STATUS DdiCp::StoreCounterToStatusReport(
    PDDI_ENCODE_STATUS_REPORT_INFO info)
{
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
        DDI_NORMALMESSAGE("AddTo: AES counter[0] = %d", m_encryptionParams[ENCODE_ENCRYPTION_PARAMS_INDEX].dwPavpAesCounter[0]);
    } while (false);

    return eStatus;
}

bool DdiCp::IsHdcpTearDown()
{
    return false;
}

void DdiCp::SetInputResourceEncryption(PMOS_INTERFACE osInterface, PMOS_RESOURCE resource)
{
    DDI_CHK_NULL(osInterface, "Null pOSInterface",);
    DDI_CHK_NULL(resource, "Null pResource",);
    DDI_CHK_NULL(osInterface->osCpInterface, "Null osCpInterface", );

    if (ecbInputEnabled)
    {
        osInterface->osCpInterface->SetResourceEncryption(resource, true);
    }
}

// WiDi encode is not POR for Linux
//HDCP interfaces are not yet defined
VAStatus DdiCp::ParseCpParamsForEncode()
{
    return VA_STATUS_SUCCESS;
}

void DdiCp::SetCpFlags(int32_t flag)
{
    isHDCP2Enabled  = flag & VA_HDCP_ENABLED;
    ecbInputEnabled = flag & VA_ECB_INPUT_ENABLED;
    isPAVPEnabled   = flag & VA_PAVP_ENABLED;
}

bool DdiCp::IsHdcp2Enabled()
{
    return isHDCP2Enabled;
}

VAStatus DdiCp::CreateCencDecode(
    CodechalDebugInterface      *debugInterface,
    PMOS_CONTEXT                osContext,
    CodechalSetting *           settings)
{
    if (settings && settings->secureMode)
    {
        CodechalCencDecode::Create((CODECHAL_STANDARD)settings->standard, &m_cencDecoder);
        if (nullptr == m_cencDecoder)
        {
            DDI_ASSERTMESSAGE("Failure in CreateCencDecode create.\n");
            MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, VA_STATUS_ERROR_ALLOCATION_FAILED);
            return VA_STATUS_ERROR_ALLOCATION_FAILED;
        }

        settings->cpParams = &m_encryptionParams[CENC_ENCRYPTION_PARAMS_INDEX];
        if (m_cencDecoder->Initialize(osContext, settings) != MOS_STATUS_SUCCESS)
        {
            DDI_ASSERTMESSAGE("Failure in CreateCencDecode create.\n");
            MT_ERR2(MT_CP_CENC_DECODE_CREATE, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, VA_STATUS_ERROR_ALLOCATION_FAILED);
            return VA_STATUS_ERROR_ALLOCATION_FAILED;
        }
    }

    return VA_STATUS_SUCCESS;
}

VAStatus DdiCp::SetDecodeParams(
    DDI_DECODE_CONTEXT *ddiDecodeContext,
    CodechalSetting *setting
)
{
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
                CodechalDecodeAvc *pCodechal = dynamic_cast<CodechalDecodeAvc*>(ddiDecodeContext->pCodecHal);
                DDI_CHK_NULL(pCodechal, "pCodechal is nullptr, failed to update decode format.", VA_STATUS_ERROR_INVALID_PARAMETER);
                // CodechalDecodeAvc::m_shortFormatInUse is used in
                //   CodechalDecodeAvc::SetFrameStates,
                //   CodechalDecodeAvc::InitPicMhwParams
                //   CodechalDecodeAvc::AddPictureCmds
                //   CodechalDecodeAvc::ParseSlice
                pCodechal->m_shortFormatInUse = shortFormatInUse;
            }
        }
    }

    return VA_STATUS_SUCCESS;
}

VAStatus DdiCp::EndPicture(
    VADriverContextP    ctx,
    VAContextID         context
)
{
    DDI_FUNCTION_ENTER();

    DDI_CHK_NULL( ctx, "nullptr ctx", VA_STATUS_ERROR_INVALID_CONTEXT );

    uint32_t  ctxType;
    PDDI_DECODE_CONTEXT decCtx  = (PDDI_DECODE_CONTEXT)DdiMedia_GetContextFromContextID(ctx, context, &ctxType);
    DDI_CHK_NULL(decCtx, "nullptr decCtx", VA_STATUS_ERROR_INVALID_CONTEXT);

    if (decCtx)
    {
        PDDI_MEDIA_CONTEXT mediaCtx = DdiMedia_GetMediaContext(ctx);
        DDI_CHK_NULL(decCtx->m_ddiDecode, "nullptr m_ddiDecode", VA_STATUS_ERROR_INVALID_SURFACE);
        DDI_CHK_RET(decCtx->m_ddiDecode->DecodeCombineBitstream(mediaCtx), "Fail to combine bitstream");
        DDI_CODEC_COM_BUFFER_MGR *bufMgr = &(decCtx->BufMgr);
        bufMgr->dwNumSliceData    = 0;
        bufMgr->dwNumSliceControl = 0;
        DDI_CHK_NULL(decCtx->pCpDdiInterface, "nullptr pCpDdiInterface", VA_STATUS_ERROR_INVALID_SURFACE);
        DdiCp* ddiCp = dynamic_cast<DdiCp*>(decCtx->pCpDdiInterface);
        DDI_CHK_NULL(ddiCp, "failed to cast to DdiCp", VA_STATUS_ERROR_INVALID_VALUE);
        DDI_CHK_RET(ddiCp->EndPictureCenc(ctx, context), "Fail to end picture process for cenc query call");

        m_isCencProcessing = false;
    }

    DDI_FUNCTION_EXIT(VA_STATUS_SUCCESS);
    return VA_STATUS_SUCCESS;
}

VAStatus DdiCp::IsAttachedSessionAlive()
{
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
                DDI_ASSERTMESSAGE("IsSessionInitialized failed. Error = 0x%x.", hr);
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

bool DdiCp::IsCencProcessing()
{
    return m_isCencProcessing;
}

void DdiCp::UpdateCpContext(uint32_t value, DDI_CP_CONFIG_ATTR &cpConfigAttr)
{
    uint32_t index = DECODE_ENCRYPTION_PARAMS_INDEX;

    DDI_VERBOSEMESSAGE("value=0x%08x", value);

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

void DdiCp::UpdateCpContext(DdiCpInterface *cpInterface)
{
    if (cpInterface == nullptr)
        return;

    DdiCp *ddiCp = dynamic_cast<DdiCp*>(cpInterface);
    if (ddiCp == nullptr)
        return;

    m_ddiCpContext.value = ddiCp->m_ddiCpContext.value;
    MOS_SecureMemcpy(m_encryptionParams, sizeof(m_encryptionParams), ddiCp->m_encryptionParams, sizeof(m_encryptionParams));
}

void DdiCp::ResetCpContext(void)
{
    m_ddiCpContext.value = 0;
    MOS_ZeroMemory(m_encryptionParams, sizeof(m_encryptionParams));
}

bool DdiCp::IsAttached()
{
    return m_attachedCpSession ? true : false;
}

void DdiCp::SetAttached(CPavpCryptoSessionVA *cpSession)
{
    m_attachedCpSession = cpSession;
}
