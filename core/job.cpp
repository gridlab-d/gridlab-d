// $Id: job.cpp 4738 2014-07-03 00:55:39Z dchassin $
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
static const char *GetLastErrorMsg(void)
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
static DIR *opendir(const char *dirname)
{
	WIN32_FIND_DATA fd;
	char search[MAX_PATH];
	sprintf(search,"%s/*",dirname);
	HANDLE dh = FindFirstFile(search,&fd);
	if ( dh==INVALID_HANDLE_VALUE )
	{
		output_error("opendir(const char *dirname='%s'): %s", dirname, GetLastErrorMsg());
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
			return NULL;
		}
		dp->d_type = (fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) ? DT_DIR : 0;
		dp->d_name = (char*)malloc(strlen(fd.cFileName)+10);
		if ( dp->d_name==NULL )
		{
			output_error("opendir(const char *dirname='%s'): %s", dirname, GetLastErrorMsg());
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
	}
	FindClose(dh);
	return dirp;
}
static struct dirent *readdir(DIR *dirp)
{
	struct dirent *dp = dirp->next;
	if ( dp ) dirp->next = dp->next;
	return dp;
}
static int closedir(DIR *dirp)
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
static char job_cmdargs[1024];

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
static bool run_job(char *file, double *elapsed_time=NULL)
{
	output_debug("run_job(char *file='%s') starting", file);

	char dir[1024];
	strcpy(dir,file);
	char *ext = strrchr(dir,'.');
	char *name = strrchr(dir,'/')+1;
	if ( ext==NULL || strcmp(ext,".glm")!=0 ) 
	{
		output_error("run_job(char *file='%s'): file is not a GLM", file);
		return false;
	}
	else
	{
		static size_t len = 0;
		char blank[1024];
		memset(blank,32,len);
		blank[len]='\0';
		len = output_raw("%s\rProcessing %s...\r",blank,name)-len; 
	}
	int64 dt = exec_clock();
	unsigned int code = vsystem("%s %s %s ", 
#ifdef WIN32
		_pgmptr,
#else
		"gridlabd",
#endif
		job_cmdargs, name);
	dt = exec_clock() - dt;
	double t = (double)dt/(double)CLOCKS_PER_SEC;
	if ( elapsed_time!=NULL ) *elapsed_time = t;
	if ( code!=0 )
	{
		output_error("exit code %d received from %s", code, name);
		return false;
	}
	output_debug("run_test(char *file='%s') done", file);
	return true;
}

/* simple stack to handle directories that need to be processed */
typedef struct s_jobstack {
	char name[1024];
	struct s_jobstack *next;
} JOBLIST;
static JOBLIST *jobstack = NULL;
static unsigned int joblock = 0;
static void pushjob(char *dir)
{
	output_debug("adding %s to job list", dir);
	JOBLIST *item = (JOBLIST*)malloc(sizeof(JOBLIST));
	strncpy(item->name,dir,sizeof(item->name)-1);
	wlock(&joblock);
	item->next = jobstack;
	jobstack = item;
	wunlock(&joblock);
}
/* popped item must be freed after no longer needed */
static JOBLIST *popjob(void)
{
	rlock(&joblock);
	JOBLIST *item = jobstack;
	if ( jobstack ) jobstack = jobstack->next;
	runlock(&joblock);
	output_debug("pulling %s from job list", item->name);
	return item;
}

static int final_result = true;
void *(run_job_proc)(void *arg)
{
	size_t id = (size_t)arg;
	output_debug("starting run_test_proc id %d", id);
	JOBLIST *item;
	bool passed = true;
	while ( (item=popjob())!=NULL )
	{
		output_debug("process %d picked up '%s'", id, item->name);
		double dt;
		if ( !run_job(item->name,&dt) )
			final_result = false;
	}
	return NULL;
}

/** routine to process a directory for autotests */
static size_t process_dir(const char *path)
{
	size_t count = 0;
	output_debug("processing job directory '%s'", path);
	struct dirent *dp;
	DIR *dirp = opendir(path);
	if ( dirp==NULL ) return 0; // nothing to do
	while ( (dp=readdir(dirp))!=NULL )
	{
		char item[1024];
		size_t len = sprintf(item,"%s/%s",path,dp->d_name);
		char *ext = strrchr(dp->d_name,'.');
		if ( dp->d_name[0]=='.' ) continue; // ignore anything that starts with a dot
		if ( ext && strcmp(ext,".glm")==0 )
		{
			pushjob(item);
			count++;
		}
	}
	closedir(dirp);
	return count;
}

/** main validation routine */
extern "C" int job(int argc, char *argv[])
{
	size_t i;
	int redirect_found = 0;
	strcpy(job_cmdargs,"");
	for ( i=1 ; i<argc ; i++ )
	{
		if ( strcmp(argv[i],"--redirect")==0 ) redirect_found = 1;
		strcat(job_cmdargs,argv[i]);
		strcat(job_cmdargs," ");
	}
	if ( !redirect_found )
		strcat(job_cmdargs," --redirect all");
	global_suppress_repeat_messages = 0;
	output_message("Starting job in directory '%s'", global_workdir);
	char var[64];
	if ( global_getvar("clean",var,sizeof(var))!=NULL && atoi(var)!=0 ) clean = true;


	char mailto[1024];
	global_getvar("mailto",mailto,sizeof(mailto));
	
	unsigned int count = process_dir(global_workdir);
	if ( count==0 )
	{
		output_warning("no models found to process job in workdir '%s'", global_workdir);
		exit(XC_RUNERR);
	}
	
	int n_procs = global_threadcount;
	if ( n_procs==0 ) n_procs = processor_count();
	pthread_t *pid = new pthread_t[n_procs];
	output_debug("starting job with cmdargs '%s' using %d threads", job_cmdargs, n_procs);
	for ( i=0 ; i<min(count,n_procs) ; i++ )
		pthread_create(&pid[i],NULL,run_job_proc,(void*)i);
	void *rc;
	output_debug("begin waiting process");
	for ( i=0 ; i<min(count,n_procs) ; i++ )
	{
		pthread_join(pid[i],&rc);
		output_debug("process %d done", i);
	}
	delete [] pid;

	double dt = (double)exec_clock()/(double)CLOCKS_PER_SEC;
	output_message("Total job elapsed time: %.1f seconds", dt);
	if ( final_result==0 )
		exec_setexitcode(XC_SUCCESS);
	else
		exec_setexitcode(XC_RUNERR);

#ifndef WIN32
#ifdef __APPLE__
#define MAILER "/usr/bin/mail"
#else
#define MAILER "/bin/mail"
#endif
// TODO
//	if ( global_getvar("mailto",mailto,sizeof(mailto))!=NULL 
//		&& vsystem(MAILER " -s 'GridLAB-D Job Report' %s <%s", 
//			count, mailto, report_file)==0 )
//		output_verbose("Mail message send to %s",mailto);
//	else
//		output_error("Error sending notification to %s", mailto);
#endif

	exit(final_result==0 ? XC_SUCCESS : XC_TSTERR);
}
