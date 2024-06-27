/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2019-2023
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

File Name: cp_hal_gsc_g12.h

Abstract:
class for GSC HAL for PAVP functions

Environment:
OS agnostic - should support - Wihdows, Linux, Android
Platform: Should support from DashG-1 onwards

Notes:
Due to open source restrictions, GSC(Graphics Security Controller)
has been replaced with RTE(Root Trust Engine) in many files

======================= end_copyright_notice ==================================*/

#ifndef __CP_HAL_GSC_G12_H__
#define __CP_HAL_GSC_G12_H__

#include "cp_hal_gsc.h"

class CPavpOsServices;

class CPavpGscHalG12 : public CPavpGscHal
{
public:

    CPavpGscHalG12(RootTrust_HAL_USE gscHalUse = RootTrust_HAL_PAVP_USE,
                    std::shared_ptr<CPavpOsServices> pOsServices = nullptr,
                    bool bEPIDProvisioningMode = true);

    virtual ~CPavpGscHalG12();

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
    /// \param      pOutputBuffer          [out] Output buffer
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT WriteResourceInputBuffer(
        uint8_t       *pInputBuffer,
        uint32_t      InputBufferLength);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Allocat MOS resource that will be use to submit input and recevie output from GSC
    /// \returns    S_OK on success, failed HRESULT otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT AllocateGSCResource();

protected:
    uint8_t                               *m_pGSCInputOutput;     // Reference to resource to send and receive data from GSC FW
    uint32_t                              m_BufferSize;           // GSC Buffer size

    MEDIA_CLASS_DEFINE_END(CPavpGscHalG12)
};

#endif // __CP_HAL_GSC_G12_H_
