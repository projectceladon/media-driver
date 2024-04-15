# Copyright (c) 2020-2022, Intel Corporation
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

#exclude opensource file
list(REMOVE_ITEM CODEC_SOURCES_
    ${MEDIA_DRIVER_ROOT}/media_softlet/agnostic/gen12_base/codec/hal/dec/shared/hucitf/huc_streamout_interface_g12.cpp
)

list(REMOVE_ITEM SOURCES_
    ${MEDIA_DRIVER_ROOT}/media_softlet/agnostic/gen12_base/codec/hal/dec/shared/hucitf/huc_streamout_interface_g12.cpp
)

set(TMP_SOURCES_
    ${CMAKE_CURRENT_LIST_DIR}/huc_cp_streamout_packet_g12.cpp
    ${CMAKE_CURRENT_LIST_DIR}/huc_streamout_interface_g12.cpp
)

set(TMP_HEADERS_
    ${CMAKE_CURRENT_LIST_DIR}/huc_cp_streamout_packet_g12.h
)

if(${PLATFORM} STREQUAL "windows")
set(CODEC_SOURCES_
    ${CODEC_SOURCES_}
    ${TMP_SOURCES_}
)

set(CODEC_HEADERS_
    ${CODEC_HEADERS_}
    ${TMP_HEADERS_}
)

set(CODEC_PRIVATE_INCLUDE_DIRS_
    ${CODEC_PRIVATE_INCLUDE_DIRS_}
    ${CMAKE_CURRENT_LIST_DIR}
)
else()
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
endif()

source_group(CpHalNext FILES ${TMP_SOURCES_} ${TMP_HEADERS_})
media_add_curr_to_include_path()
