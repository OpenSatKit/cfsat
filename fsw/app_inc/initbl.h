/*
** Purpose: Define JSON Initialization table API
**
** Notes:
**   1. The enumeration macro design is from 
**      https://stackoverflow.com/questions/147267/easy-way-to-use-variables-of-enum-types-as-string-in-c/202511 
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
#ifndef _ini_tbl_
#define _ini_tbl_

/*
** Include Files
*/

#include "osk_c_fw.h" /* Needs JSON with FW config so just include everything */


/***********************/
/** Macro Definitions **/
/***********************/

/*
** Table Structure Objects 
*/

#define INITBL_JSON_CONFIG_OBJ_PREFIX  "config."

#define INITBL_CONFIG_DEF_ERR_EID  (INITBL_BASE_EID + 0)
#define INITBL_CFG_PARAM_EID       (INITBL_BASE_EID + 1)
#define INITBL_CFG_PARAM_ERR_EID   (INITBL_BASE_EID + 2)
#define INITBL_LOAD_JSON_EID       (INITBL_BASE_EID + 3)
#define INITBL_LOAD_JSON_ERR_EID   (INITBL_BASE_EID + 4)

/**********************/
/** Type Definitions **/
/**********************/

typedef struct 
{
   
   uint32   Int;
   char     Str[INITBL_MAX_CFG_STR_LEN];

} INITBL_CfgData_t;


typedef struct 
{
 
   INILIB_CfgEnum_t  CfgEnum;
   INITBL_CfgData_t  CfgData[INITBL_MAX_CFG_ITEMS+1];  /* '+1' accounts for [0] being unused */
   
   size_t      JsonParamCnt;
   CJSON_Obj_t JsonParams[INITBL_MAX_CFG_ITEMS+1];       /* Indexed via cfg param; '+1' accounts for [0] being ununsed */

   size_t      JsonFileLen;
   char        JsonBuf[INITBL_MAX_JSON_FILE_CHAR];   

} INITBL_Class_t;
 
 
/******************************************************************************
** Function: INITBL_Constructor
**
** Notes:
**    1. This must be called prior to any other functions
**    2. Reads, validates, and processes the JSON file. If construction is
**       successful then the query functions below can be used using the
**       "CFG_" parameters defined in app_cfg.h.
**
*/
bool INITBL_Constructor(INITBL_Class_t* IniTbl, const char* IniFile,
                        INILIB_CfgEnum_t* CfgEnum);


/******************************************************************************
** Function: INITBL_GetIntConfig
**
** Notes:
**    1. This does not return a status as to whether the configuration 
**       parameter was successfully retrieved. The logic for retreiving
**       parameters should be simple and any issues should be resolved during
**       testing.
**    2. Param is one of the JSON init file "CFG_" configuration parameters
**       defined in ap_cfg.h.  If the parameter is out of range or of the wrong
**       type, a zero is returned and an event message is sent. If the 
**       parameters are defined correctly they should neverbe out of range. 
**
*/
uint32 INITBL_GetIntConfig(const INITBL_Class_t* IniTbl, uint16 Param);


/******************************************************************************
** Function: INITBL_GetStrConfig
**
** Notes:
**    1. This does not return a status as to whether the configuration 
**       parameter was successfully retrieved. The logic for retreiving
**       parameters should be simple and any issues should be resolved during
**       testing.
**    2. Param is one of the JSON init file "CFG_" configuration parameters
**       defined in ap_cfg.h.  If the parameter is out of range or of the wrong
**       type, a zero is returned and an event message is sent. If the 
**       parameters are defined correctly they should neverbe out of range. 
**
*/
const char* INITBL_GetStrConfig(const INITBL_Class_t* IniTbl, uint16 Param);


#endif /* _ini_tbl_ */
