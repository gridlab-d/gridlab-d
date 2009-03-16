#!/usr/bin/python

import sys
import time
import subprocess

installer_prefix = "gridlabd-nightly"
if len(sys.argv) > 2:
	installer_prefix = sys.argv[1]
	to_dir = sys.argv[2]
else:
	print "Too few arguments"

(y,m,d) = time.localtime()[:3]
y = str(y)[2:]
installer_name = installer_prefix + "_%02d%02d%s"%(m,d,y)

subprocess.call(["copy","Win32\\Release\\"+installer_name+".exe" ,to_dir],shell=True) 
