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

if(${AVC_Decode_Supported} STREQUAL "yes")
set(SOFTLET_DECODE_AVC_SOURCES_
    ${SOFTLET_DECODE_AVC_SOURCES_}
    ${CMAKE_CURRENT_LIST_DIR}/decode_avc_basic_feature.cpp
    ${CMAKE_CURRENT_LIST_DIR}/decode_avc_feature_manager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/decode_avc_mv_buffers.cpp
    ${CMAKE_CURRENT_LIST_DIR}/decode_avc_reference_frames.cpp
    ${CMAKE_CURRENT_LIST_DIR}/decode_avc_downsampling_feature.cpp
)

set(SOFTLET_DECODE_AVC_HEADERS_
    ${SOFTLET_DECODE_AVC_HEADERS_}
    ${CMAKE_CURRENT_LIST_DIR}/decode_avc_basic_feature.h
    ${CMAKE_CURRENT_LIST_DIR}/decode_avc_feature_manager.h
    ${CMAKE_CURRENT_LIST_DIR}/decode_avc_mv_buffers.h
    ${CMAKE_CURRENT_LIST_DIR}/decode_avc_reference_frames.h
    ${CMAKE_CURRENT_LIST_DIR}/decode_avc_downsampling_feature.h
)

source_group( CodecHalNext\\Shared\\Decode FILES ${SOFTLET_DECODE_AVC_SOURCES_} ${SOFTLET_DECODE_AVC_HEADERS_} )

endif()

set(SOFTLET_DECODE_AVC_PRIVATE_INCLUDE_DIRS_
    ${SOFTLET_DECODE_AVC_PRIVATE_INCLUDE_DIRS_}
    ${CMAKE_CURRENT_LIST_DIR}
)
media_add_curr_to_include_path()