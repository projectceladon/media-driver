# INTEL CONFIDENTIAL
# Copyright 2021 Intel Corporation.
#
# The source code contained or described herein and all documents related
# to the source code ("Material") are owned by Intel Corporation or its
# suppliers or licensors. Title to the Material remains with Intel Corporation
# or its suppliers and licensors. The Material contains trade secrets and
# proprietary and confidential information of Intel or its suppliers and
# licensors. The Material is protected by worldwide copyright and trade secret
# laws and treaty provisions. No part of the Material may be used, copied,
# reproduced, modified, published, uploaded, posted, transmitted, distributed,
# or disclosed in any way without Intel's prior express written permission.

# No license under any patent, copyright, trade secret or other intellectual
# property right is granted to or conferred upon you by disclosure or delivery
# of the Materials, either expressly, by implication, inducement, estoppel or
# otherwise. Any license under such intellectual property rights must be express
# and approved by Intel in writing.

bs_set_if_undefined(AV1_Decode_Supported "yes")
bs_set_if_undefined(AV1_SecureDecode_Supported "yes")
bs_set_if_undefined(AV1_Encode_VDEnc_Supported "yes")
bs_set_if_undefined(Hybrid_VP9_Decode_Supported "no")
bs_set_if_undefined(Hybrid_VP9_Encode_Supported "no")
bs_set_if_undefined(Hybrid_HEVC_Decode_Supported "no")
if(0)
    bs_set_if_undefined(VVC_Decode_Supported "yes")
    bs_set_if_undefined(VVC_SecureDecode_Supported "yes")
else()
    bs_set_if_undefined(VVC_Decode_Supported "no")
    bs_set_if_undefined(VVC_SecureDecode_Supported "no")
endif()
# TODO only can open when CMRT/FEI ready
bs_set_if_undefined(CMRT_ENC_FEI_Supported "no")

bs_set_if_undefined(Media_Solo_Supported "no")
bs_set_if_undefined(Media_Reserved "yes")
bs_set_if_undefined(Fdfb_Supported "no")
# need close the proprietary build after vp codes clean
bs_set_if_undefined(ModsVpBuildProprietaryComponents "yes")
bs_set_if_undefined(MEDIASOLO_SUPPORTED_LINUX "yes")
bs_set_if_undefined(Decode_Processing_Supported "yes")
bs_set_if_undefined(Private_MMC_Supported "yes")
bs_set_if_undefined(Encode_VDEnc_Reserved "yes")
bs_set_if_undefined(MD5_Debug_Supported "yes")
bs_set_if_undefined(GMM_PAGETABLE_MGR_Supported "yes")
bs_set_if_undefined(MOS_UTILITY_EXT_Supported "yes")
bs_set_if_undefined(APOGEIOS_Supported "yes")
bs_set_if_undefined(CP_Included "no")
bs_set_if_undefined(MHW_HWCMD_Parser_Supported "no")
bs_set_if_undefined(Huc_Less_Supported "no")

if(${AV1_Decode_Supported} STREQUAL "yes")
    add_definitions(-D_AV1_DECODE_SUPPORTED)
endif()

if(${AV1_SecureDecode_Supported} STREQUAL "yes")
    add_definitions(-D_AV1_SECURE_DECODE_SUPPORTED)
endif()

if (${AV1_Encode_VDEnc_Supported} STREQUAL "yes")
    add_definitions(-D_AV1_ENCODE_VDENC_SUPPORTED)
endif ()

if(${Hybrid_VP9_Encode_Supported} STREQUAL "yes")
    add_definitions(-D_HYBRID_VP9_ENCODE_SUPPORTED)
endif()

if(${Hybrid_VP9_Decode_Supported} STREQUAL "yes")
    add_definitions(-D_HYBRID_VP9_DECODE_SUPPORTED)
endif()

if(${Hybrid_HEVC_Decode_Supported} STREQUAL "yes")
    add_definitions(-D_HYBRID_HEVC_DECODE_SUPPORTED)
endif()

if(${VVC_Decode_Supported} STREQUAL "yes")
    add_definitions(-D_VVC_DECODE_SUPPORTED)
endif()

if(${VVC_SecureDecode_Supported} STREQUAL "yes")
    add_definitions(-D_VVC_SECURE_DECODE_SUPPORTED)
endif()

if(${CMRT_ENC_FEI_Supported} STREQUAL "yes")
    add_definitions(-DFEI_ENABLE_CMRT)
endif()

if(${Media_Solo_Supported} STREQUAL "yes")
    add_compile_definitions("$<$<CONFIG:Debug>:MOS_MEDIASOLO_SUPPORTED>")
    add_compile_definitions("$<$<CONFIG:ReleaseInternal>:MOS_MEDIASOLO_SUPPORTED>")
endif()

if(${Media_Reserved} STREQUAL "yes")
    add_definitions(-D_MEDIA_RESERVED)
endif()

if(${Fdfb_Supported} STREQUAL "yes")
    add_definitions(-D_FDFB_SUPPORTED)
endif()

if(${ModsVpBuildProprietaryComponents} STREQUAL "yes")
    add_definitions(-D_BuildProprietaryComponents)
endif()

if(${Decode_Processing_Supported} STREQUAL "yes")
    add_definitions(-D_DECODE_PROCESSING_SUPPORTED)
endif()

if(${Private_MMC_Supported} STREQUAL "yes")
    add_definitions(-D_PRIVATE_MMC_SUPPORTED)
endif()

if(${MD5_Debug_Supported} STREQUAL "yes")
    add_definitions(-D_MD5_DEBUG_SUPPORTED)
endif()

if(${MOS_UTILITY_EXT_Supported} STREQUAL "yes")
    add_definitions(-D_MOS_UTILITY_EXT)
endif()

if(${APOGEIOS_Supported} STREQUAL "yes")
    add_definitions(-D_APOGEIOS_SUPPORTED)
endif()

if (DEFINED CP_INCLUDED)
bs_set(CP_Included "yes")
endif()

if(${CP_Included} STREQUAL "yes")
    add_definitions(-D_CP_INCLUDED)
endif()


if(${Huc_Less_Supported} STREQUAL "yes")
    add_compile_definitions("$<$<CONFIG:Debug>:HUCLESS_SUPPORTED>")
    add_compile_definitions("$<$<CONFIG:ReleaseInternal>:HUCLESS_SUPPORTED>")
endif()