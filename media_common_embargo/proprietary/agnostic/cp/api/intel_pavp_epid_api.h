/*
//               INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2010-2012 Intel Corporation. All Rights Reserved.
//
*/
#ifndef _INTEL_PAVP_EPID_API_H
#define _INTEL_PAVP_EPID_API_H

#include "mos_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "intel_pavp_types.h"
#include "intel_pavp_error.h"
#include "intel_pavp_heci_api.h"

#define ECDSA_PRIVKEY_LEN              32
#define WV_AES_KEY_SIZE                16
#define ECDSA_PUBKEY_LEN               64
#define ECDSA_SIGNATURE_LEN            64
#define EPID_PARAM_LEN                 876        // EPID cryptosystem context length
#define EPID_CERT_LEN                  392        // EPID certificate length
#define EPID_PUBKEY_LEN                328        // EPID cert length - EC-DSA sig length
#define EPID_SIG_LEN                   569        // EPID signature length
#define SIGMA_PUBLIC_KEY_LEN           64
#define SIGMA_SESSION_KEY_LEN          16
#define SIGMA_MAC_KEY_LEN              16
#define SIGMA_MAC_LEN                  32
#define PAVP_EPID_STREAM_KEY_LEN       16
#define PAVP_SIGMA_STREAM_KEY_MAX_LEN  32
#define SIGMA_PRI_DATE_LEN_SIZE         4
#define SIGMA21_MAC_LEN                48
#define SIGMA21_STREAM_KEY_LEN         32
#define FW43_SIGNATURE_LEN             48
#define FW43_MK_KEY_LEN                48
#define FW43_NOUNCE_LEN                32

// PAVP EPID Commands:
#define FIRMWARE_CMD_CHECK_EPID_STATUS      0x00000000
#define FIRMWARE_CMD_SEND_EPID_PUBKEY       0x00000001
#define FIRMWARE_CMD_GET_HW_ECC_PUBKEY      0x00000003
#define FIRMWARE_CMD_EXCHG_HW_APP_CERT      0x00000004
#define FIRMWARE_CMD_CLOSE_SIGMA_SESSION    0x00000005
#define FIRMWARE_CMD_GET_STREAM_KEY         0x00000006
#define FIRMWARE_CMD_INV_STREAM_KEY         0x00000007
// PAVP_EPID_API_VERISON 2.0:
#define FIRMWARE_CMD_GET_STREAM_KEY_EX      0x0000000e
#define FIRMWARE_CMD_GET_HANDLE             0x00000010
#define FIRMWARE_CMD_CHECK_PCH_SUPPORT      0x00000019
// PAVP Sigma2.1 Commands
#define FIRMWARE_42_CMD_OPEN_SIGMA_SESSION  0x0000002B
#define FIRMWARE_42_CMD_CLOSE_SIGMA_SESSION 0x0000002E
#define FIRMWARE_42_CMD_GET_STREAM_KEY      0x0000002F
#define FIRMWARE_42_CMD_GET_KEY_BLOB_NONCE  0x00000030
#define FIRMWARE_42_CMD_GET_KEY_BLOB        0x00000031

typedef uint8_t EcDsaPrivKey[ECDSA_PRIVKEY_LEN];
typedef uint8_t EcDsaPubKey[ECDSA_PUBKEY_LEN];
typedef uint8_t EcDsaSig[ECDSA_SIGNATURE_LEN];
typedef uint8_t EpidSig[EPID_SIG_LEN];
typedef uint8_t SigmaPubKey[SIGMA_PUBLIC_KEY_LEN];
typedef uint8_t SigmaSessionKey[SIGMA21_STREAM_KEY_LEN];
typedef uint8_t SigmaMacKey[SIGMA_MAC_KEY_LEN];
typedef uint8_t HMAC[SIGMA_MAC_LEN];
typedef uint8_t Sigma21HMAC[SIGMA21_MAC_LEN];
typedef uint8_t StreamKey[PAVP_EPID_STREAM_KEY_LEN];

#pragma pack(push)
#pragma pack(1)

/*
**    3rd Party Certificate
*/
// Certificate Type Values for the 3p Signed CertificateType Field
/// Application's certificate
#define PAVP_EPID_PUBCERT3P_TYPE_APP         0x00000000
/// Server's certificate
#define PAVP_EPID_PUBCERT3P_TYPE_SERVER      0x00000001

// Certificate Type Values for the Intel Signed IntelSignedCertificateType Field (2.0 certificate only)
/// PAVP
#define PAVP_EPID_PUBCERT3P_TYPE_PAVP        0x00000000
/// Media Vault
#define PAVP_EPID_PUBCERT3P_TYPE_MV1_0       0x00000001

// Issuer id: Intel
#define PAVP_EPID_PUBCERT3P_ISSUER_ID        0x00000000

typedef struct tagSignBy3p 
{
    unsigned int    CertificateType;
    uint8_t         TimeValidStart[8];
    uint8_t         TimeValidEnd[8];
    unsigned int    Id3p;
    unsigned int    IssuerId;
    EcDsaPubKey     PubKey3p;
} _SignBy3p;

// PAVP 1.5 certificate structure
typedef struct _Cert3p 
{
    // 3rd Party signed part
    _SignBy3p           SignBy3p;
    EcDsaSig            Sign3p;
    // Intel signed part
    struct _SignByIntel 
    {
        uint8_t     TimeValidStart[8];
        uint8_t     TimeValidEnd[8];
        EcDsaPubKey PubKeyVerify3p;
    } SignByIntel;
    EcDsaSig            SignIntel;
} Cert3p;

// PAVP 2.0 certificate structure
typedef struct _Cert3p20
{
    // 3rd Party signed part
    _SignBy3p            SignBy3p;
    EcDsaSig             Sign3p;
    // Intel signed part
    struct _SignByIntel20
   {
        uint16_t       IntelSignedVersion;
        uint8_t        TimeValidStart[8];
        uint8_t        TimeValidEnd[8];
        uint16_t       IntelSignedCertificateType;
        EcDsaPubKey    PubKeyVerify3p;
    } SignByIntel20;
    EcDsaSig             SignIntel;
} Cert3p20;

/*
**    EPID Certificate
*/
/*
typedef struct _EpidCert 
{
    uint8_t PubKeyEpid[EPID_PUBKEY_LEN];
    EcDsaSig        SignIntel;
} EpidCert;
*/
typedef struct _EpidCert {
    uint8_t     sver[2];
    uint8_t     blobid[2];
    uint32_t    Gid;
    uint8_t     h1[64];
    uint8_t     h2[64];
    uint8_t     w[192];
    EcDsaSig    SignIntel;
} EpidCert;

typedef struct _EpidParams {
    uint8_t     sver[2];
    uint8_t     blobid[2];
    uint8_t     p[32];
    uint8_t     q[32];
    uint8_t     h[4];
    uint8_t     a[32];
    uint8_t     b[32];
    uint8_t     coeff0[32];
    uint8_t     coeff1[32];
    uint8_t     coeff2[32];
    uint8_t     qnr[32];
    uint8_t     orderG2[96];
    uint8_t     p_prim[32];
    uint8_t     q_prim[32];
    uint8_t     h_prim[4];
    uint8_t     a_prim[32];
    uint8_t     b_prim[32];
    uint8_t     g1[64];
    uint8_t     g2[192];
    uint8_t     g3[64];
    EcDsaSig    SignIntel;
} EpidParams;

/*
**       Command: pavp_set_xcript_key_in
**    InBuffer:
*/
typedef NoDataBuffer pavp_set_xcript_key_in;
/*
**       Command: pavp_set_xcript_key_out
**     OutBuffer:
*/
typedef NoDataBuffer pavp_set_xcript_key_out;

/*
**    Command: CMD_GET_PCH_CAPABILITIES
*/

typedef struct _PAVP_PCH_Capabilities
{
    PAVP_ENCRYPTION_TYPE    AudioMode;
    PAVP_COUNTER_TYPE       AudioCounterType;
    PAVP_ENDIANNESS_TYPE    AudioCounterEndiannessType;
    uint32_t                PAVPVersion;
    uint8_t                 Time[8];
} PAVP_PCH_Capabilities;
/*
**    InBuffer:
*/
typedef NoDataBuffer GetCapsInBuff;
/*
**    OutBuffer:
*/
typedef struct _GetCapsOutBuff
{
    PAVPCmdHdr              Header;
    PAVP_PCH_Capabilities   Caps;
} GetCapsOutBuff;

/*
**    Command: CMD_GET_CAPABILITIES
*/

typedef struct _PAVP_Operation_Flags
{
    uint32_t Prod_State : 1;
    uint32_t Debug_Unlocked : 1;
    uint32_t Fixed_kf1 : 1;
    uint32_t reserved : 29;
} PAVP_Operation_Flags;

typedef struct _PAVP_General_Version
{
    uint64_t Major : 16;
    uint64_t Minor : 16;
    uint64_t Hotfix : 16;
    uint64_t Build : 16;
} PAVP_General_Version;

typedef struct _PAVP_Port
{
    uint32_t A : 1;
    uint32_t B : 1;
    uint32_t C : 1;
    uint32_t D : 1;
    uint32_t E : 1;
    uint32_t F : 1;
    uint32_t reserved : 26;
} PAVP_Port;

typedef struct _PAVP_Capabilities
{
    PAVP_Operation_Flags OperationFlag;
    PAVP_General_Version FwVersion;
    uint32_t             SecurityVersion;
    uint32_t             PAVPMinVersion;
    uint32_t             PAVPMaxVersion;
    PAVP_General_Version MDRMMinVersion;
    PAVP_General_Version MDRMMaxVersion;
    PAVP_General_Version PlayreadyVersion;
    PAVP_General_Version HDCPVersion;
    PAVP_Port            EDPPort;
    PAVP_Port            LSPCONPort;
    uint32_t             StreamKeySize;
    uint32_t             MaxDisplaySlots;
    uint32_t             MaxTranscodeSlots;
} PAVP_Capabilities;
/*
**    InBuffer:
*/
typedef struct _GetCapsInBuff42
{
    PAVPCmdHdr Header;
} GetCapsInBuff42;
/*
**    OutBuffer:
*/
typedef struct _GetCapsOutBuff42
{
    PAVPCmdHdr        Header;
    PAVP_Capabilities Caps;
} GetCapsOutBuff42;

/*
**    Command: CMD_CHECK_PCH_SUPPORT32
**
**    InBuffer:
*/
typedef NoDataBuff CheckVersionSupportInBuff;
/*
**    OutBuffer:
*/
typedef NoDataBuff CheckVersionSupportOutBuff;

/*
**    Command: CMD_GET_HW_ECC_PUBKEY
**
**    InBuffer:
*/
typedef NoDataBuffer GetHwEccPubKeyInBuff;
/*
**    OutBuffer:
*/
typedef struct _GetHwEccPubKeyOutBuff 
{
    PAVPCmdHdr    Header;
    uint32_t      SigmaSessionId;
    SigmaPubKey   Ga;
} GetHwEccPubKeyOutBuff;


/*
**    Command: CMD_EXCHG_HW_APP_CERT
**
**    InBuffer (PAVP 1.5):
*/
typedef struct _HwAppExchgCertsInBuff 
{
    PAVPCmdHdr    Header;
    uint32_t      SigmaSessionId;
    SigmaPubKey   Gb;
    Cert3p        Certificate3p;
    HMAC          Certificate3pHmac;
    EcDsaSig      EcDsaSigGaGb;
} HwAppExchgCertsInBuff;
/*
**    InBuffer (PAVP 2.0)
*/
typedef struct _HwAppExchgCertsInBuff20 {
   PAVPCmdHdr     Header;
   uint32_t       SigmaSessionId;
   SigmaPubKey    Gb;
   Cert3p20       Certificate3p;
   HMAC           Certificate3pHmac;
   EcDsaSig       EcDsaSigGaGb;
} HwAppExchgCertsInBuff20;
/*
**    OutBuffer:
*/
typedef struct _HwAppExchgCertsOutBuff 
{
    PAVPCmdHdr  Header;
    uint32_t    SigmaSessionId;
    EpidCert    CertificatePch;
    HMAC        CertificatePchHmac;
    EpidSig     EpidSigGaGb;
} HwAppExchgCertsOutBuff;

/*
**    Command: CMD_CLOSE_SIGMA_SESSION
**
**    InBuffer:
*/
typedef struct _CloseSigmaSessionInBuff 
{
    PAVPCmdHdr  Header;
    uint32_t    SigmaSessionId;
} CloseSigmaSessionInBuff;
/*
**    OutBuffer:
*/
typedef struct _CloseSigmaSessionOutBuff
{
    PAVPCmdHdr  Header;
    uint32_t    SigmaSessionId;
} CloseSigmaSessionOutBuff;

/*
**    Command: pavp_42_sigma_21_open_session
**
**    InBuffer:
*/
typedef struct _OpenSessionInBuff
{
    PAVPCmdHdr Header;
    uint32_t   chain_type;
    uint32_t   ocsp_required;
} OpenSessionInBuff;
/*
**    OutBuffer:
*/
typedef struct _OpenSessionOutBuff
{
    PAVPCmdHdr Header;
    uint32_t   sigma_session_id;
} OpenSessionOutBuff;

/*
**    Command: pavp_42_sigma_21_close_session
**
**    InBuffer:
*/
typedef struct _CloseSigmaSessionInBuff42
{
    PAVPCmdHdr Header;
    uint32_t   sigma_session_id;
} CloseSigmaSessionInBuff42;
/*
**    OutBuffer:
*/
typedef struct _CloseSigmaSessionOutBuff42
{
    PAVPCmdHdr Header;
} CloseSigmaSessionOutBuff42;

/*
**    Command: CMD_GET_STREAM_KEY
**
**    InBuffer:
*/
typedef struct _GetStreamKeyInBuff
{
    PAVPCmdHdr      Header;
    uint32_t        SigmaSessionId;
    uint32_t        StreamId;
    uint32_t        MediaPathId;
} GetStreamKeyInBuff;
/*
**    OutBuffer:
*/
typedef struct _GetStreamKeyOutBuff
{
    PAVPCmdHdr   Header;
    uint32_t     SigmaSessionId;
    StreamKey    WrappedStreamKey;
    HMAC         WrappedStreamKeyHmac;
} GetStreamKeyOutBuff;

typedef struct _GetStreamKeyExInBuff
{
    PAVPCmdHdr  Header;
    uint32_t    SigmaSessionId;
} GetStreamKeyExInBuff;

typedef struct _GetStreamKeyExOutBuff
{
    PAVPCmdHdr  Header;
    uint32_t    SigmaSessionId;
    StreamKey   WrappedStreamDKey;
    StreamKey   WrappedStreamEKey;
    HMAC        WrappedStreamKeyHmac;
} GetStreamKeyExOutBuff;

/*
**    Command: CMD_GET_STREAM_KEY_42
**
**    InBuffer:
*/
typedef struct _GetStreamKeyInBuff42
{
    PAVPCmdHdr Header;
    uint32_t   sigma_session_id;
} GetStreamKeyInBuff42;
/*
**    OutBuffer:
*/
typedef struct _GetStreamKeyOutBuff42
{
    PAVPCmdHdr  Header;
    uint32_t    key_size;
    uint8_t     WrappedStreamDKey[SIGMA21_STREAM_KEY_LEN];
    uint8_t     WrappedStreamEKey[SIGMA21_STREAM_KEY_LEN];
    Sigma21HMAC WrappedStreamKeyHmac;
} GetStreamKeyOutBuff42;

/*
**    Command: pavp_43_sigma_subsession_create for stout transcode create sigma subsession, 
**    64 subsession supported, called after pavp_42_sigma_21_open_session
**
**    InBuffer:
*/
typedef struct _SigmaSubSessionCreateInBuff
{
    PAVPCmdHdr Header;
    uint32_t   sigma_session_id;
    uint8_t    verifier_nounce[FW43_NOUNCE_LEN];
    uint8_t    signature[FW43_SIGNATURE_LEN];
} SigmaSubSessionCreateInBuff;

/*
**    OutBuffer:
*/
typedef struct _SigmaSubSessionCreateOutBuff
{
    PAVPCmdHdr Header;
    uint32_t   sub_session_id;
    uint8_t    enc_sub_sk[SIGMA21_STREAM_KEY_LEN];
    uint8_t    enc_sub_mk[FW43_MK_KEY_LEN];
    uint8_t    prover_nounce[FW43_NOUNCE_LEN];
    uint8_t    signature[FW43_SIGNATURE_LEN];
} SigmaSubSessionCreateOutBuff;

/*
**    Command: pavp_43_sigma_subsession_destroy for stout transcode destroy sigma subsession
**
**    InBuffer:
*/
typedef struct _SigmaSubSessionDestroyInBuff
{
    PAVPCmdHdr Header;
    uint32_t   sub_session_id;
} SigmaSubSessionDestroyInBuff;

/*
**    OutBuffer:
*/
typedef struct _SigmaSubSessionDestroyOutBuff
{
    PAVPCmdHdr Header;
} SigmaSubSessionDestroyOutBuff;

/*
**    Command: pavp_43_get_secure_compute_keys for stout transcode get decode/encode/kernel keys
**
**    InBuffer:
*/
typedef struct _PAVP43GetSecureComputeKeysInputBuff
{
    PAVPCmdHdr Header;  //with PAVP stream ID
    uint8_t    Signature[FW43_SIGNATURE_LEN];
} PAVP43GetSecureComputeKeysInputBuff;

/*
**    OutBuffer:
*/
typedef struct _PAVP43GetSecureComputeKeysOutputBuff
{
    PAVPCmdHdr Header;
    uint32_t   KeySize;
    uint8_t    SkWrappedEkey[SIGMA21_STREAM_KEY_LEN];
    uint8_t    SkWrappedDkey[SIGMA21_STREAM_KEY_LEN];
    uint8_t    SkWrappedKernelKey[SIGMA21_STREAM_KEY_LEN];
    uint8_t    Signature[FW43_SIGNATURE_LEN];
} PAVP43GetSecureComputeKeysOutputBuff;

/*
**    Command: CMD_INV_STREAM_KEY
**
**    InBuffer:
*/
typedef struct _InvStreamKeyInBuff
{
    PAVPCmdHdr      Header;
    uint32_t        SigmaSessionId;
    uint32_t        StreamId;
    uint32_t        MediaPathId;
} InvStreamKeyInBuff;
/*
**    OutBuffer:
*/
typedef struct _InvStreamKeyOutBuff
{
    PAVPCmdHdr Header;
    uint32_t   SigmaSessionId;
} InvStreamKeyOutBuff;

typedef enum
{
   EPID_SUCCESS,
   EPID_INIT_IN_PROGRESS,
   EPID_NO_PUB_SAFEID_KEY,
   EPID_PROVISIONING_IN_PROGRESS
} EPID_PROVISION_STATUS;


typedef struct _EPID_STATUS{
    uint32_t ProvisionStatus:3;      // can be one of the SAFEID_PROVISION_STATUS
    uint32_t Rekey_Needed:1;
    uint32_t reserved:28;
}EPID_STATUS;

// CheckSafeIdStatusInBuff
typedef NoDataBuff CheckEpidStatusInBuff;

// CheckSafeIdStatusOutBuff
typedef struct _CheckEpidStatusOutBuff {
    PAVPCmdHdr      Header;
    EPID_STATUS     EPIDStatus;
    uint32_t        GroupId;
} CheckEpidStatusOutBuff;

/* ----- CMD_SEND_SAFEID_PUBKEY ----- */
// SendSafeIdPubKeyInBuff
typedef struct _SendEpidPubKeyInBuff {
    PAVPCmdHdr      Header;
    EpidParams      GlobalParameters;
    EpidCert        Certificate;
} SendEpidPubKeyInBuff;

// SendSafeIdPubKeyOutBuff
typedef NoDataBuff SendEpidPubKeyOutBuff;
#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif // _INTEL_PAVP_EPID_API_H
