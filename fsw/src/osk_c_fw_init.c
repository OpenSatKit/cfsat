/* 
** Purpose: Generic entry point function for OSK app framework library
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
** Includes
*/

#include "osk_c_fw_cfg.h"
#include "osk_c_fw_ver.h"

/*
** Exported Functions
*/

/******************************************************************************
** Entry function
**
*/
uint32 OSK_C_FW_LibInit(void)
{

   OS_printf("OSK C Application Framework Library Initialized. Version %d.%d.%d\n",
             OSK_C_FW_MAJOR_VER, OSK_C_FW_MINOR_VER, OSK_C_FW_LOCAL_REV);
   
   return OS_SUCCESS;

} /* End OSK_C_FW_LibInit() */

