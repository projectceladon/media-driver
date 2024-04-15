# INTEL CONFIDENTIAL
# Copyright 2021 Intel Corporation.
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

# TODO to seek clean

#include_directories(${BS_DIR_MEDIA}/media_embargo/windows/common/ddi/d3d9)

# prioritize cp directory. Need change once the directory changes, otherwise cp opensourced stub file will be compiled and impact functionality.
include_directories(BEFORE ${BS_DIR_MEDIA}/media_softlet_embargo/agnostic/common/cp)

# for mhw_mmio_ext.h
include_directories(BEFORE ${BS_DIR_MEDIA}/media_common_embargo/proprietary/agnostic/cp/hal/hw)

# media_driver/proprietary/agnostic/vp/hal/vphal_render_3P.c:43
#     #include "ecdsa.h"
# media_driver/proprietary/agnostic/vp/hal/ should be ignored by linux build actually,
# be this depends on VP team to clean. Below from media/build/windows/v2c/file_filters.txt
# bottom part -
#    # temp WA, compile close vebox for linux pre-si
#    # TODO : remove this WA after adding pre-si open vebox
#    #.*/media_driver/proprietary/agnostic/vp/hal/.*
# so the folder still need compile for now, then we need a path for ecdsa
#include_directories(${BS_DIR_MEDIA}/media_embargo/proprietary/windows/common/ecdsa)

if(${Media_Solo_Supported} STREQUAL "yes")
# for mhw_cp_hwcmd_common.h
#include_directories(${BS_DIR_MEDIA}/media_embargo/proprietary/agnostic/cp/hal/hw)
# for intel_pavp_types.h
#include_directories(${BS_DIR_MEDIA}/media_embargo/proprietary/agnostic/cp/api)
endif()
