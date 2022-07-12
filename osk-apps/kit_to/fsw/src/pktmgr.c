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
**    Manage Packet Table that defines which packets will be sent from
**    the software bus to a socket.
**
**  Notes:
**   1. This has some of the features of a flight app such as packet
**      filtering but it would need design/code reviews to transition it to a
**      flight mission. For starters it uses UDP sockets and it doesn't
**      regulate output bit rates. 
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide
**    2. cFS Application Developer's Guide
**
*/

/*
** Include Files:
*/

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "osapi.h"

#include "cfe_msgids.h"
#include "cfe_config.h"
#include "edslib_datatypedb.h"
#include "cfe_missionlib_api.h"
#include "cfe_missionlib_runtime.h"
#include "cfe_mission_eds_parameters.h"
#include "cfe_mission_eds_interface_parameters.h"

#include "app_cfg.h"
#include "pktmgr.h"


/******************************/
/** File Function Prototypes **/
/******************************/

static void  ComputeStats(uint16 PktsSent, uint32 BytesSent);
static void  DestructorCallback(void);
static void  FlushTlmPipe(void);
static bool  LoadPktTbl(PKTTBL_Data_t* NewTbl);
static int32 PackEdsOutputMessage(void *DestBuffer, const CFE_MSG_Message_t *SrcBuffer, 
                                  size_t SrcBufferSize, size_t *EdsDataSize);
static int32 SubscribeNewPkt(PKTTBL_Pkt_t *NewPkt);

/**********************/
/** Global File Data **/
/**********************/

static PKTMGR_Class_t*  PktMgr = NULL;
static CFE_HDR_TelemetryHeader_PackedBuffer_t SocketBuffer;
static uint16 SocketBufferLen = sizeof(SocketBuffer);

/******************************************************************************
** Function: PKTMGR_Constructor
**
*/
void PKTMGR_Constructor(PKTMGR_Class_t *PktMgrPtr, INITBL_Class_t *IniTbl)
{

   PktMgr = PktMgrPtr;

   PktMgr->IniTbl       = IniTbl;
   PktMgr->DownlinkOn   = false;
   PktMgr->SuppressSend = true;
   PktMgr->TlmSockId    = 0;
   PktMgr->TlmUdpPort   = INITBL_GetIntConfig(PktMgr->IniTbl, CFG_PKTMGR_UDP_TLM_PORT);
   strncpy(PktMgr->TlmDestIp, "000.000.000.000", PKTMGR_IP_STR_LEN);

   PKTMGR_InitStats(INITBL_GetIntConfig(IniTbl, CFG_APP_RUN_LOOP_DELAY),
                    INITBL_GetIntConfig(IniTbl, CFG_PKTMGR_STATS_INIT_DELAY));

   PKTTBL_SetTblToUnused(&(PktMgr->PktTbl.Data));

   CFE_SB_CreatePipe(&(PktMgr->TlmPipe),
                     INITBL_GetIntConfig(IniTbl, CFG_PKTMGR_PIPE_DEPTH),
                     INITBL_GetStrConfig(IniTbl, CFG_PKTMGR_PIPE_NAME));
      
   CFE_MSG_Init(CFE_MSG_PTR(PktMgr->PktTlm), 
                CFE_SB_ValueToMsgId(INITBL_GetIntConfig(IniTbl, CFG_KIT_TO_PKT_TBL_TLM_TOPICID)), 
                PKTMGR_PKT_TLM_LEN);
   
   OS_TaskInstallDeleteHandler(&DestructorCallback); /* Called when application terminates */

   PKTTBL_Constructor(&PktMgr->PktTbl, INITBL_GetStrConfig(IniTbl, CFG_APP_CFE_NAME), LoadPktTbl);

} /* End PKTMGR_Constructor() */


/******************************************************************************
** Function: PKTMGR_AddPktCmd
**
** Notes:
**   1. Command rejected if table has existing entry for commanded msg ID
**   2. Only update the table if the software bus subscription successful.  
** 
*/
bool PKTMGR_AddPktCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   const PKTMGR_AddPktCmdMsg_t *AddPktCmd = (const PKTMGR_AddPktCmdMsg_t *) MsgPtr;
   PKTTBL_Pkt_t  NewPkt;
   bool          RetStatus = true;
   int32         Status;
   uint16        AppId;

   
   AppId = AddPktCmd->MsgId & PKTTBL_APP_ID_MASK;
   
   if (PktMgr->PktTbl.Data.Pkt[AppId].MsgId == PKTTBL_UNUSED_MSG_ID)
   {
      
      NewPkt.MsgId        = AddPktCmd->MsgId;
      NewPkt.Qos          = AddPktCmd->Qos;
      NewPkt.BufLim       = AddPktCmd->BufLim;
      NewPkt.Filter.Type  = AddPktCmd->FilterType;
      NewPkt.Filter.Param = AddPktCmd->FilterParam;
   
      Status = SubscribeNewPkt(&NewPkt);
   
      if (Status == CFE_SUCCESS)
      {

         PktMgr->PktTbl.Data.Pkt[AppId] = NewPkt;
      
         CFE_EVS_SendEvent(PKTMGR_ADD_PKT_SUCCESS_EID, CFE_EVS_EventType_INFORMATION,
                           "Added message ID 0x%04X, QoS (%d,%d), BufLim %d",
                           NewPkt.MsgId, NewPkt.Qos.Priority, NewPkt.Qos.Reliability, NewPkt.BufLim);
      }
      else
      {
   
         CFE_EVS_SendEvent(PKTMGR_ADD_PKT_ERROR_EID, CFE_EVS_EventType_ERROR,
                           "Error adding message ID 0x%04X. Software Bus subscription failed with return status 0x%8x",
                           AddPktCmd->MsgId, Status);
      }
   
   } /* End if packet entry unused */
   else
   {
   
      CFE_EVS_SendEvent(PKTMGR_ADD_PKT_ERROR_EID, CFE_EVS_EventType_ERROR,
                        "Error adding message ID 0x%04X. Packet already exists in the packet table",
                        AddPktCmd->MsgId);
   }
   
   return RetStatus;

} /* End of PKTMGR_AddPktCmd() */


/******************************************************************************
** Function: PKTMGR_EnableOutputCmd
**
*/
bool PKTMGR_EnableOutputCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   const PKTMGR_EnableOutputCmdMsg_t *EnableOutputCmd = (const PKTMGR_EnableOutputCmdMsg_t *) MsgPtr;
   bool  RetStatus = true;
   int32 OsStatus;
   
   strncpy(PktMgr->TlmDestIp, EnableOutputCmd->DestIp, PKTMGR_IP_STR_LEN);

   PktMgr->SuppressSend = false;
   CFE_EVS_SendEvent(PKTMGR_TLM_OUTPUT_ENA_INFO_EID, CFE_EVS_EventType_INFORMATION,
                     "Telemetry output enabled for IP %s", PktMgr->TlmDestIp);

   /*
   ** If disabled then create the socket and turn it on. If already
   ** enabled then destination address is changed in the existing socket
   */
   if(PktMgr->DownlinkOn == false)
   {

      OsStatus = OS_SocketOpen(&PktMgr->TlmSockId, OS_SocketDomain_INET, OS_SocketType_DATAGRAM);

      if (OsStatus == OS_SUCCESS)
      {
         PKTMGR_InitStats(INITBL_GetIntConfig(PktMgr->IniTbl, CFG_APP_RUN_LOOP_DELAY),
                          INITBL_GetIntConfig(PktMgr->IniTbl, CFG_PKTMGR_STATS_CONFIG_DELAY));
         PktMgr->DownlinkOn = true;
      }
      else
      {
         RetStatus = false;
         CFE_EVS_SendEvent(PKTMGR_TLM_OUTPUT_ENA_SOCKET_ERR_EID, CFE_EVS_EventType_ERROR,
                           "Telemetry output socket open error. Status = %d", OsStatus);
      }

   } /* End if downlink disabled */

   return RetStatus;

} /* End PKTMGR_EnableOutputCmd() */


/******************************************************************************
** Function:  PKTMGR_InitStats
**
** If OutputTlmInterval==0 then retain current stats
** ComputeStats() logic assumes at least 1 init cycle
**
*/
void PKTMGR_InitStats(uint16 OutputTlmInterval, uint16 InitDelay)
{
   
   if (OutputTlmInterval != 0) PktMgr->Stats.OutputTlmInterval = (double)OutputTlmInterval;
   
   PktMgr->Stats.State = PKTMGR_STATS_INIT_CYCLE;
   PktMgr->Stats.InitCycles = (PktMgr->Stats.OutputTlmInterval >= InitDelay) ? 1 : (double)InitDelay/PktMgr->Stats.OutputTlmInterval;
            
   PktMgr->Stats.IntervalMilliSecs = 0.0;
   PktMgr->Stats.IntervalPkts = 0;
   PktMgr->Stats.IntervalBytes = 0;
      
   PktMgr->Stats.PrevIntervalAvgPkts  = 0.0;
   PktMgr->Stats.PrevIntervalAvgBytes = 0.0;
   
   PktMgr->Stats.AvgPktsPerSec  = 0.0;
   PktMgr->Stats.AvgBytesPerSec = 0.0;

} /* End PKTMGR_InitStats() */


/******************************************************************************
** Function: PKTMGR_OutputTelemetry
**
*/
uint16 PKTMGR_OutputTelemetry(void)
{

   int     SocketStatus;
   int32   SbStatus;
   uint16  NumPktsOutput  = 0;
   uint32  NumBytesOutput = 0;
   size_t  EdsDataSize;
   
   CFE_MSG_ApId_t   AppId;
   CFE_MSG_Size_t   MsgLen;
   OS_SockAddr_t    SocketAddr;
   CFE_SB_Buffer_t  *SbBufPtr;


   OS_SocketAddrInit(&SocketAddr, OS_SocketDomain_INET);
   OS_SocketAddrFromString(&SocketAddr, PktMgr->TlmDestIp);
   OS_SocketAddrSetPort(&SocketAddr, PktMgr->TlmUdpPort);
    
   /*
   ** CFE_SB_RcvMsg returns CFE_SUCCESS when it gets a packet, otherwise
   ** no packet was received
   */
   do
   {

      SbStatus = CFE_SB_ReceiveBuffer(&SbBufPtr, PktMgr->TlmPipe, CFE_SB_POLL);
 
      if ( (SbStatus == CFE_SUCCESS) && (PktMgr->SuppressSend == false) )
      {
          
         if(PktMgr->DownlinkOn)
         {
            
            CFE_MSG_GetSize(&SbBufPtr->Msg, &MsgLen);
            CFE_MSG_GetApId(&SbBufPtr->Msg, &AppId);
            AppId = AppId & PKTTBL_APP_ID_MASK;            
            if (!PktUtil_IsPacketFiltered(&SbBufPtr->Msg, &(PktMgr->PktTbl.Data.Pkt[AppId].Filter)))
            {
            
               SbStatus = PackEdsOutputMessage(SocketBuffer, &SbBufPtr->Msg, SocketBufferLen, &EdsDataSize);
               
               if (SbStatus == CFE_SUCCESS)
               {
                  SocketStatus = OS_SocketSendTo(PktMgr->TlmSockId, SocketBuffer, EdsDataSize, &SocketAddr);
          
                  ++NumPktsOutput;
                  NumBytesOutput += MsgLen;
               }
               
            } /* End if packet is not filtered */
         } /* End if downlink enabled */
         else
         {
            SocketStatus = 0;
         } 
         
         if (SocketStatus < 0)
         {
             
            CFE_EVS_SendEvent(PKTMGR_SOCKET_SEND_ERR_EID,CFE_EVS_EventType_ERROR,
                              "Error sending packet on socket %s, port %d, status %d. Tlm output suppressed\n",
                              PktMgr->TlmDestIp, PktMgr->TlmUdpPort, SocketStatus);
            PktMgr->SuppressSend = true;
         }

      } /* End if SB received msg and output enabled */

   } while(SbStatus == CFE_SUCCESS);

   ComputeStats(NumPktsOutput, NumBytesOutput);

   return NumPktsOutput;
   
} /* End of PKTMGR_OutputTelemetry() */


/******************************************************************************
** Function: PKTMGR_RemoveAllPktsCmd
**
** Notes:
**   1. The cFE to_lab code unsubscribes the command and send HK MIDs. I'm not
**      sure why this is done and I'm not sure how the command is used. This 
**      command is intended to help manage TO telemetry packets.
*/
bool PKTMGR_RemoveAllPktsCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   uint16   AppId;
   uint16   PktCnt = 0;
   uint16   FailedUnsubscribe = 0;
   int32    Status;
   bool     RetStatus = true;

   for (AppId=0; AppId < PKTUTIL_MAX_APP_ID; AppId++)
   {
      
      if (PktMgr->PktTbl.Data.Pkt[AppId].MsgId != PKTTBL_UNUSED_MSG_ID)
      {
          
         ++PktCnt;

         Status = CFE_SB_Unsubscribe(CFE_SB_ValueToMsgId(PktMgr->PktTbl.Data.Pkt[AppId].MsgId), PktMgr->TlmPipe);
         if(Status != CFE_SUCCESS)
         {
             
            FailedUnsubscribe++;
            CFE_EVS_SendEvent(PKTMGR_REMOVE_ALL_PKTS_ERROR_EID, CFE_EVS_EventType_ERROR,
                              "Error removing message ID 0x%04X at table packet index %d. Unsubscribe status 0x%8X",
                              PktMgr->PktTbl.Data.Pkt[AppId].MsgId, AppId, Status);
         }

         PKTTBL_SetPacketToUnused(&(PktMgr->PktTbl.Data.Pkt[AppId]));

      } /* End if packet in use */

   } /* End AppId loop */

   CFE_EVS_SendEvent(KIT_TO_INIT_DEBUG_EID, KIT_TO_INIT_EVS_TYPE, 
                     "PKTMGR_RemoveAllPktsCmd() - About to flush pipe\n");
   FlushTlmPipe();
   CFE_EVS_SendEvent(KIT_TO_INIT_DEBUG_EID, KIT_TO_INIT_EVS_TYPE, 
                     "PKTMGR_RemoveAllPktsCmd() - Completed pipe flush\n");

   if (FailedUnsubscribe == 0)
   {
      
      CFE_EVS_SendEvent(PKTMGR_REMOVE_ALL_PKTS_SUCCESS_EID, CFE_EVS_EventType_INFORMATION,
                        "Removed %d table packet entries", PktCnt);
   }
   else
   {
      
      RetStatus = false;
      CFE_EVS_SendEvent(PKTMGR_REMOVE_ALL_PKTS_ERROR_EID, CFE_EVS_EventType_INFORMATION,
                        "Attempted to remove %d packet entries. Failed %d unsubscribes",
                        PktCnt, FailedUnsubscribe);
   }

   return RetStatus;

} /* End of PKTMGR_RemoveAllPktsCmd() */


/*******************************************************************
** Function: PKTMGR_RemovePktCmd
**
*/
bool PKTMGR_RemovePktCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   const PKTMGR_RemovePktCmdMsg_t *RemovePktCmd = (const PKTMGR_RemovePktCmdMsg_t *) MsgPtr;
   uint16  AppId;
   int32   Status;
   bool    RetStatus = true;
  
   
   AppId = RemovePktCmd->MsgId & PKTTBL_APP_ID_MASK;
  
   if (PktMgr->PktTbl.Data.Pkt[AppId].MsgId != PKTTBL_UNUSED_MSG_ID)
   {

      PKTTBL_SetPacketToUnused(&(PktMgr->PktTbl.Data.Pkt[AppId]));
      
      Status = CFE_SB_Unsubscribe(CFE_SB_ValueToMsgId(RemovePktCmd->MsgId), PktMgr->TlmPipe);
      if(Status == CFE_SUCCESS)
      {
         CFE_EVS_SendEvent(PKTMGR_REMOVE_PKT_SUCCESS_EID, CFE_EVS_EventType_INFORMATION,
                           "Succesfully removed message ID 0x%04X from the packet table",
                           RemovePktCmd->MsgId);
      }
      else
      {
         RetStatus = false;
         CFE_EVS_SendEvent(PKTMGR_REMOVE_PKT_ERROR_EID, CFE_EVS_EventType_ERROR,
                           "Removed message ID 0x%04X from packet table, but SB unsubscribe failed with return status 0x%8x",
                           RemovePktCmd->MsgId, Status);
      }

   } /* End if found message ID in table */
   else
   {

      CFE_EVS_SendEvent(PKTMGR_REMOVE_PKT_ERROR_EID, CFE_EVS_EventType_ERROR,
                        "Error removing message ID 0x%04X. Packet not defined in packet table.",
                        RemovePktCmd->MsgId);

   } /* End if didn't find message ID in table */

   return RetStatus;

} /* End of PKTMGR_RemovePktCmd() */


/******************************************************************************
** Function:  PKTMGR_ResetStatus
**
*/
void PKTMGR_ResetStatus(void)
{

   PKTMGR_InitStats(0,INITBL_GetIntConfig(PktMgr->IniTbl, CFG_PKTMGR_STATS_CONFIG_DELAY));

} /* End PKTMGR_ResetStatus() */


/*******************************************************************
** Function: PKTMGR_SendPktTblTlmCmd
**
** Any command 
*/
bool PKTMGR_SendPktTblTlmCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   const PKTMGR_SendPktTblTlmCmdMsg_t *SendPktTblTlmCmd = (const PKTMGR_SendPktTblTlmCmdMsg_t *) MsgPtr;
   uint16        AppId;
   PKTTBL_Pkt_t* PktPtr;
   int32         Status;
  
  
   AppId  = SendPktTblTlmCmd->MsgId & PKTTBL_APP_ID_MASK;
   PktPtr = &(PktMgr->PktTbl.Data.Pkt[AppId]);
   
   PktMgr->PktTlm.MsgId  = PktPtr->MsgId;
   PktMgr->PktTlm.Qos    = PktPtr->Qos;
   PktMgr->PktTlm.BufLim = PktPtr->BufLim;

   PktMgr->PktTlm.FilterType  = PktPtr->Filter.Type;
   PktMgr->PktTlm.FilterParam = PktPtr->Filter.Param;

   CFE_SB_TimeStampMsg(CFE_MSG_PTR(PktMgr->PktTlm));
   Status = CFE_SB_TransmitMsg(CFE_MSG_PTR(PktMgr->PktTlm), true);
    
   return (Status == CFE_SUCCESS);

} /* End of PKTMGR_SendPktTblTlmCmd() */


/******************************************************************************
** Function: PKTMGR_UpdateFilterCmd
**
** Notes:
**   1. Command rejected if AppId packet entry has not been loaded 
**   2. The filter type is verified but the filter parameter values are not 
** 
*/
bool PKTMGR_UpdateFilterCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   const PKTMGR_UpdateFilterCmdMsg_t *UpdateFilterCmd = (const PKTMGR_UpdateFilterCmdMsg_t *) MsgPtr;
   bool    RetStatus = false;
   uint16  AppId;

   
   AppId = UpdateFilterCmd->MsgId & PKTTBL_APP_ID_MASK;
   
   if (PktMgr->PktTbl.Data.Pkt[AppId].MsgId != PKTTBL_UNUSED_MSG_ID)
   {
      
      if (PktUtil_IsFilterTypeValid(UpdateFilterCmd->FilterType))
      {
        
         PktUtil_Filter_t *TblFilter = &(PktMgr->PktTbl.Data.Pkt[AppId].Filter);
         
         CFE_EVS_SendEvent(PKTMGR_UPDATE_FILTER_CMD_SUCCESS_EID, CFE_EVS_EventType_INFORMATION,
                           "Successfully changed message ID 0x%04X's filter (Type,N,X,O) from (%d,%d,%d,%d) to (%d,%d,%d,%d)",
                           UpdateFilterCmd->MsgId,
                           TblFilter->Type, TblFilter->Param.N, TblFilter->Param.X, TblFilter->Param.O,
                           UpdateFilterCmd->FilterType,   UpdateFilterCmd->FilterParam.N,
                           UpdateFilterCmd->FilterParam.X,UpdateFilterCmd->FilterParam.O);
                           
         TblFilter->Type  = UpdateFilterCmd->FilterType;
         TblFilter->Param = UpdateFilterCmd->FilterParam;         
        
         RetStatus = true;
      
      } /* End if valid packet filter type */
      else
      {
   
         CFE_EVS_SendEvent(PKTMGR_UPDATE_FILTER_CMD_ERR_EID, CFE_EVS_EventType_ERROR,
                           "Error updating filter for message ID 0x%04X. Invalid filter type %d",
                           UpdateFilterCmd->MsgId, UpdateFilterCmd->FilterType);
      }
   
   } /* End if packet entry unused */
   else
   {
   
      CFE_EVS_SendEvent(PKTMGR_UPDATE_FILTER_CMD_ERR_EID, CFE_EVS_EventType_ERROR,
                        "Error updating filter for message ID 0x%04X. Packet not in use",
                        UpdateFilterCmd->MsgId);
   }
   
   return RetStatus;

} /* End of PKTMGR_UpdateFilterCmd() */


/******************************************************************************
** Function:  ComputeStats
**
** Called each output telemetry cycle
*/
static void ComputeStats(uint16 PktsSent, uint32 BytesSent)
{

   uint32 DeltaTimeMicroSec;   
   CFE_TIME_SysTime_t CurrTime = CFE_TIME_GetTime();
   CFE_TIME_SysTime_t DeltaTime;
   
   if (PktMgr->Stats.InitCycles > 0)
   {
   
      --PktMgr->Stats.InitCycles;
      PktMgr->Stats.PrevTime = CFE_TIME_GetTime();
      PktMgr->Stats.State = PKTMGR_STATS_INIT_CYCLE;

   }
   else
   {
      
      DeltaTime = CFE_TIME_Subtract(CurrTime, PktMgr->Stats.PrevTime);
      DeltaTimeMicroSec = CFE_TIME_Sub2MicroSecs(DeltaTime.Subseconds); 
      
      PktMgr->Stats.IntervalMilliSecs += (double)DeltaTime.Seconds*1000.0 + (double)DeltaTimeMicroSec/1000.0;
      PktMgr->Stats.IntervalPkts      += PktsSent;
      PktMgr->Stats.IntervalBytes     += BytesSent;

      if (PktMgr->Stats.IntervalMilliSecs >= PktMgr->Stats.OutputTlmInterval)
      {
      
         double Seconds = PktMgr->Stats.IntervalMilliSecs/1000;
         
         CFE_EVS_SendEvent(PKTMGR_DEBUG_EID, CFE_EVS_EventType_DEBUG,
                           "IntervalSecs=%f, IntervalPkts=%d, IntervalBytes=%d\n",
                           Seconds,PktMgr->Stats.IntervalPkts,PktMgr->Stats.IntervalBytes);
        
         PktMgr->Stats.AvgPktsPerSec  = (double)PktMgr->Stats.IntervalPkts/Seconds;
         PktMgr->Stats.AvgBytesPerSec = (double)PktMgr->Stats.IntervalBytes/Seconds;
         
         /* Good enough running average that avoids overflow */
         if (PktMgr->Stats.State == PKTMGR_STATS_INIT_CYCLE) {
     
            PktMgr->Stats.State = PKTMGR_STATS_INIT_INTERVAL;
       
         }
         else
         {
            
            PktMgr->Stats.State = PKTMGR_STATS_VALID;
            PktMgr->Stats.AvgPktsPerSec  = (PktMgr->Stats.AvgPktsPerSec  + PktMgr->Stats.PrevIntervalAvgPkts) / 2.0; 
            PktMgr->Stats.AvgBytesPerSec = (PktMgr->Stats.AvgBytesPerSec + PktMgr->Stats.PrevIntervalAvgBytes) / 2.0; 
  
         }
         
         PktMgr->Stats.PrevIntervalAvgPkts  = PktMgr->Stats.AvgPktsPerSec;
         PktMgr->Stats.PrevIntervalAvgBytes = PktMgr->Stats.AvgBytesPerSec;
         
         PktMgr->Stats.IntervalMilliSecs = 0.0;
         PktMgr->Stats.IntervalPkts      = 0;
         PktMgr->Stats.IntervalBytes     = 0;
      
      } /* End if report cycle */
      
      PktMgr->Stats.PrevTime = CFE_TIME_GetTime();
      
   } /* End if not init cycle */
   

} /* End ComputeStats() */


/******************************************************************************
** Function: DestructorCallback
**
** This function is called when the app is killed. This should
** never occur but if it does this will close the network socket.
*/
static void DestructorCallback(void)
{

   CFE_EVS_SendEvent(PKTMGR_DESTRUCTOR_INFO_EID, CFE_EVS_EventType_INFORMATION, 
                     "Destructor callback -- Closing TO Network socket. Downlink on = %d\n",
                     PktMgr->DownlinkOn);
   
   if (PktMgr->DownlinkOn)
   {
      
      OS_close(PktMgr->TlmSockId);
   
   }

} /* End DestructorCallback() */


/******************************************************************************
** Function: FlushTlmPipe
**
** Remove all of the packets from the input pipe.
**
*/
static void FlushTlmPipe(void)
{

   int32 SbStatus;
   CFE_SB_Buffer_t  *SbBufPtr;

   do
   {
      SbStatus = CFE_SB_ReceiveBuffer(&SbBufPtr, PktMgr->TlmPipe, CFE_SB_POLL);

   } while(SbStatus == CFE_SUCCESS);

} /* End FlushTlmPipe() */
   

/******************************************************************************
** Function: LoadPktTbl
**
** Notes:
**   1. Function signature must match the PKTTBL_LoadNewTbl_t definition
**   2. After the previous table's subscriptions are removed the new table is
**      copied into the working table data structure. However there could still
**      be subscription errors because of invalid table data so in a sense  
*/
static bool LoadPktTbl(PKTTBL_Data_t* NewTbl)
{

   uint16  AppId;
   uint16  PktCnt = 0;
   uint16  FailedSubscription = 0;
   int32   Status;
   bool    RetStatus = true;

   CFE_MSG_Message_t *MsgPtr = NULL;

   PKTMGR_RemoveAllPktsCmd(NULL, MsgPtr);  /* Both parameters are unused so OK to be NULL */

   CFE_PSP_MemCpy(&(PktMgr->PktTbl), NewTbl, sizeof(PKTTBL_Data_t));

   for (AppId=0; AppId < PKTUTIL_MAX_APP_ID; AppId++) {

      if (PktMgr->PktTbl.Data.Pkt[AppId].MsgId != PKTTBL_UNUSED_MSG_ID) {
         
         ++PktCnt;
         Status = SubscribeNewPkt(&(PktMgr->PktTbl.Data.Pkt[AppId])); 

         if(Status != CFE_SUCCESS) {
            
            ++FailedSubscription;
            CFE_EVS_SendEvent(PKTMGR_LOAD_TBL_SUBSCRIBE_ERR_EID,CFE_EVS_EventType_ERROR,
                              "Error subscribing to message ID 0x%04X, BufLim %d, Status %i",
                              PktMgr->PktTbl.Data.Pkt[AppId].MsgId, 
                              PktMgr->PktTbl.Data.Pkt[AppId].BufLim, Status);
         }
      }

   } /* End pkt loop */

   if (FailedSubscription == 0) {
      
      PKTMGR_InitStats(INITBL_GetIntConfig(PktMgr->IniTbl, CFG_APP_RUN_LOOP_DELAY),
                       INITBL_GetIntConfig(PktMgr->IniTbl, CFG_PKTMGR_STATS_INIT_DELAY));
      CFE_EVS_SendEvent(PKTMGR_LOAD_TBL_INFO_EID, CFE_EVS_EventType_INFORMATION,
                        "Successfully loaded new table with %d packets", PktCnt);
   }
   else {
      
      RetStatus = false;
      CFE_EVS_SendEvent(PKTMGR_LOAD_TBL_ERR_EID, CFE_EVS_EventType_INFORMATION,
                        "Attempted to load new table with %d packets. Failed %d subscriptions",
                        PktCnt, FailedSubscription);
   }

   return RetStatus;

} /* End LoadPktTbl() */


/******************************************************************************
** Function: PackEdsOutputMessage
**
** Notes:
**   1. Adopted from NASA"S cfE-eds-framework TO_LAB app
**
*/
static int32 PackEdsOutputMessage(void *DestBuffer, const CFE_MSG_Message_t *SrcBuffer, 
                                  size_t SrcBufferSize, size_t *EdsDataSize)
{
    EdsLib_Id_t                           EdsId;
    EdsLib_DataTypeDB_TypeInfo_t          TypeInfo;
    CFE_SB_SoftwareBus_PubSub_Interface_t PubSubParams;
    CFE_SB_Publisher_Component_t          PublisherParams;
    uint16                                TopicId;
    int32                                 Status;
    size_t                                SrcMsgSize;

    const EdsLib_DatabaseObject_t *EDS_DB = CFE_Config_GetObjPointer(CFE_CONFIGID_MISSION_EDS_DB);

    CFE_MSG_GetSize(SrcBuffer, &SrcMsgSize);

    CFE_MissionLib_Get_PubSub_Parameters(&PubSubParams, &SrcBuffer->BaseMsg);
    CFE_MissionLib_UnmapPublisherComponent(&PublisherParams, &PubSubParams);
    TopicId = PublisherParams.Telemetry.TopicId;

    Status = CFE_MissionLib_GetArgumentType(&CFE_SOFTWAREBUS_INTERFACE, CFE_SB_Telemetry_Interface_ID, TopicId, 1, 1,
                                            &EdsId);
    if (Status != CFE_MISSIONLIB_SUCCESS)
    {
        return CFE_STATUS_UNKNOWN_MSG_ID;
    }

    Status = EdsLib_DataTypeDB_PackCompleteObject(EDS_DB, &EdsId, DestBuffer, SrcBuffer, 8 * SrcBufferSize,
                                                  SrcMsgSize);
    if (Status != EDSLIB_SUCCESS)
    {
        return CFE_SB_INTERNAL_ERR;
    }

    Status = EdsLib_DataTypeDB_GetTypeInfo(EDS_DB, EdsId, &TypeInfo);
    if (Status != EDSLIB_SUCCESS)
    {
        return CFE_SB_INTERNAL_ERR;
    }

    *EdsDataSize = (TypeInfo.Size.Bits + 7) / 8;
    
    return CFE_SUCCESS;
}


/******************************************************************************
** Function: SubscribeNewPkt
**
*/
static int32 SubscribeNewPkt(PKTTBL_Pkt_t *NewPkt)
{

   int32 Status;

   Status = CFE_SB_SubscribeEx(CFE_SB_ValueToMsgId(NewPkt->MsgId), PktMgr->TlmPipe, NewPkt->Qos, NewPkt->BufLim);

   return Status;

} /* End SubscribeNewPkt(() */


