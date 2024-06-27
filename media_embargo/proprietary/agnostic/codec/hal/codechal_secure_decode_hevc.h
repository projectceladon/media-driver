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
//! \file     codechal_secure_decode_hevc.h
//! \brief    Impelements the public interface for CodecHal Secure Decode 
//!
#ifndef __CODECHAL_SECURE_DECODE_HEVC_H__
#define __CODECHAL_SECURE_DECODE_HEVC_H__

#include "codechal_decode_hevc.h"
#include "codechal_secure_decode.h"

typedef struct
{   // CP
    uint8_t     AESEnable : 1;
    uint8_t     IsPlayReady3_0InUse : 1;
    uint8_t     IsCTRMode : 1;
    uint8_t     CTRFlavor : 2;
    uint8_t     IsFairPlay : 1;
    uint8_t     InitialFairPlayEnabled : 1;
    uint8_t     IsPlayReady3_0HcpSupported : 1;

    //Fair Play
    uint16_t    InitPktBytes;
    uint32_t    EncryptedBytes;
    uint32_t    ClearBytes;
} HUC_HEVC_S2L_PIC_BSS_EXT, *PHUC_HEVC_S2L_PIC_BSS_EXT;

typedef struct
{
    // CP
    uint32_t    InitializationCounterValue0;
    uint32_t    InitializationCounterValue1;
    uint32_t    InitializationCounterValue2;
    uint32_t    InitializationCounterValue3;
}HUC_HEVC_S2L_SLICE_BSS_EXT, *PHUC_HEVC_S2L_SLICE_BSS_EXT;

class CodechalSecureDecodeHevc : public CodechalSecureDecode
{
public:

    //!
    //! \brief  Constructor
    //!
    CodechalSecureDecodeHevc(CodechalHwInterface *hwInterface);

    //!
    //! \brief  Destructor
    //!
    ~CodechalSecureDecodeHevc();

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
    //! \brief  Set Hevc Huc Dmem S2L
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS SetHevcHucDmemS2LBss(
        void          *state,
        void          *hucPicBss,
        void          *hucSliceBss);

    //!
    //! \brief  Add Hevc Huc AesState
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS AddHucSecureState(
        void                       *state,
        void                       *sliceState,
        PMOS_COMMAND_BUFFER        cmdBuffer);

    //!
    //! \brief  Set Huc StreamObject Params
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS SetHucStreamObj(
        PMHW_VDBOX_HUC_STREAM_OBJ_PARAMS hucStreamObjParams);

    //!
    //! \brief  Add Hevc Hcp AesState
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS AddHcpSecureState(
        PMOS_COMMAND_BUFFER             cmdBuffer,
        void                            *sliceState);

private:

    //!
    //! \brief  Get Slice Init Value
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS GetSliceInitValue(
        PMHW_PAVP_AES_PARAMS            params,
        uint32_t                        *value);

    uint32_t InitialCounterPerSlice[CODECHAL_HEVC_MAX_NUM_SLICES_LVL_6][4]; //!< Initial AES counter for each slice
};

#endif // __CODECHAL_SECURE_DECODE_HEVC_H__

