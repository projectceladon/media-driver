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

bs_set_if_undefined(Media_Solo_Supported "no")
bs_set_if_undefined(AV1_SecureDecode_Supported "no")
bs_set_if_undefined(Hybrid_VP9_Decode_Supported "no")
bs_set_if_undefined(Hybrid_HEVC_Decode_Supported "no")
bs_set_if_undefined(Encode_VDEnc_Reserved "no")
bs_set_if_undefined(Media_Reserved "yes")
bs_set_if_undefined(CP_Included "yes")
bs_set_if_undefined(MHW_HWCMD_Parser_Supported "no")
bs_set_if_undefined(MEDIA_EXT_CMAKE  "${BS_DIR_MEDIA}/media_embargo/cmake")
bs_set_if_undefined(MEDIA_COMMON_EXT  "${BS_DIR_MEDIA}/media_common_embargo")
bs_set_if_undefined(MEDIA_COMMON_EXT_CMAKE  "${BS_DIR_MEDIA}/media_common_embargo/cmake")
bs_set_if_undefined(MEDIA_SOFTLET_EXT        "${BS_DIR_MEDIA}/media_softlet_embargo")
bs_set_if_undefined(MEDIA_SOFTLET_EXT_CMAKE  "${BS_DIR_MEDIA}/media_softlet_embargo/cmake")
