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

#include "i915_pxp.h"
#include "xf86drm.h"
#include "mos_pxp_sm.h"
#include <string.h>

PXP_SM_STATUS Int2StateMachineStatus( uint32_t Status)
{
    switch(Status)
    {
        case PRELIM_DRM_I915_PXP_OP_STATUS_SUCCESS:
            return PXP_SM_STATUS_SUCCESS;
        case PRELIM_DRM_I915_PXP_OP_STATUS_RETRY_REQUIRED:
            return PXP_SM_STATUS_RETRY_REQUIRED;
        case PRELIM_DRM_I915_PXP_OP_STATUS_SESSION_NOT_AVAILABLE:
            return PXP_SM_STATUS_SESSION_NOT_AVAILABLE;
        case PRELIM_DRM_I915_PXP_OP_STATUS_INSUFFICIENT_RESOURCES:
            return PXP_SM_STATUS_INSUFFICIENT_RESOURCE;
        case PRELIM_DRM_I915_PXP_OP_STATUS_ERROR_UNKNOWN:
            return PXP_SM_STATUS_ERROR_UNKNOWN;
    }
    return PXP_SM_STATUS_ERROR_UNKNOWN;
}

int32_t mos_pxp_sm_submit(int32_t fd, PXP_PROTECTION_INFO* pPxpInfo)
{
    int32_t ret = 0;
    prelim_drm_i915_pxp_ops pxpOps;
    memset(&pxpOps, 0, sizeof(pxpOps));

    if (pPxpInfo == nullptr)
    {
        return -1;
    }
    switch (pPxpInfo->Action)
    {
        case PXP_ACTION_SET_SESSION_STATUS:
            prelim_drm_i915_pxp_set_session_status_params pxpSetSessionStatus;
            memset(&pxpSetSessionStatus, 0, sizeof(pxpSetSessionStatus));
            pxpSetSessionStatus.pxp_tag             = pPxpInfo->SetSessionStatus.PxpSessionId;

            switch (pPxpInfo->SetSessionStatus.SessionMode)
            {
                case KM_PAVP_SESSION_MODE_LITE:
                    pxpSetSessionStatus.session_mode = PRELIM_DRM_I915_PXP_MODE_LM;
                break;

                case KM_PAVP_SESSION_MODE_HEAVY:
                    pxpSetSessionStatus.session_mode = PRELIM_DRM_I915_PXP_MODE_HM;
                break;

                case KM_PAVP_SESSION_MODE_STOUT:
                    pxpSetSessionStatus.session_mode = PRELIM_DRM_I915_PXP_MODE_SM;
                break;

                default:
                    return -1; // no other mode is supported other than LM, HM, SM
            }

            //pxpSetSessionStatus.session_mode will be added later ad KMD ignores it for now

            switch (pPxpInfo->SetSessionStatus.ReqSessionState)
            {
                case PXP_SM_REQ_SESSION_ID_INIT:
                    pxpSetSessionStatus.req_session_state = PRELIM_DRM_I915_PXP_REQ_SESSION_ID_INIT;
                break;

                case PXP_SM_REQ_SESSION_IN_PLAY:
                    pxpSetSessionStatus.req_session_state = PRELIM_DRM_I915_PXP_REQ_SESSION_IN_PLAY;
                break;

                case PXP_SM_REQ_SESSION_TERMINATE:
                    pxpSetSessionStatus.req_session_state = PRELIM_DRM_I915_PXP_REQ_SESSION_TERMINATE;
                break;

                default:
                    return -1;
            }

            pxpSetSessionStatus.session_type = pPxpInfo->SetSessionStatus.SessionType;
            pxpOps.action = PRELIM_DRM_I915_PXP_ACTION_SET_SESSION_STATUS ;
            pxpOps.params = (uint64_t)&pxpSetSessionStatus;

            ret = drmIoctl(fd, PRELIM_DRM_IOCTL_I915_PXP_OPS, &pxpOps);

            pPxpInfo->eSMStatus                     = Int2StateMachineStatus(pxpOps.status);
            pPxpInfo->SetSessionStatus.PxpSessionId = pxpSetSessionStatus.pxp_tag;
        break;

        case PXP_ACTION_TEE_COMMUNICATION:
            prelim_drm_i915_pxp_tee_io_message_params pxpTEEMsg;
            memset(&pxpTEEMsg, 0, sizeof(pxpTEEMsg));
            pxpTEEMsg.msg_in            = uint64_t(pPxpInfo->TEEIoMessage.msg_in);
            pxpTEEMsg.msg_in_size       = pPxpInfo->TEEIoMessage.msg_in_size;
            pxpTEEMsg.msg_out           = uint64_t(pPxpInfo->TEEIoMessage.msg_out);
            pxpTEEMsg.msg_out_buf_size  = pPxpInfo->TEEIoMessage.msg_out_buf_size;

            pxpOps.action = PRELIM_DRM_I915_PXP_ACTION_TEE_IO_MESSAGE;
            pxpOps.params = uint64_t(&pxpTEEMsg);

            ret = drmIoctl(fd, PRELIM_DRM_IOCTL_I915_PXP_OPS, &pxpOps);

            pPxpInfo->eSMStatus                 = Int2StateMachineStatus(pxpOps.status);
            pPxpInfo->TEEIoMessage.msg_out_size = pxpTEEMsg.msg_out_ret_size;
        break;

        case PXP_ACTION_QUERY_CP_CONTEXT:
            prelim_drm_i915_pxp_query_tag pxpQueryTag;
            memset(&pxpQueryTag, 0, sizeof(pxpQueryTag));
            pxpQueryTag.session_is_alive    = pPxpInfo->QueryCpContext.bsessionalive;
            pxpQueryTag.pxp_tag             = pPxpInfo->QueryCpContext.PxpSessionId;

            pxpOps.action = PRELIM_DRM_I915_PXP_ACTION_QUERY_PXP_TAG;
            pxpOps.params = uint64_t(&pxpQueryTag);

            ret = drmIoctl(fd, PRELIM_DRM_IOCTL_I915_PXP_OPS, &pxpOps);

            pPxpInfo->eSMStatus                     = Int2StateMachineStatus(pxpOps.status);
            pPxpInfo->QueryCpContext.PxpSessionId   = pxpQueryTag.pxp_tag;
            pPxpInfo->QueryCpContext.bsessionalive  = pxpQueryTag.session_is_alive;
        break;

        case PXP_ACTION_HOST_SESSION_HANDLE_REQ:
            prelim_drm_i915_pxp_host_session_handle_request sessionHandleReq;
            memset(&sessionHandleReq, 0, sizeof(sessionHandleReq));
            sessionHandleReq.request_type           = pPxpInfo->HostSessionHandleReq.request_type;

            pxpOps.action = PRELIM_DRM_I915_PXP_ACTION_HOST_SESSION_HANDLE_REQ;
            pxpOps.params = uint64_t(&sessionHandleReq);

            ret = drmIoctl(fd, PRELIM_DRM_IOCTL_I915_PXP_OPS, &pxpOps);

            pPxpInfo->eSMStatus                                     = Int2StateMachineStatus(pxpOps.status);
            pPxpInfo->HostSessionHandleReq.host_session_handle      = sessionHandleReq.host_session_handle;
        break;

        default:
            return -1;
    }

    return ret;
}
