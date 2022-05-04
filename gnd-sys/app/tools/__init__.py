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
        Module initialization file

"""

import logging
logger = logging.getLogger(__name__)
logger.addHandler(logging.NullHandler())

from .apptemplate import CreateApp
from .appstore import AppStore
from .tutorial import ManageTutorials
from .texteditor import TextEditor
from .utils import *


