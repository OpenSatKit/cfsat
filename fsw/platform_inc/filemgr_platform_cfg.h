/*
**  Copyright 2022 Open STEMware Foundation
**  All Rights Reserved.
**
**  This program is free software; you can modify and/or redistribute it under
**  the terms of the GNU Affero General Public License as published by the Free
**  Software Foundation; version 3 with attribution addendums as found in the
**  LICENSE.txt
**
**  This program is distributed in the hope that it will be useful, but WITHOUT
**  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
**  FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more
**  details.
**  
**  This program may also be used under the terms of a commercial or enterprise
**  edition license of cFSAT if purchased from the copyright holder.
**
**  Purpose:
**    Define platform scope configurations for the File Manager application
**
**  Notes:
**    1. This is part of a refactoring prototype. The definitions in this file 
**    2. These definitions should be minimal and only contain parameters that
**       need to be configurable and that must be defined at compile time.  
**       Use app_cfg.h for compile-time parameters that don't need to be
**       configured when an app is deployed and the JSON initialization
**       file for parameters that can be defined at runtime.
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide.
**    2. cFS Application Developer's Guide.
**
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
