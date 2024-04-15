/*
// Copyright (C) 2011-2022 Intel Corporation
//
// Licensed under the Apache License,Version 2.0(the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,software
// distributed under the License is distributed on an "AS IS" BASIS,CP_util
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
//!
//! \file     cp_user_settings_mgr_ext.h
//! \brief    Common CP user feature for proprietary use
//! \details  Common CP user feature for proprietary use across different platform
//!
#ifndef __CP_USER_SETTINGS_MGR_EXT_H__
#define __CP_USER_SETTINGS_MGR_EXT_H__

#include "cp_user_setting_define.h"
#include "media_user_settings_mgr.h"

class CpUserSettingsMgr : public MediaUserSettingsMgr
{
public:
    //!
    //! \brief    Init CP proprietary User feature key table
    //!
    CpUserSettingsMgr();

    //!
    //! \brief    Destroy CP proprietary User feature key table
    //!
    virtual ~CpUserSettingsMgr();
};

#endif // __CP_USER_SETTINGS_MGR_EXT_H__
