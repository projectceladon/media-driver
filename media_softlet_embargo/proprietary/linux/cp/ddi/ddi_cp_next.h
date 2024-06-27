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
//! \file     ddi_cp_next.h
//! \brief    DDI interface for content protection parameters and operations.
//! \details  Other components will call this interface for content protection operations directly
//!

#ifndef  __DDI_CP_NEXT_H__
#define  __DDI_CP_NEXT_H__
#include "media_libva.h"
#include "va_private.h"
#include "mos_os.h"
#include "ddi_cp_interface_next.h"
#include "mos_os_cp_specific.h"
#include "mos_utilities_common.h"
#include "codechal_cenc_decode_next.h"
#include "ddi_cp_utils.h"

class CPavpCryptoSessionVANext;

///////////////////////////////////////////////////////////////////////////////
/// \class       DdiCpNext
/// \brief       Cp interface in DDI level
/// \details     Interface functions are expected to be the main entry points
///              from each component(decoder,encoder & vp). The outside world use these functions to
///              obtain help for PAVP related topics.
///////////////////////////////////////////////////////////////////////////////
class DdiCpNext: public DdiCpInterfaceNext
{
public:
    DdiCpNext(MOS_CONTEXT& mosCtx);

    ~DdiCpNext();

    //------------------decoder related functions------------------------------
    ///////////////////////////////////////////////////////////////////////////
    //! \brief    set HostEncryptMode in EncryptionParams
    //! \param    [in] encryptionType
    //!           input type from outside
    ///////////////////////////////////////////////////////////////////////////
    void SetCpParams(uint32_t encryptionType, CodechalSetting *setting) override;

    ///////////////////////////////////////////////////////////////////////////
    //! \brief    set Encryption type and CryptoKeyExParams in EncryptionParams
    //! \param    [in] EncryptedDecryptKey
    //!           input type from outside
    ///////////////////////////////////////////////////////////////////////////
    void SetCryptoKeyExParams(uint32_t encryptionType, uint32_t *EncryptedDecryptKey);

    ///////////////////////////////////////////////////////////////////////////
    //! \brief    parse EncryptionParams for decrypt call in decoder
    //! \param    [in] decryptParams
    //!           input decryptParams to fill in
    //! \param    [in] params
    //!           input params for VADecryptInputBuffer
    //! \return   VA_STATUS
    //!           return VA_STATUS_SUCCESS if succeed, otherwise failed
    ///////////////////////////////////////////////////////////////////////////
    VAStatus ParseEncryptionParamsforDecrypt(void *params);

    ///////////////////////////////////////////////////////////////////////////
    //! \brief    parse EncryptionParams for decode call in decoder
    //! \param    [in] params
    //!           input cipher data to be parsed
    //! \return   VA_STATUS
    //!           return VA_STATUS_SUCCESS if succeed, otherwise failed
    ///////////////////////////////////////////////////////////////////////////
    VAStatus ParseEncryptionParamsforDecode(void *params);

    ///////////////////////////////////////////////////////////////////////////
    //! \brief    parse EncryptionParams for decode call in decoder
    //! \param    [in] params
    //!           input cipher data to be parsed
    //! \return   VA_STATUS
    //!           return VA_STATUS_SUCCESS if succeed, otherwise failed
    ///////////////////////////////////////////////////////////////////////////
    VAStatus ParseEncryptionParamsforHUCDecode(void *params);

    ///////////////////////////////////////////////////////////////////////////
    //! \brief    parse EncryptionParams for encode call in encoder
    //! \param    [in] params
    //!           input cipher data to be parsed
    //! \return   VA_STATUS
    //!           return VA_STATUS_SUCCESS if succeed, otherwise failed
    ///////////////////////////////////////////////////////////////////////////
    VAStatus ParseEncryptionParamsforEncode(void *params);

    ///////////////////////////////////////////////////////////////////////////
    //! \brief    End picture process for decrypt call
    //! \param    [in] vaDrvCtx
    //!           input context in vpgDecodeEndPictureDecrypt
    //! \param    [in] contextId
    //!           input va context id to get decode ctx
    //! \return   VAStatus
    //!           return VA_STATUS_SUCCESS if succeed, otherwise failed
    ///////////////////////////////////////////////////////////////////////////
    VAStatus EndPictureCenc(
        VADriverContextP vaDrvCtx,
        VAContextID      contextId);

    ///////////////////////////////////////////////////////////////////////////
    //! \brief    render picture process for decrypt call
    //! \param    [in] vaDrvCtx
    //!           input context in vpgDecodeEndPictureDecrypt
    //! \param    [in] contextId
    //!           input va context id to get decode ctx
    //! \param    [in] buf
    //!           input media buffer
    //! \param    [in] data
    //!           input data contain parameters
    //! \return   VA_STATUS
    //!           return VA_STATUS_SUCCESS if succeed, otherwise failed
    ///////////////////////////////////////////////////////////////////////////
    VAStatus RenderCencPicture(
        VADriverContextP  vaDrvctx,
        VAContextID       contextId,
        DDI_MEDIA_BUFFER *buf,
        void             *data) override;

    ///////////////////////////////////////////////////////////////////////////
    //! \brief    create buffer for CP related buffer types
    //! \param    [in] type
    //!           buffer type to be created 
    //! \param    [in] buffer
    //!           pointer to general buffer
    //! \param    [in] size
    //!           size of general buffer size
    //! \param    [in] num_elements
    //!           number of general buffer element
    //! \return   VA_STATUS
    //!           return VA_STATUS_SUCCESS if succeed, otherwise failed
    ///////////////////////////////////////////////////////////////////////////
    VAStatus CreateBuffer(
        VABufferType      type,
        DDI_MEDIA_BUFFER *buffer,
        uint32_t          size,
        uint32_t          num_elements) override;

    //------------------encoder related functions------------------------------
    ///////////////////////////////////////////////////////////////////////////
    //! \brief    Allocate and initialize HDCP2 buffer for encode status report
    //! \param    [in] bufMgr
    //!           input buffer manager to add hdcp2 buffer.
    //! \return   VAStatus
    //!           return VA_STATUS_SUCCESS if succeed, otherwise failed
    ///////////////////////////////////////////////////////////////////////////
    VAStatus InitHdcp2Buffer(DDI_CODEC_COM_BUFFER_MGR* bufMgr) override;

    ///////////////////////////////////////////////////////////////////////////
    //! \brief    fill the HDCP buffer based on status report
    //! \param    [in] bufMgr
    //!           input BufMgr to fill in.
    //! \param    [in] info
    //!           input info to get status report info.
    //! \return   VAStatus
    //!           return VA_STATUS_SUCCESS if succeed, otherwise failed
    ///////////////////////////////////////////////////////////////////////////
    VAStatus StatusReportForHdcp2Buffer(
        DDI_CODEC_COM_BUFFER_MGR *bufMgr,
        void                     *encodeStatusReport) override;

    ///////////////////////////////////////////////////////////////////////////
    //! \brief    Allocate and initialize HDCP2 buffer for encode status report
    //! \param    [in] bufMgr
    //!           status report buffer manager to be added.
    //! \return   void
    ///////////////////////////////////////////////////////////////////////////
    void FreeHdcp2Buffer(DDI_CODEC_COM_BUFFER_MGR* bufMgr) override;

    ///////////////////////////////////////////////////////////////////////////
    //! \brief    store AES counter in status report so App can get correct values by the order in which they are added
    //! \param    [in] info
    //!           input DDI_ENCODE_STATUS_REPORT_INFO to be writed
    //! \return   MOS_STATUS
    //!           return MOS_STATUS_SUCCESS if succeed, otherwise failed
    ///////////////////////////////////////////////////////////////////////////
    MOS_STATUS StoreCounterToStatusReport(
        encode::PDDI_ENCODE_STATUS_REPORT_INFO info) override;

    ///////////////////////////////////////////////////////////////////////////
    //! \brief    to detect if HDCP teardown happened
    //! \detail   When we are doing secure encoding, but the PAVP session gets destroyed or recreated,
    //!           then HDCP2 auto tear down happened
    //! \return   bool
    //!           return true if tear down happened, otherwise false
    ///////////////////////////////////////////////////////////////////////////
    bool IsHdcpTearDown();

    ///////////////////////////////////////////////////////////////////////////
    //! \brief    parse EncryptionParams for encoder
    //! \return   VAStatus
    //!           return VA_STATUS_SUCCESS if succeed, otherwise failed
    ///////////////////////////////////////////////////////////////////////////
    VAStatus ParseCpParamsForEncode() override;

    ///////////////////////////////////////////////////////////////////////////
    //! \brief    set CP flags for encoder
    //! \param    [in] enabled
    //!           Hdcp2Enabled and PAVPEnabled to be set to enabled or not
    ///////////////////////////////////////////////////////////////////////////
    void SetCpFlags(int32_t flag) override;

    ///////////////////////////////////////////////////////////////////////////
    //! \brief    Is Hdcp2Enabled for encoder
    //! \return   [in] bool
    //!           true if hdcp2 enabled, otherwise false
    ///////////////////////////////////////////////////////////////////////////
    bool IsHdcp2Enabled() override;

    ///////////////////////////////////////////////////////////////////////////
    //! \brief    Set cp tag for an input resource
    //! \param    [in] osInterface
    //!           pointer of mos interface
    //! \param    [in] resource
    //!           pointer of the os resource
    //! \return
    ///////////////////////////////////////////////////////////////////////////
    void SetInputResourceEncryption(PMOS_INTERFACE osInterface, PMOS_RESOURCE resource) override;

    ///////////////////////////////////////////////////////////////////////////
    //! \brief    Create Cenc Decode 
    //! \detail   none
    //! \param    [in] decoder 
    //!           Pointer to class CodechalDecode
    //! \param    [in] osContext 
    //!           Pointer to class PMOS_CONTEXT
    //! \param    [in] setting 
    //!           Pointer to class CodechalSetting
    //! \return   none 
    ///////////////////////////////////////////////////////////////////////////
    VAStatus CreateCencDecode(
        CodechalDebugInterface *debugInterface,
        PMOS_CONTEXT            osContext,
        CodechalSetting        *settings) override;

    ///////////////////////////////////////////////////////////////////////////
    //! \brief    Sets Decode Parameter
    //! \detail   none
    //! \param    [in] decodeParams
    //!           Pointer to class CodechalDecodeParams
    //! \return   VAStatus
    //!           return VA_STATUS_SUCCESS if succeed, otherwise failed
    ///////////////////////////////////////////////////////////////////////////
    VAStatus SetDecodeParams(
        decode::DDI_DECODE_CONTEXT *ddiDecodeContext,
        CodechalSetting    *setting) override;

    VAStatus EndPicture(
        VADriverContextP ctx,
        VAContextID      context) override;

    VAStatus IsAttachedSessionAlive() override;

    bool IsCencProcessing() override;

    void UpdateCpContext(uint32_t value, DDI_CP_CONFIG_ATTR &cpConfigAttr);

    void UpdateCpContext(DdiCpInterfaceNext *cpInterface);

    void ResetCpContext(void);

    bool IsAttached();

    void SetAttached(CPavpCryptoSessionVANext *cpSession);

    //------------------private internal variables and functions------------------------------
private:
    CpContext                         m_ddiCpContext;
    bool                              m_isCencProcessing;
    bool                              m_isSecureDecoderEnabled;
    CPavpCryptoSessionVANext         *m_attachedCpSession;

    MHW_PAVP_ENCRYPTION_PARAMS        m_encryptionParams[NUM_ENCRYPTION_PARAMS_INDEX] = {};      //!//!< [0] is for encode and legacy decode, [1] is for cencDecode

    CodechalCencDecodeNext           *m_cencDecoder = nullptr;

    CODECHAL_CENC_PARAMS              m_cencParams;
    uint32_t                          m_numSegmentsInBuffer;
    std::vector<MHW_SUB_SAMPLE_MAPPING_BLOCK> m_subSampleMappingBlocks;

    //These variables and code related to them will be revisited once HDCP interfaces are defined
    //https://jira.devtools.intel.com/browse/VSMGWL-28074
    //encoder related variables
    uint64_t                          rIV;
    uint32_t                          streamCtr;
    bool                              isHDCP2Enabled;
    bool                              isPAVPEnabled;
    bool                              ecbInputEnabled;
    uint32_t                          cpInstanceCount;
    
MEDIA_CLASS_DEFINE_END(DdiCpNext)
};

#endif
