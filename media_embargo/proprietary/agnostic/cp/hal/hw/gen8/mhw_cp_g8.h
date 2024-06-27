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
//! \file     mhw_cp_g8.h
//! \brief    MHW interface for content protection
//! \details  Impelements the Gen-8 functionalities for content protection
//!

#ifndef __MHW_CP_G8_H__
#define __MHW_CP_G8_H__

#include "mhw_cp.h"

class MhwCpG8 : public MhwCpBase
{
public:
    // ----- interface functions ------------------------------------------- //
    MhwCpG8(MOS_STATUS& mos_status, MOS_INTERFACE& osInterface);
    virtual ~MhwCpG8();

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
    virtual bool IsHwCounterIncrement(PMOS_INTERFACE osInterface);

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
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddMfxAesState(
        bool                 isDecodeInUse,
        PMOS_COMMAND_BUFFER  cmdBuffer,
        PMHW_BATCH_BUFFER    batchBuffer,
        PMHW_PAVP_AES_PARAMS params);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add MFX_CRYPTO_COPY_BASE_ADDR to the command buffer.
    /// \details
    /// \param       cmdBuffer      [in] the command buffer to add the cmd
    /// \param       params         [in] Params structure used to populate the HW command
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddMfxCryptoCopyBaseAddr(
        PMOS_COMMAND_BUFFER               cmdBuffer,
        PMHW_PAVP_CRYPTO_COPY_ADDR_PARAMS params);

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
    /// \brief       Add MFX_CRYPTO_COPY_BASE_ADDR to the command buffer.
    /// \details
    /// \param       cmdBuffer      [in] the command buffer to add the cmd
    /// \param       params         [in] Params structure used to populate the HW command
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddMfxCryptoCopy(
        PMOS_COMMAND_BUFFER          cmdBuffer,
        PMHW_PAVP_CRYPTO_COPY_PARAMS params);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       set the bits 22 and 7 of MI_FLUSH_DW based on PAVP status
    /// \details     The function post-process a MI_FLUSH_DW instance to toggle
    ///              its protection bit and protected apptype
    ///              based on PAVP status
    ///
    /// \param       osInterface  [in] to obtain PAVP status
    /// \param       cmd          [in] pointer to the MI_FLUSH_DW instance
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS SetProtectionSettingsForMiFlushDw(
        PMOS_INTERFACE osInterface,
        void           *cmd);

    ////////////////////////////////////////////////////////////////////////////
    /// \brief       get the state level command size and patch list size for CP
    /// \details     The function calculate the cp relate comamnd size in state
    ///              level
    ///
    /// \param       cmdSize        [in] command size
    /// \param       patchListSize  [in] patch list size
    ///////////////////////////////////////////////////////////////////////////
    virtual void GetCpStateLevelCmdSize(uint32_t& cmdSize, uint32_t& patchListSize);

    ////////////////////////////////////////////////////////////////////////////
    /// \brief       get the slice level command size and patch list size for CP
    /// \details     The function calculate the cp relate comamnd size in slcie
    ///              level
    ///
    /// \param       cmdSize        [in] command size
    /// \param       patchListSize  [in] patch list size
    ///////////////////////////////////////////////////////////////////////////
    virtual void GetCpSliceLevelCmdSize(uint32_t& cmdSize, uint32_t& patchListSize);

    ///////////////////////////////////////////////////////////////////////////
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
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddProlog(
        PMOS_INTERFACE      osInterface,
        PMOS_COMMAND_BUFFER cmdBuffer);

    virtual MOS_STATUS GetTranscryptKernelInfo(
        const uint32_t                  **transcryptMetaData,
        uint32_t                        *transcryptMetaDataSize,
        const uint32_t                  **encryptedKernels,
        uint32_t                        *encryptedKernelsSize,
        MHW_PAVP_KERNEL_ENCRYPTION_TYPE encryptionType);

    // ----- static functions shared by several Gens. ------------------------------------------- //
    static MOS_STATUS AddMfxAesStateG8(
        MhwCpBase*           mhwCp,
        PMOS_INTERFACE       osInterface,
        bool                 isDecodeInUse,
        PMOS_COMMAND_BUFFER  cmdBuffer,
        PMHW_BATCH_BUFFER    batchBuffer,
        PMHW_PAVP_AES_PARAMS params);

    static MOS_STATUS AddMfxCryptoCopyBaseAddrG8(
        MhwCpBase*                        mhwCp,
        PMOS_INTERFACE                    osInterface,
        PMOS_COMMAND_BUFFER               cmdBuffer,
        PMHW_PAVP_CRYPTO_COPY_ADDR_PARAMS params);

    static MOS_STATUS AddMfxCryptoCopyG8(
        MhwCpBase*                   mhwCp,
        PMOS_COMMAND_BUFFER          cmdBuffer,
        PMHW_PAVP_CRYPTO_COPY_PARAMS params);
};

// HW units
class CpHwUnitG8Rcs : public CpHwUnitRcs
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
        PMOS_COMMAND_BUFFER cmdBuffer);

    CpHwUnitG8Rcs(MOS_STATUS& mos_status, MOS_INTERFACE& osInterface);
    virtual ~CpHwUnitG8Rcs(){};
};

class CpHwUnitG8Vecs : public CpHwUnitVecs
{
public:
    CpHwUnitG8Vecs(MOS_STATUS& mos_status, MOS_INTERFACE& osInterface);
    virtual ~CpHwUnitG8Vecs(){};
};

class CpHwUnitG8Vcs : public CpHwUnitVcs
{
public:
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

    CpHwUnitG8Vcs(MOS_STATUS& mos_status, MOS_INTERFACE& osInterface);
    virtual ~CpHwUnitG8Vcs(){};
};

const std::map<MOS_HW_COMMAND, DecryptBitOffset> g_decryptBitOffsets_g8{
    // HW Command ID                        Driver Id Bit Num, Driver ID DW Offset
    {MOS_MI_BATCH_BUFFER_START_RCS,         DecryptBitOffset()}, // No decrypt bits
    {MOS_MFX_PIPE_BUF_ADDR,                 DecryptBitOffset(2,  2)},
    {MOS_MFX_INDIRECT_OBJ_BASE_ADDR,        DecryptBitOffset(2,  2)},
    {MOS_MFX_BSP_BUF_BASE_ADDR,             DecryptBitOffset(2,  2)},
    {MOS_MFX_AVC_DIRECT_MODE,               DecryptBitOffset(2,  2)},
    {MOS_MFX_VC1_DIRECT_MODE,               DecryptBitOffset(2,  2)},
    {MOS_MFX_VP8_PIC,                       DecryptBitOffset(2,  2)},
    {MOS_MFX_DBK_OBJECT,                    DecryptBitOffset(2,  2)},
    {MOS_MI_CONDITIONAL_BATCH_BUFFER_END,   DecryptBitOffset(20, -2)},
    {MOS_MI_BATCH_BUFFER_START,             DecryptBitOffset(9,  -1)},
    {MOS_SURFACE_STATE,                     DecryptBitOffset(26, -7)},
    {MOS_SURFACE_STATE_ADV,                 DecryptBitOffset(2,  -1)},
    {MOS_STATE_BASE_ADDR,                   DecryptBitOffset(6,  0)},
    {MOS_VEBOX_STATE,                       DecryptBitOffset(27, -1)},
    {MOS_VEBOX_DI_IECP,                     DecryptBitOffset(2,  0)},
};

#endif
