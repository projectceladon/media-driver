/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2012-2020
Intel Corporation All Rights Reserved.

The source code contained or described herein and all documents related to the
source code ("Material") are owned by Intel Corporation or its suppliers or
licensors. Title to the Material remains with Intel Corporation or its suppliers
and licensors. The Material contains trade secrets and proprietary and confidential
information of Intel or its suppliers and licensors. The Material is protected by
worldwide copyright and trade secret laws and treaty provisions. No part of the
Material may be used, copied, reproduced, modified, published, uploaded, posted,
transmitted, distributed, or disclosed in any way without Intel's prior express
written permission.

No license under any patent, copyright, trade secret or other intellectual
property right is granted to or conferred upon you by disclosure or delivery
of the Materials, either expressly, by implication, inducement, estoppel
or otherwise. Any license under such intellectual property rights must be
express and approved by Intel in writing.

File Name: cp_debug.h

Abstract:
    This defines macros for content protection debugging and message logging.


Environment:
    OS agnostic

Notes:
    None

======================= end_copyright_notice ==================================*/
#ifndef __CP_DEBUG_H__
#define __CP_DEBUG_H__

#include "mos_util_debug.h"
#include "cp_os_defs.h"

#define CP_ASSERT(subComponent, _expr)                                                   \
    MOS_ASSERT(MOS_COMPONENT_CP, subComponent, _expr)

#define CP_ASSERTMESSAGE(subComponent, _message, ...)                                    \
    MOS_ASSERTMESSAGE(MOS_COMPONENT_CP, subComponent, _message, ##__VA_ARGS__)

#define CP_NORMALMESSAGE(subComponent, _message, ...)                                    \
    MOS_NORMALMESSAGE(MOS_COMPONENT_CP, subComponent, _message, ##__VA_ARGS__)

#define CP_VERBOSEMESSAGE(subComponent, _message, ...)                                   \
    MOS_VERBOSEMESSAGE(MOS_COMPONENT_CP, subComponent, _message, ##__VA_ARGS__)

#define CP_FUNCTION_ENTER(subComponent)                                                  \
    MOS_FUNCTION_ENTER(MOS_COMPONENT_CP, subComponent)

#define CP_FUNCTION_EXIT(subComponent, hr)                                               \
    MOS_FUNCTION_EXIT(MOS_COMPONENT_CP, subComponent, hr)

#define CP_CHK_STATUS(subComponent, _stmt)                                               \
    MOS_CHK_STATUS(MOS_COMPONENT_CP, subComponent, _stmt)

#define CP_CHK_STATUS_WITH_HR(subComponent, _stmt)                                       \
{                                                                                        \
    eStatus = (MOS_STATUS)(_stmt);                                                       \
    if (eStatus != MOS_STATUS_SUCCESS)                                                   \
    {                                                                                    \
        MOS_ASSERTMESSAGE(MOS_COMPONENT_CP, subComponent, "eStatus = 0x%x", eStatus);    \
        hr = E_FAIL;                                                                     \
        goto finish;                                                                     \
    }                                                                                    \
}

#define CP_CHK_STATUS_MESSAGE_WITH_HR(subComponent, _stmt, _message, ...)                \
{                                                                                        \
    eStatus = (MOS_STATUS)(_stmt);                                                       \
    if (eStatus != MOS_STATUS_SUCCESS)                                                   \
    {                                                                                    \
        MOS_ASSERTMESSAGE(MOS_COMPONENT_CP, subComponent, _message, ##__VA_ARGS__);      \
        hr = E_FAIL;                                                                     \
        goto finish;                                                                     \
    }                                                                                    \
}

#define CP_CHK_STATUS_WITH_HR_RETURN(subComponent, _stmt)                                \
{                                                                                        \
    eStatus = (MOS_STATUS)(_stmt);                                                       \
    if (eStatus != MOS_STATUS_SUCCESS)                                                   \
    {                                                                                    \
        MOS_ASSERTMESSAGE(MOS_COMPONENT_CP, subComponent, "eStatus = 0x%x", eStatus);    \
        hr = E_FAIL;                                                                     \
        return hr;                                                                       \
    }                                                                                    \
}

#define CP_CHK_STATUS_WITH_HR_BREAK(subComponent, _stmt)                                 \
{                                                                                        \
    eStatus = (MOS_STATUS)(_stmt);                                                       \
    if (eStatus != MOS_STATUS_SUCCESS)                                                   \
    {                                                                                    \
        MOS_ASSERTMESSAGE(MOS_COMPONENT_CP, subComponent, "eStatus = 0x%x", eStatus);    \
        hr = E_FAIL;                                                                     \
        break;                                                                           \
    }                                                                                    \
}

#define CP_CHK_STATUS_MESSAGE(subComponent, _stmt, _message, ...)                        \
    MOS_CHK_STATUS_MESSAGE(MOS_COMPONENT_CP, subComponent, _stmt, _message, ##__VA_ARGS__)

#define CP_CHK_STATUS_RETURN(subComponent, _stmt)                                                            \
    MOS_CHK_STATUS_RETURN(MOS_COMPONENT_CP, subComponent, _stmt)

#define CP_CHK_NULL(subComponent, _ptr)                                                  \
    MOS_CHK_NULL(MOS_COMPONENT_CP, subComponent, _ptr)

#define CP_CHK_NULL_RETURN(subComponent, _ptr)                                                  \
    MOS_CHK_NULL_RETURN(MOS_COMPONENT_CP, subComponent, _ptr)

#define CP_CHK_NULL_NO_STATUS(subComponent, _ptr)                                        \
    MOS_CHK_NULL_NO_STATUS(MOS_COMPONENT_CP, subComponent, _ptr)

#define CP_CHK_HR_MESSAGE(subComponent, _stmt, _message)                                 \
    MOS_CHK_HR_MESSAGE(MOS_COMPONENT_CP, subComponent , _stmt, _message)

#define CP_CHK_NULL_WITH_HR(subComponent, _ptr)                                          \
    MOS_CHK_NULL_WITH_HR(MOS_COMPONENT_CP, subComponent, _ptr)


#define CP_DDI_ASSERTMESSAGE(_message, ...) \
    MOS_ASSERTMESSAGE(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_DDI, _message, ##__VA_ARGS__)

#define CP_DDI_NORMALMESSAGE(_message, ...) \
    MOS_NORMALMESSAGE(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_DDI, _message, ##__VA_ARGS__)

#define CP_DDI_VERBOSEMESSAGE(_message, ...) \
    MOS_VERBOSEMESSAGE(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_DDI, _message, ##__VA_ARGS__)
#endif // __CP_DEBUG_H__.
