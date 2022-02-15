/*
** Purpose: Define methods for managing directories
**
** Notes:
**   1. Command and telemetry packets are defined in EDS file filemgr.xml. 
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

#ifndef _dir_
#define _dir_

/*
** Includes
*/

#include "app_cfg.h"


/***********************/
/** Macro Definitions **/
/***********************/

/*
** Event Message IDs
*/

#define DIR_CREATE_EID               (DIR_BASE_EID +  0)
#define DIR_CREATE_ERR_EID           (DIR_BASE_EID +  1)
#define DIR_DELETE_ALL_EID           (DIR_BASE_EID +  2)
#define DIR_DELETE_ALL_ERR_EID       (DIR_BASE_EID +  3)
#define DIR_DELETE_ALL_WARN_EID      (DIR_BASE_EID +  4)
#define DIR_DELETE_EID               (DIR_BASE_EID +  5)
#define DIR_DELETE_ERR_EID           (DIR_BASE_EID +  6)
#define DIR_SEND_LIST_PKT_EID        (DIR_BASE_EID +  7)
#define DIR_SEND_LIST_PKT_ERR_EID    (DIR_BASE_EID +  8)
#define DIR_SEND_LIST_PKT_WARN_EID   (DIR_BASE_EID +  9)
#define DIR_WRITE_LIST_FILE_EID      (DIR_BASE_EID + 10)
#define DIR_WRITE_LIST_FILE_ERR_EID  (DIR_BASE_EID + 11)
#define DIR_WRITE_LIST_FILE_WARN_EID (DIR_BASE_EID + 12)

/**********************/
/** Type Definitions **/
/**********************/


/******************************************************************************
** File Structures
*/

typedef struct
{

   char    Name[OS_MAX_PATH_LEN];
   uint32  Size;
   uint32  Time;     /* File's Last Modification Times */
   uint32  Mode;     /* Mode of the file (Permissions) */

} DIR_FileEntry_t;

typedef struct
{

   char    DirName[OS_MAX_PATH_LEN];
   uint32  DirFileCnt;                 /* Number of files in the directory */
   uint32  FilesWrittenCnt;            /* Number of entries written to file  */

} DIR_ListFilesStats_t;


/******************************************************************************
** DIR_Class
*/

typedef struct
{

   /*
   ** App Framework References
   */
   
   const INITBL_Class_t*  IniTbl;

   /*
   ** Telemetry
   */
   
   FILEMGR_DirListTlm_t  ListTlm;
   
   /*
   ** Files
   */

   DIR_ListFilesStats_t  ListFileStats;
   
   /*
   ** FileMgr State Data
   */

   uint16  CmdWarningCnt;
   

} DIR_Class_t;


/************************/
/** Exported Functions **/
/************************/

/******************************************************************************
** Function: DIR_Constructor
**
** Initialize the DIR to a known state
**
** Notes:
**   1. This must be called prior to any other function.
**
*/
void DIR_Constructor(DIR_Class_t *DirPtr, const INITBL_Class_t* IniTbl);


/******************************************************************************
** Function: DIR_ResetStatus
**
** Reset counters and status flags to a known reset state.
**
** Notes:
**   1. Any counter or variable that is reported in HK telemetry that doesn't
**      change the functional behavior should be reset.
**
*/
void DIR_ResetStatus(void);


/******************************************************************************
** Function: DIR_CreateCmd
**
** Create a new directory. 
*/
bool DIR_CreateCmd(void* DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: DIR_DeleteCmd
**
** Delete an existing empty directory.
*/
bool DIR_DeleteCmd(void* DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: DIR_DeleteAllCmd
**
*/
bool DIR_DeleteAllCmd(void* DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: DIR_SendDirListTlmCmd
**
*/
bool DIR_SendDirListTlmCmd(void* DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: DIR_WriteListFileCmd
**
** Notes:
**   1. Target file will be overwritten if it exists an is closed.
*/
bool DIR_WriteListFileCmd(void* DataObjPtr, const CFE_MSG_Message_t *MsgPtr);

#endif /* _dir_ */
