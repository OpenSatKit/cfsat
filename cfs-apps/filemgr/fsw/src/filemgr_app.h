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
**    Define the File Manager application
**
**  Notes:
**    1. This is a refactor of NASA's File Manager (FM) app. The refactor includes
**       adaptation to the OSK app framework and prootyping the usage of an app 
**       init JSON file. The idea is to rethink whcih configuration paarameters
**       should be compile time and which should be runtime.
**    2. Command and telemetry packets are defined in EDS file filemgr.xml.
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide.
**    2. cFS Application Developer's Guide.
**
*/

#ifndef _filemgr_app_
#define _filemgr_app_

/*
** Includes
*/

#include "app_cfg.h"
#include "dir.h"
#include "file.h"
#include "filesys.h"


/***********************/
/** Macro Definitions **/
/***********************/

/*
** Events
*/

#define FILEMGR_INIT_APP_EID    (FILEMGR_BASE_EID + 0)
#define FILEMGR_NOOP_EID        (FILEMGR_BASE_EID + 1)
#define FILEMGR_EXIT_EID        (FILEMGR_BASE_EID + 2)
#define FILEMGR_INVALID_MID_EID (FILEMGR_BASE_EID + 3)


/**********************/
/** Type Definitions **/
/**********************/


/******************************************************************************
** Command Packets
*/


/******************************************************************************
** Telemetry Packets
*/


//EDS typedef struct
//EDS {
//EDS 
//EDS    CFE_MSG_TelemetryHeader_t TlmHeader;
//EDS 
//EDS    uint8   CommandCounter;
//EDS    uint8   CommandErrCounter;
//EDS    uint8   Spare;                          
//EDS    uint8   NumOpenFiles;                   /* Number of open files in the system */
//EDS    uint8   ChildCmdCounter;                /* Child task command counter */
//EDS    uint8   ChildCmdErrCounter;             /* Child task command error counter */
//EDS    uint8   ChildCmdWarnCounter;            /* Child task command warning counter */
//EDS    uint8   ChildQueueCount;                /* Number of pending commands in queue */
//EDS    uint8   ChildCurrentCC;                 /* Command code currently executing */
//EDS    uint8   ChildPreviousCC;                /* Command code previously executed */
//EDS 
//EDS } FILEMGR_HkPkt_t;
//EDS #define FILEMGR_TLM_HK_LEN sizeof (FILEMGR_HkPkt_t)


/******************************************************************************
** FILEMGR_Class
*/
typedef struct
{

   /* 
   ** App Framework
   */ 
   
   INITBL_Class_t   IniTbl; 
   CFE_SB_PipeId_t  CmdPipe;
   CMDMGR_Class_t   CmdMgr;
   TBLMGR_Class_t   TblMgr;
   
   CHILDMGR_Class_t ChildMgr;
   
   FileUtil_OpenFileList_t OpenFileList;
   
   /*
   ** Telemetry Packets
   */
   
   FILEMGR_HkTlm_t HkPkt;
   
   /*
   ** App Contained Objects
   */ 
       
   uint32           PerfId;
   CFE_SB_MsgId_t   CmdMid;
   CFE_SB_MsgId_t   SendHkMid;
   
   DIR_Class_t      Dir;
   FILE_Class_t     File;
   FILESYS_Class_t  FileSys;
 
} FILEMGR_Class_t;


/*******************/
/** Exported Data **/
/*******************/

extern FILEMGR_Class_t  FileMgr;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: FILEMGR_AppMain
**
*/
void FILEMGR_AppMain(void);


/******************************************************************************
** Function: FILEMGR_NoOpCmd
**
*/
bool FILEMGR_NoOpCmd(void* ObjDataPtr, const CFE_SB_Buffer_t *SbBufPtr);


/******************************************************************************
** Function: FILEMGR_ResetAppCmd
**
*/
bool FILEMGR_ResetAppCmd(void* ObjDataPtr, const CFE_SB_Buffer_t *SbBufPtr);


/******************************************************************************
** Function: FILEMGR_SendHousekeepingPkt
**
*/
void FILEMGR_SendHousekeepingPkt(void);


#endif /* _filemgr_app_ */
