#!/usr/bin/env python
"""
Provide a base class to manage cfsat JSON files.

JSON key constants should all be used within the Json classes to
localize impacts due to JSON key changes

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
from abc import ABC, abstractmethod
import sys
import time
import os
import json
from datetime import datetime


###############################################################################

class JsonFile():
    """
    Abstract base class for managing JSON files in a consistent manner. This
    is designed for cfsat as opposed to general utility so it's up to the user
    to protect against errors.
    """
    def __init__(self, json_file):

        self.file = json_file
        f = open(json_file)
        self.json = json.load(f)
        f.close()
        
    def title(self):
        return self.json['title']

    def objective(self):
        return self.json['objective']

    def description(self):
        return self.json['description']

    def reset(self):
        """
        Reset only applies when a JSON file is being used to maintain state
        across executions.
        """
        self.json['time-stamp'] = ""
        self.reset_child()
        self.write_file()

    @abstractmethod
    def reset_child(self):
        raise NotImplementedError
            
    def update(self):
        self.json['time-stamp'] = datetime.now().strftime("%m/%d/%Y-%H:%M:%S")
        self.write_file()
    
    def write_file(self):
        with open(self.file, "w") as outfile:
            json.dump(self.json, outfile, indent=2)
    

