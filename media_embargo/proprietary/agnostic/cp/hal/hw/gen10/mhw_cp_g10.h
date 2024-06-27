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
//! \file     mhw_cp_g10.h
//! \brief    MHW interface for content protection
//! \details  Impelements the Gen-9 functionalities for content protection
//!

#ifndef __MHW_CP_G10_H__
#define __MHW_CP_G10_H__

#include "mhw_cp.h"

class MhwCpG10: public MhwCpBase
{
public:
    // ----- interface functions ------------------------------------------- //
    MhwCpG10(MOS_STATUS& eStatus, MOS_INTERFACE& osInterface);
    virtual ~MhwCpG10();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Check support for HW Counter Auto Increment
    /// \details     From KBL+ HW Counter increment is valid for Widi and 
    ///              From ICL+ HW Counter increment is supported for both Widi and Transcode
    ///
    /// \param       osInterface  [in] to obtain HW unit being used info
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual bool IsHwCounterIncrement(PMOS_INTERFACE osInterface);

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
};

// HW units
class CpHwUnitG10Rcs : public CpHwUnitRcs
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
    ////////////////////////////////////////////////////////////////////////
    MOS_STATUS AddProlog(
        PMOS_INTERFACE      osInterface,
        PMOS_COMMAND_BUFFER cmdBuffer) override;
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
    MOS_STATUS AddEpilog(
        PMOS_INTERFACE      osInterface,
        PMOS_COMMAND_BUFFER cmdBuffer) override;

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
    MOS_STATUS AddCheckForEarlyExit(
        PMOS_INTERFACE      osInterface,
        PMOS_COMMAND_BUFFER cmdBuffer) override;

    CpHwUnitG10Rcs(MOS_STATUS& mos_status, MOS_INTERFACE& osInterface);
    ~CpHwUnitG10Rcs() override;

protected:
    // ---- data members --------------------------------------------------- //
    MOS_RESOURCE m_ScratchResource;
    void*        m_pScratchBuffer = nullptr;
};

class CpHwUnitG10Vecs : public CpHwUnitVecs
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

    CpHwUnitG10Vecs(MOS_STATUS& mos_status, MOS_INTERFACE& osInterface);
    virtual ~CpHwUnitG10Vecs(){};
};

class CpHwUnitG10Vcs : public CpHwUnitVcs
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

    CpHwUnitG10Vcs(MOS_STATUS& mos_status, MOS_INTERFACE& osInterface);
    virtual ~CpHwUnitG10Vcs(){};
};

#endif
