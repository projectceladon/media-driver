/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2017-2020
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
//! \file     media_libva_caps_cp.h
//! \brief    This file defines the C++ class/interface for encryption related capbilities.
//!

#ifndef __MEDIA_LIBVA_CAPS_CP_H__
#define __MEDIA_LIBVA_CAPS_CP_H__

#include "media_libva.h"
#include "media_libva_caps_cp_interface.h"
#include "va_private.h"

class MediaLibvaCapsCp: public MediaLibvaCapsCpInterface
{
public:
    MediaLibvaCapsCp(DDI_MEDIA_CONTEXT *mediaCtx, MediaLibvaCaps *mediaCaps);
    virtual ~MediaLibvaCapsCp();

    //!
    //! \brief    Return if decode encrytion is supported
    //!
    //! \param    [in] mediaCtx
    //!           Pointer to DDI_MEDIA_CONTEXT
    //!
    //! \return   false: decode encrytion isn't supported on current platform
    //!           true: decode encrytion is supported on current platform
    //!
    bool IsDecEncryptionSupported(DDI_MEDIA_CONTEXT *mediaCtx) override;

    //!
    //! \brief    Get the supported decode encrytion types
    //!
    //! \param    [in] profile
    //!           VAProfile
    //!
    //! \param    [in/out] encryptionType
    //!           Pointer to a array of uint32_t, which will be filled with result.
    //!
    //! \param    [in] arraySize
    //!           The array size of encryptionType.
    //!
    //! \return   Return the real number of supported decode encrytion types
    //!           Return -1 if arraySize is too small or profile is invalide
    //!
    int32_t GetEncryptionTypes(VAProfile profile, uint32_t *encryptionType, uint32_t arraySize) override;

    VAStatus LoadCpProfileEntrypoints() override;

    bool IsCpConfigId(VAConfigID configId) override;

    VAStatus CreateCpConfig(
        int32_t profileTableIdx,
        VAEntrypoint entrypoint,
        VAConfigAttrib *attribList,
        int32_t numAttribs,
        VAConfigID *configId) override;

    VAStatus GetCpConfigAttr(
            VAConfigID configId,
            VAProfile *profile,
            VAEntrypoint *entrypoint,
            uint32_t *session_mode,
            uint32_t *session_type,
            uint32_t *cipher_algo,
            uint32_t *cipher_block_size,
            uint32_t *cipher_mode,
            uint32_t *cipher_sample_type,
            uint32_t *usage);

protected:

    //!
    //! \struct   CpConfig
    //! \brief    CP configuration
    //!
    struct CpConfig
    {
        uint32_t m_CpSessionMode;       //!< CP session mode
        uint32_t m_CpSessionType;       //!< CP session type
        uint32_t m_CipherAlgo;          //!< CP cipher algorithm
        uint32_t m_CipherBlockSize;     //!< CP cipher block size
        uint32_t m_CipherMode;          //!< CP cipher counter mode
        uint32_t m_CipherSampleType;    //!< CP cipher sample type
        uint32_t m_CpUsage;             //!< CP usage

        CpConfig(
            const uint32_t cpSessionMode,
            const uint32_t cpSessionType,
            const uint32_t cipherAlgo,
            const uint32_t cipherBlockSize,
            const uint32_t cipherMode,
            const uint32_t cipherSampleType,
            const uint32_t cpUsage)
        : m_CpSessionMode(cpSessionMode),
          m_CpSessionType(cpSessionType),
          m_CipherAlgo(cipherAlgo),
          m_CipherBlockSize(cipherBlockSize),
          m_CipherMode(cipherMode),
          m_CipherSampleType(cipherSampleType),
          m_CpUsage(cpUsage) {}
    };

    std::vector<CpConfig> m_cpConfigs;                  //!< Store supported cp configs

    static const uint32_t m_arrCpSessionMode[];         //!< Store cp session modes
    static const uint32_t m_arrCpSessionType[];         //!< Store cp session types
    static const uint32_t m_arrCipherMode[];            //!< Store cipher counter modes
    static const uint32_t m_arrCipherSampleType[];      //!< Store cipher sample types
    static const uint32_t m_arrCpUsage[];               //!< Store cp usages

    //!
    //! \brief  Store attribute list pointers
    //!
    std::vector<AttribMap *> m_attributeLists;

    VAStatus AddCpConfig(
        uint32_t cpSessionMode,
        uint32_t cpSessionType,
        uint32_t cipherAlgo = 0,
        uint32_t cipherBlockSize = 0,
        uint32_t cipherMode = 0,
        uint32_t cipherSampleType = 0,
        uint32_t cpUsage = 0
    );

    VAStatus CreateAttributeList(AttribMap **attributeList);

    //!
    //! \brief    Free attribuate lists
    //!
    VAStatus FreeAttributeList();

    VAStatus CreateCpAttributes(
            VAProfile profile,
            VAEntrypoint entrypoint,
            AttribMap **attributeList);

    VAStatus AddProfileEntry(
            VAProfile profile,
            VAEntrypoint entrypoint,
            AttribMap *attributeList,
            int32_t configIdxStart,
            int32_t configNum);

};
extern "C" MediaLibvaCapsCpInterface* Create_MediaLibvaCapsCp(DDI_MEDIA_CONTEXT *mediaCtx, MediaLibvaCaps *mediaCaps);
extern "C" void Delete_MediaLibvaCapsCp(MediaLibvaCapsCpInterface* pMediaLibvaCapsCpInterface);
#endif
