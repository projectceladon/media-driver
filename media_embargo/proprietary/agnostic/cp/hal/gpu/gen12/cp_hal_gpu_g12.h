/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2013-2023
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

File Name: cp_hal_gpu_g12.h

Abstract:
Header for HAL layer for PAVP functions prototypes

Environment:
OS agnostic - should support - Windows 10, Linux, Android
Platform specific - should support - Gen 12

Notes:
None

======================= end_copyright_notice ==================================*/

#ifndef __CP_HAL_GPU_G12_H__
#define __CP_HAL_GPU_G12_H__

#include "cp_hal_gpu_legacy.h"
#include <array>

#define PAVP_GEN11_MAX_SESSION_RESERVE_RETRYS            3
#define PAVP_GEN11_KCR_APP_CHAR_KEY_REFRESH_MODE_BIT_MASK     0x800000
#define PAVP_SHIFT_UINT32_TO_UINT64             32
#define PAVP_NUMBER_OF_BITS_IN_AES128_CTR       64
#define PAVP_GEN12_FIRST_RESERVED_COUNTER_BIT   60
#define PAVP_GEN12_LAST_RESERVED_COUNTER_BIT    63

/// \class CPavpGpuHal12
/// \brief HAL layer for PAVP functions prototypes for Gen 12
class CPavpGpuHal12 : public CPavpGpuHalLegacy
{
public:

    CPavpGpuHal12(PLATFORM& sPlatform, MEDIA_WA_TABLE& sWaTable, HRESULT& hr, CPavpOsServices& OsServices);

    //////////////////////////////////////////////////////////////////////////////////////

    virtual HRESULT BuildKeyExchange(MOS_COMMAND_BUFFER *pCmdBuf, const PAVP_GPU_HAL_KEY_EXCHANGE_PARAMS& sBuildKeyExchangeParams);

    virtual int32_t BuildMultiCryptoCopy(
        MOS_COMMAND_BUFFER                           *pCmdBuf,
        PAVP_GPU_HAL_MULTI_CRYPTO_COPY_PARAMS        *pBuildMultiCryptoCopyParams,
        size_t                                       cryptoCopyNumber);

    virtual bool CheckAudioCryptoCopyCounterRollover(
        PAVP_GPU_HAL_MULTI_CRYPTO_COPY_PARAMS sCryptoCopyParam,
        uint32_t                              &numBytesBeforeRollover,
        uint32_t                              &remainingSize);

    virtual HRESULT BuildCryptoCopy(MOS_COMMAND_BUFFER *pCmdBuf, const PAVP_GPU_HAL_CRYPTO_COPY_PARAMS& sBuildCryptoCopyParams);

    virtual HRESULT BuildQueryStatus(MOS_COMMAND_BUFFER *pCmdBuf, PAVP_GPU_HAL_QUERY_STATUS_PARAMS& sBuildQueryStatusParams);

    virtual HRESULT SetGpuCryptoKeyExParams(PMHW_PAVP_ENCRYPTION_PARAMS pMhwPavpEncParams);

    HRESULT SetKeyRefreshType();

private:

    BOOL CheckCryptoCopyCounterRollover(
        uint32_t DataLength,
        uint32_t CounterLow,
        uint32_t CounterHigh,
        std::array<bool, 4>& OverflowBits);

    MHW_PAVP_MFX_CRYPTO_KEY_EXCHANGE_CMD_PARAMS m_CryptoKeyExParams;
    BOOL                                        m_bIsKbKeyRefresh;

    MEDIA_CLASS_DEFINE_END(CPavpGpuHal12)
};

#endif // __CP_HAL_GPU_G12_H__
