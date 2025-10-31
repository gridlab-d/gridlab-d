#include "gldapi.h"
#include <cstdio>
#include <fstream>
#include "timestamp.h"
#include "realtime.h"
#include "exec.h"
#include "output.h"
#include "threadpool.h"
#include "cmdarg.h"
#include "legal.h"
#include "globals.h"
#include "gldrandom.h"
#include "module.h"
#include "environment.h"
#include "save.h"
#include "kml.h"
#include "local.h"
#include "exec.h"
#include <json/json.h>
//#include <module.h>
//#include <module.h>

#include <array>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

namespace {

std::optional<fs::path> g_install_root_override;
std::optional<fs::path> g_executable_override;

fs::path weakly_canonical_or_self(const fs::path& candidate) {
    std::error_code ec;
    auto canonical = fs::weakly_canonical(candidate, ec);
    return ec ? candidate : canonical;
}

fs::path locate_exec_from_root(const fs::path& root) {
    const std::array<fs::path, 4> candidates = {
        root / "bin" / "gridlabd",
        root / "bin" / "gridlabd.exe",
        root / "gridlabd",
        root / "gridlabd.exe"
    };
    for (const auto& candidate : candidates) {
        if (!candidate.empty() && fs::exists(candidate)) {
            return weakly_canonical_or_self(candidate);
        }
    }
    return {};
}

void apply_runtime_paths(const fs::path& exec_path) {
    fs::path exec = weakly_canonical_or_self(exec_path);
    fs::path bin_dir = exec.parent_path();
    fs::path root = bin_dir.parent_path();
    if (root.empty()) {
        root = bin_dir;
    }

    global_gl_executable = exec;
    global_gl_bin = bin_dir;
    global_gl_share = root / "share";
    global_gl_include = root / "include";
    global_gl_lib = root / "lib";

    std::vector<std::string> segments;
    if (const char* env_glpath = std::getenv("GLPATH")) {
        if (*env_glpath != '\0') {
            segments.emplace_back(env_glpath);
        }
    }
    for (const auto& p : {global_gl_lib, global_gl_share, global_gl_include, global_gl_bin}) {
        std::string s = p.string();
        if (!s.empty()) {
            segments.emplace_back(s);
        }
    }

    std::string combined;
    for (size_t i = 0; i < segments.size(); ++i) {
        if (i > 0) combined += env_delim;
        combined += segments[i];
    }
    global_gl_path = combined;

    std::string exec_str = exec.string();
    strncpy(global_execname, exec_str.c_str(), sizeof(global_execname) - 1);
    global_execname[sizeof(global_execname) - 1] = '\0';

    std::string exec_dir_str = bin_dir.string();
    strncpy(global_execdir, exec_dir_str.c_str(), sizeof(global_execdir) - 1);
    global_execdir[sizeof(global_execdir) - 1] = '\0';
}

} // namespace

void GridLabD::set_install_root(const std::string& install_root) {
    fs::path candidate(install_root);
    
    // If it's an executable file, use it directly
    if (fs::exists(candidate) && !fs::is_directory(candidate)) {
        g_install_root_override = candidate.parent_path();
        g_executable_override = candidate;
        apply_runtime_paths(candidate);
        return;
    }
    
    // If it's a directory, look for the executable
    if (fs::is_directory(candidate)) {
        g_install_root_override = candidate;
        fs::path exec = locate_exec_from_root(candidate);
        if (!exec.empty()) {
            g_executable_override = exec;
            apply_runtime_paths(exec);
            return;
        }
        // If no executable found, still set up paths based on the root
        apply_runtime_paths(candidate / "bin" / "gridlabd");
        return;
    }
    
    throw std::runtime_error("Invalid install root: " + install_root);
}

std::string GridLabD::get_install_root() {
    if (g_install_root_override.has_value()) {
        return g_install_root_override.value().string();
    }
    return global_gl_bin.parent_path().string();
}

std::string GridLabD::get_executable_path() {
    if (g_executable_override.has_value()) {
        return g_executable_override.value().string();
    }
    return global_gl_executable.string();
}

 // constructor
GridLabD::GridLabD() {
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

    /* determine current working directory */
    if (!getcwd(global_workdir, 1024)) {
        global_workdir[0] = 0;
    }

    // Auto-discover paths if not already set
    if (!g_install_root_override.has_value()) {
        // Try to find gridlabd executable or use package location
        const char* override_path = std::getenv("GRIDLABD_ROOT");
        if (override_path != nullptr && *override_path != '\0') {
            try {
                set_install_root(override_path);
            } catch (...) {
                // If GRIDLABD_ROOT is invalid, continue without setting paths
                // The find_file() function will search GLPATH
            }
        }
        // Note: For Python packages, paths should be set via GRIDLABD_ROOT environment
        // variable or by calling set_install_root() explicitly from Python
    }

    if (setup_before_load() == GLD_OPERATION_FAILED)
    {
        exit(XC_INIERR);
    }
    /* see if newer version is available */
    if (global_check_version)
        check_version(1);

    /* setup the random number generator */
    random_init();
}



// Set configuration file
GLDErrorCode GridLabD::set_config_file(const std::string& config_file) {
    printf("Setting config file: %s\n", config_file.c_str());
    return GLD_SUCCESS;
}

// Set working directory
GLDErrorCode GridLabD::set_working_directory(const std::string& dir) {
    if (chdir(dir.c_str()) != 0) {
        output_error("Failed to change working directory to '%s': %s", dir.c_str(), strerror(errno));
        return GLD_OPERATION_FAILED;
    }
    
    // Update global_workdir
    if (!getcwd(global_workdir, sizeof(global_workdir))) {
        output_error("Failed to get current working directory after chdir");
        return GLD_OPERATION_FAILED;
    }
    
    printf("Working directory set to: %s\n", global_workdir);
    return GLD_SUCCESS;
}

// Load a GLM file
GLDErrorCode GridLabD::load_glm(int argc, char* argv[]) {
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
    setup_after_load();

    return GLD_SUCCESS;
}

GLDErrorCode GridLabD::load_glm(const std::string& filepath) {
    if (filepath.empty()) {
        return GLD_INVALID_FORMAT;
    }

    fs::path glm_path(filepath);
    if (!fs::exists(glm_path)) {
        return GLD_FILE_NOT_FOUND;
    }

    std::vector<std::string> argument_storage;
    argument_storage.emplace_back("gridlabd");
    argument_storage.emplace_back(glm_path.string());

    std::vector<char*> argv;
    argv.reserve(argument_storage.size());
    for (auto& arg : argument_storage) {
        argv.push_back(arg.data());
    }

    return load_glm(static_cast<int>(argv.size()), argv.data());
}

void set_clocks(std::optional<double> start_time, std::optional<double> stop_time) {
    if (start_time.has_value()) {
        printf("Setting start_time: %.2f\n", start_time.value());
        global_starttime = start_time.value();
    } else {
        printf("Using previous start_time: %.2f\n", global_starttime);
    }
    if (stop_time.has_value()) {
        printf("Setting stop_time: %.2f\n", stop_time.value());
        global_stoptime = stop_time.value();
    } else {
        printf("Using previous stop_time: %.2f\n", global_stoptime);
    }
    global_clock = global_starttime;
}
// Load a GLM file
GLDErrorCode GridLabD::setup_before_load() {
    
    /* set the default timezone */
    timestamp_set_tz(nullptr);

    exec_clock(); /* initialize the wall clock */
    realtime_starttime(); /* mark start */
    
    /* main initialization */
    if (!output_init() || !exec_init())
        return GLD_OPERATION_FAILED;

    /* set thread count equal to processor count if not passed on command-line */
    if (global_threadcount == 0)
        global_threadcount = processor_count();
    output_verbose("detected %d processor(s)", processor_count());
    output_verbose("using %d helper thread(s)", global_threadcount);
    
    /* stitch clock */
    global_clock = global_starttime;

    /* Check to see if stoptime is set - if not, set to 1-year later */
    if (global_stoptime == TS_NEVER) {
        global_stoptime = global_starttime + 31536000;
    }

    /* recheck threadcount in case user set it 0 */
    if (global_threadcount == 0) {
        global_threadcount = processor_count();
        output_verbose("using %d helper thread(s)", global_threadcount);
    }

    return GLD_SUCCESS;
}

// Load a GLM file
GLDErrorCode GridLabD::setup_after_load() {
    /* ensure clocks are synced */
    global_clock = global_starttime;
    /* initialize scheduler */
    sched_init(0);

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

    /* finalize all objects */
    output_verbose("finalizing all objects");
    if (exec_finalize_all() == FAILED) {
        output_error("object finalization failed");
    }

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

// Retrieve GLM data based on a query, optionally save to filepath
Json::Value GridLabD::get_checkpoint_json(const std::string& filepath) {
    Json::Value checkpoint;
    
    if (filepath.empty()) {
        // If no filepath provided, just return the JSON without saving
        checkpoint = do_checkpoint(nullptr);
    } else {
        // Extract directory from filepath for do_checkpoint
        size_t last_slash = filepath.find_last_of("/\\");
        std::string directory;
        
        if (last_slash != std::string::npos) {
            directory = filepath.substr(0, last_slash);
        } else {
            directory = "."; // Current directory if no path separators found
        }
        
        // Get checkpoint JSON with directory specified
        checkpoint = do_checkpoint(directory.c_str());
        
        // Additionally save the JSON directly to the specified filepath
        if (!checkpoint.empty()) {
            std::ofstream json_file(filepath);
            if (json_file.is_open()) {
                Json::StreamWriterBuilder builder;
                builder["indentation"] = "  "; // 2-space indentation
                std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
                writer->write(checkpoint, &json_file);
                json_file.close();
                printf("Checkpoint JSON saved to: %s\n", filepath.c_str());
            } else {
                printf("Error: Unable to open file '%s' for writing\n", filepath.c_str());
            }
        }
    }
    
    return checkpoint;
}

// Set the GLM model with provided data
GLDErrorCode GridLabD::set_glm_data(const GLDData& data) {
    printf("Setting GLM data with %zu fields.\n", data.size());
    return GLD_SUCCESS;
}

// Save simulation checkpoint
GLDErrorCode GridLabD::save_checkpoint(const std::string& save_path, GLDCheckPointMode mode) {
    printf("Saving checkpoint to %s with mode %d\n", save_path.c_str(), static_cast<int>(mode));
    Json::Value checkpoint = do_checkpoint(save_path.c_str()); // Use provided directory
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

// Common helper to check environment and handle failures
GLDErrorCode check_environment_and_handle_failure() {
    if (strcmp(global_environment, "batch") != 0) {
        output_fatal("%s environment not recognized or supported", global_environment);
        /*	TROUBLESHOOT
            The environment specified isn't supported. Currently only
            the <b>batch</b> environment is normally supported, although 
            some builds can support other environments, such as <b>matlab</b>.
        */
        return GLD_FAILED_TO_START;
    }
    return GLD_SUCCESS;
}

// Common helper to handle simulation failure with optional dump
GLDErrorCode handle_simulation_failure(const char* context_message) {
    output_fatal("shutdown after simulation stopped prematurely");
    /*	TROUBLESHOOT
        The simulation stopped because an unexpected condition was encountered.
        This can be caused by a wide variety of things, but most often it is
        because one of the objects in the model could not be synchronized 
        properly and its clock stopped.  This message usually follows a
        more specific message that indicates what caused the simulation to
        stop.
        */
    if (global_dumpfile[0] != '\0') {
        if (!saveall(global_dumpfile)) {
            output_error("dump to '%s' failed", global_dumpfile);
            /* TROUBLESHOOT
                An attempt to create a dump file failed.  This message should be
                preceded by a more detailed message explaining why it failed.
                Follow the guidance for that message and try again.
                */
        } else {
            output_debug("dump to '%s' complete", global_dumpfile);
        }
    }
    return GLD_FAILED_TO_START;
}

// Common helper to ensure simulation is initialized for stepping
GLDErrorCode ensure_simulation_initialized() {
    if (!exec_is_initialized()) {
        printf("Simulation not initialized, attempting to initialize...\n");
        
        GLDErrorCode env_check = check_environment_and_handle_failure();
        if (env_check != GLD_SUCCESS) {
            return env_check;
        }
        
        if (run_preparation() == FAILED) {
            printf("Failed to initialize simulation for stepping\n");
            return GLD_OPERATION_FAILED;
        }
        
        printf("Simulation initialized successfully\n");
    }
    return GLD_SUCCESS;
}

// Run simulation from start to end
GLDErrorCode GridLabD::run(std::optional<double> start_time, std::optional<double> stop_time) {
    set_clocks(start_time, stop_time);
    
    GLDErrorCode env_check = check_environment_and_handle_failure();
    if (env_check != GLD_SUCCESS) {
        return env_check;
    }
    
    if (exec_start() == FAILED) {
        return handle_simulation_failure("exec_start failed");
    }
    
    return GLD_SUCCESS;
}

// Perform a single time step
GLDErrorCode GridLabD::step(double& simulation_time) {
    printf("Stepping simulation forward\n");
    
    // Ensure simulation is initialized
    GLDErrorCode init_result = ensure_simulation_initialized();
    if (init_result != GLD_SUCCESS) {
        simulation_time = (double)global_clock;
        return init_result;
    }
    
    // Store the current global clock before stepping
    TIMESTAMP prev_clock = global_clock;
    
    // Execute a single simulation step
    STATUS result = exec_step();
    
    if (result == FAILED) {
        printf("Error occurred during simulation step\n");
        simulation_time = (double)global_clock;
        return GLD_OPERATION_FAILED;
    }
    
    // Update the simulation time
    simulation_time = (double)global_clock;
    
    printf("Stepped from time %.2f to %.2f\n", (double)prev_clock, simulation_time);
    
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
    if (time_step <= 0) {
        printf("Error: Time step must be positive, got: %.2f\n", time_step);
        return GLD_OPERATION_FAILED;
    }
    
    // Convert to TIMESTAMP units (seconds to internal time units)
    // GridLAB-D uses integer TIMESTAMP, so convert double seconds to integer
    global_minimum_timestep = static_cast<int>(time_step);
    
    printf("Setting minimum simulation time step to: %d seconds\n", global_minimum_timestep);
    return GLD_SUCCESS;
}

