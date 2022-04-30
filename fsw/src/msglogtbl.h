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
**    Define the Message Log table
**
**  Notes:
**    1. Use the Singleton design pattern. A pointer to the table object
**       is passed to the constructor and saved for all other operations.
**       This is a table-specific file so it doesn't need to be re-entrant.
**    2. The first JSON table must define all parameters. After a complete
**       table has been loaded then partial tables can be loaded.
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide.
**    2. cFS Application Developer's Guide.
**
*/

#ifndef _msglogtbl_
#define _msglogtbl_

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

#define MSGLOGTBL_DUMP_ERR_EID   (MSGLOGTBL_BASE_EID + 0)
#define MSGLOGTBL_LOAD_ERR_EID   (MSGLOGTBL_BASE_EID + 1)


/**********************/
/** Type Definitions **/
/**********************/


/******************************************************************************
** Table - Local table copy used for table loads
** 
*/

typedef struct
{

   char     PathBaseName[OS_MAX_PATH_LEN];
   char     Extension[MSGLOGTBL_FILE_EXT_MAX_LEN];
   uint16   EntryCnt;

} MSGLOGTBL_File_t;


typedef struct
{

   MSGLOGTBL_File_t  File;
   uint16            PlaybkDelay;
   
} MSGLOGTBL_Data_t;


/* Return pointer to owner's table data */
typedef MSGLOGTBL_Data_t* (*MSGLOGTBL_GetDataPtr_t)(void);


typedef struct
{

   /*
   ** Table parameter data
   */
   
   MSGLOGTBL_Data_t Data;
   
   /*
   ** Standard CJSON table data
   */
   
   const char*  AppName;
   bool         Loaded;   /* Has entire table been loaded? */
   uint8        LastLoadStatus;
   uint16       LastLoadCnt;
   
   size_t       JsonObjCnt;
   char         JsonBuf[MSGLOGTBL_JSON_FILE_MAX_CHAR];   
   size_t       JsonFileLen;
   
} MSGLOGTBL_Class_t;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: MSGLOGTBL_Constructor
**
** Initialize the example table object.
**
** Notes:
**   1. The table values are not populated. This is done when the table is 
**      registered with the table manager.
**
*/
void MSGLOGTBL_Constructor(MSGLOGTBL_Class_t* TblObj, const INITBL_Class_t* IniTbl);


/******************************************************************************
** Function: MSGLOGTBL_ResetStatus
**
** Reset counters and status flags to a known reset state.  The behavior of
** the table manager should not be impacted. The intent is to clear counters
** and flags to a known default state for telemetry.
**
*/
void MSGLOGTBL_ResetStatus(void);


/******************************************************************************
** Function: MSGLOGTBL_LoadCmd
**
** Command to load the table.
**
** Notes:
**  1. Function signature must match TBLMGR_LoadTblFuncPtr_t.
**  2. Can assume valid table file name because this is a callback from 
**     the app framework table manager.
**
*/
bool MSGLOGTBL_LoadCmd(TBLMGR_Tbl_t* Tbl, uint8 LoadType, const char* Filename);


/******************************************************************************
** Function: MSGLOGTBL_DumpCmd
**
** Command to dump the table.
**
** Notes:
**  1. Function signature must match TBLMGR_DumpTblFuncPtr_t.
**  2. Can assume valid table file name because this is a callback from 
**     the app framework table manager.
**
*/
bool MSGLOGTBL_DumpCmd(TBLMGR_Tbl_t* Tbl, uint8 DumpType, const char* Filename);


#endif /* _msglogtbl_ */

