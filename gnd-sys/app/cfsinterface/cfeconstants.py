"""


LICENSE:

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
"""

###############################################################################
#todo: These definitions should come from EDS

class Cfe():

    CFE_TIME_FLY_ON_EID     = 20
    CFE_TIME_FLY_OFF_EID    = 21
    CFE_EVS_NO_FILTER       = 0x0000
    CFE_EVS_FIRST_ONE_STOP  = 0xFFFF
                
    EVS_DEBUG_MASK    = 0b0001
    EVS_INFO_MASK     = 0b0010
    EVS_ERROR_MASK    = 0b0100
    EVS_CRITICAL_MASK = 0b1000
                

