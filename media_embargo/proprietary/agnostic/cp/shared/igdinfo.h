/*========================== begin_copyright_notice ============================

INTEL CONFIDENTIAL

Copyright (C) 2018-2021 Intel Corporation

This software and the related documents are Intel copyrighted materials,
and your use of them is governed by the express license under which they were
provided to you ("License"). Unless the License provides otherwise,
you may not use, modify, copy, publish, distribute, disclose or transmit this
software or the related documents without Intel's prior written permission.

This software and the related documents are provided as is, with no express or
implied warranties, other than those that are expressly stated in the License.

============================= end_copyright_notice ===========================*/

#ifndef _GFX_INFO_H_
#define _GFX_INFO_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __TMTRY_DLL__
#define TMTRY_API __declspec(dllexport)
#else
#define TMTRY_API
#endif

//!
//! \brief    Component IDs.
//! \details  Do not change these values! You can add more component id below each group.
//!
typedef enum _GFXINFO_COMP_ID
{
    // KMD components
    GFXINFO_COMPID_KMD = 1,              // Do not change these values!!! Put new value after this block.
    GFXINFO_COMPID_KMD_GMM,
    GFXINFO_COMPID_PWRCON,
    GFXINFO_COMPID_MINIPORT,
    GFXINFO_COMPID_SECURITY,
    GFXINFO_COMPID_KMD_CP,

    // UMD components
    GFXINFO_COMPID_UMD = 101,            // Do not change these values!!! Put new value after this block.
    GFXINFO_COMPID_UMD_GMM,
    GFXINFO_COMPID_UMD_MEDIA_CODEC,
    GFXINFO_COMPID_UMD_MEDIA_VP,
    GFXINFO_COMPID_UMD_MEDIA_CP,

    // Others
    GFXINFO_COMPID_GCP = 201,            // Do not change these values!!! Put new value after this block.
    GFXINFO_COMPID_IGN = 202,            // Telemetry component id for IGCC Next

    GFXINFO_COMPID_MAX,

} GFXINFO_COMP_ID;

//!
//! \brief Parameter type
//!
typedef enum _GFXINFO_PARAM_TYPE
{
    GFXINFO_PTYPE_ANSISTRING = 1,   // win:AnsiString
    GFXINFO_PTYPE_INT8,             // win:Int8
    GFXINFO_PTYPE_UINT8,            // win:UInt8
    GFXINFO_PTYPE_INT16,            // win:Int16
    GFXINFO_PTYPE_UINT16,           // win:UInt16
    GFXINFO_PTYPE_INT32,            // win:Int32
    GFXINFO_PTYPE_UINT32,           // win:UInt32
    GFXINFO_PTYPE_INT64,            // win:Int64
    GFXINFO_PTYPE_UINT64,           // win:UInt64
    GFXINFO_PTYPE_BOOL,             // win:Boolean
    GFXINFO_PTYPE_GUID,             // win:GUID

    GFXINFO_PTYPE_FLOAT = 101,      // win:Float
    GFXINFO_PTYPE_DOUBLE,           // win:Double

    GFXINFO_PTYPE_MAX

} GFXINFO_PARAM_TYPE;

typedef void (*PFN_GFXINFO_RTERR)(uint8_t ver, uint16_t compId, uint16_t FtrId, uint32_t ErrorCode, uint8_t num, ...);
typedef void (*PFN_GFXINFO_RTERR_VARARGS)(uint8_t ver, uint16_t compId, uint16_t FtrId, uint32_t ErrorCode, uint8_t num, va_list args);
typedef void (*PFN_GFXINFO_PARAMS)(uint8_t ver, uint16_t compId, uint16_t tmtryID, uint8_t num, ...);
typedef void (*PFN_GFXINFO_VARARGS)(uint8_t ver, uint16_t compId, uint32_t tmtryID, uint8_t num, va_list args);

//!
//! \brief    GfxInfo_RTErr
//! \details  Custom telemetry trace to report runtime errors detected by each component.
//!           The xml content will be
//!             <I N="ErrorCode">ErrorCode in hex format</I>
//! \param    [in] ver
//!           Version
//! \param    [in] compId
//!           Component ID defined in GFXINFO_COMP_ID
//! \param    [in] FtrId
//!           Feature ID, an unique identifier for each component.
//! \param    [in] ErrorCode
//!           Error code that will be recorded.
//! \param    [in] num_of_triples
//!           Number of triples (name, type, value) to be compose as an <I N='name'>value</I> XML element
//! \param    [in] ...
//!           Triples (name, type, value), for example
//!             int8_t i = 3;
//!             "Name1", GFXINFO_PTYPE_UINT8, &i /* Need to pass variable address here */
//!             "Name2", GFXINFO_PTYPE_ANSISTRING, "string value"
//!             etc...
//! \return   void
//!
TMTRY_API void GfxInfo_RTErr(uint8_t ver, uint16_t compId, uint16_t FtrId, uint32_t ErrorCode, uint8_t num_of_triples, ...);

//!
//! \brief    GfxInfo_RTErr_Varargs
//! \details  va_list version of GfxInfo_RTErr.
//!           Usually used by API of each component if component do not use GfxInfo_RTErr directly.
//! \param    [in] ver
//!           Version
//! \param    [in] compId
//!           Component ID defined in GFXINFO_COMP_ID
//! \param    [in] FtrId
//!           Feature ID, an unique identifier for each component.
//! \param    [in] ErrorCode
//!           Error code that will be recorded.
//! \param    [in] num_of_triples
//!           Number of triples (name, type, value) to be compose as an <I N='name'>value</I> XML element
//! \param    [in] args
//!           va_list args
//! \return   void
//!
TMTRY_API void GfxInfo_RTErr_Varargs(uint8_t ver, uint16_t compId, uint16_t FtrId, uint32_t ErrorCode, uint8_t num_of_triples, va_list args);

//!
//! \brief    GfxInfo_Params
//! \details  A helper function to help to compose telemetry xml string
//!           The xml content will be
//!             <I N="Name1">value of i</I>
//!             <I N="Name2">string value</I>
//!             etc...
//!           Each pair of (ver, compId, tmtryID) will have the same xml content format
//! \param    [in] ver
//!           Version
//! \param    [in] compId
//!           Component ID defined in GFXINFO_COMP_ID
//! \param    [in] tmtryID
//!           Telemetry ID, an unique identifier for each component.
//! \param    [in] num_of_triples
//!           Number of triples (name, type, value) to be compose as an <I N='name'>value</I> XML element
//! \param    [in] ...
//!           Triples (name, type, value), for example
//!             int8_t i = 3;
//!             "Name1", GFXINFO_PTYPE_UINT8, &i /* Need to pass variable address here */
//!             "Name2", GFXINFO_PTYPE_ANSISTRING, "string value"
//!             etc...
//! \return   void
//!
TMTRY_API void GfxInfo_Params(uint8_t ver, uint16_t compId, uint32_t tmtryID, uint8_t num_of_triples, ...);

//!
//! \brief    GfxInfo_Vararg
//! \details  va_list version of GfxInfo_Params.
//!           Usually used by API of each component if component do not use GfxInfo_Params directly.
//! \param    [in] ver
//!           Version
//! \param    [in] compId
//!           Component ID defined in GFXINFO_COMP_ID
//! \param    [in] tmtryID
//!           Telemetry ID, an unique identifier for each component.
//! \param    [in] num_of_triples
//!           Number of triples (name, type, value) to be compose as an <I N='name'>value</I> XML element
//! \param    [in] args
//!           va_list args
//! \return   void
//!
TMTRY_API void GfxInfo_Varargs(uint8_t ver, uint16_t compId, uint32_t tmtryID, uint8_t num_of_triples, va_list args);

#ifdef __cplusplus
}
#endif

#endif //_GFX_INFO_H_
