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
    
    Notes:
      1. TODO - THIS IS A PROTOTYPE!!!!      
"""

import sys
import time
import os
import socket
import configparser

if __name__ == '__main__':
    sys.path.append('..')
    from cfeconstants  import Cfe
    from telecommand   import TelecommandScript
    from telemetry     import TelemetryMessage, TelemetryObserver, TelemetrySocketServer
    from cmdtlmprocess import CmdTlmProcess
else:
    from .cfeconstants  import Cfe
    from .telecommand   import TelecommandScript
    from .telemetry     import TelemetryMessage, TelemetryObserver, TelemetrySocketServer
    from .cmdtlmprocess import CmdTlmProcess
from tools import crc_32c, compress_abs_path, TextEditor

CCSDS   = 0
TIME    = 1
PAYLOAD = 2

# TODO - Temorary structure to try different script telemetry interfaces 
tlm_cvt = {

    'CFE_ES' : [ { 'AppId': 0,   'Sequence': 0   },
                 { 'Seconds': 0, 'Subseconds': 0 },
                 { 
                   'CommandCounter': 0,
                   'CommandErrorCounter': 0
                }]
    }


###############################################################################

class TelemetryCurrentValue(TelemetryObserver):
    """
    callback_functions
       [app_name] : {packet: [item list]} 
    
    """

    def __init__(self, tlm_server: TelemetrySocketServer, event_callback): 
        super().__init__(tlm_server)

        self.event_callback = event_callback
                
        for msg in self.tlm_server.tlm_messages:
            tlm_msg = self.tlm_server.tlm_messages[msg]
            self.tlm_server.add_msg_observer(tlm_msg, self)        
            print("TelemetryCurrentValue adding observer for %s: %s" % (tlm_msg.app_name, tlm_msg.msg_name))

        # Debug to help dertmine how to structure current value data       
        topics = self.tlm_server.get_topics()
        for topic in topics:
            #if topic != self.tlm_server.eds_mission.TOPIC_TLM_TITLE_KEY:
            if 'ES' in topic:
                print('***********topic: ' + str(topic))
                eds_id = self.tlm_server.eds_mission.get_eds_id_from_topic(topic)
                tlm_entry = self.tlm_server.eds_mission.get_database_entry(eds_id)
                tlm_obj = tlm_entry()
                print('***********tlm_entry = ' + str(tlm_obj))
                print('>>>> CCSDS: = ')
                for entry in tlm_obj.CCSDS:
                    print(str(entry))
                print('>>>> Sec: = ')
                for entry in tlm_obj.Sec:
                    print(str(entry))
                print('>>>> Payload: = ')
                for entry in tlm_obj.Payload:
                    print(str(entry))

    def update(self, tlm_msg: TelemetryMessage) -> None:
        """
        Receive telemetry updates
        """
        
        if tlm_msg.app_name == 'CFE_EVS':
            if tlm_msg.msg_name == 'LONG_EVENT_MSG':
                payload = tlm_msg.payload()
                pkt_id = payload.PacketID
                event_text = "FSW Event at %s: %s, %d - %s" % \
                             (str(tlm_msg.sec_hdr().Seconds), pkt_id.AppName, pkt_id.EventType, payload.Message)
                self.event_callback(event_text)

        elif tlm_msg.app_name == 'CFE_ES':
            payload = tlm_msg.payload()
            tlm_cvt['CFE_ES'][CCSDS]['AppId']     = tlm_msg.pri_hdr().AppId
            tlm_cvt['CFE_ES'][CCSDS]['Sequence']  = tlm_msg.pri_hdr().Sequence
            tlm_cvt['CFE_ES'][TIME]['Seconds']    = tlm_msg.sec_hdr().Seconds
            tlm_cvt['CFE_ES'][TIME]['Subseconds'] = tlm_msg.sec_hdr().Subseconds
            tlm_cvt['CFE_ES'][PAYLOAD]['CommandCounter']      = payload.CommandCounter
            tlm_cvt['CFE_ES'][PAYLOAD]['CommandErrorCounter'] = payload.CommandErrorCounter

          
###############################################################################

class ScriptRunner(CmdTlmProcess):
    """
    """
    def __init__(self, gnd_ip_addr, router_cmd_port, tlm_port, tlm_timeout):
        super().__init__(gnd_ip_addr, router_cmd_port, tlm_port, tlm_timeout)

        self.tlm_current_value = TelemetryCurrentValue(self.tlm_server, self.event_msg)
        self.tlm_server.execute()
    
    def event_msg(self, event_text):
        print('ScriptRunner received event: ' + event_text)
        
    def tlm_val(self):
        print("tlm_cvt['CFE_ES'][CCSDS]['Sequence'] = " + str(tlm_cvt['CFE_ES'][CCSDS]['Sequence']))
              
    def tlm_wait_thread(self, tlm_msg: TelemetryMessage) -> None:
        pass
        
    def run_script(self, script):
        """
        User script passed as a string parameter and it execute within the context of
        a ScriptRUnner object so it can access all of the methods 
        """
        exec(script)

    
###############################################################################

if __name__ == '__main__':

    print(f"Name of the script      : {sys.argv[0]=}")
    print(f"Arguments of the script : {sys.argv[1:]=}")
    #if len(sys.argv) > 1:
    #    script_file = sys.argv[1]
    #    #print ('filename = ' + filename)

    config = configparser.ConfigParser()
    config.read('../cfsat.ini')
    SCRIPT_PATH = config.get('PATHS','SCRIPT_PATH')
    print ("SCRIPT_PATH = " + SCRIPT_PATH)    
    demo_script_path = compress_abs_path(os.path.join(os.getcwd(), '..',SCRIPT_PATH))
    demo_script = os.path.join(demo_script_path, 'demo_script.py') 

    script_runner = ScriptRunner('127.0.0.1', 8000, 9000, 1.0)
    
    text_editor = TextEditor(demo_script, run_script_callback=script_runner.run_script)
    text_editor.execute()
    
    script_runner.shutdown()

