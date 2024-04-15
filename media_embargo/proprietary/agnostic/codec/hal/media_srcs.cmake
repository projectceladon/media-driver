# INTEL CONFIDENTIAL
# Copyright 2017-2023 Intel Corporation.
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

if(${Hybrid_HEVC_Decode_Supported} STREQUAL "yes")
    media_include_subdirectory(hybrid_hevc)
endif()

if(${Hybrid_VP9_Decode_Supported} STREQUAL "yes")
    media_include_subdirectory(hybrid_vp9)
endif()

set(TMP_1_SOURCES_ "")
set(TMP_1_HEADERS_ "")
set(TMP_2_SOURCES_ "")
set(TMP_2_HEADERS_ "")
set(TMP_CP_SOURCES_ "")
set(TMP_CP_HEADERS_ "")

#decode

if((${PLATFORM} STREQUAL "windows") OR (${CP_Included} STREQUAL "yes"))
set(TMP_CP_SOURCES_
    ${TMP_CP_SOURCES_}
    ${CMAKE_CURRENT_LIST_DIR}/secure_decode/codechal_secure_decode.cpp
    ${CMAKE_CURRENT_LIST_DIR}/secure_decode/codechal_cenc_decode.cpp
)
set(TMP_CP_HEADERS_
    ${TMP_CP_HEADERS_}
    ${CMAKE_CURRENT_LIST_DIR}/codechal_secure_decode.h
    ${CMAKE_CURRENT_LIST_DIR}/codechal_cenc_decode.h
)

if(${AVC_Decode_Supported} STREQUAL "yes")
    set(TMP_CP_SOURCES_
        ${TMP_CP_SOURCES_}
        ${CMAKE_CURRENT_LIST_DIR}/secure_decode/codechal_secure_decode_avc.cpp
        ${CMAKE_CURRENT_LIST_DIR}/secure_decode/codechal_cenc_decode_avc.cpp
    )
    set(TMP_CP_HEADERS_
        ${TMP_CP_HEADERS_}
        ${CMAKE_CURRENT_LIST_DIR}/codechal_secure_decode_avc.h
        ${CMAKE_CURRENT_LIST_DIR}/codechal_cenc_decode_avc.h
    )
endif()

if(${HEVC_Decode_Supported} STREQUAL "yes")
    set(TMP_CP_SOURCES_
        ${TMP_CP_SOURCES_}
        ${CMAKE_CURRENT_LIST_DIR}/secure_decode/codechal_secure_decode_hevc.cpp
        ${CMAKE_CURRENT_LIST_DIR}/secure_decode/codechal_cenc_decode_hevc.cpp
    )
    set(TMP_CP_HEADERS_
        ${TMP_CP_HEADERS_}
        ${CMAKE_CURRENT_LIST_DIR}/codechal_secure_decode_hevc.h
        ${CMAKE_CURRENT_LIST_DIR}/codechal_cenc_decode_hevc.h
    )
endif()

if(${VP9_Decode_Supported} STREQUAL "yes")
    set(TMP_CP_SOURCES_
        ${TMP_CP_SOURCES_}
        ${CMAKE_CURRENT_LIST_DIR}/secure_decode/codechal_secure_decode_vp9.cpp
    )
    set(TMP_CP_HEADERS_
        ${TMP_CP_HEADERS_}
        ${CMAKE_CURRENT_LIST_DIR}/codechal_secure_decode_vp9.h
    )

endif()

if(${AV1_SecureDecode_Supported} STREQUAL "yes")
    set(TMP_CP_SOURCES_
        ${TMP_CP_SOURCES_}
        ${CMAKE_CURRENT_LIST_DIR}/secure_decode/codechal_secure_decode_av1.cpp
    )
    set(TMP_CP_HEADERS_
        ${TMP_CP_HEADERS_}
        ${CMAKE_CURRENT_LIST_DIR}/codechal_secure_decode_av1.h
    )
endif()
endif()


set(SOURCES_
    ${SOURCES_}
    ${TMP_1_SOURCES_}
    ${TMP_2_SOURCES_}
)

set(HEADERS_
    ${HEADERS_}
    ${TMP_1_HEADERS_}
    ${TMP_2_HEADERS_}
)

set(CODEC_SOURCES_
    ${CODEC_SOURCES_}
    ${TMP_1_SOURCES_}
    ${TMP_2_SOURCES_}
)

set(CODEC_HEADERS_
    ${CODEC_HEADERS_}
    ${TMP_1_HEADERS_}
    ${TMP_2_HEADERS_}
)

set(CP_COMMON_SHARED_HEADERS_
    ${CP_COMMON_SHARED_HEADERS_}
    ${TMP_CP_HEADERS_}
)

set(CP_COMMON_SHARED_SOURCES_
    ${CP_COMMON_SHARED_SOURCES_}
    ${TMP_CP_SOURCES_}
)

set(D3D9_HEADERS_
    ${D3D9_HEADERS_}
    ${TMP_CP_SOURCES_}
)

set(D3D9_SOURCES_
    ${D3D9_SOURCES_}
    ${TMP_CP_SOURCES_}
)

source_group( CodecHal\\Decode FILES ${TMP_1_SOURCES_} ${TMP_1_HEADERS_} )
source_group( CodecHal\\Encode FILES ${TMP_2_SOURCES_} ${TMP_2_HEADERS_} )
source_group( CodecHal\\SecureDecode FILES ${TMP_CP_SOURCES_} ${TMP_CP_HEADERS_} )
set(TMP_1_SOURCES_ "")
set(TMP_1_HEADERS_ "")
set(TMP_2_SOURCES_ "")
set(TMP_2_HEADERS_ "")
set(TMP_CP_SOURCES_ "")
set(TMP_CP_HEADERS_ "")

media_add_curr_to_include_path()
