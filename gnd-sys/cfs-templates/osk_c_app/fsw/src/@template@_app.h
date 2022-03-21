/*
** Purpose: Define the @Template@ application
**
** Notes:
**   1. Generated from the Hello World app template using the 
**      OSK C Application Framework 
**
** References:
**   1. OpenSatKit Object-based Application Developer's Guide.
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
#ifndef _@template@_app_
#define _@template@_app_

/*
** Includes
*/

#include "app_cfg.h"
#include "exobj.h"

/***********************/
/** Macro Definitions **/
/***********************/

/*
** Events
*/

#define @TEMPLATE@_INIT_APP_EID    (@TEMPLATE@_BASE_EID + 0)
#define @TEMPLATE@_NOOP_EID        (@TEMPLATE@_BASE_EID + 1)
#define @TEMPLATE@_EXIT_EID        (@TEMPLATE@_BASE_EID + 2)
#define @TEMPLATE@_INVALID_MID_EID (@TEMPLATE@_BASE_EID + 3)


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
   ** Table Data 
   ** - Loaded with status from the last table action 
   */

   uint8   LastTblAction;
   uint8   LastTblActionStatus;

   
   /*
   ** EXOBJ Data
   */

   uint16  ExObjCounterMode;      
   uint16  ExObjCounterValue;


} @TEMPLATE@_HkPkt_t;
#define @TEMPLATE@_TLM_HK_LEN sizeof (@TEMPLATE@_HkPkt_t)


/******************************************************************************
** @TEMPLATE@_Class
*/
typedef struct
{

   /* 
   ** App Framework
   */ 
    
   INITBL_Class_t    IniTbl; 
   CMDMGR_Class_t    CmdMgr;
   TBLMGR_Class_t    TblMgr;
   
   /*
   ** Command Packets
   */


 
   /*
   ** Telemetry Packets
   */
   
   @TEMPLATE@_HkPkt_t  HkPkt;
   
   /*
   ** @TEMPLATE@ State & Contained Objects
   */ 
           
   CFE_SB_PipeId_t  CmdPipe;
   CFE_SB_MsgId_t   CmdMid;
   CFE_SB_MsgId_t   ExecuteMid;
   CFE_SB_MsgId_t   SendHkMid;
   uint32           PerfId;

   EXOBJ_Class_t  ExObj;
   
} @TEMPLATE@_Class_t;


/*******************/
/** Exported Data **/
/*******************/

extern @TEMPLATE@_Class_t  @Template@;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: @TEMPLATE@_AppMain
**
*/
void @TEMPLATE@_AppMain(void);


/******************************************************************************
** Function: @TEMPLATE@_NoOpCmd
**
*/
bool @TEMPLATE@_NoOpCmd(void* ObjDataPtr, const CFE_SB_Buffer_t *SbBufPtr);


/******************************************************************************
** Function: @TEMPLATE@_ResetAppCmd
**
*/
bool @TEMPLATE@_ResetAppCmd(void* ObjDataPtr, const CFE_SB_Buffer_t *SbBufPtr);


#endif /* _@template@_ */
