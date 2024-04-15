/*
// Copyright (C) 2019 Intel Corporation
//
// Licensed under the Apache License,Version 2.0(the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
//!
//! \file     codechal_secure_decode_av1.h
//! \brief    Impelements the public interface for CodecHal Secure Decode 
//!
#ifndef __CODECHAL_SECURE_DECODE_AV1_H__
#define __CODECHAL_SECURE_DECODE_AV1_H__

#include "codechal_decode_av1.h"
#include "codechal_secure_decode.h"

class CodechalSecureDecodeAv1 : public CodechalSecureDecode
{
public:
    //!
    //! \brief  Constructor
    //!
    CodechalSecureDecodeAv1(CodechalHwInterface *hwInterface);

    //!
    //! \brief  Destructor
    //!
    ~CodechalSecureDecodeAv1();

    //!
    //! \brief  Allocate Resource
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS AllocateResource(void *state);

    //!
    //! \brief  Execute Huc StreamOut
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS Execute(void *state);

    //!
    //! \brief  Set Bitstream Buffer for Decode
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS SetBitstreamBuffer(MHW_VDBOX_IND_OBJ_BASE_ADDR_PARAMS *indObjBaseAddrParams);

    //!
    //! \brief Encrypt default cdf table buffer through HuC copy
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS ProtectDefaultCdfTableBuffer(
        void *state,
        uint32_t bufIndex,
        PMOS_COMMAND_BUFFER cmdBuffer);

    //!
    //! \brief Return the AV1 temp cdf table buffer
    //! \return PMOS_RESOURCE
    //!
    virtual PMOS_RESOURCE GetTempCdfTableBuffer()
    {
        m_uiTempCdfBufIndex %= av1DefaultCdfTableNum;
        return &m_resTempCdfTableBuf[m_uiTempCdfBufIndex++];
    }

private:
    MOS_RESOURCE    m_resTempCdfTableBuf[av1DefaultCdfTableNum];        //!< cdf table init data is wrote to this buffer at frist, then huc copy to the dest buffer
    uint32_t        m_uiTempCdfBufIndex;                                //!< to indicate which cdf temp buffer should be used
};

#endif // __CODECHAL_SECURE_DECODE_VP9_H__