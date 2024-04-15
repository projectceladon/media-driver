/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2012-2018
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

File Name: cp_heci_session_interface.h

Abstract:
HeciSession Interface is needed to send ME FW  commands in an agnostic way through the unified PchHal

Environment:
OS agnostic

Notes:
None
======================= end_copyright_notice ==================================*/
#ifndef __CP_HECISESSION_INTERFACE_H__
#define __CP_HECISESSION_INTERFACE_H__


#include <memory>
#include "intel_pavp_error.h"
#include "igfxfmid.h"
#include "cp_os_defs.h"
#include "cp_HeciLog.h"
#include <algorithm> //std::reverse
#include <sstream>
#include <iomanip>
#include <vector>
#include <cstring>

class CPavpOsServices;

class CHeciSessionInterface
{
public:

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Creates a new CHeciSession object
    /// \par         Details:
    ///
    /// \param       status             [out] returns status value
    //  \param.      bSimulation        [in] whether MEFW Stub is used or not
    /// \param       CpMsgClass         [in] GUID indicating the type of the session (PAVP, WIDI)
    //  \param.      eProductFamily     [in] The gen product on which this interface is being created on
    //  \param.      isWiDiEnabled      [in] Whether WiDi is enabled or not.
    /// \return      New instance of CHeciSession.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CHeciSessionInterface* PchHal_HeciSessionFactory(
        const GUID* CpMsgClass,
        BOOL isWiDiEnabled,
        std::shared_ptr<CPavpOsServices> pOsServices
        );

     static BOOL PchHal_DestroyHeciSessionFactory(
        BOOL isWidiSession);
    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Send an I/O Message
    /// \par         Details:
    ///
    /// \param       status             [out] returns status value
    /// \param       Input              [in] pointer to input buffer
    /// \param       InputLen           [in] Length of Input Buffer
    /// \param       Output             [in] pointer to output buffer
    /// \param       OutputLen          [in] Length of output Buffer
    /// \param       IgdData            [in] pointer to private buffer
    /// \param       IgdDataLen         [in] Length of private Buffer
     //////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual PAVP_STATUS IoMessage(
        PUINT8 Input, uint32_t  InputLen,
        PUINT8 Output, uint32_t  OutputLen,
        PUINT8 IgdData, uint32_t *IgdDataLen) = 0;
    virtual ~CHeciSessionInterface(){};

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       Print buffer
    /// \par         Details: Print input/output UMD FW communication message after reversing the endianess
    ///
    /// \param       inputBuffer             [in] pointer to input buffer
    /// \param       size                    [in] size of input buffer
    /// \return      returns void
     //////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void OutputMessage(const uint8_t *inputBuffer, int32_t size)
    {
#if (_DEBUG || _RELEASE_INTERNAL)
        // Number of characters in a line
        const int lineWrap = 64;

        // Add space to separate after this many characters
        const int charDelim = 4;

        // Stringstream to store output string
        std::ostringstream messageData;
        PAVP_MESESSION_VERBOSEMESSAGE("Message contents:");

        if (size == 0)
        {
            // Shouldn't end up in this code path since a message will always have at least a header,
            // but assert just in case
            PAVP_MESESSION_ASSERTMESSAGE("<empty> - not expected!");
            return;
        }

        uint32_t                        alignedSize     = MOS_ALIGN_CEIL(size, 4); // aligned size should be in multiple of 4 so that endianess can be reversed appropriately
        std::vector<uint8_t>            alignedBuffer(alignedSize, 0);
        std::vector<uint8_t>::iterator  it              = alignedBuffer.begin(); // iterator tracks the buffer and reverses endianess of every four bytes
        int                             remainingBytes  = alignedSize;           // Number of bytes left in buffer
        int                             currChar        = 0;                     // Current character processed

        memcpy(alignedBuffer.data(), inputBuffer, size);

        do
        {
            if (currChar % sizeof(uint32_t) == 0)
            {
                std::reverse(it, it + sizeof(uint32_t));
            }
            // Add current character to output string
            messageData << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(alignedBuffer.at(currChar));

            // Separate groups of bytes with spaces
            if (currChar % charDelim == (charDelim - 1))
            {
                messageData << " ";
            }

            // Output current line and start a new line if at end of line
            if (currChar % lineWrap == (lineWrap - 1))
            {
                PAVP_MESESSION_VERBOSEMESSAGE("[%d/%d] %s", (currChar + 1), size, messageData.str().c_str());
                messageData.str("");
            }

            remainingBytes--;
            currChar++;
            it++;
        } while (remainingBytes > 0);

        // Output any remaining characters (could be the entire message if it was less than lineWrap characters long)
        if (!messageData.str().empty())
        {
            PAVP_MESESSION_VERBOSEMESSAGE("[%d] %s", size, messageData.str().c_str());
        }
#endif
    }
};
#endif
