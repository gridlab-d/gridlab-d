import sys
import email
import os
import shutil
import smtplib
import time
import subprocess

##
#	run_tests is the main function for the autotest validation script.
#	@param	argv	The command line arguements.
def run_tests(argv):
	print("Starting autotest script")
	
	there_dir = os.getcwd()
	err_ct = 0
	
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
			if "test_" in file and ".glm" in file:
				autotestfiles.append((path, file))
	
	for path, file in autotestfiles:
		print("Found file: \'"+file+"\'")
	
	#build test dirs
	#for file in autotestfiles:
	errlist=[]
	errct = 0
	for path, file in autotestfiles:
		err = False
		slice = file[:-4]
		xpath = os.path.join(path,slice) # path/slice
		xfile = os.path.join(xpath,file) # path/slice/file.glm
		if not os.path.exists(xpath):
			os.mkdir(xpath)
		shutil.copy2(os.path.join(path,file), xfile) # path/file to path/slice/file
		
		# build conf files
		# moo?
		
		#run files with popen
		#run file with:
		outfile = open(os.path.join(xpath,"outfile.txt"), "w")
		errfile = open(os.path.join(xpath,"errfile.txt"), "w")
		rv = subprocess.call(["gridlabd",xfile],stdout=outfile,stderr=errfile)
		outfile.close()
		errfile.close()
		
		# handle results
		if "err_" in file:
			if rv == 0:
				if "opt_" in file:
					print("WARNING: Optional file "+file+" converged when it shouldn't've!")
					err = False
				else:
					print("ERROR: "+file+" converged when it shouldn't've!")
					errct += 1
					err = True
			else:
				print("SUCCESS: File "+file+" failed to converge, as planned.")
		else:
			if rv != 0:
				if "opt_" in file:
					print("WARNING: Optional file "+file+" failed to converge!")
					err = False
				else:
					errct += 1
					print("ERROR: "+file+" failed to converge!")
					err = True
			else:
				print("SUCCESS: File "+file+" converged successfully.")
		if err:
			# zip target directory
			errlist.append((path,file))
		# end autotestfiles loop
	
	#return success/failure
	print("Validation detected "+str(errct)+" models with errors.")
	return errct
#end run_tests()

if __name__ == '__main__':
	run_tests(sys.argv)
#end main

#end validate_new.py
