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
**    Define the OSK C Demo application
**
**  Notes:
**    1. Demonstrates an application using the OSK C Framework. It also serves
**       as the final app that is developed during the Code-As-You-Go(CAYG)
**       app development tutorial.
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide.
**    2. cFS Application Developer's Guide.
**
*/

#ifndef _osk_c_demo_app_
#define _osk_c_demo_app_

/*
** Includes
*/

#include "app_cfg.h"
#include "msglog.h"

/***********************/
/** Macro Definitions **/
/***********************/

/*
** Events
*/

#define OSK_C_DEMO_INIT_APP_EID    (OSK_C_DEMO_BASE_EID + 0)
#define OSK_C_DEMO_NOOP_EID        (OSK_C_DEMO_BASE_EID + 1)
#define OSK_C_DEMO_EXIT_EID        (OSK_C_DEMO_BASE_EID + 2)
#define OSK_C_DEMO_INVALID_MID_EID (OSK_C_DEMO_BASE_EID + 3)


/**********************/
/** Type Definitions **/
/**********************/


/******************************************************************************
** Command Packets
*/


/******************************************************************************
** Telemetry Packets
*/

typedef struct
{

   CFE_MSG_TelemetryHeader_t TlmHeader;
    
   /*
   ** CMDMGR Data
   */
   uint16  ValidCmdCnt;
   uint16  InvalidCmdCnt;
 
   /*
   ** CHILDMGR Data
   */
   uint16  ChildValidCmdCnt;
   uint16  ChildInvalidCmdCnt;
 
   /*
   ** Table Data 
   ** - Loaded with status from the last table action 
   */

   uint8   LastTblAction;
   uint8   LastTblActionStatus;

   
   /*
   ** MSGLOG Data
   */

   bool    MsgLogEna;
   bool    MsgPlaybkEna;

   uint16  MsgLogCnt;      
   char    MsgLogFilename[OS_MAX_PATH_LEN];


} OSK_C_DEMO_HkPkt_t;
#define OSK_C_DEMO_TLM_HK_LEN sizeof (OSK_C_DEMO_HkPkt_t)


/******************************************************************************
** OSK_C_DEMO_Class
*/
typedef struct
{

   /* 
   ** App Framework
   */ 
    
   INITBL_Class_t    IniTbl; 
   CFE_SB_PipeId_t   CmdPipe;
   CMDMGR_Class_t    CmdMgr;
   TBLMGR_Class_t    TblMgr;
   CHILDMGR_Class_t  ChildMgr;
   
   /*
   ** Command Packets
   */

   CMDMGR_NoParamCmdMsg_t MsgLogRunChildFuncCmd;
 
   /*
   ** Telemetry Packets
   */
   
   OSK_C_DEMO_HkPkt_t  HkPkt;
   
   /*
   ** OSK_C_DEMO State & Contained Objects
   */ 
           
   uint32           PerfId;
   CFE_SB_MsgId_t   CmdMid;
   CFE_SB_MsgId_t   ExecuteMid;
   CFE_SB_MsgId_t   SendHkMid;
   
   MSGLOG_Class_t  MsgLog;
   
} OSK_C_DEMO_Class_t;


/*******************/
/** Exported Data **/
/*******************/

extern OSK_C_DEMO_Class_t  OskCDemo;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: OSK_C_DEMO_AppMain
**
*/
void OSK_C_DEMO_AppMain(void);


/******************************************************************************
** Function: OSK_C_DEMO_NoOpCmd
**
*/
bool OSK_C_DEMO_NoOpCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: OSK_C_DEMO_ResetAppCmd
**
*/
bool OSK_C_DEMO_ResetAppCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


#endif /* _osk_c_demo_ */
