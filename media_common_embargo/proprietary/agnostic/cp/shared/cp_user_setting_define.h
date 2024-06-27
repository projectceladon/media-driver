/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2022
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

File Name: cp_user_setting_define.h

Abstract:

Environment:
    OS agnostic

Notes:
    None

======================= end_copyright_notice ==================================*/
#ifndef __CP_USER_SETTING_DEFINE_H__
#define __CP_USER_SETTING_DEFINE_H__

#define __CP_USER_FEATURE_KEY_START_ID 6144

//!
//! \brief Keys for Content Protection
//!

//!
//! \brief Functional Keys
//!
#define __MEDIA_USER_FEATURE_VALUE_FIRST_KEY_EXCHANGE_IS_RSA         "First Key Exchange is RSA"
#define __MEDIA_USER_FEATURE_VALUE_CP_REPORTS_ENABLED                "CP Reports Enabled"
#define __MEDIA_USER_FEATURE_VALUE_HAS_PAVP_CTRL                     "HASPavpTestingSupportCtrl"
#define __MEDIA_USER_FEATURE_VALUE_ARBITRATOR_SESSION_ENABLED        "Arbitrator Session Enable"
#define __MEDIA_USER_FEATURE_VALUE_PAVP_PCH_HAL_UNIFIED_ENABLED      "PAVP Hal Unified Enable"
#define __MEDIA_USER_FEATURE_VALUE_PAVP_WDDM2_KEY_UPDATE_ENABLED     "Wddm2 Key Update Enable"
#define __MEDIA_USER_FEATURE_VALUE_PAVP_WDDM2_FEATURE_ENABLED        "Wddm2 Features Enable"

//!
//! \brief Report Keys
//!
#define __MEDIA_USER_FEATURE_VALUE_ENCRYPTION_MODE                   "Encryption Mode"
#define __MEDIA_USER_FEATURE_VALUE_ENCRYPTION_TYPE                   "Encryption Type"
#define __MEDIA_USER_FEATURE_VALUE_PAVP_KEY_EXCHANGE                 "PAVP Key Exchange"
#define __MEDIA_USER_FEATURE_VALUE_DYNAMIC_MEMORY_PROTECTION         "Dynamic Memory Protection"
#define __MEDIA_USER_FEATURE_VALUE_REPORT_FRAMES_ENCRYPTED           "Report Encrypted Frame Count"
#define __MEDIA_USER_FEATURE_VALUE_FRAMES_ENCRYPTED                  "Encrypted Frame Count"
#define __MEDIA_USER_FEATURE_VALUE_PROTECTION_TRIGGERED              "GPUCP Protection Triggered"
#define __MEDIA_USER_FEATURE_VALUE_AUTO_TEAR_DOWN_COUNTER            "Auto Tear-Down Counter"
#define __MEDIA_USER_FEATURE_VALUE_LITE_SESSIONS                     "LiteModeSessionsInPlay"
#define __MEDIA_USER_FEATURE_VALUE_HEAVY_SESSIONS                    "HeavyModeSessionsInPlay"
#define __MEDIA_USER_FEATURE_VALUE_RSA_COUNT                         "RSAsessionCount"
#define __MEDIA_USER_FEATURE_VALUE_COUNTER_TYPE                      "Counter Type"

//!
//! \brief Specify Key Exchange Mode for PAVP Session Establishment
//!
#define __MEDIA_USER_FEATURE_VALUE_PAVP_KEY_REFRESH_MODE                    "PAVP Key Refresh Mode"          //!< 0: Daisy Chain, others: Kb based.

//!
//! \brief Specify zero or non-zero cek: For testing pupose only
//!
#define __MEDIA_USER_FEATURE_VALUE_PAVP_NON_ZERO_CEK                        "PAVP Use NonZero CEK"           //!< 0: zero cek, others: Nonzero cek.

//!
//! \brief Specify zero or non-zero Widi Session key: For testing pupose only
//!
#define __MEDIA_USER_FEATURE_VALUE_PAVP_NON_ZERO_WIDI_KS                     "PAVP Use NonZero Widi KS"      //!< 0: zero Ks, others: Nonzero Ks.

//!
//! \brief Enable HuC access to the encrypted bitstream
//!
#define __MEDIA_USER_FEATURE_VALUE_PAVP_HUC_ACCESS_TO_ENCR_BITSTREAM        "PAVP HuC Access Ctrl"          //!< 0: Enable, others: disabled.

//!
//! \brief Enable HuC clear writes to Region 15
//!
#define __MEDIA_USER_FEATURE_VALUE_PAVP_ENCRYPT_HUC_WRITE_TO_R15            "PAVP HuC R15 Ctrl"             //!< 0: Enable clear writes, others: Force Encrypt.

//!
//! \brief Enable Crypto Copy Encrypt modes 2, 4, 7
//!
#define __MEDIA_USER_FEATURE_VALUE_PAVP_ENCRYPT_CC_CTRL                     "PAVP Encrypt CC Ctrl"          //!< 0: Enable CC modes 2,4,7, others: CC modes 2,4,7 disabled

//!
//! \brief Enable Crypto Copy Decrypt modes 2,3,d
//!
#define __MEDIA_USER_FEATURE_VALUE_PAVP_DECRYPT_CC_CTRL                     "PAVP Decrypt CC Ctrl"          //!< 0: Enable CC modes 2,3,d, others: CC modes 2,3,d disabled

//!
//! \brief Controls whether HW will use AES counter programmed by driver for crypto copy
//!
#define __MEDIA_USER_FEATURE_VALUE_PAVP_ALLOW_PROGRAMMABLE_AES_COUNTER      "PAVP Allow Programmable AES Counter"  //!< 0: Do not allow, others: allow

#if (_DEBUG || _RELEASE_INTERNAL)
//!
//!
//! \brief Fore lite mode for debug
#define __MEDIA_USER_FEATURE_VALUE_PAVP_FORCE_LITE_MODE_FOR_DEBUG           "PAVP Force Lite Mode For Debug"    //!< 0: Disabled, others Enabled
#define __MEDIA_USER_FEATURE_VALUE_PAVP_GSC_TUNNELING_SUPPORTED             "PAVP GSC Tunneling On ICL Enabled" //!< 0: Default rule, 1: Force ICL use GSCHAL, 2 Force use PCHHAL
#define __MEDIA_USER_FEATURE_VALUE_PAVP_GSC_EMULATION                       "PAVP GSC Emulation"                //!< 0: Disabled, others Enabled
#define __MEDIA_USER_FEATURE_VALUE_PAVP_SKIP_CHECK_PCH_PRODUCT_FAMILIY      "PAVP SKIP Check PCH Product Family"    //!< 0: Disabled, others Enabled

//!
//!
//! \brief HuC stream decryption for decode
#define __MEDIA_USER_FEATURE_VALUE_PAVP_DECODE_USE_HUC_DECRYPT              "PAVP Decode HuC Stream Decryption" //!< 0: Disabled, others Enabled


#define __MEDIA_USER_FEATURE_VALUE_PAVP_LITE_MODE_ENABLE_HUC_STREAMOUT_FOR_DEBUG           "Id of PAVP Lite Mode Enable Huc StreamOut For Debug"    //!< 0: Disabled, others Enabled
#endif

//!
//! \brief User Feature Value IDs
//!
typedef enum _CP_USER_FEATURE_VALUE_ID
{
    __CP_USER_FEATURE_KEY_INVALID_ID = __CP_USER_FEATURE_KEY_START_ID,
    __MEDIA_USER_FEATURE_VALUE_FIRST_KEY_EXCHANGE_IS_RSA_ID,
    __MEDIA_USER_FEATURE_VALUE_CP_REPORTS_ENABLED_ID,
    __MEDIA_USER_FEATURE_VALUE_HAS_PAVP_CTRL_ID,
    __MEDIA_USER_FEATURE_VALUE_ARBITRATOR_SESSION_ENABLED_ID,
    __MEDIA_USER_FEATURE_VALUE_PAVP_PCH_HAL_UNIFIED_ENABLED_ID,
    __MEDIA_USER_FEATURE_VALUE_ENCRYPTION_MODE_ID,
    __MEDIA_USER_FEATURE_VALUE_ENCRYPTION_TYPE_ID,
    __MEDIA_USER_FEATURE_VALUE_PAVP_KEY_EXCHANGE_ID,
    __MEDIA_USER_FEATURE_VALUE_DYNAMIC_MEMORY_PROTECTION_ID,
    __MEDIA_USER_FEATURE_VALUE_REPORT_FRAMES_ENCRYPTED_ID,
    __MEDIA_USER_FEATURE_VALUE_FRAMES_ENCRYPTED_ID,
    __MEDIA_USER_FEATURE_VALUE_PROTECTION_TRIGGERED_ID,
    __MEDIA_USER_FEATURE_VALUE_AUTO_TEAR_DOWN_COUNTER_ID,
    __MEDIA_USER_FEATURE_VALUE_LITE_SESSIONS_ID,
    __MEDIA_USER_FEATURE_VALUE_HEAVY_SESSIONS_ID,
    __MEDIA_USER_FEATURE_VALUE_RSA_COUNT_ID,
    __MEDIA_USER_FEATURE_VALUE_COUNTER_TYPE_ID,
    __MEDIA_USER_FEATURE_VALUE_EVENT_PROVIDER_REFCOUNT_ID,
    __MEDIA_USER_FEATURE_VALUE_PAVP_KEY_REFRESH_MODE_ID,
    __MEDIA_USER_FEATURE_VALUE_PAVP_HUC_ACCESS_TO_ENCR_BITSTREAM_ID,
    __MEDIA_USER_FEATURE_VALUE_PAVP_ENCRYPT_HUC_WRITE_TO_R15_ID,
    __MEDIA_USER_FEATURE_VALUE_PAVP_ENCRYPT_CC_CTRL_ID,
    __MEDIA_USER_FEATURE_VALUE_PAVP_DECRYPT_CC_CTRL_ID,
    __MEDIA_USER_FEATURE_VALUE_PAVP_NON_ZERO_CEK_ID,
    __MEDIA_USER_FEATURE_VALUE_PAVP_NON_ZERO_WIDI_KS_ID,
#if (_DEBUG || _RELEASE_INTERNAL)
    __MEDIA_USER_FEATURE_VALUE_PAVP_FORCE_LITE_MODE_FOR_DEBUG_ID,
    __MEDIA_USER_FEATURE_VALUE_PAVP_LITE_MODE_ENABLE_HUC_STREAMOUT_FOR_DEBUG_ID,
    __MEDIA_USER_FEATURE_VALUE_PAVP_GSC_TUNNELING_SUPPORTED_ID,
    __MEDIA_USER_FEATURE_VALUE_PAVP_GSC_EMULATION_ID,
    __MEDIA_USER_FEATURE_VALUE_PAVP_SKIP_CHECK_PCH_PRODUCT_FAMILIY_ID,
    __MEDIA_USER_FEATURE_VALUE_PAVP_DECODE_USE_HUC_DECRYPT_ID,
#endif
    __MEDIA_USER_FEATURE_VALUE_PAVP_ALLOW_PROGRAMMABLE_AES_COUNTER_ID,
    __CP_USER_FEATURE_KEY_MAX_ID,
} CP_USER_FEATURE_VALUE_ID;

#define CP_NUM_USERFEATURE_KEY_VALUES (__CP_USER_FEATURE_KEY_MAX_ID - __CP_USER_FEATURE_KEY_INVALID_ID - 1)

#endif //__CP_USER_SETTING_DEFINE_H__
