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
**    Implement the File Manager application
**
**  Notes:
**    1. See header notes
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide.
**    2. cFS Application Developer's Guide.
**
*/

/*
** Includes
*/

#include <string.h>
#include "filemgr_app.h"
#include "filemgr_eds_cc.h"
#include "filemgr_msgids.h"

/***********************/
/** Macro Definitions **/
/***********************/

/* Convenience macros */
#define  INITBL_OBJ   (&(FileMgr.IniTbl))
#define  CMDMGR_OBJ   (&(FileMgr.CmdMgr))
#define  TBLMGR_OBJ   (&(FileMgr.TblMgr))
#define  CHILDMGR_OBJ (&(FileMgr.ChildMgr))
#define  DIR_OBJ      (&(FileMgr.Dir))
#define  FILE_OBJ     (&(FileMgr.File))
#define  FILESYS_OBJ  (&(FileMgr.FileSys))


/*******************************/
/** Local Function Prototypes **/
/*******************************/

static int32 InitApp(void);
static int32 ProcessCommands(void);

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

FILEMGR_Class_t  FileMgr;


/******************************************************************************
** Function: FILEMGR_AppMain
**
*/
void FILEMGR_AppMain(void)
{

   uint32 RunStatus = CFE_ES_RunStatus_APP_ERROR;


   CFE_EVS_Register(NULL, 0, CFE_EVS_NO_FILTER);

   if (InitApp() == CFE_SUCCESS) /* Performs initial CFE_ES_PerfLogEntry() call */
   {
      RunStatus = CFE_ES_RunStatus_APP_RUN;  
   }
   
   /*
   ** Main process loop
   */
   while (CFE_ES_RunLoop(&RunStatus))
   {

      RunStatus = ProcessCommands(); /* Pends indefinitely & manages CFE_ES_PerfLogEntry() calls */

   } /* End CFE_ES_RunLoop */

   CFE_ES_WriteToSysLog("FILEMGR App terminating, err = 0x%08X\n", RunStatus);   /* Use SysLog, events may not be working */

   CFE_EVS_SendEvent(FILEMGR_EXIT_EID, CFE_EVS_EventType_CRITICAL, "FILEMGR App terminating, err = 0x%08X", RunStatus);

   CFE_ES_ExitApp(RunStatus);  /* Let cFE kill the task (and any child tasks) */

} /* End of FILEMGR_AppMain() */


/******************************************************************************
** Function: FILEMGR_NoOpCmd
**
*/

bool    FILEMGR_NoOpCmd(void* ObjDataPtr, const CFE_SB_Buffer_t *SbBufPtr)
{

   CFE_EVS_SendEvent (FILEMGR_NOOP_EID, CFE_EVS_EventType_INFORMATION,
                      "No operation command received for FILEMGR App version %d.%d.%d",
                      FILEMGR_MAJOR_VER, FILEMGR_MINOR_VER, FILEMGR_PLATFORM_REV);

   return true;


} /* End FILEMGR_NoOpCmd() */


/******************************************************************************
** Function: FILEMGR_ResetAppCmd
**
*/

bool FILEMGR_ResetAppCmd(void* ObjDataPtr, const CFE_SB_Buffer_t *SbBufPtr)
{

   CMDMGR_ResetStatus(CMDMGR_OBJ);
   TBLMGR_ResetStatus(TBLMGR_OBJ);
   CHILDMGR_ResetStatus(CHILDMGR_OBJ);
   
   DIR_ResetStatus();
   FILE_ResetStatus();
   FILESYS_ResetStatus();
	  
   return true;

} /* End FILEMGR_ResetAppCmd() */


/******************************************************************************
** Function: FILEMGR_SendHousekeepingPkt
**
*/
void FILEMGR_SendHousekeepingPkt(void)
{
   
   FILEMGR_HkTlm_Payload_t *HkTlmPayload = &FileMgr.HkPkt.Payload;
   
   HkTlmPayload->ValidCmdCnt   = FileMgr.CmdMgr.ValidCmdCnt;
   HkTlmPayload->InvalidCmdCnt = FileMgr.CmdMgr.InvalidCmdCnt;

   HkTlmPayload->NumOpenFiles  = FileUtil_GetOpenFileCount();

   HkTlmPayload->ChildValidCmdCnt   = FileMgr.ChildMgr.ValidCmdCnt;
   HkTlmPayload->ChildInvalidCmdCnt = FileMgr.ChildMgr.InvalidCmdCnt;
   HkTlmPayload->ChildWarningCmdCnt = FileMgr.File.CmdWarningCnt + FileMgr.Dir.CmdWarningCnt;
 
   HkTlmPayload->ChildQueueCnt   = FileMgr.ChildMgr.CmdQ.Count;
   HkTlmPayload->ChildCurrentCC  = FileMgr.ChildMgr.CurrCmdCode;
   HkTlmPayload->ChildPreviousCC = FileMgr.ChildMgr.PrevCmdCode;

   CFE_SB_TimeStampMsg(CFE_MSG_PTR(FileMgr.HkPkt.TelemetryHeader));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(FileMgr.HkPkt.TelemetryHeader), true);
   
} /* End FILEMGR_SendHousekeepingPkt() */


/******************************************************************************
** Function: InitApp
**
*/
static int32 InitApp(void)
{

   int32 Status = OSK_C_FW_CFS_ERROR;
   
   CHILDMGR_TaskInit_t ChildTaskInit;
   
   /*
   ** Initialize objects 
   */

   if (INITBL_Constructor(&FileMgr.IniTbl, FILEMGR_INI_FILENAME, &IniCfgEnum))
   {
   
      FileMgr.PerfId    = INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_MAIN_PERF_ID);
      FileMgr.CmdMid    = CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_CMD_MID));
      FileMgr.SendHkMid = CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_SEND_HK_MID));
      CFE_ES_PerfLogEntry(FileMgr.PerfId);

      /* Constructor sends error events */    
      ChildTaskInit.TaskName  = INITBL_GetStrConfig(INITBL_OBJ, CFG_CHILD_NAME);
      ChildTaskInit.StackSize = INITBL_GetIntConfig(INITBL_OBJ, CFG_CHILD_STACK_SIZE);
      ChildTaskInit.Priority  = INITBL_GetIntConfig(INITBL_OBJ, CFG_CHILD_PRIORITY);
      ChildTaskInit.PerfId    = INITBL_GetIntConfig(INITBL_OBJ, CHILD_TASK_PERF_ID);
      Status = CHILDMGR_Constructor(CHILDMGR_OBJ, 
                                    ChildMgr_TaskMainCmdDispatch,
                                    NULL, 
                                    &ChildTaskInit); 
  
   } /* End if INITBL Constructed */
  
   if (Status == CFE_SUCCESS)
   {

      DIR_Constructor(DIR_OBJ, &FileMgr.IniTbl);
      FILE_Constructor(FILE_OBJ, &FileMgr.IniTbl);
      FILESYS_Constructor(FILESYS_OBJ, &FileMgr.IniTbl);


      /*
      ** Initialize app level interfaces
      */
      
      CFE_SB_CreatePipe(&FileMgr.CmdPipe, INITBL_GetIntConfig(INITBL_OBJ, CFG_CMD_PIPE_DEPTH), INITBL_GetStrConfig(INITBL_OBJ, CFG_CMD_PIPE_NAME));  
      CFE_SB_Subscribe(FileMgr.CmdMid,    FileMgr.CmdPipe);
      CFE_SB_Subscribe(FileMgr.SendHkMid, FileMgr.CmdPipe);

      CMDMGR_Constructor(CMDMGR_OBJ);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, CMDMGR_NOOP_CMD_FC,   NULL, FILEMGR_NoOpCmd,     0);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, CMDMGR_RESET_CMD_FC,  NULL, FILEMGR_ResetAppCmd, 0);

      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILEMGR_CREATE_DIR_CC,          CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, sizeof(FILEMGR_CreateDir_Payload_t));
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILEMGR_DELETE_DIR_CC,          CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, sizeof(FILEMGR_DeleteDir_Payload_t));
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILEMGR_DELETE_ALL_DIR_CC,      CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, sizeof(FILEMGR_DeleteAllDir_Payload_t));
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILEMGR_SEND_DIR_LIST_TLM_CC,   CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, sizeof(FILEMGR_SendDirListTlm_Payload_t));
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILEMGR_SEND_DIR_TLM_CC,        CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, sizeof(FILEMGR_SendDirTlm_Payload_t));
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILEMGR_WRITE_DIR_LIST_FILE_CC, CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, sizeof(FILEMGR_WriteDirListFile_Payload_t));
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILEMGR_CREATE_DIR_CC,          DIR_OBJ, DIR_CreateCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILEMGR_DELETE_DIR_CC,          DIR_OBJ, DIR_DeleteCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILEMGR_DELETE_ALL_DIR_CC,      DIR_OBJ, DIR_DeleteAllCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILEMGR_SEND_DIR_LIST_TLM_CC,   DIR_OBJ, DIR_SendDirListTlmCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILEMGR_SEND_DIR_TLM_CC,        DIR_OBJ, DIR_SendDirTlmCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILEMGR_WRITE_DIR_LIST_FILE_CC, DIR_OBJ, DIR_WriteListFileCmd);

      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILEMGR_CONCATENATE_FILE_CC,     CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, sizeof(FILEMGR_ConcatenateFile_Payload_t));
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILEMGR_COPY_FILE_CC,            CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, sizeof(FILEMGR_CopyFile_Payload_t));
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILEMGR_DECOMPRESS_FILE_CC,      CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, sizeof(FILEMGR_DecompressFile_Payload_t));
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILEMGR_DELETE_FILE_CC,          CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, sizeof(FILEMGR_DeleteFile_Payload_t));
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILEMGR_MOVE_FILE_CC,            CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, sizeof(FILEMGR_MoveFile_Payload_t));
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILEMGR_RENAME_FILE_CC,          CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, sizeof(FILEMGR_RenameFile_Payload_t));
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILEMGR_SEND_FILE_INFO_TLM_CC,   CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, sizeof(FILEMGR_SendFileInfoTlm_Payload_t));
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILEMGR_SET_FILE_PERMISSIONS_CC, CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, sizeof(FILEMGR_SetFilePermissions_Payload_t));
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILEMGR_CONCATENATE_FILE_CC,     FILE_OBJ, FILE_ConcatenateCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILEMGR_COPY_FILE_CC,            FILE_OBJ, FILE_CopyCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILEMGR_DECOMPRESS_FILE_CC,      FILE_OBJ, FILE_DecompressCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILEMGR_DELETE_FILE_CC,          FILE_OBJ, FILE_DeleteCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILEMGR_MOVE_FILE_CC,            FILE_OBJ, FILE_MoveCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILEMGR_RENAME_FILE_CC,          FILE_OBJ, FILE_RenameCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILEMGR_SEND_FILE_INFO_TLM_CC,   FILE_OBJ, FILE_SendInfoTlmCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILEMGR_SET_FILE_PERMISSIONS_CC, FILE_OBJ, FILE_SetPermissionsCmd);

      /* 
      ** Alternative commands don't increment the main command counters. They do increment the child command counters which mimics
      ** the original FM app behavior, but I'm not sure that's desirable since the child counters are also used by ground ops.
      */
      CMDMGR_RegisterFuncAltCnt(CMDMGR_OBJ, FILEMGR_DELETE_FILE_ALT_CC, CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, sizeof(FILEMGR_DeleteFile_Payload_t));
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILEMGR_DELETE_FILE_ALT_CC, FILE_OBJ, FILE_DeleteCmd);
 
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILEMGR_SEND_OPEN_FILE_TLM_CC,     FILESYS_OBJ, FILESYS_SendOpenFileTlmCmd, PKTUTIL_NO_PARAM_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILEMGR_SEND_FILE_SYS_TBL_TLM_CC,  FILESYS_OBJ, FILESYS_SendTblTlmCmd,      PKTUTIL_NO_PARAM_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILEMGR_SET_FILE_SYS_TBL_STATE_CC, FILESYS_OBJ, FILESYS_SetTblStateCmd,     sizeof(FILEMGR_SetFileSysTblState_Payload_t));

      CFE_MSG_Init(CFE_MSG_PTR(FileMgr.HkPkt.TelemetryHeader), CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_HK_TLM_MID)), sizeof(FILEMGR_HkTlm_t));

      TBLMGR_Constructor(TBLMGR_OBJ);
   
      /*
      ** Application startup event message
      */
      CFE_EVS_SendEvent(FILEMGR_INIT_APP_EID, CFE_EVS_EventType_INFORMATION,
                        "FILEMGR App Initialized. Version %d.%d.%d",
                        FILEMGR_MAJOR_VER, FILEMGR_MINOR_VER, FILEMGR_PLATFORM_REV);
                     
     
   } /* End if CHILDMGR constructed */
   
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

   CFE_SB_Buffer_t *SbBufPtr;
   CFE_SB_MsgId_t   MsgId = CFE_SB_INVALID_MSG_ID;
   

   CFE_ES_PerfLogExit(FileMgr.PerfId);
   SysStatus = CFE_SB_ReceiveBuffer(&SbBufPtr, FileMgr.CmdPipe, CFE_SB_PEND_FOREVER);
   CFE_ES_PerfLogEntry(FileMgr.PerfId);

   if (SysStatus == CFE_SUCCESS) {
      
      SysStatus = CFE_MSG_GetMsgId(&SbBufPtr->Msg, &MsgId);
   
      if (SysStatus == OS_SUCCESS) {

         if (CFE_SB_MsgId_Equal(MsgId, FileMgr.CmdMid))
         {
            CMDMGR_DispatchFunc(CMDMGR_OBJ, SbBufPtr);
         } 
         else if (CFE_SB_MsgId_Equal(MsgId, FileMgr.SendHkMid))
         {  
            FILESYS_ManageTbl();
            FILEMGR_SendHousekeepingPkt();
         }
         else
         {
            CFE_EVS_SendEvent(FILEMGR_INVALID_MID_EID, CFE_EVS_EventType_ERROR,
                              "Received invalid command packet, MID = 0x%08X", 
                              CFE_SB_MsgIdToValue(MsgId));
         }

      }
      else {
         
         CFE_EVS_SendEvent(FILEMGR_INVALID_MID_EID, CFE_EVS_EventType_ERROR,
                           "Failed to retrieve message ID from the SB message, Status = %d", SysStatus);
      }
      
   } /* Valid SB receive */ 
   else {
   
         CFE_ES_WriteToSysLog("FILEMGR software bus error. Status = 0x%08X\n", SysStatus);   /* Use SysLog, events may not be working */
         RetStatus = CFE_ES_RunStatus_APP_ERROR;
   }  
      
   return RetStatus;
   
} /* ProcessCommands() */