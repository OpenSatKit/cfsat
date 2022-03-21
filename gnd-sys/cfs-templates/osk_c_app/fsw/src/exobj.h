/*
** Purpose: Manage logging message headers to a text file
**          and playing them back in telemetry
**
** Notes:
**   1. Generated from the Hello World app template using the 
**      OSK C Application Framework 
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


#ifndef _exobj_
#define _exobj_

/*
** Includes
*/

#include "app_cfg.h"
#include "exobjtbl.h"


/***********************/
/** Macro Definitions **/
/***********************/


/*
** Event Message IDs
*/

#define EXOBJ_SET_MODE_CMD_EID (EXOBJ_BASE_EID + 0)
#define EXOBJ_EXECUTE_EID      (EXOBJ_BASE_EID + 1)


/**********************/
/** Type Definitions **/
/**********************/

enum EXOBJ_CounterModeType
{
   EXOBJ_CounterModeType_INCREMENT = 1,
   EXOBJ_CounterModeType_DECREMENT = 2
};
typedef uint16 EXOBJ_CounterModeType_t;

typedef struct
{
   
   EXOBJ_CounterModeType_t  CounterMode;
   
} EXOBJ_CounterModeCmd_Payload_t;

/******************************************************************************
** Command Packets
** 
*/

typedef struct
{

   CFE_MSG_CommandHeader_t         CmdHeader;
   EXOBJ_CounterModeCmd_Payload_t  Payload;

} EXOBJ_SetModeCmdMsg_t;
#define EXOBJ_SET_MODE_CMD_DATA_LEN  (sizeof(EXOBJ_SetModeCmdMsg_t) - sizeof(CFE_MSG_CommandHeader_t))


/******************************************************************************
** EXOBJ_Class
*/

typedef struct
{


   /*
   ** State Data
   */

   EXOBJ_CounterModeType_t  CounterMode;
   uint16  CounterValue;
       
   /*
   ** Contained Objects
   */

   EXOBJTBL_Class_t  Tbl;
   
} EXOBJ_Class_t;



/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: EXOBJ_Constructor
**
** Initialize the example object to a known state
**
** Notes:
**   1. This must be called prior to any other function.
**
*/
void EXOBJ_Constructor(EXOBJ_Class_t* ExObjPtr, const INITBL_Class_t* IniTbl);


/******************************************************************************
** Function: EXOBJ_ResetStatus
**
** Reset counters and status flags to a known reset state.
**
** Notes:
**   1. Any counter or variable that is reported in HK telemetry that doesn't
**      change the functional behavior should be reset.
**
*/
void EXOBJ_ResetStatus(void);


/******************************************************************************
** Function: EXOBJ_SetModeCmd
**
*/
bool EXOBJ_SetModeCmd(void* DataObjPtr, const CFE_SB_Buffer_t *SbBufPtr);


/******************************************************************************
** Function: EXOBJ_Execute
**
** Perform periodic processing that is trigger by an 'execute' request message
** from the scheduler app.
*/
void EXOBJ_Execute(void);


#endif /* _exobj_ */
