# Copyright (c) 2017, Intel Corporation
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

if(WIN32)
    set(PLATFORM "windows")
endif()

SET (CMAKE_SHARED_LINKER_FLAGS_RELEASEINTERNAL "") 
SET (CMAKE_SHARED_LINKER_FLAGS_RELEASE "") 
SET (CMAKE_SHARED_LINKER_FLAGS_DEBUG "") 
SET (CMAKE_SHARED_LINKER_FLAGS "") 

SET (CMAKE_EXE_LINKER_FLAGS "")
SET (CMAKE_EXE_LINKER_FLAGS_DEBUG "")
SET (CMAKE_EXE_LINKER_FLAGS_RELEASEINTERNAL "")
SET (CMAKE_EXE_LINKER_FLAGS_RELEASE "")

SET (CMAKE_STATIC_LINKER_FLAGS "")
SET (CMAKE_LOCAL_LINKER_FLAGS "")

SET (CMAKE_CXX_STANDARD_LIBRARIES "")
SET (CMAKE_C_STANDARD_LIBRARIES "")

SET (CMAKE_CXX_FLAGS_RELEASEINTERNAL "")
SET (CMAKE_CXX_FLAGS_DEBUG  "")
SET (CMAKE_CXX_FLAGS_RELEASE "")

bs_set_if_undefined(MEDIA_SOFTLET_EXT        "${BS_DIR_MEDIA}/media_softlet_embargo")
bs_set_if_undefined(MEDIA_SOFTLET_EXT_CMAKE  "${BS_DIR_MEDIA}/media_softlet_embargo/cmake")

bs_set_if_undefined(MEDIA_COMMON_EXT        "${BS_DIR_MEDIA}/media_common_embargo")
bs_set_if_undefined(MEDIA_COMMON_EXT_CMAKE  "${BS_DIR_MEDIA}/media_common_embargo/cmake")

#need to clean up when ddi is ready
bs_set_if_undefined(MEDIA_EXT                "${BS_DIR_MEDIA}/media_embargo")