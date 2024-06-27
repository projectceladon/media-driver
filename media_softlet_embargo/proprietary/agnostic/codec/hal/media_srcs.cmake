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

set(TMP_HEADERS_
)

set(TMP_1_SOURCES_
)

set(TMP_2_HEADERS_
)

set(TMP_2_SOURCES_
)

if(${Media_Reserved} STREQUAL "yes")
    set(TMP_2_HEADERS_
        ${TMP_2_HEADERS_}
        ${CMAKE_CURRENT_LIST_DIR}/codechal_setting_ext.h
    )
endif()

if((${PLATFORM} STREQUAL "windows") OR (${CP_Included} STREQUAL "yes"))
    set(TMP_1_SOURCES_
        ${TMP_1_SOURCES_}
        ${CMAKE_CURRENT_LIST_DIR}/codechal_cenc_decode_next.cpp
    )
    set(TMP_HEADERS_
        ${TMP_HEADERS_}
        ${CMAKE_CURRENT_LIST_DIR}/codechal_cenc_decode_next.h
    )  

if(${AVC_Decode_Supported} STREQUAL "yes")
    set(TMP_1_SOURCES_
        ${TMP_1_SOURCES_}
        ${CMAKE_CURRENT_LIST_DIR}/codechal_cenc_decode_avc_next.cpp
    )
    set(TMP_HEADERS_
        ${TMP_HEADERS_}
        ${CMAKE_CURRENT_LIST_DIR}/codechal_cenc_decode_avc_next.h
    )
endif()

if(${HEVC_Decode_Supported} STREQUAL "yes")
    set(TMP_1_SOURCES_
        ${TMP_1_SOURCES_}
        ${CMAKE_CURRENT_LIST_DIR}/codechal_cenc_decode_hevc_next.cpp
    )
    set(TMP_HEADERS_
        ${TMP_HEADERS_}
        ${CMAKE_CURRENT_LIST_DIR}/codechal_cenc_decode_hevc_next.h
    )
endif()

endif()

set(CP_COMMON_NEXT_HEADERS_
    ${CP_COMMON_NEXT_HEADERS_}
    ${TMP_HEADERS_}
)

set(CP_COMMON_NEXT_SOURCES_
    ${CP_COMMON_NEXT_SOURCES_}
    ${TMP_1_SOURCES_}
)

set(SOFTLET_CODEC_COMMON_SOURCES_
    ${SOFTLET_CODEC_COMMON_SOURCES_}
    ${TMP_2_SOURCES_}
)

set(SOFTLET_CODEC_COMMON_HEADERS_
    ${SOFTLET_CODEC_COMMON_HEADERS_}
    ${TMP_2_HEADERS_}
)

source_group( "Cenc" FILES ${TMP_1_SOURCES_} ${TMP_HEADERS_} )
set(TMP_1_SOURCES_ "")
set(TMP_HEADERS_ "")
set(TMP_2_SOURCES_ "")
set(TMP_2_HEADERS_ "")

set(SOFTLET_CODEC_COMMON_PRIVATE_INCLUDE_DIRS_
    ${SOFTLET_CODEC_COMMON_PRIVATE_INCLUDE_DIRS_}
    ${CMAKE_CURRENT_LIST_DIR}
)
set(COMMON_CP_DIRECTORIES_
    ${COMMON_CP_DIRECTORIES_}
    ${CMAKE_CURRENT_LIST_DIR}
)

media_add_curr_to_include_path()
