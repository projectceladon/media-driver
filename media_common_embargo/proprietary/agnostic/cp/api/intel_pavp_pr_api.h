/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2010-2011
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

File Name: intel_pavp_pr_api.h

Abstract: 
Play Ready (PR) API is an interface for play ready specific commands that
should be dealt with at outer most layer without pushing them to PCH layer

Environment:

Notes:

======================= end_copyright_notice ==================================*/
#ifndef _INTEL_PAVP_PR_API_H
#define _INTEL_PAVP_PR_API_H

#include "intel_pavp_epid_api.h"
#include "intel_pavp_core_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push)
#pragma pack(1)

#define CMD_PLAYREADY_GENERAL           0x000D0004
#define CMD_MIRACAST_RX_GENERAL         0x0003003B
#define VER_PLAYREADY_PAVP_API          0x00030000
#define VER_MIRACAST_RX_PAVP_API        0x00010000 //version 1.0
#define VER_MIRACAST_RX_PAVP_API_V2     0x00020000 //version 2.0
#define MIRACAST_RX_GET_R_IV            0x00030025

typedef struct _PavpPlayReadyInputBuffer
{
    struct _SerializationHeader
    {
        PAVPCmdHdr  PavpHeader;
        DWORD       Reserved1;
        DWORD       Reserved2;
        DWORD       dwMaxOutputSize;
        DWORD       dwMessageSize;
    } SerializationHeader;
    BYTE            pbPayload[1];  // Beginning of a variable length byte array containing the payload
} PavpPlayReadyInputBuffer;

typedef struct _PavpPlayReadyOutputBuffer
{
    struct _SerializationHeader
    {
        PAVPCmdHdr  PavpHeader;
        DWORD       dwMessageSize;
    } SerializationHeader;
    BYTE            pbPayload[1];  // Beginning of a variable length byte array containing the payload
} PavpPlayReadyOutputBuffer;

typedef enum
{
    PAVP_SESSION_USE_VIDEO    = 0,
    PAVP_SESSION_USE_AUDIO    = 1,
    PAVP_SESSION_USE_INVALID  = 0xFF
} PAVP_SESSION_USE;

typedef struct _OemPolicyInfo
{
    BYTE                pavpType;
    BYTE                drmType;
    BYTE                pavpUse;
    BYTE                version;
    PAVP_SESSION_MODE   pavpMode;
} OemPolicyInfo;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif
#endif
