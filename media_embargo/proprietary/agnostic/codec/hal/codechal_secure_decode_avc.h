/*
// Copyright (C) 2017 Intel Corporation
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
//! \file     codechal_secure_decode_avc.h
//! \brief    Impelements the public interface for CodecHal Secure Decode 
//!
#ifndef __CODECHAL_SECURE_DECODE_AVC_H__
#define __CODECHAL_SECURE_DECODE_AVC_H__

#include "codechal_decode_avc.h"
#include "codechal_secure_decode.h"
#include "mhw_cp.h"

class CodechalSecureDecodeAvc : public CodechalSecureDecode
{
public:

    //!
    //! \brief  Constructor
    //!
    CodechalSecureDecodeAvc(CodechalHwInterface *hwInterface);

    //!
    //! \brief  Destructor
    //!
    ~CodechalSecureDecodeAvc();

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

    uint32_t hucStreamOutBufOffset = 0; //!< Offset of HuC stream out buffer
};

#endif // __CODECHAL_SECURE_DECODE_AVC_H__
