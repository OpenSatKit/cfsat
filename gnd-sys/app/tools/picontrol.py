#!/usr/bin/env python
"""
    Copyright 2022 bitValence, Inc.
    All Rights Reserved.

    This program is free software; you can modify and/or redistribute it
    under the terms of the GNU Affero General Public License
    as published by the Free Software Foundation; version 3 with
    attribution addendums as found in the LICENSE.txt.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    Purpose:
        Provide classes that manage the MQTT interface to a Pi-Sat
        
    Notes:    
      None
       
"""

import sys
import time
import os
import configparser
import json
import logging
logger = logging.getLogger(__name__)

import paho.mqtt.client as mqtt
import PySimpleGUI as sg


###############################################################################

class PiControl():


    # String constants used for JSON command message and GUI keys
    JSON_CMD_TARGET_CFS = 'cfs'
    JSON_CMD_TARGET_PI  = 'pisat'
 
    JSON_CMD_CFS_START    = 'start'
    JSON_CMD_CFS_ENA_TLM  = 'ena-tlm'
    JSON_CMD_CFS_STOP     = 'stop'

    JSON_CMD_PI_NOOP      = 'noop'
    JSON_CMD_PI_REBOOT    = 'reboot'
    JSON_CMD_PI_SHUTDOWN  = 'shutdown'

    # String constants used for JSON telemetry message and GUI keys
    JSON_TLM_SEQ_CNT = 'seq-cnt'
    JSON_TLM_RUNNING = 'running'
    JSON_TLM_CMD_CNT = 'cmd-cnt'
    JSON_TLM_APPS    = 'apps'
    JSON_TLM_EVENT   = 'event'
     
    '''
    Manage the Pi-Sat interface 
    '''
    def __init__(self, pisat_id, broker_addr, broker_port, client_name):
        """
        """
        
        self.pisat_id    = pisat_id
        self.broker_addr = broker_addr
        self.broker_port = broker_port
        self.client_name = client_name
        self.client = None
                
        self.cmd_topic = "osk/pisat-%s/cmd" % pisat_id
        self.tlm_topic = "osk/pisat-%s/tlm" % pisat_id

        self.json_tlm_keys =  [self.JSON_TLM_SEQ_CNT, self.JSON_TLM_RUNNING, self.JSON_TLM_CMD_CNT, self.JSON_TLM_APPS, self.JSON_TLM_EVENT]

        
    def on_connect(self, client, userdata, flags, rc):
        """
        """
        print("Connected with result code {0}".format(str(rc)))  # Print result of connection attempt 
        print("Subscribing to %s" % self.tlm_topic)
        self.client.subscribe(self.tlm_topic)
 
 
    def publish_cmd(self, target, cmd):
        """
        """
        payload = '{"%s": "%s"}' % (target, cmd)
        print("Command Target: %s, Command: %s, Payload: %s" % (target,cmd,payload))
        self.client.publish(self.cmd_topic, payload)

 
    def process_tlm(self, client, userdata, msg):
        """
        The callback for when a PUBLISH message is received from the server. The payload is
        a JSON telemetry object with the following fields:
        
            {
                "seq-cnt": integer,
                "running": boolean,
                "apps": "Comma separated app names",
                "event": "Event message string",
            }
        
            The field names must match the GUI keys. See the class defined constants.
            
        Test string
            {"seq-cnt": 1, "running": true, "apps": "LAB_CI, FILE_MGR,...", "event": "This is a test"}
        
        """
        msg_str = msg.payload.decode()
        print("Message received-> " + msg.topic + " " + msg_str)
        tlm = json.loads(msg.payload.decode())
        for key in tlm:
            if key in self.json_tlm_keys:
                self.window[key].update(tlm[key])
            #TODO - Is it an error to have keys not recognized by this GUI?
            
    def create_window(self):

        hdr_label_font = ('Arial bold',12)
        hdr_value_font = ('Arial',11)

        layout = [
            [sg.Button('',visible=False)], # If this isn't present then the first button in teh frame will have a border. Seems like a bug 
            [sg.Frame('Commands', 
                [[sg.Text('Pi-Sat:',    size=(5,1), font=hdr_label_font, pad=((5,0),(12,12))), 
                sg.Button('Noop',       size=(9,1), font=hdr_label_font, enable_events=True, key='-PISAT_NOOP-',     pad=((10,5),(12,12))),
                sg.Button('Reboot',     size=(9,1), font=hdr_label_font, enable_events=True, key='-PISAT_REBOOT-',   pad=((10,5),(12,12))),
                sg.Button('Shutdown',   size=(9,1), font=hdr_label_font, enable_events=True, key='-PISAT_SHUTDOWN-', pad=((10,5),(12,12)))],
                [sg.Text('cFS:',        size=(5,1), font=hdr_label_font, pad=((5,0),(12,12))), 
                sg.Button('Start',      size=(9,1), font=hdr_label_font, enable_events=True, key='-CFS_START-',   pad=((10,5),(12,12))),
                sg.Button('Enable Tlm', size=(9,1), font=hdr_label_font, enable_events=True, key='-CFS_ENA_TLM-', pad=((10,5),(12,12))),
                sg.Button('Stop',       size=(9,1), font=hdr_label_font, enable_events=True, key='-CFS_STOP-',    pad=((10,5),(12,12)))]
                ])
            ],
                [sg.Text('  ', pad=((10,5),(12,12)))],
            [sg.Frame('Status',
                [[sg.Text('Seq Cnt:',   size=(9,1),   font=hdr_label_font, pad=((5,0),(6,6))), 
                sg.Text('0',            size=(9,1),   font=hdr_value_font, key=self.JSON_TLM_SEQ_CNT)],
                [sg.Text('cFS Exe:',    size=(9,1),   font=hdr_label_font, pad=((5,0),(6,6))), 
                sg.Text('False',        size=(9,1),   font=hdr_value_font, key=self.JSON_TLM_RUNNING)],
                [sg.Text('Cmd Cnt:',    size=(9,1),   font=hdr_label_font, pad=((5,0),(6,6))), 
                sg.Text('0',            size=(9,1),   font=hdr_value_font, key=self.JSON_TLM_CMD_CNT)],
                [sg.Text('User Apps:',  size=(9,1),   font=hdr_label_font, pad=((5,0),(6,6))), 
                sg.Text('None',         size=(45,1),  font=hdr_value_font, key=self.JSON_TLM_APPS)],
                [sg.Text('Event:',      size=(9,1),   font=hdr_label_font, pad=((5,0),(6,6))), 
                sg.Text('None',         size=(45,1),  font=hdr_value_font, key=self.JSON_TLM_EVENT)]
                ])
            ]
        ]

        window = sg.Window('Pi-Sat Control', layout, auto_size_text=True, finalize=True)
        return window


    def execute(self):
        try:
            self.client = mqtt.Client(self.client_name)
            self.client.on_connect = self.on_connect   # Callback function for successful connection
            self.client.on_message = self.process_tlm  # Callback function for receipt of a message
            self.client.connect(broker_addr)
            self.client.loop_start()  # Start networking daemon
            print("After client loop")
            
        except:
            logger.error("Error configuring MQTT client")
            return
            
        self.window = self.create_window()
                        
        while True:
    
            self.event, self.values = self.window.read(timeout=250)

            if self.event in (sg.WIN_CLOSED, 'Exit') or self.event is None:
                break

            if self.event == '-PISAT_NOOP-':
                self.publish_cmd(self.JSON_CMD_TARGET_PI, self.JSON_CMD_PI_NOOP)

            elif self.event == '-PISAT_REBOOT-':
                self.publish_cmd(self.JSON_CMD_TARGET_PI, self.JSON_CMD_PI_REBOOT)
            
            elif self.event == '-PISAT_SHUTDOWN-':
                self.publish_cmd(self.JSON_CMD_TARGET_PI, self.JSON_CMD_PI_SHUTDOWN)

            elif self.event == '-CFS_START-':
                self.publish_cmd(self.JSON_CMD_TARGET_CFS, self.JSON_CMD_CFS_START)
                                 
            elif self.event == '-CFS_ENA_TLM-':
                self.publish_cmd(self.JSON_CMD_TARGET_CFS, self.JSON_CMD_CFS_ENA_TLM)

            elif self.event == '-CFS_STOP-':
                self.publish_cmd(self.JSON_CMD_TARGET_CFS, self.JSON_CMD_CFS_STOP)


###############################################################################

if __name__ == '__main__':

    config = configparser.ConfigParser()
    config.read('../cfsat.ini')

    broker_addr = config.get('NETWORK','MQTT_BROKER_ADDR')
    broker_port = config.get('NETWORK','MQTT_BROKER_PORT')
    print("Broker Address: %s, Port: %s" % (broker_addr, broker_port))
    
    client_name = config.get('NETWORK','MQTT_CLIENT_NAME')
    Pi_control = PiControl('1', broker_addr, broker_port, client_name)
    
    Pi_control.execute()
    

