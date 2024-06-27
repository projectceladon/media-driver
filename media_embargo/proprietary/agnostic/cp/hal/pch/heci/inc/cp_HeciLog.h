/*++

INTEL CONFIDENTIAL
Copyright 2011-2017 Intel Corporation All Rights Reserved.

The source code contained or described herein and all documents related to the
source code ("Material") are owned by Intel Corporation or its suppliers or licensors.
Title to the Material remains with Intel Corporation or its suppliers and licensors.
The Material may contain trade secrets and proprietary and confidential information
of Intel Corporation and its suppliers and licensors, and is protected by worldwide
copyright and trade secret laws and treaty provisions. No part of the Material
may be used, copied, reproduced, modified, published, uploaded, posted, transmitted,
distributed, or disclosed in any way without Intel's prior express written permission.

No license under any patent, copyright, trade secret or other intellectual property
right is granted to or conferred upon you by disclosure or delivery of the Materials,
either expressly, by implication, inducement, estoppel or otherwise. Any license
under such intellectual property rights must be express and approved by Intel in writing.

--*/

#ifndef _CP_HECI_LOG_H_
#define _CP_HECI_LOG_H_

#include "cp_debug.h"

#define PAVP_MESESSION_FUNCTION_ENTER   CP_FUNCTION_ENTER(MOS_CP_SUBCOMP_DLL)
#define PAVP_MESESSION_FUNCTION_EXIT(hr) CP_FUNCTION_EXIT(MOS_CP_SUBCOMP_DLL, hr)

#define PAVP_MESESSION_ASSERT(_expr)                                          \
    CP_ASSERT(MOS_CP_SUBCOMP_DLL, _expr)

#define PAVP_MESESSION_ASSERTMESSAGE(_message, ...)                           \
    CP_ASSERTMESSAGE(MOS_CP_SUBCOMP_DLL, _message, ##__VA_ARGS__)

#define PAVP_MESESSION_VERBOSEMESSAGE(_message, ...)                          \
    CP_VERBOSEMESSAGE(MOS_CP_SUBCOMP_DLL, _message, ##__VA_ARGS__)

#define PAVP_MESESSION_NORMALMESSAGE(_message, ...)                           \
    CP_NORMALMESSAGE(MOS_CP_SUBCOMP_DLL, _message, ##__VA_ARGS__)

#endif // _CP_HECI_LOG_H_
