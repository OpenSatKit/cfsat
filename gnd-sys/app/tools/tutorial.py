#!/usr/bin/env python
"""
Provide classes to manage tutorials and lessons within a tutorial.

JSON key constants should all be used within the Json class so if any key changes
only the Json class will change.

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

TUTORIAL_JSON_FILE = 'tutorial.json'
LESSON_JSON_FILE   = 'lesson.json'
LESSON_DIR         = 'lesson'

###############################################################################

class TutorialJson(JsonFile):
    """
    Manage tutor and lesson JSON files. Keep tag names and formats consistent.
    The tutor and lesson JSON files have nearly idenical keys and since this is
    not a generic utility this one class takes care of both file types.
    """
    def __init__(self, json_file):
        super().__init__(json_file)
        
    def lesson_slide(self):
        return self.json['current-slide']

    def lesson_complete(self):
        return self.json['complete']

    def reset_child(self):
        if 'current-slide' in self.json:
           self.json['current-slide'] = 1
           self.json['complete'] = False

    def set_complete(self, complete):
        self.json['complete'] = complete

    def set_slide(self, slide):
        self.json['current-slide'] = slide
    
    
###############################################################################


class Lesson():
    """
    Manage the display for a lesson. The lesson's JSON file is used to 
    determine the initial state. The execute() method allows a lesson to be
    restarted and override the JSON. The new lesson state is recorded in the
    JSON when the lesson is exited.
    """
    def __init__(self, number, path, slides):

        self.number    = number
        self.path      = path
        self.slides    = slides
        self.slide_max = len(slides)
        self.load_json()
    
    def load_json(self):
    
        self.json = TutorialJson(os.path.join(self.path, LESSON_JSON_FILE))
        
        self.title     = self.json.title()
        self.cur_slide = self.json.lesson_slide()
        self.complete  = self.json.lesson_complete()

        
    def update_slide_file(self):
        self.slide_file = os.path.join(self.path, self.slides[self.cur_slide-1])
    
    
    def execute(self):
    
        self.update_slide_file()
        
        self.layout = [
                       [sg.Text('Slide'), sg.Text(str(self.cur_slide), pad=(2,1), key='-SLIDE-')],   
                       [sg.Image(filename=self.slide_file, key='-IMAGE-')],
                       [sg.Button('Prev', size=(8,2)), sg.Button('Next', size=(8, 2)), sg.Button('Mark as Complete', pad=(10,0))]
                      ]
 
        self.window = sg.Window(self.json.title(), self.layout, element_justification='c', resizable=True, finalize=True)
        
        
        while True:
        
            self.event, self.values = self.window.read()
            
            if self.event in (sg.WIN_CLOSED, 'Exit') or self.event is None:
                break

            if self.event == 'Mark as Complete':
               self.json.set_complete(True)
               break
               
            if self.event == 'Prev':
                if self.cur_slide > 1:
                    self.cur_slide -= 1
            
            elif self.event == 'Next':
                if self.cur_slide < self.slide_max:
                    self.cur_slide += 1
            
            self.update_slide_file()
            logger.debug("filename = " + self.slide_file)
            
            self.window['-SLIDE-'].update(str(self.cur_slide))
            self.window['-IMAGE-'].update(filename=self.slide_file)

        self.json.set_slide(self.cur_slide)
        self.json.update()
        self.window.close()       

        return self.json.lesson_complete()
        
    def reset(self):
        self.json.reset()
        self.load_json()


###############################################################################


class Tutorial():
    """
    Manage the display for a tutorial. A tutorial folder contains a
    tutorial.json that describes the tutorial and a lesson folder that 
    contains numbered lesson folders. Each lesson folder contains a 
    lesson.json file.  
    """
    def __init__(self, tutorial_path):

        self.path = tutorial_path
        self.json = TutorialJson(os.path.join(tutorial_path, TUTORIAL_JSON_FILE))
        
        self.lesson_path = os.path.join(tutorial_path,LESSON_DIR)
        logger.info("self.lesson_path  = " + self.lesson_path)
        self.lesson_list = [int(l) for l in os.listdir(self.lesson_path) if l.isnumeric()]
        self.lesson_list.sort()
        logger.info("self.lesson_list = " + str(self.lesson_list))
        self.lesson_objs = {}
        for l in self.lesson_list:
            lesson_num_path = os.path.join(self.lesson_path, str(l))
            logger.info("lesson_num_path = " + lesson_num_path)
            lesson_pngs = [f for f in os.listdir(lesson_num_path) if f.lower().endswith('.png')]
            lesson_pngs.sort()
            logger.info("lesson_pngs = " + str(lesson_pngs))
            self.lesson_objs[l] = Lesson(l, lesson_num_path, lesson_pngs)
        
        self.window  = None
        self.display = True
        self.reset   = False

    def gui(self):
        """
        Navigating through lessons is not strictly enforced.  The goal is to keep the user
        interface very simple so the algotihm to determine which lesson to resume is simplistic
        and its up to the user whether they select lessons as completed.
        """
        
        hdr_label_font = ('Arial bold',12)
        hdr_value_font = ('Arial',12)
        
        objective_text = ""
        for objective_line in self.json.objective():
            objective_text += objective_line

        resume_lesson = 1
        for lesson in self.lesson_objs.values():
            if lesson.complete:
                resume_lesson += 1
            else:
                break
                    
        lesson_layout = []
        for lesson in self.lesson_objs.values():
            logger.debug("Lesson Layout " + lesson.title)
            title          = "%d-%s" % (lesson.number, lesson.title)
            complete_state = "Yes" if lesson.complete else "No"
            radio_state    = True if lesson.number == resume_lesson else False
            lesson_layout.append([sg.Radio(title, "LESSONS", default=radio_state, font=hdr_value_font, size=(30,0), key='-LESSON%d-'%lesson.number), sg.Text(complete_state, key='-COMPLETE%d-'%lesson.number)])
        
        self.layout = [
                       [sg.Text('Objectives', font=hdr_label_font)],
                       [sg.MLine(default_text=objective_text, font = hdr_value_font, size=(40, 4))],
                       # Lesson size less than lesson layout so complete status will appear centered 
                       [sg.Text('Lesson', font=hdr_label_font, size=(28,0)),sg.Text('Complete', font=hdr_label_font, size=(10,0))],  
                       lesson_layout, 
                       [sg.Button('Start'), sg.Button('Reset'), sg.Button('Exit')]
                      ]
        
        self.window = sg.Window(self.json.title(), self.layout, finalize=True)

        while True: # Event Loop
            
            self.event, self.values = self.window.read()

            if self.event in (sg.WIN_CLOSED, 'Exit') or self.event is None:       
                break
            
            if self.event == 'Start':
                for lesson in self.lesson_objs:
                    if self.values["-LESSON%d-"%lesson] == True:
                        if self.lesson_objs[lesson].execute():
                            self.window['-COMPLETE%d-'%lesson].update('Yes') 
                
            if self.event == 'Reset':
                for lesson in list(self.lesson_objs.values()):
                   lesson.reset()   
                self.reset = True
                break
        
        self.json.update()
        self.window.close()
        
        
    def execute(self):
    
        while self.display:
           self.gui()
           if self.reset:
              self.reset = False
           else:
              self.display = False


###############################################################################

class ManageTutorials():
    """
    Manage the display for a tutorial. A tutorial folder conatins a
    tutorial.json that describes the tutorial and a lesson folder that 
    contains numbered lesson folders. Each lesson folder contains a 
    lesson.json file.  
    """
    def __init__(self, tutorials_path):

        self.path = tutorials_path
        self.tutorial_titles = []
        self.tutorial_lookup = {}  # [title]  => Tutorial
        
        for tutorial_folder in os.listdir(tutorials_path):
            logger.debug("Tutorial folder: " + tutorial_folder)
            #todo: Tutorial constrcutor coudl raise exception if JSON doesn't exist or is malformed
            tutorial_json_file = os.path.join(tutorials_path, tutorial_folder, TUTORIAL_JSON_FILE)
            if os.path.exists(tutorial_json_file):
                tutorial = Tutorial(os.path.join(tutorials_path, tutorial_folder))
                self.tutorial_titles.append(tutorial.json.title())
                self.tutorial_lookup[tutorial.json.title()] = tutorial
        
        logger.debug("Tutorial Titles " + str(self.tutorial_titles))
        logger.debug("Tutorial Lookup " + str(self.tutorial_lookup))

    def run_tutorial(self, tutorial_title):
        if tutorial_title in self.tutorial_titles:
            self.tutorial_lookup[tutorial_title].execute()

###############################################################################

if __name__ == '__main__':

    config = configparser.ConfigParser()
    config.read('../cfsat.ini')

    TUTORIALS_PATH = config.get('TOOLS','TUTORIALS_PATH')

    tutorial_dir = os.path.join(os.getcwd(),'..', TUTORIALS_PATH, 'hello-app') 
    print ("tutorial_dir = " + tutorial_dir)
    tutorial = Tutorial(tutorial_dir)
    tutorial.execute()
    
    

