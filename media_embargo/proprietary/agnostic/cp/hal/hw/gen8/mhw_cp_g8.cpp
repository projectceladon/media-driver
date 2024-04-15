/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2014 - 2017
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
//! \file     mhw_cp_g8.cpp
//! \brief    MHW interface for content protection
//! \details  Impelements the Gen-8 functionalities for content protection
//!

#include "mhw_cp_g8.h"
#include "mhw_cp_hwcmd_common.h"
#include "mhw_mi_hwcmd_g8_X.h"
#include "mhw_vdbox.h"
#include "mhw_mi.h"

MhwCpG8::MhwCpG8(MOS_STATUS& eStatus, MOS_INTERFACE& osInterface) : MhwCpBase(osInterface)
{
    eStatus = MOS_STATUS_SUCCESS;
    do
    {
        m_cpCmdPropsMap = &g_decryptBitOffsets_g8;

        this->m_hwUnit = MOS_NewArray(CpHwUnit *,PAVP_HW_UNIT_MAX_COUNT);
        if (this->m_hwUnit == nullptr)
        {
            MOS_ASSERTMESSAGE(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW, "PAVP G8 HW unit pointer array allocation failed.");
            eStatus = MOS_STATUS_NO_SPACE;
            MT_ERR3(MT_CP_MHW_ALLOCATION_FAIL, MT_COMPONENT, MOS_COMPONENT_CP, MT_SUB_COMPONENT, MOS_CP_SUBCOMP_MHW,
                                                                                            MT_ERROR_CODE, eStatus);
            break;
        }

        this->m_hwUnit[PAVP_HW_UNIT_RCS] = MOS_New(CpHwUnitG8Rcs, eStatus, osInterface);
        if (eStatus != MOS_STATUS_SUCCESS)
        {
            break;
        }

        this->m_hwUnit[PAVP_HW_UNIT_VCS] = MOS_New(CpHwUnitG8Vcs, eStatus, osInterface);
        if (eStatus != MOS_STATUS_SUCCESS)
        {
            break;
        }

        this->m_hwUnit[PAVP_HW_UNIT_VECS] = MOS_New(CpHwUnitG8Vecs, eStatus, osInterface);
        if (eStatus != MOS_STATUS_SUCCESS)
        {
            break;
        }

        if ((this->m_hwUnit[PAVP_HW_UNIT_RCS] == nullptr) ||
            (this->m_hwUnit[PAVP_HW_UNIT_VCS] == nullptr) ||
            (this->m_hwUnit[PAVP_HW_UNIT_VECS] == nullptr))
        {
            MOS_ASSERTMESSAGE(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW, "PAVP G8 HW unit allocation failed.");
            eStatus = MOS_STATUS_NO_SPACE;
            MT_ERR3(MT_CP_MHW_ALLOCATION_FAIL, MT_COMPONENT, MOS_COMPONENT_CP, MT_SUB_COMPONENT, MOS_CP_SUBCOMP_MHW,
                                                                                            MT_ERROR_CODE, eStatus);
            break;
        }
    } while (false);
}

MhwCpG8::~MhwCpG8()
{
}


bool MhwCpG8::IsHwCounterIncrement(
    PMOS_INTERFACE osInterface)
{
    return false;
}

MOS_STATUS MhwCpG8::AddMfxAesState(
    bool                 isDecodeInUse,
    PMOS_COMMAND_BUFFER  cmdBuffer,
    PMHW_BATCH_BUFFER    batchBuffer,
    PMHW_PAVP_AES_PARAMS params)
{
    return AddMfxAesStateG8(this, &this->m_osInterface, isDecodeInUse, cmdBuffer, batchBuffer, params);
}

MOS_STATUS MhwCpG8::AddMfxCryptoKeyExchange(
    PMOS_COMMAND_BUFFER                          cmdBuffer,
    PMHW_PAVP_MFX_CRYPTO_KEY_EXCHANGE_CMD_PARAMS params)
{
    return MhwCpBase::AddMfxCryptoKeyExchange(cmdBuffer, params);
}

MOS_STATUS MhwCpG8::AddMfxCryptoInlineStatusRead(
    PMOS_INTERFACE                                     osInterface,
    PMOS_COMMAND_BUFFER                                cmdBuffer,
    PMHW_PAVP_MFX_CRYPTO_INLINE_STATUS_READ_CMD_PARAMS params)
{
    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(cmdBuffer);
    MHW_CP_CHK_NULL(osInterface);

    auto hwUnit = MhwCpBase::getHwUnit(osInterface);
    MHW_CP_CHK_NULL(hwUnit);
    MHW_CP_CHK_STATUS(hwUnit->AddMfxCryptoInlineStatusRead(osInterface, cmdBuffer, params));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpG8::AddMfxCryptoCopyBaseAddr(
    PMOS_COMMAND_BUFFER               cmdBuffer,
    PMHW_PAVP_CRYPTO_COPY_ADDR_PARAMS params)
{
    return AddMfxCryptoCopyBaseAddrG8(this, &this->m_osInterface, cmdBuffer, params);
}

MOS_STATUS MhwCpG8::AddMfxCryptoCopy(
    PMOS_COMMAND_BUFFER          cmdBuffer,
    PMHW_PAVP_CRYPTO_COPY_PARAMS params)
{
    return AddMfxCryptoCopyG8(this, cmdBuffer, params);
}

MOS_STATUS MhwCpG8::SetProtectionSettingsForMiFlushDw(
    PMOS_INTERFACE osInterface,
    void           *cmd)
{
    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(cmd);

    MosCp *pMosCp = dynamic_cast<MosCp*>(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(pMosCp);

    static_cast<MI_FLUSH_DW_PAVP*>(cmd)->ProtectedMemoryEnable = 
        osInterface->osCpInterface->IsHMEnabled();

    static_cast<MI_FLUSH_DW_PAVP*>(cmd)->ContentProtectionAppId =
        PAVP_SESSION_TYPE_FROM_ID(pMosCp->GetSessionId());  // DW0

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

void MhwCpG8::GetCpStateLevelCmdSize(uint32_t& cmdSize, uint32_t& patchListSize)
{
    MHW_CP_FUNCTION_ENTER;

    uint32_t prologCmdSize = mhw_mi_g8_X::MI_FLUSH_DW_CMD::byteSize + sizeof(MI_SET_APP_ID_CMD);
    uint32_t epilogCmdSize = mhw_mi_g8_X::MI_FLUSH_DW_CMD::byteSize;

    cmdSize =
        prologCmdSize +
        MOS_MAX(
        (
            sizeof(HCP_AES_STATE_CMD) +
            mhw_mi_g8_X::MFX_WAIT_CMD::byteSize +
            sizeof(MFX_CRYPTO_COPY_BASE_ADDR_CMD_G8) +
            sizeof(MFX_CRYPTO_COPY_CMD_G8)),
            (sizeof(HUC_AES_IND_STATE_CMD))) +
        epilogCmdSize;

    patchListSize =
        PATCH_LIST_COMMAND(MI_FLUSH_DW_CMD) +
        PATCH_LIST_COMMAND(MI_SET_APP_ID_CMD) +
        PATCH_LIST_COMMAND(HCP_AES_STATE_CMD) +
        PATCH_LIST_COMMAND(MFX_WAIT_CMD) +
        PATCH_LIST_COMMAND(MFX_CRYPTO_COPY_BASE_ADDR_CMD) +
        PATCH_LIST_COMMAND(MFX_CRYPTO_COPY_CMD) +
        PATCH_LIST_COMMAND(HUC_AES_IND_STATE_CMD) +
        PATCH_LIST_COMMAND(MI_FLUSH_DW_CMD);

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
}

void MhwCpG8::GetCpSliceLevelCmdSize(uint32_t& cmdSize, uint32_t& patchListSize)
{
    MHW_CP_FUNCTION_ENTER;

    uint32_t cmdSizeForMfx = sizeof(MFX_AES_STATE_CMD_G8) + mhw_mi_g8_X::MFX_WAIT_CMD::byteSize;
    uint32_t cmdSizeForHcp = sizeof(HCP_AES_STATE_CMD) + mhw_mi_g8_X::MFX_WAIT_CMD::byteSize + sizeof(HCP_AES_IND_STATE_CMD);
    uint32_t cmdSizeForHuc = sizeof(HUC_AES_STATE_CMD) + sizeof(HUC_AES_IND_STATE_CMD);

    uint32_t patchLSizeForMfx = PATCH_LIST_COMMAND(MFX_AES_STATE_CMD) + PATCH_LIST_COMMAND(MFX_WAIT_CMD);
    uint32_t patchLSizeForHcp = PATCH_LIST_COMMAND(HCP_AES_STATE_CMD) + PATCH_LIST_COMMAND(HCP_AES_IND_STATE_CMD);
    uint32_t patchLSizeForHuc = PATCH_LIST_COMMAND(HUC_AES_STATE_CMD) + PATCH_LIST_COMMAND(HUC_AES_IND_STATE_CMD);

    cmdSize = MOS_MAX((cmdSizeForMfx), MOS_MAX((cmdSizeForHcp), (cmdSizeForHuc)));
    patchListSize = MOS_MAX((patchLSizeForMfx), MOS_MAX((patchLSizeForHcp), (patchLSizeForHuc)));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
}

// ----- static functions shared by several Gens. ----------
MOS_STATUS MhwCpG8::AddMfxAesStateG8(
    MhwCpBase*               mhwCp,
    PMOS_INTERFACE       osInterface,
    bool                 isDecodeInUse,
    PMOS_COMMAND_BUFFER  cmdBuffer,
    PMHW_BATCH_BUFFER    batchBuffer,
    PMHW_PAVP_AES_PARAMS params)
{
    uint32_t        counterIncrement;
    uint32_t        numClearBytes         = 0;
    uint32_t        zeroPadding           = 0;
    uint32_t        aesCtr[4]             = {0};
    uint32_t        *subSampleMappingBlock = nullptr;
    uint32_t        *firstEncryptedOword, *lastEncryptedOword;
    MOS_LOCK_PARAMS lockFlags;

    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(mhwCp);
    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(mhwCp->m_mhwMiInterface);
    MHW_CP_CHK_NULL(params);

    auto miInterface      = mhwCp->m_mhwMiInterface;
    auto encryptionParams = mhwCp->GetEncryptionParams();
    auto cmd              = g_cInit_MFX_AES_STATE_CMD_G8;

    MHW_CP_CHK_STATUS_MESSAGE(
        MOS_SecureMemcpy(
            aesCtr,
            CODEC_DWORDS_PER_AES_BLOCK * sizeof(uint32_t),
            encryptionParams->dwPavpAesCounter,
            CODEC_DWORDS_PER_AES_BLOCK * sizeof(uint32_t)),
        "Failed to copy memory.");

    if (encryptionParams->PavpEncryptionType == CP_SECURITY_DECE_AES128_CTR)
    {
        // DECE is relevant for AVC only, the start code is always '0 0 1' .

        if (encryptionParams->pSubSampleMappingBlock != nullptr)
        {
            // DECE format for new MS interface

            // Increment the counter one sub-sample less
            counterIncrement      = 0;
            subSampleMappingBlock = (uint32_t*)encryptionParams->pSubSampleMappingBlock;
            counterIncrement += *(subSampleMappingBlock + 2 * (params->dwSliceIndex) + 1);

            // Each slice index corresponds to each sub-sample block
            // InitPacketBytesLength is the amount of bytes from the beginning of the bit stream until the first encrypted byte.
            cmd.DW7.InitPacketBytesLength = *(subSampleMappingBlock + 2 * (params->dwSliceIndex));

            // Number of encrypted bytes. The next value after clear bytes.
            cmd.DW8.EncryptedBytesLength = *(subSampleMappingBlock + 2 * (params->dwSliceIndex) + 1);
        }
        else
        {
            // Data offset is coming from the application.
            // For each slice we need to calculate the offset based on the number of Bytes in previous slices.
            zeroPadding = params->dwDataStartOffset - params->dwTotalBytesConsumed;

            // Clear bytes (which are the slice header and part of the bit stream) contains also the start code (0 0 1).
            // According to the DECE spec: Clear Bytes + zero padding = 128 exactly.
            numClearBytes = params->dwDefaultDeceInitPacketLengthInBytes - zeroPadding;

            if (params->dwDataLength <= params->dwDefaultDeceInitPacketLengthInBytes)
            {
                //Reset Counter to ZERO
                aesCtr[0] = aesCtr[1] = aesCtr[2] = aesCtr[3] = 0;

                //No increment as well
                counterIncrement              = 0;
                cmd.DW7.InitPacketBytesLength = params->dwDefaultDeceInitPacketLengthInBytes;
                cmd.DW8.EncryptedBytesLength  = 0;
            }
            else
            {
                //CounterIncremnet = Total 16 Byte Blocks consumed before this slice - Totals 16 bytes block of clear data before this slice
                //                 =  (TolalBytesConsumedBeforeThisSlice / 16) - ( 8 * SlicesProcessedSofar); 128 = 16 * 8
                counterIncrement = (params->dwTotalBytesConsumed >> 4) - (params->dwSliceIndex << 3);

                // InitPacketBytesLength is the amount of bytes from the beginning of the bit stream until the first encrypted byte NOT incuding the start code.
                cmd.DW7.InitPacketBytesLength = numClearBytes - MOS_NAL_UNIT_STARTCODE_LENGTH;

                // Number of encrypted bytes.
                cmd.DW8.EncryptedBytesLength = params->dwDataLength - numClearBytes;
            }
        }
    }
    else if (encryptionParams->PavpEncryptionType == CP_SECURITY_AES128_CBC)
    {
        MHW_CP_CHK_NULL(params->presDataBuffer);

        lastEncryptedOword = nullptr;

        MOS_ZeroMemory(&lockFlags, sizeof(MOS_LOCK_PARAMS));
        lockFlags.ReadOnly  = 1;
        firstEncryptedOword = static_cast<uint32_t*>(osInterface->pfnLockResource(
            osInterface,
            params->presDataBuffer,
            &lockFlags));
        MHW_CP_CHK_NULL(firstEncryptedOword);

        lastEncryptedOword =
            firstEncryptedOword +
            ((params->dwDataStartOffset >> 4) * sizeof(uint32_t));

        if (lastEncryptedOword != firstEncryptedOword)
        {
            lastEncryptedOword -= sizeof(uint32_t);
            aesCtr[0] = lastEncryptedOword[0];
            aesCtr[1] = lastEncryptedOword[1];
            aesCtr[2] = lastEncryptedOword[2];
            aesCtr[3] = lastEncryptedOword[3];
        }

        osInterface->pfnUnlockResource(osInterface, params->presDataBuffer);

        counterIncrement = 0;
    }
    else
    {
        // Standard AES CTR mode
        counterIncrement = params->dwDataStartOffset >> 4;
    }

    // Decode increments the counter before adding the AES state to the command buffer
    if (isDecodeInUse)
    {
        mhwCp->IncrementCounter(
            aesCtr,
            counterIncrement);
    }

    cmd.DW4.InitializationValue3 = aesCtr[3];
    cmd.DW3.InitializationValue2 = aesCtr[2];
    cmd.DW2.InitializationValue1 = aesCtr[1];
    cmd.DW1.InitializationValue0 = aesCtr[0];

    // Encode increments the counter after adding the AES state to the command buffer
    // and the counter is incremented for last pass only
    if (!isDecodeInUse && params->bLastPass)
    {
        mhwCp->IncrementCounter(
            encryptionParams->dwPavpAesCounter,
            encryptionParams->uiCounterIncrement);
    }

    if (cmdBuffer == nullptr && batchBuffer == nullptr)
    {
        MHW_ASSERTMESSAGE("There was no valid buffer to add the HW command to.");
        MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, MOS_STATUS_NO_SPACE);
        return MOS_STATUS_NO_SPACE;
    }

    MHW_CP_CHK_STATUS(Mhw_AddCommandCmdOrBB(osInterface, cmdBuffer, batchBuffer, &cmd, sizeof(cmd)));

    // This wait cmd is needed to make sure pavp command is done before decode starts
    MHW_CP_CHK_STATUS(miInterface->AddMfxWaitCmd(cmdBuffer, batchBuffer, false));

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpG8::AddMfxCryptoCopyBaseAddrG8(
    MhwCpBase*                   mhwCp,
    PMOS_INTERFACE                    osInterface,
    PMOS_COMMAND_BUFFER               cmdBuffer,
    PMHW_PAVP_CRYPTO_COPY_ADDR_PARAMS params)
{
    CpHwUnit*                        hwUnit = nullptr;
    MHW_RESOURCE_PARAMS              resourceParams;
    MFX_CRYPTO_COPY_BASE_ADDR_CMD_G8 cmd;

    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(mhwCp);
    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(params);

    cmd = g_cInit_MFX_CRYPTO_COPY_BASE_ADDR_CMD_G8;

    MOS_ZeroMemory(&resourceParams, sizeof(resourceParams));
    resourceParams.dwLsbNum      = MHW_VDBOX_MFX_UPPER_BOUND_STATE_SHIFT;
    resourceParams.HwCommandType = MOS_HW_COMMAND_NULL;

    resourceParams.presResource    = params->presSrc;
    resourceParams.dwOffset        = 0;
    resourceParams.pdwCmd          = (uint32_t*)&(cmd.DW1.Value);
    resourceParams.dwLocationInCmd = 1;
    resourceParams.dwSize          = params->dwSize;
    resourceParams.bIsWritable     = false;

    // upper bound of the allocated resource will be set at 1 DW apart from address location
    resourceParams.dwUpperBoundLocationOffsetFromCmd = 2;

    hwUnit = mhwCp->getHwUnit(osInterface);
    MHW_CP_CHK_NULL(hwUnit);
    MHW_CP_CHK_STATUS(hwUnit->pfnAddResourceToCmd(
        osInterface,
        cmdBuffer,
        &resourceParams));

    if (params->presDst != nullptr)
    {
        // Address location in the token is different than the location in the command
        resourceParams.presResource    = params->presDst;
        resourceParams.dwOffset        = (uint32_t)params->offset;
        resourceParams.pdwCmd          = (uint32_t*)&(cmd.DW5.Value);
        resourceParams.dwLocationInCmd = 5;
        resourceParams.dwSize          = 0;
        resourceParams.bIsWritable     = true;

        resourceParams.HwCommandType = params->bIsDestRenderTarget ? MOS_MFX_CC_BASE_ADDR_STATE : MOS_HW_COMMAND_NULL;

        // upper bound of the allocated resource will not be set
        resourceParams.dwUpperBoundLocationOffsetFromCmd = 0;

        hwUnit = mhwCp->getHwUnit(osInterface);
        MHW_CP_CHK_NULL(hwUnit);
        MHW_CP_CHK_STATUS(hwUnit->pfnAddResourceToCmd(
            osInterface,
            cmdBuffer,
            &resourceParams));
    }

    MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, &cmd, sizeof(cmd)));

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpG8::AddMfxCryptoCopyG8(
    MhwCpBase*                   mhwCp,
    PMOS_COMMAND_BUFFER          cmdBuffer,
    PMHW_PAVP_CRYPTO_COPY_PARAMS params)
{
    MFX_CRYPTO_COPY_CMD_G8 cmd               = g_cInit_MFX_CRYPTO_COPY_CMD_G8;
    uint32_t               maxCmdSize        = MHW_VDBOX_MAX_CRYPTO_COPY_SIZE;
    uint32_t               maxEncryptedBytes = maxCmdSize;  // In case of partial decryption, we don't want to include the clear bytes when incrementing the counter.
    uint32_t               dataOffset        = 0;

    MHW_CP_CHK_NULL(params);
    MHW_CP_CHK_NULL(mhwCp);

    // Populate the Partial Decryption data.
    if (params->ui8SelectionForIndData == CRYPTO_COPY_TYPE_AES_DECRYPT_PARTIAL)
    {
        if ((params->uiEncryptedBytes == 0) && (params->uiClearBytes == 0))
        {
            // this will cause a division by zero
            return MOS_STATUS_INVALID_PARAMETER;
        }

        if (params->AesMode == MHW_PAVP_CRYPTO_COPY_AES_MODE::CRYPTO_COPY_AES_MODE_CBC_SKIP)
        {
            MHW_VERBOSEMESSAGE("Setting crypto copy AES type to AES-CBC with skip encryption");
            cmd.DW10.AesOperationType = MHW_PAVP_AES_TYPE_CBC_SKIP;
        }
        else if (params->AesMode == MHW_PAVP_CRYPTO_COPY_AES_MODE::CRYPTO_COPY_AES_MODE_CTR)
        {
            MHW_VERBOSEMESSAGE("Setting crypto copy AES type to AES-CTR");
            cmd.DW10.AesOperationType = MHW_PAVP_AES_TYPE_CTR;
        }
        else
        {
            MHW_ASSERTMESSAGE("Invalid crypto copy AES type %d, must be CTR or CBC", params->AesMode);
            MT_ERR2(MT_CP_INVALID_ENCRYPT_TYPE, MT_CP_ENCRYPT_TYPE, params->AesMode, MT_CODE_LINE, __LINE__);
            return MOS_STATUS_INVALID_PARAMETER;
        }

        // Align uiMaxCmdSize so not to cut the frame in the middle of a line.
        if ((params->uiEncryptedBytes + params->uiClearBytes) <= MHW_VDBOX_MAX_CRYPTO_COPY_SIZE)
        {
            maxCmdSize = MHW_VDBOX_MAX_CRYPTO_COPY_SIZE - (MHW_VDBOX_MAX_CRYPTO_COPY_SIZE % (params->uiEncryptedBytes + params->uiClearBytes));

            // Calculate the maximal number of lines sent to HW in each command.
            uint32_t uiMaxLines = maxCmdSize / (params->uiEncryptedBytes + params->uiClearBytes);
            // Multiply the maximal number of lines by the number of encrypted bytes in each line, to find the maximal number of encrypted bytes sent to HW in each command.
            // Note: we should first divide and then multiply, since if we multiply first the result may exceed max uint32_t.
            maxEncryptedBytes = uiMaxLines * params->uiEncryptedBytes;
        }
        else
        {
            if (params->uiClearBytes != 0)
            {
                // maxCmdSize will cut the frame in the middle of a line if stripe size exceed MHW_VDBOX_MAX_CRYPTO_COPY_SIZE
                return MOS_STATUS_INVALID_PARAMETER;
            }

            // stripe contains encrypted bytes only, therefore the frame size is set to MHW_VDBOX_MAX_CRYPTO_COPY_SIZE
            maxCmdSize = MHW_VDBOX_MAX_CRYPTO_COPY_SIZE;

            // The maximal number of encrypted bytes sent to HW in each command is maxCmdSize in this case
            maxEncryptedBytes = maxCmdSize;
        }

        cmd.DW10.StartPacketType     = 0;
        cmd.DW10.InitialPacketLength = 0;

        cmd.DW11.EncryptedBytesLength = params->uiEncryptedBytes;
        cmd.DW12.ClearBytesLength     = params->uiClearBytes;
    }

    while (dataOffset < params->dwCopySize)
    {
        // Populate the fields as requested by the caller.
        cmd.DW1.AesCounterFlavor         = MHW_PAVP_COUNTER_TYPE_C;
        cmd.DW1.SelectionForIndirectData = params->ui8SelectionForIndData;
        if (dataOffset > 0)
        {
            // If not the very first slice of a frame, use the Initial Counter value or IV value
            // in the crypto function internal HW register
            cmd.DW1.UseIntialCounterValue = 1;
        }
        cmd.DW1.IndirectDataLength       = MOS_MIN(maxCmdSize, (params->dwCopySize - dataOffset));
        cmd.DW2.SrcBaseOffset            = params->dwSrcBaseOffset + dataOffset;
        cmd.DW4.DstBaseOffset            = params->dwDestBaseOffset + dataOffset;

        // Increment data offset in case of another pass.
        dataOffset += maxCmdSize;

        cmd.DW6.Counter                = params->dwAesCounter[0];
        cmd.DW7.InitializationValueDw1 = params->dwAesCounter[1];
        cmd.DW8.InitializationValueDw2 = params->dwAesCounter[2];
        cmd.DW9.InitializationValueDw3 = params->dwAesCounter[3];

        // CTR increment or IV setting is only required for encrypted input/output
        if (params->ui8SelectionForIndData != CRYPTO_COPY_TYPE_BYPASS)
        {
            MHW_ASSERT((maxEncryptedBytes % MHW_PAVP_AES128_NUM_BYTES_IN_BLOCK) == 0);
            // Increment counter in case of another pass.
            mhwCp->IncrementCounter(
                params->dwAesCounter,
                maxEncryptedBytes / MHW_PAVP_AES128_NUM_BYTES_IN_BLOCK);
        }
        MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, &cmd, sizeof(cmd)));
    }

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpG8::ReadEncodeCounterFromHW(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer,
    PMOS_RESOURCE       resource,
    uint16_t            currentIndex)
{
    MOS_UNUSED(osInterface);
    MOS_UNUSED(cmdBuffer);
    MOS_UNUSED(resource);
    MOS_UNUSED(currentIndex);

    return MOS_STATUS_PLATFORM_NOT_SUPPORTED;
}

MOS_STATUS MhwCpG8::GetTranscryptKernelInfo(
        const uint32_t                  **transcryptMetaData,
        uint32_t                        *transcryptMetaDataSize,
        const uint32_t                  **encryptedKernels,
        uint32_t                        *encryptedKernelsSize,
        MHW_PAVP_KERNEL_ENCRYPTION_TYPE encryptionType)
{
    MHW_CP_CHK_NULL(transcryptMetaData);
    MHW_CP_CHK_NULL(transcryptMetaDataSize);
    MHW_CP_CHK_NULL(encryptedKernels);
    MHW_CP_CHK_NULL(encryptedKernelsSize);

    *transcryptMetaData     = nullptr;
    *transcryptMetaDataSize = 0;
    *encryptedKernels       = nullptr;
    *encryptedKernelsSize   = 0;

    return MOS_STATUS_PLATFORM_NOT_SUPPORTED;
}

MOS_STATUS MhwCpG8::AddProlog(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(cmdBuffer);

    MosCp *pMosCp = dynamic_cast<MosCp *>(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(pMosCp);

    MHW_PAVP_PROLOG_TRACE_DATA eventData =
        {
            osInterface->pfnGetGpuContext(osInterface),
            osInterface->osCpInterface->IsCpEnabled(),
            m_encryptionParams.CpTag,
        };
    MOS_TraceEventExt(EVENT_MHW_PROLOG, EVENT_TYPE_INFO, &cmdBuffer->pCmdBase, sizeof(uint32_t), &eventData, sizeof(eventData));

    if (osInterface->osCpInterface->IsCpEnabled() == false)
    {
        // the operation is not needed
        return MOS_STATUS_SUCCESS;
    }

    auto hwUnit = MhwCpBase::getHwUnit(osInterface);
    MHW_CP_CHK_NULL(hwUnit);

    MHW_CP_CHK_STATUS(hwUnit->SaveEncryptionParams(&m_encryptionParams));
    MHW_CP_CHK_STATUS(hwUnit->AddProlog(osInterface, cmdBuffer));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

CpHwUnitG8Rcs::CpHwUnitG8Rcs(MOS_STATUS& eStatus, MOS_INTERFACE& osInterface) : CpHwUnitRcs(eStatus, osInterface)
{
}

MOS_STATUS CpHwUnitG8Rcs::AddProlog(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    // Render
    PIPE_CONTROL_PAVP*      pipeControl = nullptr;
    MHW_PIPE_CONTROL_PARAMS pipeControlParams;

    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(cmdBuffer);
    MHW_CP_CHK_NULL(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(this->m_mhwMiInterface);

    MosCp *pMosCp = dynamic_cast<MosCp*>(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(pMosCp);

    MOS_ZeroMemory(&pipeControlParams, sizeof(pipeControlParams));
    pipeControlParams.dwFlushMode = MHW_FLUSH_WRITE_CACHE;

    // get the address of the PIPE_CONTROL going to be added
    pipeControl = (PIPE_CONTROL_PAVP*)(cmdBuffer->pCmdPtr + 1);  // DW1

    // actually add the PIPE_CONTROL
    MHW_CP_CHK_STATUS(this->m_mhwMiInterface->AddPipeControl(cmdBuffer, nullptr, &pipeControlParams));

    // patch the ProtectedMemoryEnable bit
    pipeControl->ProtectedMemoryEnable = osInterface->osCpInterface->IsHMEnabled();

    // Send MI_SET_APPID
    MHW_CP_CHK_STATUS(AddMiSetAppId(osInterface, cmdBuffer, pMosCp->GetSessionId()));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

CpHwUnitG8Vecs::CpHwUnitG8Vecs(MOS_STATUS& eStatus, MOS_INTERFACE& osInterface) : CpHwUnitVecs(eStatus, osInterface)
{
}

CpHwUnitG8Vcs::CpHwUnitG8Vcs(MOS_STATUS& eStatus, MOS_INTERFACE& osInterface) : CpHwUnitVcs(eStatus, osInterface)
{
}

//------------------ PAVP Inline Read Command Type----------------
MOS_STATUS CpHwUnitG8Vcs::AddMfxCryptoInlineStatusRead(
    PMOS_INTERFACE                                     osInterface,
    PMOS_COMMAND_BUFFER                                cmdBuffer,
    PMHW_PAVP_MFX_CRYPTO_INLINE_STATUS_READ_CMD_PARAMS params)
{
    MHW_RESOURCE_PARAMS resourceParams;

    MHW_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(params);
    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(params->presReadData);
    MHW_CP_CHK_NULL(params->presWriteData);
    MHW_CP_CHK_NULL(osInterface->pfnGetGpuContext);

    auto cmd           = g_cInit_MFX_CRYPTO_INLINE_STATUS_READ_CMD_G8;

    // this is where the status read data requested will be stored
    {
        MOS_ZeroMemory(&resourceParams, sizeof(resourceParams));
        resourceParams.presResource    = params->presReadData;
        resourceParams.dwOffset        = params->dwReadDataOffset;
        resourceParams.pdwCmd          = (uint32_t*)&(cmd.DW2.Value);
        resourceParams.dwLsbNum        = MHW_PAVP_INLINE_STATUS_READ_DATA_SHIFT;
        resourceParams.dwLocationInCmd = 2;
        resourceParams.dwSize          = params->dwSize;
        resourceParams.bIsWritable     = true;

        // upper bound of the allocated resource will not be set
        resourceParams.dwUpperBoundLocationOffsetFromCmd = 0;

        MHW_CP_CHK_STATUS(pfnAddResourceToCmd(
            osInterface,
            cmdBuffer,
            &resourceParams));
    }

    // this is the pipelined media store dword operation
    // that guarantees the completion of status read request
    {
        MOS_ZeroMemory(&resourceParams, sizeof(resourceParams));
        resourceParams.presResource    = params->presWriteData;
        resourceParams.dwOffset        = params->dwWriteDataOffset;
        resourceParams.pdwCmd          = (uint32_t*)&(cmd.DW5.Value);
        resourceParams.dwLsbNum        = MHW_PAVP_INLINE_DWORD_STORE_DATA_SHIFT;
        resourceParams.dwLocationInCmd = 5;
        resourceParams.dwSize          = params->dwSize;
        resourceParams.bIsWritable     = true;

        // upper bound of the allocated resource will not be set
        resourceParams.dwUpperBoundLocationOffsetFromCmd = 0;

        MHW_CP_CHK_STATUS(pfnAddResourceToCmd(
            osInterface,
            cmdBuffer,
            &resourceParams));
    }

    switch (params->StatusReadType)
    {
    case CRYPTO_INLINE_MEMORY_STATUS_READ:
        //Set DW1{2:0] to the required mode (000 in this case)
        cmd.DW1.InlineReadCmdType = CRYPTO_INLINE_MEMORY_STATUS_READ;
        break;
    case CRYPTO_INLINE_GET_FRESHNESS_READ:
        cmd.DW1.InlineReadCmdType         = CRYPTO_INLINE_GET_FRESHNESS_READ;
        cmd.DW1.KeyRefreshRandomValueType = params->Value.KeyRefreshType;
        break;
    case CRYPTO_INLINE_CONNECTION_STATE_READ:
        cmd.DW1.InlineReadCmdType = CRYPTO_INLINE_CONNECTION_STATE_READ;
        cmd.DW4.Nonce             = params->Value.NonceIn;
        break;
    case CRYPTO_INLINE_GET_WIDI_COUNTER_READ:
        MHW_ASSERTMESSAGE("WiDi counter read mode not supported for platforms prior to KBL");
        MT_ERR2(MT_CP_MHW_STATUS_READ, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, MOS_STATUS_INVALID_PARAMETER);
        return MOS_STATUS_INVALID_PARAMETER;
    default:
        return MOS_STATUS_INVALID_PARAMETER;
    }

    cmd.DW7.ImmediateWriteData0 = MHW_PAVP_QUERY_STATUS_STORE_DWORD_DATA;
    cmd.DW8.ImmediateWriteData1 = MHW_PAVP_QUERY_STATUS_STORE_DWORD_DATA;
    MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, &cmd, sizeof(cmd)));

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS CpHwUnitG8Vcs::AddProlog(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(osInterface->osCpInterface);

    MosCp *pMosCp = dynamic_cast<MosCp *>(osInterface->osCpInterface);
    MHW_CP_CHK_NULL(pMosCp);
    MHW_CP_CHK_NULL(cmdBuffer);
    MHW_CP_CHK_NULL(m_mhwMiInterface);

    // VDBox
    MHW_MI_FLUSH_DW_PARAMS miFlushDwParams;

    MHW_CP_FUNCTION_ENTER;

    //Send CMD_MFX_WAIT...
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMfxWaitCmd(cmdBuffer, nullptr, true));
    MOS_ZeroMemory(&miFlushDwParams, sizeof(miFlushDwParams));
    // get the address of the MI_FLUSH_DW going to be added
    auto miFlushDw = reinterpret_cast<MI_FLUSH_DW_PAVP *>(cmdBuffer->pCmdPtr);  // DW0
    MHW_CP_CHK_NULL(miFlushDw);
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiFlushDwCmd(cmdBuffer, &miFlushDwParams));
    // patch the protection bit OFF
    miFlushDw->ProtectedMemoryEnable = 0;
    // Send MI_SET_APPID
    MHW_CP_CHK_STATUS(AddMiSetAppId(osInterface, cmdBuffer, pMosCp->GetSessionId()));
    // Send CMD_MFX_WAIT
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMfxWaitCmd(cmdBuffer, nullptr, true));

    // MI_FLUSH_DW
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiFlushDwCmd(cmdBuffer, &miFlushDwParams));

    // Insert CRYPTO_KEY_EXCHANGE_COMMAND if Kb based Key Ex
    if (m_VcsEncryptionParams.bInsertKeyExinVcsProlog)
    {
        if (m_VcsEncryptionParams.CryptoKeyExParams.KeyExchangeKeyUse == CRYPTO_TERMINATE_KEY)
        {
            // This is a programming error (either GPU HAL is not updating MHW CP or GPU HAL was not updated when a key was programmed)
            MHW_ASSERTMESSAGE("Cached decrypt/encrypt key is not valid!!!");
            MT_ERR1(MT_CP_INVALID_CACHED_KEY, MT_CODE_LINE, __LINE__);
            MHW_CP_CHK_STATUS(MOS_STATUS_INVALID_PARAMETER);
        }

        uint32_t ret = memcmp(reinterpret_cast<uint8_t *>(m_VcsEncryptionParams.CryptoKeyExParams.EncryptedDecryptionKey), 
            m_cacheEncryptDecryptKey, MHW_PAVP_AES128_NUM_BYTES_IN_BLOCK);
        if (ret != 0)
        {
            MOS_SecureMemcpy(
                &m_cacheEncryptDecryptKey,
                MHW_PAVP_AES128_NUM_BYTES_IN_BLOCK,
                reinterpret_cast<uint8_t *>(m_VcsEncryptionParams.CryptoKeyExParams.EncryptedDecryptionKey),
                MHW_PAVP_AES128_NUM_BYTES_IN_BLOCK);
            MHW_CP_CHK_STATUS(AddMfxCryptoKeyExchange(cmdBuffer, &(m_VcsEncryptionParams.CryptoKeyExParams)));
        }
    }

    //Send CMD_MFX_WAIT...
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMfxWaitCmd(cmdBuffer, nullptr, true));

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}

MOS_STATUS CpHwUnitG8Vcs::AddEpilog(
    PMOS_INTERFACE      osInterface,
    PMOS_COMMAND_BUFFER cmdBuffer)
{
    // VDBox
    MHW_MI_FLUSH_DW_PARAMS miFlushDwParams;

    MOS_UNUSED(osInterface);
    MHW_CP_CHK_NULL(m_mhwMiInterface);
    MHW_CP_CHK_NULL(cmdBuffer);

    // get the address of the MI_FLUSH_DW going to be added
    auto miFlushDw = reinterpret_cast<MI_FLUSH_DW_PAVP *>(cmdBuffer->pCmdPtr);
    MOS_ZeroMemory(&miFlushDwParams, sizeof(miFlushDwParams));
    MHW_CP_CHK_STATUS(m_mhwMiInterface->AddMiFlushDwCmd(cmdBuffer, &miFlushDwParams));
    // but patch the protection bit off
    miFlushDw->ProtectedMemoryEnable = 0;

    MHW_CP_FUNCTION_EXIT(MOS_STATUS_SUCCESS);
    return MOS_STATUS_SUCCESS;
}
