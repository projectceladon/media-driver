/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2008-2017
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

File Name: intel_pavp_heci_api.h

Abstract:
    heci command definition header file

Environment:
    Windows Vista/Linux

Notes:

======================= end_copyright_notice ==================================*/
#ifndef _INTEL_PAVP_HECI_API_H_
#define _INTEL_PAVP_HECI_API_H_

#include "cp_os_defs.h"

// The maximal size of IGD pricate data returned from PAVP DLL (4KB).
#define FIRMWARE_MAX_IGD_DATA_SIZE  (4*1024)

// A bit specifying whether the FW response contains IGD private data.
#define FIRMWARE_PRIVATE_INFO_FLAG  0x80000000

// The size of the connection status buffer expected by FW.
#define FW_CONNECTION_STATUS_SIZE   32

// The size of firmware wrapped key
#define FIRMWARE_KEY_SIZE           16

// The size of max firmware wrapped key in API definition
#define FIRMWARE_KEY_SIZE_MAX       32

// PAVP FW Commands
#define FIRMWARE_CMD_GET_ME_CERT_GROUP_ID   0x00000000
#define FIRMWARE_CMD_SET_PAVP_SLOT          0x00000008
#define FIRMWARE_CMD_GET_PLANE_ENABLE       0x0000000A
#define FIRMWARE_CMD_GET_DISPLAY_NONCE      0x0000000B
#define FIRMWARE_CMD_VERIFY_CONN_STATUS     0x0000000C
#define FIRMWARE_CMD_GET_PCH_CAPS           0x00000009
#define FIRMWARE_CMD_CHECK_PAVP_MODE        0x0000000D

#define FIRMWARE_CMD_SET_PAVP_SLOT_EX         0x00000011
#define FIRMWARE_CMD_GET_PLANE_ENABLE_EX      0x00000012
#define FIRMWARE_CMD_GET_DISPLAY_NONCE_EX     0x00000013
#define FIRMWARE_CMD_VERIFY_CONN_STATUS_EX    0x00000014
#define FIRMWARE_CMD_CHECK_PAVP_MODE_EX       0x00000015
#define FIRMWARE_CMD_INVALIDATE_PAVP_SLOT     0x00000016
#define FIRMWARE_CMD_DAISY_CHAIN              0x0000001D

//PAVP FW4 Commands
#define FIRMWARE_CMD_PAVP_INIT4               0x0000001E
#define FIRMWARE_CMD_GET_PLANE_ENABLE4        0x00000020
#define FIRMWARE_CMD_INVALIDATE_PAVP_SLOT4    0x00000021
#define FIRMWARE_CMD_GET_KEY_BLOB             0x00000023
#define FIRMWARE_CMD_GET_HDCP_STATE           0x00000024

//PAVP FW42 Commands
#define FIRMWARE_42_CMD_GET_PCH_CAPABILITIES  0x00000028
#define FIRMWARE_42_CMD_DAISY_CHAIN           0x00000033
#define FIRMWARE_42_OPEN_SIGMA_SESSION        0x0000002B
#define FIRMWARE_42_GET_STREAM_KEY            0x0000002F
#define FIRMWARE_42_CMD_GET_KEY_BLOB          0x00000031

//PAVP FW43 Commands
#define FIRMWARE_43_CMD_SIGMA_SUBSESSION_CREATE         0x00000034
#define FIRMWARE_43_CMD_SIGMA_SUBSESSION_DESTROY        0x00000035
#define FIRMWARE_43_CMD_PAVP_INIT                       0x00000036
#define FIRMWARE_43_CMD_GET_SECURE_COMPPUTE_KEYS        0x00000037


//Enable HucStreamOut in Lite Mode
#define PAVP_CMD_ID_DBG_INJECT_CHARACTERISTIC_REG_EX  0x00000042

// WideVine FW Commands
#define FIRMWARE_CMD_GET_WV_KEY                  0x000A00F1
#define FIRMWARE_CMD_GET_WV_HEART_BEAT_NONCE     0x000A000C
#define FIRMWARE_CMD_VERIFY_WV_HEART_BEAT_STATUS 0x000A000D
#define FIRMWARE_CMD_INIT_DMA                    0x000A0002
#define FIRMWARE_CMD_UNINIT_DMA                  0x000A000B

// PLAY READY FW Commands
#define FIRMWARE_CMD_PORTING_KIT_METHOD             0x000A00F4
#define FIRMWARE_PR_OUTPUT_BUFFER_MAX_SIZE          0x2000
#define FIRMWARE_CMD_GET_DBG_HEART_BEAT_ENC_KEY     0x000D0F05
#define FIRMWARE_CMD_GET_PR_HEART_BEAT_NONCE        0x000D0006
#define FIRMWARE_CMD_VERIFY_PR_HEART_BEAT_STATUS    0x000D0007
#define FIRMWARE_CMD_GET_PR_PK_VERSION              0x000D0008

// Authenticated Kernel Commands
#define FIRMWARE_CMD_AUTH_KERNEL_SETUP              0x00AA0001
#define FIRMWARE_CMD_AUTH_KERNEL_SEND               0x00AA0002
#define FIRMWARE_CMD_AUTH_KERNEL_DONE               0x00AA0003

// PAVP FW API Versions
#define FIRMWARE_API_VERSION_1_5        ((1 << 16) | (5))
#define FIRMWARE_API_VERSION_2          ((2 << 16) | (0))
#define FIRMWARE_API_VERSION_3          ((3 << 16) | (0))
#define FIRMWARE_API_VERSION_32         ((3 << 16) | (2))
#define FIRMWARE_API_VERSION_4          ((4 << 16) | (0))
#define FIRMWARE_API_VERSION_4_1        ((4 << 16) | (1))
#define FIRMWARE_API_VERSION_4_2        ((4 << 16) | (2))
#define FIRMWARE_API_VERSION_4_3        ((4 << 16) | (3))

// AppId present mask
#define PAVP_CMD_STATUS_APPID_PRESENT_MASK 0x80000000
#define PAVP_CMD_STATUS_SET_APPID(PavpCmdStatus, AppId) (PavpCmdStatus |= (PAVP_CMD_STATUS_APPID_PRESENT_MASK | AppId));

// AppId present mask for FW4.2
#define PAVP42_CMD_STATUS_APPID_PRESENT_MASK 0x00000001
#define PAVP42_CMD_STATUS_APPID_MASK 0x0000000ff
#define PAVP42_CMD_STATUS_APPID_TRANSCODE_MASK 0x00000002
#define PAVP42_CMD_STATUS_SET_APPID(PavpCmdStatus, AppId) (PavpCmdStatus |= (PAVP42_CMD_STATUS_APPID_PRESENT_MASK | ((AppId << 2) & PAVP42_CMD_STATUS_APPID_MASK) | ((AppId >> 6) & PAVP42_CMD_STATUS_APPID_TRANSCODE_MASK)));

// Simulator Defines
#define WVFIRMWARE_API_VERSION_1        ((1 << 16) | (0))

#pragma pack(push)
#pragma pack(1)

// PAVP FW Input/Output Structs:
/*
**    Input/output message buffer common header
*/
typedef struct _PAVPCmdHdr 
{
    uint32_t    ApiVersion;
    uint32_t    CommandId;
    uint32_t    PavpCmdStatus;
    uint32_t    BufferLength;
} PAVPCmdHdr;

// PAVP FW empty Input/Output struct:
typedef struct NoDataBuff
{
    PAVPCmdHdr   Header;
} NoDataBuffer;

// FIRMWARE_CMD_PAVP_INIT4 input struct
typedef struct _PavpInit4InBuff
{
    PAVPCmdHdr  Header;
    uint32_t    PavpMode;
    uint32_t    AppID;
} PavpInit4InBuff;

// FIRMWARE_CMD_PAVP_INIT4 output struct
typedef NoDataBuffer PavpInit4OutBuff;

// init flag for FIRMWARE_CMD_PAVP_INIT43
typedef struct _PavpInit43Flags
{
    union
    {
        uint32_t Value;
        struct
        {
            uint32_t SubSessionEnabled    : 1;
            uint32_t DisableHuc           : 1;
            uint32_t                      : 30;  //Must be 0
        };
    };
} PavpInit43Flags;

// FIRMWARE_CMD_PAVP_INIT43 input struct
typedef struct _PavpInit43InBuff
{
    PAVPCmdHdr      Header;  //With PAVP Stream ID
    uint32_t        PavpMode;
    uint32_t        SubSessionId;
    PavpInit43Flags InitFlags;
    uint8_t         Signature[48];
} PavpInit43InBuff;

// FIRMWARE_CMD_PAVP_INIT43 output struct
typedef struct _PavpInit43OutBuff
{
    PAVPCmdHdr Header;
    uint8_t      GscGetKeysNounce[32];
} PavpInit43OutBuff;

// PAVP_Store_Key_Blob input struct
typedef struct _PAVPStoreKeyBlobInputBuff
{
    PavpInit43Flags InitFlags;
    uint32_t        SubSessionId;
    uint8_t         Signature[48];
} PAVPStoreKeyBlobInputBuff;

// FIRMWARE_CMD_SET_PAVP_SLOT input struct
typedef struct _SetSlotInBuff
{
    PAVPCmdHdr   Header;
    uint32_t     PavpSlot;
} SetSlotInBuff;

// FIRMWARE_CMD_SET_PAVP_SLOT output struct
typedef NoDataBuffer SetSlotOutBuff;

typedef struct _PavpCmdDbgInjectCharacteristicRegExIn
{
    PAVPCmdHdr   Header;
    uint32_t     Mask[4];
    uint32_t     CharacteristicReg[4];
} PavpCmdDbgInjectCharacteristicRegExIn;

typedef struct _PavpCmdDbgInjectCharacteristicRegExOut
{
    PAVPCmdHdr   Header;
} PavpCmdDbgInjectCharacteristicRegExOut;

// FIRMWARE_CMD_GET_PLANE_ENABLE input struct for API v2
typedef struct _GetPlaneEnableInBuff_api2
{
    PAVPCmdHdr   Header;
    uint8_t      PlaneEnables;
} GetPlaneEnableInBuff_api2;

// FIRMWARE_CMD_GET_PLANE_ENABLE input struct for API v3
typedef struct _GetPlaneEnableInBuff_api3
{
    PAVPCmdHdr   Header;
    uint8_t      MemoryMode;
    uint16_t     PlaneEnables;
} GetPlaneEnableInBuff_api3;

// FIRMWARE_CMD_GET_PLANE_ENABLE input struct for API v32
typedef struct _GetPlaneEnableInBuff_api32
{
    PAVPCmdHdr   Header;
    uint8_t      MemoryMode;
    uint16_t     PlaneEnables;
    uint32_t     PavpSlot;
} GetPlaneEnableInBuff_api32;

// FIRMWARE_CMD_GET_PLANE_ENABLE input struct for API v4
typedef struct _GetPlaneEnableInBuff_api4
{
    PAVPCmdHdr   Header;
    uint8_t      MemoryMode;
    uint16_t     PlaneEnables;
    uint32_t     AppID;
} GetPlaneEnableInBuff_api4;

// FIRMWARE_CMD_GET_PLANE_ENABLE output struct
typedef struct _GetPlaneEnableOutBuff
{
    PAVPCmdHdr   Header;
    uint8_t      EncPlaneEnables[16];
} GetPlaneEnableOutBuff;

// FIRMWARE_CMD_GET_DISPLAY_NONCE input struct
typedef struct _GetDispNonceInBuff
{
    PAVPCmdHdr   Header;
    uint32_t     resetStreamKey;
} GetDispNonceInBuff;

// FIRMWARE_CMD_GET_DISPLAY_NONCE_EX input struct
typedef struct _GetDispNonceExInBuff
{
    PAVPCmdHdr    Header;
    uint32_t      resetStreamKey;
    uint32_t      PavpSlot;
} GetDispNonceExInBuff;

// FIRMWARE_CMD_GET_DISPLAY_NONCE output struct
typedef struct _GetDispNonceOutBuff
{
   PAVPCmdHdr    Header;
   uint8_t       EncryptedE0[16];
   uint32_t      Nonce;
} GetDispNonceOutBuff;

// FIRMWARE_CMD_VERIFY_CONN_STATUS input struct for API v2
typedef struct _VerifyConnStatusInBuff_api2
{
    PAVPCmdHdr   Header;
    uint8_t      ConnectionStatus[16];
} VerifyConnStatusInBuff_api2;

// FIRMWARE_CMD_VERIFY_CONN_STATUS input struct for API v3
typedef struct _VerifyConnStatusInBuff_api3
{
    PAVPCmdHdr  Header;
    uint8_t     ConnectionStatus[32];
} VerifyConnStatusInBuff_api3;

// FIRMWARE_CMD_VERIFY_CONN_STATUS input struct for API v32
typedef struct _VerifyConnStatusInBuff_api32
{
    PAVPCmdHdr   Header;
    uint8_t      ConnectionStatus[32];
    uint32_t     PavpSlot;
} VerifyConnStatusInBuff_api32;

// FIRMWARE_CMD_VERIFY_CONN_STATUS output struct
typedef NoDataBuffer VerifyConnStatusOutBuff;

// FIRMWARE_CMD_CHECK_PAVP_MODE input struct
typedef struct _CheckPavpModeInBuff
{
    PAVPCmdHdr   Header;
    uint8_t      ProtectedMemoryStatus[16];
} CheckPavpModeInBuff;

// FIRMWARE_CMD_CHECK_PAVP_MODE_EX input struct
typedef struct _CheckPavpModeExInBuff
{
    PAVPCmdHdr   Header;
    uint8_t      ProtectedMemoryStatus[16];
    uint32_t     PavpSlot;
} CheckPavpModeExInBuff;

// FIRMWARE_CMD_CHECK_PAVP_MODE output struct
typedef NoDataBuffer CheckPavpModeOutBuff;


// WideVine FW Input/Output Structs:

// FIRMWARE_CMD_GET_WV_HEART_BEAT_NONCE input struct
// FIRMWARE_CMD_GET_PR_HEART_BEAT_NONCE input struct
typedef struct _GetHeartBeatNonceInBuff
{
    PAVPCmdHdr   Header;
    uint32_t     StreamId; // Only valid for Play Ready
} GetHeartBeatNonceInBuff;

// FIRMWARE_CMD_GET_WV_HEART_BEAT_NONCE output struct
// FIRMWARE_CMD_GET_PR_HEART_BEAT_NONCE output struct
typedef struct _GetHeartBeatNonceOutBuff
{
   PAVPCmdHdr    Header;
   uint32_t      Nonce;
} GetHeartBeatNonceOutBuff;

// FIRMWARE_CMD_VERIFY_WV_HEART_BEAT_STATUS input struct
// FIRMWARE_CMD_VERIFY_PR_HEART_BEAT_STATUS input struct
typedef struct _VerifyHeartBeatInBuff
{
    PAVPCmdHdr   Header;
    uint32_t     StreamId; // Only valid for Play Ready
    uint8_t      ConnectionStatus[FW_CONNECTION_STATUS_SIZE];
} VerifyHeartBeatInBuff;

// FIRMWARE_CMD_VERIFY_WV_HEART_BEAT_STATUS output struct
// FIRMWARE_CMD_VERIFY_PR_HEART_BEAT_STATUS output struct
typedef struct _VerifyHeartBeatOutBuff
{
    PAVPCmdHdr   Header;
    uint32_t     HdcpStatus;
} VerifyHeartBeatOutBuff;

typedef struct _GetDebugHeartBeatEncKeyIn
{
    PAVPCmdHdr   Header;
    uint32_t     StreamID;
} GetDebugHeartBeatEncKeyIn;

typedef struct _GetDebugHeartBeatEncKeyOut
{
    PAVPCmdHdr   Header;
    uint8_t      EncKey[FW_CONNECTION_STATUS_SIZE/2];
} GetDebugHeartBeatEncKeyOut;

// FIRMWARE_CMD_INIT_DMA input struct
typedef struct _GetWVInitDmaInBuff
{
    PAVPCmdHdr   Header;
    uint32_t     DmaHandle;
} GetWVInitDmaInBuff;

// FIRMWARE_CMD_INIT_DMA output struct
typedef struct _GetWVInitDmaOutBuff
{
   PAVPCmdHdr    Header;
   uint32_t      SglHandle;
} GetWVInitDmaOutBuff;

// FIRMWARE_CMD_UNINIT_DMA input struct
typedef struct _GetWVUnInitDmaInBuff
{
    PAVPCmdHdr   Header;
    uint32_t     SglHandle;
} GetWVUnInitDmaInBuff;

// FIRMWARE_CMD_UNINIT_DMA output struct
typedef NoDataBuffer GetWVUnInitDmaOutBuff;

// FIRMWARE_CMD_INVALIDATE_SLOT input struct
typedef struct _InvalidateSlotInBuf
{
    PAVPCmdHdr   Header;
    uint32_t     PavpSlot;
} InvalidateSlotInBuf;

// FIRMWARE_CMD_INVALIDATE_SLOT output struct
typedef NoDataBuffer InvalidateSlotOutBuf;

typedef struct _PavpStateInfo
{
    uint32_t   uPavpFwApiVersionInfo;
    uint32_t   bIsWidiPavp;
} PavpStateInfo;

// PLAY READY Structures
// FW's definition of pr30_is_play_ready_enabled_in_bios_in
typedef NoDataBuffer Pr30GetPlayReadySupportInBuf;

// FW's definition of pr30_is_play_ready_enabled_in_bios_out
typedef struct _Pr30GetPlayReadySupportOutBuf
{
    PAVPCmdHdr      Header;
    uint32_t        Major;
    uint32_t        Minor;
    uint32_t        Build;
    uint32_t        OFE;
} Pr30GetPlayReadySupportOutBuf;


// New CSME FW structures for translating keys to ASMF mode...
typedef struct _GetDaisyChainKeyInBuf
{
    PAVPCmdHdr      Header;
    uint32_t        Reserved0;  // The data in this field is unused by FW
    uint32_t        IsDKey;
    uint8_t         SnWrappedInPrevKey[FIRMWARE_KEY_SIZE];
    uint32_t        Reserved1;  // This 4 byte field was added to intentionally break
                             // backwards compatibility with old versions of the API which
                             // contained a security vulnerability.
} GetDaisyChainKeyInBuf;

typedef struct _GetDaisyChainKeyOutBuf
{
    PAVPCmdHdr      Header;
    uint32_t        Reserved0; // Always set to zero by FW
    uint8_t         Reserved1[FIRMWARE_KEY_SIZE]; //Private data field reserved for driver use only
} GetDaisyChainKeyOutBuf;

// New CSME FW structures for translating keys to ASMF mode...
typedef struct _GetDaisyChainKeyInBuf42
{
    PAVPCmdHdr  Header;
    uint32_t    key_type;  // 0 for decryption key, 1 for encryption key
    uint32_t    key_size;
    uint8_t     stream_key[FIRMWARE_KEY_SIZE_MAX];
} GetDaisyChainKeyInBuf42;

typedef struct _GetDaisyChainKeyOutBuf42
{
    PAVPCmdHdr  Header;
    uint32_t    key_size;
    uint8_t     wrapped_stream_key[FIRMWARE_KEY_SIZE_MAX];  //The stream key wrapped with Kb
} GetDaisyChainKeyOutBuf42;

// OLD CSME FW structures for translating keys to ASMF mode
typedef struct _GetDaisyChainKeyInBufLegacyAPI
{
    PAVPCmdHdr  Header;
    uint32_t    SessionId;
    uint32_t    IsDKey;
    uint8_t     SnWrappedInPrevKey[FIRMWARE_KEY_SIZE];
} GetDaisyChainKeyInBufLegacyAPI;

// Authenticated kernel defines
#define EK_APP_ID                       (0x86)
#define RSA_KEY_BYTE_SIZE               (256)
#define CERT_MAX_BYTE_SIZE              (0x500)
#define MAX_KERNEL_SIZE_PER_HECI        (1024*10)
#define METADATA_INFO_VERSION_0_0       (0x00000000)

// Authenticated kernel status (informational snapshot of pavp_status_t in FW code base)
typedef enum _AuthKernelHeciStatus
{
    ENCRYPTED_KERNEL_SUCCESS                = 0,
    ENCRYPTED_KERNEL_SUCCESS_PRIV_DATA      = 0x80000000,               ///< No usage 
    ENCRYPTED_KERNEL_FAIL_INVALID_PARAMS    = 0x000E0001,               ///< 0x000E0001
    ENCRYPTED_KERNEL_INVALID_INPUT_HEADER,                              ///< 0x000E0002
    ENCRYPTED_KERNEL_UNKNOWN_ERROR,                                     ///< 0x000E0003
    ENCRYPTED_KERNEL_ROOT_CERT_ERROR,                                   ///< 0x000E0004
    ENCRYPTED_KERNEL_ISV_CERT_ERROR_1,                                  ///< 0x000E0005
    ENCRYPTED_KERNEL_ISV_CERT_ERROR_2,                                  ///< 0x000E0006
    ENCRYPTED_KERNEL_ISV_CERT_ERROR_3,                                  ///< 0x000E0007
    ENCRYPTED_KERNEL_ISV_CERT_ERROR_4,                                  ///< 0x000E0008
    ENCRYPTED_KERNEL_ISV_CERT_ERROR_5,                                  ///< 0x000E0009
    ENCRYPTED_KERNEL_ISV_CERT_ERROR_6,                                  ///< 0x000E000A
    ENCRYPTED_KERNEL_ISV_CERT_ERROR_7,                                  ///< 0x000E000B
    ENCRYPTED_KERNEL_ISV_CERT_ERROR_8,                                  ///< 0x000E000C
    ENCRYPTED_KERNEL_ISV_CERT_ERROR_9,                                  ///< 0x000E000D
    ENCRYPTED_KERNEL_RSA_FAILURE,                                       ///< 0x000E000E
    ENCRYPTED_KERNEL_DMA_READ_FAILURE,                                  ///< 0x000E000F
    ENCRYPTED_KERNEL_DMA_WRITE_FAILURE,                                 ///< 0x000E0010
    ENCRYPTED_KERNEL_AES_DECRYPT_FAILURE,                               ///< 0x000E0011
    ENCRYPTED_KERNEL_AES_ENCRYPT_FAILURE,                               ///< 0x000E0012
    ENCRYPTED_KERNEL_DRNG_RAILURE,                                      ///< 0x000E0013
    ENCRYPTED_KERNEL_HW_STATUS_ERROR,                                   ///< 0x000E0014
    ENCRYPTED_KERNEL_VDM_ERROR_1,                                       ///< 0x000E0015
    ENCRYPTED_KERNEL_VDM_ERROR_2,                                       ///< 0x000E0016
    ENCRYPTED_KERNEL_VDM_ERROR_3,                                       ///< 0x000E0017
    ENCRYPTED_KERNEL_VDM_ERROR_4,                                       ///< 0x000E0018
    ENCRYPTED_KERNEL_VDM_ERROR_5,                                       ///< 0x000E0019
    ENCRYPTED_KERNEL_RSA_SIG_FAIULRE,                                   ///< 0x000E001A
    ENCRYPTED_KERNEL_NO_SETUP,                                          ///< 0x000E001B
    ENCRYPTED_KERNEL_HASH_ERROR,                                        ///< 0x000E001C
    ENCRYPTED_KERNEL_UNEXPECTED_CALL,                                   ///< 0x000E001D
    ENCRYPTED_KERNEL_ROOT_CERT_ERROR_1,                                 ///< 0x000E001E
    ENCRYPTED_KERNEL_ROOT_CERT_ERROR_2,                                 ///< 0x000E001F
    ENCRYPTED_KERNEL_ROOT_CERT_ERROR_3,                                 ///< 0x000E0020
    ENCRYPTED_KERNEL_ROOT_CERT_ERROR_4,                                 ///< 0x000E0021
    ENCRYPTED_KERNEL_ROOT_CERT_ERROR_5,                                 ///< 0x000E0022
    ENCRYPTED_KERNEL_ROOT_CERT_ERROR_6,                                 ///< 0x000E0023
    ENCRYPTED_KERNEL_ROOT_CERT_ERROR_7,                                 ///< 0x000E0024
    ENCRYPTED_KERNEL_ROOT_CERT_ERROR_8,                                 ///< 0x000E0025
    ENCRYPTED_KERNEL_ROOT_CERT_ERROR_9                                  ///< 0x000E0026
} AuthKernelHeciStatus;

// Authenticated kernel metadata info, e.g., version
// Combined size of AuthKernelMetaDataInfo and AuthKernelMetaData 
// should be multiple of 16 bytes or size of full EU instruction
typedef struct _AuthKernelMetaDataInfo
{
    uint32_t  Version;
    uint32_t  FWApiVersion;
    uint32_t  Reserved[2];
} AuthKernelMetaDataInfo;

// Authenticated kernel metadata
typedef struct _AuthKernelMetaData
{
    uint8_t    RootCert[CERT_MAX_BYTE_SIZE];
    uint8_t    IsvCert[CERT_MAX_BYTE_SIZE];
    uint8_t    EncKernelKey[RSA_KEY_BYTE_SIZE];
    uint8_t    EncKernelHash[RSA_KEY_BYTE_SIZE];
} AuthKernelMetaData;

// FIRMWARE_CMD_AUTH_KERNEL_SETUP input struct
typedef struct _AuthKernelSetupInBuf
{
    PAVPCmdHdr          Header;
    uint32_t            AppID;
    AuthKernelMetaData  MetaData;
} AuthKernelSetupInBuf;

// FIRMWARE_CMD_AUTH_KERNEL_SETUP output struct
typedef NoDataBuffer AuthKernelSetupOutBuf;

// FIRMWARE_CMD_AUTH_KERNEL_SEND input struct
typedef struct _AuthKernelSendInBuf
{
    PAVPCmdHdr   Header;
    uint8_t      EncKernel[MAX_KERNEL_SIZE_PER_HECI];
} AuthKernelSendInBuf;

// FIRMWARE_CMD_AUTH_KERNEL_SEND output struct
typedef struct _AuthKernelSendOutBuf
{
    PAVPCmdHdr   Header;
    uint8_t      EncKernel[MAX_KERNEL_SIZE_PER_HECI];
} AuthKernelSendOutBuf;

// FIRMWARE_CMD_AUTH_KERNEL_DONE input struct
typedef NoDataBuffer AuthKernelDoneInBuf;

// FIRMWARE_CMD_AUTH_KERNEL_DONE output struct
typedef struct _AuthKernelDoneOutBuf
{
    PAVPCmdHdr   Header;
    uint32_t     KernelSize;
    uint32_t     IsKernelHashMatched;
} AuthKernelDoneOutBuf;

// Get ME certification Group ID input struct
typedef NoDataBuffer GetGroupIDInBuf;

// Get ME certification Group ID output struct
typedef struct _GetGroupIDOutBuf
{
    PAVPCmdHdr   Header;
    uint32_t     epIdStatus;
    uint32_t     epIdgroupId;
} GetGroupIDOutBuf;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _EPID_PROV_STATUS {
    EPID_PROV_STATUS_SUCCESS              = 0,
    EPID_PROV_STATUS_FAILED,
    EPID_PROV_STATUS_COMMUNICATION_ERROR,
    EPID_PROV_STATUS_NOT_READY
} EPID_PROV_STATUS;

typedef enum _PAVP_FW_APP_TYPE {
    PAVP_FW_DISPLAY_SESSION              = 0,
    PAVP_FW_TRANSCODE_SESSION,
} PAVP_FW_APP_TYPE;

typedef enum _PAVP_FW_PAVP_MODE {
    PAVP_FW_MODE_LITE              = 1,
    PAVP_FW_MODE_HEAVY,
    PAVP_FW_MODE_ISOLATED_DECODE,
    PAVP_FW_MODE_STOUT // TODO(mctremel): verify constant value
} PAVP_FW_PAVP_MODE;

#ifdef __cplusplus
}
#endif

#endif // _INTEL_PAVP_HECI_API_H_.
