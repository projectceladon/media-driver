# Copyright (c) 2017-2022, Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.

media_include_subdirectory(vdbox)


set(TMP_1_SOURCES_
    ${CMAKE_CURRENT_LIST_DIR}/mhw_sfc_g11_X.cpp
    ${CMAKE_CURRENT_LIST_DIR}/mhw_sfc_hwcmd_g11_X.cpp
)

set(TMP_1_HEADERS_
    ${CMAKE_CURRENT_LIST_DIR}/mhw_sfc_g11_X.h
    ${CMAKE_CURRENT_LIST_DIR}/mhw_sfc_hwcmd_g11_X.h
)

set(TMP_2_SOURCES_
    ${CMAKE_CURRENT_LIST_DIR}/mhw_vebox_g11_X.cpp
    ${CMAKE_CURRENT_LIST_DIR}/mhw_vebox_hwcmd_g11_X.cpp
)

set(TMP_2_HEADERS_
    ${CMAKE_CURRENT_LIST_DIR}/mhw_vebox_g11_X.h
    ${CMAKE_CURRENT_LIST_DIR}/mhw_vebox_hwcmd_g11_X.h
)

source_group("MHW\\SFC" FILES ${TMP_1_SOURCES_} ${TMP_1_HEADERS_})
source_group("MHW\\VEBOX" FILES ${TMP_2_SOURCES_} ${TMP_2_HEADERS_})


set(TMP_3_SOURCES_
    ${CMAKE_CURRENT_LIST_DIR}/mhw_mi_g11_X.cpp
    ${CMAKE_CURRENT_LIST_DIR}/mhw_mi_hwcmd_g11_X.cpp
)

set(TMP_3_HEADERS_
    ${CMAKE_CURRENT_LIST_DIR}/mhw_mi_g11_X.h
    ${CMAKE_CURRENT_LIST_DIR}/mhw_mi_hwcmd_g11_X.h
    ${CMAKE_CURRENT_LIST_DIR}/mhw_mmio_g11.h
)

set(TMP_4_SOURCES_
    ${CMAKE_CURRENT_LIST_DIR}/mhw_render_g11_X.cpp
    ${CMAKE_CURRENT_LIST_DIR}/mhw_render_hwcmd_g11_X.cpp
)

set(TMP_4_HEADERS_
    ${CMAKE_CURRENT_LIST_DIR}/mhw_render_g11_X.h
    ${CMAKE_CURRENT_LIST_DIR}/mhw_render_hwcmd_g11_X.h
)

set(TMP_5_SOURCES_
    ${CMAKE_CURRENT_LIST_DIR}/mhw_state_heap_g11.c
    ${CMAKE_CURRENT_LIST_DIR}/mhw_state_heap_hwcmd_g11_X.cpp
)

set(TMP_5_HEADERS_
    ${CMAKE_CURRENT_LIST_DIR}/mhw_state_heap_g11.h
    ${CMAKE_CURRENT_LIST_DIR}/mhw_state_heap_hwcmd_g11_X.h
)

set(COMMON_SOURCES_
    ${COMMON_SOURCES_}
    ${TMP_1_SOURCES_}
    ${TMP_2_SOURCES_}
    ${TMP_3_SOURCES_}
    ${TMP_4_SOURCES_}
    ${TMP_5_SOURCES_}
)

set(COMMON_HEADERS_
    ${COMMON_HEADERS_}
    ${TMP_1_HEADERS_}
    ${TMP_2_HEADERS_}
    ${TMP_3_HEADERS_}
    ${TMP_4_HEADERS_}
    ${TMP_5_HEADERS_}
)

source_group("MHW\\Common MI" FILES ${TMP_3_SOURCES_} ${TMP_3_HEADERS_})
source_group("MHW\\Render Engine" FILES ${TMP_4_SOURCES_} ${TMP_4_HEADERS_})
source_group("MHW\\State Heap" FILES ${TMP_5_SOURCES_} ${TMP_5_HEADERS_})

set(COMMON_PRIVATE_INCLUDE_DIRS_
    ${COMMON_PRIVATE_INCLUDE_DIRS_}
    ${CMAKE_CURRENT_LIST_DIR}
)
media_add_curr_to_include_path()