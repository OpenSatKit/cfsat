#!/usr/bin/env python
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
        Provide classes that manage ground and flight files through a GUI. This
        includes displaying directory listing, indiviual file manipulation, and
        transferring files. 

"""

import sys
import time
import os
import socket
import configparser
from queue import Queue
from datetime import datetime

import logging
logger = logging.getLogger(__name__)

import PySimpleGUI as sg
if __name__ == '__main__':
    sys.path.append('..')
    from telecommand import TelecommandScript
    from telemetry   import TelemetryMessage, TelemetryObserver, TelemetrySocketServer
else:
    from .telecommand import TelecommandScript
    from .telemetry   import TelemetryMessage, TelemetryObserver, TelemetrySocketServer
from tools import crc_32c, compress_abs_path


    
###############################################################################

class CmdTlmProcess():
    """
    Defines a base class used by processes that are launched by cFSAT. This
    manages the command and telemetry connections to the CmdTlmRouter.
    
    TelecommandScript() is designed to send commands to a queue and not to a
    socket. This design is based on CmdTlmRouter being part of the main app. 
    Remote app sockets were added later so rather than complicate the existing
    design this CmdTlmProcess base class was create that essentially performs
    the role of a local router. There's a little extra queueing involved but
    this is a lightweight design that does not need to scale up.
    
    The following steps outline how to create a new cFSAT GUI-base CmdTlmcProcess
    class. See filebrowser.py for a complete example:
      1. Create a standalone GUI class and identfy cmd & tlm interface points
      2. Add CmdTlmProcess subclass 
      3. Create equivalent of FileBrowserTelemetryMonitor if you need to monitor
         telemetry points. Create a tlm callback function that populates GUI
      4. Create a shutdown method that terminates all threads
       
    """
    def __init__(self, gnd_ip_addr, router_cmd_port, tlm_port, tlm_timeout):
    
        self.gnd_ip_addr = gnd_ip_addr
        self.router_cmd_port = router_cmd_port
        self.router_cmd_socket = None
        self.router_cmd_socket_addr = (self.gnd_ip_addr, self.router_cmd_port)
        self.router_cmd_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

        self.cfs_cmd_queue = Queue()
        self.cmd_script = TelecommandScript('samplemission', 'cpu1', self.cfs_cmd_queue)  #TODO - Use kwarg?
       
        self.tlm_server = TelemetrySocketServer('samplemission', 'cpu1', gnd_ip_addr, tlm_port, tlm_timeout)  #TODO - Use kwarg?
 
       
    def create_sockets(self):
        
        
        self.tlm_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.tlm_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.tlm_socket.bind(self.tlm_socket_addr)
        self.tlm_socket.setblocking(False)
        self.tlm_socket.settimeout(self.tlm_timeout)

    def send_cfs_cmd(self, app_name, cmd_name, cmd_payload):
    
        (cmd_sent, cmd_text, cmd_status) = self.cmd_script.send_cfs_cmd(app_name, cmd_name, cmd_payload)
        datagram = self.cfs_cmd_queue.get()
        self.router_cmd_socket.sendto(datagram, self.router_cmd_socket_addr)


###############################################################################

class FlightDir():
    """
    """
    def __init__(self, path, cmd_script: TelecommandScript, sg_window: sg.Window):
        self.cmd_script = cmd_script
        self.sg_window  = sg_window
        
        self.path = path
        self.file_cnt  = 0
        self.file_list = []

    def create_file_list(self, path):
        self.path = path
        self.file_list = []
        self.cmd_script.send_cfs_cmd('FILEMGR', 'SendDirTlm',  {'DirName': path, 'IncludeSizeTime': 0})

    def delete_file(self, filename):
        self.cmd_script.send_cfs_cmd('FILEMGR', 'DeleteFile',  {'Filename': filename})
        self.create_file_list(self.path)

    def rename_file(self, src_file):
        dst_file = sg.popup_get_text(title='Rename '+src_file, message='Please enter the new filename')
        if dst_file is not None:
            dst_file = self.curr_path + '/' + dst_file
            self.cmd_script.send_cfs_cmd('FILEMGR', 'RenameFile',  {'SourceFilename': src_file, 'TargetFilename': dst_file})
            self.create_file_list(self.path)

    def filemgr_dir_list_callback(self, time, payload):
        print("payload.DirName = "       + str(payload.DirName))
        print("payload.DirFileCnt = "    + str(payload.DirFileCnt))
        print("payload.PktFileCnt = "    + str(payload.PktFileCnt))
        print("payload.DirListOffset = " + str(payload.DirListOffset))        
        for entry in payload.FileList:
            if (len(str(entry['Name'])) > 0):
                self.file_list.append(str(entry['Name']))
        print("file_list: " + str(self.file_list))
        self.sg_window.update(self.file_list)


###############################################################################

class GroundDir():
    """
    """
    def __init__(self, path, sg_window: sg.Window):
        self.sg_window  = sg_window

        self.path = compress_abs_path(path if os.path.isdir(path) else "")
        self.file_list = []

    def create_file_list(self, path):
        try:
            dir_list = os.listdir(self.path)
            self.path = path
        except:
            dir_list = []
        self.file_list = [f for f in dir_list if os.path.isfile(os.path.join(self.path, f))]
        self.sg_window.update(self.file_list)
        
    def delete_file(self, filename):
        file_pathname = os.path.join(self.path, filename)
        if os.path.exists(file_pathname):
            os.remove(file_pathname)
        else:
            print("TODO: The file does not exist")
        create_file_list(self.path)
        
    def rename_file(self, src_file):
        src_file_pathname = os.path.join(self.path, src_file)
        if os.path.exists(src_file_pathname):
            dst_file = sg.popup_get_text(title='Rename '+src_file, message='Please enter the new filename')
            if dst_file is not None:
               dst_file_pathname = os.path.join(self.path, dst_file)
               os.rename(src_file_pathname, dst_file_pathname)
               create_file_list(self.path)
        else:
            print("TODO: The file does not exist")
            

###############################################################################

class FileBrowserTelemetryMonitor(TelemetryObserver):
    """
    callback_functions
       [app_name] : {packet: [item list]} 
    
    """

    def __init__(self, tlm_server: TelemetrySocketServer, tlm_monitors, event_callback, filemgr_callback): 
        super().__init__(tlm_server)

        self.tlm_monitors = tlm_monitors
        self.event_callback = event_callback
        self.filemgr_callback = filemgr_callback
        
        self.sys_apps = ['CFE_ES', 'CFE_EVS', 'FILEMGR']
        
        for msg in self.tlm_server.tlm_messages:
            tlm_msg = self.tlm_server.tlm_messages[msg]
            if tlm_msg.app_name in self.sys_apps:
                self.tlm_server.add_msg_observer(tlm_msg, self)        
                #logger.info("system telemetry adding observer for %s: %s" % (tlm_msg.app_name, tlm_msg.msg_name))
                print("system telemetry adding observer for %s: %s" % (tlm_msg.app_name, tlm_msg.msg_name))
        

    def update(self, tlm_msg: TelemetryMessage) -> None:
        """
        Receive telemetry updates
        """
        #todo: Determine best tlm identification method: if int(tlm_msg.app_id) == int(self.cfe_es_hk.app_id):
        
        if tlm_msg.app_name == 'FILEMGR':
            if tlm_msg.msg_name == 'DIR_LIST_TLM':
                payload = tlm_msg.payload()
                self.filemgr_callback(str(tlm_msg.sec_hdr().Seconds), payload)
        
        elif tlm_msg.app_name == 'CFE_EVS':
            if tlm_msg.msg_name == 'LONG_EVENT_MSG':
                payload = tlm_msg.payload()
                pkt_id = payload.PacketID
                event_text = "FSW Event at %s: %s, %d - %s" % \
                             (str(tlm_msg.sec_hdr().Seconds), pkt_id.AppName, pkt_id.EventType, payload.Message)
                self.event_callback(event_text)
                
                
###############################################################################

class FileBrowser(CmdTlmProcess):
    """
    Provide a user interface for managing ground and flight directories and
    files. It also supports transferring files between the flight and ground.
    """
    def __init__(self, gnd_path, flt_path, gnd_ip_addr, router_cmd_port, tlm_port, tlm_timeout):
        super().__init__(gnd_ip_addr, router_cmd_port, tlm_port, tlm_timeout)
        
        self.default_gnd_path = gnd_path
        self.default_flt_path = flt_path
        self.event_history = ""
        self.init_cycle = True
            
    def event_callback(self, event_txt):
        self.display_event(event_txt)
            
    def update_event_history_str(self, new_event_text):
        time = datetime.now().strftime("%H:%M:%S")
        event_str = time + " - " + new_event_text + "\n"        
        self.event_history += event_str
 
    def display_event(self, new_event_text):
        self.update_event_history_str(new_event_text)
        self.window["-EVENT_TEXT-"].update(self.event_history)
                
    def gui(self):
        col_title_font = ('Arial bold',20)
        pri_hdr_font   = ('Arial bold',14)
        sec_hdr_font   = ('Arial',12)
        log_font       = ('Courier',12)
        self.gnd_file_menu = [[''], ['Delete','Rename','Send to Flight']] 
        self.gnd_col = [
            [sg.Text('Ground', font=col_title_font)],
            [sg.Text('Folder'), sg.In(self.default_gnd_path, size=(25,1), enable_events=True ,key='-GND_FOLDER-'), sg.FolderBrowse(initial_folder=self.default_gnd_path)],
            [sg.Listbox(values=[], enable_events=True, size=(40,20),key='-GND_FILE_LIST-',right_click_menu=self.gnd_file_menu)]]
        
        # Duplicate ground names have a space. A little kludgy but it works
        self.flt_file_menu = [[''], ['Delete ','Rename ','Send to Ground']] 
        self.flt_col = [
            [sg.Text('Flight', font=col_title_font)],
            [sg.Text('Folder'), sg.In(self.default_flt_path, size=(25,1), enable_events=True ,key='-FLT-FOLDER-'), sg.FolderBrowse()],
            [sg.Listbox(values=[], enable_events=True, size=(40,20),key='-FLT_FILE_LIST-',right_click_menu=self.flt_file_menu)]]

        self.layout = [
            [sg.Column(self.gnd_col, element_justification='c'), sg.VSeperator(), sg.Column(self.flt_col, element_justification='c')],
            [sg.Text('Ground & Flight Events', font=pri_hdr_font), sg.Button('Clear', enable_events=True, key='-CLEAR_EVENTS-', pad=(5,1))],
            [sg.MLine(default_text=self.event_history, font=log_font, enable_events=True, size=(65, 5), key='-EVENT_TEXT-')]]
            
 
        self.window = sg.Window('File Browser', self.layout, resizable=True)
        
        self.flt_dir = FlightDir(self.default_flt_path, self.cmd_script, self.window['-FLT_FILE_LIST-'])
        self.gnd_dir = GroundDir(self.default_gnd_path,self.window['-GND_FILE_LIST-'])
        
        self.tlm_monitors = {'CFE_ES': {'HK_TLM': ['Seconds']}, 'FILEMGR': {'DIR_LIST_TLM': ['Seconds']}}        
        self.tlm_monitor = FileBrowserTelemetryMonitor(self.tlm_server, self.tlm_monitors, self.event_callback, self.flt_dir.filemgr_dir_list_callback)
        self.tlm_server.execute()

        while True:

            self.event, self.values = self.window.read(timeout=100)
        
            if self.event in (sg.WIN_CLOSED, 'Exit') or self.event is None:
                break

            if self.init_cycle:
                self.init_cycle = False
                self.gnd_dir.create_file_list(self.default_gnd_path)
                self.flt_dir.create_file_list(self.default_flt_path)
                
            if self.event == '-GND_FOLDER-':                         
                self.gnd_dir.create_file_list(self.values['-GND_FOLDER-'])                

            elif self.event == '-FLT_FOLDER-':
                self.flt_dir.create_file_list(self.values['-FLT_FOLDER-'])                

            elif self.event == 'Delete':
                if len(self.values['-GND_FILE_LIST-']) > 0:
                   self.gnd_dir.delete_file(self.values['-GND_FILE_LIST-'][0])

            elif self.event == 'Rename':
                if len(self.values['-GND_FILE_LIST-']) > 0:
                    self.gnd_dir.rename_file(self.values['-GND_FILE_LIST-'][0])

            elif self.event == 'Send to Flight':
                self.send_cfs_cmd('CFE_SB', 'NoopCmd', {})
                print('Selected send to flight')

            elif self.event == 'Delete ':
                if len(self.values['-FLT_FILE_LIST-']) > 0:
                    self.flt_dir.delete_file(self.values['-FLT_FILE_LIST-'][0])

            elif self.event == 'Rename ':
                if len(self.values['-FLT_FILE_LIST-']) > 0:
                    self.flt_dir.rename_file(self.values['-FLT_FILE_LIST-'][0])

            elif self.event == 'Send to Ground':
                self.send_cfs_cmd('CFE_TBL', 'NoopCmd', {})
                print('Selected send to ground')
                
            elif self.event == '-CLEAR_EVENTS-':
                self.event_history = ""
                self.display_event("Cleared event display")

    def execute(self):
        self.gui()
    
        
    def shutdown(self):

        self.window.close()       
        self.tlm_server.shutdown()    


###############################################################################

if __name__ == '__main__':

    print(f"Name of the script      : {sys.argv[0]=}")
    print(f"Arguments of the script : {sys.argv[1:]=}")

    config = configparser.ConfigParser()
    config.read('../cfsat.ini')
    FLT_SERVER_PATH = config.get('TOOLS','FLT_SERVER_PATH')
    print ("FLT_SERVER_PATH = " + FLT_SERVER_PATH)

    #tlm_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    #tlm_socket.sendto("127.0.0.1,1234".encode(),("127.0.0.1",8888))

    gnd_path = os.path.join(os.getcwd(), '..', FLT_SERVER_PATH) 
    file_browser = FileBrowser(gnd_path, '/cf', '127.0.0.1', 8000, 9000, 1.0)
    file_browser.execute()
    
    

