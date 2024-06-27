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

File Name: cp_hal_gpu.cpp

Abstract:
Abstract base class for GPU HAL for PAVP functions

Environment:
OS agnostic - should support - Windows Vista, Windows 7 (D3D9), 8 (D3D11.1), Linux, Android
Platform agnostic - should support - Gen 7.5, Gen8, Gen9

Notes:
None

======================= end_copyright_notice ==================================*/

#include "cp_hal_gpu.h"
#include "cp_hal_gpu_g12.h"
#include "cp_hal_gpu_defs.h"
#include "mhw_cp.h"
#include "mhw_mi.h"
#include "mos_utilities.h"
#include "cp_os_services.h"
#include "mhw_cp_hwcmd_common.h"

//To determine min/max value for App Id for Decode and Transcode Sessions. 
#define MIN_TRANSCODE_APP_ID   0x80
#define MAX_TRANSCODE_APP_ID   0x87
#define TRANSCODE_APP_ID_MASK  0x80
#define MIN_DECODE_APP_ID      0x0
#define MAX_DECODE_APP_ID      0xf

// Use these to mask the session in play register to get the session types in play.
#define PAVP_DECODE_SESSION_IN_PLAY_MASK      0xFFFF // Includes WiDi.
#define PAVP_TRANSCODE_SESSION_IN_PLAY_MASK   0xFF0000
#define PAVP_WIDI_SESSION_IN_PLAY_MASK        0x1000

CPavpGpuHal::~CPavpGpuHal()
{
    GPU_HAL_FUNCTION_ENTER;

    if (m_pResourcehwctr)
    {
        m_pOsServices->FreeResource(m_pResourcehwctr);
    }
}

CPavpGpuHal::CPavpGpuHal(PLATFORM& sPlatform, MEDIA_WA_TABLE& sWaTable, HRESULT& hr, CPavpOsServices& OsServices)
{
    m_bParsingAssistanceFromKmd = true;
    m_sPlatform                 = sPlatform;
    m_sWaTable                  = sWaTable;
    m_pOsServices               = &OsServices;

    hr = S_OK;
}

HRESULT CPavpGpuHal::ReadSessionInPlayRegister(
    uint32_t                sessionId,
    PAVP_GPU_REGISTER_VALUE &value)
{
    HRESULT hr = S_OK;

    GPU_HAL_FUNCTION_ENTER;

    do
    {
        // Read the 'PAVP Per App Session in Play' register.
        hr = m_pOsServices->ReadMMIORegister(
                PAVP_REGISTER_SESSION_IN_PLAY, 
                value,
                CPavpGpuHal::ConvertRegType2RegOffset(PAVP_REGISTER_SESSION_IN_PLAY),
                0,                              //Default Index value
                PAVP_HWACCESS_GPU_NOT_IDLE);
        if (FAILED(hr))
        {
            GPU_HAL_ASSERTMESSAGE("Failed to read the 'PAVP Per App Session in Play' register.");
            MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        GPU_HAL_VERBOSEMESSAGE("'PAVP Per App Session in Play' register value: 0x%x.", static_cast<uint32_t>(value));
    } while (FALSE);

    GPU_HAL_FUNCTION_EXIT(hr);
    return hr;
}

BOOL CPavpGpuHal::VerifySessionInPlay(
    uint32_t                  sessionId,
    PAVP_GPU_REGISTER_VALUE value)
{
    PAVP_GPU_REGISTER_VALUE SessionInPlayMask = 0;

    SessionInPlayMask = (PAVP_GPU_REGISTER_VALUE)1 << (sessionId & 0xF);
    SessionInPlayMask <<= ((sessionId & TRANSCODE_APP_ID_MASK) ? 16 : 0);
    return ((SessionInPlayMask & value) != 0);
}

HRESULT CPavpGpuHal::VerifyTranscodeSessionProperties(CpContext ctx)
{
    HRESULT hr = S_OK;
    PAVP_GPU_REGISTER_VALUE AppCharRegValue = 0;

    // Starting from LKF B0/TGL A0, transcode sessions need to have the "Allow Programmable AES Counter" bit set in
    // app characteristics in order to be used.
    const uint32_t PROGRAMMABLE_AES_COUNTER_BIT = __BIT(14);

    GPU_HAL_FUNCTION_ENTER;

    do
    {
        if (MEDIA_IS_WA(&m_sWaTable, WaCheckTranscodeAppCharRegister))
        {
            GPU_HAL_VERBOSEMESSAGE("Checking transcode session app characteristics for session ID 0x%02x to ensure that programmable AES counter bit is set.",
                ctx.sessionId);
            hr = m_pOsServices->ReadMMIORegister(PAVP_REGISTER_KCR_APP_CHAR, AppCharRegValue, CPavpGpuHal::ConvertRegType2RegOffset(PAVP_REGISTER_KCR_APP_CHAR), ctx.sessionId);
            if (FAILED(hr))
            {
                GPU_HAL_ASSERTMESSAGE("Failed to read app characteristics register for transcode session, hr = 0x%08x", hr);
                MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }
            GPU_HAL_VERBOSEMESSAGE("AppCharRegValue is 0x%08llx", AppCharRegValue);
            if (!(AppCharRegValue & PROGRAMMABLE_AES_COUNTER_BIT))
            {
                GPU_HAL_ASSERTMESSAGE("Programmable AES counter bit is not set in app characteristics, failing transcode session creation!");
                hr = E_UNEXPECTED;
                MT_ERR2(MT_CP_TRANSCODE_SESSION, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
                break;
            }
        }
    } while (FALSE);

    GPU_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpGpuHal::ReadSessionModeRegister(
    uint32_t                  sessionId,
    PAVP_SESSION_MODE               sessionMode,
    PAVP_GPU_REGISTER_VALUE &value)
{
   GPU_HAL_FUNCTION_ENTER;

   GPU_HAL_VERBOSEMESSAGE("Session mode register read not implemented yet!");

   GPU_HAL_FUNCTION_EXIT(S_OK);
   return S_OK;
}

BOOL CPavpGpuHal::VerifySessionMode(
    uint32_t                  sessionId,
    PAVP_SESSION_MODE       sessionMode,
    PAVP_GPU_REGISTER_VALUE ullRegValue)
{
    GPU_HAL_FUNCTION_ENTER;

    GPU_HAL_VERBOSEMESSAGE("Session mode verification not implemented yet!");

    GPU_HAL_FUNCTION_EXIT(S_OK);
    return TRUE;
}

// Return correct object based on platform.
CPavpGpuHal *CPavpGpuHal::PavpGpuHalFactory(
    HRESULT&    hr,                // [out]
    PLATFORM&    sPlatform,         // [in]
    MEDIA_WA_TABLE&    sWaTable,          // [in]
    CPavpOsServices& OsServices)
{ 

    GPU_HAL_FUNCTION_ENTER;

    CPavpGpuHal *pGpuHal = CPavpGpuHalFactory::Create(sPlatform.eProductFamily, sPlatform, sWaTable, hr, OsServices);
    if(nullptr != pGpuHal)
    {
        GPU_HAL_NORMALMESSAGE("Creating GPU HAL for Product family id: %d succeed", sPlatform.eProductFamily);
    }
    else
    {
        GPU_HAL_ASSERTMESSAGE("No supported platform detected. Product family id: %d", sPlatform.eProductFamily);
        MT_ERR3(MT_ERR_INVALID_ARG, MT_CP_PRODUCT_FAMILY_ID, sPlatform.eProductFamily, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
        hr = E_INVALIDARG;
    }

    if (pGpuHal == nullptr &&
        hr == S_OK)
    {
        GPU_HAL_ASSERTMESSAGE("Failed to allocate GPU HAL abstraction layer.");
        MT_ERR2(MT_CP_HAL_FAIL, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
        hr = E_FAIL;
    }

    GPU_HAL_FUNCTION_EXIT(hr);
        
    return pGpuHal;
}

/*******************************************************************************
*                           Public functions                                   *
********************************************************************************/

HRESULT CPavpGpuHal::GetMaxCmdBufSizeInDwords(PAVP_OPERATION_TYPE eCmdType, uint32_t& uiMaxSize)
{
    HRESULT hr = S_OK;

    GPU_HAL_FUNCTION_ENTER;

    switch(eCmdType)
    {
        case KEY_EXCHANGE:
            uiMaxSize = PAVP_MAX_KEY_EXCHANGE_SPACE;
            break;
        case QUERY_STATUS:
            uiMaxSize = PAVP_MAX_QUERY_STATUS_SPACE;
            break;
        case CRYPTO_COPY:
            uiMaxSize = PAVP_MAX_CRYPTO_COPY_SPACE;
            break;
        default:
            GPU_HAL_ASSERTMESSAGE("Unknown command type provided: %d.", eCmdType);
            uiMaxSize = 0;
            hr = E_INVALIDARG;
            MT_ERR3(MT_ERR_INVALID_ARG, MT_CP_COMMAND_TYPE, eCmdType, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
    }

    GPU_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpGpuHal::BuildKeyExchange(
    PMOS_COMMAND_BUFFER                     pCmdBuf,
    const PAVP_GPU_HAL_KEY_EXCHANGE_PARAMS& sBuildKeyExchangeParams)
{
    MOS_UNUSED(pCmdBuf);
    MOS_UNUSED(sBuildKeyExchangeParams);
    return S_FALSE; //base class function shouldn't have been called
}

int32_t CPavpGpuHal::BuildMultiCryptoCopy(
    MOS_COMMAND_BUFFER                           *pCmdBuf,
    PAVP_GPU_HAL_MULTI_CRYPTO_COPY_PARAMS        *pBuildMultiCryptoCopyParams,
    size_t                                       cryptoCopyNumber)
{
    MOS_UNUSED(pCmdBuf);
    MOS_UNUSED(pBuildMultiCryptoCopyParams);
    MOS_UNUSED(cryptoCopyNumber);

    return S_FALSE;  //base class function shouldn't have been called
}

bool CPavpGpuHal::CheckAudioCryptoCopyCounterRollover(
    PAVP_GPU_HAL_MULTI_CRYPTO_COPY_PARAMS sCryptoCopyParam,
    uint32_t                              &numBytesBeforeRollover,
    uint32_t                              &remainingSize)
{
    return false;
}

HRESULT CPavpGpuHal::BuildCryptoCopyClearToAesCounter(
    MOS_COMMAND_BUFFER *pCmdBuf,
    const PAVP_GPU_HAL_CRYPTO_COPY_PARAMS&  sBuildCryptoCopyParams,
    bool bWaitForCompletion)
{
    MOS_UNUSED(pCmdBuf);
    MOS_UNUSED(sBuildCryptoCopyParams);
    MOS_UNUSED(bWaitForCompletion);

    return S_FALSE;  //base class function shouldn't have been called
}

HRESULT CPavpGpuHal::BuildCryptoCopy(
    MOS_COMMAND_BUFFER                     *pCmdBuf,
    const PAVP_GPU_HAL_CRYPTO_COPY_PARAMS  &sBuildCryptoCopyParams)
{
    MOS_UNUSED(pCmdBuf);
    MOS_UNUSED(sBuildCryptoCopyParams);

    return S_FALSE;  //base class function shouldn't have been called
}

HRESULT CPavpGpuHal::SetGpuCryptoKeyExParams(PMHW_PAVP_ENCRYPTION_PARAMS pMhwPavpEncParams)
{
    return S_OK;
}

HRESULT CPavpGpuHal::BuildQueryStatus(
    PMOS_COMMAND_BUFFER                      pCmdBuf,
    PAVP_GPU_HAL_QUERY_STATUS_PARAMS&        sBuildQueryStatusParams)
{
    MOS_UNUSED(pCmdBuf);
    MOS_UNUSED(sBuildQueryStatusParams);

    return S_FALSE;  //base class function shouldn't have been called
}

HRESULT CPavpGpuHal::CreateCb2Buffer(
    PDWORD pBuffer,         // Kernel buffer.
    DWORD BufferSize,
    PDWORD pSignature,      // Buffer signature (platform specific).
    DWORD SigSize,
    PDWORD &pCb2Buffer,     // New buffer pointer.
    DWORD &Cb2BufferSize)   // New buffer size.
{
    HRESULT     hr = S_OK;
    MOS_STATUS  eStatus = MOS_STATUS_SUCCESS;
    PDWORD      pSig    = nullptr;

    GPU_HAL_FUNCTION_ENTER;

    pCb2Buffer = nullptr;
    Cb2BufferSize = BufferSize + SigSize;

    // We need to concatenate the buffer with the signature in a new buffer.
    do
    {
        if (BufferSize  == 0       ||
            SigSize     == 0       ||
            pBuffer     == nullptr ||
            pSignature  == nullptr)
        {
            GPU_HAL_ASSERTMESSAGE("Invalid parameter.");
            hr = E_INVALIDARG;
            MT_ERR2(MT_ERR_INVALID_ARG, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }

        // We allocate here relying on containing object (CPavpCb2Buffer) to deallocate.
        pCb2Buffer = new (std::nothrow) DWORD[Cb2BufferSize/sizeof(DWORD)];
        if (pCb2Buffer == nullptr)
        {
            GPU_HAL_ASSERTMESSAGE("Error allocating buffer");
            hr = E_OUTOFMEMORY;
            MT_ERR2(MT_ERR_NULL_CHECK, MT_CODE_LINE, __LINE__, MT_ERROR_CODE, hr);
            break;
        }
       
        GPU_HAL_VERBOSEMESSAGE("Allocated CB2 init buffer, size %d bytes.", Cb2BufferSize);
       
        GPU_HAL_CHK_STATUS_WITH_HR(MOS_SecureMemcpy(
                  pCb2Buffer,
                  Cb2BufferSize,
                  pBuffer,
                  BufferSize));        

        pSig = pCb2Buffer + (BufferSize/sizeof(DWORD));

        GPU_HAL_CHK_STATUS_WITH_HR(MOS_SecureMemcpy(
                  pSig,
                  Cb2BufferSize - BufferSize,
                  pSignature,
                  SigSize));

    } while (FALSE);

finish:
    GPU_HAL_FUNCTION_EXIT(hr);
    return hr;
}

HRESULT CPavpGpuHal::GetInfoToTranscryptKernels(
    const bool      bPreProduction,
    const uint32_t*&  pTranscryptMetaData,
    uint32_t&         uiTranscryptMetaDataSize,
    const uint32_t*&  pEncryptedKernels,
    uint32_t&         uiEncryptedKernelsSize)
{
    MOS_UNUSED(bPreProduction);
    MOS_UNUSED(pTranscryptMetaData);
    MOS_UNUSED(uiTranscryptMetaDataSize);
    MOS_UNUSED(pEncryptedKernels);
    MOS_UNUSED(pEncryptedKernels);

    return S_FALSE;  //base class function shouldn't have been called
}
