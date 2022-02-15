/*
** Purpose: Define platform configurations for the OSK File Manager application
**
** Notes:
**   1. This is part of a refactoring prototype. The definitions in this file 
**   2. These definitions should be minimal and only contain parameters that
**      need to be configurable and that must be defined at compile time.  
**      Use app_cfg.h for compile-time parameters that don't need to be
**      configured when an app is deployed and the JSON initialization
**      file for parameters that can be defined at runtime.
**
** References:
**   1. OpenSatKit Object-based Application Developer's Guide and the
**      osk_c_demo app that illustrates best practices with comments.  
**   2. cFS Application Developer's Guide.
**
**   Written by David McComas, licensed under the Apache License, Version 2.0
**   (the "License"); you may not use this file except in compliance with the
**   License. You may obtain a copy of the License at
**
**      http://www.apache.org/licenses/LICENSE-2.0
**
**   Unless required by applicable law or agreed to in writing, software
**   distributed under the License is distributed on an "AS IS" BASIS,
**   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**   See the License for the specific language governing permissions and
**   limitations under the License.
*/

#ifndef _filemgr_platform_cfg_
#define _filemgr_platform_cfg_

/*
** Includes
*/

#include "filemgr_mission_cfg.h"

/******************************************************************************
** Platform Deployment Configurations
*/

#define FILEMGR_PLATFORM_REV   0
#define FILEMGR_INI_FILENAME   "/cf/filemgr_ini.json"


/******************************************************************************
** These are frustrating. They're only needed statically because of the table
** decsriptor build process. 
*/

#define FILEMGR_APP_CFE_NAME   "FILEMGR"
#define FILEMGR_TBL_CFE_NAME   "FileSysTbl"

/******************************************************************************
** These will be in a spec file and the toolchain will create these
** definitions.
*/

#define FILEMGR_DIR_LIST_PKT_ENTRIES     20
//TODO: Remove after EDS finalized: #define FILEMGR_FILESYS_TBL_VOL_CNT       8
#define FILEMGR_TASK_FILE_BLOCK_SIZE   2048  /* Chunk of file to work with for one iteration of a task like computing a CRC */

#endif /* _filemgr_platform_cfg_ */
