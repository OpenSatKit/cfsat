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
**    Define the scheduler. Owns scheduler & message tables
**
**  Notes:
**    1. This design intentionally decouples the scheduler table from 
**       application specific processing such as command callback 
**       functions and file processing.
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide
**    2. cFS Application Developer's Guide
**
*/

#ifndef _scheduler_
#define _scheduler_

/*
** Includes
*/

#include "app_cfg.h"
#include "msgtbl.h"
#include "schtbl.h"


/***********************/
/** Macro Definitions **/
/***********************/

/*
** Semaphore & Timer
*/

#define SCHEDULER_TIMER_NAME   "SCH_MINOR_TIMER"   /* Must be shorter than OS_MAX_API_NAME */
#define SCHEDULER_SEM_NAME     "SCH_TIME_SEM"      /* Must be shorter than OS_MAX_API_NAME */
#define SCHEDULER_SEM_VALUE    0
#define SCHEDULER_SEM_OPTIONS  0


/*
** Synchronized to Mission Elapsed Time States
*/

#define SCHEDULER_SYNCH_FALSE          0
#define SCHEDULER_SYNCH_TO_MINOR       1
#define SCHEDULER_SYNCH_MAJOR_PENDING  2
#define SCHEDULER_SYNCH_TO_MAJOR       4


/*
** Major Frame Signal Source Identifiers
*/

#define SCHEDULER_MF_SOURCE_NONE               0
#define SCHEDULER_MF_SOURCE_CFE_TIME           1
#define SCHEDULER_MF_SOURCE_MINOR_FRAME_TIMER  2


/*
** Minor Frame Slot Characteristics
*/

#define SCHEDULER_NORMAL_SLOT_PERIOD (SCHEDULER_MICROS_PER_MAJOR_FRAME / SCHTBL_SLOTS)
#define SCHEDULER_SYNC_SLOT_PERIOD   (SCHEDULER_NORMAL_SLOT_PERIOD + SCHEDULER_SYNC_SLOT_DRIFT_WINDOW)
#define SCHEDULER_SHORT_SLOT_PERIOD  (SCHEDULER_NORMAL_SLOT_PERIOD - SCHEDULER_SYNC_SLOT_DRIFT_WINDOW)


/*
** Major Frame Signal Source Identifiers
*/

#define SCHEDULER_MF_SRC_NONE               0
#define SCHEDULER_MF_SRC_CFE_TIME           1
#define SCHEDULER_MF_SRC_MINOR_FRAME_TIMER  2

#define SCHEDULER_TIME_SYNC_SLOT   (SCHTBL_SLOTS-1)  /* Slot processing algorithm assumes this is set to the last slot */


/*
** Maximum allowed error in minor frame timing. Worst accuracy determined to be
** the amount of drift that would cause the loss of a minor frame over one
** major frame
*/

#define SCHEDULER_WORST_CLOCK_ACCURACY  (SCHEDULER_NORMAL_SLOT_PERIOD/(SCHTBL_SLOTS-1))


/*
** Maximum number of minor frames to sample looking for subsecs = 0. Maximum
** number of minor frame timer expirations to allow before giving up on
** finding the slot whose MET subseconds field is zero.  It is assumed that
** three complete major frames should be sufficient for finding such a slot.
*/

#define SCHEDULER_MAX_SYNC_ATTEMPTS   (SCHTBL_SLOTS * 3)


/*
** Event Message IDs
*/

#define SCHEDULER_MINOR_FRAME_TIMER_CREATE_ERR_EID   (SCHEDULER_BASE_EID + 0)
#define SCHEDULER_MINOR_FRAME_TIMER_ACC_WARN_EID     (SCHEDULER_BASE_EID + 1)
#define SCHEDULER_SEM_CREATE_ERR_EID                 (SCHEDULER_BASE_EID + 2)
#define SCHEDULER_MAJOR_FRAME_SUB_ERR_EID            (SCHEDULER_BASE_EID + 3)
#define SCHEDULER_NOISY_MAJOR_FRAME_ERR_EID          (SCHEDULER_BASE_EID + 4)

#define SCHEDULER_SAME_SLOT_EID                      (SCHEDULER_BASE_EID + 5)
#define SCHEDULER_MULTI_SLOTS_EID                    (SCHEDULER_BASE_EID + 6)
#define SCHEDULER_SKIPPED_SLOTS_EID                  (SCHEDULER_BASE_EID + 7)

#define SCHEDULER_PACKET_SEND_ERR_EID                (SCHEDULER_BASE_EID + 8)

#define SCHEDULER_CMD_SUCCESS_EID                    (SCHEDULER_BASE_EID + 9)
#define SCHEDULER_CONFIG_SCH_TBL_BOOL_ERR_EID        (SCHEDULER_BASE_EID + 10)
#define SCHEDULER_LOAD_MSG_CMD_INDEX_ERR_EID         (SCHEDULER_BASE_EID + 11)
#define SCHEDULER_SEND_MSG_EVENT_CMD_INDEX_ERR_EID   (SCHEDULER_BASE_EID + 12)
#define SCHEDULER_SEND_MSG_TYPE_ERR_EID              (SCHEDULER_BASE_EID + 13)
#define SCHEDULER_SEND_DIAG_TLM_ERR_EID              (SCHEDULER_BASE_EID + 14)

#define SCHEDULER_DEBUG_EID                          (SCHEDULER_BASE_EID + 15)

#define SCHEDULER_UNDEF_SCHTBL_ENTRY_VAL 255
#define SCHEDULER_UNDEF_MSGTBL_ENTRY_VAL   0


/**********************/
/** Type Definitions **/
/**********************/

/******************************************************************************
** Command Packets
*/

typedef struct
{
   
   CFE_MSG_CommandHeader_t  CmdHeader;
   uint16  Slot;
   uint16  Activity;
   bool    Enabled;   /* 0=FALSE(Disabled), 1=TRUE(Enabled) */

} SCHEDULER_ConfigSchEntryCmdMsg_t;
#define SCHEDULER_CFG_SCH_ENTRY_CMD_DATA_LEN  (sizeof(SCHEDULER_ConfigSchEntryCmdMsg_t) - sizeof(CFE_MSG_CommandHeader_t))

typedef struct
{
   
   CFE_MSG_CommandHeader_t  CmdHeader;
   uint16  Slot;
   uint16  Activity;
   uint16  Enabled;    /* 0=FALSE(Disabled), 1=TRUE(Enabled) */
   uint16  Period;
   uint16  Offset;
   uint16  MsgTblIndex;

} SCHEDULER_LoadSchEntryCmdMsg_t;
#define SCHEDULER_LOAD_SCH_ENTRY_CMD_DATA_LEN  (sizeof(SCHEDULER_LoadSchEntryCmdMsg_t) - sizeof(CFE_MSG_CommandHeader_t))


typedef struct
{
   
   CFE_MSG_CommandHeader_t  CmdHeader;
   uint16   Slot;
   uint16   Activity;

} SCHEDULER_SendSchEntryCmdMsg_t;
#define SCHEDULER_SEND_SCH_ENTRY_CMD_DATA_LEN  (sizeof(SCHEDULER_SendSchEntryCmdMsg_t) - sizeof(CFE_MSG_CommandHeader_t))


typedef struct
{
   
   CFE_MSG_CommandHeader_t  CmdHeader;
   uint16   Index;
   uint16   MsgId;

} SCHEDULER_LoadMsgEntryCmdMsg_t;
#define SCHEDULER_LOAD_MSG_ENTRY_CMD_DATA_LEN  (sizeof(SCHEDULER_LoadMsgEntryCmdMsg_t) - sizeof(CFE_MSG_CommandHeader_t))


typedef struct
{
   
   CFE_MSG_CommandHeader_t  CmdHeader;
   uint16   Index;

} SCHEDULER_SendMsgEntryCmdMsg_t;
#define SCHEDULER_SEND_MSG_ENTRY_CMD_DATA_LEN (sizeof(SCHEDULER_SendMsgEntryCmdMsg_t) - sizeof(CFE_MSG_CommandHeader_t))



typedef struct
{
   
   CFE_MSG_CommandHeader_t  CmdHeader;
   uint16   Slot;

} SCHEDULER_SendDiagTlmCmdMsg_t;
#define SCHEDULER_SEND_DIAG_TLM_CMD_DATA_LEN  (sizeof(SCHEDULER_SendDiagTlmCmdMsg_t) - sizeof(CFE_MSG_CommandHeader_t))


/******************************************************************************
** Telemetry Packets
*/

/*
** See SCHEDULER_SendSchEntryCmd() and SCHEDULER_SendMsgEntryCmd() prologues
** for how this packet is created for each command.
*/
typedef struct
{

   CFE_MSG_TelemetryHeader_t TlmHeader;
   uint8  Slot;
   uint8  Activity;
   
   SCHTBL_Entry_t   SchTblEntry;
   MSGTBL_Entry_t   MsgTblEntry;

} SCHEDULER_TblEntryPkt_t;
#define SCHEDULER_TBL_ENTRY_TLM_LEN sizeof (SCHEDULER_TblEntryPkt_t)


typedef struct
{

   CFE_MSG_TelemetryHeader_t TlmHeader;

   /*
   ** Scheduler processing data not in HK
   */
   
   uint32  LastProcessCount;
   uint32  TimerId;
   uint32  TimeSemaphore;
   uint32  ClockAccuracy;
   uint32  WorstCaseSlotsPerMinorFrame;
   uint8   IgnoreMajorFrame;
   uint8   SyncToMET;
   uint8   MajorFrameSource;
   uint8   Spare;
   
   /*
   ** Send all the activities for the command-specified slot
   */
   
   SCHTBL_Entry_t SchTblSlot[SCHTBL_ACTIVITIES_PER_SLOT];

} SCHEDULER_DiagPkt_t;
#define SCHEDULER_DIAG_TLM_LEN sizeof (SCHEDULER_DiagPkt_t)


/******************************************************************************
** Scheduler Class
*/

typedef struct
{

   /*
   ** Telemetry Packets
   */
   
   SCHEDULER_TblEntryPkt_t TblEntryPkt;
   SCHEDULER_DiagPkt_t     DiagPkt;

   /*
   ** Scheduler State
   */ 

   uint32  SlotsProcessedCount;           /* Total number of Schedule Slots (Minor Frames) Processed */
   uint16  SkippedSlotsCount;             /* Number of times that slot (minor frame) were skipped. NOT the number of slots that were skipped  */
   uint16  MultipleSlotsCount;            /* Number of times that multiple slots were skipped. NOT the number of slots that were skipped  */
   uint16  SameSlotCount;                 /* Number of times scheduler woke up in the same slot as last time */

   uint32  ScheduleActivitySuccessCount;  /* Number of successfully performed activities */
   uint32  ScheduleActivityFailureCount;  /* Number of unsuccessful activities attempted */

   uint32  ValidMajorFrameCount;          /* Number of valid Major Frame tones received */
   uint32  MissedMajorFrameCount;         /* Number of missing Major Frame tones */
   uint32  UnexpectedMajorFrameCount;     /* Number of unexpected Major Frame tones */

   uint32  TablePassCount;                /* Number of times Schedule Table has been processed */
   uint32  ConsecutiveNoisyFrameCounter;  /* Number of consecutive noisy Major Frames */

   uint16  MinorFramesSinceTone;          /* Number of Minor Frames since last Major Frame tone */
   uint16  NextSlotNumber;                /* Next Minor Frame to be processed */
   uint16  LastSyncMETSlot;               /* MET Slot # where Time Sync last occurred */
   uint16  SyncAttemptsLeft;              /* Timeout counter used when syncing Major Frame to MET */

   bool    SendNoisyMajorFrameMsg;        /* Flag to send noisy major frame event msg once */
   bool    IgnoreMajorFrame;              /* Major Frame too noisy to trust */
   bool    UnexpectedMajorFrame;          /* Major Frame signal was unexpected */
   uint8   SyncToMET;                     /* Slots should be aligned with subseconds */
   uint8   MajorFrameSource;              /* Major Frame Signal source identifier */

   uint32  LastProcessCount;              /* Number of Slots Processed Last Cycle */

   uint32  TimerId;                       /* OSAL assigned timer ID for minor frame timer */
   uint32  TimeSemaphore;                 /* Semaphore used by time references to control main loop */
   uint32  ClockAccuracy;                 /* Accuracy of Minor Frame Timer */
   uint32  WorstCaseSlotsPerMinorFrame;   /* When syncing to MET, worst case # of slots that may need */


   /*
   ** Contained Objects
   */ 
   
   MSGTBL_Class_t MsgTbl;
   SCHTBL_Class_t SchTbl;
   
} SCHEDULER_Class_t;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: SCHEDULER_Constructor
**
** This method creates a new scheduler instance.
**
** Notes:
**   1. This method must be called prior to all other methods. The SchTbl
**      instance variable only needs to be passed to the constructor
**      because a reference is stored by schtbl.c.
**
*/
void SCHEDULER_Constructor(SCHEDULER_Class_t* ObjPtr, const INITBL_Class_t* IniTbl);


/******************************************************************************
** Function: SCHEDULER_ResetStatus
**
** Reset counters and status flags to a known reset state.  The behavior of the scheduler
** should not be impacted. The intent is to clear counters/flags for telemetry.
**
** Notes:
**   1. See the SCHEDULER_Class definition for the effected data.
**
*/
void SCHEDULER_ResetStatus(void);


/******************************************************************************
** Function: SCHEDULER_Execute
**
** Execute the scheduler to process schduler table and dispatch messages.
**
*/
bool SCHEDULER_Execute(void);


/******************************************************************************
** Function: SCHEDULER_StartTimers
**
*/
int32 SCHEDULER_StartTimers(void);


/******************************************************************************
** Function: SCHEDULER_ConfigSchEntryCmd
**
** Notes:
**   1. Function signature must match the CMDMGR_CmdFuncPtr_t definition
**
*/
bool SCHEDULER_ConfigSchEntryCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: SCHEDULER_LoadSchEntryCmd
**
** Notes:
**   1. Function signature must match the CMDMGR_CmdFuncPtr_t definition
**
*/
bool SCHEDULER_LoadSchEntryCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: SCHEDULER_SendSchEntryCmd
**
** Sends an informational event message containing the scheduler table entry 
** for the command-specified (slot,activity). It also sends a telemetry packet
** containing the same scheduler table entry and the contents of the message 
** table entry indexed by the scheduler table entry.
**
** Notes:
**   1. Function signature must match the CMDMGR_CmdFuncPtr_t definition
**
*/
bool SCHEDULER_SendSchEntryCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: SCHEDULER_LoadMsgEntryCmd
**
** Notes:
**   1. Function signature must match the CMDMGR_CmdFuncPtr_t definition
**
*/
bool SCHEDULER_LoadMsgEntryCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: SCHEDULER_SendMsgEntryCmd
**
** Sends an informational event message containing the message table entry 
** for the command-specified index. It also sends a telemetry packet
** containing the same message table entry and the first scheduler table entry
** found that references the message table entry.
**
** Notes:
**   1. Function signature must match the CMDMGR_CmdFuncPtr_t definition
**
*/
bool SCHEDULER_SendMsgEntryCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: SCHEDULER_SendDiagTlmCmd
**
** Notes:
**   1. Function signature must match the CMDMGR_CmdFuncPtr_t definition
**
*/
bool SCHEDULER_SendDiagTlmCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


#endif /* _scheduler_ */
