/*
* Copyright (c) 2020-2022, Intel Corporation
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
//! \file     decodecp.cpp
//! \brief    Impelements the public interface for CodecHal Secure Decode
//!

#include <vector>
#include <utility>
#include "decodecp.h"
#include "codechal_setting_ext.h"
#include "cp_ind_packet.h"
#include "cp_ind_packet_register.h"
#include "decode_sub_packet.h"
#include "decode_pipeline.h"

DecodeCp::DecodeCp(
    MhwCpBase      *mhwCp,
    MhwCpInterface *cpInterface,
    PMOS_INTERFACE  osInterface)
{
    m_mhwCp       = mhwCp;
    m_cpInterface = cpInterface;
    m_osInterface = osInterface;
    if (m_osInterface)
    {
        m_userSettingPtr = m_osInterface->pfnGetUserSettingInstance(m_osInterface);

        DeclareUserSettingKey(
            m_userSettingPtr,
            "PAVP Decode HuC Stream Decryption",
            MediaUserSetting::Group::Sequence,
            int32_t(0),
            true);
        DeclareUserSettingKey(
            m_userSettingPtr,
            "Id of PAVP Lite Mode Enable Huc StreamOut For Debug",
            MediaUserSetting::Group::Sequence,
            int32_t(0),
            true);
    }
}

MOS_STATUS DecodeCp::Init(CodechalSetting *codecHalSettings)
{
    if (codecHalSettings != nullptr)
    {
        switch (codecHalSettings->standard)
        {
        case CODECHAL_RESERVED1:
            m_decodeCpMode = DECODE_PIPELINE_EXTENDED;
            DECODE_CP_NORMALMESSAGE("%s: Using VVC pipline to decrypt stream.\n", __FUNCTION__);
            break;
        default:
            break;
        }
    }
    else
    {
        DECODE_CP_ASSERTMESSAGE("%s: codecHalSettings is nullptr, won't change decodecp mode.\n", __FUNCTION__);
        return MOS_STATUS_INVALID_PARAMETER;
    }

#if (_DEBUG || _RELEASE_INTERNAL)
    MediaUserSetting::Value outValue;
    auto status = ReadUserSettingForDebug(m_userSettingPtr,
        outValue,
        "PAVP Decode HuC Stream Decryption",
        MediaUserSetting::Group::Sequence);
    int32_t data = outValue.Get<int32_t>();

    if (status == MOS_STATUS_SUCCESS && data == 0)
    {
        m_decodeCpMode = DECODE_PIPELINE_EXTENDED;
        DECODE_CP_NORMALMESSAGE("%s: Using decode pipline to do stream decrypt.\n", __FUNCTION__);
    }
    else if (status == MOS_STATUS_SUCCESS && data != 0)
    {
        m_decodeCpMode = HUC_PIPELINE_EXTENDED;
        DECODE_CP_NORMALMESSAGE("%s: Using HuC pipline to do stream decrypt.\n", __FUNCTION__);
    }
#endif

    return MOS_STATUS_SUCCESS;
}

DecodeCp::~DecodeCp()
{
    m_hucCopySegmentLengths.clear();

    if (m_pSegmentInfo)
    {
        MOS_FreeMemory(m_pSegmentInfo);
        m_pSegmentInfo = nullptr;
        MOS_FreeMemory(m_pSampleRemappingBlock);
        m_pSampleRemappingBlock = nullptr;
    }

    if (m_pSubsampleSegmentEnd)
    {
        MOS_FreeMemory(m_pSubsampleSegmentEnd);
        m_pSubsampleSegmentEnd = nullptr;
    }

    if (m_pSubsampleSegmentNum)
    {
        MOS_FreeMemory(m_pSubsampleSegmentNum);
        m_pSubsampleSegmentNum = nullptr;
    }

    if (m_pSampleSliceRemappingBlock)
    {
        MOS_FreeMemory(m_pSampleSliceRemappingBlock);
        m_pSampleSliceRemappingBlock = nullptr;
    }

    if (m_sliceSegmentNum)
    {
        MOS_FreeMemory(m_sliceSegmentNum);
        m_sliceSegmentNum = nullptr;
    }
}

MOS_STATUS DecodeCp::UpdateParams(bool input)
{
    DECODE_CP_FUNCTION_ENTER;
    DECODE_CP_CHK_NULL_RETURN(m_cpInterface);
    DECODE_CP_CHK_STATUS(m_cpInterface->UpdateParams(input));
    return MOS_STATUS_SUCCESS;
}
MOS_STATUS DecodeCp::RegisterParams(void *settings)
{
    DECODE_CP_FUNCTION_ENTER;
    DECODE_CP_CHK_NULL_RETURN(settings);
    DECODE_CP_CHK_NULL_RETURN(m_cpInterface);

    m_cpInterface->RegisterParams(((CodechalSetting *)settings)->GetCpParams());
    return MOS_STATUS_SUCCESS;
}
DecodeCpInterface *Create_DecodeCp(
    CodechalSetting *codecHalSettings,
    MhwCpBase       *mhwCp,
    MhwCpInterface  *cpInterface,
    PMOS_INTERFACE   osInterface)
{
    bool       hucBasedDrmInUse = false;
    bool       shortFormatInUse = false;
    MOS_STATUS eStatus          = MOS_STATUS_SUCCESS;
    DecodeCp * pDecodeCp        = nullptr;

    DECODE_CP_FUNCTION_ENTER;

    do
    {
        if (codecHalSettings == nullptr)
        {
            DECODE_CP_ASSERTMESSAGE("%s: codecHalSettings check null pointer failed\n", __FUNCTION__);
            break;
        }
        if (osInterface == nullptr)
        {
            DECODE_CP_ASSERTMESSAGE("%s: osInterface check null pointer failed\n", __FUNCTION__);
            break;
        }
        if (cpInterface == nullptr)
        {
            DECODE_CP_ASSERTMESSAGE("%s: osInterface check null pointer failed\n", __FUNCTION__);
            break;
        }
        if (mhwCp == nullptr)
        {
            DECODE_CP_ASSERTMESSAGE("%s: mhwCp check null pointer failed\n", __FUNCTION__);
            break;
        }
        pDecodeCp = MOS_New(DecodeCp, mhwCp, cpInterface, osInterface);
        if (pDecodeCp == nullptr)
        {
            DECODE_CP_ASSERTMESSAGE("%s: pDecodeCp check null pointer failed\n", __FUNCTION__);
            break;
        }
        if (pDecodeCp->Init(codecHalSettings) != MOS_STATUS_SUCCESS)
        {
            MOS_FreeMemAndSetNull(pDecodeCp);
            DECODE_CP_ASSERTMESSAGE("%s: Decodecp init failed.\n", __FUNCTION__);
        }
    } while (false);
    return pDecodeCp;
}

void Delete_DecodeCp(DecodeCpInterface *pDecodeCpInterface)
{
    MOS_Delete(pDecodeCpInterface);
}

MOS_STATUS DecodeCp::SetHucPipeModeSelectParameter(bool &disableProtection)
{
    if (m_pSegmentInfo != nullptr && m_osInterface->osCpInterface->IsCpEnabled())
    {
        disableProtection = false;
    }
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS DecodeCp::AddHcpState(
    PMOS_COMMAND_BUFFER cmdBuffer,
    PMOS_RESOURCE       presDataBuffer,
    uint32_t            length,
    uint32_t            startoffset,
    uint32_t            dwsliceIndex)
{
    MHW_PAVP_AES_PARAMS AESParams;
    MOS_STATUS          eStatus = MOS_STATUS_SUCCESS;

    DECODE_CP_CHK_NULL_RETURN(cmdBuffer);
    DECODE_CP_CHK_NULL_RETURN(presDataBuffer);

    if (m_mhwCp != nullptr && m_mhwCp->GetHostEncryptMode() == CP_TYPE_BASIC)
    {
        MOS_ZeroMemory(&AESParams, sizeof(AESParams));
        AESParams.presDataBuffer       = presDataBuffer;
        AESParams.dwDataLength         = length;
        AESParams.dwDataStartOffset    = startoffset;
        AESParams.bShortFormatInUse    = 0;
        AESParams.dwSliceIndex         = dwsliceIndex;
        AESParams.bLastPass            = 0;
        AESParams.dwTotalBytesConsumed = 0;

        eStatus = m_mhwCp->AddHcpAesState(
            true,
            cmdBuffer,
            nullptr,
            &AESParams);
    }

    return eStatus;
}

MOS_STATUS DecodeCp::AddHucState(
    PMOS_COMMAND_BUFFER cmdBuffer,
    PMOS_RESOURCE       presDataBuffer,
    uint32_t            length,
    uint32_t            startoffset,
    uint32_t            dwsliceIndex)
{
    MHW_PAVP_AES_PARAMS AESParams;
    MOS_STATUS          eStatus = MOS_STATUS_SUCCESS;

    DECODE_CP_CHK_NULL_RETURN(cmdBuffer);
    DECODE_CP_CHK_NULL_RETURN(presDataBuffer);

    if (m_mhwCp != nullptr && m_mhwCp->GetHostEncryptMode() == CP_TYPE_BASIC)
    {
        MOS_ZeroMemory(&AESParams, sizeof(AESParams));
        AESParams.presDataBuffer = presDataBuffer;
        m_mhwCp->SetCpAesCounter(InitialCounterPerSlice[dwsliceIndex]);
        AESParams.presDataBuffer       = presDataBuffer;
        AESParams.dwDataLength         = length;
        AESParams.dwDataStartOffset    = startoffset;
        AESParams.bShortFormatInUse    = 1;
        AESParams.dwSliceIndex         = dwsliceIndex;
        AESParams.bLastPass            = 0;
        AESParams.dwTotalBytesConsumed = 0;

        eStatus = m_mhwCp->AddHucAesState(cmdBuffer, &AESParams);
    }

    return eStatus;
}

MOS_STATUS DecodeCp::GetSliceInitValue(
    PMHW_PAVP_AES_PARAMS params,
    uint32_t *           value)
{
    MhwCpBase *     pCpInterface                     = nullptr;
    uint32_t        dwAesCtr[4]                      = {0, 0, 0, 0};
    uint32_t        dwCounterIncrement               = 0;
    uint32_t        dwNalUnitLength                  = 0;
    uint32_t        dwNumClearBytes                  = 0;
    uint32_t        dwFixedNumberOfClearBytesInSlice = MHW_PAVP_DECE_NUM_CLEAR_BYTES_IN_SLICE;
    uint32_t        dwZeroPadding                    = 0;
    uint32_t *      pdwFirstEncryptedOword           = nullptr;
    uint32_t *      pdwLastEncryptedOword            = nullptr;
    MOS_LOCK_PARAMS LockFlags;
    MOS_STATUS      eStatus = MOS_STATUS_SUCCESS;

    DECODE_CP_FUNCTION_ENTER;

    DECODE_CP_CHK_NULL_RETURN(m_mhwCp);

    eStatus = MOS_SecureMemcpy(dwAesCtr,
        CODEC_DWORDS_PER_AES_BLOCK * sizeof(uint32_t),
        m_mhwCp->GetCpAesCounter(),
        CODEC_DWORDS_PER_AES_BLOCK * sizeof(uint32_t));

    if (eStatus != MOS_STATUS_SUCCESS)
    {
        DECODE_CP_ASSERTMESSAGE("%s: Failed to copy memory\n", __FUNCTION__);
        return eStatus;
    }

    if (m_mhwCp->GetCpEncryptionType() == CP_SECURITY_DECE_AES128_CTR)
    {
        // Fair Play
        dwNalUnitLength = (params->bShortFormatInUse) ? MOS_NAL_UNIT_STARTCODE_LENGTH : MOS_NAL_UNIT_LENGTH;
        dwZeroPadding   = params->dwDataStartOffset - params->dwTotalBytesConsumed;
        dwNumClearBytes = dwFixedNumberOfClearBytesInSlice - dwZeroPadding;

        if (params->dwDataLength == MHW_PAVP_DECE_NUM_CLEAR_BYTES_IN_SLICE)
        {
            // Reset counter to zero
            dwAesCtr[0] = dwAesCtr[1] = dwAesCtr[2] = dwAesCtr[3] = 0;

            // No increment as well
            dwCounterIncrement = 0;
        }
        else
        {
            dwCounterIncrement = (params->dwTotalBytesConsumed >> 4) - (params->dwSliceIndex << 3);
        }
    }
    else if (m_mhwCp->GetCpEncryptionType() == CP_SECURITY_AES128_CBC)
    {
        // CBC
        DECODE_CP_CHK_NULL_RETURN(params->presDataBuffer);

        pdwLastEncryptedOword = nullptr;
        MOS_ZeroMemory(&LockFlags, sizeof(MOS_LOCK_PARAMS));
        LockFlags.ReadOnly     = 1;
        pdwFirstEncryptedOword = (uint32_t *)m_osInterface->pfnLockResource(
            m_osInterface,
            params->presDataBuffer,
            &LockFlags);

        DECODE_CP_CHK_NULL_RETURN(pdwFirstEncryptedOword);

        pdwLastEncryptedOword = pdwFirstEncryptedOword + ((params->dwDataStartOffset >> 4) * 4);
        if (pdwLastEncryptedOword != pdwFirstEncryptedOword)
        {
            pdwLastEncryptedOword -= KEY_DWORDS_PER_AES_BLOCK;  // The block size of AES-128 is 128 bits, so the subtrahend is 4.
            dwAesCtr[0] = pdwLastEncryptedOword[0];
            dwAesCtr[1] = pdwLastEncryptedOword[1];
            dwAesCtr[2] = pdwLastEncryptedOword[2];
            dwAesCtr[3] = pdwLastEncryptedOword[3];
        }

        m_osInterface->pfnUnlockResource(m_osInterface, params->presDataBuffer);

        dwCounterIncrement = 0;
    }
    else
    {
        // Standard AES CTR mode
        dwCounterIncrement = params->dwDataStartOffset >> 4;
    }

    // Decode increments the counter before adding the AES state to the command buffer
    m_mhwCp->IncrementCounter(
        dwAesCtr,
        dwCounterIncrement);

    eStatus = MOS_SecureMemcpy(value, sizeof(dwAesCtr), dwAesCtr, sizeof(dwAesCtr));

    if (eStatus != MOS_STATUS_SUCCESS)
    {
        DECODE_CP_ASSERTMESSAGE("%s: Failed to copy memory\n", __FUNCTION__);
    }
    return eStatus;
}

MOS_STATUS DecodeCp::SetHucDmemS2LPicBss(
    void *        hucS2LPicBss,
    PMOS_RESOURCE presDataBuffer)
{
    PHUC_S2L_PIC_BSS_EXT        hucS2LPicBssExt;
    PMHW_PAVP_ENCRYPTION_PARAMS pEncryptionParams = nullptr;
    MHW_PAVP_AES_PARAMS         AESParams;

    hucS2LPicBssExt            = (PHUC_S2L_PIC_BSS_EXT)(hucS2LPicBss);
    pEncryptionParams          = m_mhwCp->GetEncryptionParams();
    hucS2LPicBssExt->AESEnable = m_osInterface->osCpInterface->IsCpEnabled() ? (pEncryptionParams->HostEncryptMode == CP_TYPE_BASIC) : 0;

    if (hucS2LPicBssExt->AESEnable)
    {
        hucS2LPicBssExt->IsCTRMode              = (pEncryptionParams->PavpEncryptionType == CP_SECURITY_AES128_CBC) ? 0 : 1;
        hucS2LPicBssExt->CTRFlavor              = 2;
        hucS2LPicBssExt->IsFairPlay             = (pEncryptionParams->PavpEncryptionType == CP_SECURITY_DECE_AES128_CTR) ? 1 : 0;
        hucS2LPicBssExt->InitialFairPlayEnabled = 0;
        hucS2LPicBssExt->InitPktBytes           = 0;
        hucS2LPicBssExt->EncryptedBytes         = 0;
        hucS2LPicBssExt->ClearBytes             = 0;

        m_pDmemResBuffer = presDataBuffer;

        MOS_ZeroMemory(InitialCounterPerSlice, sizeof(InitialCounterPerSlice));
    }
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS DecodeCp::SetHucDmemS2LSliceBss(
    void *   hucS2LSliceBss,
    uint32_t index,
    uint32_t dataSize,
    uint32_t dataOffset)
{
    PHUC_S2L_SLICE_BSS_EXT      hucS2LSliceBssExt;
    PMHW_PAVP_ENCRYPTION_PARAMS pEncryptionParams = nullptr;
    MHW_PAVP_AES_PARAMS         AESParams;

    hucS2LSliceBssExt = (PHUC_S2L_SLICE_BSS_EXT)(hucS2LSliceBss);
    pEncryptionParams = m_mhwCp->GetEncryptionParams();

    if (pEncryptionParams->HostEncryptMode == CP_TYPE_BASIC && m_pDmemResBuffer != nullptr)
    {
        MOS_ZeroMemory(&AESParams, sizeof(AESParams));
        AESParams.presDataBuffer       = m_pDmemResBuffer;
        AESParams.dwDataLength         = dataSize;
        AESParams.dwDataStartOffset    = dataOffset;
        AESParams.bShortFormatInUse    = 1;
        AESParams.dwSliceIndex         = index;
        AESParams.bLastPass            = 0;
        AESParams.dwTotalBytesConsumed = 0;

        uint32_t dwAesCtr[4] = {0};
        DECODE_CP_CHK_STATUS(GetSliceInitValue(
            &AESParams,
            dwAesCtr));

        hucS2LSliceBssExt->InitializationCounterValue0 = InitialCounterPerSlice[index][0] = dwAesCtr[0];
        hucS2LSliceBssExt->InitializationCounterValue1 = InitialCounterPerSlice[index][1] = dwAesCtr[1];
        hucS2LSliceBssExt->InitializationCounterValue2 = InitialCounterPerSlice[index][2] = dwAesCtr[2];
        hucS2LSliceBssExt->InitializationCounterValue3 = InitialCounterPerSlice[index][3] = dwAesCtr[3];
    }
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS DecodeCp::AddHucIndState(PMOS_COMMAND_BUFFER cmdBuffer, PMOS_RESOURCE presIndState)
{
    MHW_PAVP_AES_PARAMS         AESParams;
    MHW_PAVP_AES_IND_PARAMS     IndAesParams;
    PMHW_PAVP_ENCRYPTION_PARAMS pEncryptionParams = nullptr;
    uint32_t                    curNumSegments;
    uint32_t                    segmentInfoOffset = 0;

    pEncryptionParams = m_mhwCp->GetEncryptionParams();
    curNumSegments    = (pEncryptionParams->dwBlocksStripeEncCount == 0) ? curSegmentNum : m_sampleRemappingBlockNum;

    if (m_splitWorkload)
    {
        DECODE_CP_NORMALMESSAGE("m_hucCopyLength = %d, current hucCopySegmentLengths[%d] = %d\n", m_hucCopyLength, m_indStateIdx, m_hucCopySegmentLengths[m_indStateIdx]);
        segmentInfoOffset = maxIndirectCryptoStateNum * m_indStateIdx;
        if (m_indStateIdx == m_indStateNum - 1)
        {
            curNumSegments -= segmentInfoOffset;
        }
        else
        {
            curNumSegments = maxIndirectCryptoStateNum;
        }
        DECODE_CP_NORMALMESSAGE("segmentInfosize = %d, segmentInfoOffset = %d\n", 
            m_pSegmentInfo[segmentInfoOffset + curNumSegments - 1].dwSegmentStartOffset + m_pSegmentInfo[segmentInfoOffset + curNumSegments - 1].dwSegmentLength,
            segmentInfoOffset);
        m_indStateIdx--;
    }
    else
    {
        DECODE_CP_NORMALMESSAGE("m_hucCopyLength = %d, segmentInfosize = %d\n", m_hucCopyLength, m_pSegmentInfo[curNumSegments - 1].dwSegmentStartOffset + m_pSegmentInfo[curNumSegments - 1].dwSegmentLength);
        DECODE_CP_CHK_COND_RETURN((m_hucCopyLength < (m_pSegmentInfo[curNumSegments - 1].dwSegmentStartOffset + m_pSegmentInfo[curNumSegments - 1].dwSegmentLength)),
            "Error - HuCCopyLength is less than the size AES engine expected!");
    }

    MOS_ZeroMemory(&IndAesParams, sizeof(IndAesParams));
    IndAesParams.presCryptoBuffer = presIndState;
    IndAesParams.dwOffset         = 0;
    IndAesParams.dwNumSegments    = curNumSegments;
    IndAesParams.pSegmentInfo     = m_pSegmentInfo + segmentInfoOffset;
    DECODE_CP_CHK_STATUS(m_mhwCp->SetHucIndirectCryptoState(&IndAesParams));

    MOS_ZeroMemory(&AESParams, sizeof(AESParams));
    AESParams.presDataBuffer = presIndState;
    AESParams.dwOffset       = 0;
    AESParams.dwSliceIndex   = curNumSegments;
    DECODE_CP_CHK_STATUS(m_mhwCp->AddHucAesIndState(cmdBuffer, &AESParams));

    // bitstream is not encrypted by streamKey anymore after executing upper commands
    ResetHostMode();

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS DecodeCp::AllocateStreamOutResource()  // allocate only for streamout
{
    PMHW_PAVP_ENCRYPTION_PARAMS pEncryptionParams = m_mhwCp->GetEncryptionParams();
    DECODE_CP_CHK_COND_RETURN((pEncryptionParams->dwSubSampleCount == 0),
        "Error - Segment count is zero!");
    DECODE_CP_CHK_COND_RETURN((pEncryptionParams->pSubSampleMappingBlock == nullptr),
        "Error - Null sample mapping block!");
    
    uint32_t *pSubSampleMappingBlock = (uint32_t *)pEncryptionParams->pSubSampleMappingBlock;
    uint32_t  dwSubSampleCount = pEncryptionParams->dwSubSampleCount;
    
    if (m_pSubsampleSegmentEnd)
    {
        MOS_FreeMemory(m_pSubsampleSegmentEnd);
        m_pSubsampleSegmentEnd = nullptr;
    }

    for (uint32_t i = 0; i < pEncryptionParams->dwSubSampleCount; i++)
    {
        uint32_t clearSize = *(pSubSampleMappingBlock + 2 * i);
        dwSubSampleCount   = dwSubSampleCount + ((clearSize < 0xffff) ? 0: (clearSize - 1) / 0xffff);
    }

    m_pSubsampleSegmentEnd =
        (uint32_t *)MOS_AllocAndZeroMemory(sizeof(uint32_t) * dwSubSampleCount);
    DECODE_CP_CHK_COND_RETURN((m_pSubsampleSegmentEnd == nullptr),
        "Failed to allocate subsample segment number buffer.");

    m_splitWorkload       = false;
    curSegmentNum         = 0;
    if (pEncryptionParams->dwBlocksStripeEncCount > 0)
    {
        uint32_t *pSubSampleMappingBlock = (uint32_t *)pEncryptionParams->pSubSampleMappingBlock;
        for (uint32_t i = 0, j = 0; i < dwSubSampleCount && j < dwSubSampleCount; i++, j++)
        {
            uint32_t encryptedSize = *(pSubSampleMappingBlock + 2 * i + 1);
            uint32_t clearSize     = *(pSubSampleMappingBlock + 2 * i);
            while (clearSize > 0xffff)
            {
                curSegmentNum += ComputeSubSegmentNumStripe(
                    0,
                    pEncryptionParams->dwBlocksStripeEncCount,
                    pEncryptionParams->dwBlocksStripeClearCount);
                // Save subsample ending location
                m_pSubsampleSegmentEnd[j++] = curSegmentNum;
                clearSize -= 0xffff;
            }
            curSegmentNum += ComputeSubSegmentNumStripe(
                encryptedSize,
                pEncryptionParams->dwBlocksStripeEncCount,
                pEncryptionParams->dwBlocksStripeClearCount);

            // Save subsample ending location
            m_pSubsampleSegmentEnd[j] = curSegmentNum;
        }

        if ((curSegmentNum > m_sampleRemappingBlockNum) ||
            m_pSampleRemappingBlock == nullptr)
        {
            if (m_pSampleRemappingBlock != nullptr)
            {
                MOS_FreeMemory(m_pSampleRemappingBlock);
            }

            m_pSampleRemappingBlock =
                (uint32_t *)MOS_AllocAndZeroMemory(sizeof(uint32_t) * 2 * curSegmentNum);
            DECODE_CP_CHK_COND_RETURN((m_pSampleRemappingBlock == nullptr),
                "Failed to allocate remapped sample block buffer.");
        }

        m_sampleRemappingBlockNum = curSegmentNum;
    }
    else
    {
        curSegmentNum = dwSubSampleCount;
    }

    // Segment Info buffer
    if ((curSegmentNum > m_segmentNum) ||
        m_pSegmentInfo == nullptr)
    {
        if (m_pSegmentInfo != nullptr)
        {
            MOS_FreeMemory(m_pSegmentInfo);
        }

        m_pSegmentInfo =
            (PMHW_SEGMENT_INFO)MOS_AllocAndZeroMemory(sizeof(MHW_SEGMENT_INFO) * curSegmentNum);
        DECODE_CP_CHK_COND_RETURN((m_pSegmentInfo == nullptr),
            "Failed to allocate segment info buffer.");

        m_segmentNum = curSegmentNum;
    }

    // hw limitation, check if need to split workload
    if (m_mhwCp->IsSplitHucStreamOutNeeded())
    {
        m_indStateNum = MOS_ROUNDUP_DIVIDE(curSegmentNum, maxIndirectCryptoStateNum);

        if (curSegmentNum > maxIndirectCryptoStateNum)
        {
            m_splitWorkload     = true;
            m_indStateIdx       = m_indStateNum - 1;
        }
    }

    return MOS_STATUS();
}  // allocate only for streamout

MOS_STATUS DecodeCp::UpdateSliceSegmentInfo()
{
    DECODE_CP_FUNCTION_ENTER;
    
    DECODE_CP_CHK_NULL_RETURN(m_pSubsampleSegmentNum);
    DECODE_CP_CHK_NULL_RETURN(m_pSubsampleSegmentEnd);
    DECODE_CP_CHK_NULL_RETURN(m_sliceSegmentNum);
    
    auto pSegmentInfo = m_pSegmentInfo;
    uint32_t segmentNumLeft = m_segmentNum;
    for (uint32_t i = 0; i < m_indStateNum; i++)
    {
        uint32_t sliceHeaderSize = m_sliceHeaderSize[i];
        uint32_t sliceDataSize   = m_sliceDataSize[i];
        uint32_t bsdDataStartOffset = m_bsdDataStartOffset[i];

        uint32_t sliceSize  = sliceHeaderSize + sliceDataSize;
        uint32_t segmentNum = 0;
        DECODE_CHK_NULL(pSegmentInfo);
        DECODE_CP_CHK_COND_RETURN((sliceSize < pSegmentInfo->dwSegmentLength), "Error - sliceSize is less than SegmentLength!");
        DECODE_CP_CHK_COND_RETURN((pSegmentInfo->dwSegmentLength == 0), "Error - dwSegmentLength is zero!");

        while (sliceSize >= pSegmentInfo->dwSegmentLength && pSegmentInfo->dwSegmentLength > 0)
        {
            sliceSize -= pSegmentInfo->dwSegmentLength;
            ++segmentNum;
            if (segmentNumLeft != segmentNum)
            {
                ++pSegmentInfo;
            }
        }

        if (sliceSize == 0)
        {
            m_sliceSegmentNum[i] = segmentNum;
            segmentNumLeft -= segmentNum;
        }
        else
        {
            DECODE_CP_ASSERTMESSAGE("Error occured while calculating slice segment info.");
            return MOS_STATUS_INVALID_PARAMETER;
        }
    }

    uint32_t curSegmentNum = 0;
    for (uint32_t i = 0; i < m_indStateNum; i++)
    {
        m_pSubsampleSegmentNum[i] = m_sliceSegmentNum[i];
        curSegmentNum            += m_sliceSegmentNum[i];
        m_pSubsampleSegmentEnd[i] = curSegmentNum;
    }

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS DecodeCp::UpdateSliceInfo(
    uint32_t *sliceDataSize,
    uint32_t *sliceHeaderSize,
    uint32_t *bsdDataStartOffset,
    uint32_t sliceNum)
{
    DECODE_CP_FUNCTION_ENTER;
    DECODE_CP_CHK_NULL_RETURN(sliceDataSize);
    DECODE_CP_CHK_NULL_RETURN(sliceHeaderSize);
    DECODE_CP_CHK_NULL_RETURN(bsdDataStartOffset);

    m_bsdDataStartOffset   = bsdDataStartOffset;
    m_sliceDataSize        = sliceDataSize;
    m_sliceHeaderSize      = sliceHeaderSize;

    // Set aes indirect state number to slice number
    DECODE_CP_CHK_STATUS(SetIndStateNum(sliceNum));

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS DecodeCp::AllocateDecodeResource()
{
    DECODE_CP_FUNCTION_ENTER;

    DECODE_CP_CHK_NULL_RETURN(m_mhwCp);
    PMHW_PAVP_ENCRYPTION_PARAMS pEncryptionParams = m_mhwCp->GetEncryptionParams();
    DECODE_CP_CHK_COND_RETURN((pEncryptionParams->dwSubSampleCount == 0),
        "Error - Segment count is zero!");
    DECODE_CP_CHK_COND_RETURN((pEncryptionParams->pSubSampleMappingBlock == nullptr),
        "Error - Null sample mapping block!");

    
    std::vector<std::pair<uint32_t, uint32_t>> map;
    ComputeSliceRemappingBlock(pEncryptionParams->dwSubSampleCount, 
        (uint32_t*)pEncryptionParams->pSubSampleMappingBlock, 
        map);
    m_sliceRemappingBlockNum = map.size();

    if (m_pSubsampleSegmentEnd)
    {
        MOS_FreeMemory(m_pSubsampleSegmentEnd);
        m_pSubsampleSegmentEnd = nullptr;
    }

    m_pSubsampleSegmentEnd =
        (uint32_t *)MOS_AllocAndZeroMemory(sizeof(uint32_t) * m_sliceRemappingBlockNum);
    DECODE_CP_CHK_COND_RETURN((m_pSubsampleSegmentEnd == nullptr),
        "Failed to allocate subsample segment ending index buffer.");

    if (m_pSubsampleSegmentNum)
    {
        MOS_FreeMemory(m_pSubsampleSegmentNum);
        m_pSubsampleSegmentNum = nullptr;
    }

    m_pSubsampleSegmentNum =
        (uint32_t *)MOS_AllocAndZeroMemory(sizeof(uint32_t) * m_sliceRemappingBlockNum);
    DECODE_CP_CHK_COND_RETURN((m_pSubsampleSegmentNum == nullptr),
        "Failed to allocate subsample segment number buffer.");
    if (m_pSampleSliceRemappingBlock)
    {
        MOS_FreeMemory(m_pSampleSliceRemappingBlock);
        m_pSampleSliceRemappingBlock = nullptr;
    }

    m_pSampleSliceRemappingBlock =
        (uint32_t *)MOS_AllocAndZeroMemory(sizeof(uint32_t) * m_sliceRemappingBlockNum * 2);
    DECODE_CP_CHK_COND_RETURN((m_pSampleSliceRemappingBlock == nullptr),
        "Failed to allocate subsample segment number buffer.");
    
    for (uint32_t i = 0; i < map.size(); i++)
    {
        *(m_pSampleSliceRemappingBlock + 2 * i) = map[i].first;
        *(m_pSampleSliceRemappingBlock + 2 * i + 1) = map[i].second;
    }

    curSegmentNum = 0;
    if (pEncryptionParams->dwBlocksStripeEncCount > 0)
    {
        uint32_t *pSubSampleMappingBlock = (uint32_t *)m_pSampleSliceRemappingBlock;
        DECODE_CP_CHK_NULL_RETURN(pSubSampleMappingBlock);
        for (uint32_t i = 0; i < m_sliceRemappingBlockNum; i++)
        {
            uint32_t encryptedSize = *(pSubSampleMappingBlock + 2 * i + 1);
            uint32_t subSegmentNum = ComputeSubSegmentNumStripe(
                encryptedSize,
                pEncryptionParams->dwBlocksStripeEncCount,
                pEncryptionParams->dwBlocksStripeClearCount);
            curSegmentNum += subSegmentNum;
            // Save subsample ending location and subsegment num
            m_pSubsampleSegmentEnd[i] = curSegmentNum;
            m_pSubsampleSegmentNum[i] = subSegmentNum;
        }

        if ((curSegmentNum > m_sampleRemappingBlockNum) ||
            m_pSampleRemappingBlock == nullptr)
        {
            if (m_pSampleRemappingBlock != nullptr)
            {
                MOS_FreeMemory(m_pSampleRemappingBlock);
            }

            m_pSampleRemappingBlock =
                (uint32_t *)MOS_AllocAndZeroMemory(sizeof(uint32_t) * 2 * curSegmentNum);
            DECODE_CP_CHK_COND_RETURN((m_pSampleRemappingBlock == nullptr),
                "Failed to allocate remapped sample block buffer.");
        }

        m_sampleRemappingBlockNum = curSegmentNum;
    }
    else
    {
        curSegmentNum = m_sliceRemappingBlockNum;
    }

    // Segment Info buffer
    if ((curSegmentNum > m_segmentNum) ||
        m_pSegmentInfo == nullptr)
    {
        if (m_pSegmentInfo != nullptr)
        {
            MOS_FreeMemory(m_pSegmentInfo);
        }

        m_pSegmentInfo =
            (PMHW_SEGMENT_INFO)MOS_AllocAndZeroMemory(sizeof(MHW_SEGMENT_INFO) * curSegmentNum);
        DECODE_CP_CHK_COND_RETURN((m_pSegmentInfo == nullptr),
            "Failed to allocate segment info buffer.");

        m_segmentNum = curSegmentNum;
    }

    if (m_sliceSegmentNum)
    {
        MOS_FreeMemory(m_sliceSegmentNum);
        m_sliceSegmentNum = nullptr;
    }
    m_sliceSegmentNum = (uint32_t *)MOS_AllocAndZeroMemory(sizeof(uint32_t) * m_indStateNum);
    DECODE_CP_CHK_COND_RETURN((m_sliceSegmentNum == nullptr), 
        "Failed to allocate segment number buffer.");

    return MOS_STATUS_SUCCESS;
}

uint32_t DecodeCp::GetIndStateSize()
{
    uint32_t indStateNum = 0;
    if (m_splitWorkload)
    {
        indStateNum = maxIndirectCryptoStateNum;
    }
    else
    {
        indStateNum = curSegmentNum;
    }
    return m_mhwCp->GetSizeOfCmdIndirectCryptoState() * indStateNum;
}

uint32_t DecodeCp::GetIndStateSize(uint32_t indirectCryptoStateIndex)
{
    DECODE_CP_CHK_COND((m_mhwCp == nullptr), 0, "m_mhwCp is nullptr.");
    DECODE_CP_CHK_COND((m_pSubsampleSegmentNum == nullptr), 0, "m_pSubsampleSegmentNum is nullptr.");
    DECODE_CP_CHK_COND((indirectCryptoStateIndex >= m_indStateNum), 0, "IndirectCryptoStateIndex should be less than IndStateNum.");
    
    return m_mhwCp->GetSizeOfCmdIndirectCryptoState() * m_pSubsampleSegmentNum[indirectCryptoStateIndex];
}

MOS_STATUS DecodeCp::GetSegmentInfoForIndState(uint32_t indirectCryptoStateIndex, uint32_t &curNumSegments, PMHW_SEGMENT_INFO &pSegmentInfo)
{
    DECODE_CP_CHK_NULL_RETURN(m_pSubsampleSegmentNum);
    DECODE_CP_CHK_NULL_RETURN(m_pSubsampleSegmentEnd);
    DECODE_CP_CHK_COND_RETURN((indirectCryptoStateIndex >= m_indStateNum), "IndirectCryptoStateIndex should be less than IndStateNum.");
    
    curNumSegments = m_pSubsampleSegmentNum[indirectCryptoStateIndex];
    pSegmentInfo   = &m_pSegmentInfo[m_pSubsampleSegmentEnd[indirectCryptoStateIndex] - curNumSegments];
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS DecodeCp::GetSegmentInfo(PMHW_SEGMENT_INFO &pSegmentInfo)
{
    DECODE_CP_CHK_NULL_RETURN(m_pSegmentInfo);
    pSegmentInfo = m_pSegmentInfo;
    return MOS_STATUS_SUCCESS;
}

uint32_t DecodeCp::GetIndStateNum()
{
    return m_indStateNum;
}

MOS_STATUS DecodeCp::SetIndStateNum(uint32_t indStateNum)
{
    m_indStateNum = indStateNum;
    return MOS_STATUS_SUCCESS;
}

uint32_t DecodeCp::FetchCopyLength(uint32_t index)
{
    uint32_t copyLength = 0;
    if (m_splitWorkload && index < m_indStateNum)
    {
        copyLength = m_hucCopySegmentLengths[index];
    }
    else
    {
        copyLength = m_hucCopyLength;
    }
    return copyLength;
}

bool DecodeCp::IsStreamOutNeeded()
{
    PMHW_PAVP_ENCRYPTION_PARAMS pEncryptionParams = nullptr;
    pEncryptionParams                             = m_mhwCp->GetEncryptionParams();
    return !((m_mhwCp->GetHostEncryptMode() == CP_TYPE_BASIC) || 
            (pEncryptionParams->dwSubSampleCount == 0) || 
            (m_decodeCpMode == DECODE_PIPELINE_EXTENDED));
}

bool DecodeCp::IsStreamOutEnabled()
{
    DECODE_CP_FUNCTION_ENTER;

    MosCpInterface*  osCpInterface = m_osInterface->osCpInterface;
    MEDIA_FEATURE_TABLE       *sku = m_osInterface->pfnGetSkuTable(m_osInterface);
    DECODE_CP_CHK_COND((osCpInterface == nullptr), false, "osCpInterface is nullptr.");
    DECODE_CP_CHK_COND((sku == nullptr), false, "sku is nullptr.");

#if (_DEBUG || _RELEASE_INTERNAL)
    MediaUserSetting::Value outValue;
    auto status = ReadUserSettingForDebug(m_userSettingPtr,
        outValue,
        "Id of PAVP Lite Mode Enable Huc StreamOut For Debug",
        MediaUserSetting::Group::Sequence);
    int32_t data = outValue.Get<int32_t>();

    if (status != MOS_STATUS_SUCCESS)
    {
        DECODE_CP_NORMALMESSAGE("MOS failed to read registry 'PAVP_LITE_MODE_ENABLE_HUC_STREAMOUT_FOR_DEBUG', status=%d", status);
    }
    else
    {
        // Check if PAVP_LITE_MODE_ENABLE_HUC_STREAMOUT_FOR_DEBUG registry is set for internal test.
        DECODE_CP_CHK_COND((data != 0), true, "lite mode huc streamout is enabled for internal test."); 
    }
#endif

    if(MEDIA_IS_SKU(sku, FtrPAVPLiteModeHucStreamoutDisabled) &&
        osCpInterface->IsCpEnabled() &&
        !(osCpInterface->IsHMEnabled() ||
        osCpInterface->IsSMEnabled() ))
    {
        // HuC StreamOut is disabled for PAVP Lite Mode, return false here.
        DECODE_CP_ASSERTMESSAGE("PAVP Lite Mode Huc streamout is disabled on this platform!");
        return false;
    }

    return true;
}

bool DecodeCp::IsAesIndSubPacketNeeded()
{
    DECODE_CP_CHK_COND((m_mhwCp == nullptr), false, "m_mhwCp is nullptr.");
    PMHW_PAVP_ENCRYPTION_PARAMS pEncryptionParams = m_mhwCp->GetEncryptionParams();
    return (m_mhwCp->GetHostEncryptMode() != CP_TYPE_NONE &&
            m_decodeCpMode == DECODE_PIPELINE_EXTENDED &&
            pEncryptionParams->dwSubSampleCount != 0);
}

// only for disable cp temporally when cp is enabled.
void DecodeCp::SetCpEnabled(bool isCpInUse)
{
    if (m_osInterface && m_osInterface->osCpInterface)
    {
        m_osInterface->osCpInterface->SetCpEnabled(isCpInUse);
    }
}

void DecodeCp::SetCopyLength(uint32_t size)
{
    m_hucCopyLength = size;
}

void DecodeCp::ResetHostMode()
{
    m_mhwCp->SetHostEncryptMode(CP_TYPE_NONE);
}

uint32_t DecodeCp::ComputeSubSegmentNumStripe(
    uint32_t encryptedSize,
    uint32_t stripeEncCount,
    uint32_t stripeClearCount)
{
    DECODE_CP_FUNCTION_ENTER;

    // Compute stripe pair number
    uint32_t subSegmentNum = encryptedSize /
                             ((stripeEncCount + stripeClearCount) * stripeBlockSize);

    // Comptue residual size to check if there is remaining enc block
    uint32_t residualSize = encryptedSize -
                            subSegmentNum * (stripeEncCount + stripeClearCount) * stripeBlockSize;
    if (residualSize > (stripeBlockSize * stripeEncCount))
    {
        // If residual size bigger than stripe enc size that means there are enc part with clear part.
        subSegmentNum += 2;
    }
    else if (residualSize > stripeBlockSize &&
             ((residualSize % stripeBlockSize) != 0))
    {
        // else if residual size bigger than 16 and don't align to 16, then there are enc part with clear part.
        subSegmentNum += 2;
    }
    else if (residualSize != 0 ||
             (residualSize == 0 && encryptedSize == 0) ||
             (residualSize == 0 && stripeClearCount != 0))
    {
        // Remainging four cases:
        //           1. residual size bigger than 16 and align to 16, then there will be enc part only.
        //           2. residual size equal to 16, then there will be enc part only.
        //           3. residual size smaller than 16, then there will be clear part only.
        //           4. residual size is zero, then there is no block for clear header if 1) encryptedsize not equal to 0 and 2) clear stripe equal to 0.
        subSegmentNum++;
    }

    return subSegmentNum;
}

uint32_t DecodeCp::ComputeSubStripeEncLen(
    uint32_t remappingSize,
    uint32_t stripeEncCount)
{
    if (remappingSize >= (stripeEncCount * stripeBlockSize))
    {
        return (stripeEncCount * stripeBlockSize);
    }
    else if (remappingSize >= stripeBlockSize)
    {
        return MOS_ALIGN_FLOOR(remappingSize, stripeBlockSize);
    }
    else
    {
        return 0;
    }
}

uint32_t DecodeCp::ComputeSubStripeClearLen(
    uint32_t remappingSize,
    uint32_t stripeClearCount)
{
    if (remappingSize >= (stripeClearCount * stripeBlockSize))
    {
        return (stripeClearCount * stripeBlockSize);
    }
    else
    {
        return remappingSize;
    }
}

MOS_STATUS DecodeCp::ComputeSliceRemappingBlock(
    uint32_t  subSampleCount,
    uint32_t *subSampleMappingBlock,
    std::vector<std::pair<uint32_t, uint32_t>> &map)
{
    uint32_t                        *bsdDataStartOffset       = m_bsdDataStartOffset;
    uint32_t                        *sliceDataSize            = m_sliceDataSize;
    uint32_t                        *sliceHeaderSize          = m_sliceHeaderSize;
    uint32_t                         startOffset              = 0; //Current processing bitstream startoffset
    uint32_t                         currCS                   = 0; //Current processed Clear and Secure map size
    uint32_t                         currHB                   = 0; //Current processed slice Header and Body size
    uint32_t                         i                        = 0; //subsample index
    uint32_t                         j                        = 0; //slice index
    
    uint32_t                         uiClearSize     = *(subSampleMappingBlock + 2 * i);
    uint32_t                         uiEncryptedSize = *(subSampleMappingBlock + 2 * i + 1);
    uint32_t                         H               = sliceHeaderSize[j]; //Current processing slice Header size
    uint32_t                         B               = sliceDataSize[j]; //Current processing slice Body size
    uint32_t                         bsSize          = 0;
    uint32_t                        sliceCount = m_indStateNum;

    map.resize(0);
    
    for (uint32_t i = 0; i < sliceCount; i++)
    {
        bsSize += m_sliceHeaderSize[i] + m_sliceDataSize[i];
    }

    // due to hw aes indirect command require startaddress equal to slice offset, 
    // use this function to split enc map by slice header offset, need handle some cases including:
    // 1. small slice is total clear to generate a clear segment; 
    // 2. encrypt start location equal to slice header to pass through; 
    // 3. encryption start location > slice headeroffset to generate a clear segment and a partial clear segment.

    do
    {
        if (currCS == currHB)
        {
            // Once slice length equal to segment length, start next process cycle
            startOffset += currHB;
            if (i < subSampleCount)
            {
                uiClearSize     = *(subSampleMappingBlock + 2 * i);
                uiEncryptedSize = *(subSampleMappingBlock + 2 * i + 1);
                i++;
            }
            else
            {
                //remaining clear bytes
                uiClearSize     = bsSize - startOffset;
                uiEncryptedSize = 0;
            }
            if (j < sliceCount)
            {
                H = sliceHeaderSize[j];
                B = sliceDataSize[j];
                j++;
            }
            
            currCS = uiClearSize + uiEncryptedSize;
            currHB = H + B;
        }
        else if (currCS < currHB)
        {
            // Once slice length > segment length, process next segment
            if (i < subSampleCount)
            {
                uiClearSize     = *(subSampleMappingBlock + 2 * i);
                uiEncryptedSize = *(subSampleMappingBlock + 2 * i + 1);

                currCS += uiClearSize + uiEncryptedSize;
                i++;
            }
        }
        else if (currCS > currHB)
        {
            // Once slice length < segment length, process next slice
            if (j < sliceCount)
            {
                H = sliceHeaderSize[j];
                B = sliceDataSize[j];

                currHB += H + B;
                j++;
            }
        }

        if (H + B < uiClearSize)
        {
            // Small slice should be total clear
            map.push_back(std::pair<uint32_t, uint32_t>(H + B, 0));
            uiClearSize -= H + B;
        }
        else if (uiClearSize > H &&
                 uiClearSize < H + B &&
                 currCS - (uiClearSize + uiEncryptedSize) < H)
        {
            //clear size > header size should be splited to 2 part.
            map.push_back(std::pair<uint32_t, uint32_t>(H, 0));
            map.push_back(std::pair<uint32_t, uint32_t>((uint32_t)uiClearSize - H, (uint32_t)uiEncryptedSize));
            uiClearSize -= H;
        }
        else /* if (uiClearSize == H) or other cases*/
        {
            //Other cases passthrough
            map.push_back(std::pair<uint32_t, uint32_t>((uint32_t)uiClearSize, (uint32_t)uiEncryptedSize));
        }

        if (i == subSampleCount && j == sliceCount)
        {
            // Subsample maps and slices all process done
            break;
        }
    } while (i <= subSampleCount && j <= sliceCount); // prevent deadlock

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS DecodeCp::ComputeRemappingBlockStripe(
    uint32_t  subSampleCount,
    uint32_t *subSampleMappingBlock,
    uint32_t  stripeEncCount,
    uint32_t  stripeClearCount)
{
    DECODE_CP_FUNCTION_ENTER;

    DECODE_CP_CHK_NULL_RETURN(subSampleMappingBlock);
    DECODE_CP_CHK_COND_RETURN((subSampleCount == 0), "Error - Segment count is zero!");
    DECODE_CP_CHK_COND_RETURN((stripeEncCount == 0), "Error - wrong stripe parameter!");

    uint32_t remappingIdx = 0;
    for (uint32_t i = 0; i < subSampleCount; i++)
    {
        uint32_t uiClearSize     = *(subSampleMappingBlock + 2 * i);
        uint32_t uiEncryptedSize = *(subSampleMappingBlock + 2 * i + 1);

        uint32_t *curRemappingBlock = m_pSampleRemappingBlock + 2 * remappingIdx;
        *curRemappingBlock          = uiClearSize;
        uint32_t subStripeEncLen    = ComputeSubStripeEncLen(uiEncryptedSize, stripeEncCount);
        *(curRemappingBlock + 1)    = subStripeEncLen;

        // Not enough even for first encryption block, then current block is clear at all.
        if (subStripeEncLen == 0)
        {
            *curRemappingBlock += uiEncryptedSize;
            uiEncryptedSize = 0;
        }

        remappingIdx++;

        uint32_t remappingSize = uiEncryptedSize - subStripeEncLen;
        while (remappingSize > 0)
        {
            DECODE_CP_CHK_COND_RETURN((remappingIdx >= curSegmentNum), "Error - Remapping block exceeds maximum number!\n");

            curRemappingBlock = m_pSampleRemappingBlock + 2 * remappingIdx;

            uiClearSize        = ComputeSubStripeClearLen(remappingSize, stripeClearCount);
            *curRemappingBlock = uiClearSize;
            remappingSize -= uiClearSize;
            subStripeEncLen          = ComputeSubStripeEncLen(remappingSize, stripeEncCount);
            *(curRemappingBlock + 1) = subStripeEncLen;
            remappingSize -= subStripeEncLen;

            // If current block and next block are clear only, then next block is the tail of current block
            if (subStripeEncLen == 0 &&
                remappingSize > 0 &&
                remappingSize <= (stripeClearCount * stripeBlockSize))
            {
                *curRemappingBlock += remappingSize;
                remappingSize = 0;
            }

            remappingIdx++;
        }
    }

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS DecodeCp::GetSegmentInfoLegacy(
    MhwCpInterface *cpInterface,
    PMOS_RESOURCE   dataBufferRes,
    uint32_t        subSampleCount,
    uint32_t *      subSampleMappingBlock)
{
    DECODE_CP_FUNCTION_ENTER;

    MhwCpBase *mhwCp = dynamic_cast<MhwCpBase *>(cpInterface);
    DECODE_CP_CHK_NULL_RETURN(mhwCp);
    DECODE_CP_CHK_NULL_RETURN(mhwCp->GetEncryptionParams());
    PMHW_PAVP_ENCRYPTION_PARAMS encryptionParams = mhwCp->GetEncryptionParams();

    DECODE_CP_CHK_COND_RETURN((subSampleCount == 0),
        "Error - Segment count is zero!");
    DECODE_CP_CHK_COND_RETURN((subSampleMappingBlock == nullptr),
        "Error - Null sample mapping block!");

    // CBC related
    uint8_t *       dataBuffer = nullptr;
    uint32_t        lastEncryptedOword[CODEC_DWORDS_PER_AES_BLOCK];
    CodechalResLock ResourceLock(m_osInterface, dataBufferRes);
    if (encryptionParams->PavpEncryptionType == CP_SECURITY_AES128_CBC)
    {
        dataBuffer = (uint8_t *)ResourceLock.Lock(CodechalResLock::readOnly);
        DECODE_CP_CHK_NULL_RETURN(dataBuffer);
        memset(lastEncryptedOword, 0, sizeof(lastEncryptedOword));
        MOS_SecureMemcpy(lastEncryptedOword, sizeof(lastEncryptedOword), encryptionParams->dwPavpAesCounter, sizeof(encryptionParams->dwPavpAesCounter));
    }

    m_hucCopySegmentLengths.clear();
    uint32_t headingBlock       = 0;

    uint32_t uiPreEncryptedSize = 0;
    uint8_t  ucPreAesResidual   = 0;
    uint32_t subsampleIndex     = 0;
    uint32_t clearSize          = 0;

    for (uint32_t i = 0, j = 0; i < subSampleCount; i++)
    {
        uint8_t ucCurAesResidualBegin = (ucPreAesResidual == 0) ? 0 : (16 - ucPreAesResidual);
        uint32_t uiClearSize           = 0;
        uint32_t uiEncryptedSize       = 0;
        uint32_t encryptSize           = *(subSampleMappingBlock + 2 * j + 1);

        if (i == 0)
        {
            clearSize = *(subSampleMappingBlock);
        }

        if (clearSize > 0xffff)
        {
            uiClearSize     = 0xffff;
            uiEncryptedSize = 0;
            clearSize       = clearSize - 0xffff;
        }
        else
        {
            uiClearSize     = clearSize;
            uiEncryptedSize = encryptSize;

            if (i < subSampleCount - 1)
            {
                j++;
                clearSize = *(subSampleMappingBlock + 2 * j);
            }
        }

        if (i == 0)
        {
            // Current segment is the first segment of this slice
            m_pSegmentInfo[i].dwSegmentStartOffset = 0;
            m_pSegmentInfo[i].dwInitByteLength     = uiClearSize;
        }
        else
        {
            // The remaing segment of current slice, this slice already has one or more segments
            m_pSegmentInfo[i].dwSegmentStartOffset = m_pSegmentInfo[i - 1].dwSegmentStartOffset + m_pSegmentInfo[i - 1].dwSegmentLength;
            m_pSegmentInfo[i].dwInitByteLength     = uiClearSize;
        }

        m_pSegmentInfo[i].dwSegmentLength              = uiClearSize + uiEncryptedSize;
        m_pSegmentInfo[i].dwPartialAesBlockSizeInBytes = ucCurAesResidualBegin;

        if (encryptionParams->PavpEncryptionType == CP_SECURITY_AES128_CBC)
        {
            if ((encryptionParams->dwBlocksStripeEncCount > 0) && (m_pSubsampleSegmentEnd[subsampleIndex] == i))
            {
                // Reset IV for next subsample
                MOS_SecureMemcpy(lastEncryptedOword, sizeof(lastEncryptedOword), encryptionParams->dwPavpAesCounter, sizeof(encryptionParams->dwPavpAesCounter));
                subsampleIndex++;
            }

            MOS_SecureMemcpy(m_pSegmentInfo[i].dwAesCbcIvOrCtr, sizeof(m_pSegmentInfo[i].dwAesCbcIvOrCtr), lastEncryptedOword, sizeof(lastEncryptedOword));

            if (m_pSegmentInfo[i].dwSegmentLength > m_pSegmentInfo[i].dwInitByteLength)
            {
                DECODE_CP_CHK_COND_RETURN(
                    ((m_pSegmentInfo[i].dwSegmentStartOffset + m_pSegmentInfo[i].dwSegmentLength) < sizeof(lastEncryptedOword)),
                    "Error - Wrong sample block parameters!");
                uint8_t *lastEncryptedEnd = dataBuffer + m_pSegmentInfo[i].dwSegmentStartOffset + m_pSegmentInfo[i].dwSegmentLength;
                MOS_SecureMemcpy(lastEncryptedOword, sizeof(lastEncryptedOword), lastEncryptedEnd - sizeof(lastEncryptedOword), sizeof(lastEncryptedOword));
            }
        }
        else
        {
            MOS_SecureMemcpy(m_pSegmentInfo[i].dwAesCbcIvOrCtr, sizeof(m_pSegmentInfo[i].dwAesCbcIvOrCtr), encryptionParams->dwPavpAesCounter, sizeof(encryptionParams->dwPavpAesCounter));

            mhwCp->IncrementCounter(
                m_pSegmentInfo[i].dwAesCbcIvOrCtr,
                uiPreEncryptedSize >> 4,
                encryptionParams->dwIVSize);
        }

        uiPreEncryptedSize += uiEncryptedSize;
        if (uiEncryptedSize >= ucCurAesResidualBegin)
        {
            ucPreAesResidual = (uiEncryptedSize - ucCurAesResidualBegin) & 0x0F;
        }
        else
        {
            ucPreAesResidual = ucPreAesResidual + uiEncryptedSize;
        }

        if (m_splitWorkload && i % maxIndirectCryptoStateNum == 0 && i != 0)
        {
            // Add heading block to make sure the boundary is page aligned, it will be overwritten with correct data
            // Save the actual data block size
            m_hucCopySegmentLengths.push_back(m_pSegmentInfo[i].dwSegmentStartOffset - headingBlock);
            headingBlock = m_pSegmentInfo[i].dwSegmentStartOffset & 0x0FFF;

            // Modify the pointer of source buffer for IV calculation in CBC mode
            dataBuffer += MOS_ALIGN_FLOOR(m_pSegmentInfo[i].dwSegmentStartOffset, MOS_PAGE_SIZE);

            m_pSegmentInfo[i].dwSegmentStartOffset = 0;
            m_pSegmentInfo[i].dwInitByteLength += headingBlock;
            m_pSegmentInfo[i].dwSegmentLength += headingBlock;
        }

        if (i == subSampleCount - 1)
        {
            m_hucCopySegmentLengths.push_back(m_pSegmentInfo[i].dwSegmentStartOffset + m_pSegmentInfo[i].dwSegmentLength - headingBlock);
        }
    }

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS DecodeCp::GetSegmentInfoStripe(
    MhwCpInterface *cpInterface,
    PMOS_RESOURCE   dataBufferRes,
    uint32_t        subSampleCount,
    uint32_t       *subSampleMappingBlock)
{
    DECODE_CP_FUNCTION_ENTER;

    MhwCpBase *mhwCp = dynamic_cast<MhwCpBase *>(cpInterface);
    DECODE_CP_CHK_NULL_RETURN(mhwCp);
    DECODE_CP_CHK_NULL_RETURN(mhwCp->GetEncryptionParams());
    PMHW_PAVP_ENCRYPTION_PARAMS pEncryptionParams = mhwCp->GetEncryptionParams();

    DECODE_CP_CHK_STATUS(ComputeRemappingBlockStripe(
        subSampleCount,
        subSampleMappingBlock,
        pEncryptionParams->dwBlocksStripeEncCount,
        pEncryptionParams->dwBlocksStripeClearCount));

    DECODE_CP_CHK_STATUS(GetSegmentInfoLegacy(
        cpInterface,
        dataBufferRes,
        curSegmentNum,
        m_pSampleRemappingBlock));

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS DecodeCp::CalculateSegmentInfo(
    PMOS_RESOURCE dataBufferRes)
{
    DECODE_CP_FUNCTION_ENTER;

    DECODE_CP_CHK_NULL_RETURN(m_cpInterface);
    MhwCpBase *mhwCp = dynamic_cast<MhwCpBase *>(m_cpInterface);
    DECODE_CP_CHK_NULL_RETURN(mhwCp);
    DECODE_CP_CHK_NULL_RETURN(dataBufferRes);

    PMHW_PAVP_ENCRYPTION_PARAMS pEncryptionParams = mhwCp->GetEncryptionParams();
    if (pEncryptionParams->dwBlocksStripeEncCount == 0)
    {
        DECODE_CP_CHK_STATUS(GetSegmentInfoLegacy(
            m_cpInterface,
            dataBufferRes,
            curSegmentNum,
            (uint32_t *)pEncryptionParams->pSubSampleMappingBlock));
    }
    else
    {
        DECODE_CP_CHK_STATUS(GetSegmentInfoStripe(
            m_cpInterface,
            dataBufferRes,
            pEncryptionParams->dwSubSampleCount,
            (uint32_t *)pEncryptionParams->pSubSampleMappingBlock));
    }

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS DecodeCp::CalculateSliceSegmentInfo(
    PMOS_RESOURCE dataBufferRes)
{
    DECODE_CP_FUNCTION_ENTER;

    DECODE_CP_CHK_NULL_RETURN(m_cpInterface);
    MhwCpBase *mhwCp = dynamic_cast<MhwCpBase *>(m_cpInterface);
    DECODE_CP_CHK_NULL_RETURN(mhwCp);
    DECODE_CP_CHK_NULL_RETURN(dataBufferRes);

    PMHW_PAVP_ENCRYPTION_PARAMS pEncryptionParams = mhwCp->GetEncryptionParams();

    if (pEncryptionParams->dwBlocksStripeEncCount == 0)
    {
        DECODE_CP_CHK_STATUS(GetSegmentInfoLegacy(
            m_cpInterface,
            dataBufferRes,
            curSegmentNum,
            m_pSampleSliceRemappingBlock));
    }
    else
    {
        DECODE_CP_CHK_STATUS(GetSegmentInfoStripe(
            m_cpInterface,
            dataBufferRes,
            m_sliceRemappingBlockNum,
            m_pSampleSliceRemappingBlock));
    }

    return MOS_STATUS_SUCCESS;
}

decode::DecodeSubPacket *DecodeCp::CreateDecodeCpIndSubPkt(
    decode::DecodePipeline *pipeline,
    CODECHAL_MODE mode,
    CodechalHwInterfaceNext *hwInterface)
{
    if (nullptr == pipeline || nullptr == hwInterface)
    {
        DECODE_CP_NORMALMESSAGE("NULL pointer parameters");
        return nullptr;
    }

    decode::CpIndPkt* pkt = decode::CpIndPktFactory::Create(mode, (decode::DecodePipeline *)pipeline, hwInterface);

    if(pkt == nullptr)
    {
        DECODE_CP_NORMALMESSAGE("Not supported codec mode.");
    }

    return pkt;
}

MOS_STATUS DecodeCp::ExecuteDecodeCpIndSubPkt(
    decode::DecodeSubPacket *packet,
    CODECHAL_MODE mode,
    MOS_COMMAND_BUFFER &cmdBuffer,
    uint32_t sliceIndex)
{
    DECODE_CP_CHK_NULL_RETURN(packet);

    auto aesIndPacket = dynamic_cast<decode::CpIndPkt*>(packet);
    DECODE_CP_CHK_NULL_RETURN(aesIndPacket);
    DECODE_CP_CHK_STATUS_MESSAGE(aesIndPacket->Execute(cmdBuffer, sliceIndex), "CP Packet execution failed!");
    return MOS_STATUS_SUCCESS;
}
