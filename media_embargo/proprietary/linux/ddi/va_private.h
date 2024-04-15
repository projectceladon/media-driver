/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2009-2020
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

File Name: va_private.h
Abstract: libva private API head file

Environment: Linux/Android

Notes:

======================= end_copyright_notice ==================================*/
#ifndef __VA_PRIVATE_H__
#define __VA_PRIVATE_H__

#include <va/va_vpp.h>

// Trigger Codec Hang buffer type
#define VATriggerCodecHangBufferType    -16

#define EXTERNAL_ENCRYPTED_SURFACE_FLAG (1<<16)

// Inform driver not to do any aligment for the buffer
#define BUFFER_INFO_BYPASS_FLAG         (1<<15)

//HDCP status report buffer flag
#define HDCP2_CODED_BUF_FLAG            (1<<8)

//0xf is reserved for WiDi
#define WIDI_SESSION_ID                 (0xf)

//HDCP enable flag
#define VA_HDCP_ENABLED                 (1<<12)

//ECB input flag
#define VA_ECB_INPUT_ENABLED            (1<<13)

//PAVP enable flag
#define VA_PAVP_ENABLED                 (1<<14)

//0x80 is reserved for transcode session
#define TRANSCODE_DEFAULT_SESSION_ID    (0x80)

#define PAVP_MAX_SESSION_ID             (0x10)

// For encode auto tear down reporting
#ifndef VA_CODED_BUF_STATUS_HW_TEAR_DOWN
#define VA_CODED_BUF_STATUS_HW_TEAR_DOWN   0x4000
#endif

#define VA_ENCRYPTION_UNSUPPORTED 0       // Codec encryption unsupported flag is 0.
#define VA_ENCRYPTION_SUPPORTED   1       // Codec encryption supported flag is 1.

#define VA_ENCRYPTION_NO          0       // None encryption in codec flag is 0.
#define VA_ENCRYPTION_CASCADE     1       // Cascade encryption in codec flag is 1.
#define VA_ENCRYPTION_PAVP        2       // PAVP encryption in codec flag is 2.

// tiling format support
#define VA_TILE_LINEAR                      1
#define VA_TILE_Y                           (1<<1)
#define VA_TILE_X                           (1<<2)
#define VAConfigAttribInputTiling           0xffffffff

// LE<->BE translation of DWORD
#define SwapEndian_DW(dw)    ( (((dw) & 0x000000ff) << 24) |  (((dw) & 0x0000ff00) << 8) | (((dw) & 0x00ff0000) >> 8) | (((dw) & 0xff000000) >> 24) )
// LE<->BE translation of 8 byte big number
#define SwapEndian_8B(ptr)                                                    \
{                                                                            \
    unsigned int Temp = 0;                                                    \
    Temp = SwapEndian_DW(((unsigned int*)(ptr))[0]);                        \
    ((unsigned int*)(ptr))[0] = SwapEndian_DW(((unsigned int*)(ptr))[1]);    \
    ((unsigned int*)(ptr))[1] = Temp;                                        \
}

// LE<->BE translation of DWORD
#define SwapEndian_DW(dw)    ( (((dw) & 0x000000ff) << 24) |  (((dw) & 0x0000ff00) << 8) | (((dw) & 0x00ff0000) >> 8) | (((dw) & 0xff000000) >> 24) )
// LE<->BE translation of 8 byte big number
#define SwapEndian_8B(ptr)                                                    \
{                                                                            \
    unsigned int Temp = 0;                                                    \
    Temp = SwapEndian_DW(((unsigned int*)(ptr))[0]);                        \
    ((unsigned int*)(ptr))[0] = SwapEndian_DW(((unsigned int*)(ptr))[1]);    \
    ((unsigned int*)(ptr))[1] = Temp;                                        \
}

typedef enum
{
    VA_DECRYPT_STATUS_SUCCESSFUL = 0,
    VA_DECRYPT_STATUS_INCOMPLETE,
    VA_DECRYPT_STATUS_ERROR,
    VA_DECRYPT_STATUS_UNAVAILABLE
} VADecryptStatus;

typedef struct _VAEncMiscParameterVP8Status // added for VP8 for StatusReport for long-term reference insertion
{
    /** VAEncMiscParameterVP8StatusFinal quantization index used (yac), determined by BRC.
     *  Application is providing quantization index deltas
     *  ydc(O), y2dc(1), y2ac(2), uvdc(3), uvac(4) that are applied to all segments
     *  and segmentation qi deltas, they will not be changed by BRC.
     */
    unsigned short  quantization_index;
    /** Final loopfilter levels for the frame, if segmentation is disabled
     *  only index 0 is used.
     * If loop_filter_level[] is 0, it indicates loop filter is disabled.
     */
    char            loop_filter_level[4];
    /** Long term reference frame indication from BRC.  BRC recommends the
     *  current frame that is being queried is a good candidate for a long
     *  term reference.
     */
    char            long_term_indication;
} VAEncMiscParameterVP8Status;

// Private API for openCL, share the surface between media driver and openCL
// The surfaces are created in media driver, and accessed in openCL
// Need to access the object buffer name with drm_intel_bo_flink()
#define VPG_EXT_GET_SURFACE_HANDLE  "DdiMedia_ExtGetSurfaceHandle"

// only support query the surface id one by one
typedef VAStatus (*vaExtGetSurfaceHandle)(
    VADisplay          dpy,
    VASurfaceID        *surface,
    unsigned int       *id_handle);

// Private API to extend vaMapBuffer to include read/write flag
#define VA_MAPBUFFER2_READ_FLAG      0x00000001
#define VA_MAPBUFFER2_WRITE_FLAG     0x00000002

#define VPG_EXT_VA_MAPBUFFER2  "DdiMedia_MapBuffer2"

typedef VAStatus (*vaExtMapBuffer2)(
    VADisplay           dpy,
    VABufferID          buf_id,
    void                **pbuf,
    int                 flag);


typedef struct _VAHDCP2EncryptionStatusBuffer{
    unsigned int       status;
    bool               bEncrypted;
    unsigned int       counter[4];
}VAHDCP2EncryptionStatusBuffer;

// XeHP related changes
//
// New configuration attribute used to query number of GPU nodes available using
// vaGetConfigAttributes().
// The same attribute is also used to select the GPU node when a config is created.
#define VAConfigAttribGPUNodes  100

//helper data structure to be used by config and surface attributes for exposing GPU nodes
typedef union _VAGPUNodes
{
    struct
    {
        uint32_t num_nodes : 5; // can expose a maxinum number of 16 nodes
        uint32_t padding : 11; // reserved bits for future use
        uint32_t node_masks : 16; // bit mask to indicate which node should be used
    } bits;
    uint32_t value;
} VAGPUNodes;

typedef VAGPUNodes VAConfigAttribValGPUNodes;

// New surface attribute type used to query number of GPU nodes available
// The value for this attribute type is in the form of VAGPUNodes
// This attribute can be used to select the GPU node when a surface is created.
#define VASurfaceAttribGPUNodes 100

#endif

