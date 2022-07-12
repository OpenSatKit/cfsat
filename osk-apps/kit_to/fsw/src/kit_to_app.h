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
**     Define the OpenSatKit Telemetry Output application. This app
**     receives telemetry packets from the software bus and uses its
**     packet table to determine whether packets should be sent over
**     a UDP socket.
**
**  Notes:
**    None
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide.
**    2. cFS Application Developer's Guide.
**
*/
#ifndef _kit_to_app_
#define _kit_to_app_

/*
** Includes
*/

#include "cfe_msgids.h"
#include "app_cfg.h"
#include "pkttbl.h"
#include "pktmgr.h"
#include "evt_plbk.h"


/***********************/
/** Macro Definitions **/
/***********************/

/*
** Events
*/

#define KIT_TO_APP_INIT_EID               (KIT_TO_APP_BASE_EID + 0)
#define KIT_TO_APP_INIT_ERR_EID           (KIT_TO_APP_BASE_EID + 1)
#define KIT_TO_APP_NOOP_EID               (KIT_TO_APP_BASE_EID + 2)
#define KIT_TO_APP_EXIT_EID               (KIT_TO_APP_BASE_EID + 3)
#define KIT_TO_APP_INVALID_MID_EID        (KIT_TO_APP_BASE_EID + 4)
#define KIT_TO_SET_RUN_LOOP_DELAY_EID     (KIT_TO_APP_BASE_EID + 5)
#define KIT_TO_INVALID_RUN_LOOP_DELAY_EID (KIT_TO_APP_BASE_EID + 6)
#define KIT_TO_DEMO_EID                   (KIT_TO_APP_BASE_EID + 7)
#define KIT_TO_TEST_FILTER_EID            (KIT_TO_APP_BASE_EID + 8)


/**********************/
/** Type Definitions **/
/**********************/


/******************************************************************************
** Command Packets
*/

typedef struct
{

   CFE_MSG_CommandHeader_t  CmdHeader;
   uint16   RunLoopDelay;

} KIT_TO_SetRunLoopDelayCmdMsg_t;
#define KIT_TO_SET_RUN_LOOP_DELAY_CMD_DATA_LEN  (sizeof(KIT_TO_SetRunLoopDelayCmdMsg_t) - sizeof(CFE_MSG_CommandHeader_t))


typedef struct
{

   CFE_MSG_CommandHeader_t  CmdHeader;
   uint16                   FilterType;
   PktUtil_FilterParam_t    FilterParam;

}  KIT_TO_TestFilterCmdMsg_t;
#define KIT_TO_TEST_FILTER_CMD_DATA_LEN  (sizeof(KIT_TO_TestFilterCmdMsg_t) - sizeof(CFE_MSG_CommandHeader_t))


/******************************************************************************
** Telemetry Packets
*/
typedef struct
{

   CFE_MSG_TelemetryHeader_t TlmHeader;

   /*
   ** CMDMGR Data
   */
   uint16   ValidCmdCnt;
   uint16   InvalidCmdCnt;


   uint16   RunLoopDelay;

   /*
   ** PKTTBL Data
   */

   uint8    PktTblLastLoadStatus;
   uint8    PktTblSpareAlignByte;
   uint16   PktTblAttrErrCnt;

   /*
   ** PKTMGR Data
   */

   uint8    StatsValid;
   uint8    PktMgrSpareAlignByte;
   uint16   PktsPerSec;
   uint32   BytesPerSec;
   uint16   TlmSockId;
   char     TlmDestIp[PKTMGR_IP_STR_LEN];
   
   /*
   ** EVT_PLBK Data
   */
   
   uint8    EvtPlbkEna;
   uint8    EvtPlbkHkPeriod;
   
} KIT_TO_HkPkt_t;
#define KIT_TO_TLM_HK_LEN sizeof (KIT_TO_HkPkt_t)



typedef struct
{

   CFE_MSG_TelemetryHeader_t TlmHeader;
   uint16             synch;
#if 0
   bit_field          bit1:1;
   bit_field          bit2:1;
   bit_field          bit34:2;
   bit_field          bit56:2;
   bit_field          bit78:2;

   bit_field          nibble1:4;
   bit_field          nibble2:4;
#endif
   uint8              bl1, bl2;       /* boolean */
   int8               b1, b2, b3, b4;
   int16              w1,w2;
   int32              dw1, dw2;
   float              f1, f2;
   double             df1, df2;
   char               str[10];

} KIT_TO_DataTypePkt_t;
#define KIT_TO_TLM_DATA_TYPE_LEN   sizeof (KIT_TO_DataTypePkt_t)


/******************************************************************************
** KIT_TO_Class
*/
typedef struct
{

   /* 
   ** App Framework
   */
   
   INITBL_Class_t  IniTbl;
   CFE_SB_PipeId_t CmdPipe;
   CMDMGR_Class_t  CmdMgr;
   TBLMGR_Class_t  TblMgr;

   /*
   ** Telemetry Packets
   */

   KIT_TO_HkPkt_t        HkPkt;
   KIT_TO_DataTypePkt_t  DataTypePkt;


   /*
   ** KIT_CI State & Contained Objects
   */
   
   CFE_SB_MsgId_t  CmdMid;
   CFE_SB_MsgId_t  SendHkMid;
   
   uint16  RunLoopDelay;
   uint16  RunLoopDelayMin;
   uint16  RunLoopDelayMax;

   PKTTBL_Class_t    PktTbl;
   PKTMGR_Class_t    PktMgr;
   EVT_PLBK_Class_t  EvtPlbk;
   
} KIT_TO_Class_t;


/*******************/
/** Exported Data **/
/*******************/

extern KIT_TO_Class_t  KitTo;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: KIT_TO_AppMain
**
*/
void KIT_TO_AppMain(void);


/******************************************************************************
** Function: KIT_TO_NoOpCmd
**
** Notes:
**   1. Function signature must match the CMDMGR_CmdFuncPtr_t definition
**
*/
bool KIT_TO_NoOpCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: KIT_TO_ResetAppCmd
**
** Notes:
**   1. Function signature must match the CMDMGR_CmdFuncPtr_t definition
**
*/
bool KIT_TO_ResetAppCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: KIT_TO_SetRunLoopDelayCmd
**
** Notes:
**   1. Function signature must match the CMDMGR_CmdFuncPtr_t definition
**
*/
bool KIT_TO_SetRunLoopDelayCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: KIT_TO_TestFilterCmd
**
*/
bool KIT_TO_TestFilterCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);

#endif /* _kit_to_app_ */
