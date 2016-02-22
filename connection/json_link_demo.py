#!/usr/local/bin/python3

"""
json_link_demo
~~~~~~~~~~~~~~
    
A very simple wrapper for json_link/bin/json_link_tester

This provides a feature rich command-line interface for testing both master
and slave sides of the JSON link.

See json_link/bin/json_link_tester for more details

@author: Bryan Palmintier, NREL 2013
"""
__author__ = "Bryan Palmintier"
__copyright__ = "Copyright (c) 2013 Bryan Palmintier"
__license__ = """ 
@copyright: Copyright (c) 2013, Bryan Palmintier
@license: BSD 3-clause --
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:
 1) Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
 2) Redistributions in binary form must reproduce the above copyright notice, 
    this list of conditions and the following disclaimer in the documentation 
    and/or other materials provided with the distribution.
 3) Neither the name of the National Renewable Energy Lab nor the names of its 
    contributors may be used to endorse or promote products derived from this 
    software without specific prior written permission.
    
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."""

## ===== History ====
#  [Current]             version     date       time     who     Comments
#                        -------  ----------    ----- ---------- ------------- 
__version__, __date__ = "1.0",   "2013-04-29"  #17:13   BryanP   Initial version, as part of json_link conversion to package
# [Older, in reverse order]
# version      date       time     who     Comments
# -------   ----------    ----- ---------- --------------------------------------- 

from json_link.bin import json_link_tester

if __name__ == '__main__':
    json_link_tester.main()