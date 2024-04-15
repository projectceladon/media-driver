/*
INTEL CONFIDENTIAL
Copyright 2014-2020 Intel Corporation All Rights Reserved.

The source code contained or described herein and all documents
related to the source code ("Material") are owned by Intel Corporation
or its suppliers or licensors. Title to the Material remains with
Intel Corporation or its suppliers and licensors. The Material
contains trade secrets and proprietary and confidential information of
Intel or its suppliers and licensors. The Material is protected by
worldwide copyright and trade secret laws and treaty provisions. No
part of the Material may be used, copied, reproduced, modified,
published, uploaded, posted, transmitted, distributed, or disclosed in
any way without Intel's prior express written permission.

No license under any patent, copyright, trade secret or other
intellectual property right is granted to or conferred upon you by
disclosure or delivery of the Materials, either expressly, by
implication, inducement, estoppel or otherwise. Any license under such
intellectual property rights must be express and approved by Intel in
writing.

--*/

/*
********************************************************************************
**
**  @file  pavp_oem_policy_api.h
**
**  @brief  Header file for PAVP OEM policy API, not to be exposed outside of Intel
**
********************************************************************************
*/


#ifndef _PAVP_OEM_POLICY_API_H_
#define _PAVP_OEM_POLICY_API_H_

#define PAVP_OEM_POLICY_BLOB_VERSION_V1         0x1

typedef enum
{
    PAVP_SESSION_CONTENT_TYPE_VIDEO    = 0,
    PAVP_SESSION_CONTENT_TYPE_AUDIO    = 1,
    PAVP_SESSION_CONTENT_TYPE_INVALID  = 0xff
} pavp_session_content_type_e;

#pragma pack(push)
#pragma pack(1)

typedef struct _intel_oem_policy_t
{
    uint8_t     pavp_type;
    uint8_t     drm_type;
    uint8_t     content_type;
    uint8_t     version;
    uint32_t    pavp_mode;
} intel_oem_policy_t;

typedef union _pavp_stream_extended_t
{
    uint8_t    app_id;
    uint32_t   dword;
    struct
    {
        uint8_t pavp_session_index :7;
        uint8_t app_type           :1;
        uint8_t drm_type;
        uint8_t content_type;
        uint8_t is_valid;
    } fields;
} pavp_stream_extended_t;

#pragma pack(pop)

#endif // _PAVP_OEM_POLICY_API_H_
