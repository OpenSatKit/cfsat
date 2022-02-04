/*
** Purpose: Define platform configurations for the @Template@ application
**
** Notes:
**   1. Compile-time configurations that are applied to each platform
**      deployment of this app.
**   2. These definitions should minimal and only contain parameters that
**      need to be configurable at compile time that allows users to tuned
**      the app for a particular platform. Use @template@_mission_cfg.h
**      for compile-time parameters that need to be tuned at the mission
**      level across all platform deployments. Use app_cfg.h for compile-time
**      parameters that don't need to be configured when an app is deployed
**      but are useful to be parameterized for maintaining the app and use
**      the JSON initialization file for parameters that can be defined at
**      runtime.
**
** References:
**   1. OpenSatKit Object-based Application Developer's Guide
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

#ifndef _@template@_platform_cfg_
#define _@template@_platform_cfg_


/*
** Includes
*/

#include "@template@_mission_cfg.h"
#include "@template@_msgids.h"


/*
** Platform Deployment Configurations
*/

#define @TEMPLATE@_PLATFORM_REV   0
#define @TEMPLATE@_INI_FILENAME   "/cf/@template@_ini.json"


#endif /* _@template@_platform_cfg_ */
