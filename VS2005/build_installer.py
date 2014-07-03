#!/usr/bin/python

import sys
import time
import subprocess

installer_prefix = "gridlabd-nightly"
if len(sys.argv) > 1:
	installer_prefix = sys.argv[1]

installer_script = "gridlabd.iss"
if len(sys.argv) > 2:
	installer_script = sys.argv[2]

(y,m,d) = time.localtime()[:3]
#y = str(y)[2:]
#installer_name = installer_prefix + "_%02d%02d%s"%(m,d,y)

installer_name = installer_prefix + "-%4d_%02d_%02d-nightly"%(y,m,d)

#gridlabd-win32-yyyy_mm_dd-nightly.exe

subprocess.call(["iscc.exe","/F"+installer_name ,installer_script],shell=True) 
subprocess.call(["iscc.exe","/F"+installer_prefix+"-nightly" ,installer_script],shell=True) 