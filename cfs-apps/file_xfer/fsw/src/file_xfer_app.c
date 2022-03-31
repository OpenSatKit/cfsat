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
**    Implement the File Transfer application
**
**  Notes:
**    1. See file_xfer_app.h for details.
**    2. TODO: Add performance monitors
**    3. TODO: Verify applying the running CRC is equal to a one-time end CRC 
**    4. TODO: Add event filters
**    5. TODO: Analyze separating FOTP and FITP into separate telemetry that is only sent during a transfer
**    6. TODO: Can there be more than one OS_TaskInstallDeleteHandler?
**    7. TODO: Put limit on failed data segment attempts? 
**    8. TODO: Replace FOTP loop with telemetry output flow control
**    9. TODO: Verify Fotp->NextDataSegmentId managed correctly
**   10. TODO: The SendFileTransferTlm() function is inconsistent with whether
**       TODO: the caller loads the telemetry packet prior to calling the function. 

**  References:
**    1. OpenSatKit Object-based Application Developer's Guide.
**    2. cFS Application Developer's Guide.
**
*/

/*
** Includes
*/

#include <string.h>
#include "file_xfer_app.h"


/***********************/
/** Macro Definitions **/
/***********************/

/* Convenience macros */
#define  INITBL_OBJ  (&(FileXfer.IniTbl))
#define  CMDMGR_OBJ  (&(FileXfer.CmdMgr))  
#define  FITP_OBJ    (&(FileXfer.Fitp))
#define  FOTP_OBJ    (&(FileXfer.Fotp))


/*******************************/
/** Local Function Prototypes **/
/*******************************/

static int32 InitApp(void);
static int32 ProcessCommands(void);
static void SendHousekeepingPkt(void);

/* 
** Must match DECLARE ENUM() declaration in app_cfg.h
** Defines "static INILIB_CfgEnum_t IniCfgEnum"
*/
DEFINE_ENUM(Config,APP_CONFIG)

/**********************/
/** File Global Data **/
/**********************/

static CFE_EVS_BinFilter_t  EventFilters[] =
{

   /* Event ID                       Mask                 */
   {FITP_DATA_SEGMENT_CMD_ERR_EID,   CFE_EVS_FIRST_TWO_STOP},
   {FOTP_SEND_FILE_TRANSFER_ERR_EID, CFE_EVS_FIRST_TWO_STOP}

};

/*****************/
/** Global Data **/
/*****************/

FILE_XFER_Class_t  FileXfer;


/******************************************************************************
** Function: FILE_XFER_AppMain
**
*/
void FILE_XFER_AppMain(void)
{

   uint32 RunStatus = CFE_ES_RunStatus_APP_ERROR;


   if (InitApp() == CFE_SUCCESS) /* Performs initial CFE_ES_PerfLogEntry() call */
   {
      RunStatus = CFE_ES_RunStatus_APP_RUN;
   }

   /*
   ** Main process loop
   */
   while (CFE_ES_RunLoop(&RunStatus))
   {
      
      RunStatus = ProcessCommands();
      
   } /* End CFE_ES_RunLoop */


   /* Write to system log in case events not working */

   CFE_ES_WriteToSysLog("FILE_XFER App terminating, err = 0x%08X\n", RunStatus);

   CFE_EVS_SendEvent(FILE_XFER_EXIT_EID, CFE_EVS_EventType_CRITICAL, "FILE_XFER App terminating, err = 0x%08X", RunStatus);

   CFE_ES_ExitApp(RunStatus);  /* Let cFE kill the task (and any child tasks) */

} /* End of FILE_XFER_AppMain() */


/******************************************************************************
** Function: FILE_XFER_NoOpCmd
**
*/

bool FILE_XFER_NoOpCmd(void* ObjDataPtr, const CFE_SB_Buffer_t *SbBufPtr)
{

   CFE_EVS_SendEvent (FILE_XFER_NOOP_EID, CFE_EVS_EventType_INFORMATION,
                      "No operation command received for FILE_XFER App version %d.%d.%d",
                      FILE_XFER_MAJOR_VER, FILE_XFER_MINOR_VER, FILE_XFER_PLATFORM_REV);

   return true;


} /* End FILE_XFER_NoOpCmd() */


/******************************************************************************
** Function: FILE_XFER_ResetAppCmd
**
*/

bool FILE_XFER_ResetAppCmd(void* ObjDataPtr, const CFE_SB_Buffer_t *SbBufPtr)
{

   CFE_EVS_ResetFilter (FITP_DATA_SEGMENT_CMD_ERR_EID);
   CFE_EVS_ResetFilter (FOTP_SEND_FILE_TRANSFER_ERR_EID);
   
   FITP_ResetStatus();
   FOTP_ResetStatus();

   CMDMGR_ResetStatus(CMDMGR_OBJ);

   return true;

} /* End FILE_XFER_ResetAppCmd() */


/******************************************************************************
** Function: SendHousekeepingPkt
**
** Notes:
**   1. At a minimum all FileIn/FileOut variables effected by a reset must be
**      included in HL telemetry
*/
static void SendHousekeepingPkt(void)
{

   /*
   ** FILE_XFER Application Data
   */

   FileXfer.HkPkt.ValidCmdCnt   = FileXfer.CmdMgr.ValidCmdCnt;
   FileXfer.HkPkt.InvalidCmdCnt = FileXfer.CmdMgr.InvalidCmdCnt;

   /*
   ** FITP Data
   */
   
   FileXfer.HkPkt.Fitp.FileTransferCnt     = FileXfer.Fitp.FileTransferCnt;
   FileXfer.HkPkt.Fitp.FileTransferActive  = FileXfer.Fitp.FileTransferActive; 
   FileXfer.HkPkt.Fitp.LastDataSegmentId   = FileXfer.Fitp.LastDataSegmentId;
   FileXfer.HkPkt.Fitp.DataSegmentErrCnt   = FileXfer.Fitp.DataSegmentErrCnt;             
   FileXfer.HkPkt.Fitp.FileTransferByteCnt = FileXfer.Fitp.FileTransferByteCnt;
   FileXfer.HkPkt.Fitp.FileRunningCrc      = FileXfer.Fitp.FileRunningCrc;
  
   strncpy(FileXfer.HkPkt.Fitp.DestFilename, FileXfer.Fitp.DestFilename, FITP_FILENAME_LEN);

   /*
   ** FOTP Data
   */
   
   FileXfer.HkPkt.Fotp.FileTransferCnt     = FileXfer.Fotp.FileTransferCnt;             
   FileXfer.HkPkt.Fotp.FileTransferState   = FileXfer.Fotp.FileTransferState; 
   FileXfer.HkPkt.Fotp.PausedTransferState = FileXfer.Fotp.PausedFileTransferState;
   FileXfer.HkPkt.Fotp.PrevSegmentFailed   = FileXfer.Fotp.PrevSendDataSegmentFailed;
   
   FileXfer.HkPkt.Fotp.FileTranferByteCnt  = FileXfer.Fotp.FileTransferByteCnt;
   FileXfer.HkPkt.Fotp.FileRunningCrc      = FileXfer.Fotp.FileRunningCrc;
   
   FileXfer.HkPkt.Fotp.DataTransferLen     = FileXfer.Fotp.DataTransferLen;
   FileXfer.HkPkt.Fotp.FileLen             = FileXfer.Fotp.FileLen;
   FileXfer.HkPkt.Fotp.FileByteOffset      = FileXfer.Fotp.FileByteOffset;
   FileXfer.HkPkt.Fotp.DataSegmentLen      = FileXfer.Fotp.DataSegmentLen;
   FileXfer.HkPkt.Fotp.DataSegmentOffset   = FileXfer.Fotp.DataSegmentOffset;
   FileXfer.HkPkt.Fotp.NextDataSegmentId   = FileXfer.Fotp.NextDataSegmentId;

   strncpy(FileXfer.HkPkt.Fotp.SrcFilename, FileXfer.Fotp.SrcFilename, FOTP_FILENAME_LEN);

   CFE_SB_TimeStampMsg(CFE_MSG_PTR(FileXfer.HkPkt.TlmHeader));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(FileXfer.HkPkt.TlmHeader), true);

} /* End SendHousekeepingPkt() */


/******************************************************************************
** Function: InitApp
**
*/
static int32 InitApp(void)
{

   int32 Status = CFE_SEVERITY_ERROR;

   CFE_PSP_MemSet((void*)&FileXfer, 0, sizeof(FILE_XFER_Class_t));

   Status = CFE_EVS_Register(EventFilters,sizeof(EventFilters)/sizeof(CFE_EVS_BinFilter_t),
                             CFE_EVS_EventFilter_BINARY);

   if (Status != CFE_SUCCESS)
   {
      CFE_ES_WriteToSysLog("Error registering for EVS services, status = 0x%08X", Status);
   }
 
   /*
   ** Initialize contained objects
   */
   
   if (INITBL_Constructor(INITBL_OBJ, FILE_XFER_INI_FILENAME, &IniCfgEnum))
   {
      
      FileXfer.PerfId     = INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_PERF_ID);
      FileXfer.CmdMid     = CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_CMD_MID));
      FileXfer.SendHkMid  = CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_SEND_HK_MID));
      FileXfer.ExecuteMid = CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_EXECUTE_MID));

      FITP_Constructor(FITP_OBJ);
      FOTP_Constructor(FOTP_OBJ, INITBL_OBJ);
      
      Status = CFE_SUCCESS;
      
   } /* End if INITBL Constructed */
  
   if (Status == CFE_SUCCESS)
   {

      Status = CFE_SB_CreatePipe(&FileXfer.CmdPipe,
                                 INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_CMD_PIPE_DEPTH),
                                 INITBL_GetStrConfig(INITBL_OBJ, CFG_APP_CMD_PIPE_NAME)); 

      if (Status == CFE_SUCCESS) 
      {

         /* Minimal chance of subcribe errors so keep logic simple */
         Status = CFE_SB_Subscribe(FileXfer.CmdMid, FileXfer.CmdPipe);
         if (Status == CFE_SUCCESS) 
         {
            Status = CFE_SB_Subscribe(FileXfer.ExecuteMid, FileXfer.CmdPipe);
            if (Status == CFE_SUCCESS) 
            {
               Status = CFE_SB_Subscribe(FileXfer.SendHkMid, FileXfer.CmdPipe);
            }
         }       
         if (Status != CFE_SUCCESS) 
         {
            CFE_EVS_SendEvent(FILE_XFER_INIT_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Error subscribing to messages on pipe %s. SB Status = 0x%08X",
                              INITBL_GetStrConfig(INITBL_OBJ, CFG_APP_CMD_PIPE_NAME), Status);
         }
      } /* End if create pipe */
      else
      {

         CFE_EVS_SendEvent(FILE_XFER_INIT_ERR_EID, CFE_EVS_EventType_ERROR,
                           "Error creating SB Command Pipe %s with depth %d. SB Status = 0x%08X",
                           INITBL_GetStrConfig(INITBL_OBJ, CFG_APP_CMD_PIPE_NAME), 
                           INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_CMD_PIPE_DEPTH), Status);
      
      } /* End if create pipe failed */

      if (Status == CFE_SUCCESS)
      {         
         CMDMGR_Constructor(CMDMGR_OBJ);
         CMDMGR_RegisterFunc(CMDMGR_OBJ, CMDMGR_NOOP_CMD_FC,   NULL, FILE_XFER_NoOpCmd,     0);
         CMDMGR_RegisterFunc(CMDMGR_OBJ, CMDMGR_RESET_CMD_FC,  NULL, FILE_XFER_ResetAppCmd, 0);
         
         CMDMGR_RegisterFunc(CMDMGR_OBJ, FITP_START_TRANSFER_CMD_FC,  FITP_OBJ, FITP_StartTransferCmd,  sizeof(FILE_XFER_FitpStartTransfer_Payload_t));
         CMDMGR_RegisterFunc(CMDMGR_OBJ, FITP_DATA_SEGMENT_CMD_FC,    FITP_OBJ, FITP_DataSegmentCmd,    sizeof(FILE_XFER_FitpDataSegment_Payload_t));
         CMDMGR_RegisterFunc(CMDMGR_OBJ, FITP_FINISH_TRANSFER_CMD_FC, FITP_OBJ, FITP_FinishTransferCmd, sizeof(FILE_XFER_FitpFinishTransfer_Payload_t));
         CMDMGR_RegisterFunc(CMDMGR_OBJ, FITP_CANCEL_TRANSFER_CMD_FC, FITP_OBJ, FITP_CancelTransferCmd, 0);

         CMDMGR_RegisterFunc(CMDMGR_OBJ, FOTP_START_TRANSFER_CMD_FC,  FOTP_OBJ, FOTP_StartTransferCmd,  sizeof(FILE_XFER_FotpStartTransfer_Payload_t));
         CMDMGR_RegisterFunc(CMDMGR_OBJ, FOTP_CANCEL_TRANSFER_CMD_FC, FOTP_OBJ, FOTP_CancelTransferCmd, 0);
         CMDMGR_RegisterFunc(CMDMGR_OBJ, FOTP_PAUSE_TRANSFER_CMD_FC,  FOTP_OBJ, FOTP_PauseTransferCmd,  0);
         CMDMGR_RegisterFunc(CMDMGR_OBJ, FOTP_RESUME_TRANSFER_CMD_FC, FOTP_OBJ, FOTP_ResumeTransferCmd, 0);

         CFE_MSG_Init(CFE_MSG_PTR(FileXfer.HkPkt.TlmHeader), CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_HK_TLM_MID)), sizeof(FILE_XFER_HkPkt_t));

         /*
         ** Application startup event message
         */
         Status = CFE_EVS_SendEvent(FILE_XFER_INIT_EID, CFE_EVS_EventType_INFORMATION,
                                    "FILE_XFER App Initialized. Version %d.%d.%d",
                                    FILE_XFER_MAJOR_VER, FILE_XFER_MINOR_VER, FILE_XFER_PLATFORM_REV);
      } /* End if CFE_SUCCESS */
      
   } /* End if init success */

   return Status;

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
   

   CFE_ES_PerfLogExit(FileXfer.PerfId);
   SysStatus = CFE_SB_ReceiveBuffer(&SbBufPtr, FileXfer.CmdPipe, CFE_SB_PEND_FOREVER);
   CFE_ES_PerfLogEntry(FileXfer.PerfId);

   if (SysStatus == CFE_SUCCESS)
   {
      
      SysStatus = CFE_MSG_GetMsgId(&SbBufPtr->Msg, &MsgId);
   
      if (SysStatus == CFE_SUCCESS)
      {
  
         if (CFE_SB_MsgId_Equal(MsgId, FileXfer.CmdMid)) 
         {
            
            CMDMGR_DispatchFunc(CMDMGR_OBJ, SbBufPtr);
         
         } 
         else if (CFE_SB_MsgId_Equal(MsgId, FileXfer.ExecuteMid))
         {

            FOTP_Execute();

         }
         else if (CFE_SB_MsgId_Equal(MsgId, FileXfer.SendHkMid))
         {

            SendHousekeepingPkt();
            
         }
         else {
            
            CFE_EVS_SendEvent(FILE_XFER_INVALID_MID_EID, CFE_EVS_EventType_ERROR,
                              "Received invalid command packet, MID = 0x%08X",
                              CFE_SB_MsgIdToValue(MsgId));
         } 

      }
      else {
         
         CFE_EVS_SendEvent(FILE_XFER_INVALID_MID_EID, CFE_EVS_EventType_ERROR,
                           "CFE couldn't retrieve message ID from the message, Status = %d", SysStatus);
      }
      
   } /* Valid SB receive */ 
   else {
   
         CFE_ES_WriteToSysLog("FILE_XFER software bus error. Status = 0x%08X\n", SysStatus);   /* Use SysLog, events may not be working */
         RetStatus = CFE_ES_RunStatus_APP_ERROR;
   }  
      
   return RetStatus;

} /* End ProcessCommands() */
