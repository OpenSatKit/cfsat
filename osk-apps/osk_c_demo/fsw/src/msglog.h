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
**    Manage logging message header fields to a text file and playing them back
**    in telemetry
**
**  Notes:
**    1. This demo app serves as the final result of a Code-As-You-Go(CAYG) 
**       series of self-guided exercises. 
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide.
**    2. cFS Application Developer's Guide.
**
*/

#ifndef _msglog_
#define _msglog_

/*
** Includes
*/

#include "app_cfg.h"
#include "msglogtbl.h"


/***********************/
/** Macro Definitions **/
/***********************/

/* 
** Even number of bytes since hex char array used in tlm pkt definition
*/
#define MSGLOG_TEXT_LEN  16  

/*
** Event Message IDs
*/

#define MSGLOG_PERIODIC_CMD_EID     (MSGLOG_BASE_EID + 0)
#define MSGLOG_START_LOG_CMD_EID    (MSGLOG_BASE_EID + 1)
#define MSGLOG_STOP_LOG_CMD_EID     (MSGLOG_BASE_EID + 2)
#define MSGLOG_START_PLAYBK_CMD_EID (MSGLOG_BASE_EID + 3)
#define MSGLOG_STOP_PLAYBK_CMD_EID  (MSGLOG_BASE_EID + 4)


/**********************/
/** Type Definitions **/
/**********************/


/******************************************************************************
** Command Packets
** 
** - Use osk_c_fw's utility to define parameterless command lengths
**
*/

typedef struct
{

   uint16  MsgId;

} MSGLOG_StartLogCmdMsg_Payload_t;

typedef struct
{

   CFE_MSG_CommandHeader_t          CmdHeader;
   MSGLOG_StartLogCmdMsg_Payload_t  Payload;

} MSGLOG_StartLogCmdMsg_t;
#define MSGLOG_START_LOG_CMD_DATA_LEN     (sizeof(MSGLOG_StartLogCmdMsg_t) - sizeof(CFE_MSG_CommandHeader_t))
#define MSGLOG_STOP_LOG_CMD_DATA_LEN      PKTUTIL_NO_PARAM_CMD_DATA_LEN

#define MSGLOG_START_PLAYBK_CMD_DATA_LEN  PKTUTIL_NO_PARAM_CMD_DATA_LEN
#define MSGLOG_STOP_PLAYBK_CMD_DATA_LEN   PKTUTIL_NO_PARAM_CMD_DATA_LEN

#define MSGLOG_RUN_CHILD_CMD_LEN          PKTUTIL_NO_PARAM_CMD_LEN
#define MSGLOG_RUN_CHILD_CMD_DATA_LEN     PKTUTIL_NO_PARAM_CMD_DATA_LEN

/******************************************************************************
** Telmetery Packets
*/

/* 
** Playback packet that is sent while a playback is active. One packet header
** entry sent in each packet. 
*/

typedef struct
{

   CFE_MSG_TelemetryHeader_t TlmHeader;
   uint16  LogFileEntry;                  /* Log file entry being telemetered  */
   char    MsgText[MSGLOG_TEXT_LEN];      /* ID & sequence count text          */

} MSGLOG_PlaybkPkt_t;


/******************************************************************************
** MSGLOG_Class
*/

typedef struct
{

   /*
   ** App Framework References
   */
   
   INITBL_Class_t*  IniTbl;
   CFE_SB_PipeId_t  MsgPipe;
   
   /*
   ** Telemetry Packets
   */
   
   MSGLOG_PlaybkPkt_t  PlaybkPkt;

   /*
   ** Class State Data
   */

   bool    LogEna;
   uint16  LogCnt;
   
   bool    PlaybkEna;
   uint16  PlaybkCnt;
   uint16  PlaybkDelay;
   
   uint16     MsgId;
   osal_id_t  FileHandle;
   char       Filename[OS_MAX_PATH_LEN];
    
   /*
   ** Contained Objects
   */

   MSGLOGTBL_Class_t  Tbl;
   
} MSGLOG_Class_t;



/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: MSGLOG_Constructor
**
** Initialize the packet log to a known state
**
** Notes:
**   1. This must be called prior to any other function.
**
*/
void MSGLOG_Constructor(MSGLOG_Class_t* MsgLogPtr, INITBL_Class_t* IniTbl);


/******************************************************************************
** Function: MSGLOG_ResetStatus
**
** Reset counters and status flags to a known reset state.
**
** Notes:
**   1. Any counter or variable that is reported in HK telemetry that doesn't
**      change the functional behavior should be reset.
**
*/
void MSGLOG_ResetStatus(void);


/******************************************************************************
** Function: MSGLOG_RunChildFuncCmd
**
** Notes:
**   1. This is not intended to be a ground command. This function provides a
**      mechanism for the parent app to periodically call a child task function.
**
*/
bool MSGLOG_RunChildFuncCmd(void* DataObjPtr, const CFE_SB_Buffer_t *SbBufPtr);


/******************************************************************************
** Function: MSGLOG_StartLogCmd
**
*/
bool MSGLOG_StartLogCmd(void* DataObjPtr, const CFE_SB_Buffer_t *SbBufPtr);


/******************************************************************************
** Function: MSGLOG_StopLogCmd
**
*/
bool MSGLOG_StopLogCmd(void* DataObjPtr, const CFE_SB_Buffer_t *SbBufPtr);


/******************************************************************************
** Function: MSGLOG_StartPlaybkCmd
**
*/
bool MSGLOG_StartPlaybkCmd(void* DataObjPtr, const CFE_SB_Buffer_t *SbBufPtr);


/******************************************************************************
** Function: MSGLOG_StopPlaybkCmd
**
*/
bool MSGLOG_StopPlaybkCmd(void* DataObjPtr, const CFE_SB_Buffer_t *SbBufPtr);


#endif /* _msglog_ */
