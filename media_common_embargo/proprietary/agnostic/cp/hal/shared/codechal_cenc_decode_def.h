/*
// Copyright (C) 2022 Intel Corporation
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
//! \file      codechal_cenc_decode_def.h
//! \brief     cenc decoder common definition
//!
#ifndef __CODECHAL_CENC_DECODE_DEF_H__
#define __CODECHAL_CENC_DECODE_DEF_H__

#include <vector>
#include "mhw_cp.h"
#include "mhw_vdbox.h"
#include "heap_manager.h"
#include "mos_defs.h"
#include "codec_def_cenc_decode.h"
#include "codec_def_decode.h"
#include "codechal_setting.h"
#include "cp_drm_defs.h"


//------------------------------------------------------------------------------
// Macros specific to MOS_CP_SUBCOMP_CENC sub-comp
//------------------------------------------------------------------------------
#define CENC_DECODE_ASSERT(_expr) \
    MOS_ASSERT(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_CENC, _expr)

#define CENC_DECODE_ASSERTMESSAGE(_message, ...) \
    MOS_ASSERTMESSAGE(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_CENC, _message, ##__VA_ARGS__)

#define CENC_DECODE_COND_ASSERTMESSAGE(_expr, _message, ...) \
    if (_expr)                                               \
    {                                                        \
        CENC_DECODE_ASSERTMESSAGE(_message, ##__VA_ARGS__)   \
    }

#define CENC_DECODE_NORMALMESSAGE(_message, ...) \
    MOS_NORMALMESSAGE(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_CENC, _message, ##__VA_ARGS__)

#define CENC_DECODE_VERBOSEMESSAGE(_message, ...) \
    MOS_VERBOSEMESSAGE(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_CENC, _message, ##__VA_ARGS__)

#define CENC_DECODE_FUNCTION_ENTER \
    MOS_FUNCTION_ENTER(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_CENC)

#define CENC_DECODE_CHK_STATUS(_stmt) \
    MOS_CHK_STATUS_RETURN(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_CENC, _stmt)

#define CENC_DECODE_CHK_NULL_NO_STATUS(_ptr) \
    MOS_CHK_NULL_NO_STATUS_RETURN(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_CENC, _ptr)

#define CENC_DECODE_CHK_NULL(_ptr) \
    MOS_CHK_NULL_RETURN(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_CENC, _ptr)

#define CENC_DECODE_CHK_COND(_expr, _message, ...) \
    MOS_CHK_COND_RETURN(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_CENC, _expr, _message, ##__VA_ARGS__)

// Forward Declarations
class CodechalDebugInterface;
class MhwVdboxMiInterface;
class MhwVdboxHucInterface;
class MhwVdboxHcpInterface;
class MhwVdboxMfxInterface;
class MhwCpBase;

//-----------------------------------------------------------------------------
// structure of resources used by Huc for decrpytion
//-----------------------------------------------------------------------------
#define CODECHAL_NUM_DECRYPT_BUFFERS            8
#define CODECHAL_NUM_DECRYPT_STATUS_BUFFERS     8
#define CODECHAL_NUM_IND_CRYPTO_BUFFERS         512
#define CODECHAL_DECRYPT_DELAY_START_DEFAULT    16
#define CODECHAL_DECRYPT_DELAY_STEP_DEFAULT     32
#define CODECHAL_DECRYPT_DELAY_THRESHOLD        4
#define CODECHAL_DECRYPT_DELAY_NUM_BUFFERS      8
#define CODECHAL_INCOMPLETE_REPORTS_TH1         1950
#define CODECHAL_INCOMPLETE_REPORTS_TH2         1960

typedef struct _CODECHAL_CENC_PARAMS
{
    uint16_t ui16StatusReportFeedbackNumber;
    uint8_t  ui8SizeOfLength;

    uint32_t dwNumSegments;
    void     *pSegmentInfo;

    PMOS_RESOURCE presDataBuffer;
    uint32_t      dwDataBufferSize;
    bool          bSerialMode;
} CODECHAL_CENC_PARAMS, *PCODECHAL_CENC_PARAMS;

typedef struct _CODECHAL_CENC_STATUS
{
    uint32_t dwHWStoredData;         // Value stored by HuC engine
    uint32_t dwSWStoredData;         // SW(driver) stored value
    uint32_t dwErrorStatus2Mask;     // Value of mask for HUC_STATUS2 register
    uint32_t dwErrorStatus2Reg;      // Value of HUC_STATUS2 register
    uint32_t dwKernelHeaderInfoReg;  // Value of HUC_uKERNEL_HDR_INFO register
    uint32_t dwErrorStatusReg;       // Value of HUC_STATUS register
    uint32_t dwFrameNum;
} CODECHAL_CENC_STATUS, *PCODECHAL_CENC_STATUS;

typedef struct _CODECHAL_CENC_STATUS_BUFFER
{
    MOS_RESOURCE resStatusBuffer;  // Handle of status buffer
    uint8_t      *pData;
    uint32_t     dwSWStoreData;
    uint32_t     dwReportSize;
    uint16_t     wFirstIndex;
    uint16_t     wCurrIndex;
    uint8_t      ucGlobalStoreDataLength;  // size of gloabl HW/SW store value

    // MMIO register offsets inside CODECHAL_DECRYPT_STATUS
    uint8_t ucErrorStatus2MaskOffset;
    uint8_t ucErrorStatus2Offset;
    uint8_t ucKernelHeaderInfoOffset;
    uint8_t ucDecryptErrorStatusOffset;
} CODECHAL_CENC_STATUS_BUFFER, *PCODECHAL_CENC_STATUS_BUFFER;

//#define CODECHAL_HUC_KERNEL_DEBUG

#ifdef CODECHAL_HUC_KERNEL_DEBUG
#define CODECHAL_HUC_KERNEL_DEBUG_MAX_EVENT_NUMBER 8 * 1024
#define CODECHAL_HUC_KERNEL_DEBUG_MAX_STREAM_BUFFER_DWORD_LENGTH 64 * 1024

typedef enum _CODECHAL_HUC_KERNEL_DEBUG_event_type
{
    CODECHAL_HUC_KERNEL_DEBUG_EVENT_TYPE_NAL_SPS        = 1,
    CODECHAL_HUC_KERNEL_DEBUG_EVENT_TYPE_NAL_PPS        = 2,
    CODECHAL_HUC_KERNEL_DEBUG_EVENT_TYPE_NAL_SLICE      = 3,
    CODECHAL_HUC_KERNEL_DEBUG_EVENT_TYPE_STRING_ENTER   = 4,
    CODECHAL_HUC_KERNEL_DEBUG_EVENT_TYPE_STRING_EXIT    = 5,
    CODECHAL_HUC_KERNEL_DEBUG_EVENT_TYPE_STRING_GENERAL = 6,
} CODECHAL_HUC_KERNEL_DEBUG_event_type;

typedef enum _CODECHAL_HUC_KERNEL_DEBUG_level
{
    CODECHAL_HUC_KERNEL_DEBUG_EVENT_PARSER  = 0,
    CODECHAL_HUC_KERNEL_DEBUG_STREAM_DUMP   = 1,
    CODECHAL_HUC_KERNEL_DEBUG_REGION00_DUMP = 2,
} CODECHAL_HUC_KERNEL_DEBUG_level;

typedef struct _CODECHAL_HUC_KERNEL_DEBUG_EVENT
{
    uint32_t ui32HucKernelDebugEventType;

    union
    {
        uint32_t ui32HucKernelDebugEventPayload[7];

        /*NAL SPS PARSING EVENT*/
        struct
        {
            uint32_t ui32HucKernelDebugEventNalSpsId;
            uint32_t ui32HucKernelDebugEventNalSpsProfile;
            uint32_t ui32HucKernelDebugEventNalSpsLevel;
            uint32_t ui32HucKernelDebugEventNalSpsNumOfRef;
            uint32_t ui32HucKernelDebugEventNalSpsPicOrderCntType;
            uint32_t ui32HucKernelDebugEventNalSpsReserved[2];
        } HucKernelDebugEventNalSps;

        /*NAL PPS PARSING EVENT*/
        struct
        {
            uint32_t ui32HucKernelDebugEventNalPpsId;
            uint32_t ui32HucKernelDebugEventNalPpsEntropyFlag;
            uint32_t ui32HucKernelDebugEventNalPpsPicOrderPresentFlag;
            uint32_t ui32HucKernelDebugEventNalPpsNumSliceGroupMinus1;
            uint32_t ui32HucKernelDebugEventNalPpsReserved[3];
        } HucKernelDebugEventNalPps;
    };
} CODECHAL_HUC_KERNEL_DEBUG_EVENT, *PCODECHAL_HUC_KERNEL_DEBUG_EVENT;

typedef struct _CODECHAL_HUC_KERNEL_DEBUG_STREAM_BUFFER
{
    uint32_t HucKernelDebugStreamBufferLengthInByte;
    uint32_t HucKernelDebugStreamBufferPayload[CODECHAL_HUC_KERNEL_DEBUG_MAX_STREAM_BUFFER_DWORD_LENGTH];
} CODECHAL_HUC_KERNEL_DEBUG_STREAM_BUFFER, *PCODECHAL_HUC_KERNEL_DEBUG_STREAM_BUFFER;

typedef struct _CODECHAL_HUC_KERNEL_DEBUG_EVENT_LOG
{
    uint32_t                        EventCnt;
    CODECHAL_HUC_KERNEL_DEBUG_EVENT Event[CODECHAL_HUC_KERNEL_DEBUG_MAX_EVENT_NUMBER];
} CODECHAL_HUC_KERNEL_DEBUG_EVENT_LOG, *PCODECHAL_HUC_KERNEL_DEBUG_EVENT_LOG;

typedef struct _CODECHAL_HUC_KERNEL_DEBUG_INFO
{
    union
    {
        CODECHAL_HUC_KERNEL_DEBUG_EVENT_LOG HucEvent;

        CODECHAL_HUC_KERNEL_DEBUG_STREAM_BUFFER HucKernelDebugStream;
    };
} CODECHAL_HUC_KERNEL_DEBUG_INFO, *PCODECHAL_HUC_KERNEL_DEBUG_INFO;
#endif

typedef struct _CODECHAL_CENC_DMEM
{
    uint32_t dwRegion0Size;
    uint32_t dwRegion1Size;
    uint32_t dwRegion2Size;

    uint8_t  ui8SizeofLength;
    uint8_t  ucStatusReportIdx;
    uint16_t ui16StatusReportFeedbackNumber;

    uint32_t uiDashMode;
    uint32_t uiSliceHeaderParse;
    uint32_t StandardInUse;
    uint32_t ProductFamily;
    uint16_t RevId;
    uint32_t uiFulsimInUse;
    uint32_t uiAesFromHuC;
    uint32_t dwNumOfSegment;
    uint32_t uiDummyRefIdxState;
    uint32_t uiVP9PrimitiveCmdOffset;

#ifdef CODECHAL_HUC_KERNEL_DEBUG
    uint8_t hucKernelDebugLevel;
#endif
} CODECHAL_CENC_DMEM, *PCODECHAL_CENC_DMEM;

typedef struct MOS_ALIGNED(MHW_CACHELINE_SIZE) _CODECHAL_HUC_SEGMENT_INFO
{
    uint32_t dwSegmentStartOffset;
    uint32_t dwSegmentLength;
    uint32_t dwPartialAesBlockSizeInBytes;
    uint32_t dwInitByteLength;
    uint32_t dwAesCbcIvOrCtr[4];
} CODECHAL_HUC_SEGMENT_INFO, *PCODECHAL_HUC_SEGMENT_INFO;

//! \brief  Describes heap information per frame
typedef struct _CODECHAL_CENC_HEAP_INFO
{
    uint32_t     u32TrackerId;
    MemoryBlock  SecondLvlBbBlock;
    MemoryBlock  BitstreamBlock;
    MOS_RESOURCE resOriginalBitstream;
    uint32_t     u32OriginalBitstreamSize;
} CODECHAL_CENC_HEAP_INFO;

//!
//! \struct    CencHucStreamoutParams
//! \brief     Cenc Huc streamout parameters
//!
struct CencHucStreamoutParams
{
    CODECHAL_MODE mode;

    // Indirect object addr command params
    PMOS_RESOURCE dataBuffer;
    uint32_t      dataSize;    // 4k aligned
    uint32_t      dataOffset;  // 4k aligned
    PMOS_RESOURCE streamOutObjectBuffer;
    uint32_t      streamOutObjectSize;    // 4k aligned
    uint32_t      streamOutObjectOffset;  //4k aligned

    // Stream object params
    uint32_t indStreamInLength;
    uint32_t inputRelativeOffset;
    uint32_t outputRelativeOffset;

    // Segment Info
    void *segmentInfo;

    // Indirect Security State
    MOS_RESOURCE hucIndState;
    uint32_t     curIndEntriesNum;
    uint32_t     curNumSegments;
};

#define SizeOfInBits(__x__) (sizeof(__x__) * 8)
#define SIGNED_VALUE(code) (((code) > 0) ? (((code) << 1) - 1) : ((-((code)) << 1)))
#define COUNTOF(x) (sizeof(x) / sizeof(x[0]))
#define WRITE_BITS(value, writtenBits, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, bytesWritten, fixEmulation) \
    do                                                                                                                          \
    {                                                                                                                           \
        uint32_t bytesToBeWritten = (writtenBits + *accumulatorValidBits) / 8;                                                  \
        if ((writtenBits + *accumulatorValidBits) % 8 != 0)                                                                     \
        {                                                                                                                       \
            bytesToBeWritten++;                                                                                                 \
        }                                                                                                                       \
        if ((bytesToBeWritten + *bytesWritten) > bufSize)                                                                       \
        {                                                                                                                       \
            return MOS_STATUS_NOT_ENOUGH_BUFFER;                                                                                \
        }                                                                                                                       \
        WriteBits(value, writtenBits, accumulator, accumulatorValidBits, outBitStreamBuf, bytesWritten, fixEmulation);          \
    } while (0);

#define ENCODE_EXP_GOLOMB(uValue, valueValidBits, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten)          \
    do                                                                                                                               \
    {                                                                                                                                \
        eStatus = EncodeExpGolomb(uValue, valueValidBits, bufSize, accumulator, accumulatorValidBits, outBitStreamBuf, byteWritten); \
        if (eStatus != MOS_STATUS_SUCCESS)                                                                                           \
        {                                                                                                                            \
            return eStatus;                                                                                                          \
        }                                                                                                                            \
    } while (0);

#define CENC_SEI_INTERNAL_BUFFER_SIZE 512

typedef struct _CODECHAL_CENC_SEI
{
    uint32_t payloadSize;
    uint8_t  data[CENC_SEI_INTERNAL_BUFFER_SIZE];
} CODECHAL_CENC_SEI, *PCODECHAL_CENC_SEI;

typedef union _HUC_STATUS
{
    struct
    {
        uint32_t MinuteIAInReset : BITFIELD_BIT(0);
        uint32_t BootRomCodeStatus : BITFIELD_RANGE(1, 7);
        uint32_t KernelOsStatus : BITFIELD_RANGE(8, 15);
        uint32_t P24CCoreStatus : BITFIELD_RANGE(16, 23);
        uint32_t Reserved : BITFIELD_RANGE(24, 31);
    } Fields;
    uint32_t Value;
} HUC_STATUS;

#endif  // __CODECHAL_CENC_DECODE_DEF_H__
