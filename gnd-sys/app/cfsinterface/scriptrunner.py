"""
Provide a simple environment to execute user python scripts. This is not intended to
replace a fuly featured ground system. cFSAT is an educational tool that should be
simple to utilize. cFSAT and more specifically the cfe-eds-framework can be used as 
the foundation to integrate the cFS with a ground system.

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

from telecommand import Telecommand, ScriptTelecommand
from telemetry import TelemetryMessage, TelemetryObserver, TelemetryServer


class ScriptRunner:
    """
    Provide a simple environment for automated cFSAT 
    """

    @abstractmethod
    def procedure(self):
        """
        
        """
        raise NotImplementedError
        


###############################################################################

def main():
    
    config = configparser.ConfigParser()
    config.read('../cfsat.ini')
    MISSION    = config.get('MISSION','EDS_NAME')
    CFS_TARGET = config.get('CFS_TARGET','EDS_NAME')
    HOST_ADDR  = config.get('CFS_TARGET','HOST_ADDR')
    CMD_PORT   = config.getint('CFS_TARGET','SEND_CMD_PORT')

    system_string = "Mission: %s, Target: %s, Host: %s, Command Port %d" % (MISSION, CFS_TARGET, HOST_ADDR, CMD_PORT)

    try:
        script_telecommand   = ScriptTelecommand(MISSION, CFS_TARGET, HOST_ADDR, CMD_PORT)      
        cmd_line_telecommand = CmdLineTelecommand(MISSION, CFS_TARGET, HOST_ADDR, CMD_PORT)
        logger.info("Telecommand object created for " + system_string)
        
    except RuntimeError:
        print("Error creating telecommand object for " + system_string)
        sys.exit(2)


    #script_telecommand.send_app_cmd('TO_LAB','EnableOutputCmd',{'dest_IP':'127.0.0.1'})
  
    cmd_line_telecommand.execute()


if __name__ == "__main__":
    main()

###############################################################################
        
def main():
    
    config = configparser.ConfigParser()
    config.read('cfsat-proto.ini')
    MISSION    = config.get('MISSION','EDS_NAME')
    CFS_TARGET = config.get('CFS_TARGET','EDS_NAME')
    HOST_ADDR  = config.get('CFS_TARGET','HOST_ADDR')
    TLM_PORT   = config.getint('CFS_TARGET','RECV_TLM_PORT')

    system_string = "Mission: %s, Target: %s, Host: %s, Telemetry Port %d" % (MISSION, CFS_TARGET, HOST_ADDR, TLM_PORT)
    try:
        telemetry_server = TelemetryServer(MISSION, CFS_TARGET, HOST_ADDR, TLM_PORT)
        cmd_line_telemetry = CmdLineTelemetry(telemetry_server, True)
        print ("Telemetry object created for " + system_string)
        
    except RuntimeError:
        print("Error creating telemetry object for " + system_string)
        sys.exit(2)

    #cmd_line_telemetry.reverse_eng()
    telemetry_server.execute()
    

if __name__ == "__main__":
    main()
