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
**    Implement OpenSat Kit Scheduler application
**
**  Notes:
**    1. This is non-flight code so an attempt has been made to balance keeping
**       it simple while making it robust. Limiting the number of configuration
**       parameters and integration items (message IDs, perf IDs, etc) was
**       also taken into consideration.
**    2. Event message filters are not used since this is for test environments.
**       This may be reconsidered if event flooding ever becomes a problem.
**    3. Performance traces are not included.
**    4. Most functions are global to assist in unit testing
**    5. Functions I removed from original that need to be thought through:
**         SCH_ValidateMessageData()
**         SCH_ValidateScheduleData()
**         SCH_ProcessCommands()
**         SCH_TblInit()
**         InitEventFilters()
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide
**    2. cFS Application Developer's Guide
**
*/

/*
** Includes
*/

#include "kit_sch_app.h"


/***********************/
/** Macro Definitions **/
/***********************/

/* Convenience macros */
#define  INITBL_OBJ    (&(KitSch.IniTbl))
#define  CMDMGR_OBJ    (&(KitSch.CmdMgr)) 
#define  TBLMGR_OBJ    (&(KitSch.TblMgr))
#define  SCHEDULER_OBJ (&(KitSch.Scheduler))


/*******************************/
/** Local Function Prototypes **/
/*******************************/

static int32 InitApp(void);
static int32 ProcessCommands(void);
static void  SendHousekeepingPkt(void);


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

KIT_SCH_Class   KitSch;


/******************************************************************************
** Function: KIT_SCH_Main
**
*/
void KIT_SCH_AppMain(void)
{
	
   uint32 RunStatus = CFE_ES_RunStatus_APP_ERROR;
   
   CFE_EVS_Register(NULL, 0, CFE_EVS_NO_FILTER);

   /* 
   ** Load KIT_SCH towards the end in cfe_es_startup.scr (see file comments) to
   ** avoid startup pipe overflows. The local event log can be used to analyze
   ** the events during startup.   
   */
   if (InitApp() == CFE_SUCCESS)      /* Performs initial CFE_ES_PerfLogEntry() call */
   {
      CFE_ES_WaitForStartupSync(KitSch.StartupSyncTimeout);   
      if (SCHEDULER_StartTimers() == CFE_SUCCESS)
      {
         RunStatus = CFE_ES_RunStatus_APP_RUN;
      }
   } /* End if App initialized successfully */

   /*
   ** Main process loop
   */
   CFE_EVS_SendEvent(KIT_SCH_APP_DEBUG_EID, CFE_EVS_EventType_DEBUG,"KIT_SCH: About to enter loop\n");
   while (CFE_ES_RunLoop(&RunStatus))
   {
  
      if (!SCHEDULER_Execute())
      {
         RunStatus = CFE_ES_RunStatus_APP_ERROR;
      }

      RunStatus = ProcessCommands();
      
   } /* End CFE_ES_RunLoop */

   /* Write to system log in case events not working */

   CFE_ES_WriteToSysLog("KIT_SCH App terminating, err = 0x%08X\n", RunStatus);

   CFE_EVS_SendEvent(KIT_SCH_APP_EXIT_EID, CFE_EVS_EventType_CRITICAL, "KIT_SCH App: terminating, err = 0x%08X", RunStatus);

   CFE_ES_ExitApp(RunStatus);  /* Let cFE kill the task (and any child tasks) */

} /* End of KIT_SCH_Main() */


/******************************************************************************
** Function: KIT_SCH_NoOpCmd
**
*/
bool KIT_SCH_NoOpCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   CFE_EVS_SendEvent (KIT_SCH_APP_NOOP_EID, CFE_EVS_EventType_INFORMATION,
                      "Kit Scheduler (KIT_SCH) version %d.%d.%d received a no operation command",
                      KIT_SCH_MAJOR_VER, KIT_SCH_MINOR_VER, KIT_SCH_PLATFORM_REV);

   return true;

} /* End KIT_SCH_NoOpCmd() */


/******************************************************************************
** Function: KIT_SCH_ResetAppCmd
**
*/
bool KIT_SCH_ResetAppCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   CMDMGR_ResetStatus(CMDMGR_OBJ);
   TBLMGR_ResetStatus(TBLMGR_OBJ);

   SCHEDULER_ResetStatus();

   return true;

} /* End KIT_SCH_ResetAppCmd() */


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
   
   if (INITBL_Constructor(INITBL_OBJ, KIT_SCH_INI_FILENAME, &IniCfgEnum))
   {

      KitSch.CmdMid    = CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_KIT_SCH_CMD_TOPICID));
      KitSch.SendHkMid = CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_KIT_SCH_SEND_HK_TOPICID));
      
      KitSch.StartupSyncTimeout = INITBL_GetIntConfig(INITBL_OBJ, CFG_STARTUP_SYNC_TIMEOUT);
      
      SCHEDULER_Constructor(SCHEDULER_OBJ,INITBL_OBJ);
   
      Status = CFE_SUCCESS;
         
   } /* End if INITBL Constructed */
   
   /*
   ** Initialize application managers
   */

   if (Status == CFE_SUCCESS)
   {
   
      CFE_SB_CreatePipe(&KitSch.CmdPipe, INITBL_GetIntConfig(INITBL_OBJ, CFG_CMD_PIPE_DEPTH), INITBL_GetStrConfig(INITBL_OBJ, CFG_CMD_PIPE_NAME));
      CFE_SB_Subscribe(KitSch.CmdMid,    KitSch.CmdPipe);
      CFE_SB_Subscribe(KitSch.SendHkMid, KitSch.CmdPipe);

      CMDMGR_Constructor(CMDMGR_OBJ);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, CMDMGR_NOOP_CMD_FC,  NULL, KIT_SCH_NoOpCmd,     0);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, CMDMGR_RESET_CMD_FC, NULL, KIT_SCH_ResetAppCmd, 0);

      CMDMGR_RegisterFunc(CMDMGR_OBJ, KIT_SCH_LOAD_TBL_CMD_FC,      TBLMGR_OBJ, TBLMGR_LoadTblCmd, TBLMGR_LOAD_TBL_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, KIT_SCH_DUMP_TBL_CMD_FC,      TBLMGR_OBJ, TBLMGR_DumpTblCmd, TBLMGR_DUMP_TBL_CMD_DATA_LEN);

      CMDMGR_RegisterFunc(CMDMGR_OBJ, SCHEDULER_CFG_SCH_TBL_ENTRY_CMD_FC,  SCHEDULER_OBJ, SCHEDULER_ConfigSchEntryCmd, SCHEDULER_CFG_SCH_ENTRY_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, SCHEDULER_LOAD_SCH_TBL_ENTRY_CMD_FC, SCHEDULER_OBJ, SCHEDULER_LoadSchEntryCmd,   SCHEDULER_LOAD_SCH_ENTRY_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, SCHEDULER_SEND_SCH_TBL_ENTRY_CMD_FC, SCHEDULER_OBJ, SCHEDULER_SendSchEntryCmd,   SCHEDULER_SEND_SCH_ENTRY_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, SCHEDULER_LOAD_MSG_TBL_ENTRY_CMD_FC, SCHEDULER_OBJ, SCHEDULER_LoadMsgEntryCmd,   SCHEDULER_LOAD_MSG_ENTRY_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, SCHEDULER_SEND_MSG_TBL_ENTRY_CMD_FC, SCHEDULER_OBJ, SCHEDULER_SendMsgEntryCmd,   SCHEDULER_SEND_MSG_ENTRY_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, SCHEDULER_SEND_DIAG_TLM_CMD_FC,      SCHEDULER_OBJ, SCHEDULER_SendDiagTlmCmd,    SCHEDULER_SEND_DIAG_TLM_CMD_DATA_LEN);
    
      CFE_MSG_Init(CFE_MSG_PTR(KitSch.HkPkt.TlmHeader), CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_KIT_SCH_HK_TLM_TOPICID)), KIT_SCH_HK_TLM_LEN);

      CFE_EVS_SendEvent(KIT_SCH_INIT_DEBUG_EID, KIT_SCH_INIT_EVS_TYPE,"KIT_SCH_InitApp() Before TBLMGR calls");
      TBLMGR_Constructor(TBLMGR_OBJ);
      TBLMGR_RegisterTblWithDef(TBLMGR_OBJ, MSGTBL_LoadCmd, MSGTBL_DumpCmd, INITBL_GetStrConfig(INITBL_OBJ, CFG_MSG_TBL_LOAD_FILE));
      TBLMGR_RegisterTblWithDef(TBLMGR_OBJ, SCHTBL_LoadCmd, SCHTBL_DumpCmd, INITBL_GetStrConfig(INITBL_OBJ, CFG_SCH_TBL_LOAD_FILE));

      /*
      ** Application startup event message
      */
      Status = CFE_EVS_SendEvent(KIT_SCH_APP_INIT_EID, CFE_EVS_EventType_INFORMATION,
                               "KIT_SCH Initialized. Version %d.%d.%d",
                               KIT_SCH_MAJOR_VER, KIT_SCH_MINOR_VER, KIT_SCH_PLATFORM_REV);

      } /* End if init success */
      
      return(Status);

} /* End of InitApp() */


/******************************************************************************
** Function: ProcessCommands
**
*/
static int32 ProcessCommands(void)
{

   int32  RetStatus = CFE_ES_RunStatus_APP_RUN;
   int32  SysStatus;

   CFE_SB_Buffer_t* SbBufPtr;
   CFE_SB_MsgId_t   MsgId = CFE_SB_INVALID_MSG_ID;

   SysStatus = CFE_SB_ReceiveBuffer(&SbBufPtr, KitSch.CmdPipe, CFE_SB_POLL);

   if (SysStatus == CFE_SUCCESS)
   {
      SysStatus = CFE_MSG_GetMsgId(&SbBufPtr->Msg, &MsgId);
      
      if (SysStatus == CFE_SUCCESS)
      {

         if (CFE_SB_MsgId_Equal(MsgId, KitSch.CmdMid))
         {
            CMDMGR_DispatchFunc(CMDMGR_OBJ, &SbBufPtr->Msg);
         } 
         else if (CFE_SB_MsgId_Equal(MsgId, KitSch.SendHkMid))
         {   
            SendHousekeepingPkt();
         }
         else
         {   
            CFE_EVS_SendEvent(KIT_SCH_APP_MID_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Received invalid command packet, MID = 0x%08X", 
                              CFE_SB_MsgIdToValue(MsgId));
         }

      } /* End if got message ID */
   } /* End if received buffer */
   else
   {
      if (SysStatus == CFE_SB_PIPE_RD_ERR)
         RetStatus = CFE_ES_RunStatus_APP_ERROR;
   } 

   return RetStatus;

} /* End ProcessCommands() */


/******************************************************************************
** Function: SendHousekeepingPkt
**
*/
static void SendHousekeepingPkt(void)
{

   /*
   ** KIT_SCH Data
   */

   KitSch.HkPkt.ValidCmdCnt   = KitSch.CmdMgr.ValidCmdCnt;
   KitSch.HkPkt.InvalidCmdCnt = KitSch.CmdMgr.InvalidCmdCnt;

   /*
   ** TBLMGR Data
   */

   KitSch.HkPkt.MsgTblLastLoadStatus = KitSch.Scheduler.MsgTbl.LastLoadStatus;
   KitSch.HkPkt.MsgTblAttrErrCnt     = KitSch.Scheduler.MsgTbl.LastLoadCnt;
   
   KitSch.HkPkt.SchTblLastLoadStatus = KitSch.Scheduler.SchTbl.LastLoadStatus;
   KitSch.HkPkt.SchTblAttrErrCnt     = KitSch.Scheduler.SchTbl.LastLoadCnt;

   /*
   ** Scheduler Data
   ** - At a minimum every scheduler variable effected by a reset must be included
   ** - These have been rearranged to align data words
   */

   KitSch.HkPkt.SlotsProcessedCount          = KitSch.Scheduler.SlotsProcessedCount;
   KitSch.HkPkt.ScheduleActivitySuccessCount = KitSch.Scheduler.ScheduleActivitySuccessCount;
   KitSch.HkPkt.ScheduleActivityFailureCount = KitSch.Scheduler.ScheduleActivityFailureCount;
   KitSch.HkPkt.ValidMajorFrameCount         = KitSch.Scheduler.ValidMajorFrameCount;
   KitSch.HkPkt.MissedMajorFrameCount        = KitSch.Scheduler.MissedMajorFrameCount;
   KitSch.HkPkt.UnexpectedMajorFrameCount    = KitSch.Scheduler.UnexpectedMajorFrameCount;
   KitSch.HkPkt.TablePassCount               = KitSch.Scheduler.TablePassCount;
   KitSch.HkPkt.ConsecutiveNoisyFrameCounter = KitSch.Scheduler.ConsecutiveNoisyFrameCounter;
   KitSch.HkPkt.SkippedSlotsCount            = KitSch.Scheduler.SkippedSlotsCount;
   KitSch.HkPkt.MultipleSlotsCount           = KitSch.Scheduler.MultipleSlotsCount;
   KitSch.HkPkt.SameSlotCount                = KitSch.Scheduler.SameSlotCount;
   KitSch.HkPkt.SyncAttemptsLeft             = KitSch.Scheduler.SyncAttemptsLeft;
   KitSch.HkPkt.LastSyncMETSlot              = KitSch.Scheduler.LastSyncMETSlot;
   KitSch.HkPkt.IgnoreMajorFrame             = KitSch.Scheduler.IgnoreMajorFrame;
   KitSch.HkPkt.UnexpectedMajorFrame         = KitSch.Scheduler.UnexpectedMajorFrame;

   CFE_SB_TimeStampMsg(CFE_MSG_PTR(KitSch.HkPkt.TlmHeader));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(KitSch.HkPkt.TlmHeader), true);

} /* End SendHousekeepingPkt() */

