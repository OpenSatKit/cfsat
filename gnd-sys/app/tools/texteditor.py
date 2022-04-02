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
        Provide a simple text editor to assist users with their app development
        workflows. This is not intended to replace a full-featured editor.
  
    TODO - Resolve all TODO dialogs
    TODO - Add dirty flag and prompt user to save before exit without save
          
"""
import sys
import time
import os
from datetime import datetime

import logging
logger = logging.getLogger(__name__)

import PySimpleGUI as sg


###############################################################################

class EditorConfig():
    """
    """
    def __init__(self):
  
        self.config = {
            'theme':   'BluePurple',
            'themes':  sg.list_of_look_and_feel_values(),
            'font':    ('Consolas', 12),
            'tabsize': 4 
            }
        
    def get(self, type):
        config = None
        if type in self.config:
            config = self.config[type]
        return config


###############################################################################

class HelpText():
    """
    """
    def __init__(self):
  
        self.text = {

            'NEW_FILE': 
               ("This is a simple text file editor intended to help with quick edits to assist users\n"
                "with their app development workflows. The 'Execute' menu provides quick access to\n"
                "common activities like building the cFS. The following file types are recognized by\n"
                "the editor and the lower GUI pane provides file specific guidance.\n\n"
                "  '*.py'               - User test scripts\n"
                "  '*.json'             - cFS application initialization and table files\n"
                "  'targets.cmake'      - Define cFS build target apps and files\n"
                "  'cfe_es_startup.scr' - Define apps and libraries loaded during cFS initialization\n"),
         
            'CMAKE':  ("TODO - CMAKE"),
            'SCR':    ("TODO - SCR"),
            'JSON':   ("TODO - JSON"),
            'PYTHON': ("TODO - PYTHON"),
            'OTHER':  ("No guidance for this file type")
                
            }
            
    def display(self, filename):
        file_type = 'NEW_FILE'
        if filename not in (None, ''):
            file_ext = filename.split('.')[-1]
            print('file_ext = ' + file_ext)
            file_type = 'OTHER'
            
        sg.popup(self.text[file_type], line_width=85, font=('Courier',12), title='File Guidance', grab_anywhere=True)


###############################################################################

class TextEditor():
    """
    Provide a user interface for managing ground and flight directories and
    files. It also supports transferring files between the flight and ground.
    """
    
    NEW_FILE_STR = '-- New File --'
    
    def __init__(self, filename=None):        
      
        self.filename = None
        if filename not in (None, ''):
           if os.path.isfile(filename):
               self.filename = filename
           
        self.config = EditorConfig()
        self.help_text = HelpText()
    
    def open_file(self, filename, window):
        
        if filename not in (None,''):
            if os.path.isfile(filename):
                self.filename = filename
                with open(self.filename,'r') as f:
                    file_text = f.read()
                window['-FILE_TEXT-'].update(value=file_text)
                window['-FILE_TITLE-'].update(value=self.filename)
    
    def save_file(self, filename, window):
        """
        This function protects against multiple filename empty types. None
        amd a string length of 0 are most common, but this could be called 
        with the return value after a cancelled save so filename could be
        a zero length tuple.
        """
        updated = False
        if filename is not None:
            if len(self.filename) > 0:
                with open(filename,'w') as f:
                    f.write(self.values['-FILE_TEXT-'])
                window['-FILE_TITLE-'].update(value=filename)
                self.filename = filename
                updated = True
                               
        if not updated:
            window['-FILE_TITLE-'].update(value = self.NEW_FILE_STR)

    def text_left_click(self):
        print('text_left_click')
        
    def create_window(self):
        """
        Create the main window. Non-class variables are used so it can be refreshed, PySimpleGui
        layouts can't be shared.
        This editor is intentionally very simple. I orginally had the guidance as a second window
        pane but this wastes screen space and is annoying when you don't need the guidance.
        """
        window_width = 100
        menu_layout = [
                ['File',['New','Open','Save','Save As','---','Exit']],
                ['Edit',['Select All','Cut','Copy','Paste','Undo','---','Find...','Replace...']],
                ['Execute',['Build cFS', 'Run Script']],
            ]

        self.file_text = sg.Multiline(default_text='', font=self.config.get('font'), key='-FILE_TEXT-', size=(window_width,30))
        
        window_layout = [
            [sg.Menu(menu_layout)],
            [[
              sg.Button('Help', enable_events=True, key='-FILE_HELP-', pad=((5,5),(12,12))),
              sg.Text(self.NEW_FILE_STR, key='-FILE_TITLE-', font=self.config.get('font'), size=(window_width-10,1))
            ]],
            [sg.Column([[self.file_text]])]]


        window = sg.Window('Text File Editor', window_layout, resizable=True, margins=(0,0), return_keyboard_events=True, finalize=True)
        return window
       
                
    def gui(self):
    
        window = self.create_window()
        
        #TODO window.find_element['-FILE_TEXT-'].Widget.bind('<Button-1>', self.text_left_click()) #Copy') #
        #TODO self.file_text.Widget.bind('<Button-1>', self.text_left_click())
        
        if self.filename is not None:
            self.open_file(self.filename, window)
        
        while True:

            self.event, self.values = window.read(timeout=100)
        
            if self.event in (sg.WIN_CLOSED, 'Exit') or self.event is None:
                break

            elif self.event in ('-FILE_HELP-'):
                self.help_text.display(self.filename)

            ### File Menu ###

            elif self.event in ('New',):
                self.filename = None
                window['-FILE_TEXT-'].update(value = '')
                window['-FILE_TITLE-'].update(value = self.NEW_FILE_STR)
                
            elif self.event in ('Open',):
                try: # Some platforms may raise exceptions
                    filename = sg.popup_get_file('File Name:', title='Open', no_window=True)
                    self.open_file(filename, window)
                except:
                    pass
              
            elif self.event in ('Save',):

                if self.filename in (None,''):
                    try: # Some platforms may raise exceptions on cancel
                        filename = sg.popup_get_file('Save File', save_as=True, no_window=True)
                        self.save_file(filename, window)
                    except:
                        pass
                
            elif self.event in ('Save As',):
                try: # Some platforms may raise exceptions on cancel
                    filename = sg.popup_get_file('Save File', save_as=True, no_window=True)
                    self.save_file(filename, window)
                except:
                    pass                
            
            ### Edit Menu ###
                
            elif self.event in ('Select All',):
                sg.popup("<Select All> not implemented", title='TODO', grab_anywhere=True, modal=False)

            elif self.event in ('Cut',):
                sg.popup("<Cut> not implemented", title='TODO', grab_anywhere=True, modal=False)

            elif self.event in ('Copy',):
                selection = window['-FILE_TEXT-'].Widget.selection_get()
                sg.clipboard_set(selection)
                
            elif self.event in ('Paste',):
                sg.popup("<Paste> not implemented. Clipboard contains '%s' "%sg.clipboard_get(), title='TODO', grab_anywhere=True, modal=False)

            elif self.event in ('Undo',):
                sg.popup("<Undo> not implemented", title='TODO', grab_anywhere=True, modal=False)

            elif self.event in ('Find...',):
                sg.popup("<Find...> not implemented", title='TODO', grab_anywhere=True, modal=False)

            elif self.event in ('Replace...',):
                sg.popup("<Replace...> not implemented", title='TODO', grab_anywhere=True, modal=False)


            ### Execute ###

            elif self.event in  ('Build cFS',):
                sg.popup("<Build cFS> not implemented", title='TODO', grab_anywhere=True, modal=False)

            elif self.event in  ('Run Script',):
                sg.popup("<Run Script> not implemented", title='TODO', grab_anywhere=True, modal=False)

            
        window.close()
        
    def execute(self):
        self.gui()
    

###############################################################################

if __name__ == '__main__':
    """
    sys.argv[0] - Name of script
    sys.argv[1] - If provided is the filename to be edited
    """
    print(f"Name of the script      : {sys.argv[0]=}")
    print(f"Arguments of the script : {sys.argv[1:]=}")

    filename = None
    if len(sys.argv) > 1:
        filename = sys.argv[1]
        #print ('filename = ' + filename)
        
    text_editor = TextEditor(filename)
    text_editor.execute()
    



