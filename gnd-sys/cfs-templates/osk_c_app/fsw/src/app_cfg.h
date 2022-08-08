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
**    Define configurations for the @Template@ application
**
**  Notes:
**   1. These configurations should have an application scope and define
**      parameters that shouldn't need to change across deployments. If
**      a change is made to this file or any other app source file during
**      a deployment then the definition of the @TEMPLATE@_REV
**      macro in @template@_platform_cfg.h should be updated.
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide
**    2. cFS Application Developer's Guide
**
*/
#ifndef _app_cfg_
#define _app_cfg_

/*
** Includes
*/

#include "osk_c_fw.h"
#include "@template@_platform_cfg.h"
#include "@template@_eds_typedefs.h"


/******************************************************************************
** Versions
**
** 1.0 - Initial version, compatible with cFE Caelum
*/

#define  @TEMPLATE@_MAJOR_VER   1
#define  @TEMPLATE@_MINOR_VER   0


/******************************************************************************
** Init JSON file declarations. The following steps show how to define and
** use initialization parameters defined in the JSON ini file. Users don't 
** need to know the structures created by the macros but they are shown for
** completeness. The app's command pipe definitions are used as an example. 
**
** 1. Define configuration parameter names
**
**    #define CFG_CMD_PIPE_NAME   CMD_PIPE_NAME
**    #define CFG_CMD_PIPE_DEPTH  CMD_PIPE_DEPTH
**
** 2. Add the parameter to the APP_CONFIG(XX) macro using the name as defined
**    in step 1
**
**    #define APP_CONFIG(XX) \
**       XX(CMD_PIPE_NAME,char*) \
**       XX(CMD_PIPE_DEPTH,uint32) \
**
** 3. Define the parameterin the JSON ini file's "config" object using the
**    same parameter as defined in step 1
**
**    "config": {
**       "CMD_PIPE_NAME":  "@TEMPLATE@_CMD",
**       "CMD_PIPE_DEPTH": 5,
** 
** 4. Access the parameteres in your code 
**    
**    INITBL_GetStrConfig(INITBL_OBJ, CFG_CMD_PIPE_NAME)
**    INITBL_GetIntConfig(INITBL_OBJ, CFG_CMD_PIPE_DEPTH)
**
** The following declarations are created using the APP_CONFIG(XX) and 
** XX(name,type) macros:
** 
**    typedef enum {
**       CMD_PIPE_DEPTH,
**       CMD_PIPE_NAME
**    } INITBL_ConfigEnum;
**    
**    typedef struct {
**       CMD_PIPE_DEPTH,
**       CMD_PIPE_NAME
**    } INITBL_ConfigStruct;
**
**    const char *GetConfigStr(value);
**    ConfigEnum GetConfigVal(const char *str);
**
*/

#define CFG_APP_CFE_NAME        APP_CFE_NAME
#define CFG_APP_PERF_ID         APP_PERF_ID

#define CFG_APP_CMD_PIPE_NAME   APP_CMD_PIPE_NAME
#define CFG_APP_CMD_PIPE_DEPTH  APP_CMD_PIPE_DEPTH

#define CFG_OSK_DEV_CMD_TOPICID      OSK_DEV_CMD_TOPICID
#define CFG_OSK_DEV_EXE_TOPICID      OSK_DEV_EXE_TOPICID
#define CFG_OSK_DEV_SEND_HK_TOPICID  OSK_DEV_SEND_HK_TOPICID
#define CFG_OSK_DEV_HK_TLM_TOPICID   OSK_DEV_HK_TLM_TOPICID

#define CFG_TBL_LOAD_FILE       TBL_LOAD_FILE
#define CFG_TBL_DUMP_FILE       TBL_DUMP_FILE


#define APP_CONFIG(XX) \
   XX(APP_CFE_NAME,char*) \
   XX(APP_PERF_ID,uint32) \
   XX(APP_CMD_PIPE_NAME,char*) \
   XX(APP_CMD_PIPE_DEPTH,uint32) \
   XX(OSK_DEV_CMD_TOPICID,uint32) \
   XX(OSK_DEV_EXE_TOPICID,uint32) \
   XX(OSK_DEV_SEND_HK_TOPICID,uint32) \
   XX(OSK_DEV_HK_TLM_TOPICID,uint32) \
   XX(TBL_LOAD_FILE,char*) \
   XX(TBL_DUMP_FILE,char*) \

DECLARE_ENUM(Config,APP_CONFIG)


/******************************************************************************
** Event Macros
**
** Define the base event message IDs used by each object/component used by the
** application. There are no automated checks to ensure an ID range is not
** exceeded so it is the developer's responsibility to verify the ranges. 
*/

#define @TEMPLATE@_BASE_EID  (OSK_C_FW_APP_BASE_EID +  0)
#define EXOBJ_BASE_EID       (OSK_C_FW_APP_BASE_EID + 20)
#define EXOBJTBL_BASE_EID    (OSK_C_FW_APP_BASE_EID + 40)


/******************************************************************************
** Example Object Table Macros
*/

#define EXOBJTBL_JSON_MAX_OBJ          10
#define EXOBJTBL_JSON_FILE_MAX_CHAR  2000 


#endif /* _app_cfg_ */
