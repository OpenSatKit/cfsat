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
**    Implement the MSGLOG_Class methods
**
**  Notes:
**    1. The log/playback functionality is for demonstration purposes and 
**       the logic is kept simple so users can focus on learning developing 
**       apps using the OSK C Framework.
**    2. Logging and playback can't be enabled at the same time. If a command
**       to start a playback is received when logging is in progress, the
**       logging will be stopped and a playback will be started. The same
**       occurs in reverse when a playback is in progress and a command to 
**       start a message log is received. Neither case is considered an error.
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide.
**    2. cFS Application Developer's Guide.
**
*/

/*
** Include Files:
*/

#include <string.h>
#include "msglog.h"


/***********************/
/** Macro Definitions **/
/***********************/

/* Convenience macro */
#define TBL_OBJ (&(MsgLog->Tbl))  


/*******************************/
/** Local Function Prototypes **/
/*******************************/

static void CreateFilename(void);
static void LogMessages(void);
static void PlaybkMessages(void);
static void StopLog(void);
static void StopPlaybk(void);


/**********************/
/** Global File Data **/
/**********************/

static MSGLOG_Class_t*  MsgLog = NULL;



/******************************************************************************
** Function: MSGLOG_Constructor
**
*/
void MSGLOG_Constructor(MSGLOG_Class_t*  MsgLogPtr, INITBL_Class_t* IniTbl)
{
 
   MsgLog = MsgLogPtr;

   CFE_PSP_MemSet((void*)MsgLog, 0, sizeof(MSGLOG_Class_t));
 
   MsgLog->IniTbl = IniTbl;
   
   CFE_SB_CreatePipe(&MsgLog->MsgPipe, INITBL_GetIntConfig(MsgLog->IniTbl, CFG_MSGLOG_PIPE_DEPTH), 
                     INITBL_GetStrConfig(MsgLog->IniTbl, CFG_MSGLOG_PIPE_NAME));  

   CFE_MSG_Init(CFE_MSG_PTR(MsgLog->PlaybkPkt.TlmHeader), 
                CFE_SB_ValueToMsgId(INITBL_GetIntConfig(MsgLog->IniTbl,CFG_OSK_C_DEMO_PLAYBK_TLM_TOPICID)),
                sizeof(MSGLOG_PlaybkPkt_t));
   
   MSGLOGTBL_Constructor(TBL_OBJ, IniTbl);

} /* End MSGLOG_Constructor */


/******************************************************************************
** Function:  MSGLOG_ResetStatus
**
*/
void MSGLOG_ResetStatus()
{
 
   /* Nothing to do */
   
} /* End MSGLOG_ResetStatus() */


/******************************************************************************
** Function: MSGLOG_RunChildFuncCmd
**
** Notes:
**   1. This is not intended to be a ground command. This function provides a
**      mechanism for the parent app to periodically call a child task function.
**
*/
bool MSGLOG_RunChildFuncCmd(void* DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   
   CFE_EVS_SendEvent (MSGLOG_PERIODIC_CMD_EID, CFE_EVS_EventType_DEBUG, 
                      "Run child function command called");

   if (MsgLog->LogEna)
   {
   
      LogMessages();
   
   } /* End if log in progress */
   
   if (MsgLog->PlaybkEna)
   {
      
      MsgLog->PlaybkDelay++;
      if (MsgLog->PlaybkDelay > MsgLog->Tbl.Data.PlaybkDelay)
      {
         
         MsgLog->PlaybkDelay = 0;
         PlaybkMessages();
      
      }
   
   } /* End if playback enabled */
   
   return true;

} /* End MSGLOG_RunChildFuncCmd() */


/******************************************************************************
** Function: MSGLOG_StartLogCmd
**
** Notes:
**   1. See file prologue for logging/playback logic.
*/
bool MSGLOG_StartLogCmd(void* DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   bool   RetStatus = false;
   int32  SysStatus;
   os_err_name_t OsErrStr;
   const MSGLOG_StartLogCmdMsg_Payload_t* StartLog = CMDMGR_PAYLOAD_PTR(MsgPtr, MSGLOG_StartLogCmdMsg_t);
      
   if (MsgLog->LogEna)
   {
      StopLog();
   }
   
   if (MsgLog->PlaybkEna)
   {
      StopPlaybk();
   }

   SysStatus = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(StartLog->MsgId), MsgLog->MsgPipe);

   if (SysStatus == CFE_SUCCESS)
   {
      
      MsgLog->LogCnt = 0;
      MsgLog->MsgId  = StartLog->MsgId;
      CreateFilename();

      SysStatus = OS_OpenCreate(&MsgLog->FileHandle, MsgLog->Filename, OS_FILE_FLAG_CREATE, OS_READ_WRITE);

      if (SysStatus == OS_SUCCESS)
      {
      
         RetStatus      = true;
         MsgLog->LogEna = true;

         CFE_EVS_SendEvent (MSGLOG_START_LOG_CMD_EID,
                            CFE_EVS_EventType_INFORMATION, "Created new log file %s with a maximum of %d entries",
                            MsgLog->Filename, MsgLog->Tbl.Data.File.EntryCnt);

      }
      else
      {
         
         CFE_SB_Unsubscribe(CFE_SB_ValueToMsgId(MsgLog->MsgId), MsgLog->MsgPipe);
         
         OS_GetErrorName(SysStatus, &OsErrStr);
         CFE_EVS_SendEvent (MSGLOG_START_LOG_CMD_EID, CFE_EVS_EventType_ERROR, 
                            "Start message log rejected. Error creating new log file %s. Status = %s",
                            MsgLog->Filename, OsErrStr);         
      
      }

   
   } /* End if SB subscribe */
   else {
   
      CFE_EVS_SendEvent (MSGLOG_START_LOG_CMD_EID, CFE_EVS_EventType_ERROR,
                         "Start message log rejected. SB message 0x%04X subscription failed, Status = 0x%04X",
                         StartLog->MsgId, SysStatus);
   
   }
   
   return RetStatus;

} /* End MSGLOG_StartLogCmd() */


/******************************************************************************
** Function: MSGLOG_StartPlaybkCmd
**
** Notes:
**   1. See file prologue for logging/playback logic.
*/
bool MSGLOG_StartPlaybkCmd(void* DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   bool   RetStatus = false;
   int32  SysStatus;

   os_err_name_t OsErrStr;
   FileUtil_FileInfo_t FileInfo;
   

   if (MsgLog->LogEna)
   {
      StopLog();
   }

   if (!MsgLog->PlaybkEna)
   {
      
      if (MsgLog->LogCnt > 0)
      {

         FileInfo = FileUtil_GetFileInfo(MsgLog->Filename, OS_MAX_PATH_LEN, false);

         if (FILEUTIL_FILE_EXISTS(FileInfo.State))
         {

            SysStatus = OS_OpenCreate(&MsgLog->FileHandle, MsgLog->Filename, OS_FILE_FLAG_NONE, OS_READ_ONLY);

            if (SysStatus == OS_SUCCESS)
            {
               RetStatus           = true;
               MsgLog->PlaybkEna   = true;
               MsgLog->PlaybkCnt   = 0;
               MsgLog->PlaybkDelay = 0;
            
               CFE_EVS_SendEvent (MSGLOG_START_PLAYBK_CMD_EID, CFE_EVS_EventType_INFORMATION,
                                  "Playback file %s started with a %d cycle delay between updates",
                                  MsgLog->Filename, MsgLog->Tbl.Data.PlaybkDelay);
            }
            else
            {
               OS_GetErrorName(SysStatus, &OsErrStr);
               CFE_EVS_SendEvent (MSGLOG_START_PLAYBK_CMD_EID, CFE_EVS_EventType_ERROR,
                                  "Start playback failed. Error opening file %s. Status = %s",
                                 MsgLog->Filename, OsErrStr);
            }
         
         } /* End if file exists */
         else
         {
            CFE_EVS_SendEvent (MSGLOG_START_PLAYBK_CMD_EID, CFE_EVS_EventType_ERROR,
                              "Start playback failed. Message log file does not exist");
         }
      
      } /* MsgLog->LogCnt > 0 */
      else
      {
         
         CFE_EVS_SendEvent (MSGLOG_START_PLAYBK_CMD_EID, CFE_EVS_EventType_ERROR,
                            "Start playback failed. Message log count is zero");
      }
   } /* End if playback not in progress */ 
   else
   {

      CFE_EVS_SendEvent (MSGLOG_START_PLAYBK_CMD_EID, CFE_EVS_EventType_ERROR,
                         "Start playback ignored. Playback already in progress");

   }
   
   return RetStatus;
   
} /* End MSGLOG_StartPlaybkCmd() */


/******************************************************************************
** Function: MSGLOG_StopLogCmd
**
*/
bool MSGLOG_StopLogCmd(void* DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   if (MsgLog->LogEna)
   {
      StopLog();
   }
   else
   {
      CFE_EVS_SendEvent (MSGLOG_STOP_LOG_CMD_EID, CFE_EVS_EventType_INFORMATION,
                         "Stop log command received with no log in progress");
   }
   
   return true;
   
} /* End MSGLOG_StopLogCmd() */


/******************************************************************************
** Function: MSGLOG_StopPlaybkCmd
**
*/
bool MSGLOG_StopPlaybkCmd(void* DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   if (MsgLog->PlaybkEna)
   {
      StopPlaybk();
   }
   else
   {
      CFE_EVS_SendEvent (MSGLOG_STOP_LOG_CMD_EID, CFE_EVS_EventType_INFORMATION,
                         "Stop playback command received with no playback in progress");
   }
   
   return true;
   
} /* End MSGLOG_StopPlaybkCmd() */


/******************************************************************************
** Function: CreateFilename
**
** Create a filename using the table-defined base path/filename, current
** message ID, and the table-defined extension. 
**
** Notes:
**   1. No string buffer error checking performed
*/
static void CreateFilename(void)
{
   
   int  i;
   char MsgIdStr[16];

   CFE_EVS_SendEvent (MSGLOG_START_LOG_CMD_EID, CFE_EVS_EventType_DEBUG,
                      "CreateFilename using table values: %s,%s,%d",
                      MsgLog->Tbl.Data.File.PathBaseName, 
                      MsgLog->Tbl.Data.File.Extension,
                      MsgLog->Tbl.Data.File.EntryCnt); 

   sprintf(MsgIdStr,"%04X",MsgLog->MsgId);

   strcpy (MsgLog->Filename, MsgLog->Tbl.Data.File.PathBaseName);

   i = strlen(MsgLog->Filename);  /* Starting position for message ID */
   strcat (&(MsgLog->Filename[i]), MsgIdStr);
   
   i = strlen(MsgLog->Filename);  /* Starting position for extension */
   strcat (&(MsgLog->Filename[i]), MsgLog->Tbl.Data.File.Extension);
   

} /* End CreateFilename() */


/******************************************************************************
** Functions: LogMessages
**
** Read messages from SB, convert ApId and Sequence count to hex text, and 
** write them to a log file.
*/
static void LogMessages(void)
{
   
   int32  SysStatus;
   char   MsgLogText[MSGLOG_TEXT_LEN];
   
   CFE_SB_Buffer_t         *SbBufPtr;
   CFE_SB_MsgId_t           MsgId;
   CFE_MSG_SequenceCount_t  SeqCnt;      
         
   
   
   do
   {
   
      SysStatus = CFE_SB_ReceiveBuffer(&SbBufPtr, MsgLog->MsgPipe, CFE_SB_POLL);

      if (SysStatus == CFE_SUCCESS)
      {
         
         CFE_MSG_GetMsgId(&SbBufPtr->Msg, &MsgId);
         CFE_MSG_GetSequenceCount(&SbBufPtr->Msg, &SeqCnt);

         sprintf(MsgLogText,"0x%04X 0x%04X \n", CFE_SB_MsgIdToValue(MsgId), SeqCnt);
         SysStatus = OS_write(MsgLog->FileHandle, MsgLogText, MSGLOG_TEXT_LEN);

         MsgLog->LogCnt++;
         if (MsgLog->LogCnt >= MsgLog->Tbl.Data.File.EntryCnt)
         {   
            StopLog();  
         }
      
      } /* End SB read */
      
   } while((SysStatus == CFE_SUCCESS) && MsgLog->LogEna);
      

} /* End LogMessages() */


/******************************************************************************
** Functions: PlaybkMessages
**
** Copy one message into playback telemetry packet and send the telemetry
** packet. 
**
** Notes:
**   1. Uses log counter to determine when to wrap around to start of file.
**      Startlog command ensures at least one message is in the log file.
**   2. FileUtil_ReadLine() could have been used for reading text file lines
**      but decided on fixed sized binary reads 
**
*/
static void PlaybkMessages(void)
{
   
   int32    SysStatus;

   if (MsgLog->PlaybkCnt < MsgLog->LogCnt)
   {
   
      SysStatus = OS_read(MsgLog->FileHandle, MsgLog->PlaybkPkt.MsgText, MSGLOG_TEXT_LEN);

      if (SysStatus == MSGLOG_TEXT_LEN)
      {
      
         MsgLog->PlaybkPkt.LogFileEntry = MsgLog->PlaybkCnt;
        
         CFE_SB_TimeStampMsg(CFE_MSG_PTR(MsgLog->PlaybkPkt.TlmHeader));
         CFE_SB_TransmitMsg(CFE_MSG_PTR(MsgLog->PlaybkPkt.TlmHeader), true);

         MsgLog->PlaybkCnt++;
         if (MsgLog->PlaybkCnt >= MsgLog->LogCnt)
         {
         
            MsgLog->PlaybkCnt = 0;
            OS_lseek(MsgLog->FileHandle, 0, OS_SEEK_SET);
         
         }
      }
      else
      {
         StopPlaybk();   
      }
      
   }
   else
   {
      StopPlaybk();   
   }

} /* End PlaybkMessages() */


/******************************************************************************
** Function: StopLog
**
** Notes:
**   1. Assumes caller checked if log was in progress
*/
static void StopLog(void)
{
   
   OS_close(MsgLog->FileHandle);
   CFE_SB_Unsubscribe(CFE_SB_ValueToMsgId(MsgLog->MsgId), MsgLog->MsgPipe);
   MsgLog->LogEna = false;
   
   CFE_EVS_SendEvent (MSGLOG_STOP_LOG_CMD_EID, CFE_EVS_EventType_INFORMATION,
                      "Logging stopped. Closed log file %s with %d entries", 
                      MsgLog->Filename, MsgLog->LogCnt);

}/* End StopLog() */


/******************************************************************************
** Function: StopPlaybk
**
** Notes:
**   1. Assumes caller checked if playback was in progress. 
**   2. Clears playback state data
*/
static void StopPlaybk(void)
{
   
   OS_close(MsgLog->FileHandle);
   
   MsgLog->PlaybkEna = false;
   MsgLog->PlaybkPkt.LogFileEntry = 0;
   memset(MsgLog->PlaybkPkt.MsgText, '\0', MSGLOG_TEXT_LEN);

   CFE_EVS_SendEvent (MSGLOG_STOP_PLAYBK_CMD_EID, CFE_EVS_EventType_INFORMATION,
                      "Playback stopped. Closed log file %s", MsgLog->Filename);

}/* End StopPlaybk() */
