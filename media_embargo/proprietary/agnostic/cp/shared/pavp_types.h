/*========================== begin_copyright_notice ============================

INTEL CONFIDENTIAL

Copyright (C) 2011-2021 Intel Corporation

This software and the related documents are Intel copyrighted materials,
and your use of them is governed by the express license under which they were
provided to you ("License"). Unless the License provides otherwise,
you may not use, modify, copy, publish, distribute, disclose or transmit this
software or the related documents without Intel's prior written permission.

This software and the related documents are provided as is, with no express or
implied warranties, other than those that are expressly stated in the License.

============================= end_copyright_notice ===========================*/

// structs used by PAVP Device and state machine

#ifndef PAVP_TYPES_INCLUDE
#define PAVP_TYPES_INCLUDE

#include <stdio.h>

#if !defined(_WIN32)
#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif
#endif

////////////////////////////////////////////////////////////////////////
// This file should be minimized to include only types used by both the
// KmRender component and the PAVP Escape interface.
// Keep in mind that anything added to this file is also seen in UMD.
////////////////////////////////////////////////////////////////////////

#undef Status

#define INVALID_REG_OFFSET 0
typedef unsigned long      PAVP_GPU_REGISTER_OFFSET;

typedef enum
{
    PAVP_FRESHNESS_REQUEST_DECRYPT = 0,
    PAVP_FRESHNESS_REQUEST_ENCRYPT = 1,
} PAVP_FRESHNESS_REQUEST_TYPE;

typedef enum
{
    KM_PAVP_SESSION_TYPE_DECODE = 0,
    KM_PAVP_SESSION_TYPE_TRANSCODE = 1,
    KM_PAVP_SESSION_TYPE_WIDI = 2,
    KM_PAVP_SESSION_TYPES_MAX = 3
} KM_PAVP_SESSION_TYPE;

typedef enum
{
    KM_PAVP_SET_KEY_DECRYPT        = 0, // Update Sn_d
    KM_PAVP_SET_KEY_ENCRYPT        = 1, // Update Sn_e
    KM_PAVP_SET_KEY_BOTH           = 2, // Update both Sn_d and Sn_e
    KM_PAVP_SET_KEY_FIXED_EXCHANGE = 4, // Reset to a new S1 (for fixed key exchange)
} KM_PAVP_SET_KEY_TYPE;

// The enum values of KM_PAVP_SESSION_MODE and GMM_PAVP_MODE (gmmCommonDefns.h) should match .
// Linux KMD will use KM_PAVP_SESSION_MODE
// Windows KMD will use GMM_PAVP_MODE. Eventually, Win KMD should migrate to KM_PAVP_SESSION_MODE
typedef enum
{
    KM_PAVP_SESSION_MODE_UNKNOWN     = 0,
    KM_PAVP_SESSION_MODE_BIG_PCM     = 1, // Big PCM Mode
    KM_PAVP_SESSION_MODE_LITE        = 2, // Lite Mode
    KM_PAVP_SESSION_MODE_HEAVY       = 3, // Heavy Mode
    KM_PAVP_SESSION_MODE_PER_APP_MEM = 4, // Per App Mode
    KM_PAVP_SESSION_MODE_ISO_DEC     = 5, // Isolated Decode Mode
    KM_PAVP_SESSION_MODE_STOUT       = 6, // Isolated Decode with Authenticated EU
    KM_PAVP_SESSION_MODE_THV_D       = 7, // Isolated Display/Threadville-Display
    KM_PAVP_SESSION_MODE_HUC_GUARD   = 8, // HuC Signed cmd buff
    KM_PAVP_SESSION_MODES_MAX        = 9
} KM_PAVP_SESSION_MODE;

// PAVP display enable types
typedef enum 
{
    PAVP_PLANE_DISPLAY_C        = 0x0080,
    PAVP_PLANE_SPRITE_C         = 0x0100,
    PAVP_PLANE_CURSOR_C         = 0x0200,
    PAVP_PLANE_DISPLAY_A        = 0x0400,
    PAVP_PLANE_DISPLAY_B        = 0x0800,
    PAVP_PLANE_SPRITE_A         = 0x1000,
    PAVP_PLANE_SPRITE_B         = 0x2000,
    PAVP_PLANE_CURSOR_A         = 0x4000,
    PAVP_PLANE_CURSOR_B         = 0x8000
} PAVP_PLANE_ENABLE_TYPE;

// PAVP session status 
typedef enum 
{
    PAVP_CRYPTO_SESSION_STATUS_OK                   = 0x0,
    PAVP_CRYPTO_SESSION_STATUS_KEY_LOST             = 0x1,
    PAVP_CRYPTO_SESSION_STATUS_KEY_AND_CONTENT_LOST = 0x2
} PAVP_CRYPTO_SESSION_STATUS;

// PAVP key programming status 
typedef enum 
{
    PAVP_GET_SET_HW_KEY_NONE      = 0x0,    // Return App ID from UMD without intreaction with GPU and ME
    PAVP_SET_CEK_KEY_IN_ME        = 0x1,    // Program CEK in ME 
    PAVP_GET_CEK_KEY_FROM_ME      = 0x2,    // Obtain CEK from ME
    PAVP_SET_CEK_KEY_IN_GPU       = 0x3,    // Program key rotation blob on GPU
    PAVP_SET_CEK_INVALIDATE_IN_ME = 0x4     // Cancel CEK in ME
} PAVP_GET_SET_DATA_NEW_HW_KEY;

typedef struct 
{
    union {
        struct {
            unsigned char SessionIndex : 7; // PAVP Session Index for this App ID
            unsigned char SessionType  : 1; // PAVP HW App ID
        };
        unsigned char   SessionIdValue;
    };
} PAVP_SESSION_ID;

typedef PAVP_SESSION_ID* PPAVP_SESSION_ID;

struct PAVP_EXTENDED_SESSION_ID
{
    uint8_t         SessionID;  // 1 byte
    uint8_t         drmType;    // 1 byte
    uint8_t         pavpUse;    // 1 byte
    uint8_t         reserved;   // 1 byte
};

// cp context struct
typedef struct _CpContext
{
    union
    {
        uint32_t value;                       //!< Cp context in value, in name of CpTag
        struct
        {
            uint32_t sessionId          : 8;  //!< PAVP session id
            uint32_t instanceId         : 8;  //!< PAVP session instance id
            uint32_t enable             : 1;  //!< PAVP enable state
            uint32_t heavyMode          : 1;  //!< PAVP heavy mode state
            uint32_t isolatedDecodeMode : 1;  //!< PAVP isolated decode mode state
            uint32_t stoutMode          : 1;  //!< PAVP stout mode state
            uint32_t tearDown           : 1;  //!< PAVP teardown happens
            uint32_t                    : 11; //!< Reserved for future use.
        };
    };
} CpContext;

#define CP_SURFACE_TAG  0x3000f         // heavy mode session 15 for default cp surface tag

// 64 bit unique session handle
typedef struct 
{
    union
    {
        struct
        {
            uint64_t uiSessionId : 8;  // 1 byte
            uint64_t uiProcessId : 32; // 4 bytes
            uint64_t uiTimeStamp : 24; // 3 bytes (truncated timestamp)
        };
        struct
        {
            uint64_t PavpSessionHandleValue;
        };
    };
} PAVP_SESSION_HANDLE;

typedef PAVP_SESSION_HANDLE* PPAVP_SESSION_HANDLE;

typedef struct 
{
    void *pContext;
    void *pCryptoSession;
} PAVP_SESSION_ASSOCIATORS;

static const PAVP_SESSION_ASSOCIATORS PAVP_SESSION_ASSOCIATORS_INITIALIZER_INVALID =
{
    NULL,   // pContext
    NULL    // pCryptoSession
};

typedef enum 
{
    PAVP_ESC_STATUS_SUCCESS,
    PAVP_ESC_STATUS_INVALID_PROCESS_ID,
    PAVP_ESC_STATUS_INVALID_SESSION_ID,
    PAVP_ESC_STATUS_INVALID_SESSION_INDEX,
    PAVP_ESC_STATUS_UNEXPECTED_REFCOUNT,
    PAVP_ESC_STATUS_UNKNOWN,
    PAVP_ESC_STATUS_COUNT
} PAVP_ESCAPE_STATUS;

typedef struct 
{
    uint32_t   Nonce;
    uint32_t   ProtectedMemoryStatus[4];
    uint32_t   PortStatusType;
    uint32_t   DisplayPortStatus[4];
} PAVP_GET_CONNECTION_STATE_PARAMS;

typedef struct 
{
    uint32_t NonceIn;
    uint8_t  ConnectionStatus[32];
    uint8_t  ProtectedMemoryStatus[16];
} PAVP_GET_CONNECTION_STATE_PARAMS2;

typedef enum 
{
    PAVP_REGISTER_READ = 1,                 // Read Register
    PAVP_REGISTER_WRITE,                    // Write Register
    PAVP_REGISTER_WRITEREAD,                // Write Register, followed by read
    PAVP_REGISTER_READWRITE_AND,            // Read, AND value, write back out
    PAVP_REGISTER_READWRITE_OR,             // Read, OR value, write back out
    PAVP_REGISTER_READWRITE_XOR,            // Read, XOR value, write back out
} PAVP_GPU_REGISTER_OP;

//do not but conditional compiles in this as it must be common between UMD, KMD, Windows Android etc
typedef enum 
{
    PAVP_REGISTER_INVALID = 0,              //!< make 0 invalid as its common to make this mistake
    PAVP_REGISTER_APPID,                    //!< The App ID Register
    PAVP_REGISTER_SESSION_IN_PLAY,          //!< The Session in Play Register
    PAVP_REGISTER_STATUS,                   //!< The status register (for most recently active session)
    PAVP_REGISTER_DEBUG,                    //!< The silicon debug register (PCH pairing status, attack log, etc  
    PAVP_REGISTER_PAVPC,                    //!< The silicon PAVP Configuration register (should be locked by BIOS)
    PAVP_MFX_MODE_REGISTER,
    PAVP_CONTROL_REGISTER,
    PAVP_REGISTER_GLOBAL_TERMINATE,
    PAVP_REGISTER_VLV_GUNIT_DISPLAY_GCI_CONTROL_REG,
    PAVP_SECURE_OFFSET_REG,                 //!< Secure offset register used for signalling CB2 insertion into ring
    PAVP_REGISTER_EXPLICIT_TERMINATE,
    PAVP_REGISTER_KCR_INIT,
    PAVP_REGISTER_KCR_STATUS_1,
    PAVP_REGISTER_KCR_STATUS_2,
    PAVP_REGISTER_KCR_SESSION_IN_PLAY,
    PAVP_REGISTER_KCR_APP_CHAR,
    PAVP_REGISTER_WIDI0_KCR_COUNTER_DW1,
    PAVP_REGISTER_WIDI0_KCR_COUNTER_DW2,
    PAVP_REGISTER_WIDI0_KCR_COUNTER_DW3,
    PAVP_REGISTER_WIDI0_KCR_COUNTER_DW4,
    PAVP_REGISTER_WIDI1_KCR_COUNTER_DW1,
    PAVP_REGISTER_WIDI1_KCR_COUNTER_DW2,
    PAVP_REGISTER_WIDI1_KCR_COUNTER_DW3,
    PAVP_REGISTER_WIDI1_KCR_COUNTER_DW4,
    PAVP_REGISTER_MSG_REQ_GFX_KEY,
    PAVP_REGISTER_MSG_DELIVER_GFX_KEY_1,
    PAVP_REGISTER_MSG_DELIVER_GFX_KEY_2,
    PAVP_REGISTER_MSG_DELIVER_GFX_KEY_3,
    PAVP_REGISTER_MSG_DELIVER_GFX_KEY_4,
    PAVP_REGISTER_MSG_CHECK_GFX_KEY_1,
    PAVP_REGISTER_MSG_CHECK_GFX_KEY_2,
    PAVP_REGISTER_MSG_CHECK_GFX_KEY_3,
    PAVP_REGISTER_MSG_CHECK_GFX_KEY_4,
    PAVP_REGISTER_MSG_CHECK_GFX_KEY_STATUS,
    PAVP_REGISTER_MSG_PAVP_OPERATION,
    PAVP_REGISTER_SECURE_CHARACTERISTIC_1,
    PAVP_REGISTER_SECURE_CHARACTERISTIC_2,
    PAVP_REGISTER_SECURE_CHARACTERISTIC_3,
    PAVP_REGISTER_SECURE_CHARACTERISTIC_4,
    PAVP_REGISTER_SECURE_KEY_1,
    PAVP_REGISTER_SECURE_KEY_2,
    PAVP_REGISTER_SECURE_KEY_3,
    PAVP_REGISTER_SECURE_KEY_4,
    PAVP_REGISTER_MSG_DELIVER_SESSION_KEY_1,
    PAVP_REGISTER_MSG_DELIVER_SESSION_KEY_2,
    PAVP_REGISTER_MSG_DELIVER_SESSION_KEY_3,
    PAVP_REGISTER_MSG_DELIVER_SESSION_KEY_4,
    PAVP_REGISTER_MSG_DELIVER_WIDI_KEY_1,
    PAVP_REGISTER_MSG_DELIVER_WIDI_KEY_2,
    PAVP_REGISTER_MSG_DELIVER_WIDI_KEY_3,
    PAVP_REGISTER_MSG_DELIVER_WIDI_KEY_4,
    PAVP_REGISTER_MSG_PAVP_OPERATION_DONE,
    PAVP_REGISTER_SEND_MSG_HW_STATUS_1,
    PAVP_REGISTER_SEND_MSG_HW_STATUS_2,
    PAVP_REGISTER_SEND_MSG_HW_STATUS_3,
    PAVP_REGISTER_SEND_MSG_HW_STATUS_4,
    PAVP_REGISTER_TRANSCODE_SESSION_IN_PLAY_1,    //!< For DG2+, transcode sessions (0-31) in Play are queried from Register 32264
    PAVP_REGISTER_TRANSCODE_SESSION_IN_PLAY_2,    //!< For DG2+, transcode sessions (32-63) in Play are queried from Register 32268
    PAVP_REGISTER_TRANSCODE_STOUT_STATUS_1,       //!< For DG2+, transcode sessions (0-31) stout mode query are from Register 32674
    PAVP_REGISTER_TRANSCODE_STOUT_STATUS_2,       //!< For DG2+, transcode sessions (32-63) stout mode query are from Register 32678
    PAVP_REGISTER_MAX_ENUM_VALUE
} PAVP_GPU_REGISTER_TYPE;

typedef enum 
{
    PAVP_GFX_ADDRESS_MULTIPATCHS  = -1,     // Multiple patch locations 
    PAVP_GFX_ADDRESS_SINGLEPATCH  = 0       // Single patch location 
} PAVP_GFX_ADDRESS_PATCHS;

typedef unsigned long      PAVP_GPU_REGISTER;
typedef unsigned long long PAVP_GPU_REGISTER_VALUE;

// These are states a PAVP session goes through to become established or torn down.
// Refer to the pcp_pavp_session_states document in the content protection share for more information
typedef enum
{
    PAVP_SESSION_GET_ANY_DECODE_SESSION_IN_USE = 0x0, // Input to check if any decode session is in use
    PAVP_SESSION_GET_SESSION_STATUS,                // Input to request the status of the current session
    PAVP_SESSION_UNSUPPORTED,                       // Can't be used, session slots are marked with this at boot
    PAVP_SESSION_POWERUP,                           // Driver has initialized (powered up), UMD can setup for PAVP
    PAVP_SESSION_POWERUP_IN_PROGRESS,               // UMD is in process of running power up tasks, indicates to other processes this is in progress
    PAVP_SESSION_UNINITIALIZED,                     // Available, but init sequence required
    PAVP_SESSION_IN_INIT,                           // In initialization process, slot is reserved by UMD process
    PAVP_SESSION_INITIALIZED,                       // Init sequence complete (or not required) but key exchange still pending
    PAVP_SESSION_READY,                             // Key exchange and plane enables all complete, ready for decode
    PAVP_SESSION_TERMINATED,                        // Indicates this session slot's UMD process has crashed, will be recovered by UMD when reserved.
    PAVP_SESSION_TERMINATE_DECODE,                  // Called from UMD Pavp_Destroy
    PAVP_SESSION_TERMINATE_WIDI,                    // Called with the Widi app is closing
    PAVP_SESSION_TERMINATE_IN_PROGRESS,             // Called when a session is about to be destroyed gracefully by the UMD process
    PAVP_SESSION_IN_USE,                            // Returned when all sessions are in use
    PAVP_SESSION_ATTACKED,                          // Session has been attacked via autoteardown
    PAVP_SESSION_ATTACKED_IN_PROGRESS,              // UMD is recovering from attacked state, indicates to other processes this is in progress
    PAVP_SESSION_RECOVER,                           // We have recovered from powerup or attack, puts sessions in UNINITIALIZED state, ready for use
    PAVP_SESSION_RECOVER_IN_PROGRESS,               // Called when UMD is about to be recover from attack.
    PAVP_SESSION_POWERUPINIT,                       // WiDi only - WiDi needs to do power up init from the KMD via this status
    PAVP_SESSION_POWERUP_RESERVED,                  // Session has been attacked, but had been reserved when this happened
    PAVP_SESSION_POWERUP_RESERVED_IN_PROGRESS,      // This is a powerup reserved session whose recovery is handled by some process
    PAVP_SESSION_POWERUP_RESERVED_RECOVERED,        // powerup recovery has completed and this session is awaiting uninitialization by its owner
    PAVP_SESSION_ATTACKED_RESERVED,                 // Session has been attacked, but had been reserved when this happened
    PAVP_SESSION_ATTACKED_RESERVED_IN_PROGRESS,     // This is an attacked reserved session whose recovery is handled by some process
    PAVP_SESSION_ATTACKED_RESERVED_RECOVERED,       // Attack recovery has completed and this session is awaiting uninitialization by its owner
    PAVP_SESSION_UNKNOWN          = 0xFFFFFFFF      // Indicates unknown or meaningless state
} PAVP_SESSION_STATUS;

typedef enum _PXP_SM_SESSION_REQ
{
    PXP_SM_REQ_SESSION_ID_INIT                      = 0x0,          // Request KMD to allocate session id and move it to IN INIT
    PXP_SM_REQ_SESSION_IN_PLAY,                                     // Inform KMD that UMD has completed the initialization
    PXP_SM_REQ_SESSION_TERMINATE                                    // Request KMD to terminate the session
} PXP_SM_SESSION_REQ;

typedef enum _PXP_UMD_REQUEST
{
    PXP_ACTION_QUERY_CP_CONTEXT,
    PXP_ACTION_SET_SESSION_STATUS,
    PXP_ACTION_TEE_COMMUNICATION,
    PXP_ACTION_HOST_SESSION_HANDLE_REQ
}PXP_UMD_REQUEST;

typedef enum _PXP_SM_STATUS
{
    PXP_SM_STATUS_SUCCESS,
    PXP_SM_STATUS_RETRY_REQUIRED,
    PXP_SM_STATUS_SESSION_NOT_AVAILABLE,
    PXP_SM_STATUS_INSUFFICIENT_RESOURCE,
    PXP_SM_STATUS_ERROR_UNKNOWN
}PXP_SM_STATUS;

// These function are implemented in cp_pavpdevice_new_sm.cpp.
extern const char *StateMachineStatus2Str(PXP_SM_STATUS Status);
extern const char *StateMachineRequest2Str(PXP_SM_SESSION_REQ Status);

// This function is implemented in cp_pavpdevice.cpp.
extern const char *SessionStatus2Str(PAVP_SESSION_STATUS Status);

//bit field definitions for the above registers
#define PAVP_REGISTER_GLOBAL_TERMINATE_BIT_DECODE               0x1
#define PAVP_REGISTER_EXPLICIT_TERMINATE_DECODE_MASK            0x4
#define PAVP_REGISTER_DEBUG_BIT_OMAC_VERIFICATION               0x8000
#define PAVP_REGISTER_SESSION_IN_PLAY_DECODE_SESSION_MASK       0x0000FFFF
// Mask to check if Decoder Session 15 is alive or not 
#define PAVP_REGISTER_SESSION_IN_PLAY_ARBITRATOR_SESSION_MASK   0x8000

#define PAVP_INVALID_SESSION_ID      0xFF

// Invalid handle is defined as 0x0 for the following reasons:
// - Process ID can never be 0
// - Timestamp can never be 0
#define PAVP_INVALID_SESSION_HANDLE 0x0

#define PAVP_REGISTER_PAVPC_PAVPLOCK    0x4
#define PAVP_REGISTER_PAVPC_PAVPE       0x2           // enable
#define PAVP_REGISTER_PAVPC_PCME        0x1           // PAVP:[0]--Protected Content Memory Enable
#define PAVP_REGISTER_PAVPC_DEFAULT     0xAF900007    // from IVB WIn BIOS, use at own risk
#define PAVP_SECURE_OFFSET_DEF_VAL      0x000C0000    // from VLV
#define PAVP_ROTATION_KEY_BLOB_SIZE     0x10          // AES key blob size - 16 bytes
#define PAVP_STOUT_TRANSCODE_INDEX_MASK 0x20          // the mask for stout transcode sessions index

#endif // PAVP_TYPES_INCLUDE
