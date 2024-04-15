/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2012-2013
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

File Name: cp_heci_definitions.h

Abstract:
   Defines ME firmware commands for use inside PCP.


Environment:
    OS agnostic

Notes:
    None

======================= end_copyright_notice ==================================*/

#ifndef __CP_HECI_DEFINITIONS_H__
#define __CP_HECI_DEFINITIONS_H__

#define MDRM_OUT_MSG_FLAG 0x80000000

typedef enum {
   /*mdrm apis*/
   wv2_begin = 0x000C0001,
   wv2_open_session = wv2_begin,
   wv2_generate_nonce,
   wv2_generate_derived_keys,
   wv2_generate_hmac_signature,
   wv2_load_keys,
   wv2_refresh_keys,
   wv2_select_key,
   wv2_inject_key,
   wv2_rewrap_device_RSA_key,
   wv2_load_device_RSA_key,
   wv2_generate_RSA_signature,
   wv2_derived_sessionkeys,
   wv2_process_video_frame,
   wv2_decrypt_ctr,
   wv2_generic_encrypt,
   wv2_generic_decrypt,
   wv2_generic_sign,
   wv2_generic_verify_signature,
   wv2_close_session,
   wv2_dbg_get_keys,
   wv2_dbg_get_title_key,
   wv9_dbg_pavp_lite_key,
   wv2_end = wv9_dbg_pavp_lite_key,

   /*wv9 apis*/
   wv9_begin = 0x000C0905,
   wv9_load_keys = wv9_begin,
   wv9_rewrap_device_RSA_key = 0x000C0909,
   wv9_load_device_RSA_key,
   wv9_generate_RSA_signature,
   wv9_update_usage_table = 0x000C0914,
   wv9_deactivate_usage_entry,
   wv9_report_usage,
   wv9_delete_usage_entry,
   wv9_delete_usage_table,
   wv9_load_usage_table,
   wv9_wrapped_title_keys,
   wv9_end = wv9_wrapped_title_keys,
} wv2_heci_command_id;

//Error Codes
typedef enum {
   MDRM_SUCCESS = 0,
   MDRM_SUCCESS_PRIV_DATA = 0x80000000,
   MDRM_FAIL_INVALID_PARAMS = 0x000F0001,
   MDRM_FAIL_INVALID_INPUT_HEADER,
   MDRM_FAIL_NO_SESSION_AVAILABLE,
   MDRM_FAIL_INVALID_SESSION,
   MDRM_FAIL_GENERATE_RANDOM_NUMBER_FAILURE,
   MDRM_FAIL_INVALID_KEY_LENGTH,
   MDRM_FAIL_INVALID_MSG_LENGTH,
   MDRM_FAIL_INVALID_SESSION_KEY,
   MDRM_FAIL_INTERNAL_ERROR,
   MDRM_FAIL_UNKNOWN_ERROR,
   MDRM_FAIL_AES_CMAC_FAILURE,
   MDRM_INVALID_NUM_KEYS,
   MDRM_HASH_MISMATCH,
   MDRM_INVALID_LEN_IN_KEY_CONTROL_BLOCK,
   MDRM_ERROR_CONTROL_INVALID,
   MDRM_ERROR_KEY_ID_INVALID,
   MDRM_FAIL_STATUS_CHAIN_NOT_INITIALIZED,
   MDRM_FAIL_NOT_EXPECTED,
   MDRM_FAIL_HDCP_OFF,
   MDRM_FAIL_INVALID_PAVP_MEMORY_MODE,
   MDRM_FAIL_INVALID_NONCE,
   MDRM_FAIL_INVALID_RSA_KEY,
   MDRM_FAIL_INVALID_TITLE_KEY,
   MDRM_FAIL_CLEAR_PATH_NOT_ALLOWED,
   MDRM_FAIL_OUT_OF_MEMORY,
   MDRM_FAIL_ERROR_DECRYPT_SESSION_KEY_FAILED,
   MDRM_FAIL_RSA_PARSER_ERROR,
   MDRM_FAIL_AES_MISMATCH,
   MDRM_FAIL_SSA_FW_MISMATCH,
   MDRM_FAIL_SSA_SW_MISMATCH,
   MDRM_FAIL_PRTC_INTERNAL_ERROR,
   MDRM_FAIL_INVALID_ENCRYPT_ALGO,
   MDRM_FAIL_OPERATION_NOT_PERMITTED,
   MDRM_FAIL_INVALID_TRANS_CONF,
   MDRM_FAIL_NO_XCRIPT_KEY,
   MDRM_FAIL_NO_KEY_BOX,
   MDRM_FAIL_INVALID_KEY_LEN,
   MDRM_FAIL_NO_AES_CBC_KEY,
   MDRM_FAIL_AES_ECB_FAILURE,
   MDRM_FAIL_INVALID_OFFSET,
   MDRM_FAIL_INVALID_KEY_ID_LENGTH,
   WVv9_FAIL_INVALID_PST_LENGTH,
   WVv9_FAIL_INVALID_REPLAY_CONTROL_VALUE,
   WVv9_FAIL_USAGE_TABLE_NOT_LOADED,
   WVv9_FAIL_INVALID_NONCE_PST,
   WVv9_FAIL_INVALID_PADDING_SCHEME,
   WVv9_FAIL_RSA_SIGN_GENERATION_PKCS,
   WVv9_FAIL_VERIFY_RSA_SIGN_PKCS,
   WVv9_USAGE_TABLE_NOT_MODIFIED,
   WVv9_FAIL_MISMATCH_IN_PST,
   WVv9_FAIL_INVALID_PST_INDEX,
   WVv9_FAIL_PST_INACTIVE,
   WVv9_FAIL_INVALID_USAGE_TABLE,
   WVv9_FAIL_INVALID_KEYS,
   Wvv9_FAIL_INVALID_NONCE_INDEX,
   WVv9_FAIL_INVALID_CONTEXT,
   WVv9_FAIL_PRTC_CLEARED,
   WVv9_FAIL_EXCEEDED_MAX_NONCE_CALL,
   WVv9_FAIL_HDCP_VERSION_NOT_SUPPORTED,
   WVv9_FAIL_KEY_LEN_NOT_SUPPORTED,
   WVv9_FAIL_TITLE_KEY_EXPIRED,

   //TMP
   MDRM_FAIL_DATA_MISMATCH1,
   MDRM_FAIL_DATA_MISMATCH2,
   MDRM_FAIL_DATA_MISMATCH3,
   MDRM_FAIL_DATA_MISMATCH4,
} mdrm_heci_status;

#endif
