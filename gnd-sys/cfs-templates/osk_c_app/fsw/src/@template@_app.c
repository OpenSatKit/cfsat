/*
** Purpose: Implement the @Template@ application
**
** Notes:
**   1. See header notes. 
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

/*
** Includes
*/

#include <string.h>
#include "@template@_app.h"


/***********************/
/** Macro Definitions **/
/***********************/

/* Convenience macros */
#define  INITBL_OBJ    (&(@Template@.IniTbl))
#define  CMDMGR_OBJ    (&(@Template@.CmdMgr))
#define  TBLMGR_OBJ    (&(@Template@.TblMgr))
#define  EXOBJ_OBJ     (&(@Template@.ExObj))

/*******************************/
/** Local Function Prototypes **/
/*******************************/

static int32 InitApp(void);
static int32 ProcessCommands(void);
static void SendHousekeepingPkt(void);


/**********************/
/** File Global Data **/
/**********************/

/* 
** Must match DECLARE ENUM() declaration in app_cfg.h
** Defines "static INILIB_CfgEnum_t IniCfgEnum"
*/
DEFINE_ENUM(Config,APP_CONFIG)  


/*****************/
/** Global Data **/
/*****************/

@TEMPLATE@_Class_t  @Template@;


/******************************************************************************
** Function: @TEMPLATE@_AppMain
**
*/
void @TEMPLATE@_AppMain(void)
{

   uint32 RunStatus = CFE_ES_RunStatus_APP_ERROR;
   
   CFE_EVS_Register(NULL, 0, CFE_EVS_NO_FILTER);

   if (InitApp() == CFE_SUCCESS)      /* Performs initial CFE_ES_PerfLogEntry() call */
   {
      RunStatus = CFE_ES_RunStatus_APP_RUN; 
   }
   
   /*
   ** Main process loop
   */
   while (CFE_ES_RunLoop(&RunStatus))
   {
      
      RunStatus = ProcessCommands();  /* Pends indefinitely & manages CFE_ES_PerfLogEntry() calls */
      
   } /* End CFE_ES_RunLoop */

   CFE_ES_WriteToSysLog("@TEMPLATE@ App terminating, run status = 0x%08X\n", RunStatus);   /* Use SysLog, events may not be working */

   CFE_EVS_SendEvent(@TEMPLATE@_EXIT_EID, CFE_EVS_EventType_CRITICAL, "@TEMPLATE@ App terminating, run status = 0x%08X", RunStatus);

   CFE_ES_ExitApp(RunStatus);  /* Let cFE kill the task (and any child tasks) */

} /* End of @TEMPLATE@_AppMain() */


/******************************************************************************
** Function: @TEMPLATE@_NoOpCmd
**
*/
bool @TEMPLATE@_NoOpCmd(void* ObjDataPtr, const CFE_SB_Buffer_t *SbBufPtr)
{

   CFE_EVS_SendEvent (@TEMPLATE@_NOOP_EID, CFE_EVS_EventType_INFORMATION,
                      "No operation command received for @TEMPLATE@ App version %d.%d.%d",
                      @TEMPLATE@_MAJOR_VER, @TEMPLATE@_MINOR_VER, @TEMPLATE@_PLATFORM_REV);

   return true;


} /* End @TEMPLATE@_NoOpCmd() */


/******************************************************************************
** Function: @TEMPLATE@_ResetAppCmd
**
** Notes:
**   1. Framework objects require an object reference since they are
**      reentrant. Applications use the singleton pattern and store a
**      reference pointer to the object data during construction.
*/
bool @TEMPLATE@_ResetAppCmd(void* ObjDataPtr, const CFE_SB_Buffer_t *SbBufPtr)
{

   CMDMGR_ResetStatus(CMDMGR_OBJ);
   TBLMGR_ResetStatus(TBLMGR_OBJ);
   
   EXOBJ_ResetStatus();
	  
   return true;

} /* End @TEMPLATE@_ResetAppCmd() */


/******************************************************************************
** Function: SendHousekeepingPkt
**
*/
static void SendHousekeepingPkt(void)
{

   /* Good design practice in case app expands to more than one table */
   const TBLMGR_Tbl_t* LastTbl = TBLMGR_GetLastTblStatus(TBLMGR_OBJ);

   /*
   ** Framework Data
   */
   
   @Template@.HkPkt.ValidCmdCnt   = @Template@.CmdMgr.ValidCmdCnt;
   @Template@.HkPkt.InvalidCmdCnt = @Template@.CmdMgr.InvalidCmdCnt;
   
   /*
   ** Table Data 
   ** - Loaded with status from the last table action 
   */

   @Template@.HkPkt.LastTblAction       = LastTbl->LastAction;
   @Template@.HkPkt.LastTblActionStatus = LastTbl->LastActionStatus;

   
   /*
   ** Example Object Data
   */

   @Template@.HkPkt.ExObjCounterMode  = @Template@.ExObj.CounterMode;
   @Template@.HkPkt.ExObjCounterValue = @Template@.ExObj.CounterValue;
   

   CFE_SB_TimeStampMsg(CFE_MSG_PTR(@Template@.HkPkt.TlmHeader));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(@Template@.HkPkt.TlmHeader), true);

} /* End SendHousekeepingPkt() */


/******************************************************************************
** Function: InitApp
**
*/
static int32 InitApp(void)
{

   int32 Status = OSK_C_FW_CFS_ERROR;
   

   /*
   ** Initialize objects 
   */
   
   if (INITBL_Constructor(INITBL_OBJ, @TEMPLATE@_INI_FILENAME, &IniCfgEnum))
   {
   
      @Template@.PerfId  = INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_PERF_ID);
      CFE_ES_PerfLogEntry(@Template@.PerfId);

      @Template@.CmdMid     = CFE_SB_ValueToMsgId(@TEMPLATE@_CMD_MID);
      @Template@.ExecuteMid = CFE_SB_ValueToMsgId(@TEMPLATE@_EXE_MID);
      @Template@.SendHkMid  = CFE_SB_ValueToMsgId(@TEMPLATE@_SEND_HK_MID);
      
      Status = CFE_SUCCESS; 
  
   } /* End if INITBL Constructed */
  
   if (Status == CFE_SUCCESS)
   {

      /*
      ** Constuct app's contained objects
      */
            
      EXOBJ_Constructor(EXOBJ_OBJ, INITBL_OBJ);
      
      /*
      ** Initialize app level interfaces
      */
      
      CFE_SB_CreatePipe(&@Template@.CmdPipe, INITBL_GetIntConfig(INITBL_OBJ, CFG_CMD_PIPE_DEPTH), INITBL_GetStrConfig(INITBL_OBJ, CFG_CMD_PIPE_NAME));  
      CFE_SB_Subscribe(@Template@.CmdMid,     @Template@.CmdPipe);
      CFE_SB_Subscribe(@Template@.ExecuteMid, @Template@.CmdPipe);
      CFE_SB_Subscribe(@Template@.SendHkMid,  @Template@.CmdPipe);

      CMDMGR_Constructor(CMDMGR_OBJ);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, CMDMGR_NOOP_CMD_FC,  NULL, @TEMPLATE@_NoOpCmd,     0);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, CMDMGR_RESET_CMD_FC, NULL, @TEMPLATE@_ResetAppCmd, 0);
      
      CMDMGR_RegisterFunc(CMDMGR_OBJ, @TEMPLATE@_TBL_LOAD_CMD_FC, TBLMGR_OBJ, TBLMGR_LoadTblCmd, TBLMGR_LOAD_TBL_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, @TEMPLATE@_TBL_DUMP_CMD_FC, TBLMGR_OBJ, TBLMGR_DumpTblCmd, TBLMGR_DUMP_TBL_CMD_DATA_LEN);

      CMDMGR_RegisterFunc(CMDMGR_OBJ, EXOBJ_SET_MODE_CMD_FC, EXOBJ_OBJ, EXOBJ_SetModeCmd,EXOBJ_SET_MODE_CMD_DATA_LEN);


      /* Contained table object must be constructed prior to table registration because its table load function is called */
      TBLMGR_Constructor(TBLMGR_OBJ);
      TBLMGR_RegisterTblWithDef(TBLMGR_OBJ, EXOBJTBL_LoadCmd, EXOBJTBL_DumpCmd, INITBL_GetStrConfig(INITBL_OBJ, CFG_TBL_LOAD_FILE));

      /*
      ** Initialize app messages 
      */

      CFE_MSG_Init(CFE_MSG_PTR(@Template@.HkPkt.TlmHeader), CFE_SB_ValueToMsgId(@TEMPLATE@_HK_TLM_MID), @TEMPLATE@_TLM_HK_LEN);

      /*
      ** Application startup event message
      */
      CFE_EVS_SendEvent(@TEMPLATE@_INIT_APP_EID, CFE_EVS_EventType_INFORMATION,
                        "@TEMPLATE@ App Initialized. Version %d.%d.%d",
                        @TEMPLATE@_MAJOR_VER, @TEMPLATE@_MINOR_VER, @TEMPLATE@_PLATFORM_REV);

   } /* End if CHILDMGR constructed */
   
   return(Status);

} /* End of InitApp() */


/******************************************************************************
** Function: ProcessCommands
**
** 
*/
static int32 ProcessCommands(void)
{
   
   int32  RetStatus = CFE_ES_RunStatus_APP_RUN;
   int32  SysStatus;

   CFE_SB_Buffer_t* SbBufPtr;
   CFE_SB_MsgId_t   MsgId = CFE_SB_INVALID_MSG_ID;


   CFE_ES_PerfLogExit(@Template@.PerfId);
   SysStatus = CFE_SB_ReceiveBuffer(&SbBufPtr, @Template@.CmdPipe, CFE_SB_PEND_FOREVER);
   CFE_ES_PerfLogEntry(@Template@.PerfId);

   if (SysStatus == CFE_SUCCESS)
   {
      SysStatus = CFE_MSG_GetMsgId(&SbBufPtr->Msg, &MsgId);

      if (SysStatus == CFE_SUCCESS)
      {

         if (CFE_SB_MsgId_Equal(MsgId, @Template@.CmdMid))
         {
            CMDMGR_DispatchFunc(CMDMGR_OBJ, SbBufPtr);
         } 
         else if (CFE_SB_MsgId_Equal(MsgId, @Template@.ExecuteMid))
         {
            EXOBJ_Execute();
         }
         else if (CFE_SB_MsgId_Equal(MsgId, @Template@.SendHkMid))
         {   
            SendHousekeepingPkt();
         }
         else
         {   
            CFE_EVS_SendEvent(@TEMPLATE@_INVALID_MID_EID, CFE_EVS_EventType_ERROR,
                              "Received invalid command packet, MID = 0x%08X", 
                              CFE_SB_MsgIdToValue(MsgId));
         }

      } /* End if got message ID */
   } /* End if received buffer */
   else
   {
      RetStatus = CFE_ES_RunStatus_APP_ERROR;
   } 

   return RetStatus;
   
} /* End ProcessCommands() */
