#!/usr/bin/python

import sys
import time
import subprocess

installer_prefix = "gridlabd-nightly"
if len(sys.argv) > 1:
	installer_prefix = sys.argv[1]

(y,m,d) = time.localtime()[:3]
y = str(y)[2:]
installer_name = installer_prefix + "_%02d%02d%s"%(m,d,y)

subprocess.call(["iscc.exe","/F"+installer_name ,'gridlabd.iss'],shell=True) 
subprocess.call(["iscc.exe","/F"+installer_prefix ,'gridlabd.iss'],shell=True) 