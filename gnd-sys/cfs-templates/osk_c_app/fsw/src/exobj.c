/*
** Purpose: Implement the EXOBJ_Class methods
**
** Notes:
**   None
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


/*
** Include Files:
*/

#include <string.h>
#include "exobj.h"


/***********************/
/** Macro Definitions **/
/***********************/

/* Convenience macro */
#define TBL_OBJ (&(ExObj->Tbl))  


/*******************************/
/** Local Function Prototypes **/
/*******************************/

const char* CounterModeStr(EXOBJ_CounterModeType_t  CounterMode);

/**********************/
/** Global File Data **/
/**********************/

static EXOBJ_Class_t*  ExObj = NULL;



/******************************************************************************
** Function: EXOBJ_Constructor
**
*/
void EXOBJ_Constructor(EXOBJ_Class_t *ExObjPtr, const INITBL_Class_t* IniTbl)
{
 
   ExObj = ExObjPtr;

   CFE_PSP_MemSet((void*)ExObj, 0, sizeof(EXOBJ_Class_t));
    
   EXOBJTBL_Constructor(TBL_OBJ, IniTbl);

   ExObj->CounterMode  = EXOBJ_CounterModeType_INCREMENT;
   ExObj->CounterValue = ExObj->Tbl.Data.LowLimit;

} /* End MSGLOG_Constructor */


/******************************************************************************
** Function:  EXOBJ_ResetStatus
**
*/
void EXOBJ_ResetStatus()
{
 
   /* Nothing to do */
   
} /* End EXOBJ_ResetStatus() */


/******************************************************************************
** Function: EXOBJ_SetModeCmd
**
** Notes:
**   1. See file prologue for logging/playback logic.
*/
bool EXOBJ_SetModeCmd(void* DataObjPtr, const CFE_SB_Buffer_t *SbBufPtr)
{
   
   bool RetStatus = false;
   const EXOBJ_CounterModeCmd_Payload_t* Cmd = &((const EXOBJ_SetModeCmdMsg_t*)SbBufPtr)->Payload;
   EXOBJ_CounterModeType_t PrevMode = ExObj->CounterMode;
   
   if ((Cmd->CounterMode == EXOBJ_CounterModeType_INCREMENT) ||
       (Cmd->CounterMode == EXOBJ_CounterModeType_DECREMENT))
   {
   
      RetStatus = true;
      
      ExObj->CounterMode = Cmd->CounterMode;
      
      CFE_EVS_SendEvent (EXOBJ_SET_MODE_CMD_EID, CFE_EVS_EventType_INFORMATION,
                         "Counter mode changed from %s to %s",
                         CounterModeStr(PrevMode), CounterModeStr(ExObj->CounterMode));

   }
   else
   {
      CFE_EVS_SendEvent (EXOBJ_SET_MODE_CMD_EID, CFE_EVS_EventType_ERROR,
                         "Set counter mode rejected. Invalid mode %d",
                         Cmd->CounterMode);
   }
   
   return RetStatus;

} /* End EXOBJ_SetModeCmd() */


/******************************************************************************
** Function: EXOBJ_Execute
**
*/
void EXOBJ_Execute(void)
{
   
   if (ExObj->CounterMode == EXOBJ_CounterModeType_INCREMENT)
   {
      if (ExObj->CounterValue < ExObj->Tbl.Data.HighLimit)
      {
         ExObj->CounterValue++;
      }
      else
      {
         ExObj->CounterValue = ExObj->Tbl.Data.LowLimit;
      }
   } /* End if increment */
   else
   {
      if (ExObj->CounterValue > ExObj->Tbl.Data.LowLimit)
      {
         ExObj->CounterValue--;
      }
      else
      {
         ExObj->CounterValue = ExObj->Tbl.Data.HighLimit;
      }
   } /* End if decrement */
   
   
   CFE_EVS_SendEvent (EXOBJ_EXECUTE_EID, CFE_EVS_EventType_DEBUG,
                      "%s counter mode: Value %d", 
                      CounterModeStr(ExObj->CounterMode), ExObj->CounterValue);

   
} /* End EXOBJ_Execute() */


/******************************************************************************
** Function: CounterModeStr
**
** Type checking should enforce valid parameter
*/
const char* CounterModeStr(EXOBJ_CounterModeType_t  CounterMode)
{

   static const char* CounterModeEnumStr[] =
   {
      "UNDEFINED", 
      "INCREMENT",    /* EXOBJ_CounterModeType_INCREMENT */
      "DECREMENT"     /* EXOBJ_CounterModeType_DECREMENT */
   };
        
   return CounterModeEnumStr[CounterMode];

} /* End CounterModeStr() */


