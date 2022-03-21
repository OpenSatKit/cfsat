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
sys.path.append('..')

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

    def send_app_cmd(self, app_name, cmd_name, cmd_payload):
    
        (cmd_sent, cmd_text, cmd_status) = self.cmd_script.send_app_cmd(app_name, cmd_name, cmd_payload)
        datagram = self.cfs_cmd_queue.get()
        self.router_cmd_socket.sendto(datagram, self.router_cmd_socket_addr)


###############################################################################

class GroundFileSys():
    """
    """
    def __init__(self, default_path):

        self.curr_path = compress_abs_path(default_path if os.path.isdir(default_path) else "")
        self.file_list = []
        self.create_file_list(default_path)


    def create_file_list(self, path):

        try:
            dir_list = os.listdir(path)
        except:
            dir_list = []
        self.file_list = [f for f in dir_list if os.path.isfile(os.path.join(path, f))]
        return self.file_list
   
   
    def delete_file(self, file):
   
        file_pathname = os.path.join(self.curr_path, file)
        if os.path.exists(file_pathname):
            os.remove(file_pathname)
        else:
            print("TODO: The file does not exist")

            
    def rename_file(self, src_file):
   
        src_file_pathname = os.path.join(self.curr_path, src_file)
        if os.path.exists(src_file_pathname):
            dst_file = sg.popup_get_text(title='Rename '+src_file, message='Please enter the new filename')
            if dst_file is not None:
               dst_file_pathname = os.path.join(self.curr_path, dst_file)
               os.rename(src_file_pathname, dst_file_pathname)
        else:
            print("TODO: The file does not exist")
            

###############################################################################

class FileBrowserTelemetryMonitor(TelemetryObserver):
    """
    callback_functions
       [app_name] : {packet: [item list]} 
    
    """

    def __init__(self, tlm_server: TelemetrySocketServer, tlm_monitors, tlm_callback):
        super().__init__(tlm_server)

        self.tlm_monitors = tlm_monitors
        self.tlm_callback = tlm_callback
        
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
        
        if tlm_msg.app_name in self.tlm_monitors:
            self.tlm_callback(tlm_msg.app_name, tlm_msg.msg_name, "Seconds", str(tlm_msg.sec_hdr().Seconds))


###############################################################################

class FileBrowser(CmdTlmProcess):
    """
    Provide a user interface for managing ground and flight directories and
    files. It also supports transferring files between the flight and ground.
    """
    def __init__(self, gnd_path, flt_path, gnd_ip_addr, router_cmd_port, tlm_port, tlm_timeout):
        super().__init__(gnd_ip_addr, router_cmd_port, tlm_port, tlm_timeout)

        self.gnd_file_sys = GroundFileSys(gnd_path)
        
        self.flt_path      = flt_path
        self.flt_file_list = []
            
    def refresh_gnd_file_list(self):
    
        file_names = self.gnd_file_sys.create_file_list(self.gnd_file_sys.curr_path)
        self.window['-GND_FILE_LIST-'].update(file_names)
    
    def tlm_monitor_callback(self, app_name, tlm_msg, tlm_item, tlm_text):
        logger.info("Received [%s, %s, %s] %s" % (app_name, tlm_msg, tlm_item, tlm_text))
        print("Received [%s, %s, %s] %s" % (app_name, tlm_msg, tlm_item, tlm_text))
            
    def gui(self):
    
        col_title_font = 'Courier 20 bold'
        self.gnd_file_menu = [[''], ['Delete','Rename','Send to Flight']] 
        self.gnd_col = [
            [sg.Text('Ground', font=col_title_font)],
            [sg.Text('Folder'), sg.In(self.gnd_file_sys.curr_path, size=(25,1), enable_events=True ,key='-GND_FOLDER-'), sg.FolderBrowse(initial_folder=self.gnd_file_sys.curr_path)],
            [sg.Listbox(values=self.gnd_file_sys.file_list, enable_events=True, size=(40,20),key='-GND_FILE_LIST-',right_click_menu=self.gnd_file_menu)]]
        
        # Duplicate ground names have a space. A little kludgy but it works
        self.flt_file_menu = [[''], ['Delete ','Rename ','Send to Ground']] 
        self.flt_col = [
            [sg.Text('Flight', font=col_title_font)],
            [sg.Text('Folder'), sg.In(size=(25,1), enable_events=True ,key='-FLT-FOLDER-'), sg.FolderBrowse()],
            [sg.Listbox(values=[], enable_events=True, size=(40,20),key='-FLT_FILE_LIST-',right_click_menu=self.flt_file_menu)]]

        self.layout = [[sg.Column(self.gnd_col, element_justification='c'), sg.VSeperator(), sg.Column(self.flt_col, element_justification='c')]]
 
        self.window = sg.Window('File Browser', self.layout, resizable=True)
        
        while True:

            self.event, self.values = self.window.read(timeout=100)
        
            if self.event in (sg.WIN_CLOSED, 'Exit') or self.event is None:
                break

            

            if self.event == '-GND_FOLDER-':                         
                gnd_folder = self.values['-GND_FOLDER-']
                file_names = self.gnd_file_sys.create_file_list(gnd_folder)
                self.window['-GND_FILE_LIST-'].update(file_names)

            elif self.event == '-FLT_FOLDER-':
                print('Selected flight folder')

            elif self.event == 'Delete':
                if len(self.values['-GND_FILE_LIST-']) > 0:
                   self.gnd_file_sys.delete_file(self.values['-GND_FILE_LIST-'][0])
                   self.refresh_gnd_file_list()

            elif self.event == 'Rename':
                if len(self.values['-GND_FILE_LIST-']) > 0:
                    self.gnd_file_sys.rename_file(self.values['-GND_FILE_LIST-'][0])
                    self.refresh_gnd_file_list()

            elif self.event == 'Send to Flight':
                self.send_app_cmd('CFE_SB', 'NoopCmd', {})
                print('Selected send to flight')

            elif self.event == 'Delete ':
                print('Selected flight delete')

            elif self.event == 'Rename ':
                print('Selected flight rename')

            elif self.event == 'Send to Ground':
                print('Selected send to ground')
                

    def execute(self):
    
        self.tlm_monitors = {'CFE_ES': {'HK_TLM': ['Seconds']}, 'FILEMGR': {'DIR_LIST_TLM': ['Seconds']}}
        
        self.tlm_monitor = FileBrowserTelemetryMonitor(self.tlm_server, self.tlm_monitors, self.tlm_monitor_callback)
        self.tlm_server.execute()
        
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
    
    

