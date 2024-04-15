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
//! \file     codechal_secure_decode_vp9.h
//! \brief    Impelements the public interface for CodecHal Secure Decode 
//!
#ifndef __CODECHAL_SECURE_DECODE_VP9_H__
#define __CODECHAL_SECURE_DECODE_VP9_H__

#include "codechal_decode_vp9.h"
#include "codechal_secure_decode.h"

class CodechalSecureDecodeVp9 : public CodechalSecureDecode
{
public:
    //!
    //! \brief  Constructor
    //!
    CodechalSecureDecodeVp9(CodechalHwInterface *hwInterface);

    //!
    //! \brief  Destructor
    //!
    ~CodechalSecureDecodeVp9();

    //!
    //! \brief  Reset VP9 SegId Buffer With Huc
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS ResetVP9SegIdBufferWithHuc(
        void                    *state,
        PMOS_COMMAND_BUFFER     cmdBuffer);

    //!
    //! \brief  Update VP9 ProbBuffer With Huc
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS UpdateVP9ProbBufferWithHuc(
        bool                    isFullUpdate,
        void                    *state,
        PMOS_COMMAND_BUFFER     cmdBuffer);

    //!
    //! \brief  Add VP9 AesState
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS AddHcpSecureState(
        PMOS_COMMAND_BUFFER             cmdBuffer,
        void                            *state);

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

private:

    //!
    //! \brief  Prob Buffer Full Update
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS ProbBufFullUpdate(
        void                        *state,
        PMOS_COMMAND_BUFFER         cmdBuffer);

    //!
    //! \brief  Prob Buffer Partial Update
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS ProbBufPartialUpdate(
        void                    *state,
        PMOS_COMMAND_BUFFER     cmdBuffer);

    //!
    //! \brief  Set Huc Dmem Params
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS SetHucDmemParams(
        CodechalDecodeVp9*                 vp9State,
        PMOS_INTERFACE                     osInterface,
        PCODECHAL_DECODE_VP9_PROB_UPDATE   probUpdate);
};

#endif // __CODECHAL_SECURE_DECODE_VP9_H__

