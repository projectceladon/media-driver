/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2020
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
of the Materials, either expressly, by implication, inducement, estoppel or
otherwise. Any license under such intellectual property rights must be express
and approved by Intel in writing.

======================= end_copyright_notice ==================================*/

#ifndef PXP_STATE_MACHINE_H
#define PXP_STATE_MACHINE_H

#include "pavp_types.h"

//===========================================================================
// typedef:
//        _PXP_SM_QUERY_CP_Context
//
// Description:
//     KMD returns CP Context
//---------------------------------------------------------------------------
typedef struct _PXP_SM_QUERY_CP_Context
{
    uint32_t                bsessionalive;          // out - true if session is alive else false
    uint32_t                PxpSessionId;           // in  - Session ID, out CP Context
} PXP_SM_QUERY_CP_Context;

//===========================================================================
// typedef:
//        _PXP_ESC_SET_SESSION_STATUS_PARAMS
//
// Description:
//     The SessionType is input from the application passed down
//     to KMD determining what pool the session is allocated from.
//     The SessionIndex is returned from the KMD if an allocation
//     is available. The session status is returned from the KMD
//     to indicate the status of this particular session.
//     ReqSessionState is UMD request to KMD for partuicular session.
//---------------------------------------------------------------------------
typedef struct _PXP_SM_SET_SESSION_STATUS_PARAMS
{
    uint32_t                PxpSessionId;       // in[optional] - Inplay, Terminate, out[optional] - CPContext for session Init
    KM_PAVP_SESSION_TYPE    SessionType;        // in - session type
    KM_PAVP_SESSION_MODE    SessionMode;        // in - session mode
    PXP_SM_SESSION_REQ      ReqSessionState;    // in - new session state
} PXP_SM_SET_SESSION_STATUS_PARAMS;

//==========================================================================================================
// typedef:
//        _PXP_ACTION_TEE_COMMUNICATION_PARAMS
//
// Description:
//     This structure defined input output parameters required to communicate with CSME/GSC via KMD
//----------------------------------------------------------------------------------------------------------
typedef struct _PXP_ACTION_TEE_COMMUNICATION_PARAMS
{
    uint8_t     *msg_in;                    // in - pointer to input buffer
    uint32_t    msg_in_size;                // in - size of input pointer
    uint8_t     *msg_out;                   // out - pointer to output buffer
    uint32_t    msg_out_size;               // out - size of FW output message sent by FW
    uint32_t    msg_out_buf_size;           // in - size of output buffer
} PXP_ACTION_TEE_COMMUNICATION_PARAMS;

//==========================================================================================================
// typedef:
//        _PXP_ACTION_HOST_SESSIOn_HANDLE_PARAMS
//
// Description:
//     This structure defined input output parameters required to obtain host session handle
//----------------------------------------------------------------------------------------------------------
typedef struct _PXP_HOST_SESSION_HANDLE_REQUEST_PARAMS
{
    uint32_t request_type;          // in - type of request for host-session-handle operation */
    uint64_t host_session_handle;   // out - host session handle */
} PXP_HOST_SESSION_HANDLE_REQUEST_PARAMS;

#pragma pack(4)
typedef struct _PXP_PROTECTION_INFO
{
    PXP_UMD_REQUEST             Action;
    PXP_SM_STATUS               eSMStatus;
    union {
        PXP_SM_QUERY_CP_Context                 QueryCpContext;
        PXP_SM_SET_SESSION_STATUS_PARAMS        SetSessionStatus;
        PXP_ACTION_TEE_COMMUNICATION_PARAMS     TEEIoMessage;
        PXP_HOST_SESSION_HANDLE_REQUEST_PARAMS  HostSessionHandleReq;
    };
} PXP_PROTECTION_INFO;
#pragma pack()
#endif /* PXP_STATE_MACHINE_H */
