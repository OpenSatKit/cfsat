/*******************************************************************************
**
**      GSC-18128-1, "Core Flight Executive Version 6.7"
**
**      Copyright (c) 2006-2019 United States Government as represented by
**      the Administrator of the National Aeronautics and Space Administration.
**      All Rights Reserved.
**
**      Licensed under the Apache License, Version 2.0 (the "License");
**      you may not use this file except in compliance with the License.
**      You may obtain a copy of the License at
**
**        http://www.apache.org/licenses/LICENSE-2.0
**
**      Unless required by applicable law or agreed to in writing, software
**      distributed under the License is distributed on an "AS IS" BASIS,
**      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**      See the License for the specific language governing permissions and
**      limitations under the License.
**
** File: sample_app.c
**
** Purpose:
**   This file contains the source code for the Sample App.
**
*******************************************************************************/

/*
** Include Files:
*/
#include "sample_app_events.h"
#include "sample_app.h"

#include "sample_app_eds_dispatcher.h"
#include "sample_app_eds_dictionary.h"

/*
 * Define a lookup table for SAMPLE app command codes
 */
static const SAMPLE_APP_Application_Component_Telecommand_DispatchTable_t SAMPLE_TC_DISPATCH_TABLE =
{
        .CMD =
        {
                .NoopCmd_indication = SAMPLE_APP_Noop,
                .ResetCountersCmd_indication = SAMPLE_APP_ResetCounters,
                .ProcessCmd_indication = SAMPLE_APP_Process
        },
        .SEND_HK =
        {
                .indication = SAMPLE_APP_ReportHousekeeping
        }
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*  Name:  SAMPLE_APP_ProcessCommandPacket                                    */
/*                                                                            */
/*  Purpose:                                                                  */
/*     This routine will process any packet that is received on the SAMPLE    */
/*     command pipe.                                                          */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void SAMPLE_APP_ProcessCommandPacket(CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_Status_t Status;
    CFE_SB_MsgId_t MsgId;
    CFE_MSG_Size_t MsgSize;
    CFE_MSG_FcnCode_t MsgFc;

    Status = SAMPLE_APP_Application_Component_Telecommand_Dispatch(
                    CFE_SB_Telecommand_indication_Command_ID,
                    SBBufPtr, &SAMPLE_TC_DISPATCH_TABLE);

    if (Status != CFE_SUCCESS)
    {
        CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);
        CFE_MSG_GetSize(&SBBufPtr->Msg, &MsgSize);
        CFE_MSG_GetFcnCode(&SBBufPtr->Msg, &MsgFc);
        ++SAMPLE_APP_Data.ErrCounter;

        if (Status == CFE_STATUS_UNKNOWN_MSG_ID)
        {
            CFE_EVS_SendEvent(SAMPLE_APP_INVALID_MSGID_ERR_EID, CFE_EVS_EventType_ERROR,
                              "SAMPLE: invalid command packet,MID = 0x%x", (unsigned int)CFE_SB_MsgIdToValue(MsgId));
        }
        else if (Status == CFE_STATUS_WRONG_MSG_LENGTH)
        {
            CFE_EVS_SendEvent(SAMPLE_APP_LEN_ERR_EID, CFE_EVS_EventType_ERROR,
                            "Invalid Msg length: ID = 0x%X,  CC = %u, Len = %u",
                            (unsigned int)CFE_SB_MsgIdToValue(MsgId), (unsigned int)MsgFc, (unsigned int)MsgSize);
        }
        else
        {
            CFE_EVS_SendEvent(SAMPLE_APP_COMMAND_ERR_EID, CFE_EVS_EventType_ERROR,
                              "SAMPLE: Invalid ground command code: CC = %d", (int)MsgFc);
        }
    }

}
