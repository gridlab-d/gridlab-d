// $Id: validate.cpp 4738 2014-07-03 00:55:39Z dchassin $
// Copyright (C) 2012 Battelle Memorial Institute
//

#ifdef WIN32
#include <windows.h>
#include <direct.h>
#include <io.h>
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
public:
	counters(void) { _lock=0; n_scanned=n_tested=n_passed=n_files=n_success=n_failed=n_exceptions=n_access=0; };
	counters operator+(counters a) 
	{ 
		wlock(); 
		n_scanned += a.get_scanned();
		n_tested += a.get_tested();
		n_passed += a.get_passed();
		n_files += a.get_nfiles(); 
		n_success += a.get_nsuccess(); 
		n_failed += a.get_nfailed(); 
		n_exceptions += a.get_nexceptions(); 
		wunlock(); 
		return *this;
	};
	counters operator+=(counters a) { *this = *this+a; return *this; };
private:
	unsigned int _lock;
	// directories
	unsigned int n_scanned; // number of directories scanned
	unsigned int n_tested; // number of autotest directories tested
	unsigned int n_passed; // number of autotest directories that passed
public:
	unsigned int get_scanned() { return n_scanned; };
	unsigned int get_tested() { return n_tested; };
	unsigned int get_passed() { return n_passed; };
	void inc_scanned() { wlock(); n_scanned++; wunlock(); };
	void inc_tested() { wlock(); n_tested++; wunlock(); };
	void inc_passed() { wlock(); n_passed++; wunlock(); };
private:
	// files
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
public:
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
	void inc_failed(const char *name, int code, double t) { output_error("%s error unexpected, code %d (%s) in %.1f seconds",name, code, exec_getexitcodestr(code), t); wlock(); n_failed++; wunlock(); };
	void inc_exceptions(const char *name, int code, double t) { output_error("%s exception unexpected, code %d (%s) in %.1f seconds",name, code, exec_getexitcodestr(code), t); wlock(); n_exceptions++; wunlock(); };
	void print(void) 
	{
		rlock();
		unsigned int n_ok = n_files-n_success-n_failed-n_exceptions;
		output_message("\nValidation report:");
		if ( n_access ) output_message("%d directory access failures", n_access);
		output_message("%d models tested",n_files);
		if ( n_files!=0 )
		{
			if ( n_success ) output_message("%d unexpected successes",n_success); 
			if ( n_failed ) output_message("%d unexpected errors",n_failed); 
			if ( n_exceptions ) output_message("%d unexpected exceptions",n_exceptions); 
			output_message("%d tests succeeded",n_ok);
				output_message("%.0f%% success rate", 100.0*n_ok/n_files);
		}
		runlock();
	};
	unsigned int get_nerrors(void) { return n_success+n_failed+n_exceptions+n_access; };
};

static counters final; // global counter
static bool clean = false; // set to true to force purge of test directories

/* report generation functions */
static FILE *report_fp = NULL;
static char report_file[1024] = "validate.txt";
static const char *report_ext = NULL;
static const char *report_col="    ";
static const char *report_eol="\n";
static const char *report_eot="\f";
static unsigned int report_cols=0;
static unsigned int report_rows=0;
static unsigned int report_lock=0;
static bool report_open(void)
{
	wlock(&report_lock);

	global_getvar("validate_report",report_file,sizeof(report_file));
	if ( report_fp==NULL )
	{
		report_ext = strrchr(report_file,'.');
		if ( strcmp(report_ext,".csv")==0 ) 
		{
			report_col = ",";
			report_eot = "\n";
		}
		else if ( strcmp(report_ext,".txt")==0) 
		{
			report_col = "\t";
			report_eot = "\n";
		}
		report_fp = fopen(report_file,"w");
	}
	wunlock(&report_lock);
	return report_fp!=NULL;
}
static int report_title(const char *fmt,...)
{
	int len = 0;
	if ( report_fp )
	{
		wlock(&report_lock);
		if ( report_cols++>0 )
			len = fprintf(report_fp,"%s",report_eol);
		va_list ptr;
		va_start(ptr,fmt);
		len = +vfprintf(report_fp,fmt,ptr);
		va_end(ptr);
		fflush(report_fp);
		wunlock(&report_lock);
	}
	return len;
}
static int report_data(const char *fmt="",...)
{
	int len = 0;
	if ( report_fp )
	{
		wlock(&report_lock);
		if ( report_cols++>0 )
			len = fprintf(report_fp,"%s",report_col);
		va_list ptr;
		va_start(ptr,fmt);
		len = +vfprintf(report_fp,fmt,ptr);
		va_end(ptr);
		wunlock(&report_lock);
	}
	return len;
}
static int report_newrow(void)
{
	int len = 0;
	if ( report_fp )
	{
		wlock(&report_lock);
		report_cols=0;
		report_rows++;
		len = fprintf(report_fp,"%s",report_eol);
		fflush(report_fp);
		wunlock(&report_lock);
	}
	return len;
}
static int report_newtable(const char *table)
{
	int len = 0;
	if ( report_fp )
	{
		wlock(&report_lock);
		report_rows++;
		if ( report_cols>0 ) len += fprintf(report_fp,"%s",report_eol);
		report_cols=0;
		len = fprintf(report_fp,"%s%s%s",report_eot,table,report_eol);
		fflush(report_fp);
		wunlock(&report_lock);
	}
	return len;
}
static int report_close(void)
{
	wlock(&report_lock);
	if ( report_fp ) fclose(report_fp);
	report_fp = NULL;
	wunlock(&report_lock);
	return report_rows;
}

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
static counters run_test(char *file, double *elapsed_time=NULL)
{
	output_debug("run_test(char *file='%s') starting", file);
	counters result;

	bool is_err = strstr(file,"_err.")!=NULL || strstr(file,"_err_")!=NULL;
	if ( is_err && (global_validateoptions&VO_TSTERR)==0 ) return result;

	bool is_exc = strstr(file,"_exc.")!=NULL || strstr(file,"_exc_")!=NULL;
	if ( is_exc && (global_validateoptions&VO_TSTEXC)==0 ) return result;

	bool is_opt = strstr(file,"_opt.")!=NULL || strstr(file,"_opt_")!=NULL;
	if ( is_opt && (global_validateoptions&VO_TSTOPT)==0 ) return result;

	if ( !is_err && !is_opt && !is_exc && (global_validateoptions&VO_TSTRUN)==0 ) return result;

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
	if ( !mkdir(dir,0750) && clean )
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
	if ( elapsed_time!=NULL ) *elapsed_time = t;
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
				output_warning("optional test %s error, code %d in %.1f seconds", name, code, t);
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
		else if ( code!=XC_SUCCESS ) // unexpected error
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
	unsigned short id;
	struct s_dirstack *next;
} DIRLIST;
static DIRLIST *dirstack = NULL;
static unsigned short next_id = 0;
static char *result_code = NULL;
static unsigned int dirlock = 0;
static void pushdir(char *dir)
{
	output_debug("adding %s to process stack", dir);
	DIRLIST *item = (DIRLIST*)malloc(sizeof(DIRLIST));
	strncpy(item->name,dir,sizeof(item->name)-1);
	wlock(&dirlock);
	item->next = dirstack;
	item->id = next_id++;
	dirstack = item;
	wunlock(&dirlock);
}
static void sortlist(void)
{
	bool done = false;
	while ( !done )
	{
		DIRLIST *item, *prev=NULL;
		done = true;
		for ( item=dirstack ; item!=NULL && item->next!=NULL ; prev=item,item=item->next )
		{
			DIRLIST *first = item;
			DIRLIST *second = item->next;
			if ( strcmp(first->name,second->name)>0 )
			{
				if ( prev!=NULL )
					prev->next = second;
				else
					dirstack = second;
				first->next = second->next;
				second->next = first;
				done = false;
			}
		}
	}
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
	bool passed = true;
	while ( (item=popdir())!=NULL )
	{
		output_debug("process %d picked up '%s'", id, item->name);
		double dt;
		counters result = run_test(item->name,&dt);
		if ( result.get_nerrors()>0 ) passed=false;
		if ( global_validateoptions&VO_RPTGLM )
		{
			char *flags[] = {"","E","S","X"};
			char code = 0;
			if ( result.get_nerrors() ) code=1;
			if ( result.get_nsuccess() ) code=2;
			if ( result.get_nexceptions() ) code=3;
			result_code[item->id] = code;
			char buffer[1024];
			sprintf(buffer,"%s%s%6.1f%s%s",flags[code],report_col,dt,report_col,item->name);
			report_data("%s",buffer);
			report_newrow();
		}
		final += result;
	}
	if ( passed ) final.inc_passed();
	return NULL;
}

/** routine to process a directory for autotests */
static size_t process_dir(const char *path, bool runglms=false)
{
	// check for block file
	char blockfile[1024];
	sprintf(blockfile,"%s/validate.no",path);
	if ( access(blockfile,00)==0 && !global_isdefined("force_validate") )
	{
		output_debug("processing directory '%s' blocked by presence of 'validate.no' file", path);
		return 0;
	}

	size_t count = 0;
	output_debug("processing directory '%s' with run of GLMs %s", path, runglms?"enabled":"disabled");
	counters result;
	final.inc_scanned();
	if ( runglms ) final.inc_tested();
	struct dirent *dp;
	DIR *dirp = opendir(path);
	if ( dirp==NULL ) return 0; // nothing to do
	while ( (dp=readdir(dirp))!=NULL )
	{
		char item[1024];
		size_t len = sprintf(item,"%s/%s",path,dp->d_name);
		char *ext = strrchr(item,'.');
		if ( dp->d_name[0]=='.' ) continue; // ignore anything that starts with a dot
		if ( dp->d_type==DT_DIR && strcmp(dp->d_name,"autotest")==0 )
		{
			count+=process_dir(item,true);
			if ( global_validateoptions&VO_RPTDIR )
			{
				report_data();
				report_data("%d",count);
				report_data("%s",item);
				report_newrow();
			}
		}
		else if ( dp->d_type==DT_DIR )
			count+=process_dir(item);
		else if ( runglms==true && strstr(item,"/test_")!=0 && strcmp(ext,".glm")==0 )
		{
			pushdir(item);
			count++;
		}
	}
	closedir(dirp);
	result_code = (char *)malloc(next_id);
	return count;
}

char *encode_result(char *data,size_t sz)
{
	size_t len = (sz+1)/2+1;
	char *code = (char*)malloc(len);
	memset(code,0,len);
	size_t i;
	for ( i=0 ; i<sz ; i++ )
	{
		size_t ndx = i/2;
		size_t shft = (i%2)*2;
		code[ndx] |= (data[i]<<shft);
	}
	for ( i=0 ; i<len ; i++ )
	{
		static char t[] = "0123456789ABCDEF";
		code[i] = t[code[i]];
	}
	code[len]='\0';
	return code;
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

	report_open();
	if ( report_fp==NULL )
		output_warning("unable to open '%s' for writing", report_file);
	report_title("VALIDATION TEST REPORT");
	report_title("GridLAB-D %d.%d.%d-%d (%s)", global_version_major, global_version_minor, global_version_patch, global_version_build, global_version_branch);
	
	report_newrow();
	report_newtable("TEST CONFIGURATION");
	
	char tbuf[64];
	time_t now = time(NULL);
	struct tm *ts = localtime(&now);
	report_data();
	report_data("Date");
	report_data("%s",strftime(tbuf,sizeof(tbuf),"%Y-%m-%d %H:%M:%S %z",ts)?tbuf:"???");
	report_newrow();
	
#ifdef WIN32
	char *user=getenv("USERNAME"); 
#else
	char *user=getenv("USER"); 
#endif
	report_data();
	report_data("User");
	report_data("%s",user?user:"(NA)");
	report_newrow();

#ifdef WIN32
	char *host=getenv("COMPUTERNAME");
#else
	char *host=getenv("HOSTNAME");
#endif
	report_data();
	report_data("Host");
	report_data("%s",host?host:"(NA)");
	report_newrow();
	
	report_data();
	report_data("Platform");
	report_data("%d-bit %s %s", sizeof(void*), global_platform,
#ifdef _DEBUG
		"DEBUG"
#else
		"RELEASE"
#endif
		);
	report_newrow();
	
	report_data();
	report_data("Workdir");
	report_data("%s",global_workdir);
	report_newrow();
	
	report_data();
	report_data("Arguments");
	report_data("%s",validate_cmdargs);
	report_newrow();
	
	report_data();
	report_data("Clean");
	report_data("%s",clean?"TRUE":"FALSE");
	
	report_newrow();
	report_data();
	report_data("Threads");
	report_data("%d",global_threadcount);
	report_newrow();
	
	char options[1024]="";
	report_data();
	report_data("Options");
	report_data("%s",global_getvar("validate",options,sizeof(options))?options:"(NA)");
	report_newrow();

	char mailto[1024]="";
	report_data();
	report_data("Mailto");
	report_data("%s",global_getvar("mailto",mailto,sizeof(mailto))!=NULL?mailto:"(NA)");
	report_newrow();
	
	if ( global_validateoptions&VO_RPTDIR ) 
		report_newtable("DIRECTORY SCAN RESULTS");
	process_dir(global_workdir);
	sortlist();
	
	if ( global_validateoptions&VO_RPTGLM )
		report_newtable("FILE TEST RESULTS");
	int n_procs = global_threadcount;
	if ( n_procs==0 ) n_procs = processor_count();
	n_procs = min(final.get_tested(),n_procs);
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
	double dt = (double)exec_clock()/(double)CLOCKS_PER_SEC;
	output_message("Total validation elapsed time: %.1f seconds", dt);
	if ( report_fp ) output_message("See '%s/%s' for details", global_workdir, report_file);
	if ( final.get_nerrors()==0 )
		exec_setexitcode(XC_SUCCESS);
	else
		exec_setexitcode(XC_TSTERR);

	report_newtable("OVERALL RESULTS");

	char *flag="!!!";
	report_data();
	report_data("Directory results");
	report_newrow();

	report_data();
	report_data();
	report_data("Scanned");
	report_data("%d",final.get_scanned());
	report_newrow();

	report_data("%s",final.get_naccess()?flag:"");
	report_data();
	report_data("Denied");
	report_data("%d",final.get_naccess());
	report_newrow();

	report_data();
	report_data();
	report_data("Tested");
	report_data("%d",final.get_tested());
	report_newrow();

	int n_failed = final.get_tested()-final.get_passed();
	report_data("%s",n_failed?flag:"");
	report_data();
	report_data("Failed");
	report_data("%d",n_failed);
	report_newrow();

	report_data();
	report_data("File results");
	report_newrow();

	report_data();
	report_data();
	report_data("Tested");
	report_data("%d",final.get_nfiles());
	report_newrow();

	report_data();
	report_data();
	report_data("Passed");
	report_data("%d", final.get_nfiles()-final.get_nerrors());
	report_newrow();

	report_data("%s",final.get_nerrors()?flag:"");
	report_data();
	report_data("Failed");
	report_data("%d",final.get_nerrors());
	report_newrow();

	report_data();
	report_data("Unexpected results");
	report_newrow();

	report_data("%s",final.get_nsuccess()?flag:"");
	report_data();
	report_data("Successes");
	report_data("%d",final.get_nsuccess());
	report_newrow();

	report_data("%s",final.get_nfailed()?flag:"");
	report_data();
	report_data("Errors");
	report_data("%d",final.get_nfailed());
	report_newrow();

	report_data("%s",final.get_nexceptions()?flag:"");
	report_data();
	report_data("Exceptions");
	report_data("%d",final.get_nexceptions());
	report_newrow();

	report_data();
	report_data("Runtime");
	report_data("%.1f s", dt);
	report_newrow();

	report_data();
	report_data("Result code");
	report_data("%s",encode_result(result_code,next_id));
	report_newrow();

	report_newrow();
	report_title("END TEST REPORT");
	report_newrow();

	fclose(report_fp);

#ifndef WIN32
#ifdef __APPLE__
#define MAILER "/usr/bin/mail"
#else
#define MAILER "/bin/mail"
#endif
	if ( strcmp(mailto,"")!=0 )
	{
		if ( vsystem(MAILER " -s 'GridLAB-D Validation Report (%d errors)' %s <%s", 
				final.get_nerrors(), mailto, report_file)==0 )
			output_verbose("Mail message send to %s",mailto);
		else
			output_error("Error sending notification to %s", mailto);
	}
#endif

	exit(final.get_nerrors()==0 ? XC_SUCCESS : XC_TSTERR);
}
