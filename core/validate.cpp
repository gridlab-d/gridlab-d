// $Id$
// Copyright (C) 2012 Battelle Memorial Institute
//

#ifdef WIN32
#include <windows.h>
#include <direct.h>
#else
#include <unistd.h>
#include <dirent.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <signal.h>

#include "globals.h"
#include "output.h"
#include "validate.h"
#include "exec.h"
#include "lock.h"
#include "threadpool.h"

/** validating result counter */
class counters {
private:
	unsigned int _lock;
	unsigned int n_files; // number of tests completed
	unsigned int n_success; // unexpected successes
	unsigned int n_failed; // unexpected failures
	unsigned int n_exceptions; // unexpected exceptions
	unsigned int n_access; // folder access failure
private:
	void wlock(void) { ::wlock(&_lock); };
	void wunlock(void) { ::wunlock(&_lock); };
	void rlock(void) { ::rlock(&_lock); };
	void runlock(void) { ::runlock(&_lock); };
public:
	counters(void) { n_files=n_success=n_failed=n_exceptions=n_access=0; };
	counters operator+(counters a) { wlock(); n_files+=a.get_nfiles(); n_success+=a.get_nsuccess(); n_failed+=a.get_nfailed(); n_exceptions+=a.get_nexceptions(); wunlock(); return *this;};
	counters operator+=(counters a) { *this = *this+a; return *this; };
	unsigned int get_nfiles(void) { return n_files; };
	unsigned int get_nsuccess(void) { return n_success; };
	unsigned int get_nfailed(void) { return n_failed; };
	unsigned int get_nexceptions(void) { return n_exceptions; };
	unsigned int get_naccess(void) { return n_access; };
	void inc_files(const char *name) 
	{
		if ( global_debug_mode || global_verbose_mode )
			output_debug("processing %s", name); 
		else
		{
			static size_t len = 0;
			char blank[1024];
			memset(blank,32,len);
			blank[len]='\0';
			len = output_raw("%s\rProcessing %s...\r",blank,name)-len; 
		}
		wlock(); 
		n_files++;
		wunlock(); 
	};
	void inc_access(const char *name) { output_debug("%s folder access failure", name); wlock(); n_access++; wunlock(); };
	void inc_success(const char *name, int code, double t) { output_error("%s success unexpected, code %d in %.1f seconds",name, code, t); wlock(); n_success++; wunlock(); };
	void inc_failed(const char *name, int code, double t) { output_error("%s failure unexpected, code %d in %.1f seconds",name, code, t); wlock(); n_failed++; wunlock(); };
	void inc_exceptions(const char *name, int code, double t) { output_error("%s exception unexpected, code %d in %.1f seconds",name, code, t); wlock(); n_exceptions++; wunlock(); };
	void print(void) 
	{
		rlock();
		unsigned int n_ok = n_files-n_success-n_failed-n_exceptions;
		output_message("Validation report:");
		if ( n_access ) output_message("%d directory access failures", n_access);
		output_message("%d models tested",n_files);
		if ( n_success ) output_message("%d unexpected successes",n_success); 
		if ( n_failed ) output_message("%d unexpected failures",n_failed); 
		if ( n_exceptions ) output_message("%d unexpected exceptions",n_exceptions); 
		output_message("%d tests succeeded",n_ok);
		output_message("%.0f%% success rate", 100.0*n_ok/n_files);
		runlock();
	};
	unsigned int get_nerrors(void) { return n_success+n_failed+n_exceptions+n_access; };
};

static counters final; // global counter
static bool clean = false; // set to true to force purge of test directories

/* Windows implementation of opendir/readdir/closedir */
#ifdef WIN32
struct dirent {
	unsigned char  d_type;	/* file type, see below */
	char *d_name;			/* name must be no longer than this */
	struct dirent *next;	/* next entry */
};
typedef struct {
	struct dirent *first;
	struct dirent *next;
} DIR;
#define DT_DIR 0x01
const char *GetLastErrorMsg(void)
{
	static unsigned int lock = 0;
	wlock(&lock);
    static TCHAR szBuf[256]; 
    LPVOID lpMsgBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

	char *p;
	while ( (p=strchr((char*)lpMsgBuf,'\n'))!=NULL ) *p=' ';
	while ( (p=strchr((char*)lpMsgBuf,'\r'))!=NULL ) *p=' ';
    sprintf(szBuf, "%s (error code %d)", lpMsgBuf, dw); 
 
    LocalFree(lpMsgBuf);
	wunlock(&lock);
	return szBuf;
}
DIR *opendir(const char *dirname)
{
	WIN32_FIND_DATA fd;
	char search[MAX_PATH];
	sprintf(search,"%s/*",dirname);
	HANDLE dh = FindFirstFile(search,&fd);
	if ( dh==INVALID_HANDLE_VALUE )
	{
		output_error("opendir(const char *dirname='%s'): %s", dirname, GetLastErrorMsg());
		final.inc_access(dirname);
		return NULL;
	}
	DIR *dirp = new DIR;
	dirp->first = dirp->next = new struct dirent;
	dirp->first->d_type = (fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) ? DT_DIR : 0;
	dirp->first->d_name = new char[strlen(fd.cFileName)+1];
	strcpy(dirp->first->d_name,fd.cFileName);
	dirp->first->next = NULL;
	struct dirent *last = dirp->first;
	while ( FindNextFile(dh,&fd)!=0 )
	{
		struct dirent *dp = (struct dirent*)malloc(sizeof(struct dirent));
		if ( dp==NULL )
		{
			output_error("opendir(const char *dirname='%s'): %s", dirname, GetLastErrorMsg());
			final.inc_access(dirname);
			return NULL;
		}
		dp->d_type = (fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) ? DT_DIR : 0;
		dp->d_name = (char*)malloc(strlen(fd.cFileName)+10);
		if ( dp->d_name==NULL )
		{
			output_error("opendir(const char *dirname='%s'): %s", dirname, GetLastErrorMsg());
			final.inc_access(dirname);
			return NULL;
		}
		strcpy(dp->d_name,fd.cFileName);
		dp->next = NULL;
		last->next = dp;
		last = dp;
	}
	if ( GetLastError()!=ERROR_NO_MORE_FILES )
	{
		output_error("opendir(const char *dirname='%s'): %s", dirname, GetLastErrorMsg());
		final.inc_access(dirname);
	}
	FindClose(dh);
	return dirp;
}
struct dirent *readdir(DIR *dirp)
{
	struct dirent *dp = dirp->next;
	if ( dp ) dirp->next = dp->next;
	return dp;
}
int closedir(DIR *dirp)
{
	struct dirent *dp = dirp->first; 
	while ( dp!=NULL )
	{
		struct dirent *del = dp;
		dp = dp->next;
		free(del->d_name);
		free(del);
	}
	return 0;
}
#define WIFEXITED(X) (X>=0&&X<128)
#define WEXITSTATUS(X) (X&127)
#define WTERMSIG(X) (X&127)
#endif

/** command line arguments that are passed to test runs */
static char validate_cmdargs[1024];

/** variable arg system call */
static int vsystem(const char *fmt, ...)
{
	char command[1024];
	va_list ptr;
	va_start(ptr,fmt);
	vsprintf(command,fmt,ptr);
	va_end(ptr);
	output_debug("calling system('%s')",command);
	int rc = system(command);
	output_debug("system('%s') returns code %x", command, rc);
	return rc;
}

/** routine to destroy the contents of a directory */
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

/** copyfile routine */
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

/** routine to run a validation test */
static counters run_test(char *file)
{
	output_debug("run_test(char *file='%s') starting", file);
	counters result;
	bool is_err = strstr(file,"_err")!=NULL;
	bool is_opt = strstr(file,"_opt")!=NULL;
	bool is_exc = strstr(file,"_exc")!=NULL;
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
	if ( clean && !destroy_dir(dir) )
	{
		output_error("run_test(char *file='%s'): unable to destroy test folder", dir);
		result.inc_access(file);
		return result;
	}
#ifdef WIN32
	if ( !mkdir(dir) && clean )
#else
	if ( !mkdir(dir,0750) && !clean )
#endif
	{
		output_error("run_test(char *file='%s'): unable to create test folder", dir);
		result.inc_access(file);
		return result;
	}
	else
		output_debug("created test folder '%s'", dir);
	char out[1024];
	sprintf(out,"%s/%s.glm",dir,name);
	if ( !copyfile(file,out) )
	{
		output_error("run_test(char *file='%s'): unable to copy to test folder %s", file, dir);
		result.inc_access(file);
		return result;
	}
	//output_debug("changing to directory '%s'", dir);
	//chdir(dir);
	int64 dt = exec_clock();
	result.inc_files(file);
	unsigned int code = vsystem("%s -W %s %s %s.glm ", 
#ifdef WIN32
		_pgmptr,
#else
		"gridlabd",
#endif
		dir,validate_cmdargs, name);
	dt = exec_clock() - dt;
	double t = (double)dt/(double)CLOCKS_PER_SEC;
	//output_debug("returning to directory '%s'", cwd);
	//chdir(cwd);
	bool exited = WIFEXITED(code);
	if ( exited )
	{
		code = WEXITSTATUS(code);
		output_debug("exit code %d received from %s", code, name);
		if ( code==XC_SIGINT ) // ctrl-c caught
			return result;
		else if ( is_opt ) // no expected outcome
		{
			if ( code==XC_SUCCESS ) 
				output_verbose("optional test %s succeeded, code %d in %.1f seconds", name, code, t);
			else if ( code==XC_EXCEPTION )
				output_warning("optional test %s exception, code %d in %.1f seconds", name, code, t);
			else 
				output_warning("optional test %s failure, code %d in %.1f seconds", name, code, t);
		}
		else if ( is_exc && code==XC_EXCEPTION ) // expected exception
			output_verbose("%s exception was expected, code %d in %.1f seconds", name, code, t);
		else if ( is_err && code!=XC_SUCCESS ) // expected error
			output_verbose("%s error was expected, code %d in %.1f seconds", name, code, t);
		else if ( code==XC_SUCCESS ) // expected success
			output_verbose("%s success was expected, code %d in %.1f seconds", name, code, t);
		else if ( code==XC_EXCEPTION ) // unexpected exception
			result.inc_exceptions(file,code,t);
		else if ( code==XC_SUCCESS  ) // unexpected success
			result.inc_success(file,code,t);
		else if ( code!=XC_SUCCESS ) // unexpected failure
			result.inc_failed(file,code,t);
	}
	else // signaled
	{
		code = WTERMSIG(code);
		output_debug("signal %d received from %s", code, name);
		if ( is_opt ) // no expected outcome
			output_warning("optional test %s exception, code %d in %.1f seconds", name, code, t);
		else if ( is_exc ) // expected exception
			output_warning("%s exception expected, code %d in %.1f seconds", name, code, t);
		else 
			result.inc_exceptions(file,code,t);
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

void *(run_test_proc)(void *arg)
{
	size_t id = (size_t)arg;
	output_debug("starting run_test_proc id %d", id);
	DIRLIST *item;
	while ( (item=popdir())!=NULL )
	{
		output_debug("process %d picked up '%s'", id, item->name);
		counters result = run_test(item->name);
		final += result;
	}
	return NULL;
}

/** routine to process a directory for autotests */
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
		if ( dp->d_name[0]=='.' ) continue;
		if ( dp->d_type==DT_DIR && strcmp(dp->d_name,"autotest")==0 )
			process_dir(item,true);
		else if ( dp->d_type==DT_DIR )
			process_dir(item);
		else if ( runglms==true && strstr(item,"/test_")!=0 && strcmp(ext,".glm")==0 )
			pushdir(item);
	}
	closedir(dirp);
	return;
}

/** main validation routine */
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
	output_message("Starting validation test in directory '%s'", global_workdir);
	char var[64];
	if ( global_getvar("clean",var,sizeof(var))!=NULL && atoi(var)!=0 ) clean = true;

	process_dir(global_workdir);
	
	int n_procs = global_threadcount;
	if ( n_procs==0 ) n_procs = processor_count();
	pthread_t *pid = new pthread_t[n_procs];
	output_debug("starting validation with cmdargs '%s' using %d threads", validate_cmdargs, n_procs);
	for ( i=0 ; i<n_procs ; i++ )
		pthread_create(&pid[i],NULL,run_test_proc,(void*)i);
	void *rc;
	output_debug("begin waiting process");
	for ( i=0 ; i<n_procs ; i++ )
	{
		pthread_join(pid[i],&rc);
		output_debug("process %d done", i);
	}
	delete [] pid;
	final.print();
	output_message("Total validation elapsed time: %.1f seconds", (double)exec_clock()/(double)CLOCKS_PER_SEC);
	if ( final.get_nerrors()==0 )
		exec_setexitcode(XC_SUCCESS);
	else
		exec_setexitcode(XC_TSTERR);
	exit(final.get_nerrors()==0 ? XC_SUCCESS : XC_TSTERR);
}
