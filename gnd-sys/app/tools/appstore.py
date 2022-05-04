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
        Provide classes that manage the creation of apps from templates.
"""

import sys
import time
import os
import requests
import configparser
from datetime import datetime

import logging
logger = logging.getLogger(__name__)

if __name__ == '__main__':
    from utils    import compress_abs_path
else:
    from .utils    import compress_abs_path

import PySimpleGUI as sg


###############################################################################


class GitHubAppProject():
    '''
    Manage the interface to cFS apps in github repos  
    '''
    def __init__(self, git_url, usr_app_path):
        """
        usr_app_path is an absolute path to where git repos should be cloned into
        """
        self. usr_clone_path = usr_app_path
        self.git_url  = git_url
        self.app_repo = None
        self.app_dict = {}
         
         
    def create_dict(self):
        """
        Queries git URL to a list of apps. A dictionary is created using app
        names as the keys. This function is not part of the constructor 
        to allow the caller to interact with the user in the event that the
        URL can't be accessed.
        """
        ret_status = False
        self.app_repo = requests.get(self.git_url)
        if self.app_repo.status_code == 200:
            app_repo_list = self.app_repo.json() 
            # Create a dictionary with app names as the key
            for repo in app_repo_list:                
                print(repo['name'])
                print(repo['git_url'])
                self.app_dict[repo['name']] = repo
            #print(self.app_dict['kit_ci'])
            ret_status = True
        return ret_status
        

    def clone(self, app_name):
        """
        """
        if app_name in self.app_dict:
            saved_cwd = os.getcwd()
            os.chdir(self. usr_clone_path)
            clone_url = self.app_dict[app_name]["clone_url"]
            print("Cloning " + clone_url)
            os.system("git clone {}".format(self.app_dict[app_name]["clone_url"]))
            os.chdir(saved_cwd)

    def get_descr(self, app_name):
        """
        """
        descr = ''
        if app_name in self.app_dict:
            descr = self.app_dict[app_name]['description']
        return descr

###############################################################################

class AppStore():
    """
    Manage the user interface for downloading apps from github and cloning
    them into the user's app directory. 
    """
    def __init__(self, git_url, usr_app_path):

        self.git_app_repo = GitHubAppProject(git_url, usr_app_path)
        self.window  = None

        
    def create_window(self):
        """
        """
        hdr_label_font = ('Arial bold',12)
        hdr_value_font = ('Arial',12)
        app_layout = []
        for app in self.git_app_repo.app_dict.keys():
            print(app)
            app_layout.append([sg.Radio(app, "APPS", default=False, font=hdr_label_font, size=(10,0), key='-%s-'%app),  
                               sg.Text(self.git_app_repo.get_descr(app), font=hdr_value_font, size=(30,1))])
                
        layout = [
                  [sg.Text("Select an app to download then follow the 'Add App' tutorial", font=hdr_value_font)],
                  app_layout, 
                  [sg.Button('Download', font=hdr_label_font), sg.Button('Cancel', font=hdr_label_font)]
                 ]

        window = sg.Window('Download App', layout, modal=False)
        return window


    def gui(self):
        """
        """        
        self.window = self.create_window() 
        
        while True: # Event Loop
            
            self.event, self.values = self.window.read()

            if self.event in (sg.WIN_CLOSED, 'Cancel') or self.event is None:       
                break
            
            if self.event == 'Download':
                for app in self.git_app_repo.app_dict.keys():
                    if self.values["-%s-"%app] == True:
                        self.git_app_repo.clone(app)
                break
                
        self.window.close()

    def execute(self):
        """
        """        
        if self.git_app_repo.create_dict():
            self.gui()
        else:
            sg.popup('Error accessing the git url %d. Check your network connection and the git URL'%self.git_app_repo.git_url, 'Error')


###############################################################################

if __name__ == '__main__':

    config = configparser.ConfigParser()
    config.read('../cfsat.ini')

    git_url = config.get('APP','APP_STORE_URL')
    usr_app_path = compress_abs_path(os.path.join(os.getcwd(),'..', config.get('PATHS', 'USR_APP_PATH'))) 

    app_store = AppStore(git_url, usr_app_path)
    app_store.execute()
    
    
    
    

