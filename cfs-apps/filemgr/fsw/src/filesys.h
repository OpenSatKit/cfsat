/*
** Purpose: Provide a table for identifying onboard file systems and for
**          managing commands that obtain information about the file systems. 
**
** Notes:
**   1. Refactored from NASA's FM FreeSpace table. I renamed to FileSys 
**      because "free space" is an attrbute of a file system volume.
**   2. The original design doesn't have concepts such as File and Dir
**      objects but it did separate table from non-table functions. This
**      design includes file system functions like "SendOpenFilesTlm"
**      because it is not operating on a File object. 
**   3. Use the Singleton design pattern. A pointer to the table object
**      is passed to the constructor and saved for all other operations.
**      Note the cFE's buffers are used to store the actual data itself.
**      This is a table-specific file so it doesn't need to be re-entrant.
**   4. Command and telemetry packets are defined in EDS file filemgr.xml.
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

#ifndef _filesys_
#define _filesys_

/*
** Includes
*/

#include "app_cfg.h"

/***********************/
/** Macro Definitions **/
/***********************/


#define FILESYS_TBL_ENTRY_DISABLED     0
#define FILESYS_TBL_ENTRY_ENABLED      1
#define FILESYS_TBL_ENTRY_UNUSED       2


#define FILESYS_TBL_REGISTER_ERR_EID         (FILESYS_BASE_EID + 0)
#define FILESYS_TBL_VERIFY_ERR_EID           (FILESYS_BASE_EID + 1)
#define FILESYS_TBL_VERIFIED_EID             (FILESYS_BASE_EID + 2)
#define FILESYS_SEND_TLM_ERR_EID             (FILESYS_BASE_EID + 3)
#define FILESYS_SEND_TLM_CMD_EID             (FILESYS_BASE_EID + 4)
#define FILESYS_SET_TBL_STATE_LOAD_ERR_EID   (FILESYS_BASE_EID + 5)
#define FILESYS_SET_TBL_STATE_ARG_ERR_EID    (FILESYS_BASE_EID + 5)
#define FILESYS_SET_TBL_STATE_UNUSED_ERR_EID (FILESYS_BASE_EID + 6)
#define FILESYS_SET_TBL_STATE_CMD_EID        (FILESYS_BASE_EID + 7)
#define FILESYS_SEND_OPEN_FILES_CMD_EID      (FILESYS_BASE_EID + 8)


/**********************/
/** Type Definitions **/
/**********************/


/******************************************************************************
** Table Struture
*/

typedef struct
{
   
   uint32  State; 
   char    Name[OS_MAX_PATH_LEN];

} FILESYS_Volume_t;


typedef struct{
   
   FILESYS_Volume_t Volume[FILEMGR_FILESYS_TBL_VOL_CNT];

} FILESYS_TblData_t;


/******************************************************************************
** Class Struture
*/

typedef struct
{

   bool                Registered;
   int32               Status;        /* Status of last cFE Table service call */
   CFE_TBL_Handle_t    Handle;
   FILESYS_TblData_t*  DataPtr;

} FILESYS_CfeTbl_t;

typedef struct
{

   /*
   ** App Framework
   */
   
   const INITBL_Class_t* IniTbl;

   /*
   ** Tables
   */
   
   FILESYS_CfeTbl_t  CfeTbl;

   /*
   ** Telemetry
   */
   
   FILEMGR_FileSysTblTlm_t  TblTlm;
   FILEMGR_OpenFileTlm_t    OpenFileTlm;

   /*
   ** Class State Data
   */

   const char* CfeTblName;
   FileUtil_OpenFileList_t OpenFileList;
   

} FILESYS_Class_t;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: FILESYS_Constructor
**
** Initialize the example cFE Table object.
**
** Notes:
**   None
*/
void FILESYS_Constructor(FILESYS_Class_t* FileSysPtr, const INITBL_Class_t* IniTbl);


/******************************************************************************
** Function: FILESYS_ResetStatus
**
** Reset counters and status flags to a known reset state.  The behavior of
** the table manager should not be impacted. The intent is to clear counters
** and flags to a known default state for telemetry.
**
*/
void FILESYS_ResetStatus(void);


/******************************************************************************
** Function: FILESYS_ManageTbl
**
** Manage the cFE table interface for table loads and validation. 
*/
void FILESYS_ManageTbl(void);


/******************************************************************************
** Function: FILESYS_SendOpenFileTlmCmd
**
** Note:
**  1. This function must comply with the CMDMGR_CmdFuncPtr_t definition
*/
bool FILESYS_SendOpenFileTlmCmd(void* DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: FILESYS_SendTblTlmCmd
**
** Note:
**  1. This function must comply with the CMDMGR_CmdFuncPtr_t definition
*/
bool FILESYS_SendTblTlmCmd(void* DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: FILESYS_SetTblStateCmd
**
** Note:
**  1. This function must comply with the CMDMGR_CmdFuncPtr_t definition
*/
bool FILESYS_SetTblStateCmd(void* DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


#endif /* _filesys_ */
