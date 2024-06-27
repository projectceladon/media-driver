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
//! \file     mos_os_cp_specific.h
//! \brief    OS specific implement for CP related functions
//!

#ifndef __MOS_OS_CP_SPECIFIC_H__
#define __MOS_OS_CP_SPECIFIC_H__

#include <map>
#include "mos_defs.h"
#include "mos_os_hw.h"
#include "mos_os_cp_interface_specific.h"
#include "pavp_types.h"
#include "GmmLib.h"

//! \brief Forward declarations
typedef struct _MOS_INTERFACE *PMOS_INTERFACE;

typedef union _MOS_CP_COMMAND_PROPERTIES
{
    struct
    {
        uint32_t HasDecryptBits            : 1; // This cmd has a PAVP related decrypt bit
        int32_t DecryptBitDwordOffset      : 8; // signed offset (in # of DWORDS) from the patch location to where the cmd's decrypt bit is
        uint32_t DecryptBitNumber          : 5; // which bit to set in the dword specified by DecryptBitDwordOffset (0 - 31)
        uint32_t ForceEncryptionForNoMocs  : 1; // force encryption
        uint32_t writeAsSecure             : 1; // write as secured
        uint32_t reserved                  : 16;// Reserved
    };
    uint32_t Value;
} MOS_CP_COMMAND_PROPERTIES;

class MosCp: public MosCpInterface
{
public:
    MosCp(void  *pvOsInterface);

    ~MosCp();

    //!
    //! \brief    Registers CP patch entry for Heavy Mode
    //! \details  The function registers parameters for CP heavy mode tracking.
    //! \param    [in] pPatchAddress
    //!           Address to patch location in GPU command buffer
    //! \param    [in] bWrite
    //!           is write operation
    //! \param    [in] HwCommandType
    //!           the cmd that the cpCmdProps assocites with
    //! \param    [in] forceDwordOffset
    //!           override of uint32_t offset to Encrypted Data parameter
    //! \param    [in] plResource
    //!           hAllocation - Allocation handle to notify GMM about possible 
    //!           changes in protection flag for display surface tracking
    //!           Parameter is void to mos_os OS agnostic
    //!           pGmmResourceInfo - GMM resource information to notify GMM about 
    //!           possible changes in protection flag for display surface tracking
    //!           Parameter is void to mos_os OS agnostic
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS RegisterPatchForHM(
        uint32_t       *pPatchAddress,
        uint32_t       bWrite,
        MOS_HW_COMMAND HwCommandType,
        uint32_t       forceDwordOffset,
        void           *plResource,
        void           *pPatchLocationList);

    //!
    //! \brief    UMD level patching
    //! \details  The function performs command buffer patching for encrypted surfaces for Android
    //! \param    virt
    //!           [in] virtual address to be patched
    //! \param    pvCurrentPatch
    //!           [in] pointer to current patch address
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if success, else fail reason
    MOS_STATUS PermeatePatchForHM(
        void        *virt,
        void        *pvCurrentPatch,
        void        *resource);

    //!
    //! \brief    UMD level patching
    //! \details  This function will only called from mhw AddMiBatchBufferEnd to ensure encryption bit in 2nd/3rd BB can get patched correctly.
    //!           In linux flow, 2nd/3rd BB will be locked before patch operation.
    //!           This is a temp solution to patch encryption bit for 2nd/3rd BB
    //!           The final solution should be patch encryption bit when resource address is added to a BB. However encryption bit is easily modified in current MHW design
    //!           as driver will continue to modify cmd DWs after addResourceToCmd. Can not afford the risk of the final solution now.
    //!
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if success, else fail reason
    MOS_STATUS PermeateBBPatchForHM();

    //!
    //! \brief    prepare source & target surface for cp
    //! \details  no action require for linux implement
    //! \param    [in] source
    //!           Refernce to list of OsResources
    //! \param    [in] sourceCount
    //!           Refernce to the number of OsResources
    //! \param    [in] target
    //!           Refernce to list of OsResources
    //! \param    [in] targetCount
    //!           Refernce to the number of OsResources
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS PrepareResources(
        void        *source[],
        uint32_t    sourceCount,
        void        *target[],
        uint32_t    targetCount);

    //!
    //! \brief    Get physical buffer from GMM
    //! \details  DG1 GSC cannot acces system memory
    //!           This function gets physical buffer from GMM
    //!           This buffer is required for UMD-GSC communication
    //! \param    [in] ppGSCBuffer
    //!           Reference to pointer of GSC special buffer
    //! \param    [in] pBufferSize
    //!           Reference to size of memory allocated
    //! \return   GMM_STATUS
    //!           GMM_SUCCESS if success, else fail reason
    //!
    MOS_STATUS AllocateTEEPhysicalBuffer(
        void        **ppGSCBuffer,
        uint32_t    *pBufferSize);

    //!
    //! \brief    Free UMD-GSC communication physical buffer
    //! \details  This function calls GMM function to deallocate physical memory
    //! \param    [in] pGSCBuffer
    //!           Reference to GSC special buffer
    //! \return   GMM_STATUS
    //!           GMM_SUCCESS if success, else fail reason
    //!
    MOS_STATUS DeAllocateTEEPhysicalBuffer(
        void     *pGSCBuffer);

    //!
    //! \brief    Determines if this UMD Context is running with CP enabled
    //! \return   bool
    //!           true if CP enabled, false otherwise
    //!
    bool IsCpEnabled();

    //!
    //! \brief    transist UMD Context running with/without CP enabled
    //! \param    [in] isCpInUse
    //!           is PAVP in use
    //! \return   void
    //!
    void SetCpEnabled(
        bool        isCpInUse);

    //!
    //! \brief    register protected gem context to CP session
    //! \param    [in] bRegister
    //!           register gem context if true else check validation of gem context
    //! \param    [in] identifier
    //!           class creating the protected gem context
    //! \param    [out] bIsStale
    //!           if gem context is stale or not
    //! \return   void
    //!
    void RegisterAndCheckProtectedGemCtx(bool bRegister, void *identifier, bool *bIsStale);

    //!
    //! \brief    update Cp context in os context
    //! \param    [in] value
    //!           CpTag value
    //! \return   void
    //!
    void UpdateCpContext(
        uint32_t     value);

    //!
    //! \brief    Determines if this UMD Context is running in PAVP Heavy Mode
    //! \return   bool
    //!           true if PAVP Heavy Mode, false otherwise
    //!
    bool IsHMEnabled();

    //!
    //! \brief    Determines if this UMD Context is running in PAVP transcode session
    //! \return   bool
    //!           true if PAVP transcode session, false otherwise
    //!
    bool IsTSEnabled();

    //!
    //! \brief    Determines if this UMD Context is running in PAVP Isolated Decode Mode
    //! \return   bool
    //!           true if PAVP Isolated Decode Mode, false otherwise
    //!
    bool IsIDMEnabled();

    //!
    //! \brief    Determines if this UMD Context is running in PAVP Stout Mode
    //! \return   bool
    //!           true if PAVP Stout Mode, false otherwise
    //!
    bool IsSMEnabled();

    //!
    //! \brief    Determines if CENC Secure Decode can use context based submission
    //! \return   bool
    //!           true if gen12+, false otherwise
    //!
    bool IsCencCtxBasedSubmissionEnabled();

    //!
    //! \brief    Determines if tear down happened
    //! \return   bool
    //!           true if tear down happened, false otherwise
    //!
    bool IsTearDownHappen();

    //!
    //! \brief    Get Pavp session ID from UMD
    //! \return   uint32_t
    //!           Current Pavp session ID
    //!
    uint32_t GetSessionId();

    //!
    //! \brief     This function is unimplemented in Windows
    //! \param     [in] pOsInterface
    //!            OS Interface
    //! \param     [in] pOsResource
    //!            Pointer to OS resource structure
    //! \return    bool
    //!            true if the bo is encrypted, otherwise false
    //!
    bool GetResourceEncryption(
        void        *pResource);

    MOS_STATUS SetResourceEncryption(
        void        *pResource,
        bool        bEncryption);

    //!
    //! \brief    Used by content protection to cache the transcrypted and authenticated kernels for access
    //!           by video processor.
    //! \param    [in] *pTKs
    //!           Transcrypted kernels to be cached
    //! \param    [in] uiTKSize
    //!           Size in bytes of the to-be-cached transcrypted kernels
    //! \return   void
    //!
    void SetTK(
        uint32_t    *pTKs,
        uint32_t    uiTKsSize);

    //!
    //! \brief    Used by video processor to request the cached version of transcrypted and authenticated kernels
    //! \param    [out] **ppTKs
    //!           Cached version of transcrypted kernels
    //! \param    [out] *pTKsSize
    //!           Sized in bytes of the cached transcrypted kernels
    //! \param    [out] *pTKsUpdateCnt
    //!           The update count of the cached transcrypted kernels.
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    MOS_STATUS GetTK(
        uint32_t    **ppTKs,
        uint32_t    *pTKSize,
        uint32_t    *pTKsUpdateCnt);

    //!
    //! \brief    Determines whether or not render has access to PAVP content.
    //! \details  Render does not have access to PAVP content when PAVP Stout
    //!           or PAVP Isolated Decode is in use.
    //! \return   bool
    //!           True if render does not have access to PAVP (blocked), false otherwise
    //!
    bool RenderBlockedFromCp();

    //!
    //! \brief    This function is implemented in Windows.
    //! \details  Used by HalOcaInterfacet for cp parameters dump.
    //! \return   void*
    //!
    void *GetOcaDumper();

    //!
    //! \brief    This function is implemented in Windows.
    //! \details  Create Cp Oca dumper instance for adding
    //!           cp parameters.
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    MOS_STATUS CreateOcaDumper();

    //!
    //! \brief    Determines whether or not PAVP register can be read from non privileged command buffers.
    //! \details  If PAVP register is HW whitelisted then register could be non privileged read.
    //! \return   bool
    //!           True if PAVP register can be non privileged read, false otherwise
    //!
    bool CpRegisterAccessible();

private:
    //!
    //! \brief    handle surface handle and cptag cache
    //! \details  Since hwc could not ensure 1v1 map of surface id and bo handle, driver need
    //!           handle this. the case is hwc may create new surface by libva create surface,
    //!           while there is other surface in driver created for the same bo handle.
    //!           in order to detect teardown, driver need reuse cptag from old surface.
    //! \param    [in] handle
    //!           gem surface handle from kmd
    //! \param    [inout] pGmmResInfo
    //!           umd gmm resource info structure
    //! \return   bool
    //!           Return true if teardown detected, otherwise return false.
    //!
    bool UpdateCpTagCache(int32_t handle, PGMM_RESOURCE_INFO pGmmResInfo);

    //!
    //! \brief    get session instance from KMD, and flush cptag cache if need
    //! \return   none
    //!
    void PrepareCpTagCache(void);

    PMOS_INTERFACE               m_osInterface;
    CpContext                    m_cpContext;
    uint32_t                     m_flushInst;
    std::map<int32_t, uint32_t>  m_surfTags;
    std::map<void*, CpContext>   m_protectedgemctxmap;

    MEDIA_CLASS_DEFINE_END(MosCp)
};
MosCpInterface* Create_MosCp(void *osInterface);
void Delete_MosCp(MosCpInterface* pMosCpInterface);
#endif  // __MOS_OS_PAVP_SPECIFIC_H__