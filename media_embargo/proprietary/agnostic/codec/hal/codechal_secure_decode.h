/*
// Copyright (C) 2017-2021 Intel Corporation
//
// Licensed under the Apache License,Version 2.0(the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
//!
//! \file     codechal_secure_decode.h
//! \brief    Impelements the public interface for CodecHal Secure Decode 
//!
#ifndef __CODECHAL_SECURE_DECODE_H__
#define __CODECHAL_SECURE_DECODE_H__

#include "codechal_hw.h"
#include "mhw_cp.h"
#include "codechal_secure_decode_interface.h"

#define INDIRECT_CRYPTO_STATE_BUFFER_NUM    64          // Equal to Command Buffer Max Number
#define STREAM_OUT_BUFFER_NUM               16

class CodechalSecureDecode : public CodechalSecureDecodeInterface
{
public:
    //!
    //! \brief  Allocate Resource
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    virtual MOS_STATUS AllocateResource(void *state)
    {
        return MOS_STATUS_SUCCESS;
    }

    //!
    //! \brief  Execute Huc StreamOut
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    virtual MOS_STATUS Execute(void *state)
    {
        return MOS_STATUS_SUCCESS;
    }

    //!
    //! \brief  Set Bitstream Buffer for Decode
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    virtual MOS_STATUS SetBitstreamBuffer(MHW_VDBOX_IND_OBJ_BASE_ADDR_PARAMS *indObjBaseAddrParams)
    {
        return MOS_STATUS_SUCCESS;
    }

    //!
    //! \brief  Set HEVC Huc Dmem S2L
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    virtual MOS_STATUS SetHevcHucDmemS2LBss(
        void          *state,
        void          *hucPicBss,
        void          *hucSliceBss)
    {
        return MOS_STATUS_SUCCESS;
    }

    //!
    //! \brief  Add Huc SecureState
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    virtual MOS_STATUS AddHucSecureState(
        void                      *state,
        void                      *sliceState,
        PMOS_COMMAND_BUFFER       cmdBuffer)
    {
        return MOS_STATUS_SUCCESS;
    }

    //!
    //! \brief  Set Huc StreamObject Params
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    virtual MOS_STATUS SetHucStreamObj(
        PMHW_VDBOX_HUC_STREAM_OBJ_PARAMS hucStreamObjParams)
    {
        return MOS_STATUS_SUCCESS;
    }

    //!
    //! \brief  Add Hcp SecureState
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    virtual MOS_STATUS AddHcpSecureState(
        PMOS_COMMAND_BUFFER             cmdBuffer,
        void                            *state)
    {
        return MOS_STATUS_SUCCESS;
    }

    //!
    //! \brief  Reset VP9 SegId Buffer With Huc
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    virtual MOS_STATUS ResetVP9SegIdBufferWithHuc(
        void                    *state,
        PMOS_COMMAND_BUFFER     cmdBuffer)
    {
        return MOS_STATUS_SUCCESS;
    }

    //!
    //! \brief  Update VP9 ProbBuffer With Huc
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    virtual MOS_STATUS UpdateVP9ProbBufferWithHuc(
        bool                    isFullUpdate,
        void                    *state,
        PMOS_COMMAND_BUFFER     cmdBuffer)
    {
        return MOS_STATUS_SUCCESS;
    }

    //!
    //! \brief Encrypt default cdf table buffer through HuC copy
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    virtual MOS_STATUS ProtectDefaultCdfTableBuffer(
        void *state,
        uint32_t bufIndex,
        PMOS_COMMAND_BUFFER cmdBuffer)
    {
        return MOS_STATUS_SUCCESS;
    }

    //!
    //! \brief Return the AV1 temp cdf table buffer
    //! \return PMOS_RESOURCE
    //!
    virtual PMOS_RESOURCE GetTempCdfTableBuffer()
    {
        return nullptr;
    }

    //!
    //! \brief  Is aux data invalid
    //! \return bool
    //!         true if aux data is invalid, else false
    //!
    virtual bool IsAuxDataInvalid(
        PMOS_RESOURCE res);

    //!
    //! \brief Encrypt aux buffer through crypto copy
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    virtual MOS_STATUS InitAuxSurface(
        PMOS_RESOURCE res,
        bool auxUV,
        bool isMmcEnabled);

    //!
    //! \brief  Is Dummy Stream Out Enabled
    //! \return bool
    //!         true if Playready30 and WA Enabled, else false
    //!
    virtual bool IsDummyStreamEnabled()
    {
        return MEDIA_IS_WA(waTable, WaHucStreamoutEnable);
    }

    //!
    //! \brief  Destructor
    //!
    virtual ~CodechalSecureDecode();

    //!
    //! \brief  Constructor
    //! \param  [in] hwInterface
    //!         Hardware interface
    //!
    CodechalSecureDecode(CodechalHwInterface *hwInterfaceInput);

    //!
    //! \brief    Copy constructor
    //!
    CodechalSecureDecode(const CodechalSecureDecode&) = delete;

    //!
    //! \brief    Copy assignment operator
    //!
    CodechalSecureDecode& operator=(const CodechalSecureDecode&) = delete;

    //!
    //! \brief  Update huc streamout buffer index
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS UpdateHuCStreamoutBufferIndex();

    //!
    //! \brief  Use same IV for each subsample
    //!
    void EnableSampleGroupConstantIV();

    bool isStreamoutNeeded = false;

protected:

    //!
    //! \brief  Allocate Huc stream out related resource
    //! \param  [in] streamOutBufRequiredSize
    //!         The required stream out buffer size for current frame
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS AllocateStreamOutResource(uint32_t streamOutBufRequiredSize);

    //!
    //! \brief  Free Huc stream out related resource
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS FreeStreamOutResource();

    //!
    //! \brief  Perform Huc StreamOut to decrypt Bitstream
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS PerformHucStreamOut(
        CodechalHucStreamoutParams          *hucStreamOutParams,
        PMOS_COMMAND_BUFFER                 cmdBuffer);

    //!
    //! \brief  Get Segment Info
    //! \param  [in] cpInterface
    //!         MHW CP interface
    //! \param  [in] dataBuffer
    //!         Resource of encrypted data buffer
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS GetSegmentInfo(
        MhwCpInterface *cpInterface,
        PMOS_RESOURCE dataBufferRes);

    CodechalHwInterface*    hwInterface         = nullptr;
    PMOS_INTERFACE          osInterface         = nullptr;
    MEDIA_WA_TABLE          *waTable            = nullptr;

    uint32_t            hucStreamOutBufSize[STREAM_OUT_BUFFER_NUM];                  //!< Size of HuC stream out buffer
    MOS_RESOURCE        hucStreamOutBuf[STREAM_OUT_BUFFER_NUM];                      //!< Handle of HuC stream out buffer
    uint32_t            hucStreamOutBufIndex      = 0;                               //!< Index of HuC stream out buffer
    uint32_t            segmentNum                = 0;                               //!< Segment number of segment info buffer
    PMHW_SEGMENT_INFO   segmentInfo               = nullptr;                         //!< Segment info buffer
    uint32_t            sampleRemappingBlockNum   = 0;                               //!< Block number of remapped sample block buffer
    uint32_t            *sampleRemappingBlock     = nullptr;                         //!< Remapped sub sample block for stripe mode
    uint32_t            curSegmentNum             = 0;                               //!< Segment number required for current frame
    uint32_t            hucIndCryptoStateBufIndex = 0;                               //!< Index of HucIndCryptoState buffer
    MOS_RESOURCE        hucIndCryptoStateBuf[INDIRECT_CRYPTO_STATE_BUFFER_NUM];      //!< For INDIRECT_CRYPTO_STATE used in AES_IND_STATE command
    uint32_t            hucIndCryptoStateBufSize[INDIRECT_CRYPTO_STATE_BUFFER_NUM];  //!< Size of HucIndCryptoState buffer
    MOS_RESOURCE        auxBuf;                                                      //!< For encrypted 0's initialization
    bool                m_mmcEnabled = false;
    uint32_t            *m_pSubsampleSegmentNum       = nullptr;                     //!< Segment number per Subsample buffer
    bool                m_enableSampleGroupConstantIV = false;

private:
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
        uint32_t subSampleCount,
        uint32_t *subSampleMappingBlock,
        uint32_t stripeEncCount,
        uint32_t stripeClearCount);

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
        PMOS_RESOURCE dataBufferRes,
        uint32_t subSampleCount,
        uint32_t *subSampleMappingBlock);

    //!
    //! \brief  Get Segment Info on stripe mode
    //! \param  [in] cpInterface
    //!         MHW CP interface
    //! \param  [in] dataBufferRes
    //!         Resource of encrypted data buffer
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS GetSegmentInfoStripe(
        MhwCpInterface *cpInterface,
        PMOS_RESOURCE dataBufferRes);

    const uint32_t stripeBlockSize = 16;
};

CodechalSecureDecodeInterface* Create_SecureDecode(
        CodechalSetting *       codecHalSettings,
        CodechalHwInterface      *hwInterfaceInput);

void Delete_SecureDecode(CodechalSecureDecodeInterface* pCodechalSecureDecoeInterface);

#endif // __CODECHAL_SECURE_DECODE_H__
