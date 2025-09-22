#include "gldapi.h"
#include <cstdio>
#include "timestamp.h"
#include "realtime.h"
#include "exec.h"
#include "output.h"
#include "threadpool.h"
#include "cmdarg.h"
#include <module.h>

 // Default constructor
GridLabD::GridLabD() {
        // Initialization code goes here
}

 // constructor
GridLabD::GridLabD(int argc, char* argv[]) {
    // Initialization code goes here
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
    timestamp_set_tz(nullptr);

    exec_clock(); /* initialize the wall clock */
    realtime_starttime(); /* mark start */

    /* set the process info */
    global_process_id = getpid();

    /* specify the default browser */
    if (browser != nullptr)
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
    setup_before_load(argc, argv);
    
}

// Set configuration file
GLDErrorCode GridLabD::set_config_file(const std::string& config_file) {
    printf("Setting config file: %s\n", config_file.c_str());
    return GLD_SUCCESS;
}

// Load a GLM file
GLDErrorCode GridLabD::load_glm(const std::string& filepath) {
    glm_file_path = filepath;
    printf("Loading GLM file: %s\n", filepath.c_str());
    // load_all()

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

    return GLD_SUCCESS;
}

// Load a GLM file
GLDErrorCode GridLabD::setup_before_load(const std::string& filepath) {
    glm_file_path = filepath;
    printf("Setup GLD %s\n", filepath.c_str());
    // load_all()
    
    /* set the default timezone */
    timestamp_set_tz(nullptr);

    exec_clock(); /* initialize the wall clock */
    realtime_starttime(); /* mark start */
    
    /* main initialization */
    if (!output_init(argc, argv) || !exec_init())
        exit(XC_INIERR);

    /* set thread count equal to processor count if not passed on command-line */
    if (global_threadcount == 0)
        global_threadcount = processor_count();
    output_verbose("detected %d processor(s)", processor_count());
    output_verbose("using %d helper thread(s)", global_threadcount);


    return GLD_SUCCESS;
}

// Load a GLM file
GLDErrorCode GridLabD::setup_after_load(const std::string& filepath) {
    glm_file_path = filepath;
    printf("Loading GLM file: %s\n", filepath.c_str());
    // load_all()
    
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
    return GLD_SUCCESS;
}

// Exit GLD
GLDErrorCode GridLabD::exit_gld(const std::string& filepath) {
    glm_file_path = filepath;
    printf("Exit GLD: %s\n", filepath.c_str());
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
    return GLD_SUCCESS;
}

// Retrieve GLM data based on a query
GLDErrorCode GridLabD::get_glm_data(const std::string& query, GLDData& result) {
    printf("Getting GLM data for query: %s\n", query.c_str());
    result["status"] = std::string("mocked_result");
    return GLD_SUCCESS;
}

// Set the GLM model with provided data
GLDErrorCode GridLabD::set_glm_data(const GLDData& data) {
    printf("Setting GLM data with %zu fields.\n", data.size());
    return GLD_SUCCESS;
}

// Save simulation checkpoint
GLDErrorCode GridLabD::save_checkpoint(const std::string& save_path, GLDCheckPointMode mode) {
    printf("Saving checkpoint to %s with mode %d\n", save_path.c_str(), static_cast<int>(mode));
    return GLD_SUCCESS;
}

// Load simulation checkpoint
GLDErrorCode GridLabD::load_checkpoint(const std::string& file_path) {
    printf("Loading checkpoint from %s\n", file_path.c_str());
    return GLD_SUCCESS;
}

// Add an object
GLDErrorCode GridLabD::add_object(GLDData& object_data) {
    printf("Adding object with %zu fields.\n", object_data.size());
    return GLD_SUCCESS;
}

// Delete an object
GLDErrorCode GridLabD::delete_object(const std::string& name) {
    printf("Deleting object named: %s\n", name.c_str());
    return GLD_SUCCESS;
}

// Edit an object
GLDErrorCode GridLabD::edit_object(const std::string& name, const GLDData& updated_data) {
    printf("Editing object: %s with %zu fields.\n", name.c_str(), updated_data.size());
    return GLD_SUCCESS;
}

// Run simulation from start to end
GLDErrorCode GridLabD::run(double start_time, double end_time, double& simulation_time) {
    printf("Running simulation from %.2f to %.2f\n", start_time, end_time);
    simulation_time = end_time;


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

    return GLD_SUCCESS;
}

// Perform a single time step
GLDErrorCode GridLabD::step(double& simulation_time) {
    printf("Stepping simulation forward\n");
    simulation_time += 1.0;
    return GLD_SUCCESS;
}

// Set pre-step callback
GLDErrorCode GridLabD::set_prestep_callback(GLDCallback callback) {
    printf("Setting pre-step callback\n");
    return GLD_SUCCESS;
}

// Set post-step callback
GLDErrorCode GridLabD::set_poststep_callback(GLDCallback callback) {
    printf("Setting post-step callback\n");
    return GLD_SUCCESS;
}

// Reset timestep
GLDErrorCode GridLabD::reset_step(double& current_time) {
    printf("Resetting simulation step\n");
    current_time = 0.0;
    return GLD_SUCCESS;
}

// Set time manually
GLDErrorCode GridLabD::set_time(const std::string& timestamp) {
    printf("Setting time to: %s\n", timestamp.c_str());
    return GLD_SUCCESS;
}

// Get current simulation time
GLDErrorCode GridLabD::get_time(std::string& current_time) {
    current_time = "2025-06-12T12:00:00";
    printf("Getting current time: %s\n", current_time.c_str());
    return GLD_SUCCESS;
}

// Set application mode
GLDErrorCode GridLabD::set_application_mode(GLDApplicationType mode) {
    printf("Setting application mode: %d\n", static_cast<int>(mode));
    return GLD_SUCCESS;
}

// Set timestep
GLDErrorCode GridLabD::set_time_step(double time_step) {
    printf("Setting simulation time step to: %.2f\n", time_step);
    return GLD_SUCCESS;
}
