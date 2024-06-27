# INTEL CONFIDENTIAL
# Copyright 2018-2024 Intel Corporation.
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

if(GEN8)
    media_include_subdirectory(gen8)
endif()

if(GEN9)
    media_include_subdirectory(gen9)
endif()

if(GEN10)
    media_include_subdirectory(gen10)
endif()

if(GEN11)
    media_include_subdirectory(gen11)
endif()

if(GEN12)
    media_include_subdirectory(gen12)
endif()







set(TMP_SOURCES_
    ${CMAKE_CURRENT_LIST_DIR}/mhw_cp.cpp
    ${CMAKE_CURRENT_LIST_DIR}/mhw_cp_hwcmd_common.c
    ${CMAKE_CURRENT_LIST_DIR}/mhw_cp_unsupported.cpp
    ${CMAKE_CURRENT_LIST_DIR}/mhw_cp_next.cpp
    ${CMAKE_CURRENT_LIST_DIR}/mhw_cp_hwcmd_next.cpp
)

set(TMP_HEADERS_
    ${CMAKE_CURRENT_LIST_DIR}/mhw_cp_unsupported.h
    ${CMAKE_CURRENT_LIST_DIR}/mhw_cp_next.h
    ${CMAKE_CURRENT_LIST_DIR}/mhw_cp_hwcmd_next.h
)

set(CP_COMMON_SHARED_SOURCES_
    ${CP_COMMON_SHARED_SOURCES_}
    ${TMP_SOURCES_}
)

set(CP_COMMON_SHARED_HEADERS_
    ${CP_COMMON_SHARED_HEADERS_}
    ${TMP_HEADERS_}
)

set(COMMON_CP_DIRECTORIES_
    ${COMMON_CP_DIRECTORIES_}
    ${CMAKE_CURRENT_LIST_DIR}
)

source_group( "MHW\\Content Protection" FILES ${TMP_SOURCES_} ${TMP_HEADERS_} )
media_include_subdirectory(xe_lpm_plus)

media_add_curr_to_include_path()
