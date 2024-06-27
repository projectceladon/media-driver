# INTEL CONFIDENTIAL
# Copyright 2020 Intel Corporation.
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
set(TMP_SOURCES_
    ${CMAKE_CURRENT_LIST_DIR}/codechal_cenc_decode.cpp
    ${CMAKE_CURRENT_LIST_DIR}/codechal_cenc_decode_avc.cpp
    ${CMAKE_CURRENT_LIST_DIR}/codechal_cenc_decode_hevc.cpp
    ${CMAKE_CURRENT_LIST_DIR}/codechal_secure_decode.cpp
    ${CMAKE_CURRENT_LIST_DIR}/codechal_secure_decode_avc.cpp
    ${CMAKE_CURRENT_LIST_DIR}/codechal_secure_decode_vp9.cpp
    ${CMAKE_CURRENT_LIST_DIR}/codechal_secure_decode_hevc.cpp
)

if(${AV1_Decode_Supported} STREQUAL "yes")
set(TMP_SOURCES_
    ${TMP_SOURCES_}
    ${CMAKE_CURRENT_LIST_DIR}/codechal_secure_decode_av1.cpp
)
endif()

set(TMP_HEADERS_
)

set(CP_COMMON_SHARED_SOURCES_
    ${CP_COMMON_SHARED_SOURCES_}
    ${TMP_SOURCES_}
)

set(CP_COMMON_SHARED_HEADERS_
    ${CP_COMMON_SHARED_HEADERS_}
    ${TMP_HEADERS_}
)

source_group( "CodecHal\\Decode\\secure_decode" FILES ${TMP_SOURCES_} ${TMP_HEADERS_} )


media_add_curr_to_include_path()
