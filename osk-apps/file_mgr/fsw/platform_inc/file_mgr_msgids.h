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
**    Define messages IDs for the File Manager application
**
**  Notes:
**    1. The Topic IDs are defined in the mission's EDS specs
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide.
**    2. cFS Application Developer's Guide.
**
*/

#ifndef _file_mgr_platform_msgids_
#define _file_mgr_platform_msgids_

#include "cfe_msgids.h"


#define FILE_MGR_CMD_MID                CFE_PLATFORM_CMD_TOPICID_TO_MID(CFE_MISSION_FILE_MGR_CMD_TOPICID)
#define FILE_MGR_SEND_HK_MID            CFE_PLATFORM_CMD_TOPICID_TO_MID(CFE_MISSION_FILE_MGR_SEND_HK_TOPICID)
#define FILE_MGR_HK_TLM_MID             CFE_PLATFORM_TLM_TOPICID_TO_MID(CFE_MISSION_FILE_MGR_HK_TLM_TOPICID)
#define FILE_MGR_DIR_LIST_TLM_MID       CFE_PLATFORM_TLM_TOPICID_TO_MID(CFE_MISSION_FILE_MGR_DIR_LIST_TLM_TOPICID)
#define FILE_MGR_FILE_INFO_TLM_MID      CFE_PLATFORM_TLM_TOPICID_TO_MID(CFE_MISSION_FILE_MGR_FILE_INFO_TLM_TOPICID)
#define FILE_MGR_OPEN_FILE_TLM_MID      CFE_PLATFORM_TLM_TOPICID_TO_MID(CFE_MISSION_FILE_MGR_OPEN_FILE_TLM_TOPICID)
#define FILE_MGR_FILE_SYS_TBL_TLM_MID   CFE_PLATFORM_TLM_TOPICID_TO_MID(CFE_MISSION_FILE_MGR_FILE_SYS_TBL_TLM_TOPICID)

#endif /* _file_mgr_platform_msgids_ */
