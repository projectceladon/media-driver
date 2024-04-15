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
//! \file     decodecp.h
//! \brief    Impelements the public interface for CodecHal Secure Decode
//!
#ifndef __DECODE_CP_H__
#define __DECODE_CP_H__

#include "mhw_cp.h"
#include "decodecp_interface.h"

typedef struct
{  // CP
    uint8_t AESEnable : 1;
    uint8_t IsPlayReady3_0InUse : 1;
    uint8_t IsCTRMode : 1;
    uint8_t CTRFlavor : 2;
    uint8_t IsFairPlay : 1;
    uint8_t InitialFairPlayEnabled : 1;
    uint8_t IsPlayReady3_0HcpSupported : 1;

    //Fair Play
    uint16_t InitPktBytes;
    uint32_t EncryptedBytes;
    uint32_t ClearBytes;
} HUC_S2L_PIC_BSS_EXT, *PHUC_S2L_PIC_BSS_EXT;

typedef struct
{
    // CP
    uint32_t InitializationCounterValue0;
    uint32_t InitializationCounterValue1;
    uint32_t InitializationCounterValue2;
    uint32_t InitializationCounterValue3;
} HUC_S2L_SLICE_BSS_EXT, *PHUC_S2L_SLICE_BSS_EXT;

class DecodeCp : public DecodeCpInterface
{
public:
    //!
    //! \brief  Add Hcp SecureState
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS AddHcpState(
        PMOS_COMMAND_BUFFER cmdBuffer,
        PMOS_RESOURCE       presDataBuffer,
        uint32_t            length,
        uint32_t            startoffset,
        uint32_t            dwsliceIndex);

    //!
    //! \brief  Create Vvcp Ind State Packet
    //! \return DecodeSubPacket pointer
    //! 
    //!
    decode::DecodeSubPacket *CreateDecodeCpIndSubPkt(
        decode::DecodePipeline *pipeline, 
        CODECHAL_MODE           mode,
        CodechalHwInterfaceNext *hwInterface);

    //!
    //! \brief  Execute Vvcp Ind State Packet
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS ExecuteDecodeCpIndSubPkt(
        decode::DecodeSubPacket *packet,
        CODECHAL_MODE            mode,
        MOS_COMMAND_BUFFER& cmdBuffer,
        uint32_t            sliceIndex);


    //!
    //! \brief  Add Huc State
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS AddHucState(
        PMOS_COMMAND_BUFFER cmdBuffer,
        PMOS_RESOURCE       presDataBuffer,
        uint32_t            length,
        uint32_t            startoffset,
        uint32_t            dwsliceIndex);

    //!
    //! \brief  Add Huc Indirect Crypto State
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS AddHucIndState(
        PMOS_COMMAND_BUFFER cmdBuffer,
        PMOS_RESOURCE presIndState);

    //!
    //! \brief  Set Huc Dmem S2L Pic Bss
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS SetHucDmemS2LPicBss(
        void *hucS2LPicBss,
        PMOS_RESOURCE presDataBuffer);

    //!
    //! \brief  Set Huc Dmem S2L Slice Bss
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS SetHucDmemS2LSliceBss(
        void *hucS2LSliceBss,
        uint32_t index,
        uint32_t dataSize,
        uint32_t dataOffset);

    MOS_STATUS RegisterParams(void *settings);

    MOS_STATUS UpdateParams(bool input);

    MOS_STATUS SetHucPipeModeSelectParameter(bool &disableProtection);

    MOS_STATUS CalculateSegmentInfo(PMOS_RESOURCE dataBufferRes);
    MOS_STATUS CalculateSliceSegmentInfo(PMOS_RESOURCE dataBufferRes);
    //!
    //! \brief  Allocate StreamOut Resource
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS AllocateStreamOutResource();

    //!
    //! \brief  Allocate Decode Resource
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS AllocateDecodeResource();

    //!
    //! \brief  Update Slice Segment Info
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS UpdateSliceSegmentInfo();

    MOS_STATUS UpdateSliceInfo(
        uint32_t *sliceDataSize,
        uint32_t *sliceHeaderSize,
        uint32_t *bsdDataStartOffset,
        uint32_t sliceNum);

    //!
    //! \brief  Get SegmentInfo For Ind State
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS GetSegmentInfoForIndState(
        uint32_t indirectCryptoStateIndex, 
        uint32_t &curNumSegments, 
        PMHW_SEGMENT_INFO &pSegmentInfo);

    //!
    //! \brief  Get All SegmentInfo 
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS GetSegmentInfo(
        PMHW_SEGMENT_INFO &pSegmentInfo);

    //!
    //! \brief  Get Indirect State Size
    //!
    uint32_t GetIndStateSize();

    //!
    //! \brief  Get Indirect State Size by indirectCryptoStateIndex
    //!
    uint32_t GetIndStateSize(uint32_t indirectCryptoStateIndex);

    //!
    //! \brief  Get Indirect State Number
    //!
    uint32_t   GetIndStateNum();

    //!
    //! \brief  Set Indirect State Number
    //!
    MOS_STATUS SetIndStateNum(uint32_t indStateNum);

    //!
    //! \brief  Fetch current HuC Copy Length
    //! \param  [in] index
    //!         huc copy segment length index
    //!
    uint32_t   FetchCopyLength(uint32_t index);

    //!
    //! \brief  check if streamOut is needed
    //! \return true if streamOut is needed
    //!
    bool IsStreamOutNeeded();

    //!
    //! \brief  check if streamOut is enabled
    //! \return true if streamOut is enabled
    //!
    bool IsStreamOutEnabled();

    //!
    //! \brief  check if AES indirect packet is needed
    //! \return true if AES indirect packet is needed
    //!
    bool IsAesIndSubPacketNeeded();

    //!
    //! \brief  check if CpEnabled
    //! \return true if CP enabled, false otherwise
    //!
    bool       IsCpEnabled() { return m_osInterface->osCpInterface->IsCpEnabled(); };

    // only for disable cp temporally when cp is enabled.
    void       SetCpEnabled(bool isCpInUse);

    //!
    //! \brief  Set HuC Copy Length for further check
    //! \param  [in] size
    //!         huc copy size
    //!
    void SetCopyLength(uint32_t size);

    //!
    //! \brief  Init
    //! \param  [in] codecHalSettings
    //!         Codechal settings
    //!
    MOS_STATUS Init(CodechalSetting *codecHalSettings);

    //!
    //! \brief  Destructor
    //!
    ~DecodeCp();

    //!
    //! \brief  Constructor
    //!
    DecodeCp(MhwCpBase *mhwCp,
        MhwCpInterface *cpInterface,
        PMOS_INTERFACE  osInterface);

    //!
    //! \brief    Copy constructor
    //!
    DecodeCp(const DecodeCp &) = delete;

    //!
    //! \brief    Copy assignment operator
    //!
    DecodeCp &operator=(const DecodeCp &) = delete;

protected:
    MhwCpBase *     m_mhwCp       = nullptr;
    MhwCpInterface *m_cpInterface = nullptr;
    PMOS_INTERFACE  m_osInterface = nullptr;
    MediaUserSettingSharedPtr m_userSettingPtr = nullptr;

private:
    PMOS_RESOURCE         m_pDmemResBuffer          = nullptr;
    uint32_t              m_segmentNum              = 0;        //!< Segment number of segment info buffer
    PMHW_SEGMENT_INFO     m_pSegmentInfo            = nullptr;  //!< Segment info buffer
    uint32_t              m_sampleRemappingBlockNum = 0;        //!< Block number of remapped sample block buffer
    uint32_t *            m_pSampleRemappingBlock   = nullptr;  //!< Remapped sub sample block for stripe mode
    uint32_t *            m_pSampleSliceRemappingBlock = nullptr;
    uint32_t              curSegmentNum             = 0;        //!< Segment number required for current frame
    uint32_t              m_hucCopyLength           = 0;        //!< huc copy length for further check

    uint32_t *            m_pSubsampleSegmentNum    = nullptr;  //!< Segment number per Subsample buffer
    uint32_t *            m_pSubsampleSegmentEnd    = nullptr;  //!< Segment ending index per Subsample buffer

    const uint32_t        maxIndirectCryptoStateNum = 65536;    //!< Max count of indirect crypto state with 16 bits entries number
    bool                  m_splitWorkload           = false;
    uint32_t              m_indStateNum             = 1;        //!< Indirect state buffer count required for current frame
    std::vector<uint32_t> m_hucCopySegmentLengths   = {};       //!< huc copy segment length for each splitted workload
    uint32_t              m_indStateIdx             = 0; 
    DecodeCpMode          m_decodeCpMode            = HUC_PIPELINE_EXTENDED;

    uint32_t             *m_bsdDataStartOffset      = nullptr;
    uint32_t             *m_sliceDataSize           = nullptr;
    uint32_t             *m_sliceHeaderSize         = nullptr;
    uint32_t             *m_sliceSegmentNum         = nullptr;  //!< Segment numbers of each slice
    uint32_t              m_sliceRemappingBlockNum  = 0;

    //!
    //! \brief  Get Slice Init Value
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS GetSliceInitValue(
        PMHW_PAVP_AES_PARAMS params,
        uint32_t *           value);

    uint32_t InitialCounterPerSlice[CODECHAL_HEVC_MAX_NUM_SLICES_LVL_6][4];  //!< Initial AES counter for each slice

    //!
    //! \brief  Compute sub segment number on stripe mode
    //! \param  [in] encryptedSize
    //!         Encryption size for sample blocks
    //! \param  [in] stripeEncCount
    //!         Number of strpe encryption blocks
    //! \param  [in] stripeClearCount
    //!         Number of strpe clear blocks
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    uint32_t ComputeSubSegmentNumStripe(
        uint32_t encryptedSize,
        uint32_t stripeEncCount,
        uint32_t stripeClearCount);

    //!
    //! \brief  Compute sub encryption length on stripe mode
    //! \param  [in] remappingSize
    //!         The size for remaing bytes to parse
    //! \param  [in] stripeEncCount
    //!         Number of strpe encryption blocks
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    uint32_t ComputeSubStripeEncLen(
        uint32_t remappingSize,
        uint32_t stripeEncCount);

    //!
    //! \brief  Compute sub encryption length on stripe mode
    //! \param  [in] remappingSize
    //!         The size for remaing bytes to parse
    //! \param  [in] stripeClearCount
    //!         Number of strpe clear blocks
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    uint32_t ComputeSubStripeClearLen(
        uint32_t remappingSize,
        uint32_t stripeClearCount);

    //!
    //! \brief  Compute remapping blocks on stripe mode
    //! \param  [in] subSampleCount
    //!         Number of sample blocks
    //! \param  [in] subSampleMappingBlock
    //!         Sample mapping blocks
    //! \param  [in] stripeEncCount
    //!         Number of strpe encryption blocks
    //! \param  [in] stripeClearCount
    //!         Number of strpe clear blocks
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS ComputeRemappingBlockStripe(
        uint32_t  subSampleCount,
        uint32_t *subSampleMappingBlock,
        uint32_t  stripeEncCount,
        uint32_t  stripeClearCount);

    MOS_STATUS ComputeSliceRemappingBlock(
        uint32_t  subSampleCount,
        uint32_t *subSampleMappingBlock,
        std::vector<std::pair<uint32_t, uint32_t>> &map);

    //!
    //! \brief  Get Segment Info in legacy mode which doesn't support stripe
    //! \param  [in] cpInterface
    //!         MHW CP interface
    //! \param  [in] dataBufferRes
    //!         Resource of encrypted data buffer
    //! \param  [in] subSampleCount
    //!         Number of sample blocks
    //! \param  [in] subSampleMappingBlock
    //!         Sample mapping blocks
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS GetSegmentInfoLegacy(
        MhwCpInterface *cpInterface,
        PMOS_RESOURCE   dataBufferRes,
        uint32_t        subSampleCount,
        uint32_t *      subSampleMappingBlock);

    //!
    //! \brief  Get Segment Info on stripe mode
    //! \param  [in] cpInterface
    //!         MHW CP interface
    //! \param  [in] dataBufferRes
    //!         Resource of encrypted data buffer
    //! \param  [in] subSampleCount
    //!         Number of sample blocks
    //! \param  [in] subSampleMappingBlock
    //!         Sample mapping blocks
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS GetSegmentInfoStripe(
        MhwCpInterface *cpInterface,
        PMOS_RESOURCE   dataBufferRes,
        uint32_t        subSampleCount,
        uint32_t       *subSampleMappingBlock);

    //!
    //! \brief  Reset Host Encrypt Mode
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    void ResetHostMode();

    const uint32_t stripeBlockSize = 16;

MEDIA_CLASS_DEFINE_END(DecodeCp)
};

DecodeCpInterface *Create_DecodeCp(
    CodechalSetting *codecHalSettings,
    MhwCpBase       *mhwCp,
    MhwCpInterface  *cpInterface,
    PMOS_INTERFACE   osInterface);

void Delete_DecodeCp(DecodeCpInterface *pDecodeCpInterface);

#endif  // __CODECHAL_DECODE_CP_H__