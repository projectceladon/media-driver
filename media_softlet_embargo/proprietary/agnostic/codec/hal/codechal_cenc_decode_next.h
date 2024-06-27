/*
// Copyright (C) 2022 Intel Corporation
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
//! \file      codechal_cenc_decode_next.h
//! \brief     Impelements the public interface for CodecHal CENC Decode
//!
#ifndef __CODECHAL_CENC_DECODE_NEXT_H__
#define __CODECHAL_CENC_DECODE_NEXT_H__

#include "codechal_cenc_decode_def.h"
#include "media_interfaces_mhw_next.h"
#include "codec_hw_next.h"

class CodechalCencDecodeNext
{
public:
    //!
    //! \brief    Copy constructor
    //!
    CodechalCencDecodeNext(const CodechalCencDecodeNext &) = delete;

    //!
    //! \brief    Copy assignment operator
    //!
    CodechalCencDecodeNext &operator=(const CodechalCencDecodeNext &) = delete;

    //!
    //! \brief  Destructor
    //!
    virtual ~CodechalCencDecodeNext();

    //!
    //! \brief  Create Cenc Decode
    //! \param  [in] standard
    //!         The codec standard
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    static MOS_STATUS Create(
        CODECHAL_STANDARD      standard,
        CodechalCencDecodeNext **cencDecoder);

    //!
    //! \brief  Initilize
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    virtual MOS_STATUS Initialize(
        PMOS_CONTEXT     osContext,
        CodechalSetting *settings);

    //!
    //! \brief  Execute Bitstream Cenc Decode
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS Execute(void *params);

    //!
    //! \brief  Get Status Report
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS GetStatusReport(void *status, uint16_t numStatus, HUC_STATUS *hucStatus = nullptr);

    //!
    //! \brief  Set Decode Parameters
    //! \param  [in] decodeParams
    //!         Pointer of CodechalDecodeParams
    //! \param  [in] cpParam
    //!         Pointer of cpParams
    //! \return  MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    virtual MOS_STATUS SetDecodeParams(
        CodechalDecodeParams *decodeParams,
        void                 *cpParam)
    {
        return MOS_STATUS_SUCCESS;
    };

    //!
    //! \brief  Get the data buffer
    //! \return  PMOS_RESOURCE
    //!         Pointer to the data buffer
    //!
    PMOS_RESOURCE GetDataBuffer() { return &m_resDataBuffer; }

    //!
    //! \brief  Update the data buffer
    //! \return  MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS UpdateDataBuffer(void *buffer, uint32_t size);

    //!
    //! \brief  Get cp interface
    //! \return Pointer of MhwCpBase
    //!         Pointer of MhwCpBase
    //!
    MhwCpBase *GetCpInterface() { return m_cpInterface; }

    //!
    //! \brief  Get os interface
    //! \return Pointer of PMOS_INTERFACE
    //!         Pointer of PMOS_INTERFACE
    //!
    PMOS_INTERFACE GetOsInterface() { return m_osInterface; }

    //!
    //! \brief  Set status report buffer size
    //! \param  [in] size
    //!         status buffer size
    //! \return void
    //!
    void SetReportSizeApp(uint32_t size);

protected:
    //!
    //! \brief  Constructor
    //!
    CodechalCencDecodeNext();

    //!
    //! \brief  Parse Shared Buffer
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    virtual MOS_STATUS ParseSharedBufParams(
        uint16_t bufIdx,
        uint16_t reportIdx,
        void     *buffer)
    {
        return MOS_STATUS_SUCCESS;
    };

    //!
    //! \brief  Update Secure OptMode
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    virtual MOS_STATUS UpdateSecureOptMode(PMHW_PAVP_ENCRYPTION_PARAMS params)
    {
        return MOS_STATUS_SUCCESS;
    };

    //!
    //! \brief  Initilize Shared Buffer
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    virtual MOS_STATUS InitializeSharedBuf(bool initStatusReportOnly)
    {
        return MOS_STATUS_SUCCESS;
    };

    //!
    //! \brief  Initilize Allocation
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    virtual MOS_STATUS InitializeAllocation()
    {
        return MOS_STATUS_SUCCESS;
    };

    //!
    //! \brief  Update Global Cmdbuffer Id
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    virtual MOS_STATUS UpdateGlobalCmdBufId(
        std::shared_ptr<mhw::mi::Itf> m_miItf,
        PMOS_COMMAND_BUFFER           cmdBuffer,
        CODECHAL_CENC_HEAP_INFO       *heapInfo);

    //!
    //! \brief  Allocate Resources
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    virtual MOS_STATUS AllocateResources();

    //!
    //! \brief  Allocate Decode Resource
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    virtual MOS_STATUS AllocateDecodeResource(uint8_t bufIdx);

    //!
    //! \brief    Add Huc stream out copy cmds
    //! \details  Prepare and add Huc stream out copy cmds
    //!
    //! \param    [in,out] cmdBuffer
    //!           Command buffer
    //!
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS AddHucDummyStreamOut(
        PMOS_COMMAND_BUFFER cmdBuffer);

    //!
    //! \brief    Perform Huc stream out copy
    //! \details  Implement the copy using huc stream out
    //!
    //! \param    [in] hucStreamOutParams
    //!           Huc stream out parameters
    //! \param    [in,out] cmdBuffer
    //!           Command buffer
    //!
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS PerformHucStreamOut(
        CencHucStreamoutParams  *hucStreamOutParams,
        PMOS_COMMAND_BUFFER     cmdBuffer);

    //!
    //! \brief    Fix emulation prevention
    //! \details  Fix emulation prevention
    //!
    //! \param    [in,out] outBitStreamBuf
    //!           Output bitstream buffer
    //! \param    [out] byteWritten
    //!           Byte written to the bitstream buffer
    //!
    //! \return   void
    //!
    void FixEmulationPrevention(uint8_t *&outBitStreamBuf, uint32_t *byteWritten);

    //!
    //! \brief    Write bits
    //! \details  Writes bits to the bitstream buffer
    //!
    //! \param    [in] value
    //!           Value needs to be packed
    //! \param    [in] valueValidBits
    //!           Number of bits of value type
    //! \param    [in,out] accumulator
    //!           Bits accumulator
    //! \param    [in,out] accumulatorValidBits
    //!           Number of valid bits in accumulator
    //! \param    [in,out] outBitStreamBuf
    //!           Output bitstream buffer
    //! \param    [out] byteWritten
    //!           Byte written to the bitstream buffer
    //! \param    [in] fixEmulation
    //!
    //! \return   void
    //!
    void WriteBits(uint32_t value, uint8_t valueValidBits,
    uint8_t *accumulator, uint8_t *accumulatorValidBits,
    uint8_t *&outBitStreamBuf, uint32_t *byteWritten, bool fixEmulation = true);

    //!
    //! \brief    Encode exp golomb
    //! \details  Implement the exp golomb bitstream packing
    //!
    //! \param    [in] value
    //!           Value needs to be packed
    //! \param    [in] valueValidBits
    //!           Number of bits of value type
    //! \param    [in] bufSize
    //!           bitstream buffer size
    //! \param    [in,out] accumulator
    //!           Bits accumulator
    //! \param    [in,out] accumulatorValidBits
    //!           Number of valid bits in accumulator
    //! \param    [in,out] outBitStreamBuf
    //!           Output bitstream buffer
    //! \param    [out] byteWritten
    //!           Byte written to the bitstream buffer
    //!
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS EncodeExpGolomb(uint32_t value, uint8_t valueValidBits,
    uint32_t bufSize, uint8_t *accumulator, uint8_t *accumulatorValidBits,
    uint8_t *&outBitStreamBuf, uint32_t *byteWritten);

    // External components and decrypt flag
    CodechalDebugInterface        *m_debugInterface = nullptr;  //! \debug interface
    MhwInterfacesNext             *m_mhwInterfaces   = nullptr;
    CodechalHwInterfaceNext       *m_hwInterface    = nullptr;
    MhwCpBase                     *m_cpInterface    = nullptr;

    std::shared_ptr<mhw::mi::Itf>         m_miItf  = nullptr;
    std::shared_ptr<mhw::vdbox::huc::Itf> m_hucItf = nullptr;
    std::shared_ptr<mhw::vdbox::mfx::Itf> m_mfxItf = nullptr;
    std::shared_ptr<mhw::vdbox::hcp::Itf> m_hcpItf = nullptr;

    PLATFORM                m_platform;               //!< Platform information
    MEDIA_FEATURE_TABLE     *m_skuTable   = nullptr;  //!< SKU table
    MEDIA_WA_TABLE          *m_waTable    = nullptr;  //!< WA table
    PMOS_INTERFACE          m_osInterface = nullptr;
    CencDecodeShareBuf      m_shareBuf    = {};
    uint32_t                m_standard    = 0;                          // so we can do diff things on diff Codec standards
    uint32_t                m_mode        = CODECHAL_UNSUPPORTED_MODE;  //! \brief Decode mode
    uint32_t                dwFrameNum    = 0;
    uint32_t                m_HuCFamily   = 0;

    HeapManager             *m_secondLvlBbHeap = nullptr;  // Heap manager for second level
    HeapManager             *m_bitstreamHeap   = nullptr;  // Heap manager for streamed out bitstream buffers
    CODECHAL_CENC_HEAP_INFO m_heapInfo[CENC_NUM_DECRYPT_SHARED_BUFFERS];
    // consider to swap out for DW0 of status report for decode later
    MOS_RESOURCE             m_resTracker     = {0};  // tracker resource to mark frame completion for the heaps
    uint32_t                 *m_lockedTracker = nullptr;
    uint32_t                 m_currTrackerId  = 0;
    std::vector<MemoryBlock> m_blocks         = {};
    std::vector<uint32_t>    m_blockSizes     = {};

    // HuC WA implementation
    MOS_RESOURCE m_dummyStreamIn    = {0};  //!> Resource of dummy stream in
    MOS_RESOURCE m_dummyStreamOut   = {0};  //!> Resource of dummy stream out
    MOS_RESOURCE m_hucDmemDummy     = {0};  //!> Resource of Huc DMEM for dummy streamout WA
    uint32_t     m_dummyDmemBufSize = 0;    //!> Dummy Dmem size

    // Internally used by AVC/HEVC decode
    MOS_GPU_NODE    m_videoGpuNode           = MOS_GPU_NODE_MAX;
    MOS_GPU_CONTEXT m_videoContextForDecrypt = MOS_GPU_CONTEXT_INVALID_HANDLE;
    //! \brief The vdbox index for current frame
    MHW_VDBOX_NODE_IND m_vdboxIndex = MHW_VDBOX_NODE_1;
    //! \brief Indicates if video context uses null hardware
    bool                        m_videoContextUsesNullHw = false;
    CODECHAL_CENC_STATUS_BUFFER m_decryptStatusBuf;
    uint16_t                    m_HuCRevId              = 0;  // HuC familiy revision ID
    uint8_t                     m_urrDecryptBufIdx      = 0;  // the current idx for decrypt
    uint8_t                     m_currSharedBufIdx      = 0;  // the current idx for shared buffers between decrypt and decode
    uint32_t                    m_hucDecryptSizeNeeded  = 0;  // Huc command buffer size
    uint32_t                    m_standardSlcSizeNeeded = 0;  // Huc slice level batch buffer size
    uint32_t                    m_standardReportSizeApp = 0;
    uint32_t                    m_incompleteReportNum   = 0;
    uint32_t                    m_numSlices             = 0;  // number of slices
    uint32_t                    m_picSize               = 0;

    uint8_t m_sizeOfLength[CODECHAL_NUM_DECRYPT_BUFFERS] = {0};
    bool    m_dashMode                                   = false;
    bool    m_containsOutOfBandData                      = false;
    bool    m_lengthModeDisabledForDash                  = false;
    bool    m_enableAesNativeSupport                     = false;  // Aes Native Optimization is enabled once it is true
    bool    m_aesFromHuC                                 = false;  // HuC will generate Aes state information once it is true
    bool    m_noStreamOutOpt                             = false;  // Stream out from HuC will be disabled once it is true

    bool m_pr30Enabled = false;

    MOS_RESOURCE m_resHucSharedBuf                                       = {0};      // shared buffer for status reporting
    uint32_t     m_sharedBufSize                                         = 0;        // size of shared buffer (varies with coding standard)
    void         *m_hucSharedBuf                                          = nullptr;  // pointer to the shared buffer
    MOS_RESOURCE m_resHucIndCryptoState[CODECHAL_NUM_DECRYPT_BUFFERS]    = {{0}};    // for INDIRECT_CRYPTO_STATE used in HuC_AES_IND_STATE command
    uint32_t     m_hucIndCryptoStateNumSegments                          = 0;
    MOS_RESOURCE m_resHcpIndCryptoState[CENC_NUM_DECRYPT_SHARED_BUFFERS] = {{0}};  // for INDIRECT_CRYPTO_STATE used in HCP_AES_IND_STATE command
    uint32_t     m_hcpIndCryptoStateSize                                 = 0;

    MOS_RESOURCE m_resHucSegmentInfoBuf[CODECHAL_NUM_DECRYPT_BUFFERS]      = {{0}};  // for segment buffer for native dash support
    uint32_t     m_hucSegmentInfoNumSegments[CODECHAL_NUM_DECRYPT_BUFFERS] = {0};
    MOS_RESOURCE m_resHucSegmentInfoBufClear[CODECHAL_NUM_DECRYPT_BUFFERS] = {{0}};  //Clear mode segment buffer
    uint32_t     m_hucSegmentInfoBufSize                                   = 0;

    MOS_RESOURCE m_resHucDmem[CODECHAL_NUM_DECRYPT_BUFFERS] = {{0}};  // resource to program DMEM command
    uint32_t     m_dmemBufSize                              = 0;      // size of DMEM buf

    MHW_BATCH_BUFFER m_resSlcLevelBatchBuf[CODECHAL_NUM_DECRYPT_BUFFERS];  // pool of 2nd level BB written to by Huc
    uint32_t         m_sliceLevelBatchBufSize = 0;                         // maximum 2nd level batch buffer size

    uint16_t                   m_statusReportNumbers[CENC_NUM_DECRYPT_SHARED_BUFFERS]   = {0};
    void                       *m_picParams[CENC_NUM_DECRYPT_SHARED_BUFFERS]             = {0};  // pool of Pic Params
    void                       *m_qmParams[CENC_NUM_DECRYPT_SHARED_BUFFERS]              = {0};
    void                       *m_dpbParams[CENC_NUM_DECRYPT_SHARED_BUFFERS]             = {0};  // parameters for DPB managment in VP9
    MHW_PAVP_ENCRYPTION_PARAMS m_savedEncryptionParams[CENC_NUM_DECRYPT_SHARED_BUFFERS] = {};
    MOS_RESOURCE               m_resEncryptedStatusReportNum                            = {0};  // 64*3 bytes for every decrypt index
    uint8_t                    m_hucKernelDebugLevel                                    = 0;
    uint32_t                   m_bitstreamSize                                          = 0;
    uint32_t                   m_bitstreamSizeMax                                       = 0;
    uint32_t                   m_sliceLevelBBSize                                       = 0;
    uint32_t                   m_sliceLevelBBSizeMax                                    = 0;
    uint32_t                   m_decryptDelayStart                                      = 0;
    uint32_t                   m_decryptDelayStep                                       = 0;

    bool         m_serialMode    = false;
    MOS_RESOURCE m_resDataBuffer = {0};  // data buffer for serial mode
    bool         m_mmcEnabled    = false;

    MediaUserSettingSharedPtr m_userSettingPtr = nullptr;

private:
    //!
    //! \brief    Verify Space Available
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS VerifySpaceAvailable();

    //!
    //! \brief    Send Prolog With Frame Tracking
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS SendPrologWithFrameTracking(PMOS_COMMAND_BUFFER cmdBuffer);

    //!
    //! \brief    Set Huc Dmem Params
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS SetHucDmemParams();

    //!
    //! \brief    Set Huc Dash Aes Segment Info
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS SetHucDashAesSegmentInfo(
        PMHW_PAVP_AES_IND_PARAMS params,
        PMOS_COMMAND_BUFFER      cmdBuffer);

    //!
    //! \brief    Update Current CmdBuf Id
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS UpdateCurrCmdBufId();

    //!
    //! \brief    Assign Sync Tag
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS AssignSyncTag();

    //!
    //! \brief    Update Heap Extend Size
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS UpdateHeapExtendSize(
        HeapManager  *heapManager,
        uint32_t     requiredSize);

    //!
    //! \brief    Assign Space In State Heap
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS AssignSpaceInStateHeap(
        CODECHAL_CENC_HEAP_INFO *heapInfo,
        HeapManager             *heapManager,
        MemoryBlock             *block,
        uint32_t                dwSize);

    //!
    //! \brief    Assign and Copy SliceLevel BB
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS AssignAndCopyToSliceLevelBb(uint8_t bufIdx);

    //!
    //! \brief    Start Status Report
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS StartStatusReport(PMOS_COMMAND_BUFFER cmdBuffer);

    //!
    //! \brief    End Status Report
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS EndStatusReport(PMOS_COMMAND_BUFFER cmdBuffer);

    //!
    //! \brief    Reset Status Report
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS ResetStatusReport(bool nullHwInUse);

MEDIA_CLASS_DEFINE_END(CodechalCencDecodeNext)
};

#endif  // __CODECHAL_CENC_DECODE_NEXT_H__
