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
**    Define methods for managing files
**
**  Notes:
**    1. Command and telemetry packets are defined in EDS file filemgr.xml.
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide.
**    2. cFS Application Developer's Guide.
**
*/

#ifndef _file_
#define _file_

/*
** Includes
*/

#include "app_cfg.h"

/***********************/
/** Macro Definitions **/
/***********************/

#define FILE_IGNORE_CRC  0


/*
** Event Message IDs
*/

#define FILE_CONCATENATE_EID          (FILE_BASE_EID +  0)
#define FILE_CONCATENATE_ERR_EID      (FILE_BASE_EID +  1)
#define FILE_COPY_EID                 (FILE_BASE_EID +  2)
#define FILE_COPY_ERR_EID             (FILE_BASE_EID +  3)
#define FILE_DECOMPRESS_EID           (FILE_BASE_EID +  4)
#define FILE_DECOMPRESS_ERR_EID       (FILE_BASE_EID +  5)
#define FILE_DELETE_EID               (FILE_BASE_EID +  6)
#define FILE_DELETE_ERR_EID           (FILE_BASE_EID +  7)
#define FILE_MOVE_EID                 (FILE_BASE_EID +  8)
#define FILE_MOVE_ERR_EID             (FILE_BASE_EID +  9)
#define FILE_RENAME_EID               (FILE_BASE_EID + 10)
#define FILE_RENAME_ERR_EID           (FILE_BASE_EID + 11)
#define FILE_SEND_INFO_EID            (FILE_BASE_EID + 12)
#define FILE_SEND_INFO_ERR_EID        (FILE_BASE_EID + 13)
#define FILE_SET_PERMISSIONS_EID      (FILE_BASE_EID + 14)
#define FILE_SET_PERMISSIONS_ERR_EID  (FILE_BASE_EID + 15)
#define FILE_COMPUTE_FILE_CRC_ERR_EID (FILE_BASE_EID + 16)

/**********************/
/** Type Definitions **/
/**********************/


/******************************************************************************
** Telmetery Packets
*/

/* 
** Packet sent in response to FILE_SendInfoPktCmd that contains information
** for a single file
*/

typedef struct
{

   CFE_MSG_TelemetryHeader_t TlmHeader;
   
   uint8   State;                 /* See FileUtil_FileState definitions */
   uint8   CrcComputed;           /* Flag indicating whether a CRC was computed */
   uint8   Spare[2];              /* Double word alignment padding */
   uint32  Crc;                   /* CRC value if computed */
   uint32  Size;                  /* File Size in bytes */
   uint32  Time;                  /* Time of last file modification */
   uint32  Mode;                  /* Permissions as received by call to TODO */    
   char    Filename[OS_MAX_PATH_LEN];

} FILE_InfoPkt_t;


/******************************************************************************
** FILE_Class
*/

typedef struct
{

   /*
   ** App Framework References
   */
   
   const INITBL_Class_t*  IniTbl;

   /*
   ** Telemetry Packets
   */
   
   FILE_MGR_FileInfoTlm_t  InfoTlm;

   /*
   ** File State Data
   */

   uint16  CmdWarningCnt;

   char FileTaskBuf[FILE_MGR_TASK_FILE_BLOCK_SIZE];
   
} FILE_Class_t;


/******************************************************************************
** File Structure
*/


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: FILE_Constructor
**
** Initialize the example object to a known state
**
** Notes:
**   1. This must be called prior to any other function.
**
*/
void FILE_Constructor(FILE_Class_t *FilePtr, const INITBL_Class_t* IniTbl);


/******************************************************************************
** Function: FILE_ResetStatus
**
** Reset counters and status flags to a known reset state.
**
** Notes:
**   1. Any counter or variable that is reported in HK telemetry that doesn't
**      change the functional behavior should be reset.
**
*/
void FILE_ResetStatus(void);


/******************************************************************************
** Function: FILE_ConcatenateCmd
**
*/
bool FILE_ConcatenateCmd(void* DataObjPtr, const CFE_SB_Buffer_t *SbBufPtr);


/******************************************************************************
** Function: FILE_DecompressCmd
**
*/
bool FILE_DecompressCmd(void* DataObjPtr, const CFE_SB_Buffer_t *SbBufPtr);


/******************************************************************************
** Function: FILE_CopyCmd
**
*/
bool FILE_CopyCmd(void* DataObjPtr, const CFE_SB_Buffer_t *SbBufPtr);


/******************************************************************************
** Function: FILE_DeleteCmd
**
*/
bool FILE_DeleteCmd(void* DataObjPtr, const CFE_SB_Buffer_t *SbBufPtr);


/******************************************************************************
** Function: FILE_MoveCmd
**
*/
bool FILE_MoveCmd(void* DataObjPtr, const CFE_SB_Buffer_t *SbBufPtr);


/******************************************************************************
** Function: FILE_RenameCmd
**
*/
bool FILE_RenameCmd(void* DataObjPtr, const CFE_SB_Buffer_t *SbBufPtr);


/******************************************************************************
** Function: FILE_SendInfoTlmCmd
**
** Notes:
**   1. If the file exists then a telemetry packet will be sent regardless of
**      whether a CRC was request but could not be computed due to the file
**      being open
**
*/
bool FILE_SendInfoTlmCmd(void* DataObjPtr, const CFE_SB_Buffer_t *SbBufPtr);


/******************************************************************************
** Function: FILE_SetPermissionsCmd
**
*/
bool FILE_SetPermissionsCmd(void* DataObjPtr, const CFE_SB_Buffer_t *SbBufPtr);


#endif /* _file_ */
