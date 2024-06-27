/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2022
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
//! \file     mhw_cp_next.cpp
//! \brief    MHW interface for content protection
//! \details  Impelements the base functionalities for content protection
//!

#include "mhw_cp_next.h"
#include "mhw_cp_hwcmd_common.h"
#include "mhw_vdbox.h"
#include "mhw_cp_hwcmd_next.h"

// ----- static functions shared by several Gens. ----------
MOS_STATUS MhwCpNextBase::AddMfxCryptoCopyBaseAddr(
    MhwCpBase*                        mhwCp,
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

MhwCpNextBase::~MhwCpNextBase()
{
}

MOS_STATUS MhwCpNextBase::AddMfxAesState(
    MhwCpBase*      mhwCp,
    PMOS_INTERFACE       osInterface,
    bool                 isDecodeInUse,
    PMOS_COMMAND_BUFFER  cmdBuffer,
    PMHW_BATCH_BUFFER    batchBuffer,
    PMHW_PAVP_AES_PARAMS params)
{
    uint32_t        counterIncrement;
    uint32_t        aesCtr[4] = {0};
    MOS_LOCK_PARAMS lockFlags;

    MHW_CP_FUNCTION_ENTER;

    MHW_CP_CHK_NULL(mhwCp);
    MHW_CP_CHK_NULL(osInterface);
    MHW_CP_CHK_NULL(params);

    auto encryptionParams = mhwCp->GetEncryptionParams();
    auto cmd              = g_cInit_MFX_AES_STATE_CMD_G9;

    MHW_CP_CHK_STATUS_MESSAGE(MOS_SecureMemcpy(
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
            PMHW_SUB_SAMPLE_MAPPING_BLOCK pSubSampleBlocks = reinterpret_cast<PMHW_SUB_SAMPLE_MAPPING_BLOCK>(encryptionParams->pSubSampleMappingBlock);
            PMHW_SUB_SAMPLE_MAPPING_BLOCK pCurrentBlock = &(pSubSampleBlocks[params->dwSliceIndex]);

            cmd.DW7.InitPacketBytesLength = pCurrentBlock->ClearSize;
            cmd.DW8.EncryptedBytesLength = pCurrentBlock->EncryptedSize;
            cmd.DW9.NumberByteInputBitstream = pCurrentBlock->ClearSize;
            cmd.DW8.LastPartialBlockAdj = 1;

            uint32_t encryptedSize = 0;
            for(uint32_t i = 0; i < params->dwSliceIndex; i++)
            {
                encryptedSize += pSubSampleBlocks[i].EncryptedSize;
            }
            counterIncrement = encryptedSize >> 4;
        }
        else
        {
            // Data offset is coming from the application.
            // For each slice we need to calculate the offset based on the number of Bytes in previous slices.
            uint32_t dwZeroPadding = params->dwDataStartOffset - params->dwTotalBytesConsumed;

            // Clear bytes (which are the slice header and part of the bit stream) contains also the start code (0 0 1).
            // According to the DECE spec: Clear Bytes + zero padding = 128 exactly.
            uint32_t dwNumClearBytes = params->dwDefaultDeceInitPacketLengthInBytes - dwZeroPadding;

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
                cmd.DW7.InitPacketBytesLength = dwNumClearBytes - MOS_NAL_UNIT_STARTCODE_LENGTH;

                // Number of encrypted bytes.
                cmd.DW8.EncryptedBytesLength = params->dwDataLength - dwNumClearBytes;
            }
        }
    }
    else if (encryptionParams->PavpEncryptionType == CP_SECURITY_AES128_CBC)
    {
        MHW_CP_CHK_NULL(params->presDataBuffer);

        uint32_t *pdwLastEncryptedOword = nullptr;

        MOS_ZeroMemory(&lockFlags, sizeof(MOS_LOCK_PARAMS));
        lockFlags.ReadOnly          = 1;
        auto pdwFirstEncryptedOword = static_cast<uint32_t*>(osInterface->pfnLockResource(
            osInterface,
            params->presDataBuffer,
            &lockFlags));
        MHW_CP_CHK_NULL(pdwFirstEncryptedOword);

        pdwLastEncryptedOword =
            pdwFirstEncryptedOword +
            ((params->dwDataStartOffset >> 4) * 4);

        if (pdwLastEncryptedOword != pdwFirstEncryptedOword)
        {
            pdwLastEncryptedOword -= KEY_DWORDS_PER_AES_BLOCK;  // The block size of AES-128 is 128 bits, so the subtrahend is 4.
            aesCtr[0] = pdwLastEncryptedOword[0];
            aesCtr[1] = pdwLastEncryptedOword[1];
            aesCtr[2] = pdwLastEncryptedOword[2];
            aesCtr[3] = pdwLastEncryptedOword[3];
        }

        osInterface->pfnUnlockResource(osInterface, params->presDataBuffer);

        counterIncrement = 0;
    }
    else
    {
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

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS MhwCpNextBase::AddMfxCryptoCopy(
    MhwCpBase*                   mhwCp,
    PMOS_COMMAND_BUFFER          cmdBuffer,
    PMHW_PAVP_CRYPTO_COPY_PARAMS params)
{
    mhw_pavp_next::MFX_CRYPTO_COPY_CMD cmd;
    uint32_t                            maxCmdSize        = MHW_VDBOX_MAX_CRYPTO_COPY_SIZE;
    uint32_t                            maxEncryptedBytes = maxCmdSize;  // In case of partial decryption, we don't want to include the clear bytes when incrementing the counter.
    uint32_t                            dataOffset        = 0;

    MHW_CP_CHK_NULL(mhwCp);
    MHW_CP_CHK_NULL(params);

    if (params->CmdMode == CRYPTO_COPY_CMD_LIST_MODE)
    {
        cmd.DW1.SelectionForIndirectData = params->ui8SelectionForIndData;
        cmd.DW2.SrcBaseOffset            = params->dwSrcBaseOffset;
        cmd.DW4.DstBaseOffset            = params->dwDestBaseOffset;

        cmd.DW13.cryptoCopyCmdMode = params->CmdMode;
        cmd.DW13.lengthOfTable     = params->LengthOfTable;

        MHW_CP_CHK_STATUS(Mos_AddCommand(cmdBuffer, &cmd, sizeof(cmd)));
    }

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
