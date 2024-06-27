/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2012-2022
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

File Name: cp_drm_defs.h

Abstract:
    This defines various types for internal PAVP DRM specific structures. These
    could be used between the various PAVP classes. 
    This file is not to be shared with ISVs. 

Environment:
    OS agnostic

Notes:
    None

======================= end_copyright_notice ==================================*/
#ifndef __CP_DRM_DEFS_H__
#define __CP_DRM_DEFS_H__

#include <array>
#include <cstdint>
#include <vector>
#include <type_traits>
#include <map>

#include "pavp_types.h"
#include "cenc_def_decode_avc.h"
#include "mhw_cp.h"

#define AES_BLOCK_SIZE 16
#define CRYPTOCOPY_BLOCK_SIZE 64

// DRM Return Values
#define DRM_E_HDCP_OFF                                      (0x8004DD1EL)
#define DRM_E_TEE_OUTPUT_PROTECTION_REQUIREMENTS_NOT_MET    (0x8004CD22L)
#define DRM_E_H264_PARSING_FAILED                           (0x8004DA00L)
#define DRM_E_TEE_INVALID_CONTEXT                           (0x8004CD12L)
#define DRM_E_NOTIMPL                                       (0x80004001L)
#define DRM_E_CRYPTO_FAILED                                 (0x8004C054L)
#define DRM_E_POINTER                                       (0x80004003L)
#define DRM_E_BUFFERTOOSMALL                                (0x8007007AL)
#define DRM_E_TEE_INVALID_KEY_DATA                          (0x8004CD10L)
#define DRM_E_TEE_PROXY_INVALID_SERIALIZATION_TYPE          (0x8004CD1BL)
#define DRM_E_TEE_MESSAGE_TOO_LARGE                         (0x8004CD1EL)
#define DRM_E_TEE_PROXY_INVALID_BUFFER_ALIGNMENT            (0x8004CD20L)
#define DRM_E_TEE_PROXY_INVALID_ALIGNMENT                   (0x8004CD21L)
#define DRM_E_H264_NALU_NO_START_CODE                       (0x8004DA40L)
#define DRM_E_H264_NALU_ALL_ZERO                            (0x8004DA41L)
#define DRM_E_H264_NALU_EMULATION                           (0x8004DA42L)
#define DRM_E_H264_SPS_PROFILE                              (0x8004DA01L)
#define DRM_E_H264_SPS_IDC                                  (0x8004DA02L)
#define DRM_E_H264_SPS_SPSID                                (0x8004DA51L)
#define DRM_E_H264_SPS_CHROMA_IDC                           (0x8004DA1AL)
#define DRM_E_H264_SPS_BITDEPTHLUMA                         (0x8004DA1BL)
#define DRM_E_H264_SPS_BITDEPTHCHROMA                       (0x8004DA1CL)
#define DRM_E_H264_SPS_DELTASCALE1                          (0x8004DA1DL)
#define DRM_E_H264_SPS_DELTASCALE2                          (0x8004DA1EL)
#define DRM_E_H264_SPS_FRAMENUM                             (0x8004DA04L)
#define DRM_E_H264_SPS_POCTYTPE                             (0x8004DA05L)
#define DRM_E_H264_SPS_POCLSB                               (0x8004DA06L)
#define DRM_E_H264_SPS_POCCYCLE                             (0x8004DA07L)
#define DRM_E_H264_SPS_NUMREFFRAMES                         (0x8004DA08L)
#define DRM_E_LOGICERR                                      (0x8004C3E8L)
#define DRM_E_H264_SPS_CHROMATOP                            (0x8004DA09L)
#define DRM_E_H264_SPS_CHROMABOTTOM                         (0x8004DA0AL)
#define DRM_E_H264_SPS_NALHRD                               (0x8004DA0BL)
#define DRM_E_H264_SPS_VLDHRD                               (0x8004DA0CL)
#define DRM_E_H264_SPS_VUIBPPD                              (0x8004DA0DL)
#define DRM_E_H264_SPS_VUIBPMD                              (0x8004DA0EL)
#define DRM_E_H264_SPS_VUIMMLH                              (0x8004DA0FL)
#define DRM_E_H264_SPS_VUIMMLV                              (0x8004DA10L)
#define DRM_E_H264_SPS_VUINRF                               (0x8004DA11L)
#define DRM_E_H264_SPS_VUIMDFB                              (0x8004DA12L)
#define DRM_E_H264_SPS_WIDTH_HEIGHT                         (0x8004DA13L)
#define DRM_E_H264_SPS_AREA                                 (0x8004DA14L)
#define DRM_E_H264_SPS_MINHEIGHT2                           (0x8004DA15L)
#define DRM_E_H264_SPS_MINHEIGHT3                           (0x8004DA16L)
#define DRM_E_H264_SPS_CROPWIDTH                            (0x8004DA17L)
#define DRM_E_H264_SPS_CROPHEIGHT                           (0x8004DA18L)
#define DRM_E_H264_SPS_MORE_RBSP                            (0x8004DA19L)
#define DRM_E_H264_PPS_PPSID                                (0x8004DA50L)
#define DRM_E_H264_PPS_SPSID                                (0x8004DA51L)
#define DRM_E_H264_PPS_SPS_NOT_FOUND                        (0x8004DA52L)
#define DRM_E_ARITHMETIC_OVERFLOW                           (0x80070216L)
#define DRM_E_H264_PPS_NUM_SLICE_GROUPS                     (0x8004DA53L)
#define DRM_E_H264_PPS_SLICE_GROUP_MAX                      (0x8004DA54L)
#define DRM_E_H264_PPS_RUN_LENGTH                           (0x8004DA55L)
#define DRM_E_H264_PPS_TOP_LEFT                             (0x8004DA56L)
#define DRM_E_H264_PPS_SLICE_GROUP_RATE                     (0x8004DA57L)
#define DRM_E_H264_PPS_SLICE_GROUP_MAP                      (0x8004DA58L)
#define DRM_E_H264_PPS_SLICE_GROUP_ID                       (0x8004DA59L)
#define DRM_E_H264_PPS_REF_IDX_L0                           (0x8004DA5AL)
#define DRM_E_H264_PPS_REF_IDX_L1                           (0x8004DA5BL)
#define DRM_E_H264_PPS_WEIGHTED_BIPRED                      (0x8004DA5CL)
#define DRM_E_H264_PPC_PIC_INIT_QP                          (0x8004DA5DL)
#define DRM_E_H264_PPS_PIC_INIT_QS                          (0x8004DA5EL)
#define DRM_E_H264_PPS_PIC_CHROMA_QP                        (0x8004DA5FL)
#define DRM_E_H264_PPS_REDUN_PIC_COUNT                      (0x8004DA61L)
#define DRM_E_H264_PPS_DELTA_SCALE1                         (0x8004DA62L)
#define DRM_E_H264_PPS_DELTA_SCALE2                         (0x8004DA63L)
#define DRM_E_H264_PPS_SECOND_CHROMA_QP                     (0x8004DA64L)
#define DRM_E_H264_PPS_MORE_RBSP                            (0x8004DA65L)
#define DRM_E_H264_SH_SLICE_TYPE                            (0x8004DA70L)
#define DRM_E_H264_SH_SLICE_TYPE_UNSUPPORTED                (0x8004DA71L)
#define DRM_E_H264_SH_PPSID                                 (0x8004DA72L)
#define DRM_E_H264_SH_PPS_NOT_FOUND                         (0x8004DA73L)
#define DRM_E_H264_SH_SPS_NOT_FOUND                         (0x8004DA74L)
#define DRM_E_H264_SH_SLICE_TYPE_PROFILE                    (0x8004DA75L)
#define DRM_E_H264_SH_IDR_FRAME_NUM                         (0x8004DA76L)
#define DRM_E_H264_SH_FIRST_MB_IN_SLICE                     (0x8004DA77L)
#define DRM_E_H264_SH_IDR_PIC_ID                            (0x8004DA78L)
#define DRM_E_H264_SH_REDUN_PIC_COUNT                       (0x8004DA79L)
#define DRM_E_H264_SH_NUM_REF_IDX_LX0                       (0x8004DA7AL)
#define DRM_E_H264_SH_NUM_REF_IDX_LX1                       (0x8004DA7BL)
#define DRM_E_H264_SH_REF_PIC_LIST_REORDER0                 (0x8004DA7CL)
#define DRM_E_H264_SH_REF_PIC_LIST_REORDER1                 (0x8004DA7DL)
#define DRM_E_H264_SH_LUMA_WEIGHT_DENOM                     (0x8004DA7EL)
#define DRM_E_H264_SH_CHROMA_WEIGHT_DENOM                   (0x8004DA7FL)
#define DRM_E_H264_SH_WP_WEIGHT_LUMA0                       (0x8004DA80L)
#define DRM_E_H264_SH_WP_OFFSET_LUMA0                       (0x8004DA81L)
#define DRM_E_H264_SH_WP_WEIGHT_CHROMA0                     (0x8004DA82L)
#define DRM_E_H264_SH_WP_OFFSET_CHROMA0                     (0x8004DA83L)
#define DRM_E_H264_SH_WP_WEIGHT_LUMA1                       (0x8004DA84L)
#define DRM_E_H264_SH_WP_OFFSET_LUMA1                       (0x8004DA85L)
#define DRM_E_H264_SH_WP_WEIGHT_CHROMA1                     (0x8004DA86L)
#define DRM_E_H264_SH_WP_OFFSET_CHROMA1                     (0x8004DA87L)
#define DRM_E_H264_SH_NUM_REF_PIC_MARKING                   (0x8004DA88L)
#define DRM_E_H264_SH_MMCO5_FOLLOWS_MMCO6                   (0x8004DA8CL)
#define DRM_E_H264_SH_MMCO5_DUPLICATE                       (0x8004DA8BL)
#define DRM_E_H264_SH_MMCO6_DUPLICATE                       (0x8004DA8EL)
#define DRM_E_H264_SH_MMCO4_DUPLICATE                       (0x8004DA89L)
#define DRM_E_H264_SH_MMCO4_MAX_LONG_TERM_FRAME             (0x8004DA8AL)
#define DRM_E_H264_SH_MMCO5_COEXIST_MMCO_1_OR_3             (0x8004DA8DL)
#define DRM_E_H264_SH_MODEL_NUMBER                          (0x8004DA8FL)
#define DRM_E_H264_SH_SLICE_QP                              (0x8004DA90L)
#define DRM_E_H264_SH_LF_ALPHA_C0_OFFSET                    (0x8004DA91L)
#define DRM_E_H264_SH_LF_BETA_OFFSET                        (0x8004DA92L)
#define DRM_E_H264_SH_SLICE_GROUP_CHANGE                    (0x8004DA93L)

// HW DRM function ID definitions
typedef enum _PAVP_HWDRM_FUNCTION_ID
{
    PAVP_PLAYREADY_VIDEO = 0x80000022,
    PAVP_PLAYREADY_AUDIO = 0x80000011,
    PAVP_MIRACAST_RX_VIDEO = 0x800f0004,
    PAVP_MIRACAST_RX_AUDIO = 0x000f0005,
    PAVP_WV_HWDRM_PASS_THROUGH = 0x90000001,
    PAVP_WV_HWDRM_CAPS = 0x90000002,
    PAVP_WV_KMD_UPDATE_USAGE_TABLE = 0xC0914,
    PAVP_WV_KMD_DEACTIVE_USAGE_ENTRY = 0xC0915,
    PAVP_WV_KMD_REPORT_USAGE = 0xC0916,
    PAVP_WV_KMD_DELETE_USAGE_ENTRY = 0xC0917,
    PAVP_WV_KMD_DELETE_USAGE_TABLE = 0xC0918,
    PAVP_WV_KMD_LOAD_USAGE_TABLE = 0xC0919,
    DRM_TEE_CTR_AUDIO_MULTIPLE = 0x80000024,
    DRM_TEE_CTR_CONTENT_MULTIPLE = 0x80000027,
    DRM_TEE_CBC_CONTENT_MULTIPLE = 0x80000028,
} PAVP_HWDRM_FUNCTION_ID;

typedef enum _H264_HUC_PASRE_ERROR
{
    //reserve 0~15

    //Error codes specific to general H.264 parsing
    H264_NALU_NO_START_CODE=16,
    H264_NALU_ALL_ZERO,
    H264_NALU_EMULATION,

    //Error codes specific to H.264 SPS parsing
    H264_SPS_PROFILE,
    H264_SPS_IDC,
    H264_SPS_SPSID,
    H264_SPS_CHROMA_IDC,
    H264_SPS_BITDEPTHLUMA,
    H264_SPS_BITDEPTHCHROMA,
    H264_SPS_DELTASCALE1,
    H264_SPS_DELTASCALE2,
    H264_SPS_FRAMENUM,
    H264_SPS_POCTYTPE,
    H264_SPS_POCLSB,
    H264_SPS_POCCYCLE,
    H264_SPS_NUMREFFRAMES,
    LOGICERR,
    H264_SPS_CHROMATOP,
    H264_SPS_CHROMABOTTOM,
    H264_SPS_NALHRD,
    H264_SPS_VLDHRD,
    H264_SPS_VUIBPPD,
    H264_SPS_VUIBPMD,
    H264_SPS_VUIMMLH,
    H264_SPS_VUIMMLV,
    H264_SPS_VUINRF,
    H264_SPS_VUIMDFB,
    H264_SPS_WIDTH_HEIGHT,
    H264_SPS_AREA,
    H264_SPS_MINHEIGHT2,
    H264_SPS_MINHEIGHT3,
    H264_SPS_CROPWIDTH,
    H264_SPS_CROPHEIGHT,
    H264_SPS_MORE_RBSP,

    //Error codes specific to H.264 PPS parsing
    H264_PPS_PPSID,
    H264_PPS_SPSID,
    H264_PPS_SPS_NOT_FOUND,
    ARITHMETIC_OVERFLOW,
    H264_PPS_NUM_SLICE_GROUPS,
    H264_PPS_SLICE_GROUP_MAX,
    H264_PPS_RUN_LENGTH,
    H264_PPS_TOP_LEFT,
    H264_PPS_SLICE_GROUP_RATE,
    H264_PPS_SLICE_GROUP_MAP,
    H264_PPS_SLICE_GROUP_ID,
    H264_PPS_REF_IDX_L0,
    H264_PPS_REF_IDX_L1,
    H264_PPS_WEIGHTED_BIPRED,
    H264_PPC_PIC_INIT_QP,
    H264_PPS_PIC_INIT_QS,
    H264_PPS_PIC_CHROMA_QP,
    H264_PPS_REDUN_PIC_COUNT,
    H264_PPS_DELTA_SCALE1,
    H264_PPS_DELTA_SCALE2,
    H264_PPS_SECOND_CHROMA_QP,
    H264_PPS_MORE_RBSP,

    //Error codes specific to H.264 slice header parsing
    H264_SH_SLICE_TYPE,
    H264_SH_SLICE_TYPE_UNSUPPORTED,
    H264_SH_PPSID,
    H264_SH_PPS_NOT_FOUND,
    H264_SH_SPS_NOT_FOUND,
    H264_SH_SLICE_TYPE_PROFILE,
    H264_SH_IDR_FRAME_NUM,
    H264_SH_FIRST_MB_IN_SLICE,
    H264_SH_IDR_PIC_ID,
    H264_SH_REDUN_PIC_COUNT,
    H264_SH_NUM_REF_IDX_LX0,
    H264_SH_NUM_REF_IDX_LX1,
    H264_SH_REF_PIC_LIST_REORDER0,
    H264_SH_REF_PIC_LIST_REORDER1,
    H264_SH_LUMA_WEIGHT_DENOM,
    H264_SH_CHROMA_WEIGHT_DENOM,
    H264_SH_WP_WEIGHT_LUMA0,
    H264_SH_WP_OFFSET_LUMA0,
    H264_SH_WP_WEIGHT_CHROMA0,
    H264_SH_WP_OFFSET_CHROMA0,
    H264_SH_WP_WEIGHT_LUMA1,
    H264_SH_WP_OFFSET_LUMA1,
    H264_SH_WP_WEIGHT_CHROMA1,
    H264_SH_WP_OFFSET_CHROMA1,
    H264_SH_NUM_REF_PIC_MARKING,
    H264_SH_MMCO5_FOLLOWS_MMCO6,
    H264_SH_MMCO5_DUPLICATE,
    H264_SH_MMCO6_DUPLICATE,
    H264_SH_MMCO4_DUPLICATE,
    H264_SH_MMCO4_MAX_LONG_TERM_FRAME,
    H264_SH_MMCO5_COEXIST_MMCO_1_OR_3,
    H264_SH_MODEL_NUMBER,
    H264_SH_SLICE_QP,
    H264_SH_LF_ALPHA_C0_OFFSET,
    H264_SH_LF_BETA_OFFSET,
    H264_SH_SLICE_GROUP_CHANGE,
} H264_HUC_PARSE_ERROR;

// Play Ready DRM Blobs

// Intel OEM key blobs 
// Video
struct INTEL_OEM_KEYINFO_NONHUC_BLOB
{
    std::array<uint8_t, PAVP_ROTATION_KEY_BLOB_SIZE> TranscryptionKeyWrappedByKb;
    std::array<uint8_t, PAVP_ROTATION_KEY_BLOB_SIZE> Reserved0;
    uint32_t AppId;
    std::array<uint8_t, PAVP_ROTATION_KEY_BLOB_SIZE> CekWrappedByKb;
    std::array<uint8_t, PAVP_ROTATION_KEY_BLOB_SIZE> Reserved1;

    bool operator==(INTEL_OEM_KEYINFO_NONHUC_BLOB const& other) const
    {
        return this->AppId == other.AppId &&
               this->TranscryptionKeyWrappedByKb == other.TranscryptionKeyWrappedByKb &&
               this->CekWrappedByKb == other.CekWrappedByKb;
    }
};

// Audio
struct INTEL_OEM_KEYINFO_NONSECURE_BLOB
{
    std::array<uint8_t, PAVP_ROTATION_KEY_BLOB_SIZE> SampleKeyWrappedByKb;
    std::array<uint8_t, PAVP_ROTATION_KEY_BLOB_SIZE> Reserved0;
    uint32_t AppId;
    std::array<uint8_t, PAVP_ROTATION_KEY_BLOB_SIZE> CekWrappedByKb;
    std::array<uint8_t, PAVP_ROTATION_KEY_BLOB_SIZE> Reserved1;

    bool operator==(INTEL_OEM_KEYINFO_NONSECURE_BLOB const& other) const
    {
        return this->AppId == other.AppId &&
               this->SampleKeyWrappedByKb == other.SampleKeyWrappedByKb && 
               this->CekWrappedByKb == other.CekWrappedByKb;
    }
};

// HuC Video
struct INTEL_OEM_KEYINFO_HUC_BLOB
{
    std::array<uint8_t, PAVP_ROTATION_KEY_BLOB_SIZE> Reserved0;
    std::array<uint8_t, PAVP_ROTATION_KEY_BLOB_SIZE> Reserved1;
    uint32_t  AppId;
    std::array<uint8_t, PAVP_ROTATION_KEY_BLOB_SIZE> CekWrappedByKb;
    std::array<uint8_t, PAVP_ROTATION_KEY_BLOB_SIZE> Reserved2;

    bool operator==(INTEL_OEM_KEYINFO_HUC_BLOB const& other) const
    {
        return this->AppId == other.AppId && 
               this->CekWrappedByKb == other.CekWrappedByKb;
    }
};

enum INTEL_OEM_KEYINFO_BLOB_TYPE
{
    /* Video key info blob */
    INTEL_OEM_KEYINFO_NONHUC_BLOB_TYPE      = 0,

    /* Audio key info blob */
    INTEL_OEM_KEYINFO_NONSECURE_BLOB_TYPE   = 1,

    /* Video HuC key info blob */
    INTEL_OEM_KEYINFO_HUC_BLOB_TYPE         = 2
};

struct INTEL_OEM_KEYINFO_BLOB
{
    INTEL_OEM_KEYINFO_BLOB_TYPE blobType;
    union
    {
        INTEL_OEM_KEYINFO_NONHUC_BLOB       sNonHuCData;
        INTEL_OEM_KEYINFO_NONSECURE_BLOB    sNonSecureData;
        INTEL_OEM_KEYINFO_HUC_BLOB          sHuCData;
    };

    bool operator==(INTEL_OEM_KEYINFO_BLOB const& other) const
    {
        if (this->blobType != other.blobType)
        {
            return false;
        }

        switch (this->blobType)
        {
            case INTEL_OEM_KEYINFO_NONHUC_BLOB_TYPE:
                return this->sNonHuCData == other.sNonHuCData;
                break;
            case INTEL_OEM_KEYINFO_NONSECURE_BLOB_TYPE:
                return this->sNonSecureData == other.sNonSecureData;
                break;
            case INTEL_OEM_KEYINFO_HUC_BLOB_TYPE:
                return this->sHuCData == other.sHuCData;
                break;
        }

        return false;
    }
};

static_assert(std::is_standard_layout<INTEL_OEM_KEYINFO_NONHUC_BLOB>::value, "INTEL_OEM_KEYINFO_NONHUC_BLOB must be standard layout.");
static_assert(std::is_standard_layout<INTEL_OEM_KEYINFO_NONSECURE_BLOB>::value, "INTEL_OEM_KEYINFO_NONSECURE_BLOB must be standard layout.");
static_assert(std::is_standard_layout<INTEL_OEM_KEYINFO_HUC_BLOB>::value, "INTEL_OEM_KEYINFO_HUC_BLOB must be standard layout.");
static_assert(std::is_standard_layout<INTEL_OEM_KEYINFO_BLOB>::value, "INTEL_OEM_KEYINFO_BLOB must be standard layout.");

static_assert(std::is_copy_assignable<INTEL_OEM_KEYINFO_NONHUC_BLOB>::value, "INTEL_OEM_KEYINFO_NONHUC_BLOB must be copy assignable.");
static_assert(std::is_copy_assignable<INTEL_OEM_KEYINFO_NONSECURE_BLOB>::value, "INTEL_OEM_KEYINFO_NONSECURE_BLOB must be copy assignable.");
static_assert(std::is_copy_assignable<INTEL_OEM_KEYINFO_HUC_BLOB>::value, "INTEL_OEM_KEYINFO_HUC_BLOB must be copy assignable.");
static_assert(std::is_copy_assignable<INTEL_OEM_KEYINFO_BLOB>::value, "INTEL_OEM_KEYINFO_BLOB must be copy assignable.");

static_assert(sizeof(INTEL_OEM_KEYINFO_NONHUC_BLOB) == sizeof (INTEL_OEM_KEYINFO_NONSECURE_BLOB), "expected nonhuc blob to be same size as nonsecure blob");
static_assert(sizeof(INTEL_OEM_KEYINFO_NONSECURE_BLOB) == sizeof (INTEL_OEM_KEYINFO_HUC_BLOB), "expected nonsecure blob to be same size as huc blob");
static_assert(sizeof(INTEL_OEM_KEYINFO_BLOB) == (sizeof(INTEL_OEM_KEYINFO_HUC_BLOB)+sizeof(INTEL_OEM_KEYINFO_BLOB_TYPE)), "wrong size for INTEL_OEM_KEYINFO_BLOB_TYPE");

struct HuCSliceHeaderExtractionParams
{
    uint8_t                        *encryptedFrame;
    size_t                          encryptedFrameSize;
    INTEL_OEM_KEYINFO_BLOB         *oemKeyInfo;
    std::vector<MHW_SEGMENT_INFO> segmentInfos;
    DecryptStatusAvc                DXVAStatusReport;
    bool                            bRIVRetieved;
    uint64_t                        uRIVNonce;
};

struct CP_HWDRM_IV
{
    union
    {
        struct {
            uint32_t DW0;
            uint32_t DW1;
            uint32_t DW2;
            uint32_t DW3;
        };
        std::array<uint8_t, 16> data;
    };
};

// User-defined hash and equality functions for CP_HWDRM_IV struct to support std::unordered_map
namespace std
{
    template<> struct hash<CP_HWDRM_IV>
    {
        size_t operator()(const CP_HWDRM_IV &iv) const
        {
            // Hash function based on Fowler-Noll-Vo FNV-1 algorithm
            // but with smaller values since size_t is a 32-bit integer on 32-bit builds
            size_t basis = 2166136261;
            size_t prime = 16777619;
            size_t result = basis;
            for (auto&& byte : iv.data)
            {
                result = (result * prime) ^ byte;
            }
            return result;
        }
    };
    template<> struct equal_to<CP_HWDRM_IV>
    {
        bool operator()(const CP_HWDRM_IV &lhs, const CP_HWDRM_IV &rhs) const
        {
            return lhs.data == rhs.data;
        }
    };
}

// First entry in this pair is the number of clear bytes in a given encrypted region
// Second entry in this pair is the number of encrypted bytes in a given encrypted region
typedef std::pair<uint32_t, uint32_t> RegionMapEntry;

typedef struct ByteOffsetEntry
{
    // Offset of the bytes within the entire encrypted buffer
    uint32_t offset;
    // Number of bytes to be processed
    uint32_t numBytes;
    // Whether the bytes were encrypted in the input buffer
    bool wasEncrypted;

    ByteOffsetEntry(uint32_t offset, uint32_t numBytes, bool wasEncrypted) : offset(offset), numBytes(numBytes), wasEncrypted(wasEncrypted) {}

    bool operator< (const ByteOffsetEntry& other)
    {
        return this->offset < other.offset;
    }
} ByteOffsetEntry;

struct AudioDecryptParams
{
    uint8_t                     *encryptedBuf;
    size_t                      encryptedBufSize;
    uint8_t                     *outputBuf;
    INTEL_OEM_KEYINFO_BLOB      *oemKeyInfo;
    bool                        is16ByteIV;
    std::vector<CP_HWDRM_IV>    ivValues;
    std::vector<uint32_t>       regionCounts;
    std::vector<RegionMapEntry> regionMap;
    uint32_t                    numEncryptedStripeBlocks;
    uint32_t                    numClearStripeBlocks;
    bool                        isCBC;
};

typedef struct AudioBufferInfo
{
    // offset after padding
    size_t offsetAfterPadding;

    // Number of bytes before padding,not need to deal with padding for alignment
    size_t bufferSizeBeforePadding;

    // offset before padding
    size_t offsetBeforePadding;

    // The start AES encrypted counter value of the current subsample
    size_t startAesEncCounter;

    // Whether the bytes were encrypted in the input buffer
    bool wasEncrypted;

    // Whether the current encrypted buffer is generated by rollover
    bool wasGenerateByRollover;

    AudioBufferInfo(size_t offsetAfterPadding, size_t bufferSizeBeforePadding, size_t offsetBeforePadding,
        size_t startAesEncCounter, bool wasEncrypted, bool wasGenerateByRollover) :
        offsetAfterPadding(offsetAfterPadding), bufferSizeBeforePadding(bufferSizeBeforePadding), offsetBeforePadding(offsetBeforePadding),
        startAesEncCounter(startAesEncCounter),wasEncrypted(wasEncrypted), wasGenerateByRollover(wasGenerateByRollover) {}

} AudioBufferInfo;

typedef struct RolloverMapInfo
{
    // offset within the encrypted buffer
    size_t offset;

    // Number of bytes that not need to rollover
    size_t numBytesBeforeRollover;

    // Number of bytes that need to rollover
    size_t numBytesAfterRollover;

    RolloverMapInfo(size_t offset, size_t numBytesBeforeRollover, size_t numBytesAfterRollover) :
        offset(offset), numBytesBeforeRollover(numBytesBeforeRollover), numBytesAfterRollover(numBytesAfterRollover) {}

} RolloverMapInfo;

#endif  // __CP_DRM_DEFS_H__.
