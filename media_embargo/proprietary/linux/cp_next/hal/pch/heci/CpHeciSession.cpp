/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2011-2020 Intel Corporation All Rights Reserved.

The source code contained or described herein and all documents related to the
source code ("Material") are owned by Intel Corporation or its suppliers or licensors.
Title to the Material remains with Intel Corporation or its suppliers and licensors.
The Material may contain trade secrets and proprietary and confidential information
of Intel Corporation and its suppliers and licensors, and is protected by worldwide
copyright and trade secret laws and treaty provisions. No part of the Material
may be used, copied, reproduced, modified, published, uploaded, posted, transmitted,
distributed, or disclosed in any way without Intel's prior express written permission.

No license under any patent, copyright, trade secret or other intellectual property
right is granted to or conferred upon you by disclosure or delivery of the Materials,
either expressly, by implication, inducement, estoppel or otherwise. Any license
under such intellectual property rights must be express and approved by Intel in writing.

======================= end_copyright_notice ==================================*/

#include "CpHeciSession.h"
#include "intel_pavp_error.h"
#include "cp_HeciLog.h"
#include "intel_pavp_heci_api.h"
#include "cp_os_services_linux.h"

#define SIGMA_PRI_DATE_LEN_SIZE     4

CHeciSessionInterface* CHeciSessionInterface::PchHal_HeciSessionFactory(
    const GUID* CpMsgClass,
    BOOL isWidiEnabled,
    std::shared_ptr<CPavpOsServices> pOsServices
    )
{
    if (isWidiEnabled == TRUE)
    {
        PAVP_MESESSION_ASSERTMESSAGE("Linux stack doesn't support WiDi");
        return nullptr;
    }
    CHeciSessionInterface   *pHeciSession   = nullptr;

    pHeciSession = new (std::nothrow) CpHeciSession(pOsServices);

    return pHeciSession;
}

BOOL CHeciSessionInterface::PchHal_DestroyHeciSessionFactory(BOOL isWidiEnabled)
{
    MOS_UNUSED(isWidiEnabled);
    return TRUE;
}

CpHeciSession::CpHeciSession(std::shared_ptr<CPavpOsServices> pOsServices) : m_OsServices(pOsServices)
{
    PAVP_MESESSION_FUNCTION_ENTER;
}

CpHeciSession::~CpHeciSession()
{
    PAVP_MESESSION_FUNCTION_ENTER;
    PAVP_MESESSION_FUNCTION_EXIT(0);
}

PAVP_STATUS CpHeciSession::IoMessage(
    uint8_t *pInput, uint32_t  InputLen,
    uint8_t *pOutput, uint32_t  OutputLen,
    uint8_t *pIgdData, uint32_t *pIgdDataLen)
{
    PAVP_STATUS     status                  = PAVP_STATUS_SUCCESS;
    PAVPCmdHdr      *pPavpCmdHdr            = nullptr;
    uint32_t        RecvSize                = 0;
    uint8_t         *pMaxOutputBuffer       = nullptr;
    uint32_t        MaxOutputBufferSize     = 0;

    PAVP_MESESSION_FUNCTION_ENTER;

    do
    {
        if ( pInput  == nullptr  ||
             pOutput == nullptr)
        {
            PAVP_MESESSION_ASSERTMESSAGE("Invalid  input parameter.");
            status = PAVP_STATUS_INVALID_PARAMS;
            break;
        }

        //make sure outputbuffer is big enough to hold private dta
        MaxOutputBufferSize = OutputLen + FIRMWARE_MAX_IGD_DATA_SIZE;
        pMaxOutputBuffer = (uint8_t*)MOS_AllocAndZeroMemory(MaxOutputBufferSize);
        if (pMaxOutputBuffer == nullptr)
        {
            PAVP_MESESSION_ASSERTMESSAGE("failed to allocate MEI output buffer");
            break;
        }

        CPavpOsServices_Linux* OsServices = dynamic_cast<CPavpOsServices_Linux*>(m_OsServices.get());

        //copy anydata in output buffer being sent from App/driver to FW
        //Application will send vtag in Output buffer for Chrome Widevine usage so don't set output buffer to zero
        if (MOS_FAILED(MOS_SecureMemcpy(pMaxOutputBuffer, OutputLen, pOutput, OutputLen)))
        {
            PAVP_MESESSION_ASSERTMESSAGE("Failed to copy output buffer");
            status = PAVP_STATUS_UNKNOWN_ERROR;
            break;
        }

        if (OsServices == nullptr)
        {
            status = PAVP_STATUS_UNKNOWN_ERROR;
            PAVP_MESESSION_ASSERTMESSAGE("OsServices is nullptr");
            break;
        }

        CHeciSessionInterface::OutputMessage( pInput, InputLen);

        if (FAILED(OsServices->SendRecvDataTEE(
                    pInput,
                    InputLen,
                    MaxOutputBufferSize,
                    pMaxOutputBuffer,
                    &RecvSize)))
        {
            PAVP_MESESSION_ASSERTMESSAGE("cannot send or receive buffer to Mei");
            status = PAVP_STATUS_HECI_COMMUNICATION_ERROR;
            break;
        }

        // i915 has similar check but keep it here just to be safe
        if (RecvSize > MaxOutputBufferSize)
        {
            PAVP_MESESSION_ASSERTMESSAGE("Received too large buffer from FW");
            status = PAVP_STATUS_UNKNOWN_ERROR;
            break;
        }
        CHeciSessionInterface::OutputMessage( pMaxOutputBuffer, RecvSize);

        //each message should have PAVP command header
        if (RecvSize < sizeof(PAVPCmdHdr))
        {
            PAVP_MESESSION_ASSERTMESSAGE("Received too short buffer from FW");
            status = PAVP_STATUS_UNKNOWN_ERROR;
            break;
        }

        pPavpCmdHdr = reinterpret_cast<PAVPCmdHdr*>(pMaxOutputBuffer);
        uint32_t PavpAppMsgSize = sizeof(PAVPCmdHdr) + pPavpCmdHdr->BufferLength;

        if (PavpAppMsgSize > RecvSize)  // FW should atleast sent PavpAppMsgSize bytes
        {
            PAVP_MESESSION_ASSERTMESSAGE("Buffer received from FW is of unexpected size");
            status = PAVP_STATUS_UNKNOWN_ERROR;
            break;
        }
        if (PavpAppMsgSize > OutputLen)  // App buffer size should be large enough to hold PavpAppMsgSize bytes
        {
            PAVP_MESESSION_ASSERTMESSAGE("Buffer received from FW is of larger size than expected by ring 3");
            status = PAVP_STATUS_UNKNOWN_ERROR;
            break;
        }

        if (MOS_FAILED(MOS_SecureMemcpy(pOutput, PavpAppMsgSize, pMaxOutputBuffer, PavpAppMsgSize)))
        {
            PAVP_MESESSION_ASSERTMESSAGE("Failed to copy output buffer");
            status = PAVP_STATUS_UNKNOWN_ERROR;
            break;
        }

        // check if IGD private data present.
        if (pPavpCmdHdr->PavpCmdStatus == FIRMWARE_PRIVATE_INFO_FLAG)
        {
            PAVP_MESESSION_NORMALMESSAGE("The response contains IGD private data.");

            if (pIgdData    == nullptr ||
                pIgdDataLen == nullptr)
            {
                PAVP_MESESSION_ASSERTMESSAGE("Invalid IGD private data parameters.");
                status = PAVP_STATUS_INVALID_PARAMS;
                break;
            }

            if ((pPavpCmdHdr->ApiVersion > FIRMWARE_API_VERSION_4_1))
            {
                // PavpAppMsgSize + SIGMA_PRI_DATE_LEN_SIZE should be less than RecvSize
                PAVP_MESESSION_ASSERT(RecvSize >= PavpAppMsgSize + SIGMA_PRI_DATE_LEN_SIZE);

                if (MOS_FAILED(MOS_SecureMemcpy(pIgdDataLen, SIGMA_PRI_DATE_LEN_SIZE, pMaxOutputBuffer + PavpAppMsgSize, SIGMA_PRI_DATE_LEN_SIZE)))
                {
                    PAVP_MESESSION_ASSERTMESSAGE("Failed to copy private data length");
                    status = PAVP_STATUS_UNKNOWN_ERROR;
                    break;
                }

                // Private data size for this command should be less than equal to  max private data size
                PAVP_MESESSION_ASSERT(FIRMWARE_MAX_IGD_DATA_SIZE >= *pIgdDataLen)

                // PavpAppMsgSize + SIGMA_PRI_DATE_LEN_SIZE + private data should be equal to RecvSize
                // because no other data is present after private data
                PAVP_MESESSION_ASSERT(RecvSize == PavpAppMsgSize + SIGMA_PRI_DATE_LEN_SIZE+ *pIgdDataLen);

                // Copy data for IGD.
                if (MOS_FAILED(MOS_SecureMemcpy(pIgdData, *pIgdDataLen, pMaxOutputBuffer + PavpAppMsgSize + SIGMA_PRI_DATE_LEN_SIZE, *pIgdDataLen)))
                {
                    PAVP_MESESSION_ASSERTMESSAGE("Failed to copy private data");
                    status = PAVP_STATUS_UNKNOWN_ERROR;
                    break;
                }
            }
            else
            {
                *pIgdDataLen = RecvSize - PavpAppMsgSize;

                // Private data size for this command should be less than equal to  max private data size
                PAVP_MESESSION_ASSERT(FIRMWARE_MAX_IGD_DATA_SIZE >= *pIgdDataLen)

                // PavpAppMsgSize + private data should be equalto RecvSize
                // because no other data is present after private data
                PAVP_MESESSION_ASSERT(RecvSize == PavpAppMsgSize + *pIgdDataLen);

                if (MOS_FAILED(MOS_SecureMemcpy(pIgdData, *pIgdDataLen, pMaxOutputBuffer + PavpAppMsgSize, *pIgdDataLen)))
                {
                    PAVP_MESESSION_ASSERTMESSAGE("Failed to copy private data");
                    status = PAVP_STATUS_UNKNOWN_ERROR;
                    break;
                }
            }
        }
    } while (false);

    if (pMaxOutputBuffer != nullptr)
    {
        MOS_FreeMemory(pMaxOutputBuffer);
        pMaxOutputBuffer = nullptr;
    }

    PAVP_MESESSION_FUNCTION_EXIT(status);
    return status;
}