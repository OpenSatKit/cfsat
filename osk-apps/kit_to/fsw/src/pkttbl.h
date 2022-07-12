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
**    Define KIT_TO's Packet Table management functions
**
**  Notes:
**    1. Use the Singleton design pattern. A pointer to the table object
**       is passed to the constructor and saved for all other operations.
**       This is a table-specific file so it doesn't need to be re-entrant.
**    2. The table file is a JSON text file.
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide
**    2. cFS Application Developer's Guide
**
*/
#ifndef _pkttbl_
#define _pkttbl_

/*
** Includes
*/

#include "app_cfg.h"

/***********************/
/** Macro Definitions **/
/***********************/

#define PKTTBL_APP_ID_MASK   (0x07FF)  /* CCSDS v1 ApId mask    */

#define PKTTBL_UNUSED_MSG_ID CFE_SB_MsgIdToValue(CFE_SB_INVALID_MSG_ID)

/*
** Event Message IDs
*/

#define PKTTBL_CREATE_FILE_ERR_EID (PKTTBL_BASE_EID + 0)
#define PKTTBL_LOAD_EID            (PKTTBL_BASE_EID + 1)
#define PKTTBL_LOAD_ERR_EID        (PKTTBL_BASE_EID + 2)


/**********************/
/** Type Definitions **/
/**********************/

/******************************************************************************
** Table -  Local table copy used for table loads
** 
*/

typedef struct
{

   uint16        MsgId;
   CFE_SB_Qos_t  Qos;
   uint16        BufLim;

   PktUtil_Filter_t Filter;
   
} PKTTBL_Pkt_t;


typedef struct
{
   
   PKTTBL_Pkt_t Pkt[PKTUTIL_MAX_APP_ID];

} PKTTBL_Data_t;


/* Callback function for table owner to perform the load */
typedef bool (*PKTTBL_LoadNewTbl_t)(PKTTBL_Data_t* NewTbl);


typedef struct
{

   /*
   ** Table parameter data
   */
   
   PKTTBL_Data_t Data;

   PKTTBL_LoadNewTbl_t LoadNewTbl;

   /*
   ** Standard CJSON table data
   */
   
   const char  *AppName;
   bool        Loaded;   /* Has entire table been loaded? */
   uint8       LastLoadStatus;
   uint16      LastLoadCnt;
   
   size_t      JsonObjCnt;
   char        JsonBuf[PKTTBL_JSON_FILE_MAX_CHAR];   
   size_t      JsonFileLen;

} PKTTBL_Class_t;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: PKTTBL_Constructor
**
** Initialize the Packet Table object.
**
** Notes:
**   1. The table values are not populated. This is done when the table is 
**      registered with the table manager.
*/
void PKTTBL_Constructor(PKTTBL_Class_t *ObjPtr, const char *AppName,
                        PKTTBL_LoadNewTbl_t LoadNewTbl);


/******************************************************************************
** Function: PKTTBL_DumpCmd
**
** Command to dump the table.
**
** Notes:
**  1. Function signature must match TBLMGR_DumpTblFuncPtr_t.
**  2. Can assume valid table file name because this is a callback from 
**     the app framework table manager.
**
*/
bool PKTTBL_DumpCmd(TBLMGR_Tbl_t *Tbl, uint8 DumpType, const char *Filename);


/******************************************************************************
** Function: PKTTBL_LoadCmd
**
** Command to load the table.
**
** Notes:
**  1. Function signature must match TBLMGR_LoadTblFuncPtr_t.
**  2. Can assume valid table file name because this is a callback from 
**     the app framework table manager.
**
*/
bool PKTTBL_LoadCmd(TBLMGR_Tbl_t *Tbl, uint8 LoadType, const char *Filename);


/******************************************************************************
** Function: PKTTBL_ResetStatus
**
** Reset counters and status flags to a known reset state.  The behavior of
** the table manager should not be impacted. The intent is to clear counters
** and flags to a known default state for telemetry.
**
*/
void PKTTBL_ResetStatus(void);


/******************************************************************************
** Function: KTTBL_SetPacketToUnused
**
*/
void PKTTBL_SetPacketToUnused(PKTTBL_Pkt_t *PktPtr);


/******************************************************************************
** Function: PKTTBL_SetTblToUnused
**
*/
void PKTTBL_SetTblToUnused(PKTTBL_Data_t *TblPtr);


#endif /* _pkttbl_ */
