# INTEL CONFIDENTIAL
# Copyright 2017-2020 Intel Corporation.
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

media_include_subdirectory(i915childpxp)

set(TMP_SOURCES_
    ${CMAKE_CURRENT_LIST_DIR}/cp_os_services_linux.cpp
    ${CMAKE_CURRENT_LIST_DIR}/mos_os_cp_specific.cpp
)

set(TMP_HEADERS_
    ${CMAKE_CURRENT_LIST_DIR}/cp_os_services_linux.h
    ${CMAKE_CURRENT_LIST_DIR}/mos_os_cp_specific.h
    ${CMAKE_CURRENT_LIST_DIR}/pxp_statemachine.h
)

set(SOURCES_
    ${SOURCES_}
    ${TMP_SOURCES_}
 )

set(HEADERS_
    ${HEADERS_}
    ${TMP_HEADERS_}
)

media_add_curr_to_include_path()
