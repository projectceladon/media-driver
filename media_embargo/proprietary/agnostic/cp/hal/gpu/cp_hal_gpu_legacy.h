/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2023
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

File Name: cp_hal_gpu_legacy.h

Abstract:
Abstract base class for GPU HAL for PAVP functions

Environment:
OS agnostic - should support - Windows, Linux
Platform should support - Gen8, Gen9, Gen11, Gen12

Notes:
None

======================= end_copyright_notice ==================================*/
#ifndef __CP_HAL_GPU_LEGACY_H__
#define __CP_HAL_GPU_LEGACY_H__

#include "cp_hal_gpu.h"
#include "media_interfaces_mhw.h"

struct _PAVP_MHW_INTERFACE_
{
    MhwMiInterface *pMiInterface;
    MhwCpInterface *pPavpHwInterface;
};

/// \class CPavpGpuHalLegacy
/// \brief GPU HAL base abstract class
class CPavpGpuHalLegacy: public CPavpGpuHal
{
public:
    CPavpGpuHalLegacy(PLATFORM &sPlatform, MEDIA_WA_TABLE &sWaTable, HRESULT &hr, CPavpOsServices &OsServices);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Builds the key exchange functionality by appending specific HW commands to the command buffer.
    ///
    /// \param pCmdBuf                  [in] The mos command buffer ptr to append  the command
    /// \param pBuildKeyExchangeParams  [in] The key exchange parameteres
    /// \return HRESULT
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT BuildKeyExchange(MOS_COMMAND_BUFFER *pCmdBuf, const PAVP_GPU_HAL_KEY_EXCHANGE_PARAMS &sBuildKeyExchangeParams);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Builds multiple crypto copy functionality by appending specific HW commands to the command buffer.
    ///
    /// \param pCmdBuf                       [in] The mos command buffer ptr to append the command
    /// \param pBuildMultiCryptoCopyParams   [in] Point to the multiple crypto copy parameteres
    /// \param cryptoCopyNumber              [in] The Number of multiple crypto copy parameteres
    /// \return int32_t
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual int32_t BuildMultiCryptoCopy(
        MOS_COMMAND_BUFFER                    *pCmdBuf,
        PAVP_GPU_HAL_MULTI_CRYPTO_COPY_PARAMS *pBuildMultiCryptoCopyParams,
        size_t                                 cryptoCopyNumber);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Two steps operation (1) clear=>serpent and (2) serpent=>AES-counter to replace BSD_CRYPTO_COPY mode 0b0110 (clear to AES-counter)
    /// \param       pCmdBuf                [in] Pointer to command buffer
    /// \param       sBuildCryptoCopyParams [in] Params for crypto copy
    /// \param       bWaitForCompletion     [in] If need to wait hardware CryptoCopy to complete: true:synchronous; false:asynchronous

    /// \return      HRESULT
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT BuildCryptoCopyClearToAesCounter(MOS_COMMAND_BUFFER *pCmdBuf, const PAVP_GPU_HAL_CRYPTO_COPY_PARAMS &sBuildCryptoCopyParams, bool bWaitForCompletion);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Builds the crypto copy functionality by appending specific HW commands to the command buffer.
    ///
    /// \param pCmdBuf                  [in] The mos command buffer ptr to append  the command
    /// \param sBuildCryptoCopyParams   [in] The crypto copy parameteres
    /// \return HRESULT
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT BuildCryptoCopy(MOS_COMMAND_BUFFER *pCmdBuf, const PAVP_GPU_HAL_CRYPTO_COPY_PARAMS &sBuildCryptoCopyParams);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Builds the query status functionality by appending specific HW commands to the command buffer.
    ///
    /// \param pCmdBuf                  [in] The mos command buffer ptr to append  the command
    /// \param sBuildQueryStatusParams  [in] The crypto copy parameteres
    /// \return HRESULT
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT BuildQueryStatus(MOS_COMMAND_BUFFER *pCmdBuf, PAVP_GPU_HAL_QUERY_STATUS_PARAMS &sBuildQueryStatusParams);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Gets encrypted video processor kernels to be authenticated then transcrypted
    /// \param       bPreProduction             [in]  whether platform is Pre-production
    /// \param       pTranscryptMetaData        [out] Pointer to transcrypt metadata structure
    /// \param       uiTranscryptMetaDataSize   [out] Size in bytes of transcrypt metadata structure
    /// \param       pEncryptedKernels          [out] Pointer to encrypted kernels
    /// \param       uiEncryptedKernelsSize     [out] Size in bytes of encrypted kernels
    /// \returns     S_OK on success, a failed HRESULT otherwise
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT GetInfoToTranscryptKernels(
        const bool     bPreProduction,
        const uint32_t *&pTranscryptMetaData,
        uint32_t       &uiTranscryptMetaDataSize,
        const uint32_t *&pEncryptedKernels,
        uint32_t       &uiEncryptedKernelsSize);

    virtual ~CPavpGpuHalLegacy();

protected:
    MhwInterfaces           *m_pMhwInterfacesLegacy;
    _PAVP_MHW_INTERFACE_    m_HwInterface;

    MEDIA_CLASS_DEFINE_END(CPavpGpuHalLegacy)
};
#endif // __CP_HAL_GPU_LEGACY_H__

