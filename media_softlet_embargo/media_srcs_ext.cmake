# INTEL CONFIDENTIAL
# Copyright 2021-2024 Intel Corporation.
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

media_include_subdirectory(linux)
media_include_subdirectory(proprietary)
media_include_subdirectory(media_interface)

# add embargo resources here for softlet binary reuse
set(SOFTLET_CODEC_EXT_HEADERS_
    ${SOFTLET_CODEC_EXT_HEADERS_}
    ${SOFTLET_DECODE_VVC_HEADERS_}
)

set(SOFTLET_CODEC_EXT_SOURCES_
    ${SOFTLET_CODEC_EXT_SOURCES_}
    ${SOFTLET_DECODE_VVC_SOURCES_}
)

set(SOFTLET_CODEC_EXT_PRIVATE_INCLUDE_DIRS_
    ${SOFTLET_CODEC_EXT_PRIVATE_INCLUDE_DIRS_}
    ${SOFTLET_DECODE_VVC_PRIVATE_INCLUDE_DIRS_}
)

set(SOFTLET_MHW_EXT_SOURCES_
    ${SOFTLET_MHW_EXT_SOURCES_}
    ${SOFTLET_MHW_VDBOX_VVCP_SOURCES_}
    ${SOFTLET_MHW_VDBOX_JWP_SOURCES_}
)
