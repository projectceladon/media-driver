/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2009-2017
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

======================= end_copyright_notice ==================================*/
//!
//! \file     cp_os_defs.h
//! \brief    OS specific related definitions for CP 
//!

#ifndef __CP_DEFS_OS_H__
#define __CP_DEFS_OS_H__

#ifndef E_TIMEOUT
#define E_TIMEOUT 0x8001011FL
#endif  // E_TIMEOUT

#if defined(_WIN32) || defined(WDDM_LINUX)

#else // _WIN32

#include <stdint.h>
#include <unistd.h>

#ifndef SUCCEEDED
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#endif // !SUCCEEDED

#ifndef FAILED
#define FAILED(hr) (((hr)) < 0)
#endif // !FAILED

//**************************************************************************************
// will be removed
typedef int32_t     BOOL, *PBOOL, *LPBOOL;      //!< Boolean value, should be TRUE or FALSE
typedef uint8_t     BOOLEAN, *PBOOLEAN;         //!< Boolean value, should be TRUE or FALSE

#define TRUE        1
#define FALSE       0

typedef char        CHAR, *PCHAR;               //!< 8 bit signed char
typedef char        *LPSTR, TCHAR, *LPTSTR;     //!< Pointer to 8 bit signed chars
typedef const char  *PCSTR, *LPCSTR, *LPCTSTR;

typedef int8_t      INT8, *PINT8;               //!< 8 bit signed value

#if !defined TYPEDEF_DEFINED_UCHAR
#define TYPEDEF_DEFINED_UCHAR 1
typedef uint8_t     UCHAR;                      //!< 8 bit unsigned value
#endif

typedef uint8_t     *PUCHAR;                    //!< 8 bit unsigned value
typedef uint8_t     UINT8, *PUINT8;             //!< 8 bit unsigned value
typedef uint8_t     BYTE, *PBYTE, *LPBYTE;      //!< 8 bit unsigned value

typedef int16_t     INT16, *PINT16;             //!< 16 bit signed value

#if !defined TYPEDEF_DEFINED_SHORT
#define TYPEDEF_DEFINED_SHORT 1
typedef int16_t     SHORT;                      //!< 16 bit signed value
#endif

typedef int16_t     *PSHORT;                    //!< 16 bit signed value

typedef uint16_t    UINT16, *PUINT16;           //!< 16 bit unsigned value
typedef uint16_t    USHORT, *PUSHORT;           //!< 16 bit unsigned value
typedef uint16_t    WORD, *PWORD;               //!< 16 bit unsigned value

typedef int32_t     INT, *PINT;                 //!< 32 bit signed value
typedef int32_t     INT32, *PINT32;             //!< 32 bit signed value
typedef int32_t     LONG, *PLONG;               //!< 32 bit unsigned value

typedef uint32_t    UINT, *PUINT;               //!< 32 bit unsigned value
typedef uint32_t    UINT32, *PUINT32;           //!< 32 bit unsigned value
typedef uint32_t    ULONG, *PULONG;             //!< 32 bit unsigned value
typedef uint32_t    DWORD, *PDWORD, *LPDWORD;   //!< 32 bit unsigned value

typedef int64_t     INT64, *PINT64;             //!< 64 bit signed value
typedef int64_t     LONGLONG, *PLONGLONG;       //!< 64 bit signed value
typedef int64_t     LONG64, *PLONG64;           //!< 64 bit signed value

typedef uint64_t    UINT64, *PUINT64;           //!< 64 bit unsigned value
typedef uint64_t    ULONGLONG;                  //!< 64 bit unsigned value
typedef uint64_t    ULONG64;                    //!< 64 bit unsigned value
typedef uint64_t    QWORD, *PQWORD;             //!< 64 bit unsigned value

typedef float       FLOAT, *PFLOAT;             //!< Floating point value
typedef double      DOUBLE;

typedef size_t      SIZE_T;                     //!< unsigned size value
typedef ssize_t     SSIZE_T;                    //!< signed size value

typedef uintptr_t   ULONG_PTR;                  //!< unsigned long type used for pointer precision
typedef uintptr_t   UINT_PTR;
typedef intptr_t    INT_PTR;

typedef void        VOID, *PVOID, *LPVOID;      //!< Void
typedef const void  *LPCVOID;                   //!< Const pointer to void

#define CONST const


typedef int32_t HRESULT;

#define S_OK                    0
#define S_FALSE                 1
#define E_ABORT                 (0x80004004)
#define E_FAIL                  (0x80004005)
#define E_OUTOFMEMORY           (0x8007000E)
#define E_INVALIDARG            (0x80070057)
#define ERROR_NOT_FOUND         (0x80070490)

typedef struct _GUID 
{
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t Data4[8];
} GUID;


// will be removed --end
//***************************************************************************************

#endif

#endif  // __CP_DEFS_OS_H__
