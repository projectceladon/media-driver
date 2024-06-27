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
//! \file     media_libva_cp.h
//! \brief    DDI interface for content protection parameters and operations.
//! \details  Other components will call this interface for content protection operations directly
//!

#ifndef  __MEDIA_LIBVA_CP_H__
#define  __MEDIA_LIBVA_CP_H__
#include "media_libva.h"
#include "va_private.h"
#include "codechal_cenc_decode.h"
#include "media_libva_cp_interface.h"
#include "mos_os.h"
#include "mos_utilities_common.h"
#include "mos_os_cp_specific.h"
#include "ddi_cp_utils.h"

#define CP_DDI_CHK_NULL_RETURN(_ptr)                                                  \
    MOS_CHK_NULL_RETURN(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_DDI, _ptr)

#define CP_DDI_CHK_STATUS_RETURN(_stmt)                                               \
    MOS_CHK_STATUS_RETURN(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_DDI, _stmt)

typedef struct _DDI_ENCODE_STATUS_REPORT_INFO *PDDI_ENCODE_STATUS_REPORT_INFO;

class CPavpCryptoSessionVA;

///////////////////////////////////////////////////////////////////////////////
/// \class       DdiCp
/// \brief       Cp interface in DDI level
/// \details     Interface functions are expected to be the main entry points
///              from each component(decoder,encoder & vp). The outside world use these functions to
///              obtain help for PAVP related topics.
///////////////////////////////////////////////////////////////////////////////
class DdiCp: public DdiCpInterface
{
public:
    DdiCp(MOS_CONTEXT& mosCtx);

    ~DdiCp();

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
        VADriverContextP      vaDrvctx,
        VAContextID           contextId,
        DDI_MEDIA_BUFFER      *buf,
        void                  *data) override;

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
        VABufferType             type,
        DDI_MEDIA_BUFFER*        buffer,
        uint32_t                 size,
        uint32_t                 num_elements) override;

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
        DDI_CODEC_COM_BUFFER_MGR*       bufMgr,
        void*              encodeStatusReport) override;

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
        PDDI_ENCODE_STATUS_REPORT_INFO info) override;

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
        CodechalDebugInterface      *debugInterface,
        PMOS_CONTEXT                osContext,
        CodechalSetting             *settings) override;

    ///////////////////////////////////////////////////////////////////////////
    //! \brief    Sets Decode Parameter
    //! \detail   none
    //! \param    [in] decodeParams
    //!           Pointer to class CodechalDecodeParams
    //! \return   VAStatus
    //!           return VA_STATUS_SUCCESS if succeed, otherwise failed
    ///////////////////////////////////////////////////////////////////////////
    VAStatus SetDecodeParams(
        DDI_DECODE_CONTEXT *ddiDecodeContext,
        CodechalSetting *setting) override;

    VAStatus EndPicture(
        VADriverContextP    ctx,
        VAContextID         context
    ) override;

    VAStatus IsAttachedSessionAlive() override;

    bool IsCencProcessing() override;

    void UpdateCpContext(uint32_t value, DDI_CP_CONFIG_ATTR &cpConfigAttr);

    void UpdateCpContext(DdiCpInterface *cpInterface);

    void ResetCpContext(void);

    bool IsAttached();

    void SetAttached(CPavpCryptoSessionVA *cpSession);

    //------------------private internal variables and functions------------------------------
private:
    CpContext                         m_ddiCpContext;
    bool                              m_isCencProcessing;
    bool                              m_isSecureDecoderEnabled;
    CPavpCryptoSessionVA              *m_attachedCpSession;

    MHW_PAVP_ENCRYPTION_PARAMS        m_encryptionParams[NUM_ENCRYPTION_PARAMS_INDEX] = {};      //!//!< [0] is for encode and legacy decode, [1] is for cencDecode

    CodechalCencDecode                *m_cencDecoder      = nullptr;

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
};
extern "C" DdiCpInterface* Create_DdiCp(MOS_CONTEXT* pMosCtx);
extern "C" void Delete_DdiCp(DdiCpInterface* pDdiCpInterface);
#endif