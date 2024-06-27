/*++

INTEL CONFIDENTIAL
Copyright 2011-2018 Intel Corporation All Rights Reserved.

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

#ifndef _CP_HECI_DEFS_H_
#define _CP_HECI_DEFS_H_

#include <windows.h>
#include <winioctl.h>
#define FILE_DEVICE_HECI 0x8000
#define MAX_RECONNECTION_ATTEMPTS 100
#define SLEEP_PERIOD_PER_ATTEMPT 50  // ms
#define IOCTL_HECI_CONNECT_CLIENT \
    CTL_CODE(FILE_DEVICE_HECI, 0x801, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

// HECI GUID(Global Unique ID): {E2D1FF34-3458-49A9-88DA-8E6915CE9BE5}
const GUID HeciGuid = {0xE2D1FF34, 0x3458, 0x49A9, 0x88, 0xDA, 0x8E, 0x69, 0x15, 0xCE, 0x9B, 0xE5};
const GUID PavpGuid = {0xfbf6fcf1, 0x96cf, 0x4e2e, 0xa6, 0xa6, 0x1b, 0xab, 0x8c, 0xbe, 0x36, 0xb1};
const GUID WidiGuid = {0xb638ab7e, 0x94e2, 0x4ea2, 0xa5, 0x52, 0xd1, 0xc5, 0x4b, 0x62, 0x7f, 0x04};

// values copied from PAVP DLL
const uint32_t DEF_SEND_TIMEOUT = 20000;
const uint32_t DEF_RECV_TIMEOUT = 20000;
const uint32_t DEF_LOCK_TIMEOUT = 20000;

#pragma pack(push, 1)
typedef struct _HECI_CLIENT
{
    uint32_t MaxMessageLength;
    uint8_t  ProtocolVersion;
} HECI_CLIENT;
#pragma pack(pop)

#endif  //_CP_HECI_DEFS_H_