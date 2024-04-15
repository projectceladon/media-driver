/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2014 - 2017
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
//! \file     mhw_cp.h
//! \brief    MHW interface for content protection
//! \details  Impelements the functionalities across all platforms for content protection
//!

#ifndef __MHW_CP_H__
#define __MHW_CP_H__

#include <cstdint>
#include <map>
#include <new>
#include <memory>

#include "codec_def_common.h"
#include "codec_def_encode_cp.h"

#include "intel_pavp_types.h"
#include "mhw_utilities.h"
#include "mhw_mi_itf.h"
#include "mos_os.h"
#include "mos_util_debug.h"
#include "mhw_cp_interface.h"
#include "mos_os_cp_specific.h"
#include "cp_factory.h"

class MhwMiInterface;

#define CODEC_DWORDS_PER_AES_BLOCK          4
#define KEY_DWORDS_PER_AES_BLOCK            4

#define MHW_VDBOX_MAX_CRYPTO_COPY_SIZE 0x03FFFC0
#define CRYPTO_COPY_CLEAR_TO_AES_ENCRYPT 6

#define MHW_PAVP_QUERY_STATUS_MAX_DWORDS 8
#define MHW_PAVP_QUERY_STATUS_STORE_DWORD_DATA 0x01010101
#define MHW_PAVP_STORE_DATA_DWORD_DATA 0x01010102

#define MHW_PAVP_AES128_NUM_BYTES_IN_BLOCK 16
#define CENC_NUM_DECRYPT_SHARED_BUFFERS     128

#define MHW_IV_SIZE_8                      8  //8 bytes IV and 8 bytes counter
#define MHW_IV_SIZE_16                     16 //16 bytes counter

#define PAVP_SESSION_TYPE_FROM_ID(x) (((x)&0x80) >> 7)
#define TRANSCODE_APP_ID_MASK 0x80 //All PAVP Transcode app ids have highest order bit set. Supported Transcodeapp Ids (0x80 to 0x85)

#define MHW_CP_ASSERT(_ptr)                                                  \
    MOS_ASSERT(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW, _ptr)

#define MHW_CP_CHK_STATUS(_stmt)                                               \
    MOS_CHK_STATUS_RETURN(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW, _stmt)

#define MHW_CP_CHK_STATUS_MESSAGE(_stmt, _message, ...)                        \
    MOS_CHK_STATUS_MESSAGE_RETURN(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW, _stmt, _message, ##__VA_ARGS__)

#define MHW_CP_CHK_NULL(_ptr)                                                  \
    MOS_CHK_NULL_RETURN(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW, _ptr)

#define MHW_CP_CHK_NULL_NO_STATUS_RETURN(_ptr)                                                  \
    MOS_CHK_NULL_NO_STATUS_RETURN(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW, _ptr)

#define MHW_CP_CHK_COND(_condition, retVal, _message, ...)                                                  \
    MOS_CHK_COND_RETURN_VALUE(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW, (_condition), (retVal), (_message),  ##__VA_ARGS__)

#define MHW_CP_FUNCTION_ENTER                                                  \
    MOS_FUNCTION_ENTER(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW)

#define MHW_CP_FUNCTION_EXIT(eStatus)                                               \
    MOS_FUNCTION_EXIT(MOS_COMPONENT_CP, MOS_CP_SUBCOMP_MHW, eStatus)

// PAVP AES types understood by the hardware
typedef enum MHW_PAVP_AES_TYPE_REC {
    MHW_PAVP_AES_TYPE_CTR = 0x1,  // AES-CTR
    MHW_PAVP_AES_TYPE_CBC = 0x0,  // AES-CBC
    MHW_PAVP_AES_TYPE_CBC_SKIP = 0x2 // AES-CBC crypto copy mode that supports skip/stripe encryption ("Fairplay mode" in Bspec)
} MHW_PAVP_AES_TYPE;

#define MHW_PAVP_ENCRYPTION_TYPE(x) \
    (((x) == CP_SECURITY_AES128_CBC) ? MHW_PAVP_AES_TYPE_CBC : MHW_PAVP_AES_TYPE_CTR)

#ifndef __MHW_PAVP_COUNTER_TYPE_PROTECTOR__
#define __MHW_PAVP_COUNTER_TYPE_PROTECTOR__
// PAVP counter type values understood by the hardware
typedef enum MHW_PAVP_COUNTER_TYPE_REC {
    MHW_PAVP_COUNTER_TYPE_C = 0x02   // 64-bit counter, 64-bit nonce
} MHW_PAVP_COUNTER_TYPE;
#endif

typedef enum _MHW_PAVP_INLINE_STATUS_SHIFT {
    MHW_PAVP_INLINE_STATUS_READ_DATA_SHIFT = 0,
    MHW_PAVP_INLINE_DWORD_STORE_DATA_SHIFT = 3,
} MHW_PAVP_INLINE_STATUS_SHIFT;

//!
//! \brief VP authenticated kernel type
//!
typedef enum _MHW_PAVP_KERNEL_ENCRYPTION_TYPE {
    MHW_PAVP_KERNEL_ENCRYPTION_PRODUCTION,
    MHW_PAVP_KERNEL_ENCRYPTION_PREPRODUCTION,

    MHW_PAVP_KERNEL_ENCRYPTION_TYPE_NUM
} MHW_PAVP_KERNEL_ENCRYPTION_TYPE;

class CpHwUnit;

// DECE slices always contain 128 bytes clear data (pad + start code + header)
#define MHW_PAVP_DECE_NUM_CLEAR_BYTES_IN_SLICE 128

enum MHW_CRYPTO_INLINE_STATUS_READ_TYPE
{
    MHW_CRYPTO_INLINE_MEMORY_STATUS_READ    = 1,
    MHW_CRYPTO_INLINE_GET_FRESHNESS_READ    = 2,
    MHW_CRYPTO_INLINE_CONNECTION_STATE_READ = 3,
    MHW_CRYPTO_INLINE_GET_HW_COUNTER_READ   = 4
};

enum MHW_CRYPTO_KEY_SELECT
{
    MHW_CRYPTO_DECRYPTION_KEY = 1,
    MHW_CRYPTO_ENCRYPTION_KEY = 2
};

typedef struct _MHW_PAVP_MFX_CRYPTO_KEY_EXCHANGE_CMD_PARAMS
{
    uint32_t KeyExchangeKeyUse;
    uint32_t KeyExchangeKeySelect;
    uint32_t EncryptedDecryptionKey[4];  //!< Encrypted decrypt key for set decrypt key or set both keys
    uint32_t EncryptedEncryptionKey[4];  //!< Encrypted encrypt key for set encrypt key or set both keys
} MHW_PAVP_MFX_CRYPTO_KEY_EXCHANGE_CMD_PARAMS, *PMHW_PAVP_MFX_CRYPTO_KEY_EXCHANGE_CMD_PARAMS;

enum _CP_SECURITY_TYPE: int32_t
{
    CP_SECURITY_NONE            = 0,
    CP_SECURITY_BASIC           = 1,
    CP_SECURITY_AES128_CTR      = 2,
    CP_SECURITY_AES128_CBC      = 4,
    CP_SECURITY_AES128_ECB      = 8,
    CP_SECURITY_DECE_AES128_CTR = 16,
    CP_SECURITY_SKIP_AES128_CBC = 32
};

#define CP_TYPE_CASCADE 1
#define CP_TYPE_BASIC 2
#define CENC_TYPE_CBC 3
#define CENC_TYPE_CTR_LENGTH 4
#define CENC_TYPE_CTR 5

enum DecodeCpMode
{
    /*This enum indicate which method is selected for bitstream decryption.*/
    HUC_PIPELINE_BASIC, // Use HUC AES State
    HUC_PIPELINE_EXTENDED, // Use HUC AES Indirect State
    DECODE_PIPELINE_BASIC, // Use decode pieline AES State
    DECODE_PIPELINE_EXTENDED, // Use decode pieline AES Indirect State
};

typedef struct _MHW_GSC_HECI_Params
{
    PMOS_RESOURCE presSrc;
    PMOS_RESOURCE presDst;
    uint32_t      inputBufferLength;
    uint32_t      outputBufferLength;
} MHW_GSC_HECI_Params, * PMHW_GSC_HECI_Params;

typedef struct _MHW_SEGMENT_INFO
{
    /** \brief  The offset relative to the start of the bitstream input in
     *  bytes of the start of the segment*/
    uint32_t       dwSegmentStartOffset;
    /** \brief  The length of the segments in bytes*/
    uint32_t       dwSegmentLength;
    /** \brief  The length in bytes of the remainder of an incomplete block
     *  from a previous segment*/
    uint32_t       dwPartialAesBlockSizeInBytes;
    /** \brief  The length in bytes of the initial clear data */
    uint32_t       dwInitByteLength;
    /** \brief  This will be AES 128 counter for secure decode and secure
     *  encode when numSegments equals 1 */
    uint32_t       dwAesCbcIvOrCtr[4];
} MHW_SEGMENT_INFO, *PMHW_SEGMENT_INFO;

typedef struct _MHW_SUB_SAMPLE_MAPPING_BLOCK
{
    uint32_t   ClearSize;
    uint32_t   EncryptedSize;
} MHW_SUB_SAMPLE_MAPPING_BLOCK, *PMHW_SUB_SAMPLE_MAPPING_BLOCK;

typedef enum _CODEC_ENCRYPTION_BUFFER_MODE
{
    BUFFER_IN_SYSMEM    = 0,
    BUFFER_IN_VIDEOMEM  = 1
} CODEC_ENCRYPTION_BUFFER_MODE;

typedef struct _MHW_PAVP_ENCRYPTION_PARAMS
{
    // HostEncryptMode MUST be the first element to align with cipherdata
    CP_MODE               HostEncryptMode;          // decoder has requested to activate encryption
    uint32_t              CpTag;                    // CP context in the form of CpTag
    bool                  bInsertKeyExinVcsProlog;
    MHW_PAVP_MFX_CRYPTO_KEY_EXCHANGE_CMD_PARAMS CryptoKeyExParams;
    uint32_t              dwPavpAesCounter[CODEC_DWORDS_PER_AES_BLOCK];  // Application passes in counter value for AES decryption
    CP_SECURITY_TYPE      PavpEncryptionType;                            // Encryption type in use by the app
    uint32_t              uiCounterIncrement;
    void                  *pSubSampleMappingBlock;  // Generic array of sub sample blocks for DECE
    uint32_t              dwSubSampleCount;         // Number of sub samples blocks
    uint32_t              dwIVSize;                 // Size of IV, should be 8 or 16
    uint32_t              dwBlocksStripeEncCount;   // Number of stripe encryption blocks
    uint32_t              dwBlocksStripeClearCount; // Number of stripe clear blocks
    uint32_t              dwPavpAesCbcSkipInitial;
    uint32_t              dwPavpAesCbcSkipEncrypted;
    uint32_t              dwPavpAesCbcSkipClear;
    bool                  bPavpAesCbcSkipIsFrameBased;
    uint32_t              dwPavpAesCbcSkipFrameOffset;
    uint32_t              dwPavpAesCbcSkipFirstOffset;
    uint32_t              dwPavpAesCbcSkipFrameEndOffset;
    CP_MODE               BackupHostEncryptMode;    // backup HostEncryptMode as WinDVD has clear frame which will set HostEncryptMode to CP_TYPE_NONE
    bool                  bResetHostEncryptMode;
} MHW_PAVP_ENCRYPTION_PARAMS, *PMHW_PAVP_ENCRYPTION_PARAMS;

typedef struct _MHW_PAVP_AES_PARAMS
{
    PMOS_RESOURCE presDataBuffer;
    uint32_t      dwOffset;
    uint32_t      dwDataLength;
    uint32_t      dwDataStartOffset;
    bool          bShortFormatInUse;
    uint32_t      dwSliceIndex;
    bool          bLastPass;
    uint32_t      dwTotalBytesConsumed;
    uint32_t      dwDefaultDeceInitPacketLengthInBytes;
} MHW_PAVP_AES_PARAMS, *PMHW_PAVP_AES_PARAMS;

typedef struct _MHW_PAVP_AES_IND_PARAMS
{
    PMOS_RESOURCE       presCryptoBuffer;
    uint32_t            dwOffset;
    uint32_t            dwNumSegments;
    uint32_t            dwSliceIndex;
    uint32_t            dwSliceOffset;
    uint32_t            dwNumSegmentsSubmitted;
    PMHW_SEGMENT_INFO   pSegmentInfo;
} MHW_PAVP_AES_IND_PARAMS, *PMHW_PAVP_AES_IND_PARAMS;

typedef struct _MHW_PAVP_CRYPTO_COPY_ADDR_PARAMS
{
    PMOS_RESOURCE presSrc;
    PMOS_RESOURCE presDst;
    uint32_t      dwSize;
    bool          bIsDestRenderTarget;
    uint64_t      offset;
} MHW_PAVP_CRYPTO_COPY_ADDR_PARAMS, *PMHW_PAVP_CRYPTO_COPY_ADDR_PARAMS;

typedef struct _MHW_PAVP_MFX_CRYPTO_INLINE_STATUS_READ_CMD_PARAMS
{
    uint32_t      StatusReadType;
    PMOS_RESOURCE presReadData;
    PMOS_RESOURCE presWriteData;
    uint32_t      dwReadDataOffset;
    uint32_t      dwWriteDataOffset;
    uint32_t      dwSize;
    union
    {
        uint32_t NonceIn;         //!< Nonce value that is expected to be read back in connection state
        uint32_t KeyRefreshType;  //!< Encrypt(1) or decrypt(0) key freshness
        uint32_t HWCounter;       //!< KBL+: HW key's working counter value
    } Value;
    uint32_t OutData[MHW_PAVP_QUERY_STATUS_MAX_DWORDS];  //!< Connection state, memory status, or freshness value
} MHW_PAVP_MFX_CRYPTO_INLINE_STATUS_READ_CMD_PARAMS, *PMHW_PAVP_MFX_CRYPTO_INLINE_STATUS_READ_CMD_PARAMS;

// The numbers of this enum should match the Bspec.
typedef enum _MHW_PAVP_CRYPTO_COPY_TYPE {
    CRYPTO_COPY_TYPE_BYPASS                   = 0,   //!< Input data not signed and not encrypted
    CRYPTO_COPY_TYPE_AES_DECRYPT              = 2,   //!< Input data not signed, but encrypted using AES-128 Ctr mode
    CRYPTO_COPY_TYPE_SERPENT_TO_AES_ENCRYPT   = 4,   //!< Input is Serpent mode encrypted, output to be AES-128 Ctr mode encrypted
    CRYPTO_COPY_TYPE_CLEAR_TO_AES_ENCRYPT     = 6,   //!< Input is in clear, output to be AES-128 Ctr mode encrypted. It's performed in 2 passes, one pass is not supproted according to PAVP BSpec due to 6(0110) is reserved.
                                                     //!< the 1st pass is clear-> serpent, the 2nd pass is serpent -> AES Ctr mode encrypted.
    CRYPTO_COPY_TYPE_CLEAR_TO_SERPENT_ENCRYPT = 7,   //!< Input is in clear, output to be Serpent mode encrypted
    CRYPTO_COPY_TYPE_SET_PLANE_ENABLE         = 10,  //!< Set plane enable - PCM enabled display information
    CRYPTO_COPY_TYPE_AES_DECRYPT_PARTIAL      = 13,  //!< Same as TYPE_AES_DECRYPT, but input is padded with unencrypted data at the end of each row
} MHW_PAVP_CRYPTO_COPY_TYPE;

typedef enum _MHW_PAVP_CRYPTO_CMD_MODE
{
    CRYPTO_COPY_CMD_LEGACY_MODE  = 0,    //!< Legacy cmd mode
    CRYPTO_COPY_CMD_LIST_MODE    = 1,    //!< list cmd mode for stitching
} MHW_PAVP_CRYPTO_COPY_CMD_MODE;

typedef enum _MHW_PAVP_CRYPTO_COPY_AES_MODE {
    CRYPTO_COPY_AES_MODE_CTR = 0,       //!< AES-CTR mode
    CRYPTO_COPY_AES_MODE_CBC_SKIP = 1,  //!< AES-CBC mode with skip decryption
} MHW_PAVP_CRYPTO_COPY_AES_MODE;

typedef struct _MHW_PAVP_CRYPTO_COPY_PARAMS
{
    uint32_t dwCopySize;
    uint8_t ui8SelectionForIndData;
    uint32_t dwSrcBaseOffset;
    uint32_t dwDestBaseOffset;
    uint32_t dwAesCounter[4];
    uint32_t uiEncryptedBytes;
    uint32_t uiClearBytes;
    MHW_PAVP_CRYPTO_COPY_CMD_MODE  CmdMode;
    uint16_t LengthOfTable;
    MHW_PAVP_CRYPTO_COPY_AES_MODE  AesMode;
} MHW_PAVP_CRYPTO_COPY_PARAMS, *PMHW_PAVP_CRYPTO_COPY_PARAMS;

// CP prolog trace event data structure
typedef struct _MHW_PAVP_PROLOG_TRACE_DATA
{
    MOS_GPU_CONTEXT GpuContext;
    uint32_t        PavpEnabled;
    uint32_t        AppId;
} MHW_PAVP_PROLOG_TRACE_DATA;

typedef struct _CP_ENCODE_HW_COUNTER
{
    uint64_t   IV;         // Big-Endian IV
    uint64_t   Count;      // Big-Endian Block Count
}CP_ENCODE_HW_COUNTER, *PCP_ENCODE_HW_COUNTER;

typedef struct _TRANSCRYPT_KERNEL_INFO
{
    const uint32_t *TranscryptMetaData;
    uint32_t  TranscryptMetaDataSize;
    const uint32_t *EncryptedKernels;
    uint32_t  EncryptedKernelsSize;
}TRANSCRYPT_KERNEL_INFO;

// Extra code to maximize the similarity to other MHW components
struct DecryptBitOffset
{
    // The ccmdProps decrypt bit indicates the command's Encrypted Data bit
    // location for software post-processing of a batch buffer before
    // submitting it to the GPU.
    // The Encrypted Data bit is part of the
    // MEMORY_OBJECT_CONTROL_STATE (MOCS), which the GPU uses to enable
    // decryption while reading data.
    // The BSPEC specifies the bit range of the MOCS for each instruction by
    // the zero-based dword offset and the corresponding bit offset.
    // The decrypt bit indicates that a command consumes a heavy-mode
    // (using display-key) AES-ECB encrypted resource.

    bool     hasDecryptBits;
    uint32_t bitNumber;
    int32_t  dwordOffset;

    DecryptBitOffset() : hasDecryptBits(false),
                         bitNumber(0),
                         dwordOffset(0){};

    DecryptBitOffset(const uint32_t bitNum, const int32_t dwOff) : hasDecryptBits(true),
                                                                   bitNumber(bitNum),
                                                                   dwordOffset(dwOff){};
};

///////////////////////////////////////////////////////////////////////////////
/// \class       MhwCpBase
/// \brief       The primary class to interact with clients
/// \details     The class is expected to have one implementation for each gen.
///              The class also has two types of functions: interface and internal.
///              Interface functions are expected to be the main entry points
///              from the clients.  The outside world use these functions to
///              obtain help for PAVP related topics.
///              Internal functions are not expected to be called by clients.
///              Only the members inside the MHW PAVP modlue are using them.
///////////////////////////////////////////////////////////////////////////////
class MhwCpBase: public MhwCpInterface
{
public:
    // ----- interface functions ------------------------------------------- //
    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Instantiate a MhwCpBase object for the client.
    /// \details     The MhwCpBase object is created based on PLATFORM provided.
    ///              The client can carry the instance on its HAL object and
    ///              invoke the needed functions from the MhwCpBase instance
    ///
    /// \param       osInterface [in] to obatin Platfrom info
    /// \param       eStatus   [in] to report error
    /// \return      If success, returning a pointer to the created instance,
    ///              If fail, nullptr is returned
    ///////////////////////////////////////////////////////////////////////////
    static MhwCpInterface* CpFactory(
        PMOS_INTERFACE osInterface,
        MOS_STATUS&    eStatus);

    MhwCpBase(MOS_INTERFACE& osInterface);

    //!
    /// \brief       Add necessary commands and the beginning of the command buffer.
    /// \details     Based on Gen info provided in CpFactory() and the HW unit
    ///              info provided in PMOS_INTERFACE, the function forward the
    ///              request to the right CpHwUnit object.
    ///
    /// \param       osInterface  [in] to obtain HW unit being used info
    /// \param       cmdBuffer    [in] the command buffer being built
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    //!
    virtual MOS_STATUS AddProlog(
        PMOS_INTERFACE      osInterface,
        PMOS_COMMAND_BUFFER cmdBuffer);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add necessary commands and the end of the command buffer.
    /// \details     Based on Gen info provided in CpFactory() and the HW unit
    ///              info provided in PMOS_INTERFACE, the function forward the
    ///              request to the right CpHwUnit object.
    ///
    /// \param       osInterface  [in] to obtain HW unit being used info
    /// \param       cmdBuffer    [in] the command buffer being built
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddEpilog(
        PMOS_INTERFACE      osInterface,
        PMOS_COMMAND_BUFFER cmdBuffer);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add MI_SET_APPID to command buffer
    /// \details     Add MI_SET_APPID to command buffer. VDBox need to override
    ///              default behavior due to one extra MFX_WAIT_CMD is needed
    ///
    /// \param       osInterface  [in] to obtain HW unit being used info
    /// \param       cmdBuffer    [in] the command buffer being built
    /// \param       appId         [in] the app id to be set
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddMiSetAppId(
        PMOS_INTERFACE      osInterface,
        PMOS_COMMAND_BUFFER cmdBuffer,
        uint32_t            appId);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Check support for HW Counter Auto Increment
    /// \details     From KBL+ HW Counter increment is valid for Widi and 
    ///              From ICL+ HW Counter increment is supported for both Widi and Transcode
    ///
    /// \param       pOsInterface  [in] to obtain HW unit being used info
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual bool IsHwCounterIncrement(
        PMOS_INTERFACE pOsInterface) = 0;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add check to exit if expected PAVP mode is not active
    /// \details     Add check to exit if expected PAVP mode is not active
    ///
    /// \param       osInterface  [in] to obtain HW unit being used info
    /// \param       cmdBuffer    [in] the command buffer being built
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddCheckForEarlyExit(
        PMOS_INTERFACE      osInterface,
        PMOS_COMMAND_BUFFER cmdBuffer);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add check to exit if expected PAVP session is tear down
    /// \details     Add check to exit if expected PAVP session is tear down
    ///
    /// \param       cencBufIndex    [in] index of the cenc shared buffer
    /// \param       statusReportNum [in] status report number
    /// \param       lockAddress     [in] lock address of encrypted resource
    /// \param       resource        [in] resource to compare encrypted number
    /// \param       cmdBuffer       [in] the command buffer being built
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS UpdateStatusReportNum(
        uint32_t            cencBufIndex,
        uint32_t            statusReportNum,
        PMOS_RESOURCE       resource,
        PMOS_COMMAND_BUFFER cmdBuffer);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add check to exit if expected PAVP session is tear down
    /// \details     Add check to exit if expected PAVP session is tear down
    ///
    /// \param       mfxRegisters  [in] registers to be used
    /// \param       cencBufIndex  [in] index of the cenc shared buffer
    /// \param       resource      [in] resource to compare encrypted number
    /// \param       cmdBuffer     [in] the command buffer being built
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS CheckStatusReportNum(
        void*                       mfxRegisters,
        uint32_t                    cencBufIndex,
        PMOS_RESOURCE               resource,
        PMOS_COMMAND_BUFFER         cmdBuffer);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add MFX_AES_STATE to the command buffer.
    /// \details
    /// \param       isDecodeInUse    [in] indicate if decode in use
    /// \param       cmdBuffer      [in] the command buffer to add the cmd
    /// \param       batchBuffer    [in] the batch buffer to add the cmd
    /// \param       params         [in] Params structure used to populate the HW command
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    /// \note        AddMfxAesState() is the main interface function for the client
    ///              addMfxAesState_g9() and addMfxAesState_g8() are the real
    ///              task executor
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddMfxAesState(
        bool                 isDecodeInUse,
        PMOS_COMMAND_BUFFER  cmdBuffer,
        PMHW_BATCH_BUFFER    batchBuffer,
        PMHW_PAVP_AES_PARAMS params) = 0;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add MFX_CRYPTO_COPY_BASE_ADDR to the command buffer.
    /// \details
    /// \param       cmdBuffer      [in] the command buffer to add the cmd
    /// \param       params         [in] Params structure used to populate the HW command
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    /// \note        AddMfxCryptoCopyBaseAddr() is the main interface function for the client
    ///              addMfxCryptoCopyBaseAddr_g8() is the real
    ///              task executor
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddMfxCryptoCopyBaseAddr(
        PMOS_COMMAND_BUFFER               cmdBuffer,
        PMHW_PAVP_CRYPTO_COPY_ADDR_PARAMS params) = 0;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add MFX_CRYPTO_COPY_BASE_ADDR to the command buffer.
    /// \details
    /// \param       cmdBuffer      [in] the command buffer to add the cmd
    /// \param       params         [in] Params structure used to populate the HW command
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    /// \note        AddMfxCryptoCopy() is the main interface function for the client
    ///              addMfxCryptoCopy_g8() is the real
    ///              task executor
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddMfxCryptoCopy(
        PMOS_COMMAND_BUFFER          cmdBuffer,
        PMHW_PAVP_CRYPTO_COPY_PARAMS params) = 0;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Set Crypto Copy Related Params For Scalable Encode
    /// \details
    /// \param       cmdBuffer      [in] the command buffer to add the cmd
    /// \param       params         [in] Params structure used to populate the HW command
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS SetCpCopy(
        PMOS_INTERFACE      osInterface,
        PMOS_COMMAND_BUFFER cmdBuffer,
        PMHW_CP_COPY_PARAMS params);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Set Crypto Copy Related Params For Encrypt Init
    /// \details
    /// \param       cmdBuffer      [in] the command buffer to add the cmd
    /// \param       params         [in] Params structure used to populate the HW command
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddCpCopy(
        PMOS_INTERFACE      osInterface,
        PMOS_COMMAND_BUFFER cmdBuffer,
        PMHW_ADD_CP_COPY_PARAMS params);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add MFX_CRYPTO_KEY_EXCHANGE to the command buffer.
    /// \details
    /// \param       cmdBuffer      [in] the command buffer to add the cmd
    /// \param       params         [in] Params structure used to populate the HW command
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    /// \note        AddMfxCryptoKeyExchange() is the real task executor
    ///////////////////////////////////////////////////////////////////////////

    virtual MOS_STATUS AddMfxCryptoKeyExchange(
        PMOS_COMMAND_BUFFER                          cmdBuffer,
        PMHW_PAVP_MFX_CRYPTO_KEY_EXCHANGE_CMD_PARAMS params);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add Pavp Crypto Inline Status Read to the command buffer.
    /// \details
    /// \param       osInterface    [in] pointer to OS interface
    /// \param       cmdBuffer      [in] the command buffer to add the cmd
    /// \param       params         [in] Params structure used to populate the HW command
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddMfxCryptoInlineStatusRead(
        PMOS_INTERFACE                                     osInterface,
        PMOS_COMMAND_BUFFER                                cmdBuffer,
        PMHW_PAVP_MFX_CRYPTO_INLINE_STATUS_READ_CMD_PARAMS params) = 0;

    /////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       KCR updates/refreshes counter on EncryptionOff(AddMfxCryptoKeyExchange)
    /// \details
    /// \param       osInterface    [in] pointer to OS interface
    /// \param       cmdBuffer      [in] the command buffer to add the cmd
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS error
    /////////////////////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS RefreshCounter(
        PMOS_INTERFACE osInterface,
        PMOS_COMMAND_BUFFER cmdBuffer);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Read HDCP encryption counter from HW
    /// \details
    /// \param       osInterface    [in] pointer to OS interface
    /// \param       cmdBuffer      [in] the command buffer to add the cmd
    /// \param       resource       [in] destination resource to get result
    /// \param       currentIndex   [in] current index of encode
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS ReadEncodeCounterFromHW(
        PMOS_INTERFACE      osInterface,
        PMOS_COMMAND_BUFFER cmdBuffer,
        PMOS_RESOURCE       resource,
        uint16_t            currentIndex);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add HCP_AES_STATE to the command buffer.
    /// \details
    /// \param       isDecodeInUse    [in] bool to indicate if it's decode in use
    /// \param       cmdBuffer      [in] the command buffer to add the cmd
    /// \param       batchBuffer    [in] the batch buffer to add the cmd
    /// \param       params         [in] Params structure used to populate the HW command
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    /// \note        AddHcpAesState() is the main interface function for the client
    ///              addHcpAesState_g8lp() is the real task executor
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddHcpAesState(
        bool                 isDecodeInUse,
        PMOS_COMMAND_BUFFER  cmdBuffer,
        PMHW_BATCH_BUFFER    batchBuffer,
        PMHW_PAVP_AES_PARAMS params);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add VVCP_AES_IND_STATE to the command buffer.
    /// \details
    /// \param       isDecodeInUse    [in] bool to indicate if it's decode in use
    /// \param       cmdBuffer      [in] the command buffer to add the cmd
    /// \param       batchBuffer    [in] the batch buffer to add the cmd
    /// \param       params         [in] Params structure used to populate the HW command
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    /// \note        AddVvcpAesIndState() is the main interface function for the client
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddVvcpAesIndState(
        bool                 isDecodeInUse,
        PMOS_COMMAND_BUFFER  cmdBuffer,
        PMHW_BATCH_BUFFER    batchBuffer,
        PMHW_PAVP_AES_PARAMS params);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add Huc AES State to the command buffer.
    /// \details
    /// \param       cmdBuffer      [in] the command buffer to add the cmd
    /// \param       params         [in] Params structure used to populate the HW command
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    /// \note        AddHucAesState() is the main interface function for the client
    ///              addHucAesState_g8lp() is the real task executor
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddHucAesState(
        PMOS_COMMAND_BUFFER  cmdBuffer,
        PMHW_PAVP_AES_PARAMS params);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add Huc AES Indirect State to the command buffer.
    /// \details
    /// \param       cmdBuffer      [in] the command buffer to add the cmd
    /// \param       params         [in] Params structure used to populate the HW command
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    /// \note        AddHucAesIndState() is the main interface function for the client
    ///              addHucAesIndState_g8lp() is the real task executor
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddHucAesIndState(
        PMOS_COMMAND_BUFFER  cmdBuffer,
        PMHW_PAVP_AES_PARAMS params);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Set HUC_INDIRECT_CRYPTO_STATE command
    /// \details     Client facing function to set HUC_INDIRECT_CRYPTO_STATE command
    /// \param       params         [in] Params structure used to populate the HW command
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    /// \note        SetHucIndirectCryptoState() is the main interface function for the client
    ///              SetHucIndirectCryptoState_g8lp() is the real task executor
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS SetHucIndirectCryptoState(
        PMHW_PAVP_AES_IND_PARAMS params);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Set VVCP_AES_IND_STATE command
    /// \details     Client facing function to set VVCP_AES_IND_STATE command
    /// \param       params         [in] Params structure used to populate the HW command
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    /// \note        SetVvcpIndirectCryptoState() is the main interface function for the client
    /// 
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS SetVvcpIndirectCryptoState(
        PMHW_PAVP_AES_IND_PARAMS params);

    ////////////////////////////////////////////////////////////////////////////
    /// \brief       Set the protection bits of MI_FLUSH_DW based on PAVP status
    /// \details     The function post-process a MI_FLUSH_DW instance to toggle
    ///              its protected flush bit (bit 22) and AppType bit (bit 7
    ///              applicable only for gen8- ) based on PAVP heavy mode
    ///              status and apptype(display/transcode)
    ///
    /// \param       osInterface  [in] to obtain PAVP status
    /// \param       cmd          [in] pointer to the MI_FLUSH_DW instance
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    /// \note        Here it's a pure virtual function only, to be implemented
    ///              in gen specific files
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS SetProtectionSettingsForMiFlushDw(
        PMOS_INTERFACE osInterface,
        void           *cmd);

    ////////////////////////////////////////////////////////////////////////////
    /// \brief       Set the protection bits of MFX_WAIT based on PAVP status
    /// \details     The function post-process a MFX_WAIT instance to toggle
    ///              its protected bit (PavpSyncControlFlag) based on PAVP heavy mode
    ///              status
    ///
    /// \param       osInterface  [in] to obtain PAVP status
    /// \param       cmd          [in] pointer to the MFX_WAIT instance
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS SetProtectionSettingsForMfxWait(
        PMOS_INTERFACE osInterface,
        void           *cmd);
    ////////////////////////////////////////////////////////////////////////////
    /// \brief       Set the protection bits of MFX_PIPE_MODE_SELECT based on PAVP status
    /// \details     The function post-process a MFX_PIPE_MODE_SELECT instance to set
    ///              AES_CONTROL fields based on PAVP encryption parameter
    ///
    /// \param       data          [in] pointer to the AES_CONTROL instance
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS SetProtectionSettingsForMfxPipeModeSelect(uint32_t *data);
    ////////////////////////////////////////////////////////////////////////////
    /// \brief       Set the protection bits of HCP_PIPE_MODE_SELECT based on PAVP status
    /// \details     The function post-process a HCP_PIPE_MODE_SELECT instance to set
    ///              AES_CONTRO fields based on PAVP encryption parameter
    ///
    /// \param       data           [in] pointer to the AES_CONTROL instance
    /// \param       scalableEncode [in] true if two pass Scalable encode workload
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS SetProtectionSettingsForHcpPipeModeSelect(uint32_t *data, bool twoPassScalableEncode = false);

    ////////////////////////////////////////////////////////////////////////////
    /// \brief       Set the protection bits of HUC_PIPE_MODE_SELECT based on PAVP status
    /// \details     The function is to set AES_CONTROL fields based on PAVP status
    ///
    /// \param       data          [in] pointer to the AES_CONTROL instance
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS SetProtectionSettingsForHucPipeModeSelect(uint32_t *data);
    ////////////////////////////////////////////////////////////////////////////
    /// \brief       Set the protection setting for avc mfx command based on PAVP status
    /// \details     The function sets aes paramater based on PAVP status then sets
    ///              mfx aes state accordingly
    ///
    /// \param       isDecodeInUse       [in] indicate if decode in use
    /// \param       cmdBuffer         [in] the command buffer to add the cmd
    /// \param       presDataBuffer     [in] pointer to PMOS_RESOURCE
    /// \param       sliceInfoParam    [in] pointer to MHW_CP_SLICE_INFO_PARAMS
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS SetMfxProtectionState(
        bool                        isDecodeInUse,
        PMOS_COMMAND_BUFFER         cmdBuffer,
        PMHW_BATCH_BUFFER           batchBuffer,
        PMHW_CP_SLICE_INFO_PARAMS   sliceInfoParam);

    ////////////////////////////////////////////////////////////////////////////
    /// \brief       Set the protection setting for avc mfx command based on PAVP status
    /// \details     The function sets aes paramater based on PAVP status then sets
    ///              mfx aes state accordingly
    ///
    /// \param       isDecodeInUse      [in] indicate if decode in use
    /// \param       cmdBuffer          [in] the command buffer to add the cmd
    /// \param       batchBuffer        [in] pointer to PMHW_BATCH_BUFFER
    /// \param       sliceInfoParam     [in] pointer to MHW_CP_SLICE_INFO_PARAMS
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS SetHcpProtectionState(
        bool                        isDecodeInUse,
        PMOS_COMMAND_BUFFER         cmdBuffer,
        PMHW_BATCH_BUFFER           batchBuffer,
        PMHW_CP_SLICE_INFO_PARAMS   sliceInfoParam);

    ///////////////////////////////////////////////////////////////////////////////
    /// \brief       Check if driver need to split hucStreamOut workload
    /// \details     Driver need to split hucStreamOut workload if segment number
    ///              too large, only DG2- have such limitation
    ///
    /// \return      return true if needed
    ///////////////////////////////////////////////////////////////////////////////
    virtual bool IsSplitHucStreamOutNeeded();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Register the MHW_MI_INTERFACE instance
    /// \details     Save the pointer to the instance for reusing send MI_FLUSH_DW
    ///              function in MHW_MI_INTERFACE.  this function is expected to be
    ///              used only by Mhw_CommonMi_InitInterface(), because by the time
    ///              MHW_MI_INTERFACE is being init, there is already an instance of
    ///              MHW_PAVP.  We want to reuse the same instance and MHW_COMMON_MI
    ///              init function can help register MHW_MI_INTERFACE to the MHW_PAVP instance.
    ///              if a client doesn't need MHW_MI_INTERFACE, then this function is
    ///              not needed either.
    ///
    /// \param       miInterface [in] pointer to the MHW_MI_INTERFACE
    ///                                    just created in Mhw_RenderEngine_Create()
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS RegisterMiInterface(
        MhwMiInterface* miInterface);

    
    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Register the MHW_MI_INTERFACE instance
    /// \details     Save the pointer to the instance for reusing send MI_FLUSH_DW
    ///              function in MHW_MI_INTERFACE.  this function is expected to be
    ///              used only by Mhw_CommonMi_InitInterface(), because by the time
    ///              MHW_MI_INTERFACE is being init, there is already an instance of
    ///              MHW_PAVP.  We want to reuse the same instance and MHW_COMMON_MI
    ///              init function can help register MHW_MI_INTERFACE to the MHW_PAVP instance.
    ///              if a client doesn't need MHW_MI_INTERFACE, then this function is
    ///              not needed either.
    ///
    /// \param       miInterface [in] pointer to the MHW_MI_INTERFACE
    ///                                    just created in Mhw_RenderEngine_Create()
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS RegisterMiInterfaceNext(
        std::shared_ptr<mhw::mi::Itf> m_miItf);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Increments the given counter by the given incrementation value.
    /// \details
    /// \param       counter      [in/out]
    /// \param       increment    [in]
    /// \param       size         [in] size of IV
    ///
    /// \return      void
    ///////////////////////////////////////////////////////////////////////////
    virtual void IncrementCounter(
        uint32_t          (&counter)[CODEC_DWORDS_PER_AES_BLOCK],
        uint32_t          increment,
        uint32_t          size = 8);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       calculates the AES CBC Skip parameters for the slice.
    /// \details
    /// \param       pParams     [in]
    /// \param       pbInitialTypeEncrypted  [out]
    /// \param       pdwInitialSize    [out]
    /// \param       pdwAesCbcIv    [out]
    /// \param       pbLastBlock    [out]
    /// \param       frameBase    [in]
    ///
    /// \return      void
    ///////////////////////////////////////////////////////////////////////////
    void CalcAesCbcSkipState(
                                PMHW_PAVP_AES_PARAMS pParams,
                                bool *pbInitialTypeEncrypted,
                                uint32_t *pdwInitialSize,
                                uint32_t *pdwAesCbcIv,
                                bool *pbLastBlock,
                                uint8_t *frameBase);

    ////////////////////////////////////////////////////////////////////////////
    /// \brief       get the state level command size and patch list size for CP
    /// \details     The function calculate the cp relate comamnd size in state
    ///              level
    ///
    /// \param       cmdSize        [in] command size
    /// \param       patchListSize  [in] patch list size
    ///////////////////////////////////////////////////////////////////////////
    virtual void GetCpStateLevelCmdSize(uint32_t& cmdSize, uint32_t& patchListSize) = 0;

    ////////////////////////////////////////////////////////////////////////////
    /// \brief       get the slice level command size and patch list size for CP
    /// \details     The function calculate the cp relate comamnd size in slcie
    ///              level
    ///
    /// \param       cmdSize        [in] command size
    /// \param       patchListSize  [in] patch list size
    ///////////////////////////////////////////////////////////////////////////
    virtual void GetCpSliceLevelCmdSize(uint32_t& cmdSize, uint32_t& patchListSize) = 0;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       get the value of m_sizeOfCmdIndirectCryptoState
    /// \details     the value is init to 0 in MhwCpBase's CTOR and each
    ///              gen assign an proper value to it in its CTOR
    ///
    /// \return      the value of m_sizeOfCmdIndirectCryptoState
    ///////////////////////////////////////////////////////////////////////////
    virtual uint32_t GetSizeOfCmdIndirectCryptoState() const;

    ///////////////////////////////////////////////////////////////////////////////
    /// \brief       Register encryption params struct from user.
    /// \details     MHW_CP need to keep a address of encryption params from user
    ///              so that could update encryption params each frame.
    ///////////////////////////////////////////////////////////////////////////////
    void RegisterParams(void* params);

    ///////////////////////////////////////////////////////////////////////////////
    /// \brief       Update internal encryption params struct.
    /// \details     Update internal encryption params struct from registered
    ///              params pointer from user.
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////////
    MOS_STATUS UpdateParams(bool isInput);

    ///////////////////////////////////////////////////////////////////////////////
    /// \brief       get the address of internal encryption params
    /// \details     this function is called by secure Decode and CNEC Decode for
    ///              directly setting.
    ///
    /// \return      internal encryption params.
    ///////////////////////////////////////////////////////////////////////////////
    virtual PMHW_PAVP_ENCRYPTION_PARAMS GetEncryptionParams();

    virtual void                  SetHostEncryptMode(CP_MODE mode);
    virtual CP_MODE               GetHostEncryptMode() const;

    void                  SetCpSecurityType(CP_SECURITY_TYPE type = CP_SECURITY_AES128_CBC);
    virtual CP_SECURITY_TYPE      GetCpEncryptionType() const;

    virtual void   SetCpAesCounter(uint32_t *pdwCounter);
    virtual uint32_t *GetCpAesCounter();

    //!
    //! \brief    Provide encrypted kernels and associated metadata to
    //!           enable transcryption of the kernels
    //! \details  Provide render-specific information on encrypted kernels
    //!           that are used in premium content playback such as UHD-BD
    //!           playback and needed to be transcrypted or re-encrypted for
    //!           use in render core
    //! \param    const uint32_t ** ppTranscryptMetaData
    //!           [out] Pointer to the metadata to transcrypt kernels
    //! \param    uint32_t * pTranscryptMetaDataSize
    //!           [out] Size in bytes for the metadata to transcrypt kernels
    //! \param    const uint32_t ** ppEncryptedKernels
    //!           [out] Pointer to the encrypted kernels
    //! \param    uint32_t * pEncryptedKernelsSize
    //!           [out] Size in bytes for the encrypted kernels
    //! \param    MHW_PAVP_KERNEL_ENCRYPTION_TYPE eEncryptionType
    //!           [in] production/preproduction kernel encryption type
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    virtual MOS_STATUS GetTranscryptKernelInfo(
        const uint32_t                  **transcryptMetaData,
        uint32_t                        *transcryptMetaDataSize,
        const uint32_t                  **encryptedKernels,
        uint32_t                        *encryptedKernelsSize,
        MHW_PAVP_KERNEL_ENCRYPTION_TYPE encryptionType);

    // ----- internal functions -------------------------------------------- //
    MhwCpBase(PMOS_INTERFACE osInterface);
    MhwCpBase(const MhwCpBase&) = delete;
    MhwCpBase& operator=(const MhwCpBase&) = delete;
    virtual ~MhwCpBase();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       find the pointer to the CpHwUnit object to handle the request
    /// \details     based on the OS interface provided to find its corresponding
    ///              CpHwUnit object to handle the request from m_hwUnit
    ///
    /// \param       osInterface  [in] to obtain HW unit info
    ///
    /// \return      If success, return the pointer to the object
    ///              If fail, return nullptr
    ///////////////////////////////////////////////////////////////////////////
    CpHwUnit* getHwUnit(PMOS_INTERFACE osInterface);

    // ---- data members --------------------------------------------------- //
    CpHwUnit**                 m_hwUnit;
    MhwMiInterface*            m_mhwMiInterface;
    std::shared_ptr<mhw::mi::Itf> m_miItf = nullptr;
    uint32_t                   m_sizeOfCmdIndirectCryptoState;

    PMHW_PAVP_ENCRYPTION_PARAMS m_userEncryptionParams;
    MHW_PAVP_ENCRYPTION_PARAMS  m_encryptionParams;

    ///////////////////////////////////////////////////////////////////////////
    /// \details     this enumerates all engines and indexes the above array.
    ///              If a future gen does not have an engine there is a hole
    ///              in the array.
    ///////////////////////////////////////////////////////////////////////////
    typedef enum _PAVP_HW_UNIT_INDEX {
        PAVP_HW_UNIT_RCS       = 0,
        PAVP_HW_UNIT_VCS       = 1,
        PAVP_HW_UNIT_VECS      = 2,
        PAVP_HW_UNIT_GCS       = 3,
        PAVP_HW_UNIT_MAX_COUNT = 4
    } PAVP_HW_UNIT_INDEX;

#define PATCH_LIST_COMMAND(x)  (x##_NUMBER_OF_ADDRESSES)

    enum CommandsNumberOfAddresses
    {
        MI_FLUSH_DW_CMD_NUMBER_OF_ADDRESSES = 1,  //  1 address field
        MI_SET_APP_ID_CMD_NUMBER_OF_ADDRESSES = 0,  //  0 address fields
        HCP_AES_STATE_CMD_NUMBER_OF_ADDRESSES = 0,  //  0 address field
        AVP_AES_STATE_CMD_NUMBER_OF_ADDRESSES = 0,  //  0 address field
        VVCP_AES_STATE_CMD_NUMBER_OF_ADDRESSES = 0,  //  0 address field
        MFX_WAIT_CMD_NUMBER_OF_ADDRESSES = 0,  //  0 address fields
        MFX_CRYPTO_COPY_BASE_ADDR_CMD_NUMBER_OF_ADDRESSES = 2,  //  2 address fields
        MFX_CRYPTO_COPY_CMD_NUMBER_OF_ADDRESSES = 2,  //  2 address fields
        HUC_AES_IND_STATE_CMD_NUMBER_OF_ADDRESSES = 1,  //  2 address fields
        MFX_AES_STATE_CMD_NUMBER_OF_ADDRESSES = 1,  //  2 address field
        HCP_AES_IND_STATE_CMD_NUMBER_OF_ADDRESSES = 1,  //  2 address field
        AVP_AES_IND_STATE_CMD_NUMBER_OF_ADDRESSES = 1,  //  2 address field
        VVCP_AES_IND_STATE_CMD_NUMBER_OF_ADDRESSES = 1,  //  1 address field
        HUC_AES_STATE_CMD_NUMBER_OF_ADDRESSES = 0,  //  0 address fields
        MFX_CRYPTO_KEY_EXCHANGE_CMD_NUMBER_OF_ADDRESSES = 0   //  0 address fields
    };

///////////////////////////////////////////////////////////////////////////
/// \brief       Init driver ID according to a hw command indicated
///
/// \param       platform [in] the current platform
/// \param       hwCommand [in] the cmd that the cpCmdProps assocites with
/// \param       cpCmdProps [out] pointer to the cpCmdProps instance
/// \param       forceDwordOffset [in] override of uint32_t offset to Encrypted Data parameter
///
/// \return      If success, return MOS_STATUS_SUCCESS
///              If fail, return other MOS errors
///////////////////////////////////////////////////////////////////////////
    static MOS_STATUS InitCpCmdProps(
        PLATFORM                   platform,
        const MOS_HW_COMMAND       hwCommandType,
        MOS_CP_COMMAND_PROPERTIES *cpCmdProps,
        const uint32_t             forceDwordOffset);

protected:
    MhwCpBase() = delete;

    MOS_INTERFACE& m_osInterface;

    static const std::map<MOS_HW_COMMAND, DecryptBitOffset>* m_cpCmdPropsMap;

    MEDIA_CLASS_DEFINE_END(MhwCpBase)
};

///////////////////////////////////////////////////////////////////////////////
/// \class       CpHwUnit
/// \brief       The basic interface for MhwCpBase
/// \details     This class hierarchy is used to implement HW dependent behavior.
///////////////////////////////////////////////////////////////////////////////
class CpHwUnit
{
public:
    ///////////////////////////////////////////////////////////////////////////
    //! \brief    Adds a resource to the command buffer or indirect state (SSH)
    //! \details  Internal MHW function to add either a graphics address of a resource or
    //!           add the resource to the patch list for the requested buffer or state
    //! \param    [in] osInterface
    //!           OS interface
    //! \param    [in] cmdBuffer
    //!           If adding a resource to the command buffer, the buffer to which the resource
    //!           is added
    //! \param    [in] params
    //!           Parameters necessary to add the graphics address
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if success, else fail reason
    ///////////////////////////////////////////////////////////////////////////
    MOS_STATUS (*pfnAddResourceToCmd)
    (
        PMOS_INTERFACE       osInterface,
        PMOS_COMMAND_BUFFER  cmdBuffer,
        PMHW_RESOURCE_PARAMS params);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add necessary commands and the beginning of the command buffer.
    /// \details     Based on Gen info provided in CpFactory() and the HW unit
    ///              info provided in PMOS_INTERFACE, the function add necessary
    ///              HW commands as Prolog to turn on protection
    ///
    /// \param       osInterface  [in] to obtain HW unit being used info
    /// \param       cmdBuffer    [in] the command buffer being built
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddProlog(
        PMOS_INTERFACE      osInterface,
        PMOS_COMMAND_BUFFER cmdBuffer) = 0;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Save Encryption parameters passed by codec DDI
    ///
    /// \param       pEncParams  [in] Encryption params
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS SaveEncryptionParams(PMHW_PAVP_ENCRYPTION_PARAMS pEncParams);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add necessary commands and the beginning of the command buffer.
    /// \details     Based on Gen info provided in CpFactory() and the HW unit
    ///              info provided in PMOS_INTERFACE, the function add necessary
    ///              HW commands as Epilog to turn off protection
    ///
    /// \param       osInterface  [in] to obtain HW unit being used info
    /// \param       cmdBuffer    [in] the command buffer being built
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddEpilog(
        PMOS_INTERFACE      osInterface,
        PMOS_COMMAND_BUFFER cmdBuffer) = 0;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add check to exit if expected PAVP mode is not active
    /// \details     Add check to exit if expected PAVP mode is not active
    ///
    /// \param       osInterface  [in] to obtain HW unit being used info
    /// \param       cmdBuffer    [in] the command buffer being built
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddCheckForEarlyExit(
        PMOS_INTERFACE      osInterface,
        PMOS_COMMAND_BUFFER cmdBuffer);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add MI_SET_APPID to command buffer
    /// \details     Add MI_SET_APPID to command buffer
    ///
    /// \param       osInterface  [in] to obtain HW unit being used info
    /// \param       cmdBuffer    [in] the command buffer being built
    /// \param       appId         [in] the app id to be set
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddMiSetAppId(
        PMOS_INTERFACE      osInterface,
        PMOS_COMMAND_BUFFER cmdBuffer,
        uint32_t            appId);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Register the MHW_MI_INTERFACE instance
    /// \details     Save the pointer to the instance for reusing send MI_FLUSH_DW
    ///              function in MHW_MI_INTERFACE
    ///
    /// \param       pRenderInterface [in] pointer to the MHW_MI_INTERFACE
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    MOS_STATUS RegisterMiInterface(
        MhwMiInterface* miInterface);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Register the MHW_MI_INTERFACE instance
    /// \details     Save the pointer to the instance for reusing send MI_FLUSH_DW
    ///              function in MHW_MI_INTERFACE
    ///
    /// \param       pRenderInterface [in] pointer to the MHW_MI_INTERFACE
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    MOS_STATUS RegisterMiInterfaceNext(
        std::shared_ptr<mhw::mi::Itf> m_miItfIn);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add Pavp Crypto Inline Status Read to the command buffer.
    /// \details
    /// \param       osInterface    [in] pointer to OS interface
    /// \param       cmdBuffer      [in] the command buffer to add the cmd
    /// \param       params         [in] Params structure used to populate the HW command
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddMfxCryptoInlineStatusRead(
        PMOS_INTERFACE                                     osInterface,
        PMOS_COMMAND_BUFFER                                cmdBuffer,
        PMHW_PAVP_MFX_CRYPTO_INLINE_STATUS_READ_CMD_PARAMS params);

    CpHwUnit(MOS_INTERFACE& osInterface);
    virtual ~CpHwUnit(){};

protected:
    // ---- data members --------------------------------------------------- //
    MhwMiInterface*                             m_mhwMiInterface;
    std::shared_ptr<mhw::mi::Itf>               m_miItf = nullptr;
    MOS_INTERFACE&                              m_osInterface;
    MHW_PAVP_MFX_CRYPTO_KEY_EXCHANGE_CMD_PARAMS m_CryptoKeyExParams;
    bool                                        m_bValidKeySet;

    MEDIA_CLASS_DEFINE_END(CpHwUnit)
};

///////////////////////////////////////////////////////////////////////////////
/// \class       CpHwUnitRcs
/// \brief       The basic render engine implement for CpHwUnit
/// \details     This class hierarchy is used to implement render engine behavior.
///              Whenever a HW unit is added and removed, it's expected to
///              add/remove the corresponding child class.
///              The current design is to have specific classes for a certain gen
///              i.e. the child classes should be named as CPavpGx_Y
///                   where x is the gen number, and Y is the HW's name
///////////////////////////////////////////////////////////////////////////////
class CpHwUnitRcs : public CpHwUnit
{
public:
    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add necessary commands and the beginning of the command buffer.
    /// \details     Based on Gen info provided in CpFactory() and the HW unit
    ///              info provided in PMOS_INTERFACE, the function add necessary
    ///              HW commands as Prolog to turn on protection
    ///
    /// \param       osInterface  [in] to obtain HW unit being used info
    /// \param       cmdBuffer    [in] the command buffer being built
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddProlog(
        PMOS_INTERFACE      osInterface,
        PMOS_COMMAND_BUFFER cmdBuffer) = 0;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add necessary commands and the beginning of the command buffer.
    /// \details     Based on Gen info provided in CpFactory() and the HW unit
    ///              info provided in PMOS_INTERFACE, the function add necessary
    ///              HW commands as Epilog to turn off protection
    ///
    /// \param       osInterface  [in] to obtain HW unit being used info
    /// \param       cmdBuffer    [in] the command buffer being built
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddEpilog(
        PMOS_INTERFACE      osInterface,
        PMOS_COMMAND_BUFFER cmdBuffer);

    CpHwUnitRcs(MOS_STATUS& mos_status, MOS_INTERFACE& osInterface);
    virtual ~CpHwUnitRcs(){};

    MEDIA_CLASS_DEFINE_END(CpHwUnitRcs)
};

///////////////////////////////////////////////////////////////////////////////
/// \class       CpHwUnitVecs
/// \brief       The basic VEbox implement for CpHwUnit
/// \details     This class hierarchy is used to implement VEbox behavior.
///              Whenever a HW unit is added and removed, it's expected to
///              add/remove the corresponding child class.
///              The current design is to have specific classes for a certain gen
///              i.e. the child classes should be named as CPavpGx_Y
///                   where x is the gen number, and Y is the HW's name
///////////////////////////////////////////////////////////////////////////////
class CpHwUnitVecs : public CpHwUnit
{
public:
    // ----- VEBox ----------------------------------------------------------//
    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add necessary commands and the beginning of the command buffer.
    /// \details     Based on Gen info provided in CpFactory() and the HW unit
    ///              info provided in PMOS_INTERFACE, the function add necessary
    ///              HW commands as Prolog to turn on protection
    ///
    /// \param       osInterface  [in] to obtain HW unit being used info
    /// \param       cmdBuffer    [in] the command buffer being built
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddProlog(
        PMOS_INTERFACE      osInterface,
        PMOS_COMMAND_BUFFER cmdBuffer);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add necessary commands and the beginning of the command buffer.
    /// \details     Based on Gen info provided in CpFactory() and the HW unit
    ///              info provided in PMOS_INTERFACE, the function add necessary
    ///              HW commands as Epilog to turn off protection
    ///
    /// \param       osInterface  [in] to obtain HW unit being used info
    /// \param       cmdBuffer    [in] the command buffer being built
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddEpilog(
        PMOS_INTERFACE      osInterface,
        PMOS_COMMAND_BUFFER cmdBuffer);

    CpHwUnitVecs(MOS_STATUS& mos_status, MOS_INTERFACE& osInterface);
    virtual ~CpHwUnitVecs(){};

    MEDIA_CLASS_DEFINE_END(CpHwUnitVecs)
};

///////////////////////////////////////////////////////////////////////////////
/// \class       CpHwUnitVcs
/// \brief       The basic VDbox implement for CpHwUnit
/// \details     This class hierarchy is used to implement VDbox behavior.
///              Whenever a HW unit is added and removed, it's expected to
///              add/remove the corresponding child class.
///              The current design is to have specific classes for a certain gen
///              i.e. the child classes should be named as CPavpGx_Y
///                   where x is the gen number, and Y is the HW's name
///////////////////////////////////////////////////////////////////////////////
class CpHwUnitVcs : public CpHwUnit
{
public:
    // ----- VDBox ----------------------------------------------------------//
    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add necessary commands and the beginning of the command buffer.
    /// \details     Based on Gen info provided in CpFactory() and the HW unit
    ///              info provided in PMOS_INTERFACE, the function add necessary
    ///              HW commands as Prolog to turn on protection
    ///
    /// \param       osInterface  [in] to obtain HW unit being used info
    /// \param       cmdBuffer    [in] the command buffer being built
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddProlog(
        PMOS_INTERFACE      osInterface,
        PMOS_COMMAND_BUFFER cmdBuffer);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add necessary commands and the beginning of the command buffer.
    /// \details     Based on Gen info provided in CpFactory() and the HW unit
    ///              info provided in PMOS_INTERFACE, the function add necessary
    ///              HW commands as Epilog to turn off protection
    ///
    /// \param       osInterface  [in] to obtain HW unit being used info
    /// \param       cmdBuffer    [in] the command buffer being built
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddEpilog(
        PMOS_INTERFACE      osInterface,
        PMOS_COMMAND_BUFFER cmdBuffer);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add Pavp Crypto Key Exchange to the command buffer.
    /// \details
    /// \param       cmdBuffer      [in] the command buffer to add the cmd
    /// \param       params         [in] Params structure used to populate the HW command
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddMfxCryptoKeyExchange(
        PMOS_COMMAND_BUFFER                          cmdBuffer,
        PMHW_PAVP_MFX_CRYPTO_KEY_EXCHANGE_CMD_PARAMS params);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Saves the Encryption Parameters passed from Codec DDI.
    /// \details
    /// \param       pEncParams [in] Encryption Params structure used to
    ///              populate the Crypto Key Exchange command
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    MOS_STATUS SaveEncryptionParams(PMHW_PAVP_ENCRYPTION_PARAMS pEncParams);

    CpHwUnitVcs(MOS_STATUS& mos_status, MOS_INTERFACE& osInterface);
    virtual ~CpHwUnitVcs(){};

    MHW_PAVP_ENCRYPTION_PARAMS m_VcsEncryptionParams;
    uint8_t                    m_cacheEncryptDecryptKey[MHW_PAVP_AES128_NUM_BYTES_IN_BLOCK];

    MEDIA_CLASS_DEFINE_END(CpHwUnitVcs)
};


///////////////////////////////////////////////////////////////////////////////
/// \class       CpHwUnitGcs
/// \brief       The basic GSC implement for CpHwUnit
/// \details     This class hierarchy is used to implement GSC behavior.
///              Whenever a HW unit is added and removed, it's expected to
///              add/remove the corresponding child class.
///              The current design is to have specific classes for a certain gen
///              i.e. the child classes should be named as CPavpGx_Y
///                   where x is the gen number, and Y is the HW's name
///////////////////////////////////////////////////////////////////////////////
class CpHwUnitGcs : public CpHwUnit
{
public:
    // ----- GSC ----------------------------------------------------------//
    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add necessary commands and the beginning of the command buffer.
    /// \details     Based on Gen info provided in CpFactory() and the HW unit
    ///              info provided in PMOS_INTERFACE, the function add 
    ///              GSC header and HECI header in command buffer
    ///
    /// \param       osInterface  [in] to obtain HW unit being used info
    /// \param       cmdBuffer    [in] the command buffer being built
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddProlog(
        PMOS_INTERFACE      osInterface,
        PMOS_COMMAND_BUFFER cmdBuffer);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add necessary commands and the beginning of the command buffer.
    /// \details     Based on Gen info provided in CpFactory() and the HW unit
    ///              info provided in PMOS_INTERFACE, the function add necessary
    ///              HW commands as Epilog to turn off protection
    ///              Currently there is no need to add epilog for GSC
    ///
    /// \param       osInterface  [in] to obtain HW unit being used info
    /// \param       cmdBuffer    [in] the command buffer being built
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddEpilog(
        PMOS_INTERFACE      osInterface,
        PMOS_COMMAND_BUFFER cmdBuffer);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add GSC header at the beginijng of GSC commnad buffer
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddGscHeader();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add HECI header in the GSC commnad buffer
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddHeciHeader();

    CpHwUnitGcs(MOS_STATUS& mos_status, MOS_INTERFACE& osInterface);
    virtual ~CpHwUnitGcs() {};

    MEDIA_CLASS_DEFINE_END(CpHwUnitGcs)
};

typedef CpFactory<MhwCpInterface, MOS_STATUS&, MOS_INTERFACE&> MhwCpFactory;

MhwCpInterface* Create_MhwCp(PMOS_INTERFACE   osInterface);
void Delete_MhwCp(MhwCpInterface* pMhwCpInterface);
#endif
