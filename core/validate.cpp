// $Id$
// Copyright (C) 2012 Battelle Memorial Institute
//

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <copyfile.h>
#include <errno.h>
#include <sys/stat.h>

#include "globals.h"
#include "output.h"
#include "validate.h"
#include "exec.h"

class counters {
private:
	unsigned int n_files; // number of tests completed
	unsigned int n_success; // unexpected successes
	unsigned int n_failed; // unexpected failures
	unsigned int n_exceptions; // unexpected exceptions
public:
	counters(void) { n_files=n_success=n_failed=n_exceptions=0; };
	counters operator+(counters a) { n_files+=a.get_nfiles(); n_success+=a.get_nsuccess(); n_failed+=a.get_nfailed(); n_exceptions+=get_nexceptions(); return *this;};
	counters operator+=(counters a) { *this = *this+a; return *this; };
	unsigned int get_nfiles(void) { return n_files; };
	unsigned int get_nsuccess(void) { return n_success; };
	unsigned int get_nfailed(void) { return n_failed; };
	unsigned int get_nexceptions(void) { return n_exceptions; };
	void inc_files(char *name) 
	{
		output_verbose("processing %s", name);
		n_files++;
	};
	void inc_success(char *name) 
	{
		output_error("%s success unexpected",name);
		n_success++; 
	};
	void inc_failed(char *name) 
	{
		output_error("%s failure unexpected",name); 
		n_failed++; 
	};
	void inc_exceptions(char *name) 
	{
		output_error("%s exception unexpected",name); 
		n_exceptions; 
	};
	void print(void) 
	{
		unsigned int n_ok = n_files-n_success-n_failed-n_exceptions;
		output_message("Validation report");
		output_message("%d files performed",n_files);
		output_message("%d unexpected successes",n_success); 
		output_message("%d unexpected failures",n_failed); 
		output_message("%d unexpected exceptions",n_exceptions); 
		output_message("%d tests succeeded",n_ok);
		output_message("%.0f%% success rate", 100.0*n_ok/n_files);
	};
};

int vsystem(const char *fmt, ...)
{
	char command[1024];
	va_list ptr;
	va_start(ptr,fmt);
	vsprintf(command,fmt,ptr);
	va_end(ptr);
	return system(command);
}

bool destroy_dir(char *name)
{
	DIR *dirp = opendir(name);
	if ( dirp==NULL ) return true; // directory does not exist
	struct dirent *dp;
	while ( dirp!=NULL && (dp=readdir(dirp))!=NULL )
	{
		if ( strcmp(dp->d_name,".")!=0 && strcmp(dp->d_name,"..")!=0 ) // && unlink(dp->d_name)!=0 )
output_verbose("removing %s", dp->d_name); if ( false )
		{
			output_error("destroy_dir(char *name='%s'): %s", name, strerror(errno));
			return false;
		}
	}
	return true;
}
bool copy_file(char *file, char *dir)
{
	copyfile_state_t s = copyfile_state_alloc();
	copyfile(file,dir,s,COPYFILE_DATA);
	copyfile_state_free(s);
	return true;
}

static counters run_test(char *file)
{
	counters result;
	bool is_err = strstr(file,"_err")==0;
	bool is_opt = strstr(file,"_opt")==0;
	bool is_exc = strstr(file,"_exc")==0;
	char dir[1024];
	strcpy(dir,file);
	char *ext = strrchr(dir,'.');
	char *name = strrchr(dir,'/')+1;
	if ( ext==NULL || strcmp(ext,".glm")!=0 ) 
	{
		output_error("run_test(char *file='%s'): file is not a GLM", file);
		return result;
	}
	*ext = '\0'; // remove extension from dir
	char cwd[1024];
	getcwd(cwd,sizeof(cwd));	
	if ( !destroy_dir(dir) )
	{
		output_error("run_test(char *file='%s'): unable to destroy test folder", file);
		return result;
	}
	mkdir(dir,0750);
	if ( !copy_file(file,dir) )
	{
		output_error("run_test(char *file='%s'): unable to copy to test folder %s", file, dir);
		return result;
	}
	chdir(dir);
	clock_t t0 = clock();
	int code = vsystem("gridlabd %s.glm 1>&outfile.txt 2>&1", name);
	clock_t dt = clock() - t0;
	output_message("%s.glm returns code %d in %.1f seconds", name, code, (double)dt/CLOCKS_PER_SEC);
	output_verbose("return code is %d", code);
	chdir(cwd);
	result.inc_files(file);
	if ( code==XC_SIGINT ) // ctrl-c caught
		return result;
	else if ( is_opt ) {} // no expected outcome
	else if ( is_exc && code==XC_EXCEPTION ) {} // expected exception
	else if ( is_err && code!=XC_SUCCESS ) {} // expected error
	else if ( code==XC_SUCCESS ) {} // expected success
	else if ( code==XC_EXCEPTION ) 
		result.inc_exceptions(file);
	else if ( code==XC_SUCCESS  )
		result.inc_success(file);
	else if ( code!=XC_SUCCESS ) 
		result.inc_failed(file); 
	return result;
}

static counters process_dir(const char *path, bool runglms=false)
{
	counters result;
	struct dirent *dp;
	DIR *dirp = opendir(path);
	while ( (dp=readdir(dirp))!=NULL )
	{
		char item[1024];
		size_t len = sprintf(item,"%s/%s",path,dp->d_name);
		char *ext = strrchr(item,'.');
		if ( ext==NULL ) continue;
		if ( strcmp(dp->d_name,".")==0 || strcmp(dp->d_name,"..")==0 ) continue;
		if ( strcmp(dp->d_name,"autotest")==0 )
			result += process_dir(item,true);
		else if ( dp->d_type==DT_DIR )
			result += process_dir(item);
		else if ( runglms==true && strcmp(ext,".glm")==0 )
			result += run_test(item);
	}
	closedir(dirp);
	return result;
}

int validate(int argc, char *argv[])
{
	global_suppress_repeat_messages = 0;
	clock_t t0 = clock();
	counters result = process_dir(".");
	result.print();
	clock_t dt = clock() - t0;
	output_message("total validation elapsed time is %.1f seconds", (double)dt/CLOCKS_PER_SEC);
	return 0;
}
