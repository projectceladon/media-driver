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

File Name: cp_hal_gpu.h

Abstract:
Abstract base class for GPU HAL for PAVP functions

Environment:
OS agnostic - should support - Windows Vista, Windows 7 (D3D9), 8 (D3D11.1), Linux, Android
Platform Agnostic - should support - Gen 6, Gen 7, Gen 7.5

Notes:
None

======================= end_copyright_notice ==================================*/
#ifndef __CP_HAL_GPU_H__
#define __CP_HAL_GPU_H__

#include "intel_pavp_core_api.h"
#include "cp_debug.h"
#include "cp_hal_gpu_defs.h"
#include "cp_factory.h"
#include "pavp_types.h"
#include "media_interfaces_mhw_next.h"

typedef enum _EPID_CERT_TYPE
{
    EPID_CERT_TYPE_DEFAULT = 0,
    EPID_CERT_TYPE_PREPRODUCTION = 0x000004DC,
    EPID_CERT_TYPE_PREPRODUCTION_KBL_R = 0x00002710
}EPID_CERT_TYPE;

// Define to remove debug or release internal code that significantly changes timing vs. release build.
// This wraps (_DEBUG || RELEASE-INTERNAL), keep this set to 0 for checkins.
#define EMULATE_CP_RELEASE_TIMING_TEST 0

#define GPU_HAL_FUNCTION_ENTER    CP_FUNCTION_ENTER(MOS_CP_SUBCOMP_GPU_HAL)
#define GPU_HAL_FUNCTION_EXIT(hr) CP_FUNCTION_EXIT(MOS_CP_SUBCOMP_GPU_HAL, hr)

#define GPU_HAL_ASSERTMESSAGE(_message, ...) \
    CP_ASSERTMESSAGE(MOS_CP_SUBCOMP_GPU_HAL, _message, ##__VA_ARGS__)

#define GPU_HAL_VERBOSEMESSAGE(_message, ...) \
    CP_VERBOSEMESSAGE(MOS_CP_SUBCOMP_GPU_HAL, _message, ##__VA_ARGS__)

#define GPU_HAL_NORMALMESSAGE(_message, ...) \
    CP_NORMALMESSAGE(MOS_CP_SUBCOMP_GPU_HAL, _message, ##__VA_ARGS__)

#define GPU_HAL_CHK_STATUS_WITH_HR(_stmt)            \
    CP_CHK_STATUS_WITH_HR(MOS_CP_SUBCOMP_GPU_HAL, _stmt)

#define GPU_HAL_ASSERT(_expr) \
    CP_ASSERT(MOS_CP_SUBCOMP_GPU_HAL, _expr)

#define GPU_HAL_CHK_NULL(_ptr) \
    CP_CHK_NULL(MOS_CP_SUBCOMP_GPU_HAL, _ptr)

#define CP_GPU_HAL_CHK_NULL_WITH_HR(_ptr) \
    MOS_CHK_NULL_WITH_HR_RETURN(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_GPU_HAL, _ptr)

#define CP_GPU_HAL_CHK_STATUS_WITH_HR(_stmt) \
    CP_CHK_STATUS_WITH_HR_RETURN(MOS_CP_SUBCOMP_GPU_HAL, _stmt)

#define CP_GPU_HAL_CHK_STATUS_WITH_HR_BREAK(_stmt) \
    CP_CHK_STATUS_WITH_HR_BREAK(MOS_CP_SUBCOMP_GPU_HAL, _stmt)

// PXP-related MMIO register offsets.
// The offsets may or may not change between generations.
// Consult the PXP b-spec for more information.
#define KM_PAVP_GEN7_REG_APPID                              0x320FC
#define KM_PAVP_GEN7_REG_SESSION_IN_PLAY                    0x32260
#define KM_PAVP_GEN7_REG_STATUS                             0x320E8
#define KM_PAVP_GEN7_REG_DEBUG                              0x320F4
#define KM_GEN10_ISOLATED_DECODE_SESSION_IN_PLAY            0x32670
#define KM_PAVP_GEN7_REG_EXPLICIT_TERMINATE                 0x31034
#define KM_PAVP_GEN11_REG_KCR_INIT                          0x320F0
#define KM_PAVP_GEN11_REG_KCR_STATUS_1                      0x320F4
#define KM_PAVP_GEN11_REG_KCR_STATUS_2                      0x320E8
#define KM_PAVP_GEN11_REG_KCR_SESSION_IN_PLAY               0x32260
#define KM_PAVP_GEN11_REG_KCR_DISPLAY_APP_CHAR_REG_BASE     0x32F00
#define KM_PAVP_GEN11_REG_WIDI0_KCR_COUNTER_1               0x32860
#define KM_PAVP_GEN11_REG_WIDI0_KCR_COUNTER_2               0x32864
#define KM_PAVP_GEN11_REG_WIDI0_KCR_COUNTER_3               0x32868
#define KM_PAVP_GEN11_REG_WIDI0_KCR_COUNTER_4               0x3286C
#define KM_PAVP_GEN11_REG_WIDI1_KCR_COUNTER_1               0x32870
#define KM_PAVP_GEN11_REG_WIDI1_KCR_COUNTER_2               0x32874
#define KM_PAVP_GEN11_REG_WIDI1_KCR_COUNTER_3               0x32878
#define KM_PAVP_GEN11_REG_WIDI1_KCR_COUNTER_4               0x3287C
#define KM_PAVP_GEN12_REG_TRANSCODE_SESSION_IN_PLAY_1       0x32264
#define KM_PAVP_GEN12_REG_TRANSCODE_SESSION_IN_PLAY_2       0x32268
#define KM_PAVP_GEN12_REG_TRANSCODE_STOUT_STATUS_1          0x32674
#define KM_PAVP_GEN12_REG_TRANSCODE_STOUT_STATUS_2          0x32678

#define KM_PAVP_GEN11_REG_KCR_DISPLAY_APP_CHAR_12           0x32830
#define KM_PAVP_GEN11_REG_KCR_TRANSCODE_APP_CHAR_REG_BASE   0x32F40
#define KM_PAVP_GEN11_REG_MSG_PAVP_OPERATION                0x320FC

#define KM_PAVP_GLOBAL_TERMINATE                            0x320f8
#define PAVPC_MMIO_GT_OFFSET                                0x1082C0

#if ((_DEBUG || _RELEASE_INTERNAL))
#define KM_PAVP_GEN11_REG_MSG_REQ_GFX_KEY                   0x31030
#define KM_PAVP_GEN11_REG_MSG_DELIVER_GFX_KEY_1             0x129010
#define KM_PAVP_GEN11_REG_MSG_DELIVER_GFX_KEY_2             0x129014
#define KM_PAVP_GEN11_REG_MSG_DELIVER_GFX_KEY_3             0x129018
#define KM_PAVP_GEN11_REG_MSG_DELIVER_GFX_KEY_4             0x12901C
#define KM_PAVP_GEN11_REG_MSG_CHECK_GFX_KEY_1               0x31000
#define KM_PAVP_GEN11_REG_MSG_CHECK_GFX_KEY_2               0x31004
#define KM_PAVP_GEN11_REG_MSG_CHECK_GFX_KEY_3               0x31008
#define KM_PAVP_GEN11_REG_MSG_CHECK_GFX_KEY_4               0x3100C
#define KM_PAVP_GEN11_REG_MSG_CHECK_GFX_KEY_STATUS           0x12900C
#define KM_PAVP_GEN11_REG_SEC_PAVP_CHARACTERISTIC_1         0x323A0
#define KM_PAVP_GEN11_REG_SEC_PAVP_CHARACTERISTIC_2         0x323A4
#define KM_PAVP_GEN11_REG_SEC_PAVP_CHARACTERISTIC_3         0x323A8
#define KM_PAVP_GEN11_REG_SEC_PAVP_CHARACTERISTIC_4         0x323AC
#define KM_PAVP_GEN11_REG_SEC_PAVP_KEY_1                    0x323B0
#define KM_PAVP_GEN11_REG_SEC_PAVP_KEY_2                    0x323B4
#define KM_PAVP_GEN11_REG_SEC_PAVP_KEY_3                    0x323B8
#define KM_PAVP_GEN11_REG_SEC_PAVP_KEY_4                    0x323BC
#define KM_PAVP_GEN11_REG_MSG_DELIVER_SESSION_KEY_1         0x32100
#define KM_PAVP_GEN11_REG_MSG_DELIVER_SESSION_KEY_2         0x32104
#define KM_PAVP_GEN11_REG_MSG_DELIVER_SESSION_KEY_3         0x32108
#define KM_PAVP_GEN11_REG_MSG_DELIVER_SESSION_KEY_4         0x3210C
#define KM_PAVP_GEN11_REG_MSG_DELIVER_WIDI_KEY_1            0x32130
#define KM_PAVP_GEN11_REG_MSG_DELIVER_WIDI_KEY_2            0x32134
#define KM_PAVP_GEN11_REG_MSG_DELIVER_WIDI_KEY_3            0x32138
#define KM_PAVP_GEN11_REG_MSG_DELIVER_WIDI_KEY_4            0x3213C
#define KM_PAVP_GEN11_REG_MSG_PAVP_OPERATION_DONE           0x129040
#define KM_PAVP_GEN11_REG_SEND_MSG_HW_STATUS_1              0x129020
#define KM_PAVP_GEN11_REG_SEND_MSG_HW_STATUS_2              0x129024
#define KM_PAVP_GEN11_REG_SEND_MSG_HW_STATUS_3              0x129028
#define KM_PAVP_GEN11_REG_SEND_MSG_HW_STATUS_4              0x12902C
#endif //((_DEBUG || _RELEASE_INTERNAL))

#define PAVP_TYPE_FROM_ID(x)                        ((x & PAVP_SESSION_TYPE_MASK) >> 7)
#define PAVP_INDEX_FROM_ID(x)                       (x & PAVP_SESSION_INDEX_MASK)
#define PAVP_SESSION_TYPE_MASK                      0x80
#define PAVP_SESSION_INDEX_MASK                     0x7f
#define PAVP_GEN11_KCR_APP_ID_12                    12
#define PAVP_GEN11_KCR_APP_CHAR_OFFSET              4

class MhwCpInterface;
class CPavpOsServices;

/// \class CPavpGpuHal
/// \brief GPU HAL base abstract class
class CPavpGpuHal
{
public:
    static DWORD ConvertRegType2RegOffset(PAVP_GPU_REGISTER_TYPE eRegType, DWORD Index = 0)
    {
        DWORD RegOffset = INVALID_REG_OFFSET;
        switch (eRegType)
        {
            case PAVP_REGISTER_APPID:
                RegOffset = KM_PAVP_GEN7_REG_APPID;
                break;

            case PAVP_REGISTER_SESSION_IN_PLAY:
                RegOffset = KM_PAVP_GEN7_REG_SESSION_IN_PLAY;
                break;

            case PAVP_REGISTER_TRANSCODE_SESSION_IN_PLAY_1:
                RegOffset = KM_PAVP_GEN12_REG_TRANSCODE_SESSION_IN_PLAY_1;
                break;

            case PAVP_REGISTER_TRANSCODE_SESSION_IN_PLAY_2:
                RegOffset = KM_PAVP_GEN12_REG_TRANSCODE_SESSION_IN_PLAY_2;
                break;

            case PAVP_REGISTER_TRANSCODE_STOUT_STATUS_1:
                RegOffset = KM_PAVP_GEN12_REG_TRANSCODE_STOUT_STATUS_1;
                break;

            case PAVP_REGISTER_TRANSCODE_STOUT_STATUS_2:
                RegOffset = KM_PAVP_GEN12_REG_TRANSCODE_STOUT_STATUS_2;
                break;

            case PAVP_REGISTER_PAVPC:
                RegOffset = PAVPC_MMIO_GT_OFFSET;
                break;

            case PAVP_REGISTER_KCR_SESSION_IN_PLAY:
                RegOffset = KM_PAVP_GEN11_REG_KCR_SESSION_IN_PLAY;
                break;

            case PAVP_REGISTER_WIDI0_KCR_COUNTER_DW1:
                RegOffset = KM_PAVP_GEN11_REG_WIDI0_KCR_COUNTER_1;
                break;

            case PAVP_REGISTER_WIDI0_KCR_COUNTER_DW2:
                RegOffset = KM_PAVP_GEN11_REG_WIDI0_KCR_COUNTER_2;
                break;

            case PAVP_REGISTER_WIDI0_KCR_COUNTER_DW3:
                RegOffset = KM_PAVP_GEN11_REG_WIDI0_KCR_COUNTER_3;
                break;

            case PAVP_REGISTER_WIDI0_KCR_COUNTER_DW4:
                RegOffset = KM_PAVP_GEN11_REG_WIDI0_KCR_COUNTER_4;
                break;

            case PAVP_REGISTER_WIDI1_KCR_COUNTER_DW1:
                RegOffset = KM_PAVP_GEN11_REG_WIDI1_KCR_COUNTER_1;
                break;

            case PAVP_REGISTER_WIDI1_KCR_COUNTER_DW2:
                RegOffset = KM_PAVP_GEN11_REG_WIDI1_KCR_COUNTER_2;
                break;

            case PAVP_REGISTER_WIDI1_KCR_COUNTER_DW3:
                RegOffset = KM_PAVP_GEN11_REG_WIDI1_KCR_COUNTER_3;
                break;

            case PAVP_REGISTER_WIDI1_KCR_COUNTER_DW4:
                RegOffset = KM_PAVP_GEN11_REG_WIDI1_KCR_COUNTER_4;
                break;

            case PAVP_REGISTER_KCR_APP_CHAR:

                // The app characteristics base register is different for display and transcode sessions.
                // It is also different for display session 12.
                if (PAVP_INDEX_FROM_ID(Index) != PAVP_GEN11_KCR_APP_ID_12)
                {
                    if (PAVP_TYPE_FROM_ID(Index) == KM_PAVP_SESSION_TYPE_DECODE)
                    {
                        RegOffset = KM_PAVP_GEN11_REG_KCR_DISPLAY_APP_CHAR_REG_BASE;
                    }
                    else
                    {
                        RegOffset = KM_PAVP_GEN11_REG_KCR_TRANSCODE_APP_CHAR_REG_BASE;
                    }
                    RegOffset += (PAVP_GEN11_KCR_APP_CHAR_OFFSET * PAVP_INDEX_FROM_ID(Index));
                }
                else
                {
                    RegOffset = KM_PAVP_GEN11_REG_KCR_DISPLAY_APP_CHAR_12;
                }
                break;

#if ((_DEBUG || _RELEASE_INTERNAL))

            case PAVP_REGISTER_STATUS:
                RegOffset = KM_PAVP_GEN7_REG_STATUS;
                break;

            case PAVP_REGISTER_KCR_INIT:
                RegOffset = KM_PAVP_GEN11_REG_KCR_INIT;
                break;

            case PAVP_REGISTER_KCR_STATUS_1:
                RegOffset = KM_PAVP_GEN11_REG_KCR_STATUS_1;
                break;

            case PAVP_REGISTER_KCR_STATUS_2:
                RegOffset = KM_PAVP_GEN11_REG_KCR_STATUS_2;
                break;

            case PAVP_REGISTER_DEBUG:
                RegOffset = KM_PAVP_GEN7_REG_DEBUG;
                break;

            case PAVP_REGISTER_MSG_REQ_GFX_KEY:
                RegOffset = KM_PAVP_GEN11_REG_MSG_REQ_GFX_KEY;
                break;

            case PAVP_REGISTER_MSG_DELIVER_GFX_KEY_1:
                RegOffset = KM_PAVP_GEN11_REG_MSG_DELIVER_GFX_KEY_1;
                break;

            case PAVP_REGISTER_MSG_DELIVER_GFX_KEY_2:
                RegOffset = KM_PAVP_GEN11_REG_MSG_DELIVER_GFX_KEY_2;
                break;

            case PAVP_REGISTER_MSG_DELIVER_GFX_KEY_3:
                RegOffset = KM_PAVP_GEN11_REG_MSG_DELIVER_GFX_KEY_3;
                break;

            case PAVP_REGISTER_MSG_DELIVER_GFX_KEY_4:
                RegOffset = KM_PAVP_GEN11_REG_MSG_DELIVER_GFX_KEY_4;
                break;

            case PAVP_REGISTER_MSG_CHECK_GFX_KEY_1:
                RegOffset = KM_PAVP_GEN11_REG_MSG_CHECK_GFX_KEY_1;
                break;

            case PAVP_REGISTER_MSG_CHECK_GFX_KEY_2:
                RegOffset = KM_PAVP_GEN11_REG_MSG_CHECK_GFX_KEY_2;
                break;

            case PAVP_REGISTER_MSG_CHECK_GFX_KEY_3:
                RegOffset = KM_PAVP_GEN11_REG_MSG_CHECK_GFX_KEY_3;
                break;

            case PAVP_REGISTER_MSG_CHECK_GFX_KEY_4:
                RegOffset = KM_PAVP_GEN11_REG_MSG_CHECK_GFX_KEY_4;
                break;

            case PAVP_REGISTER_MSG_CHECK_GFX_KEY_STATUS:
                RegOffset = KM_PAVP_GEN11_REG_MSG_CHECK_GFX_KEY_STATUS;
                break;

            case PAVP_REGISTER_MSG_PAVP_OPERATION:
                RegOffset = KM_PAVP_GEN11_REG_MSG_PAVP_OPERATION;
                break;

            case PAVP_REGISTER_SECURE_CHARACTERISTIC_1:
                RegOffset = KM_PAVP_GEN11_REG_SEC_PAVP_CHARACTERISTIC_1;
                break;

            case PAVP_REGISTER_SECURE_CHARACTERISTIC_2:
                RegOffset = KM_PAVP_GEN11_REG_SEC_PAVP_CHARACTERISTIC_2;
                break;

            case PAVP_REGISTER_SECURE_CHARACTERISTIC_3:
                RegOffset = KM_PAVP_GEN11_REG_SEC_PAVP_CHARACTERISTIC_3;
                break;

            case PAVP_REGISTER_SECURE_CHARACTERISTIC_4:
                RegOffset = KM_PAVP_GEN11_REG_SEC_PAVP_CHARACTERISTIC_4;
                break;

            case PAVP_REGISTER_SECURE_KEY_1:
                RegOffset = KM_PAVP_GEN11_REG_SEC_PAVP_KEY_1;
                break;

            case PAVP_REGISTER_SECURE_KEY_2:
                RegOffset = KM_PAVP_GEN11_REG_SEC_PAVP_KEY_2;
                break;

            case PAVP_REGISTER_SECURE_KEY_3:
                RegOffset = KM_PAVP_GEN11_REG_SEC_PAVP_KEY_3;
                break;

            case PAVP_REGISTER_SECURE_KEY_4:
                RegOffset = KM_PAVP_GEN11_REG_SEC_PAVP_KEY_4;
                break;

            case PAVP_REGISTER_MSG_DELIVER_SESSION_KEY_1:
                RegOffset = KM_PAVP_GEN11_REG_MSG_DELIVER_SESSION_KEY_1;
                break;

            case PAVP_REGISTER_MSG_DELIVER_SESSION_KEY_2:
                RegOffset = KM_PAVP_GEN11_REG_MSG_DELIVER_SESSION_KEY_2;
                break;

            case PAVP_REGISTER_MSG_DELIVER_SESSION_KEY_3:
                RegOffset = KM_PAVP_GEN11_REG_MSG_DELIVER_SESSION_KEY_3;
                break;

            case PAVP_REGISTER_MSG_DELIVER_SESSION_KEY_4:
                RegOffset = KM_PAVP_GEN11_REG_MSG_DELIVER_SESSION_KEY_4;
                break;

            case PAVP_REGISTER_MSG_DELIVER_WIDI_KEY_1:
                RegOffset = KM_PAVP_GEN11_REG_MSG_DELIVER_WIDI_KEY_1;
                break;

            case PAVP_REGISTER_MSG_DELIVER_WIDI_KEY_2:
                RegOffset = KM_PAVP_GEN11_REG_MSG_DELIVER_WIDI_KEY_2;
                break;

            case PAVP_REGISTER_MSG_DELIVER_WIDI_KEY_3:
                RegOffset = KM_PAVP_GEN11_REG_MSG_DELIVER_WIDI_KEY_3;
                break;

            case PAVP_REGISTER_MSG_DELIVER_WIDI_KEY_4:
                RegOffset = KM_PAVP_GEN11_REG_MSG_DELIVER_WIDI_KEY_4;
                break;

            case PAVP_REGISTER_MSG_PAVP_OPERATION_DONE:
                RegOffset = KM_PAVP_GEN11_REG_MSG_PAVP_OPERATION_DONE;
                break;

            case PAVP_REGISTER_SEND_MSG_HW_STATUS_1:
                RegOffset = KM_PAVP_GEN11_REG_SEND_MSG_HW_STATUS_1;
                break;

            case PAVP_REGISTER_SEND_MSG_HW_STATUS_2:
                RegOffset = KM_PAVP_GEN11_REG_SEND_MSG_HW_STATUS_2;
                break;

            case PAVP_REGISTER_SEND_MSG_HW_STATUS_3:
                RegOffset = KM_PAVP_GEN11_REG_SEND_MSG_HW_STATUS_3;
                break;

            case PAVP_REGISTER_SEND_MSG_HW_STATUS_4:
                RegOffset = KM_PAVP_GEN11_REG_SEND_MSG_HW_STATUS_4;
                break;
#endif //((_DEBUG || _RELEASE_INTERNAL))
            default:
                RegOffset = INVALID_REG_OFFSET;
        }
        return RegOffset;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Creates a new CPavpGpuHal object
    /// \par         Details:
    ///
    /// \param       hr             [out] return value  S_OK upon success, a failed HRESULT otherwise.
    /// \param       sPlatform      [in] platform type
    /// \return      New instance of CPavpGpuHal.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CPavpGpuHal *PavpGpuHalFactory(HRESULT& hr, PLATFORM& sPlatform, MEDIA_WA_TABLE& sWaTable, CPavpOsServices& OsServices);

    CPavpGpuHal(PLATFORM& sPlatform, MEDIA_WA_TABLE& sWaTable, HRESULT& hr, CPavpOsServices& OsServices);

    virtual ~CPavpGpuHal();

    /// \defgroup CommonLogic Common logic for entire command in CPavpGpuHal
    /// @{

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Builds the key exchange functionality by appending specific HW commands to the command buffer.
    ///
    /// \param pCmdBuf                  [in] The mos command buffer ptr to append  the command
    /// \param pBuildKeyExchangeParams  [in] The key exchange parameteres
    /// \return HRESULT
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT BuildKeyExchange(MOS_COMMAND_BUFFER *pCmdBuf, const PAVP_GPU_HAL_KEY_EXCHANGE_PARAMS& sBuildKeyExchangeParams);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Builds multiple crypto copy functionality by appending specific HW commands to the command buffer.
    ///
    /// \param pCmdBuf                       [in] The mos command buffer ptr to append the command
    /// \param pBuildMultiCryptoCopyParams   [in] Point to the multiple crypto copy parameteres
    /// \param cryptoCopyNumber              [in] The Number of multiple crypto copy parameteres
    /// \return int32_t
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual int32_t BuildMultiCryptoCopy(
        MOS_COMMAND_BUFFER                           *pCmdBuf,
        PAVP_GPU_HAL_MULTI_CRYPTO_COPY_PARAMS        *pBuildMultiCryptoCopyParams,
        size_t                                       cryptoCopyNumber);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Check whether the counter will rollover when serpent to AES,and calculate the "midpoint" (rollover value) based on which bits rolled over
    ///
    /// \param sCryptoCopyParam              [in] The crypto copy parameteres
    /// \param numBytesBeforeRollover        [out] Number of bytes that do not need to rollover
    /// \param remainingSize                 [out] Number of bytes that need to rollover
    /// \return bool
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual bool CheckAudioCryptoCopyCounterRollover(
        PAVP_GPU_HAL_MULTI_CRYPTO_COPY_PARAMS sCryptoCopyParam,
        uint32_t                              &numBytesBeforeRollover,
        uint32_t                              &remainingSize);

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
    virtual HRESULT BuildQueryStatus(MOS_COMMAND_BUFFER *pCmdBuf, PAVP_GPU_HAL_QUERY_STATUS_PARAMS& sBuildQueryStatusParams);
    /// @}

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Saves the Crypto Key params in MHW layer. This is used for inserting Crypto Key Exchange 
    /// \command for VCS Decode workloads for Gen11+
    ///
    /// \param pMhwPavpEncParams  [in] The Mhw Pavp Encryption params ptr 
    /// \return HRESULT
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT SetGpuCryptoKeyExParams(PMHW_PAVP_ENCRYPTION_PARAMS pMhwPavpEncParams);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Called to determine the size of a command buffer prior to allocatoin
    /// \details     On success, uiMaxSize is populated with the maximum # of DWORDS needed for the buffer
    ///
    /// \param       type       [in] The command type
    /// \param       uiMaxSize  [out] The maximal size of such command
    /// \return      HRESULT, uiMaxSize populated
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT GetMaxCmdBufSizeInDwords(PAVP_OPERATION_TYPE type, uint32_t& uiMaxSize);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Read session mode register
    ///
    /// \param       sessionId      [in]  The PAVP sessionId
    /// \param       sessionMode    [in]  The PAVP session mode
    /// \param       value          [out] The value read from the corresponding register
    /// \return      HRESULT
    ///////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT ReadSessionModeRegister(
        uint32_t                  sessionId,
        PAVP_SESSION_MODE       sessionMode,
        PAVP_GPU_REGISTER_VALUE &value);

    //////////////////////////////////////////////////////////////////////////////////////
    /// \brief     Verify the session mode based on the current MMIO register status.
    ///
    /// \param       sessionId      [in] The PAVP session Id
    /// \param       sessionMode    [in] The PAVP session mode
    /// \param       value          [in] The value read from the corresponding register
    ///
    /// \return       true if the session mode is matched according to session mode MMIO register
    ///////////////////////////////////////////////////////////////////////////////////////
    virtual BOOL VerifySessionMode(uint32_t sessionId, PAVP_SESSION_MODE sessionMode, PAVP_GPU_REGISTER_VALUE value);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Read session in play register
    ///
    /// \param       sessionId      [in]  The PAVP sessionId
    /// \param       value          [out] The value read from the corresponding register
    /// \return      HRESULT
    ///////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT ReadSessionInPlayRegister(
        uint32_t                  sessionId,
        PAVP_GPU_REGISTER_VALUE &value);

    /// \brief       Determines if a PAVP session is alive based on the current MMIO register status.
    ///
    /// \param       sessionId      [in] The PAVP sessionId
    /// \param       value          [in] The value read from the corresponding register
    /// \return      True if the session is currently alive according
    ///              to the session-in-play MMIO register
    /// \note        Use GetRegisterOffset to find the MMIO offset to query on this platform
    ///////////////////////////////////////////////////////////////////////////////////////
    virtual BOOL VerifySessionInPlay(
        uint32_t                  sessionId,
        PAVP_GPU_REGISTER_VALUE value);

    //////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Determines if a transcode session has appropriate characteristics.
    /// \param       ctx                [in] CP context (session ID) to verify
    ///
    /// \return      HRESULT
    ///////////////////////////////////////////////////////////////////////////////////////    
    HRESULT VerifyTranscodeSessionProperties(CpContext ctx);

    //////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Two steps operation (1) clear=>serpent and (2) serpent=>AES-counter to replace BSD_CRYPTO_COPY mode 0b0110 (clear to AES-counter)
    /// \param       pCmdBuf                [in] Pointer to command buffer
    /// \param       sBuildCryptoCopyParams [in] Params for crypto copy
    /// \param       bWaitForCompletion     [in] If need to wait hardware CryptoCopy to complete: true:synchronous; false:asynchronous

    /// \return      HRESULT
    ///////////////////////////////////////////////////////////////////////////////////////    
    HRESULT BuildCryptoCopyClearToAesCounter(MOS_COMMAND_BUFFER *pCmdBuf, const PAVP_GPU_HAL_CRYPTO_COPY_PARAMS &sBuildCryptoCopyParams, bool bWaitForCompletion);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Sets whether parsing assistance is provided by KMD or not.  TRUE if there is assistance
    ///              (and therefore no need to add commands such as MI_BATCH_BUFFER_END to the end of batch
    ///              buffers).
    /// \details     Currently, KMD parsing assistance is available only in Windows.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    void SetParsingAssistanceFromKmd(BOOL bParsingAssistance) { m_bParsingAssistanceFromKmd = bParsingAssistance; }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Sets if a combined workload is required to read counter and do cryptocopy for ID case
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    inline BOOL IsOneWLRequired() { return (m_sPlatform.eProductFamily >= IGFX_TIGERLAKE_LP); }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Sets if Key Ex command is required in Prolog
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    inline BOOL IsPrologCryptoKeyExMgmtRequired() { return (m_sPlatform.eProductFamily >= IGFX_ICELAKE || m_sPlatform.eProductFamily >= IGFX_ICELAKE_LP); }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Sets if UMD needs to manage AppID Register on certain GEN graphics product family
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    inline BOOL IsAppIDRegisterMgmtRequired() { return (m_sPlatform.eProductFamily < IGFX_ICELAKE); }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Sets if UMD needs to set Display Plane Enables on certain GEN graphics product family
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    inline BOOL IsPlaneEnablesRequired() { return (m_sPlatform.eProductFamily < IGFX_ICELAKE); }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Sets if UMD needs to use older FW API version on certain GEN graphics product family
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    inline BOOL DoesPlatformFWRequireFallback() { return (m_sPlatform.eProductFamily <=IGFX_KABYLAKE); }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Determines if HW incrementing inputCtr is supported (KBL+)
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    inline BOOL IsGpuWidiCounterSupported() { return (m_sPlatform.eProductFamily >= IGFX_KABYLAKE); }

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
        const bool      bPreProduction,
        const uint32_t*&  pTranscryptMetaData,
        uint32_t&         uiTranscryptMetaDataSize,
        const uint32_t*&  pEncryptedKernels,
        uint32_t&         uiEncryptedKernelsSize);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Determines if HW support ASMF mode. It always returns S_OK for ICK+ (Gen11+)
    ///              , so the funciton should be removed if gfx driver doesn't support Gen9 and Gen10 
    ///              in the future.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HRESULT CheckAsmfModeSupport() { return S_OK; }

protected:

    // \brief Parsing assistance in KMD is different between OSs (non-existent in Linux). This
    //        assistance (among other things) adds an MI_BATCH_BUFFER_END command automatically,
    //        so in Linux we need to add it manually. Therefore, the GPU HAL must be informed of this.
    BOOL m_bParsingAssistanceFromKmd;

    PLATFORM                    m_sPlatform;    // Remember platform to differentiate VLV from IVB for instance.
    MEDIA_WA_TABLE              m_sWaTable;     // Workaround table required for BDW+ GT3/GT4 platforms.
    CPavpOsServices             *m_pOsServices;
    PMOS_RESOURCE               m_pResourcehwctr = nullptr;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief  Allocates a buffer big enough for buffer and signature, copies both to the new buffer.
    /// 
    /// \param  pBuffer         [in] DWORD pointer to buffer 
    /// \param  BufferSize      [in] DWORD buffer size (bytes)
    /// \param  pSignature      [in] DWORD pointer to signature
    /// \param  SigSize         [in] DWORD signature size (bytes) 
    /// \param  pCb2Buffer      [out] DWORD pointer reference to new buffer
    /// \param  Cb2BufferSize   [out] DWORD reference to new buffer size 
    /// \return S_OK on success     
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    HRESULT CreateCb2Buffer(
        PDWORD pBuffer,
        DWORD  BufferSize,
        PDWORD pSignature,
        DWORD  SigSize,
        PDWORD &pCb2Buffer,
        DWORD  &Cb2BufferSize);

    MEDIA_CLASS_DEFINE_END(CPavpGpuHal)
};
typedef CpFactory<CPavpGpuHal, PLATFORM&, MEDIA_WA_TABLE&, HRESULT&, CPavpOsServices&> CPavpGpuHalFactory;

#endif // __CP_HAL_GPU_H__