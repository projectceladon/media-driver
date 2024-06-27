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

#ifndef _CP_HECI_SESSION_H
#define _CP_HECI_SESSION_H

#include "cp_heci_session_interface.h"

class CpHeciSession : public CHeciSessionInterface
{

public:

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      CpHeciSession constructor.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    CpHeciSession(std::shared_ptr<CPavpOsServices> pOsServices);


    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      CpHeciSession destructor.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~CpHeciSession();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief      Writes the given input data to MEI and reads the response.
    /// \par        Details:
    /// \li         The function sends the data in the Input buffer to MEI and reads the response into the
    ///             Output buffer.
    /// \li         If MEI returns IGD private data, it would be stored in the IgdData buffer.
    ///
    /// \param      pInput       [in] Pointer to the input data (FW command)
    /// \param      InputLen    [in] The buffer size of Input.
    /// \param      pOutput      [out] Pointer to the location in which the output data (FW response) will be saved.
    /// \param      OutputLen   [in] The buffer size of Output.
    /// \param      pIgdData     [out, optional] Pointer to the location in which the private driver data will be saved (if exists).
    /// \param      pIgdDataLen  [in, optional] pointer to size of IgdData.
    /// \return     PAVP_STATUS_SUCCESS upon success, a failed status otherwise.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    PAVP_STATUS IoMessage(
        uint8_t *pInput,   uint32_t   InputLen,
        uint8_t *pOutput,  uint32_t   OutputLen,
        uint8_t *pIgdData, uint32_t   *pIgdDataLen);

private:
    std::shared_ptr<CPavpOsServices> m_OsServices;

    // Copy constructor and assignment operator should not be used.
    CpHeciSession& operator=(const CpHeciSession& other);
    CpHeciSession(const CpHeciSession& other);

};

#endif // _CP_HECI_SESSION_H