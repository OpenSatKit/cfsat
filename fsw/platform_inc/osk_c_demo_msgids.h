/*
**  Copyright 2022 bitValence, Inc.
**  All Rights Reserved.
**
**  This program is free software; you can modify and/or redistribute it
**  under the terms of the GNU Affero General Public License
**  as published by the Free Software Foundation; version 3 with
**  attribution addendums as found in the LICENSE.txt
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU Affero General Public License for more details.
**
**  Purpose:
**    Define messages IDs for the OSK C Demo application
**
**  Notes:
**    1. The Topic IDs are defined in the mission's EDS specs
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide.
**    2. cFS Application Developer's Guide.
**
*/

#ifndef _osk_c_demo_platform_msgids_
#define _osk_c_demo_platform_msgids_

#include "cfe_msgids.h"

/* 
** Since demo app is delivered as part of baseline app suite define the development MIDs here
** so they'll be available for the create app tool.
*/
#define CFSAT_DEV_EXE_MID         CFE_PLATFORM_CMD_TOPICID_TO_MID(CFE_MISSION_CFSAT_DEV_EXE_TOPICID)
#define CFSAT_DEV_SEND_HK_MID     CFE_PLATFORM_CMD_TOPICID_TO_MID(CFE_MISSION_CFSAT_DEV_SEND_HK_TOPICID)
#define CFSAT_DEV_HK_TLM_MID      CFE_PLATFORM_TLM_TOPICID_TO_MID(CFE_MISSION_CFSAT_DEV_HK_TLM_TOPICID)


#define OSK_C_DEMO_CMD_MID         CFE_PLATFORM_CMD_TOPICID_TO_MID(CFE_MISSION_OSK_C_DEMO_CMD_TOPICID)
#define OSK_C_DEMO_EXE_MID         CFE_PLATFORM_CMD_TOPICID_TO_MID(CFE_MISSION_OSK_C_DEMO_EXE_TOPICID)
#define OSK_C_DEMO_SEND_HK_MID     CFE_PLATFORM_CMD_TOPICID_TO_MID(CFE_MISSION_OSK_C_DEMO_SEND_HK_TOPICID)
#define OSK_C_DEMO_HK_TLM_MID      CFE_PLATFORM_TLM_TOPICID_TO_MID(CFE_MISSION_OSK_C_DEMO_HK_TLM_TOPICID)
#define OSK_C_DEMO_PLAYBK_TLM_MID  CFE_PLATFORM_TLM_TOPICID_TO_MID(CFE_MISSION_OSK_C_DEMO_PLAYBK_TLM_TOPICID)

#endif /* _osk_c_demo_platform_msgids_ */
