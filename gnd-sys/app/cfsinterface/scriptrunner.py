"""
    Copyright 2022 Open STEMware Foundation
    All Rights Reserved.

    This program is free software; you can modify and/or redistribute it under
    the terms of the GNU Affero General Public License as published by the Free
    Software Foundation; version 3 with attribution addendums as found in the
    LICENSE.txt

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more
    details.

    This program may also be used under the terms of a commercial or enterprise
    edition license of cFSAT if purchased from the copyright holder.

    Purpose:
        Provide a simple environment to execute user python scripts. This is not
        intended to replace a fuly featured ground system. cFSAT is an educational
        tool that should be simple to utilize. cFSAT and more specifically the 
        cfe-eds-framework can be used as the foundation to integrate the cFS with
        a ground system.
    
"""

from telecommand import Telecommand, ScriptTelecommand
from telemetry import TelemetryMessage, TelemetryObserver, TelemetryServer


class ScriptRunner:
    """
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
