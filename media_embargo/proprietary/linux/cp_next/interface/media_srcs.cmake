# INTEL CONFIDENTIAL
# Copyright 2021-2022 Intel Corporation.
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

media_include_subdirectory(public_platform)

if(GEN12_RYF)
    media_include_subdirectory(gen12_ryf)
endif()
if(GEN12_RKL)
    media_include_subdirectory(gen12_rkl)
endif()
if(GEN12_ADLS)
    media_include_subdirectory(gen12_adls)
endif()
if(GEN12_ADLP)
    media_include_subdirectory(gen12_adlp)
endif()
if(GEN12_ADLN)
    media_include_subdirectory(gen12_adln)
endif()



media_include_subdirectory(cp_interface_mtl)

