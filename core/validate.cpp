// $Id$
// Copyright (C) 2012 Battelle Memorial Institute
//

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

#include "globals.h"
#include "output.h"
#include "validate.h"
#include "exec.h"
#include "lock.h"
#include "threadpool.h"

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
		output_debug("processing %s", name);
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
		output_message("Validation report:");
		output_message("%d models tested",n_files);
		output_verbose("%d unexpected successes",n_success); 
		output_verbose("%d unexpected failures",n_failed); 
		output_verbose("%d unexpected exceptions",n_exceptions); 
		output_message("%d tests succeeded",n_ok);
		output_message("%.0f%% success rate", 100.0*n_ok/n_files);
	};
	bool get_nerrors(void) { return n_success+n_failed+n_exceptions; };
};

static char validate_cmdargs[1024];

static int vsystem(const char *fmt, ...)
{
	char command[1024];
	va_list ptr;
	va_start(ptr,fmt);
	vsprintf(command,fmt,ptr);
	va_end(ptr);
	output_debug("calling system('%s')",command);
	int rc = system(command);
	output_debug("system('%s') returns %d", command, rc);
	return rc;
}

static bool destroy_dir(char *name)
{
	DIR *dirp = opendir(name);
	if ( dirp==NULL ) return true; // directory does not exist
	struct dirent *dp;
	output_debug("destroying contents of '%s'", name);
	while ( dirp!=NULL && (dp=readdir(dirp))!=NULL )
	{
		if ( strcmp(dp->d_name,".")!=0 && strcmp(dp->d_name,"..")!=0 )
		{
			char file[1024];
			sprintf(file,"%s/%s",name,dp->d_name);
			if ( unlink(file)!=0 )
			{
				output_error("destroy_dir(char *name='%s'): unlink('%s') returned '%s'", name, dp->d_name,strerror(errno));
				closedir(dirp);
				return false;
			}
			else
				output_debug("deleted %s", dp->d_name);
		}
	}
	closedir(dirp);
	return true;
}
static bool copyfile(char *from, char *to)
{
	output_debug("copying '%s' to '%s'", from, to);
	FILE *in = fopen(from,"r");
	if ( in==NULL )
	{
		output_error("copyfile(char *from='%s', char *to='%s'): unable to open '%s' for reading - %s", from,to,from,strerror(errno));
		return false; 
	}
	FILE *out = fopen(to,"w");
	if ( out==NULL )
	{
		output_error("copyfile(char *from='%s', char *to='%s'): unable to open '%s' for writing - %s", from,to,to,strerror(errno));
		fclose(in);
		return false; 
	}
	char buffer[65536];
	size_t len;
	while ( (len=fread(buffer,1,sizeof(buffer),in))>0 )
	{
		if ( fwrite(buffer,1,len,out)!=len )
		{
			output_error("copyfile(char *from='%s', char *to='%s'): unable to write to '%s' - %s", from,to,to,strerror(errno));
			fclose(in);
			fclose(out);
			return false;
		}
	}
	fclose(in);
	fclose(out);
	return true;
}

static counters run_test(char *file)
{
	output_debug("run_test(char *file='%s') starting", file);
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
		output_error("run_test(char *file='%s'): unable to destroy test folder", dir);
		return result;
	}
	if ( !mkdir(dir,0750) )
	{
		output_error("run_test(char *file='%s'): unable to create test folder", dir);
		return result;
	}
	else
		output_debug("created test folder '%s'", dir);
	char out[1024];
	sprintf(out,"%s/%s.glm",dir,name);
	if ( !copyfile(file,out) )
	{
		output_error("run_test(char *file='%s'): unable to copy to test folder %s", file, dir);
		return result;
	}
	output_debug("changing to directory '%s'", dir);
	chdir(dir);
	int64 dt = exec_clock();
	result.inc_files(file);
	if ( global_verbose_mode==0 && global_debug_mode==0 ) {	putchar('.'); fflush(stdout); }
	int code = vsystem("gridlabd %s %s.glm ", validate_cmdargs, name);
	dt = exec_clock() - dt;
	output_verbose("%s completed in %.1f seconds", out, (double)dt/(double)CLOCKS_PER_SEC);
	output_debug("returning to directory '%s'", cwd);
	chdir(cwd);
	bool exited = WIFEXITED(code);
	if ( exited )
	{
		code = WEXITSTATUS(code);
		output_debug("exit code %d received from %s", code, name);
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
	}
	else // signaled
	{
		code = WTERMSIG(code);
		output_debug("signal %d received from %s", code, name);
		if ( is_opt ) {} // no expected outcome
		else if ( is_exc ) {} // expected exception
		else result.inc_exceptions(file);
	} 
	output_debug("run_test(char *file='%s') done", file);
	return result;
}

/* simple stack to handle directories that need to be processed */
typedef struct s_dirstack {
	char name[1024];
	struct s_dirstack *next;
} DIRLIST;
static DIRLIST *dirstack = NULL;
static unsigned int dirlock = 0;
static void pushdir(char *dir)
{
	output_debug("adding %s to process stack", dir);
	DIRLIST *item = (DIRLIST*)malloc(sizeof(DIRLIST));
	strncpy(item->name,dir,sizeof(item->name)-1);
	wlock(&dirlock);
	item->next = dirstack;
	dirstack = item;
	wunlock(&dirlock);
}
/* popped item must be freed after no longer needed */
static DIRLIST *popdir(void)
{
	rlock(&dirlock);
	DIRLIST *item = dirstack;
	if ( dirstack ) dirstack = dirstack->next;
	runlock(&dirlock);
	output_debug("pulling %s from process stack", item->name);
	return item;
}

/* global counter */
static counters final;
static unsigned int countlock = 0;
void *(run_test_proc)(void *arg)
{
	size_t id = (size_t)arg;
	output_debug("starting run_test_proc id %d", id);
	DIRLIST *item;
	while ( (item=popdir())!=NULL )
	{
		counters result = run_test(item->name);
		wlock(&countlock);
		final += result;
		wunlock(&countlock);
	}
	return NULL;
}

static void process_dir(const char *path, bool runglms=false)
{
	output_debug("processing directory '%s' with run of GLMs %s", path, runglms?"enabled":"disabled");
	counters result;
	struct dirent *dp;
	DIR *dirp = opendir(path);
	if ( dirp==NULL ) return; // nothing to do
	while ( (dp=readdir(dirp))!=NULL )
	{
		char item[1024];
		size_t len = sprintf(item,"%s/%s",path,dp->d_name);
		char *ext = strrchr(item,'.');
		if ( ext==NULL ) continue;
		if ( strcmp(dp->d_name,".")==0 || strcmp(dp->d_name,"..")==0 ) continue;
		if ( strcmp(dp->d_name,"autotest")==0 )
			process_dir(item,true);
		else if ( dp->d_type==DT_DIR )
			process_dir(item);
		else if ( runglms==true && strcmp(ext,".glm")==0 )
			pushdir(item);
	}
	closedir(dirp);
	return;
}

int validate(int argc, char *argv[])
{
	size_t i;
	int redirect_found = 0;
	strcpy(validate_cmdargs,"");
	for ( i=1 ; i<argc ; i++ )
	{
		if ( strcmp(argv[i],"--redirect")==0 ) redirect_found = 1;
		strcat(validate_cmdargs,argv[i]);
		strcat(validate_cmdargs," ");
	}
	if ( !redirect_found )
		strcat(validate_cmdargs," --redirect all");
	global_suppress_repeat_messages = 0;
	output_debug("starting scan for autotest folders");
	process_dir(".");
	
	int n_procs = global_threadcount;
	if ( n_procs==0 ) n_procs = 1; //processor_count();
	pthread_t *pid = new pthread_t[n_procs];
	output_debug("starting validation with cmdargs '%s' using %d threads", validate_cmdargs, n_procs);
	for ( i=0 ; i<n_procs ; i++ )
		pthread_create(&pid[i],NULL,run_test_proc,(void*)i);
	void *rc;
	for ( i=0 ; i<n_procs ; i++ )
		pthread_join(pid[i],&rc);
	final.print();
	output_message("Total validation elapsed time is %.1f", (double)exec_clock()/(double)CLOCKS_PER_SEC);
	if ( final.get_nerrors()==0 )
		exec_setexitcode(XC_SUCCESS);
	else
		exec_setexitcode(XC_TSTERR);
	return argc;
}
