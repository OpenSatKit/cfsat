#!/usr/bin/env python
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
import os
import sys
sys.path.append(r'../../cfe-eds-framework/build/exe/lib/python')
if 'LD_LIBRARY_PATH' not in os.environ:
    print("LD_LIBRARY_PATH not defined. Run setvars.sh to corrrect the problem")
    sys.exit(1)
    """
    This code fails due to permission errors. It's also dangerous becuase it attempts to restart
    the python interpreter with a ne environment. That' why it's attempted immediately after the
    os and sys imports which define teh minimal context to attempt the restart 
    os.environ['LD_LIBRARY_PATH'] = '../../cfe-eds-framework/build/exe/lib'
    try:
        os.execv(sys.argv[0], sys.argv)
    except:
        print("Auto define LD_LIBRARY_PATH failed")
        sys.exit(1)
    """

import time
import socket
import configparser
import operator
import subprocess
import threading
import queue
import json
from datetime import datetime

import logging
from logging.config import fileConfig
fileConfig('logging.ini')
logger = logging.getLogger(__name__)

import PySimpleGUI as sg

from cfsinterface import Cfe, EdsMission
from cfsinterface import Telecommand, ScriptTelecommand
from cfsinterface import TelemetryMessage, TelemetryObserver, TelemetryServer
from tools import CreateApp, ManageTutorials


###############################################################################

class GuiTelecommand(Telecommand):
    '''
    GUI to manage a user selecting and sending a single telecommand 
    '''

    def __init__(self, mission, target, host_addr, cmd_port):
        super().__init__(mission, target, host_addr, cmd_port)

        '''
        eds_mission -
        eds_target  - String containing the identical target name used in the EDS  
        host        - String containing the socket address, e.g. '127.0.0.1:1234'
        port        - Integer containing socket port
        '''
        self.UNDEFINED_LIST = [self.eds_mission.NULL_STR]

        self.PAYLOAD_ROWS, self.PAYLOAD_COLS, self.PAYLOAD_HEADINGS = 8, 3, ('Parameter Name','Type','Value',)
        self.PAYLOAD_INPUT_START = 2 # First row of input payloads (see SendCmd() payload_layout comment)
        self.PAYLOAD_TEXT_INPUT  = "text"
        self.PAYLOAD_COMBO_INPUT = "combo"
        
        self.PAYLOAD_NAME_IDX  = 0
        self.PAYLOAD_TYPE_IDX  = 1
        self.PAYLOAD_VALUE_IDX = 2
        self.PAYLOAD_INPUT_IDX = 3

        self.payload_struct = None        # Payload structure for the current command. Null if no command selected
        self.payload_gui_entries = {}     # Payload entry information required to display and retrieve payload data from the GUI

        self.sg_values = None
        
    def get_payload_gui_entries(self, payload_struct):
        '''
        Create a list of a command's payload entries from the EDS

        Inputs:
        payload_struct - The payload structure output from mission_db.GetPayload()
        '''
        return_str = ""
        if isinstance(payload_struct, dict):
            return_str = "Recursively extracting dictionary"
            for item in list(payload_struct.keys()):
                return_str = self.get_payload_gui_entries(payload_struct[item])
        elif isinstance(payload_struct, list):
            return_str = "Recursively extracting list"
            for item in payload_struct:
                return_str = get_payload_gui_entries(item)
        elif isinstance(payload_struct, tuple):
            logger.debug("TUPLEDATA: "+str(payload_struct))
            return_str = "Extracting tuple entry: " + str(payload_struct)
            eds_name  = payload_struct[0]
            gui_name  = self.remove_eds_payload_name_prefix(eds_name)
            logger.debug("gui_name = " + str(gui_name))
            eds_entry = payload_struct[1] 
            eds_obj   = eds_entry()
            logger.debug("eds_entry:\n" + str(type(eds_entry)))
            logger.debug(str(dir(eds_entry)))
            logger.debug("eds_obj:\n" + str(type(eds_obj)))
            logger.debug(str(dir(eds_obj)))
            eds_obj_list = str(type(eds_obj)).split(',')
            logger.debug(str(eds_obj_list))
            gui_type  = eds_obj_list[1].replace("'","").replace(")","")
            gui_input = self.PAYLOAD_TEXT_INPUT
            if payload_struct[2] == 'entry':
                gui_value = ["--input value--"]
            elif payload_struct[2] == 'enum':
                gui_input = self.PAYLOAD_COMBO_INPUT
                gui_value = []
                for enum_value in list(payload_struct[3].keys()):
                    gui_value.append(enum_value)
            else:
                return_str = "Error extracting entries from payload structure tuple: " + str(payload_struct)
            logger.debug(gui_type)
            # The first two list entries for each list below must have same semantic meaning.
            # The eds_entry contains the full EDS type while entry_type has been parsed for display
            #todo: self.payload_entries.append([gui_name, ])
            self.payload_gui_entries[gui_name] = {'eds_entry': eds_entry, 'eds_name': eds_name,       
                                                  'gui_type': gui_type, 'gui_value': gui_value, 'gui_input': gui_input, 
                                                  'gui_value_key': self.eds_mission.NULL_STR}
            
        else:
            return_str = "Error extracting entries from unkown payload structure instance type: " + str(payload_struct)
        
        logger.debug("return_str="+return_str)
        return return_str


    def display_payload_entries(self, payload_gui_entries):
        '''
        See SendCmd() payload_layout definition comment for initial payload display
        When there are no payload paramaters (zero length) hide all of the except for the first parameter.
        '''
        
        for row in range(self.PAYLOAD_ROWS):
            self.window["-PAYLOAD_%d_NAME-"%row].update(visible=False)
            self.window["-PAYLOAD_%d_TYPE-"%row].update(visible=False)
            self.window["-PAYLOAD_%d_VALUE-"%row].update(visible=False, value=self.UNDEFINED_LIST[0])

        enum_row  = 0
        entry_row = self.PAYLOAD_INPUT_START
        row = 0

        if len(payload_gui_entries) > 0:
            for payload_gui_name in payload_gui_entries.keys():
                logger.debug("payload_gui_name = " + str(payload_gui_name))
                if payload_gui_entries[payload_gui_name]['gui_input'] == self.PAYLOAD_TEXT_INPUT:
                    row = entry_row
                    entry_row += 1                
                    self.window["-PAYLOAD_%d_VALUE-"%row].update(visible=True, value=payload_gui_entries[payload_gui_name]['gui_value'][0])
                    
                else:
                    row = enum_row
                    enum_row += 1                
                    payload_enum_list = payload_gui_entries[payload_gui_name]['gui_value']
                    self.window["-PAYLOAD_%d_VALUE-"%row].update(visible=True,value=payload_enum_list[0], values=payload_enum_list)
                
                payload_gui_entries[payload_gui_name]['gui_value_key'] = "-PAYLOAD_%d_VALUE-"%row
                self.window["-PAYLOAD_%d_NAME-"%row].update(visible=True,value=payload_gui_name)
                self.window["-PAYLOAD_%d_TYPE-"%row].update(visible=True,value=payload_gui_entries[payload_gui_name]['gui_type'])

        else:
            self.window["-PAYLOAD_0_NAME-"].update(visible=True, value='No Parameters')


    def load_payload_entry_value(self, payload_name, payload_eds_entry, payload_type, payload_list):
        logger.debug("load_payload_entry_value() - Entry")
        logger.debug("payload_name=%s, payload_eds_entry=%s, payload_type=%s, payload_list=%s"%(payload_name, payload_eds_entry, payload_type, payload_list))
        logger.debug("self.payload_gui_entries = " + str(self.payload_gui_entries))
        logger.debug("self.sg_values = " + str(self.sg_values))
        #todo: Add type check error reporting
        value_key = self.payload_gui_entries[self.remove_eds_payload_name_prefix(payload_name)]['gui_value_key']
        value = self.sg_values[value_key]
        return value
                    
                    
    def get_gui_payload_values(self, payload_gui_entries, values):
        '''
        Retrieve payload values from the command screen and returns a dictionary of values if
        the payload values are valid.

        Inputs:
        payload_entries - The payload structure output from mission_db.GetPayload()
        values - sg list of GUI values 
        TODO - Add error checking for GUI-payload_entries mismatch which should never happen but want to avoid an exception
        '''
        
        payload_valid  = True
        payload_values = {}
        logger.debug("payload_gui_entries = " + str(payload_gui_entries))
        for payload_gui_entry in payload_gui_entries:
            payload_gui_name = payload_gui_entry[self.PAYLOAD_NAME_IDX]
            payload_eds_name = payload_gui_entry[2]
            value_key = self.payload_gui_value_keys[payload_gui_name]
            value = values[value_key]
            logger.debug("Key %s value=%s type=%s"%(value_key,value,type(value)))
            if MissionDb.ValidPayload(payload_gui_entry[self.PAYLOAD_TYPE_IDX],value):
                payload_values[payload_eds_name] = value
            else:
                payload_valid = False
                payload_values['error'] = "Invalid value input for '{}'".format(payload_eds_name)
                
        return payload_valid, payload_values


    def execute(self, topic_name):
    
        cmd_sent = True
        cmd_text = "Send command aborted"
        cmd_status = ""

        topic_list = list(self.get_topics().keys())
        logger.debug("topic_list = " + str(topic_list))

        self.command_list = list(self.get_topic_commands(topic_name).keys())
        
        # This GUI is designed for application command topics. The ini file has a non-standard configuration that allows all
        # topics to be sent from teh GUI. Topics like 'send HK req' do not have subcommands so this alerts he users 
        if (len(self.command_list) == 1):
            sg.Popup('This is a topic-only command do not select a command/payload', title=topic_name, modal=False)
        
        # Create a layout with more than enough input and combo boxes. Then hide what's not needed for a particular command 
        # The top rows below self.PAYLOAD_INPUT_START are used for enumerated types with combo boxes and the remaining rows
        # are input boxes
        #todo: Had some GUI alignment issues with hiding. See https://github.com/PySimpleGUI/PySimpleGUI/issues/1154
        #todo: Other payload ideas: Create a new send command window. Search command list and tailor max input/combo to specific topic
        row_font = 'Courier 12'
        row_title_font = 'Courier 12 bold'
        row_label_size = (20,1)
        row_input_size = (20,1)
        self.payload_layout = [[sg.Text(heading, font=row_title_font, size=row_label_size,) for i, heading in enumerate(self.PAYLOAD_HEADINGS)]]
        for row in range(self.PAYLOAD_ROWS):
            if row < self.PAYLOAD_INPUT_START:
                self.payload_layout += [[sg.pin(sg.Text('Name', font=row_font, size=row_label_size, key="-PAYLOAD_%d_NAME-"%row))] + [sg.pin(sg.Text('Type', font=row_font, size=row_label_size, key="-PAYLOAD_%d_TYPE-"%row))] + [sg.pin(sg.Combo((self.UNDEFINED_LIST), font=row_font, size=row_input_size, enable_events=True, key="-PAYLOAD_%d_VALUE-"%row, default_value=self.UNDEFINED_LIST[0]))]]
            else:
                self.payload_layout += [[sg.pin(sg.Text('Name', font=row_font, size=row_label_size, key="-PAYLOAD_%d_NAME-"%row))] + [sg.pin(sg.Text('Type', font=row_font, size=row_label_size, key="-PAYLOAD_%d_TYPE-"%row))] + [sg.pin(sg.Input(self.UNDEFINED_LIST[0], font=row_font, size=row_input_size, enable_events=True, key="-PAYLOAD_%d_VALUE-"%row))]]

            
        #todo: [sg.Text('Topic', size=(10,1)),  sg.Combo((topic_list),   size=(40,1), enable_events=True, key='-TOPIC-',   default_value=topic_list[0])], 
        self.layout = [
                      [sg.Text('Command',size=(10,1)), sg.Combo((self.command_list), size=(40,1), enable_events=True, key='-COMMAND-', default_value=self.command_list[0])],
                      [sg.Text(' ')],
                      [sg.Col(self.payload_layout, size=(650, 250), scrollable=True, element_justification='l', expand_x=True, expand_y=True)],
                      [sg.Text(' ')],
                      [sg.Button('Send', enable_events=True, key='-SEND_CMD-',pad=((0,10),(1,1))), sg.Exit()]
                      ]
                 
        self.window = sg.Window("Send %s Telecommand" % topic_name, self.layout, element_padding=(1,1), default_element_size=(20,1)) #TODO - default_element_size=(14,1),  return_keyboard_events=True
                
        while True:
        
            self.event, self.values = self.window.read()
            logger.debug("Command Window Read()\nEvent: %s\nValues: %s" % (self.event, self.values))

            self.sg_values = self.values

            if self.event in (sg.WIN_CLOSED, 'Exit') or self.event is None:       
                break
            
            logger.debug("Matching event " + self.event)
            cmd_name   = self.values['-COMMAND-']
            logger.debug("Topic: %s, Command: %s" % (topic_name, cmd_name))
            
            if self.event == '-COMMAND-':
                if cmd_name != self.eds_mission.COMMAND_TITLE_KEY:
                    
                    cmd_id, cmd_text = self.get_cmd_id(cmd_name)
                    
                    if cmd_id !=0:
                       
                        cmd_valid, cmd_entry, cmd_obj = self.get_cmd_entry(topic_name, cmd_name)
                        cmd_has_payload, cmd_payload_item = self.get_cmd_entry_payload(cmd_entry)
                        logger.debug("self.cmd_entry = " + str(cmd_entry))
                        logger.debug("self.cmd_obj = " + str(cmd_obj))
                        logger.debug("cmd_payload_item = " + str(cmd_payload_item))
    
                        self.payload_struct = None 
                        self.payload_gui_entries = None
                        
                        if cmd_has_payload:
            
                            payload_entry = self.eds_mission.get_database_named_entry(cmd_payload_item[2])
                            payload = payload_entry()
                            self.payload_struct = self.get_payload_struct(payload_entry, payload, 'Payload')
                            logger.debug("payload_entry = " + str(payload_entry))
                            logger.debug("payload = " + str(payload))
                            self.payload_gui_entries = {}
                            status_str = self.get_payload_gui_entries(self.payload_struct)
                            logger.debug("status_str = " + status_str)
                            logger.debug("pself.payload_gui_entries: " + str(self.payload_gui_entries))
                            if len(self.payload_gui_entries) > 0:
                                self.display_payload_entries(self.payload_gui_entries)
                            else:
                                cmd_text = "Error extracting payload parameters from %s" % str(self.payload_struct)
                        else:
                            self.display_payload_entries([])
                
            if self.event == '-SEND_CMD-':

                if topic_name == self.eds_mission.TOPIC_CMD_TITLE_KEY:
                    cmd_text  = "Please select a topic before sending a command"
                    cmd_sent = False
                    break
                    
                if (cmd_name == self.eds_mission.COMMAND_TITLE_KEY and len(self.command_list) > 1):
                    cmd_text  = "Please select a command before sending a command"
                    cmd_sent = False
                    break
                
                topic_id, topic_text = self.get_topic_id(topic_name)
                
                cmd_valid, cmd_entry, cmd_obj = self.get_cmd_entry(topic_name, cmd_name)

                if cmd_valid == True:
    
                    logger.debug("self.cmd_entry = " + str(cmd_entry))
                    logger.debug("self.cmd_obj = " + str(cmd_obj))

                    self.set_cmd_hdr(topic_id, cmd_obj)

                    cmd_has_payload, cmd_payload_item = self.get_cmd_entry_payload(cmd_entry)
                    logger.debug("cmd_payload_item = " + str(cmd_payload_item))
            
                    if cmd_has_payload:
            
                        # Use the information from the database entry iterator to get a payload Entry and object
                        logger.debug("cmd_payload_item[1] = " + str(cmd_payload_item[1]))
                        logger.debug("cmd_payload_item[2] = " + str(cmd_payload_item[2]))
                        #todo: payload_entry = self.eds_mission.lib_db.DatabaseEntry(cmd_payload_item[1], cmd_payload_item[2])
                        payload_entry = self.eds_mission.get_database_named_entry(cmd_payload_item[2])
                        payload = payload_entry()
                        logger.debug("payload_entry = " + str(payload_entry))
                        logger.debug("payload = " + str(payload))

                        #todo: Check if None? payload_struct = self.get_payload_struct(payload_entry, payload, 'Payload')
                        eds_payload = self.set_payload_values(self.payload_struct)
                        payload = payload_entry(eds_payload)

                        cmd_obj['Payload'] = payload
    
                    (cmd_sent, cmd_text, cmd_status) = self.send_command(cmd_obj)
                    if cmd_sent:
                        cmd_status = "%s %s command sent" % (topic_name, cmd_name)
                    
                else:    
            
                    print("Error retrieving command %s using topic ID %d" % (cmd_name, topic_id)) 


                # Keep GUI active if a command error occurs to allow user to fixed and resend or cancel
                if cmd_sent:
                    break
                    
        self.window.close()

        return (cmd_sent, cmd_text, cmd_status)


###############################################################################

class GuiTelemetry(TelemetryObserver):
    '''
    Create a screen that displays a single telemetry message
    '''

    def __init__(self, tlm_server: TelemetryServer, topic_name):
        super().__init__(tlm_server)

        self.topic_name = topic_name
        self.tlm_msg = tlm_server.get_tlm_msg_from_topic(topic_name)

        self.payload_fmt_str = "{:<50}: {}\n"
        self.payload_str_max_len = 0
                
        self.paused = False
        self.payload_text = None
      
        self.event = None
        self.values = None
        self.lock = threading.Lock()
        self.gui_first_loop = True
      
        self.current_msg = None
        self.window = None
        
        self.gui_thread = threading.Thread(target=self.gui)
        self.gui_thread.kill = False
 
    def update(self, tlm_msg: TelemetryMessage) -> None:
        '''
        Receive telemetry updates
        self.payload_str_max_len is set as opposed to the construtor because an initial
        tlm_msg object does not have its eds_obj and eds_entry attributes set
        '''
        self.lock.acquire()
        if self.payload_str_max_len == 0:
            self.payload_str_max(self.tlm_msg.eds_obj, self.tlm_msg.eds_entry.Name)
            if self.payload_str_max_len > 0:
                self.payload_fmt_str = "{:<%d}: {}\n" % self.payload_str_max_len
                logger.info("%s: %s %d" % (self.topic_name, self.tlm_msg.msg_name, self.payload_str_max_len))

        self.current_msg = tlm_msg
        if not self.paused:
            logger.debug("%s %s received at %s" % (tlm_msg.app_name, tlm_msg.msg_name, str(tlm_msg.sec_hdr().Seconds)))
            self.window['-APP_ID-'].update(tlm_msg.pri_hdr().AppId)
            self.window['-LENGTH-'].update(tlm_msg.pri_hdr().Length)
            self.window['-SEQ_CNT-'].update(tlm_msg.pri_hdr().Sequence)
            self.window['-TIME-'].update(str(tlm_msg.sec_hdr().Seconds))
            self.payload_text = ""
            self.format_payload_text(tlm_msg.eds_obj, tlm_msg.eds_entry.Name)
            self.window['-PAYLOAD_TEXT-'].update(self.payload_text)  
            self.window['-TLM_UPDATE-'].click()
        self.lock.release()


    def format_payload_text(self, base_object, base_name):
        '''
        Recursive function that iterates over an EDS object and creates a string that can be displayed.
        '''
        # Array display string
        if (self.tlm_server.eds_mission.lib_db.IsArray(base_object)):
            for i in range(len(base_object)):
                self.format_payload_text(base_object[i], f"{base_name}[{i}]")
        # Container display string
        elif (self.tlm_server.eds_mission.lib_db.IsContainer(base_object)):
            for item in base_object:
                self.format_payload_text(item[1], f"{base_name}.{item[0]}")
        # Everything else (number, enumeration, string, etc.)
        else:
            if '.Payload.' in base_name:
                self.payload_text += self.payload_fmt_str.format(base_name, base_object)

    def payload_str_max(self, base_object, base_name):
        '''
        Recursive function that determines the longest payload string. This
        is helpful for formatting displayes .
        '''
        # Array display string
        if (self.tlm_server.eds_mission.lib_db.IsArray(base_object)):
            for i in range(len(base_object)):
                self.payload_str_max(base_object[i], f"{base_name}[{i}]")
        # Container display string
        elif (self.tlm_server.eds_mission.lib_db.IsContainer(base_object)):
            for item in base_object:
                self.payload_str_max(item[1], f"{base_name}.{item[0]}")
        # Everything else (number, enumeration, string, etc.)
        else:
            if '.Payload.' in base_name:
                base_name_len = len(base_name)
                if base_name_len > self.payload_str_max_len:
                    self.payload_str_max_len = base_name_len
               

    def gui(self):

        thread = threading.currentThread()
        
        hdr_label_font = ('Arial bold',12)
        hdr_value_font = ('Arial',12)
                
        self.layout = [[sg.Text('App ID: ', font=hdr_label_font),  sg.Text('--Undefined--', font=hdr_value_font, size=(12,1), key='-APP_ID-'), 
                        sg.Text('Length: ', font=hdr_label_font),  sg.Text('--Undefined--', font=hdr_value_font, size=(12,1), key='-LENGTH-'),
                        sg.Text('Seq Cnt: ', font=hdr_label_font), sg.Text('--Undefined--', font=hdr_value_font, size=(12,1), key='-SEQ_CNT-'),
                        sg.Text('Time: ', font=hdr_label_font),    sg.Text('--Undefined--', font=hdr_value_font, size=(12,1), key='-TIME-')],
                       [sg.Text('')], 
                       [sg.Text('Payload', font = ('Arial bold',14)), sg.Text('', font=hdr_value_font, key='-PAUSED-', pad=(10,0))],
                       [sg.MLine(default_text='-- No Messages Received --', font = ('Courier',12), enable_events=True, size=(65, 30), key='-PAYLOAD_TEXT-')],
                       [sg.Button('Pause'), sg.Button('Resume'), sg.Button('Close'), sg.Button('', key='-TLM_UPDATE-', visible=False)]]

        self.window = sg.Window(self.topic_name, self.layout, resizable=True, grab_anywhere=True)


        while not self.gui_thread.kill:  # Event Loop
            
            #Big kludge for multi-threaded tlm windows
            try:
                self.event, self.values = self.window.read(timeout=500)
            except (RuntimeError, AttributeError):
                #print("GI read loop exception")
                self.event = "void"
            logger.debug("GUI Window Read()\nEvent: %s\nValues: %s" % (self.event, self.values))

            if self.gui_first_loop:
                # Attach self.update() observer. Must be done after the first read
                self.tlm_server.add_msg_observer(self.tlm_msg, self)
            self.gui_first_loop = False
            
            if self.event in (sg.WIN_CLOSED, 'Close') or self.event is None:       
                break
            
            if self.event == 'Pause':
                self.paused = True
                self.window['-PAUSED-'].update('Display Paused')

            if self.event == 'Resume':
                self.paused = False
                self.window['-PAUSED-'].update('')

            if self.event == '-TLM_UPDATE-':
                #Keep hook becuase may need event loop context for updates
                #print("telemetry update click")
                '''
                if not self.paused:
                    logger.debug("%s %s received at %s" % (tlm_msg.app_name, tlm_msg.msg_name, str(tlm_msg.sec_hdr().Seconds)))
                    self.window['-APP_ID-'].update(tlm_msg.pri_hdr().AppId)
                    self.window['-LENGTH-'].update(tlm_msg.pri_hdr().Length)
                    self.window['-SEQ_CNT-'].update(tlm_msg.pri_hdr().Sequence)
                    self.window['-TIME-'].update(str(tlm_msg.sec_hdr().Seconds))
                    self.payload_text = ""
                    self.format_payload_text(tlm_msg.eds_obj, tlm_msg.eds_entry.Name)
                    self.window['-PAYLOAD_TEXT-'].update(self.payload_text)
                
                pass
                '''
        self.tlm_server.remove_msg_observer(self.tlm_msg, self)
        self.window.close()
        #thread.join()


    def execute(self):
        self.gui_thread.start()
        #self.gui()

             
    def shutdown(self):
        self.gui_thread.kill = True
        logger.info("%s GUI shutting down" % self.topic_name)


###############################################################################

class SystemTelemetry(TelemetryObserver):
    '''
    callback_functions
       [app_name] : {packet: [item list]} 
    
    '''

    def __init__(self, tlm_server: TelemetryServer, tlm_monitors, tlm_callback, event_queue):
        super().__init__(tlm_server)

        self.tlm_monitors = tlm_monitors
        self.tlm_callback = tlm_callback
        self.event_queue  = event_queue
        
        self.sys_apps = ['CFE_ES', 'CFE_EVS', 'CFE_SB', 'CFE_TBL', 'CFE_TIME', 'OSK_C_DEMO']
        
        for msg in self.tlm_server.tlm_messages:
            tlm_msg = self.tlm_server.tlm_messages[msg]
            if tlm_msg.app_name in self.sys_apps:
                self.tlm_server.add_msg_observer(tlm_msg, self)        
                logger.info("system telemetry adding observer for %s: %s" % (tlm_msg.app_name, tlm_msg.msg_name))
        

    def update(self, tlm_msg: TelemetryMessage) -> None:
        '''
        Receive telemetry updates
        '''
        #todo: Deteminer best tlm identification method: if int(tlm_msg.app_id) == int(self.cfe_es_hk.app_id):
        
        if tlm_msg.app_name in self.tlm_monitors:
            self.tlm_callback(tlm_msg.app_name, tlm_msg.msg_name, "Seconds", str(tlm_msg.sec_hdr().Seconds))

        elif tlm_msg.app_name == 'CFE_EVS':
            if tlm_msg.msg_name == 'LONG_EVENT_MSG':
                payload = tlm_msg.payload()
                pkt_id = payload.PacketID
                event_text = "FSW Event at %s: %s, %d - %s" % \
                             (str(tlm_msg.sec_hdr().Seconds), pkt_id.AppName, pkt_id.EventType, payload.Message)
                self.event_queue.put_nowait(event_text)
                '''        
                LongEventTlm.Payload.PacketID.AppName                        = CFE_TIME
                LongEventTlm.Payload.PacketID.EventID                        = 20
                LongEventTlm.Payload.PacketID.EventType                      = 2
                LongEventTlm.Payload.PacketID.SpacecraftID                   = 66
                LongEventTlm.Payload.PacketID.ProcessorID                    = 1
                LongEventTlm.Payload.Message  
                '''

###############################################################################

class App():

    GUI_IMAGE_TXT_PREFIX = "Image: "
    GUI_NO_IMAGE_TXT     = GUI_IMAGE_TXT_PREFIX + "--None Selected--"
        
    def __init__(self, ini_file):

        self.path = os.getcwd()
        self.config = configparser.ConfigParser()
        self.config.read(ini_file)

        self.APP_VERSION = self.config.get('APP','VERSION')

        self.EDS_MISSION_NAME    = self.config.get('MISSION','EDS_NAME')
        self.EDS_CFS_TARGET_NAME = self.config.get('CFS_TARGET','EDS_NAME')

        self.CFS_TARGET_HOST_ADDR   = self.config.get('CFS_TARGET','HOST_ADDR')
        self.CFS_TARGET_CMD_PORT    = self.config.getint('CFS_TARGET','SEND_CMD_PORT')
        self.CFS_TARGET_TLM_PORT    = self.config.getint('CFS_TARGET','RECV_TLM_PORT')
        self.CFS_TARGET_TLM_TIMEOUT = self.config.getint('CFS_TARGET','RECV_TLM_TIMEOUT')
        
        self.GUI_CMD_PAYLOAD_TABLE_ROWS = self.config.getint('GUI','CMD_PAYLOAD_TABLE_ROWS')

        self.event_history = ""
        self.cfe_time_event_filter = False  #todo: Retaining the state here doesn't work if user starts and stops the cFS and doesn't restart cFSAT
        
        self.event_queue = queue.Queue()
        self.gui_tlm_objects = {}
        self.gui_tlm_threads = {}
        self.window = None
        
        self.cfs_thread     = None
        self.cfs_subprocess = None
        
        self.manage_tutorials = ManageTutorials(self.config.get('TOOLS', 'TUTORIALS_PATH'))
        self.create_app       = CreateApp(self.config.get('TOOLS', 'APP_TEMPLATES_PATH'))
        
    def update_event_history_str(self, new_event_text):
    
        time = datetime.now().strftime("%H:%M:%S")
        event_str = time + " - " + new_event_text + "\n"        
        self.event_history += event_str
    
    
    def display_event(self, new_event_text):
        self.update_event_history_str(new_event_text)
        self.window["-EVENT_TEXT-"].update(self.event_history)

    def display_tlm_monitor(self, app_name, tlm_msg, tlm_item, tlm_text):

        print("Received [%s, %s, %s] %s" % (app_name, tlm_msg, tlm_item, tlm_text))
        

    def send_app_cmd(self, app_name, cmd_name, cmd_payload):

        (cmd_sent, cmd_text, cmd_status) = self.script_telecommand.send_app_cmd(app_name, cmd_name, cmd_payload)
        self.display_event(cmd_status)

    def cfs_subprocess_thread(self):
        # Good overview: https://janakiev.com/blog/python-shell-commands/
        # This works but didn't return. Need to spawn a thread. Couldn't use '~', had to be absolute path
        # cfs = subprocess.run(['sudo','/home/grandpa-dave/cfe-eds-framework/build/exe/cpu1/core-cpu1','osk'],stdout=subprocess.PIPE,universal_newlines=True)
    
        thread = threading.currentThread()
        
        print("Before subprocess")
        cfs_dir = self.cfs_exe_str[0:self.cfs_exe_str.rfind("/")]
        print("cfs_dir = " + cfs_dir)
        self.cfs_subprocess = subprocess.Popen([self.cfs_exe_str],
                                  cwd=cfs_dir, stdout=subprocess.PIPE,universal_newlines=True)
        print("After subprocess")

        while not thread.kill:
            output = self.cfs_subprocess.stdout.readline()
            #print(output.strip())
            time.sleep(0.25)
            
        print("Before subprocess terminate")
        #self.cfs_subprocess.terminate()

    def disable_flywheel_event(self):
        # CFE_TIME does not configure an event filter so the first time through add an event filter to CFE_TIME
        # Set and add filter commands have identical parameters
        if self.cfe_time_event_filter:
            evs_cmd = 'SetFilterCmd'
        else:
            evs_cmd = 'AddEventFilterCmd'
            self.cfe_time_event_filter = True
                        
            self.send_app_cmd('CFE_EVS', evs_cmd,  {'AppName': 'CFE_TIME', 'EventID': Cfe.CFE_TIME_FLY_ON_EID, 'Mask': Cfe.CFE_EVS_FIRST_ONE_STOP})
            time.sleep(0.5)
            self.send_app_cmd('CFE_EVS', evs_cmd,  {'AppName': 'CFE_TIME', 'EventID': Cfe.CFE_TIME_FLY_OFF_EID, 'Mask': Cfe.CFE_EVS_FIRST_ONE_STOP})

    def ComingSoonPopup(self, feature_str):
        sg.popup(feature_str, title='Coming soon...', grab_anywhere=True, modal=False)
 
    def shutdown(self):
        logger.info("Starting app shutdown sequence")
        self.tlm_server.shutdown()
        for tlm_topic in self.gui_tlm_objects:
            if self.gui_tlm_objects[tlm_topic] != None:
                 self.gui_tlm_objects[tlm_topic].shutdown()
        time.sleep(self.CFS_TARGET_TLM_TIMEOUT)
        self.window.close()

    def execute(self):
    
        sys_target_str = "cFSAT version %s initialized with mission %s, target %s on %s" % (self.APP_VERSION, self.EDS_MISSION_NAME, self.EDS_CFS_TARGET_NAME, datetime.now().strftime("%m/%d/%Y"))
        sys_comm_str = "cFSAT target host %s, command port %d, telemetry port %d" % (self.CFS_TARGET_HOST_ADDR, self.CFS_TARGET_CMD_PORT, self.CFS_TARGET_TLM_PORT)
    
        logger.info(sys_target_str)
        logger.info(sys_comm_str)
        
        self.tlm_monitors = {'CFE_ES': {'HK_TLM': ['Seconds']}}
        
        try:
    
             self.gui_telecommand = GuiTelecommand(self.EDS_MISSION_NAME, self.EDS_CFS_TARGET_NAME, self.CFS_TARGET_HOST_ADDR, self.CFS_TARGET_CMD_PORT)
             self.script_telecommand = ScriptTelecommand(self.EDS_MISSION_NAME, self.EDS_CFS_TARGET_NAME, self.CFS_TARGET_HOST_ADDR, self.CFS_TARGET_CMD_PORT)
             
             self.tlm_server = TelemetryServer(self.EDS_MISSION_NAME, self.EDS_CFS_TARGET_NAME, self.CFS_TARGET_HOST_ADDR,
                                               self.CFS_TARGET_TLM_PORT, self.CFS_TARGET_TLM_TIMEOUT)
         
             self.sys_telemetry = SystemTelemetry(self.tlm_server, self.tlm_monitors, self.display_tlm_monitor, self.event_queue)
         
             self.tlm_server.execute()
         
             logger.info("Successfully created application objects")
        
        except RuntimeError:
            print("Error creating telecommand/telemetry objects and/or telemetry server. See log file for details")
            logger.error("Error creating application objects")
            sys.exit(2)

        sg.theme('LightGreen')
        sg.set_options(element_padding=(0, 0))
    
        menu_def = [
                       ['System', ['Options','Perf Monitor', 'About...', 'Exit']],
                       #['cFS Target', ['Set Target', 'Start cFS', 'Stop cFS', 'App Suite...']],
                       ['Apps', ['Create...']], #, 'Develop', 'Test']],
                       ['Files', ['Uplink File', 'Downlink File', 'Load Binary Table', 'Dump Binary Table', 'Load JSON Table', 'Dump JSON Table']],
                       ['Tutorials', self.manage_tutorials.tutorial_titles]
                   ]

        cfs_config_cmds = ['-- cFS Configuration--', 'Enable Telemetry', 'cFE Version', 'Reset Time', 'Configure Events', 'Ena/Dis Flywheel']


        # Events can't be posted until after first window.read() so initialization string is format here and used as the default string
        
        self.update_event_history_str(sys_target_str)
        self.update_event_history_str(sys_comm_str)
            
        cmd_topics = []
        cmd_topic_list = list(self.gui_telecommand.get_topics().keys())
        all_cmd_topics = self.config.getboolean('GUI','CMD_TOPICS_ALL')
        for topic in cmd_topic_list:
            if 'Application/CMD' in topic:
                cmd_topics.append(topic)
            else:
                if all_cmd_topics:
                    cmd_topics.append(topic)
            
        logger.debug("cmd_topics = " + str(cmd_topics))

        tlm_topics = list(self.tlm_server.get_topics().keys())
        logger.debug("tlm_topics = " + str(tlm_topics))
            
        pri_hdr_font = ('Arial bold',14)
        sec_hdr_font = ('Arial',12)
        log_font = ('Courier',12)
        layout = [
                     [sg.Menu(menu_def, tearoff=False, pad=(400, 10))],
                     [sg.Text('Mission: ' + self.EDS_MISSION_NAME, font=pri_hdr_font),
                      sg.Text('Target: ' + self.gui_telecommand.target_name, font=pri_hdr_font, pad=(10,1)),
                      sg.Text(self.GUI_NO_IMAGE_TXT, key='-CFS_IMAGE-', font=pri_hdr_font, pad=(10,1)),],
                     [sg.Button('Start cFS', enable_events=True, key='-START_CFS-',  image_data=green, image_subsample=2, button_color=('black', sg.theme_background_color()), border_width=0),
                      sg.Button('Stop cFS', enable_events=True, key='-STOP_CFS-',image_data=red, image_subsample=2, button_color=('black', sg.theme_background_color()), border_width=0),
                      sg.Text('cFS Config:', font=sec_hdr_font, pad=(5,1)),
                      sg.Combo(cfs_config_cmds, enable_events=True, key="-CFS_CONFIG_CMD-", default_value=cfs_config_cmds[0]),
                      sg.Text('Send Cmd:', font=sec_hdr_font, pad=(5,1)),
                      sg.Combo(cmd_topics, enable_events=True, key="-CMD_TOPICS-", default_value=cmd_topics[0]),
                      sg.Text('View Tlm:', font=sec_hdr_font, pad=(5,1)),
                      sg.Combo(tlm_topics, enable_events=True, key="-TLM_TOPICS-", default_value=tlm_topics[0], size=(40,1))],
                     [sg.Text('FSW Status', font=pri_hdr_font), sg.Text('(Placeholder for cFE and runtime app status)', pad=(2,1))],
                     [sg.Output(font=log_font, size=(125, 10))],
                     [sg.Text('Ground & Flight Events', font=pri_hdr_font), sg.Button('Clear', enable_events=True, key='-CLEAR_EVENTS-', pad=(5,1))],
                     [sg.MLine(default_text=self.event_history, font=log_font, enable_events=True, size=(125, 20), key='-EVENT_TEXT-')]
                 ]

        #sg.Button('Send Cmd', enable_events=True, key='-SEND_CMD-', pad=(10,1)),
        #sg.Button('View Tlm', enable_events=True, key='-VIEW_TLM-', pad=(10,1)),
        self.window = sg.Window('cFS Application Toolkit - Beta', layout, auto_size_text=True, finalize=True)
    
        # --- Loop taking in user input --- #
        while True:
    
            self.event, self.values = self.window.read(timeout=250)
            logger.debug("App Window Read()\nEvent: %s\nValues: %s" % (self.event, self.values))
            
            try:
                new_event_text = self.event_queue.get_nowait()
                self.display_event(new_event_text)
            except:
                pass
            
            if self.event in (sg.WIN_CLOSED, 'Exit') or self.event is None:
                break
        
            #######################
            ##### MENU EVENTS #####
            #######################

            ### SYSTEM ###

            if self.event == 'Options':
                self.ComingSoonPopup("Configure system behavioral options")
            
            if self.event == 'Perf Monitor':
                self.ComingSoonPopup("Launch the cFS Performanc Monitor tool")
            
            if self.event == 'About...':
                sg.popup('Version ' + self.APP_VERSION,
                         title='About cFS Application Toolkit', 
                         grab_anywhere=True)
       
            ### APPS ###

            if self.event == 'Create...':
                self.create_app.execute()

            ### FILES ###

            if self.event == 'Uplink File':
                self.ComingSoonPopup("Uplink a file from the ground to the FSW")

            if self.event == 'Downlink File':
                self.ComingSoonPopup("Downlink a file from the FSW to the ground")

            if self.event == 'Load Binary Table':
                self.ComingSoonPopup("Load an app's binary table")
       
            if self.event == 'Dump Binary Table':
                self.ComingSoonPopup("Dump an app's binary table")

            if self.event == 'Load JSON Table':
                self.ComingSoonPopup("Load an app's JSON table")
       
            if self.event == 'Dump JSON Table':
                self.ComingSoonPopup("Dump an app's JSON table")

            ### TUTORIALS ###
                   
            if self.event in self.manage_tutorials.tutorial_titles:
                self.manage_tutorials.run_tutorial(self.event)
                
            #################################
            ##### TOP ROW BUTTON EVENTS #####
            #################################
 
            if self.event == '-START_CFS-':

                #todo: Kill current thread if running
                #todo: history_setting_filename doesn't seem to do anything. My goal is to save cFS imagelocations across app invocations 
                self.cfs_exe_str = sg.popup_get_file('Please select your cFS executable image', title='cFS Executable Dialog',
                                       default_path = self.config.get('CFS_TARGET','EXE_PATH'), history = True,
                                       history_setting_filename = 'cfs_exe.log')
                
                if self.cfs_exe_str != None:
                    print("self.cfs_exe_str = " + self.cfs_exe_str)
                    self.window["-CFS_IMAGE-"].update(self.GUI_IMAGE_TXT_PREFIX + self.cfs_exe_str)
                    #threading.Thread(target=self.cfs_thread).start()
                    self.cfs_thread = threading.Thread(target=self.cfs_subprocess_thread)
                    self.cfs_thread.kill = False
                    self.cfs_thread.start()
                
            if self.event == '-STOP_CFS-':
                if self.cfs_subprocess != None:
                    self.cfs_thread.kill = True
                self.window["-CFS_IMAGE-"].update(self.GUI_NO_IMAGE_TXT)

            if self.event == '-CFS_CONFIG_CMD-':
                cfs_config_cmd = self.values['-CFS_CONFIG_CMD-']
                if cfs_config_cmd == cfs_config_cmds[1]: # Enable Telemetry
                    self.send_app_cmd('TO_LAB', 'EnableOutputCmd', {'dest_IP': self.CFS_TARGET_HOST_ADDR})
                    # Disable flywheel events. Assume new cFS instance running so set time_event_filter to false 
                    self.cfe_time_event_filter = False 
                    time.sleep(0.5)
                    self.disable_flywheel_event()

                elif cfs_config_cmd == cfs_config_cmds[2]: # cFE Version (CFE ES Noop)
                    self.send_app_cmd('CFE_ES', 'NoopCmd', {})
            
                elif cfs_config_cmd == cfs_config_cmds[3]: # Reset Time
                    self.send_app_cmd('CFE_TIME', 'SetMETCmd', {'Seconds': 0,'MicroSeconds': 0 })
                    time.sleep(0.5)
                    self.send_app_cmd('CFE_TIME', 'SetTimeCmd', {'Seconds': 0,'MicroSeconds': 0 })
            
                elif cfs_config_cmd == cfs_config_cmds[4]: # Configure Events
                
                    app_list = ['CFE_ES', 'CFE_EVS', 'CFE_SB', 'CFE_TBL', 'CFE_TIME'] #todo: dynamically create list using topics. Add method to base class
                
                    pop_win = sg.Window('Configure App Events',
                                        [[sg.Text("Select App"), sg.Combo((app_list), size=(20,1), key='-APP_NAME-', default_value=app_list[0])],
                                         [sg.Checkbox('Debug', key='-DEBUG-', default=False), sg.Checkbox('Information', key='-INFO-', default=True),
                                          sg.Checkbox('Error', key='-ERROR-', default=True),  sg.Checkbox('Critical', key='-CRITICAL-', default=True)], 
                                         [sg.Button('Enable', button_color=('green'), enable_events=True, key='-ENABLE-', pad=(10,1)),
                                          sg.Button('Disable', button_color=('red'), enable_events=True, key='-DISABLE-', pad=(10,1)), 
                                          sg.Cancel(button_color=('gray'))]])
                
                    pop_event, pop_values = pop_win.read()
                
                    app_name = pop_values['-APP_NAME-'] 
                    bit_mask = 0
                    bit_mask = (bit_mask | (Cfe.EVS_DEBUG_MASK    if pop_values['-DEBUG-']    else 0))
                    bit_mask = (bit_mask | (Cfe.EVS_INFO_MASK     if pop_values['-INFO-']     else 0))
                    bit_mask = (bit_mask | (Cfe.EVS_ERROR_MASK    if pop_values['-ERROR-']    else 0))
                    bit_mask = (bit_mask | (Cfe.EVS_CRITICAL_MASK if pop_values['-CRITICAL-'] else 0))

                    if pop_event == '-ENABLE-':
                        self.send_app_cmd('CFE_EVS', 'EnableAppEventTypeCmd',  {'AppName': app_name, 'BitMask': bit_mask})
                    if pop_event == '-DISABLE-':
                        self.send_app_cmd('CFE_EVS', 'DisableAppEventTypeCmd',  {'AppName': app_name, 'BitMask': bit_mask})

                    pop_win.close()
                
            
                elif cfs_config_cmd == cfs_config_cmds[5]: # Ena/Dis Flywheel
            
                    pop_text = "cFE TIME outputs an event when it starts/stops flywheel mode\nthat occurs when time can't synch to the 1Hz pulse. Use the\nbuttons to enable/disable the flywheel event messages..."
                    pop_win = sg.Window('Flywheel Message Configuration',
                                        [[sg.Text(pop_text)],
                                        [sg.Text("")],
                                        [sg.Button('Enable', button_color=('green'), enable_events=True, key='-FLYWHEEL_ENABLE-', pad=(10,1)),
                                         sg.Button('Disable', button_color=('red'), enable_events=True, key='-FLYWHEEL_DISABLE-', pad=(10,1)), 
                                         sg.Cancel(button_color=('gray'))]])
                
                    pop_event, pop_values = pop_win.read()
                 
                    if pop_event == '-FLYWHEEL_ENABLE-':
                
                        self.send_app_cmd('CFE_EVS', 'SetFilterCmd',  {'AppName': 'CFE_TIME','EventID': Cfe.CFE_TIME_FLY_ON_EID, 'Mask': Cfe.CFE_EVS_NO_FILTER})
                        time.sleep(0.5)
                        self.send_app_cmd('CFE_EVS', 'SetFilterCmd',  {'AppName': 'CFE_TIME','EventID': Cfe.CFE_TIME_FLY_OFF_EID, 'Mask': Cfe.CFE_EVS_NO_FILTER})
                
                    if pop_event == '-FLYWHEEL_DISABLE-':
                        
                        self.disable_flywheel_event()

                    pop_win.close()
                   
            if self.event == '-CMD_TOPICS-':
                #todo: Create a command string for event window. Raw text may be an option so people can capture commands
                cmd_topic = self.values['-CMD_TOPICS-']
                (cmd_sent, cmd_text, cmd_status) = self.gui_telecommand.execute(cmd_topic)
                self.display_event(cmd_status)
                self.display_event(cmd_text)
            
            if self.event == '-TLM_TOPICS-':
                print("View TLM")
                tlm_topic = self.values['-TLM_TOPICS-']
                print("tlm_topic = " + tlm_topic)
                if tlm_topic != EdsMission.TOPIC_TLM_TITLE_KEY:
                    '''
                    The thread start() nevers returns and I don't know why. Guessing underlying GUI control issue.
                    The GuiTelemetry object contains thread management but this causing execeptions. For now I"m
                    catching the exceptions because eveything else hangs together, but it's a kludge!!
                    '''
                    self.gui_tlm_objects[tlm_topic] = GuiTelemetry(self.tlm_server, tlm_topic)
                    if self.gui_tlm_objects[tlm_topic] != None:
                        self.gui_tlm_objects[tlm_topic].execute()
                        #self.gui_tlm_threads[tlm_topic] = Thread(target=self.gui_tlm_objects[tlm_topic].gui())
                        #self.gui_tlm_threads[tlm_topic].start()
                        self.display_event("Created telemetry screen for %s" % tlm_topic)
                    else:
                        self.display_event("Failed to create telemetry screen for %s. Verify EDS naming standard compliance" % tlm_topic)
                else:
                    sg.popup('Please select a telmeetry topic from the dropdown list', title='Telemetry Topic')
                
            if self.event == '-CLEAR_EVENTS-':
                self.event_history = ""
                self.display_event("Cleared event display")

        self.shutdown()


if __name__ == '__main__':

    green = b'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'
    red = b'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'

    
    app = App('cfsat.ini')
    app.execute()    
    

