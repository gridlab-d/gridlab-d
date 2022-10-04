/** $Id: main.c 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file main.c
	@author David P. Chassin

 @{
 **/
#define _MAIN_C

//#define USE_MPI

#include <cstdlib>
#include <cstring>
#include <filesystem>

#include "globals.h"

#ifdef _WIN32

#include <direct.h>
#include <process.h>

#else

#include <unistd.h>

#endif

#include "globals.h"
#include "legal.h"
#include "cmdarg.h"
#include "module.h"
#include "output.h"
#include "environment.h"
#include "test.h"
#include "gldrandom.h"
#include "realtime.h"
#include "save.h"
#include "local.h"
#include "exec.h"
#include "kml.h"
#include "kill.h"
#include "threadpool.h"
#include "cpp_threadpool.h"

#if defined WIN32 && _DEBUG
/** Implements a pause on exit capability for Windows consoles
 **/
void pause_at_exit(void) 
{
    if (global_pauseatexit)
        system("pause");
}
#endif

namespace fs = std::filesystem;

void delete_pidfile(void) {
    unlink(global_pidfile);
}

std::vector<std::string> split_path(const std::string& path, char sep){
    std::vector<std::string> tokens;
    std::size_t start = 0, end;
    while ((end = path.find(sep, start)) != std::string::npos) {
        tokens.push_back(path.substr(start, end - start));
        start = end + 1;
    }
    tokens.push_back(path.substr(start));
    return tokens;
}

fs::path
findExecutable(const std::string &name, const std::string &execName, const std::string &pathString) {
    fs::path execPath(execName);
    if (execPath.is_absolute()) {
        return execPath;
    } else if (execPath.is_relative() && execName.front() == '.') {
        return fs::absolute(execPath);
    } else {
        auto sys_path = pathString;
        size_t pos;
        std::string path_token;

        auto check_exists = [](fs::path gldpath, fs::path gldpath_exe) {
            if (fs::exists(gldpath)) {
                return gldpath;
            } else if (fs::exists(gldpath_exe)) {
                return gldpath_exe;
            }
            return fs::path();
        };

        auto splitPath = split_path(sys_path, env_delim_char);

        for(const auto& path : splitPath){
            auto gldpath = fs::path(path) / name;
            auto gldpath_exe = fs::path(path) / (name + ".exe");
            auto check_path = check_exists(gldpath, gldpath_exe);
            if (!check_path.empty()) {
                return check_path;
            }
        }
    }
    throw std::runtime_error("Unable to determine GridLAB-D executable path");
}

/** The main entry point of GridLAB-D
    @returns Exit codes XC_SUCCESS, etc. (see gridlabd.h)
 **/
int main(int argc, /**< the number entries on command-line argument list \p argv */
         char *argv[]) /**< a list of pointers to the command-line arguments */
{
    char *pd1, *pd2;
    int i, pos = 0;

    global_gl_executable = findExecutable("gridlabd", argv[0], getenv("PATH"));
    auto root_path = global_gl_executable.parent_path().parent_path();
    global_gl_share = root_path / "share";
    global_gl_include = root_path / "include";
    global_gl_lib = root_path / "lib";
    global_gl_bin = root_path / "bin";

    global_gl_path = std::string((getenv("GLPATH") != nullptr ? std::string(getenv("GLPATH")) + env_delim : "") +
                                 global_gl_lib.string() + env_delim +
                                 global_gl_share.string() + env_delim +
                                 global_gl_include.string() + env_delim +
                                 global_gl_bin.string());

    char *browser = getenv("GLBROWSER");

    /* set the default timezone */
    timestamp_set_tz(NULL);

    exec_clock(); /* initialize the wall clock */
    realtime_starttime(); /* mark start */

    /* set the process info */
    global_process_id = getpid();

    /* specify the default browser */
    if (browser != NULL)
        strncpy(global_browser, browser, sizeof(global_browser) - 1);

#if defined WIN32 && _DEBUG
    atexit(pause_at_exit);
#endif

#ifdef _WIN32
    kill_starthandler();
    atexit(kill_stophandler);
#endif

    /* capture the execdir */
    strcpy(global_execname, argv[0]);
    strcpy(global_execdir, argv[0]);
    pd1 = strrchr(global_execdir, '/');
    pd2 = strrchr(global_execdir, '\\');
    if (pd1 > pd2) *pd1 = '\0';
    else if (pd2 > pd1) *pd2 = '\0';

    /* determine current working directory */
    char *result = getcwd(global_workdir, 1024);

    /* capture the command line */
    for (i = 0; i < argc; i++) {
        if (pos < (int) (sizeof(global_command_line) - strlen(argv[i])))
            pos += sprintf(global_command_line + pos, "%s%s", pos > 0 ? " " : "", argv[i]);
    }

    /* main initialization */
    if (!output_init(argc, argv) || !exec_init())
        exit(XC_INIERR);

    /* set thread count equal to processor count if not passed on command-line */
    if (global_threadcount == 0)
        global_threadcount = processor_count();
    output_verbose("detected %d processor(s)", processor_count());
    output_verbose("using %d helper thread(s)", global_threadcount);

    /* process command line arguments */
    if (cmdarg_load(argc, argv) == FAILED) {
        output_fatal("shutdown after command line rejected");
        /*	TROUBLESHOOT
            The command line is not valid and the system did not
            complete its startup procedure.  Correct the problem
            with the command line and try again.
         */
        exit(XC_ARGERR);
    }

    /* stitch clock */
    global_clock = global_starttime;

    /* Check to see if stoptime is set - if not, set to 1-year later */
    if (global_stoptime == TS_NEVER) {
        global_stoptime = global_starttime + 31536000;
    }

    /* initialize scheduler */
    sched_init(0);

    /* recheck threadcount in case user set it 0 */
    if (global_threadcount == 0) {
        global_threadcount = processor_count();
        output_verbose("using %d helper thread(s)", global_threadcount);
    }

    /* see if newer version is available */
    if (global_check_version)
        check_version(1);

    /* setup the random number generator */
    random_init();

    /* pidfile */
    if (strcmp(global_pidfile, "") != 0) {
        FILE *fp = fopen(global_pidfile, "w");
        if (fp == NULL) {
            output_fatal("unable to create pidfile '%s'", global_pidfile);
            /*	TROUBLESHOOT
                The system must allow creation of the process id file at
                the location indicated in the message.  Create and/or
                modify access rights to the path for that file and try again.
             */
            exit(XC_PRCERR);
        }
#ifdef _WIN32
#define getpid _getpid
#endif
        fprintf(fp, "%d\n", getpid());
        output_verbose("process id %d written to %s", getpid(), global_pidfile);
        fclose(fp);
        atexit(delete_pidfile);
    }

    /* do legal stuff */
#ifdef LEGAL_NOTICE
    if (strcmp(global_pidfile,"")==0 && legal_notice()==FAILED)
        exit(XC_USRERR);
#endif

    /* start the processing environment */
    output_verbose("load time: %d sec", realtime_runtime());
    output_verbose("starting up %s environment", global_environment);
    if (environment_start(argc, argv) == FAILED) {
        output_fatal("environment startup failed: %s", strerror(errno));
        /*	TROUBLESHOOT
            The requested environment could not be started.  This usually
            follows a more specific message regarding the startup problem.
            Follow the recommendation for the indicated problem.
         */
        if (exec_getexitcode() == XC_SUCCESS)
            exec_setexitcode(XC_ENVERR);
    }

    /* save the model */
    if (strcmp(global_savefile, "") != 0) {
        if (saveall(global_savefile) == FAILED)
            output_error("save to '%s' failed", global_savefile);
    }

    /* do module dumps */
    if (global_dumpall != false) {
        output_verbose("dumping module data");
        module_dumpall();
    }

    /* KML output */
    if (strcmp(global_kmlfile, "") != 0)
        kml_dump(global_kmlfile);

    /* terminate */
    module_termall();

    /* wrap up */
    output_verbose("shutdown complete");

    /* profile results */
    if (global_profiler) {
        class_profiles();
        module_profiles();
    }

#ifdef DUMP_SCHEDULES
    /* dump a copy of the schedules for reference */
    schedule_dumpall("schedules.txt");
#endif

    /* restore locale */
    locale_pop();

    /* if pause enabled */
#ifndef WIN32
#ifdef _DEBUG
    if (global_pauseatexit) {
        output_verbose("pausing at exit");

        /* Replicate "pause" on Windows */
        output_message("Press Enter to continue . . .");
        getchar();
    }
#endif
#endif

    /* compute elapsed runtime */
    output_verbose("elapsed runtime %d seconds", realtime_runtime());
    output_verbose("exit code %d", exec_getexitcode());
    exit(exec_getexitcode());
}

/** @} **/
