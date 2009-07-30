import sys
import email
import os
import shutil
import smtplib
import time
import subprocess


os.putenv("PATH","%PATH%;..\\..\\..\\VS2005\\Win32\\Debug")
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
	#scan for --help and --clean
	if len(argv) > 1:
		for arg in argv:
			if "--help" in arg:
				do_help();
				exit(0)
			if "--clean" in arg:
				clean = 1

	print("Starting autotest script")
	
	there_dir = os.getcwd()
	err_ct = 0
	ex_ct = 0
	start_time = time.time()
	
	if clean == 1:
		print("Go clean?")
	
	# determine where the script starts
	here_dir = os.getcwd()
	print("Starting from \'"+here_dir+"\'")
	
	#determine where we _want_ the script to start
	if len(argv) > 1:
		argn = 1
		while argn < len(argv):
			if argv[argn][0] == '-':
				argn+=1
				# ignore anything that looks like a flag
			else:
				there_dir = argv[argn]
				# take the first path and be done with it
				# poor form but we're not doing much complicated
				break
	
	#locate autotest dirs
	#autotestdirs = find_autotest_dirs(there_dir)
	autotestdirs = []
	for path, dirs, files in os.walk(there_dir):
		if "autotest" in dirs:
			autotestdirs.append(os.path.abspath(os.path.join(path,"autotest")))
	
	for path in autotestdirs:
		print("Found dir: \'"+path+"\'")
	
	#locate autotest test files
	#autotestfiles = find_autotest_files(autotestdirs)
	autotestfiles = []
	for path in autotestdirs:
		for file in os.listdir(path):
			if "test_" in file and ".glm" in file and file[0] != '.':
				autotestfiles.append((path, file))
	
	for path, file in autotestfiles:
		print("Found file: \'"+file+"\'")
	
	#build test dirs
	#for file in autotestfiles:
	errlist=[]
	cleanlist=[]
	errct = 0
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
		#print("NOTICE:  Running \'"+xfile+"\'")
		rv = subprocess.call(["gridlabd",xfile],stdout=outfile,stderr=errfile)
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
					print("WARNING: Optional file "+file+" converged when it shouldn't've!")
					cleanlist.append((path, file))
					err = False
				else:
					print("ERROR: "+file+" converged when it shouldn't've!")
					errct += 1
					err = True
			elif rv == 2:
				print("SUCCESS: File "+file+" failed to converge, as planned.")
				cleanlist.append((path, file))
			elif rv == 1:
				print("ERROR:  "+file+" failed to load!")
				errct += 1
				err = True
			else:
				print("ERROR:  "+file+" ended with unrecognized return value! ("+str(rv)+")")
				errct += 1
				err = True
		else:
			if rv == 2:
				if "opt_" in file or "_opt" in file:
					print("WARNING: Optional file "+file+" failed to converge!")
					cleanlist.append((path, file))
					err = False
				else:
					errct += 1
					print("ERROR: "+file+" failed to converge!")
					err = True
			elif rv == 1:
				print("ERROR:  "+file+" failed to load!")
				errct += 1
				err = True
			elif rv == 0:
				print("SUCCESS: File "+file+" converged successfully.")
				cleanlist.append((path, file))
			else:
				print("ERROR:  "+file+" ended with unrecognized return value! ("+str(rv)+")")
				errct += 1
				err = True
		if err:
			# zip target directory
			errlist.append((path,file))
			
		os.chdir(currpath)
		#print("cwd: "+currpath)
		# end autotestfiles loop

	#cleanup as appropriate
	#for cleanfile, cleanpath in cleanlist:
	#	for path, dirs, file in os.walk(cleanpath):
	#		print("foo")
	#end
	#print("bar")
	
	#return success/failure
	print("Validation detected "+str(errct)+" models with errors.")
	for errpath, errfile in errlist:
		print(" * "+os.path.join(errpath, errfile))
	
	exit(errct)
	#return errct
#end run_tests()

if __name__ == '__main__':
	run_tests(sys.argv)
#end main

#end validate_new.py
