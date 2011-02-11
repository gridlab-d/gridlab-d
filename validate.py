# $Id: validate.py 1 2009-06-19 16:09:36Z mhauer $
# Copyright (C) 2009 Battelle Memorial Institute

import sys
import email
import os
import shutil
import smtplib
import time
import subprocess
import time
import archive_failed_tests
import getopt

def do_help():
	print("validate.py - GridLAB-D autotest/validation script")
	print("     validate.py [dir=.] ~ runs the autotest script with \'dir\' used as")
	print("                           the root directory to walk through")
	print("")
	print("  The validate script will search through all directories underneath the")
	print("   target directory and locate all files that start with \"test_\" inside")
	print("   directories named \"autotest\".  After locating as many files as possible,")
	print("   it will run each file in GridLAB-D and count the number of model files")
	print("   that fail to converge, then return that number.")
	return 0

##
#	run_tests is the main function for the autotest validation script.
#	@param	argv	The command line arguements.
def run_tests(argv):
	clean = 0
	printerr = 1
	there_dir = os.getcwd()
	err_ct = 0
	ex_ct = 0
	first_time = time.time()
	end_time = 0
	installed_dir = '/'
	
#	if clean == 1:
#		print("Go clean?")
	
	# determine where the script starts
	here_dir = os.getcwd()
#	print("Starting from \'"+here_dir+"\'")
	#scan for --help and --clean
	# Process command line arguments
	try:
		# process arguments Single character options with arguments are followed by :
		# Multi-character options with arguments are followed by =
		opts, args = getopt.getopt(argv[1:], "",["idir=",'help','clean','error'])
    	
		for o,a in opts:
			if "--help" in o or "-h" in o:
				do_help();
				sys.exit(0)
			elif "--clean" in o:
				clean = 1
			elif "--error" in o:
				if printerr == 0:
					printerr = 1
				if printerr == 1:
					printerr = 0
			elif "--idir" in o:
				installed_dir = a
		
		for arg in args:
			there_dir = arg
			break
            
	except getopt.GetoptError,error:
		print error.msg
		print error.opt
		do_help()
		return 2

	

	
	if(sys.platform == 'win32'):
		import platform
		cwd = sys.path[0]
		# Additional checks required because Python reports both 64 and 32 bit windows as 'win32'.
		if platform.architecture()[0] == '64bit':
			os.environ["PATH"]+=os.pathsep+cwd + "\\VS2005\\x64\\Release"
			os.environ["GLPATH"]=cwd+"\\VS2005\\x64\\Release"
		elif platform.architecture()[0] == '32bit':
			os.environ["PATH"]="c:\\MinGW\\bin"+os.pathsep+os.environ["PATH"]+os.pathsep+cwd + "\\VS2005\\Win32\\Release"
			os.environ["GLPATH"]=cwd+"\\VS2005\\Win32\\Release;"+os.pathsep+cwd +"\\core"
	elif(sys.platform.startswith('linux') or sys.platform.startswith('darwin')):
		os.environ["PATH"]+= os.pathsep+ installed_dir +"/bin"
		os.environ["GLPATH"]=installed_dir + "/lib/gridlabd"
	else:
		print "Validate not supported on this platform",sys.platform
		sys.exit(1)
		

	#print("Starting autotest script")
	
	
	#locate autotest dirs
	#autotestdirs = find_autotest_dirs(there_dir)
	autotestdirs = []
	for path, dirs, files in os.walk(there_dir):
		if "autotest" in dirs:
			autotestdirs.append(os.path.abspath(os.path.join(path,"autotest")))
	
#	for path in autotestdirs:
#		print("Found dir: \'"+path+"\'")
	
	#locate autotest test files
	#autotestfiles = find_autotest_files(autotestdirs)
	autotestfiles = []
	for path in autotestdirs:
		for file in os.listdir(path):
			if file.startswith("test_") and file.endswith(".glm") and file[0] != '.':
				autotestfiles.append((path, file))
	
#	for path, file in autotestfiles:
#		print("Found file: \'"+file+"\'")
	
	#build test dirs
	#for file in autotestfiles:
	errlist=[]
	testerrlist = []
	cleanlist=[]
	for path, file in autotestfiles:
		err = False
		slice = file[:-4]
		currpath = os.getcwd()
		xpath = os.path.join(path,slice) # path/slice
		xfile = os.path.join(xpath,file) # path/slice/file.glm
		if not os.path.exists(xpath):
			os.mkdir(xpath)
			if os.path.exists(os.path.join(xpath,"gridlabd.xml")):
				os.remove(os.path.join(xpath,"gridlabd.xml"))
		shutil.copy2(os.path.join(path,file), xfile) # path/file to path/slice/file
		
		os.chdir(xpath)
		#print("cwd: "+xpath)
		# build conf files
		# moo?
		
		#run files with popen
		#run file with:
		outfile = open(os.path.join(xpath,"outfile.txt"), "w")
		errfile = open(os.path.join(xpath,"errfile.txt"), "w")
		print("NOTICE:  Running \'"+xfile+"\'")
		sys.stdout.flush()
		start_time = time.time();
		rv = subprocess.call(["gridlabd",xfile],stdout=outfile,stderr=errfile)
		end_time = time.time();
		dt = end_time - start_time
		outfile.close()
		errfile.close()
		
		if os.path.exists(os.path.join(xpath,"gridlabd.xml")):
			statinfo = os.stat(os.path.join(xpath, "gridlabd.xml"))
			if(statinfo.st_mtime > start_time):
				if rv == 0: # didn't succeed if gridlabd.xml exists & updated since runtime
					rv = 1
		
		# handle results
		if "err_" in file or "_err" in file:
			if rv == 0:
				if "opt_" in file or "_opt" in file:
					print("WARNING: Optional file "+file+" converged when it shouldn't've!"+" ("+str(round(dt,2))+"s)")
					cleanlist.append((path, file))
					err = False
				else:
					print("ERROR: "+file+" converged when it shouldn't've!"+" ("+str(round(dt,2))+"s)")
					err_ct += 1
					err = True
			elif rv == 2:
#				print("SUCCESS: "+file+" failed to converge, as planned."+" ("+str(round(dt,2))+"s)")
				cleanlist.append((path, file))
			elif rv == 1:
				print("EXCEPTION:  "+file+" failed to load!"+" ("+str(dt)+"s)")
				ex_ct += 1
				err = True
			else:
				print("EXCEPTION:  "+file+" ended with unrecognized return value! ("+str(rv)+")"+" ("+str(round(dt,2))+"s)")
				ex_ct += 1
				err = True
		elif "exc_" in file or "_exc" in file:
			if rv == 0:
				if "opt_" in file or "_opt" in file:
					print("WARNING: Optional file "+file+" loaded when it shouldn't've!"+" ("+str(round(dt,2))+"s)")
					cleanlist.append((path, file))
					err = False
				else:
					print("ERROR: "+file+" loaded when it shouldn't've!"+" ("+str(round(dt,2))+"s)")
					err_ct += 1
					err = True
			elif rv == 1:
#				print("SUCCESS:  "+file+" failed to load, as planned"+" ("+str(round(dt,2))+"s)")
				cleanlist.append((path, file))
			else:
				print("EXCEPTION:  "+file+" ended with unrecognized return value! ("+str(rv)+")"+" ("+str(round(dt,2))+"s)")
				ex_ct += 1
				err = True
		else:
			if rv == 2:
				if "opt_" in file or "_opt" in file:
					print("WARNING: Optional file "+file+" failed to converge!"+" ("+str(round(dt,2))+"s)")
					cleanlist.append((path, file))
					err = False
				else:
					err_ct += 1
					print("ERROR: "+file+" failed to converge!"+" ("+str(round(dt,2))+"s)")
					err = True
			elif rv == 1:
				print("EXCEPTION:  "+file+" failed to load!"+" ("+str(round(dt,2))+"s)")
				ex_ct += 1
				err = True
			elif rv == 0:
#				print("SUCCESS: "+file+" converged"+" ("+str(round(dt,2))+"s)")
				cleanlist.append((path, file))
			else:
				print("EXCEPTION:  "+file+" ended with unrecognized return value! ("+str(rv)+")"+" ("+str(round(dt,2))+"s)")
				ex_ct += 1
				err = True
		if err:
			# zip target directory
			testerrlist.append(file[:-4])
			errlist.append((path,file))
			
		os.chdir(currpath)
		sys.stdout.flush()
		#print("cwd: "+currpath)
		# end autotestfiles loop

	#cleanup as appropriate
	#for cleanfile, cleanpath in cleanlist:
	#	for path, dirs, file in os.walk(cleanpath):
	#		print("foo")
	#end
	#print("bar")
	
	last_time = time.time()
	dt = last_time - first_time
	#return success/failure
	print("Validation detected "+str(err_ct)+" models with errors and "+str(ex_ct)+" models with exceptions in "+str(round(dt,2))+" seconds.")
	for errpath, errfile in errlist:
		print(" * "+os.path.join(errpath, errfile))
	if len(testerrlist) > 0:
		archive_failed_tests.createArchiveFiles(testerrlist)
	
	sys.exit(err_ct+ex_ct)
#end run_tests()

if __name__ == '__main__':
	run_tests(sys.argv)
#end main

#end validate_new.py
