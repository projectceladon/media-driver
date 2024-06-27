/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2020
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
//! \file     mhw_cp_xe_lpm_plus.h
//! \brief    MHW interface for content protection
//! \details  Impelements the Gen-13 functionalities for content protection
//!

#ifndef __MHW_CP_G13_H__
#define __MHW_CP_G13_H__

#include "mhw_cp.h"

class MhwCpG13 : public MhwCpBase
{
public:
    // ----- interface functions ------------------------------------------- //
    MhwCpG13(MOS_STATUS& eStatus, MOS_INTERFACE& osInterface);
    virtual ~MhwCpG13();

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
        PMOS_COMMAND_BUFFER cmdBuffer) override;

    //////////////////////////////////////////////////////////////////////////////////////////
    /// \brief       KCR updates/refreshes counter on EncryptionOff(AddMfxCryptoKeyExchange)
    /// \details
    /// \param       osInterface    [in] pointer to OS interface
    /// \param       cmdBuffer      [in] the command buffer to add the cmd
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS error
    ///////////////////////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS RefreshCounter(
        PMOS_INTERFACE osInterface,
        PMOS_COMMAND_BUFFER cmdBuffer) override;

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
    virtual bool IsHwCounterIncrement(PMOS_INTERFACE osInterface) override;

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
        PMHW_PAVP_AES_PARAMS params) override;

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
        uint16_t            currentIndex) override;

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
        PMHW_PAVP_CRYPTO_COPY_ADDR_PARAMS params) override;

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
        PMHW_PAVP_MFX_CRYPTO_KEY_EXCHANGE_CMD_PARAMS params) override;

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
        PMHW_PAVP_MFX_CRYPTO_INLINE_STATUS_READ_CMD_PARAMS params) override;

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
        PMHW_PAVP_CRYPTO_COPY_PARAMS params) override;

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
        PMHW_PAVP_AES_PARAMS params) override;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add HCP_AES_STATE to the command buffer.
    /// \details
    /// \param       isDecodeInUse    [in] indicate if decode in use
    /// \param       cmdBuffer      [in] the command buffer to add the cmd
    /// \param       batchBuffer    [in] the batch buffer to add the cmd
    /// \param       params         [in] Params structure used to populate the HW command
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddHcpAesState(
        bool                 isDecodeInUse,
        PMOS_COMMAND_BUFFER  cmdBuffer,
        PMHW_BATCH_BUFFER    batchBuffer,
        PMHW_PAVP_AES_PARAMS params) override;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief       Add Huc AES State to the command buffer.
    /// \details
    /// \param       cmdBuffer      [in] the command buffer to add the cmd
    /// \param       params         [in] Params structure used to populate the HW command
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddHucAesState(
        PMOS_COMMAND_BUFFER  cmdBuffer,
        PMHW_PAVP_AES_PARAMS params) override;
    ////////////////////////////////////////////////////////////////////////////
    /// \brief       get the state level command size and patch list size for CP
    /// \details     The function calculate the cp relate comamnd size in state
    ///              level
    ///
    /// \param       cmdSize        [in] command size
    /// \param       patchListSize  [in] patch list size
    ///////////////////////////////////////////////////////////////////////////
    virtual void GetCpStateLevelCmdSize(uint32_t& cmdSize, uint32_t& patchListSize) override;

    ////////////////////////////////////////////////////////////////////////////
    /// \brief       get the slice level command size and patch list size for CP
    /// \details     The function calculate the cp relate comamnd size in slcie
    ///              level
    ///
    /// \param       cmdSize        [in] command size
    /// \param       patchListSize  [in] patch list size
    ///////////////////////////////////////////////////////////////////////////
    virtual void GetCpSliceLevelCmdSize(uint32_t& cmdSize, uint32_t& patchListSize) override;

    virtual MOS_STATUS GetTranscryptKernelInfo(
        const uint32_t                  **transcryptMetaData,
        uint32_t                        *transcryptMetaDataSize,
        const uint32_t                  **encryptedKernels,
        uint32_t                        *encryptedKernelsSize,
        MHW_PAVP_KERNEL_ENCRYPTION_TYPE encryptionType) override;

    ////////////////////////////////////////////////////////////////////////////
    /// \brief      add GSC heci command to the batch buffer
    /// \details    The function assemables the GSC heci command which carries the
    ///             input/output addresss of PAVP FW commands, and insert the GSC
    ///             HECI command to MI batch buffer
    ///
    /// \param       osInterface    [in] pointer to OS interface
    /// \param       cmdBuffer      [in] the command buffer to add the cmd
    /// \param       params         [in] Params structure used to populate the HW command
    ///
    /// \return      If success, return MOS_STATUS_SUCCESS
    ///              If fail, return other MOS errors
    ///////////////////////////////////////////////////////////////////////////
    virtual MOS_STATUS AddGSCHeciCmd(
        PMOS_INTERFACE                  osInterface,
        PMOS_COMMAND_BUFFER             cmdBuffer,
        PMHW_GSC_HECI_Params            params);


    // ----- static functions shared by several Gens. ------------------------------------------- //
    static MOS_STATUS AddMfxCryptoCopyG13(
        MhwCpBase*                   mhwCp,
        PMOS_COMMAND_BUFFER          cmdBuffer,
        PMHW_PAVP_CRYPTO_COPY_PARAMS params);
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
        PMOS_COMMAND_BUFFER cmdBuffer) override;
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
        void               *mfxRegisters,
        uint32_t            cencBufIndex,
        PMOS_RESOURCE       resource,
        PMOS_COMMAND_BUFFER cmdBuffer) override;
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
        PMHW_CP_COPY_PARAMS params) override;

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
        MhwMiInterface* miInterface) override;


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
        std::shared_ptr<mhw::mi::Itf> m_miItf) override;

    MEDIA_CLASS_DEFINE_END(MhwCpG13)
};

// HW units
class CpHwUnitG13Rcs : public CpHwUnitRcs
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
        PMOS_COMMAND_BUFFER cmdBuffer) override;
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

    CpHwUnitG13Rcs(MOS_STATUS& eStatus, MOS_INTERFACE& osInterface);
    ~CpHwUnitG13Rcs() override;

protected:
    // ---- data members --------------------------------------------------- //
    MOS_RESOURCE m_ScratchResource;
    void*        m_pScratchBuffer = nullptr;

    MEDIA_CLASS_DEFINE_END(CpHwUnitG13Rcs)
};

class CpHwUnitG13Vecs : public CpHwUnitVecs
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
        PMOS_COMMAND_BUFFER cmdBuffer) override;

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

    CpHwUnitG13Vecs(MOS_STATUS& eStatus, MOS_INTERFACE& osInterface);
    virtual ~CpHwUnitG13Vecs();

protected:
    // ---- data members --------------------------------------------------- //
    MOS_RESOURCE m_ResForKeyValidRegisterScratch;
    MOS_RESOURCE m_ResForSessionInPlayRegisterScratch;

    MEDIA_CLASS_DEFINE_END(CpHwUnitG13Vecs)
};

class CpHwUnitG13Vcs : public CpHwUnitVcs
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
        PMOS_COMMAND_BUFFER cmdBuffer) override;

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
        PMOS_COMMAND_BUFFER cmdBuffer) override;

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
        PMHW_PAVP_MFX_CRYPTO_INLINE_STATUS_READ_CMD_PARAMS params) override;

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
        PMHW_PAVP_MFX_CRYPTO_KEY_EXCHANGE_CMD_PARAMS params) override;

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

    CpHwUnitG13Vcs(MOS_STATUS& eStatus, MOS_INTERFACE& osInterface);
    ~CpHwUnitG13Vcs() override;

protected:
    // ---- data members --------------------------------------------------- //
    MOS_RESOURCE m_ResForKeyValidRegisterScratch;
    MOS_RESOURCE m_ResForSessionInPlayRegisterScratch;

    MEDIA_CLASS_DEFINE_END(CpHwUnitG13Vcs)
};

class CpHwUnitG13Gcs : public CpHwUnitGcs
{
public:
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

    CpHwUnitG13Gcs(MOS_STATUS& eStatus, MOS_INTERFACE& osInterface);
    ~CpHwUnitG13Gcs();

protected:
    // ---- data members --------------------------------------------------- //
    MOS_RESOURCE m_ResForSWProxyRegisterScratch;

    MEDIA_CLASS_DEFINE_END(CpHwUnitG13Gcs)
};

#endif //__MHW_CP_G13_H__