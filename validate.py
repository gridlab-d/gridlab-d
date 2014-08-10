import sys
import email
import os
import shutil
import smtplib
import time
import subprocess
import time
import getopt

def do_help():
	print("Usage: validate.py [OPTION]... [DIRECTORY]")
	print("Run validation tests on test files in DIRECTORY.")
	print("")
	print("    --32                   run 32-bit build of executable, only for 64-bit Windows (may be used with --debug)")
	print("    -c, --clean            remove all directories created for testing and exit without running tests")
	print("    -d, --debug            use the debug build of the executable (default is the release build)")
	print("    -h, --help             print this help message")
	print("    -i=DIR, --idir=DIR     set the installation directory to DIR, only for Linux-based systems")
	print("    -r, --recompile-dll    delete all DLLs, forcing them to be recompiled")
	print("    -s, --suppress-opt     suppress output of summary of failing optional tests") 
	print("    -v, --verbose          display additional message for each file that passes")
	print("    -T=N                   use N threads")
	print("")
	print("With no DIRECTORY, all directories in the current directory are searched for directories named 'autotest'. Within each 'autotest' directory, files starting with 'test_' and ending with '.glm' are located. GridLAB-D is run on each file found, and the number of files not behaving as expected is returned.")
	print("")
	print("The default expected behavior is convergence of GridLAB-D on the test file.")
	print("Specifying '_err' or 'err_' in the filename of a test file specifies that the file is expected to fail.")
	print("Specifying '_exc' or 'exc_' in the filename of a test file specifies that the file is expected to cause an exception.")
	print("Specifying '_opt' or 'opt_' in the filename of a test file will cause this file not to be counted if it does not behave as expected.")
	print("")
	print("For each test file found, a temporary directory is created in the respective 'autotest' directory and each test file is copied into its temporary directory. If the test file generates a DLL (or an SO on Linux-based systems), it will not be regenerated on subsequent runs if the test file has not been modified since the DLL(SO) was created. The --recompile-dll option deletes all DLLs(SOs), forcing them to be regenerated. The --clean option can be used to remove all temporary directories created during testing and their contents.")
	print("")
	print("On Linux, the installation directory must be specified. The installation directory must contain the following directories:")
	print("    bin             this directory must contain the GridLAB-D executable")
	print("    lib/gridlabd    this directory must contain any library files, including gridlabd.conf")
	print("")
	print("Returns 0 if all non-optional files behave as expected, otherwise returns the number of non-optional files not behaving as expected.")
	return 0

#	run_tests is the main function for the autotest validation script.
#	@param	argv	The command line arguements.
def run_tests(argv):
	clean = 0
	recompile = 0
	verbose = 0
	debug = 0
	threads = "1"
	suppress = 0
	plat_spec = 0
	there_dir = os.getcwd()
	err_ct = 0
	ex_ct = 0
	opt_err_ct = 0
	opt_ex_ct = 0
	test_count = 0
	first_time = time.time()
	end_time = 0
	installed_dir = '/'

	# determine where the script starts
	here_dir = os.getcwd()
#	print("Starting from \'"+here_dir+"\'")
	
	# Process command line arguments
	try:
		# process arguments Single character options with arguments are followed by :
		# Multi-character options with arguments are followed by =
		opts, args = getopt.getopt(argv[1:], "i:hcrvdsT:",["idir=",'help','clean','recompile-dll','verbose','debug','suppress-opt','32','threads='])
				
		for o,a in opts:
			if o in ("-h", "--help"):
				do_help();
				sys.exit(0)
			elif o in ("-c", "--clean"):
				clean = 1
			elif o in ("-i", "--idir"):
				installed_dir = a
			elif o in ("-r", "--recompile-dll"):
				recompile = 1
			elif o in ("-v", "--verbose"):
				verbose = 1
			elif o in ("-d", "--debug"):
				debug = 1
			elif o in ("-s", "--suppress-opt"):
				suppress = 1
			elif o == "--32":
				plat_spec = 32
			elif o in ("-T", "--threads"):
				threads = a
		
		for arg in args:
			there_dir = arg
			break    
	except getopt.GetoptError as err: # to run with Python 3.x, replace the following line with this line 	
#	except getopt.GetoptError, err:
		print(err.msg)
		do_help()
		return 2

	

	
	if sys.platform == 'win32':
		import platform
		cwd = sys.path[0]
		# Additional checks required because Python reports both 64 and 32 bit windows as 'win32'.
		if platform.architecture()[0] == '64bit' and plat_spec == 0:
			if debug == 1:
				os.environ["PATH"]+=os.pathsep+cwd + "\\VS2005\\x64\\debug"
				os.environ["GLPATH"]=cwd+"\\VS2005\\x64\\debug"
			else:
				os.environ["PATH"]+=os.pathsep+cwd + "\\VS2005\\x64\\Release"
				os.environ["GLPATH"]=cwd+"\\VS2005\\x64\\Release"
		elif (platform.architecture()[0] == '32bit' and plat_spec == 0) or plat_spec == 32:
			if debug == 1:
				os.environ["PATH"]="c:\\MinGW\\bin"+os.pathsep+os.environ["PATH"]+os.pathsep+cwd + "\\VS2005\\Win32\\debug"
				os.environ["GLPATH"]=cwd+"\\VS2005\\Win32\\debug;"+os.pathsep+cwd +"\\core"
			else:
				os.environ["PATH"]="c:\\MinGW\\bin"+os.pathsep+os.environ["PATH"]+os.pathsep+cwd + "\\VS2005\\Win32\\Release"
				os.environ["GLPATH"]=cwd+"\\VS2005\\Win32\\Release;"+os.pathsep+cwd +"\\core"
	elif(sys.platform.startswith('linux') or sys.platform.startswith('darwin')):
		os.environ["PATH"]+= os.pathsep+ installed_dir +"/bin"
		os.environ["GLPATH"]=installed_dir + "/lib/gridlabd"
	else:
		print("Validate not supported on this platform "+sys.platform)
		sys.exit(1)
		
	print("PATH="+os.environ["PATH"])

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
				test_count += 1
				
	if debug==1:
		print("NOTICE:  Found "+str(test_count)+" test files")
	
#	for path, file in autotestfiles:
#		print("Found file: \'"+file+"\'")
	
	#build test dirs
	#for file in autotestfiles:
	errlist=[]
	exlist=[]
	testerrlist = []
	opterrlist=[]
	optexlist=[]
	for path, file in autotestfiles:
		err = False
		opt_err = False
		slice = file[:-4]
		currpath = os.getcwd()
		xpath = os.path.join(path,slice) # path/slice
		xfile = os.path.join(xpath,file) # path/slice/file.glm
		if clean == 1:
			if os.path.exists(xpath):
				print("REMOVING: " + xpath)
				shutil.rmtree(xpath,1)
		else:
			if not os.path.exists(xpath):
				os.mkdir(xpath)
				if os.path.exists(os.path.join(xpath,"gridlabd.xml")):
					os.remove(os.path.join(xpath,"gridlabd.xml"))
			elif recompile == 1:
				for oldfile in os.listdir(xpath):
					if oldfile.endswith(".dll") or oldfile.endswith(".so"):
						print("REMOVING DLL: "+os.path.join(xpath,oldfile))
						os.remove(os.path.join(xpath,oldfile))
			shutil.copy2(os.path.join(path,file), xfile) # path/file to path/slice/file
			
			os.chdir(xpath)
			#print("cwd: "+xpath)
			# build conf files
			# moo?
			
			#run files with popen
			#run file with:
			outfile = open(os.path.join(xpath,"outfile.txt"), "w")
			errfile = open(os.path.join(xpath,"errfile.txt"), "w")
			if verbose==1:
				print("NOTICE:  Running \'"+xfile+"\'")
			start_time = time.time();
			if debug==1 :
				rv = subprocess.call(["gridlabd","--debug","-T",threads,xfile],stdout=outfile,stderr=errfile)
			else :
				rv = subprocess.call(["gridlabd","-T",threads,xfile],stdout=outfile,stderr=errfile)
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
						opt_err_ct += 1
						opterrlist.append((path,file))
					else:
						print("ERROR: "+file+" converged when it shouldn't've!"+" ("+str(round(dt,2))+"s)")
						err_ct += 1
						errlist.append((path,file))
				elif rv == 2:
					if verbose == 1:
						print("SUCCESS: "+file+" failed to converge, as planned."+" ("+str(round(dt,2))+"s)")
				elif rv == 1:
					if "opt_" in file or "_opt" in file:
						print("WARNING: Optional file "+file+" failed to load!"+" ("+str(dt)+"s)")
						opt_ex_ct += 1
						optexlist.append((path,file))
					else:
						print("EXCEPTION: "+file+" failed to load!"+" ("+str(dt)+"s)")
						ex_ct += 1
						exlist.append((path,file))
				else:
					if "opt_" in file or "_opt" in file:
						print("WARNING: Optional file "+file+" ended with unrecognized return value! ("+str(rv)+")"+" ("+str(round(dt,2))+"s)")
						opt_ex_ct += 1
						optexlist.append((path,file))
					else:
						print("EXCEPTION: "+file+" ended with unrecognized return value! ("+str(rv)+")"+" ("+str(round(dt,2))+"s)")
						ex_ct += 1
						exlist.append((path,file))
			elif "exc_" in file or "_exc" in file:
				if rv == 0:
					if "opt_" in file or "_opt" in file:
						print("WARNING: Optional file "+file+" loaded when it shouldn't've!"+" ("+str(round(dt,2))+"s)")
						opt_err_ct += 1
						opterrlist.append((path,file))
					else:
						print("ERROR: "+file+" loaded when it shouldn't've!"+" ("+str(round(dt,2))+"s)")
						err_ct += 1
						errlist.append((path,file))
				elif rv == 1:
					if verbose == 1:
						print("SUCCESS:  "+file+" failed to load, as planned"+" ("+str(round(dt,2))+"s)")
				else:
					if "opt_" in file or "_opt" in file:
						print("WARNING: Optional file "+file+" ended with unrecognized return value! ("+str(rv)+")"+" ("+str(round(dt,2))+"s)")
						opt_ex_ct += 1
						optexlist.append((path,file))
					else:
						print("EXCEPTION: "+file+" ended with unrecognized return value! ("+str(rv)+")"+" ("+str(round(dt,2))+"s)")
						ex_ct += 1
						exlist.append((path,file))
			else:
				if rv == 2:
					if "opt_" in file or "_opt" in file:
						print("WARNING: Optional file "+file+" failed to converge!"+" ("+str(round(dt,2))+"s)")
						opt_err_ct += 1
						opterrlist.append((path,file))
					else:
						print("ERROR: "+file+" failed to converge!"+" ("+str(round(dt,2))+"s)")
						err_ct += 1
						errlist.append((path,file))
				elif rv == 1:
					if "opt_" in file or "_opt" in file:
						print("WARNING: Optional file "+file+" failed to load!"+" ("+str(round(dt,2))+"s)")
						opt_ex_ct += 1
						optexlist.append((path,file))
					else:
						print("EXCEPTION: "+file+" failed to load!"+" ("+str(round(dt,2))+"s)")
						ex_ct += 1
						exlist.append((path,file))
				elif rv == 0:
					if verbose == 1:
						print("SUCCESS: "+file+" converged"+" ("+str(round(dt,2))+"s)")
				else:
					if "opt_" in file or "_opt" in file:
						print("WARNING: Optional file "+file+" ended with unrecognized return value! ("+str(rv)+")"+" ("+str(round(dt,2))+"s)")
						opt_ex_ct += 1
						optexlist.append((path,file))
					else:
						print("EXCEPTION: "+file+" ended with unrecognized return value! ("+str(rv)+")"+" ("+str(round(dt,2))+"s)")
						ex_ct += 1
						exlist.append((path,file))
				
			os.chdir(currpath)
			#print("cwd: "+currpath)
			# end autotestfiles loop

			
	if clean == 1:
		sys.exit(0)
	
	last_time = time.time()
	dt = last_time - first_time
	#return success/failure
	if suppress == 0:
		print("Validation detected "+str(opt_err_ct)+" optional models with errors and "+str(opt_ex_ct)+" optional models with exceptions.")
		print("  Errors: ")
		for opterrpath, opterrfile in opterrlist:
			print("    * "+os.path.join(opterrpath, opterrfile))
		if not opterrlist:
			print("    No errors")
		print("  Exceptions:")
		for optexpath, optexfile in optexlist:
			print("    * "+os.path.join(optexpath, optexfile))
		if not optexlist:
			print("    No exceptions")
			
	print("Validation detected "+str(err_ct)+" models with errors and "+str(ex_ct)+" models with exceptions in "+str(round(dt,2))+" seconds.")
	print("  Errors: ")
	for errpath, errfile in errlist:
		print("    * "+os.path.join(errpath, errfile))
	if not errlist:
		print("    No errors")
	print("  Exceptions:")
	for expath, exfile in exlist:
		print("    * "+os.path.join(expath, exfile))
	if not exlist:
		print("    No exceptions")
	sys.exit(err_ct+ex_ct)
#end run_tests()

if __name__ == '__main__':
	run_tests(sys.argv)
#end main

#end validate_new.py
