/*
** Purpose: Manage command dispatching for an application
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

#ifndef _cmdmgr_
#define _cmdmgr_

/*
** Includes
*/

#include "osk_c_fw_cfg.h"


/***********************/
/** Macro Definitions **/
/***********************/

/*
** Event Message IDs
*/

#define CMDMGR_REG_INVALID_FUNC_CODE_ERR_EID       (CMDMGR_BASE_EID + 0)
#define CMDMGR_DISPATCH_UNUSED_FUNC_CODE_ERR_EID   (CMDMGR_BASE_EID + 1)
#define CMDMGR_DISPATCH_INVALID_CHECKSUM_ERR_EID   (CMDMGR_BASE_EID + 2)
#define CMDMGR_DISPATCH_INVALID_LEN_ERR_EID        (CMDMGR_BASE_EID + 3)
#define CMDMGR_DISPATCH_INVALID_FUNC_CODE_ERR_EID  (CMDMGR_BASE_EID + 4)
#define CMDMGR_TOTAL_EID  5

/**********************/
/** Type Definitions **/
/**********************/

typedef bool (*CMDMGR_CmdFuncPtr_t) (void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);

/*
** Alternate command counters allow an individual command to have its own 
** counters. The class counters are not incremented for the command. This
** is useful when commands are issued from onboard apps and incrementing 
** the class command counters may be confusing to ground operations. 
*/
typedef struct
{

   bool    Enabled;  /* Use alternate cmd counters */            
   uint16  Valid;    /* Number of valid messages received since init or reset */
   uint16  Invalid;  /* Number of invalid messages received since init or reset */

} CMDMGR_AltCnt_t;

/*
** Objects register their command functions so each command structure
** contains a pointer to the object's data and to the command function.
*/

typedef struct
{

   uint16               UserDataLen;    /* User data length in bytes  */
   void*                DataPtr;
   CMDMGR_CmdFuncPtr_t  FuncPtr; 

   CMDMGR_AltCnt_t      AltCnt;

} CMDMGR_Cmd_t;

typedef struct
{

   uint16        ValidCmdCnt;       /* Number of valid messages received since init or reset */
   uint16        InvalidCmdCnt;     /* Number of invalid messages received since init or reset */
   CMDMGR_Cmd_t  Cmd[CMDMGR_CMD_FUNC_TOTAL];

} CMDMGR_Class_t;

/************************/
/** Exported Functions **/
/************************/

/******************************************************************************
** Function: CMDMGR_Constructor
**
** Notes:
**    1. This function must be called prior to any other functions being
**       called using the same cmdmgr instance.
*/
void CMDMGR_Constructor(CMDMGR_Class_t* CmdMgr);


/******************************************************************************
** Function: CMDMGR_RegisterFunc
**
*/
bool CMDMGR_RegisterFunc(CMDMGR_Class_t* CmdMgr, uint16 FuncCode, void* ObjDataPtr, 
                         CMDMGR_CmdFuncPtr_t ObjFuncPtr, uint16 UserDataLen);


/******************************************************************************
** Function: CMDMGR_RegisterFuncAltCnt
**
** Register a command function that will increment its own private alternate
** command counters.
*/
bool CMDMGR_RegisterFuncAltCnt(CMDMGR_Class_t* CmdMgr, uint16 FuncCode, void* ObjDataPtr, 
                               CMDMGR_CmdFuncPtr_t ObjFuncPtr, uint16 UserDataLen);


/******************************************************************************
** Function: CMDMGR_ResetStatus
**
*/
void CMDMGR_ResetStatus(CMDMGR_Class_t* CmdMgr);


/******************************************************************************
** Function: CMDMGR_DispatchFunc
**
*/
bool CMDMGR_DispatchFunc(CMDMGR_Class_t* CmdMgr,  const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: CMDMGR_ValidBoolArg
**
** Use uint16 because commands use both uint8 and uint16 for booleans and test   
** is valid for casting to larger storage but not truncating to shorter storage
*/
bool CMDMGR_ValidBoolArg(uint16 BoolArg);


/******************************************************************************
** Function: CMDMGR_BoolStr
**
** Purpose: Return a pointer to a string describing the enumerated type
**
** Notes:
**   None
*/
const char* CMDMGR_BoolStr(bool BoolArg);

#endif /* _cmdmgr_ */
