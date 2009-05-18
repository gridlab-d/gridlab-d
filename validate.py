import os, sys, time, shutil
import email, smtplib

os.putenv("PATH","%PATH%;..\\..\\..\\VS2005\\Win32\\Release")
	
def email_error():
	return 0
	#end email_error()

def parse_outfile(filename):
	return 0
	# end parse_outfile

# configuration directory
# glm file name
def build_conf(confdir, glmname):
	#	fopen conf file
	conf = open("gridlabd.conf", "w")
	conf.write("// $Id$\n")
	conf.write("// Copyright (C) 2009 Battelle Memorial Institute\n//\n")
	conf.write("// Auto-generated gridlabd.conf for "+glmname+" by the autotest script\n")
	conf.write("\n")
	
	
	#	set streams
	#set include="../../core"
	#include "../../core/rt/msvc_debugger.glm"
	#include "assert.glm"
	
	#	set dump file
	#	set debug=1
	#	set warning=1
	#	close conf file
	return 0
	# end build_conf


#	basedir ~ for the glmfile
#	testdir ~ for the 
def run_file(basedir, testdir, glmname):
	#	validate basedir
	#	validate testdir
	#	validate glmname
	#	point to gridlabd.conf in testdir
	#	set dump file to glmname + "_dump.glm"
	#	set stdout to glmname + "_stdout.txt"
	#	set stderr to glmname + "_stderr.txt"

	# 	mkdir(glmname)
	#	copy glmname.glm to testdir/glmname.glm
	#	cd testdir/glmname
	#	build conf file

	#	set output to glmname + "_out.glm"
	outpath = basedir+glmname+".xml"
	dumppath = basedir+glmname+"_dump.glm"
	#	rv = exec("gridlabd "+
	return os.system("gridlabd --verbose -o "+outpath+" "+filepath)
	#end run_file

def print_header(logfile, logtime):
	logfile.write("Log file opened at "+time.asctime(logtime)+"\n")
	logfile.write("Working directory is \'"+os.getcwd()+"\'\n")
	#end print_header

def gld_validate(argv):
	#	capture cwd
	autotest_cwd = os.getcwd()
	
	#	if args = null, copy cwd to active dir
	test_wd = os.getcwd()
	if len(argv) > 1:
		argn = 1
		# scan for flags, else use as working dir
		while argn < len(argv):
			if argv[argn][0] == '-':
				argn+=1
			else :
				# copy argv[argn] to autotest_cwd
				test_wd = argv[argn]
				break
	
	# stuff?
	
	#	open autotest log file
	logtime = time.localtime()
	logtimestr = time.strftime("%Y%m%d_%H%M%S", logtime)
	logfilename = "autotest_"+logtimestr+".log"
	logfile = open(logfilename, 'w')
	print_header(logfile, logtime)
	
	
	#	walk active dir and cache autotest directories
	autotest_list = []
	for path, dirs, files in os.walk(test_wd): # os.path.walk before 3.0
		if "autotest" in dirs:
			autotest_list.append(os.path.abspath(os.path.join(path, "autotest")))
	
	#	print autotest dir list to output file
	print(autotest_list)
	for path in autotest_list:
		logfile.write("Found autotest directory \'"+path+"\'\n")
	
	#	for each path in autotest list
	result = 0
	errcount = 0
	errlist = []
	for path in autotest_list: #	get file list for dir
		print("path: \'"+path+"\'")
		os.chdir(path)
		print("chdir: \'"+os.getcwd()+"\'")
		#	for each file
		print(os.listdir('.'))
		for file in os.listdir('.'):
			#	if it ends with ".glm"'
			if "test_" in file and ".glm" in file:
				#	base dir is where the GLM file resides
				base = file[:-4]
				if not os.path.exists(base):
					os.mkdir(base)
				
				shutil.copy2(file, base)
				os.chdir(base)
				
				#	rv = run_file(base dir, autotest dir, glm file name - ".glm")
				if not os.path.exists(file):
					logfile.write("ERROR:  Error copying "+file+" to "+path+".\n")
					result = -1
				else:
					#build conf file
					#	cwd usage ~ we should be in the directory the current file is in ~ MH
					#build_conf(os.getcwd(), file) # in development
					#run GLD
					logfile.write("RUN: \'gridlabd "+os.path.abspath(file)+"\'\n")
					result = os.system("gridlabd --verbose --debug \""+os.path.abspath(file)+"\"")
				
				os.chdir('..')
				print("chdir: \'"+os.getcwd()+"\'")
				
				#	if rv is not empty
				if result > 0:
					#	if strstr(file name, "_opt")
					if "opt_" in file:
						#	fprintf warning
						logfile.write("WARNING:  Optional test \'"+file+"\' failed to converge.\n")
					#	else
					elif not "err_" in file:
						#	fprintf error
						logfile.write("ERROR:  Test \'"+file+"\' failed to converge.\n")
						#	add to error list
						errlist.append(os.path.join(path, base))
						#	++error
						errcount+=1
			else:
				logfile.write("SUCCESS:  Test \'"+file+"\' passed.\n")
				
	#	if error > 0
	if errcount > 0:
		# where are the email settings fetched from?
		#	email_error(error_ct, error_list)
		logfile.write(str(errcount)+" errors were found.\n")
	else:
		logfile.write("No errors were found.\n")
	logfile.close()
	return errcount
	#end gld_validate()


if __name__ == '__main__':
	gld_validate(sys.argv)
	#end main

