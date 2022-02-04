#!/usr/bin/env python
"""
Provide classes to manage the creation of apps from templates.

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

import sys
import time
import os
import json
import configparser
from datetime import datetime

import logging
logger = logging.getLogger(__name__)
if __name__ == '__main__':
    from jsonfile import JsonFile
else:
    from .jsonfile import JsonFile

import PySimpleGUI as sg

TEMPLATE_JSON_FILE = 'app_template.json'
TEMPLATE_VAR_UPPER = "@TEMPLATE@"
TEMPLATE_VAR_MIXED = "@Template@"
TEMPLATE_VAR_LOWER = "@template@"

TEMPLATE_COMMENT_START  = "/*##"
TEMPLATE_COMMENT_END    = "##*/"

###############################################################################

class AppTemplateJson(JsonFile):
    """
    Manage app template JSON files.
    """
    def __init__(self, json_file):
        super().__init__(json_file)
        
    def version(self):
        return self.json['version']

    def reset_child(self):
        pass
        
    def directories(self):
        return self.json['dir']

    def subdir_path(self, dir_list):
        return dir_list['path']

    def subdir_files(self, dir_list):
        return dir_list['files']
        
###############################################################################

class AppTemplate():
    """
    """
    def __init__(self, template_path):

        self.path = template_path
        self.json = AppTemplateJson(os.path.join(template_path, TEMPLATE_JSON_FILE))
        self.dirs = self.json.directories()
        
        self.appname = {}
        self.app_name_map = {}
        self.new_app_dir = None

    def create_app(self, app_name, cfs_app_dir):

        app_created = False
        
        self.app_name = {'UPPER': app_name.upper(), 'LOWER': app_name.lower(), 'MIXED': app_name.capitalize()}
        self.app_name_map = {
                               TEMPLATE_VAR_UPPER: self.app_name['UPPER'],
                               TEMPLATE_VAR_LOWER: self.app_name['LOWER'],
                               TEMPLATE_VAR_MIXED: self.app_name['MIXED'],
                            }
        
        self.new_app_dir = os.path.join(cfs_app_dir, self.app_name['LOWER'])
        
        try: 
            os.mkdir(self.new_app_dir) 
        except OSError: 
            print("Error creating " + self.new_app_dir)  
        
        for dir in self.dirs:
            
            subdir_path  = self.json.subdir_path(dir)
            subdir_files = self.json.subdir_files(dir)
            logger.debug("path = " + subdir_path)
            logger.debug("files = " + str(subdir_files))
            
            if len(subdir_path) > 0:
                template_file_path = os.path.join(self.path, subdir_path)
                new_app_file_path  = os.path.join(self.new_app_dir, subdir_path)
            else:
                template_file_path = self.path
                new_app_file_path  = self.new_app_dir
            
            try: 
                os.makedirs(new_app_file_path) 
            except OSError: 
                print("Error creating " + new_app_file_path)
            
            for template_file in subdir_files:
                self.instantiate_file(template_file_path, new_app_file_path, template_file)
                          
        app_created = True
      
        return app_created
        
    def instantiate_file(self, template_file_path, new_app_file_path, template_file):
        """
        Since template files should be short no need for fancy optimized 
        algorithms. Keep it simple and readable. 
        Comment blocks Delimited with keywords are skipped. 
        """
        logger.debug("template_file_path = " + template_file_path)
        logger.debug("new_app_file_path  = " + new_app_file_path)
        logger.debug("template_file      = " + template_file)
        replace_tokens = True
        instantiated_text = ""
      
        with open(os.path.join(template_file_path, template_file)) as f:
            for line in f:

                # Continue skipping until end of comment. Assume nothing else on last comment line
                if (replace_tokens):
                    if TEMPLATE_COMMENT_START in line:
                        replace_tokens = False
                    else:
                       # Replace all occurrences for each case
                       for template_token, app_name in self.app_name_map.items():
                           line = line.replace(template_token, app_name)
                       instantiated_text += line
                else:
                    replace_tokens = True if TEMPLATE_COMMENT_END in line else False

        # Replace template variable in filename 
        for template_token, app_name in self.app_name_map.items():
            template_file = template_file.replace(template_token, app_name)
        
        with open(os.path.join(new_app_file_path, template_file), 'w') as f:
            f.write(instantiated_text)
    			

        
###############################################################################

class CreateApp():
    """
    Create a database of app_templates and a display for a user to select
    one.  App_template titles defined in the JSON files are used as template
    identifiers for screen displays and as dictionary keys 
    """
    def __init__(self, app_templates_path):

        self.app_templates_path = app_templates_path
        self.app_template_titles = []
        self.app_template_lookup = {}  # [title]  => AppTemplate
        for app_template_folder in os.listdir(self.app_templates_path):
            logger.debug("App template folder: " + app_template_folder)
            #todo: AppTemplate constrcutor coudl raise exception if JSON doesn't exist or is malformed
            app_template_json_file = os.path.join(app_templates_path, app_template_folder, TEMPLATE_JSON_FILE)
            if os.path.exists(app_template_json_file):
                app_template = AppTemplate(os.path.join(app_templates_path, app_template_folder))
                self.app_template_titles.append(app_template.json.title())
                self.app_template_lookup[app_template.json.title()] = app_template
                    
        logger.debug("App Template Lookup " + str(self.app_template_lookup))
                
        self.window  = None
        self.selected_app = None
        
    def execute(self):
        """
        """
        
        hdr_label_font = ('Arial bold',12)
        hdr_value_font = ('Arial',12)
        
        app_template_layout = []
        for app_title, app_meta_data in self.app_template_lookup.items():
            logger.debug("self.app_template_lookup[%s] => %s" % (app_title, str(app_meta_data)))
            app_template_layout.append([sg.Radio(app_title, "APP_TEMPLATES", default=False, font=hdr_value_font, size=(30,0), key=app_title)])
        
        self.layout = [
                       [sg.Text('Select Application Template: ', font=hdr_label_font)],
                       app_template_layout, 
                       [sg.Button('Description'), sg.Button('Create App'), sg.Button('Cancel')]
                      ]
        
        self.window = sg.Window('Create Application', self.layout, finalize=True)

        while True: # Event Loop
            
            self.event, self.values = self.window.read()

            if self.event in (sg.WIN_CLOSED, 'Cancel') or self.event is None:       
                break
            
            self.selected_app = None
            for title in self.app_template_titles:
                if self.values[title] == True:
                    self.selected_app = self.app_template_lookup[title]
                    break
                    
            if self.event == 'Description':
                if self.selected_app is not None:
                    description = ""
                    for decsription_line in self.selected_app.json.description():
                        description += decsription_line
                    sg.popup(description, title=self.selected_app.json.title())
                else:
                    sg.popup("Please select an application template", title="Create Application")
                
            if self.event == 'Create App':
                if self.selected_app is not None:
                    app_name = None
                    layout = [[sg.T('Application Name')],
                              [sg.In(key='-INPUT-')],
                              [sg.OK(), sg.Button('Cancel')]]
                    window = sg.Window('Create Application', layout)
                    event, values = window.read()
                    if event == 'OK':
                        if values['-INPUT-'] is not None:
                            app_name = values['-INPUT-']
                            if len(app_name) > 0:
                                self.selected_app.create_app(app_name, os.path.join(os.getcwd(),'../../../cfs-apps'))
                    break
                else:
                    sg.popup("Please select an application template", title="Create Application")
        
        self.window.close()


###############################################################################

if __name__ == '__main__':

    config = configparser.ConfigParser()
    config.read('../cfsat.ini')

    APP_TEMPLATES_PATH = config.get('TOOLS','APP_TEMPLATES_PATH')

    templates_dir = os.path.join(os.getcwd(),'..', APP_TEMPLATES_PATH) 
    print ("templates_dir = " + templates_dir)
    CreateApp(templates_dir).execute()
    
    #Test without GUI
    #app_template = AppTemplate(template_dir)
    #app_template.create_app('hello', os.path.join(os.getcwd(),'../../../cfs-apps'))

    


