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
**    Implement the Message Log table
**
**  Notes:
**    1. The static "TblData" serves as a table load buffer. Table dump data is
**       read directly from table owner's table storage.
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
#include "msglogtbl.h"


/***********************/
/** Macro Definitions **/
/***********************/


/**********************/
/** Type Definitions **/
/**********************/


/************************************/
/** Local File Function Prototypes **/
/************************************/

static bool LoadJsonData(size_t JsonFileLen);


/**********************/
/** Global File Data **/
/**********************/

static MSGLOGTBL_Class_t* MsgLogTbl = NULL;

static MSGLOGTBL_Data_t TblData; /* Working buffer for loads */

static CJSON_Obj_t JsonTblObjs[] = {

   /* Table Data Address        Table Data Length             Updated, Data Type,  Float  core-json query string, length of query string(exclude '\0') */
   
   { TblData.File.PathBaseName, OS_MAX_PATH_LEN,              false,   JSONString, false, { "file.path-base-name", (sizeof("file.path-base-name")-1)} },
   { TblData.File.Extension,    MSGLOGTBL_FILE_EXT_MAX_LEN,   false,   JSONString, false, { "file.extension",      (sizeof("file.extension")-1)}      },
   { &TblData.File.EntryCnt,    sizeof(TblData.File.EntryCnt),false,   JSONNumber, false, { "file.entry-cnt",      (sizeof("file.entry-cnt")-1)}      },
   { &TblData.PlaybkDelay,      sizeof(TblData.PlaybkDelay),  false,   JSONNumber, false, { "playbk-delay",        (sizeof("playbk-delay")-1)}        }
   
};


/******************************************************************************
** Function: MSGLOGTBL_Constructor
**
** Notes:
**    1. This must be called prior to any other functions
**
*/
void MSGLOGTBL_Constructor(MSGLOGTBL_Class_t* MsgLogTblPtr, const INITBL_Class_t* IniTbl)
{

   MsgLogTbl = MsgLogTblPtr;

   CFE_PSP_MemSet(MsgLogTbl, 0, sizeof(MSGLOGTBL_Class_t));

   MsgLogTbl->AppName = INITBL_GetStrConfig(IniTbl, CFG_APP_CFE_NAME);
   MsgLogTbl->JsonObjCnt = (sizeof(JsonTblObjs)/sizeof(CJSON_Obj_t));
         
} /* End MSGLOGTBL_Constructor() */


/******************************************************************************
** Function: MSGLOGTBL_DumpCmd
**
** Notes:
**  1. Function signature must match TBLMGR_DumpTblFuncPtr_t.
**  2. Can assume valid table filename because this is a callback from 
**     the app framework table manager that has verified the file.
**  3. DumpType is unused.
**  4. File is formatted so it can be used as a load file. It does not follow
**     the cFE table file format. 
**  5. Creates a new dump file, overwriting anything that may have existed
**     previously
*/
bool MSGLOGTBL_DumpCmd(TBLMGR_Tbl_t* Tbl, uint8 DumpType, const char* Filename)
{

   bool       RetStatus = false;
   int32      SysStatus;
   osal_id_t  FileHandle;
   os_err_name_t OsErrStr;
   char DumpRecord[256];
   char SysTimeStr[128];

   
   SysStatus = OS_OpenCreate(&FileHandle, Filename, OS_FILE_FLAG_CREATE, OS_READ_WRITE);

   if (SysStatus == OS_SUCCESS)
   {
 
      sprintf(DumpRecord,"{\n   \"app-name\": \"%s\",\n   \"tbl-name\": \"Message Log\",\n",MsgLogTbl->AppName);
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

      CFE_TIME_Print(SysTimeStr, CFE_TIME_GetTime());
      sprintf(DumpRecord,"   \"description\": \"Table dumped at %s\",\n",SysTimeStr);
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

      sprintf(DumpRecord,"   \"file\": {\n");
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
      
      sprintf(DumpRecord,"     \"path-base-name\": \"%s\",\n", MsgLogTbl->Data.File.PathBaseName);
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

      sprintf(DumpRecord,"     \"extension\": \"%s\",\n", MsgLogTbl->Data.File.Extension);
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

      sprintf(DumpRecord,"     \"entry-cnt\": %d\n   },\n", MsgLogTbl->Data.File.EntryCnt);
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

      sprintf(DumpRecord,"   \"playbk-delay\": %d\n}\n", MsgLogTbl->Data.PlaybkDelay);
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

      OS_close(FileHandle);

      RetStatus = true;

   } /* End if file create */
   else
   {
      OS_GetErrorName(SysStatus, &OsErrStr);
      CFE_EVS_SendEvent(MSGLOGTBL_DUMP_ERR_EID, CFE_EVS_EventType_ERROR,
                        "Error creating dump file '%s', status=%s",
                        Filename, OsErrStr);
   
   } /* End if file create error */

   return RetStatus;
   
} /* End of MSGLOGTBL_DumpCmd() */


/******************************************************************************
** Function: MSGLOGTBL_LoadCmd
**
** Notes:
**  1. Function signature must match TBLMGR_LoadTblFuncPtr_t.
**  2. This could migrate into table manager but I think I'll keep it here so
**     user's can add table processing code if needed.
*/
bool MSGLOGTBL_LoadCmd(TBLMGR_Tbl_t* Tbl, uint8 LoadType, const char* Filename)
{

   bool  RetStatus = false;

   if (CJSON_ProcessFile(Filename, MsgLogTbl->JsonBuf, MSGLOGTBL_JSON_FILE_MAX_CHAR, LoadJsonData))
   {
      
      MsgLogTbl->Loaded = true;
      MsgLogTbl->LastLoadStatus = TBLMGR_STATUS_VALID;
      RetStatus = true;
   
   }
   else
   {

      MsgLogTbl->LastLoadStatus = TBLMGR_STATUS_INVALID;

   }

   return RetStatus;
   
} /* End MSGLOGTBL_LoadCmd() */


/******************************************************************************
** Function: MSGLOGTBL_ResetStatus
**
*/
void MSGLOGTBL_ResetStatus(void)
{

   MsgLogTbl->LastLoadStatus = TBLMGR_STATUS_UNDEF;
   MsgLogTbl->LastLoadCnt = 0;
 
} /* End MSGLOGTBL_ResetStatus() */


/******************************************************************************
** Function: LoadJsonData
**
** Notes:
**  1. See file prologue for full/partial table load scenarios
*/
static bool LoadJsonData(size_t JsonFileLen)
{

   bool      RetStatus = false;
   size_t    ObjLoadCnt;


   MsgLogTbl->JsonFileLen = JsonFileLen;

   /* 
   ** 1. Copy table owner data into local table buffer
   ** 2. Process JSON file which updates local table buffer with JSON supplied values
   ** 3. If valid, copy local buffer over owner's data 
   */
   
   memcpy(&TblData, &MsgLogTbl->Data, sizeof(MSGLOGTBL_Data_t));
   
   ObjLoadCnt = CJSON_LoadObjArray(JsonTblObjs, MsgLogTbl->JsonObjCnt, MsgLogTbl->JsonBuf, MsgLogTbl->JsonFileLen);

   if (!MsgLogTbl->Loaded && (ObjLoadCnt != MsgLogTbl->JsonObjCnt))
   {

      CFE_EVS_SendEvent(MSGLOGTBL_LOAD_ERR_EID, CFE_EVS_EventType_ERROR, 
                        "Table has never been loaded and new table only contains %d of %d data objects",
                        (unsigned int)ObjLoadCnt, (unsigned int)MsgLogTbl->JsonObjCnt);
   
   }
   else
   {
   
      memcpy(&MsgLogTbl->Data,&TblData, sizeof(MSGLOGTBL_Data_t));
      MsgLogTbl->LastLoadCnt = ObjLoadCnt;
      RetStatus = true;
      
   }
   
   return RetStatus;
   
} /* End LoadJsonData() */
