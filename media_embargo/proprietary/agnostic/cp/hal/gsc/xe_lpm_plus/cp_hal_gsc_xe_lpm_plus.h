/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2020-2023
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

File Name: cp_hal_gsc_xe_lpm_plus.h

Abstract:
class for GSC HAL for PAVP functions

Environment:
OS agnostic - should support - Wihdows, Linux, Android
Platform: Should support from Gen13 onwards

Notes:
Due to open source restrictions, GSC(Graphics Security Controller)
has been replaced with RTE(Root Trust Engine) in many files

======================= end_copyright_notice ==================================*/

#ifndef __CP_HAL_GSC_G13_H__
#define __CP_HAL_GSC_G13_H__

#include "cp_hal_gsc.h"
#include "mhw_utilities.h"
#include "media_interfaces_mhw_next.h"

#define GSC_MAX_INPUT_BUFF_SIZE_XE_LPM_PLUS          65 * 1024
#define GSC_MAX_OUTPUT_BUFF_SIZE_XE_LPM_PLUS         65 * 1024

#define WIDI_ME_FIX_CLIENT              18
#define PAVP_ME_FIX_CLIENT              17
#define MKHI_ME_FIX_CLIENT              7
#define HECI_VALIDITY_MARKER            0xA578875A
#define SW_PROXY_POLLING_INTERVAL       50
#define SW_PROXY_MAX_RETRY_POLLING      20

class CPavpGscHalG13 : public CPavpGscHal
{
public:

    CPavpGscHalG13(RootTrust_HAL_USE gscHalUse = RootTrust_HAL_PAVP_USE,
                    std::shared_ptr<CPavpOsServices> pOsServices = nullptr,
                    bool bEPIDProvisioningMode = true);

    virtual ~CPavpGscHalG13();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Initializes the GSC HAL.
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT Init() override;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Cleans up the GSC HAL.
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT Cleanup() override;

protected:
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Sets all member variables to a default value.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    void ResetMemberVariables() override;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Build and submit GSC Command Buffer
    /// \param      pInputBuffer           [in] Input buffer
    /// \param      InputBufferLength      [in] Size of input buffer
    /// \param      pOutputBuffer          [out] Output buffer
    /// \param      OutputBufferLength     [in] Size of output buffer
    /// \param      pIgdData               [out] IGD data buffer (can be NULL if the FW command does not use IGD data).
    /// \param      pIgdDataLen            [out] Pointer to uint32_t which will contain the size of the resulted IGD data buffer
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT BuildSubmitGSCCommandBuffer(
        uint8_t     *pInputBuffer,
        uint32_t    InputBufferLength,
        uint8_t     *pOutputBuffer,
        uint32_t    OutputBufferLength,
        uint8_t     *pIgdData,
        uint32_t    *pIgdDataLen,
        uint8_t     vTag) override;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Build GSC Command Buffer
    /// \param      pCmdBuffer             [in] Pointer to GSC input output command buffer
    /// \param      InputBufferLength      [in] Size of input buffer
    /// \param      OutputBufferLength     [in] Size of output buffer
    ///
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT BuildCommandBuffer(
        PMOS_COMMAND_BUFFER pCmdBuffer,
        uint32_t              InputBufferLength,
        uint32_t              OutputBufferLength);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Read outbuffer from MOS resource sent by GSC via KMD
    /// \param      pOutputBuffer          [out] Output buffer
    /// \param      pIgdData               [out] IGD data buffer (can be NULL if the FW command does not use IGD data).
    /// \param      pIgdDataLen            [out] Pointer to uint32_t which will contain the size of the resulted
    ///                                          IGD data buffer
    /// \param       RecvBytes             [in] Number of bytes sent by FW
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT ReadResourceOutputBuffer(
        uint8_t     *pOutputBuffer,
        uint32_t    OutputBufferLength,
        uint8_t     *pIgdData,
        uint32_t    *pIgdDataLen,
        uint32_t    RecvBytes = 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Write inputbuffer into MOS resource that shall be sent o GSC via
    /// \param      pInputBuffer           [in] Input buffer
    /// \param      InputBufferLength      [in] Size of input buffer
    /// \param      HeciClient             [in] Indicate which heci client to use
    /// \param      pExtensionBuffer       [in] buffer containing extension
    /// \param      extensionSize          [in] size of extension
    ///
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT WriteResourceInputBuffer(
        uint8_t     *pInputBuffer,
        uint32_t    InputBufferLength,
        uint8_t     HeciClient,
        uint8_t     *pExtensionBuffer,
        uint32_t    extensionSize);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Allocat MOS resource that will be use to submit input and recevie output from GSC
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT AllocateGSCResource();

protected:
    MOS_RESOURCE                        m_GSCInputBuffer;
    MOS_RESOURCE                        m_GSCOutputBuffer;
    GSC_CS_MEMORY_HEADER                m_GSCCSMemoryhdr;
    MhwInterfacesNext                   *m_pMhwInterfaces;
    UINT64                              m_GSCHostSessionHandle;
    UINT32                              m_maxInputBufferSize;
    UINT32                              m_maxOutputBufferSize;

    MEDIA_CLASS_DEFINE_END(CPavpGscHalG13)
};

#pragma pack()

#endif // __CP_HAL_GSC_G13_H_
