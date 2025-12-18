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
#include "object.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <map>
#include <chrono>
#include <unistd.h>
#include <sys/wait.h>
//#include <module.h>
//#include <module.h>

namespace fs = std::filesystem;


std::vector<std::string> split_path_x(const std::string& path, char sep){
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
findExecutable_x(const std::string &name, const std::string &execName, const std::string &pathString) {
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

        auto splitPath = split_path_x(sys_path, env_delim_char);

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


 // constructor
GridLabD::GridLabD() {
    // Initialization code goes here
        char *pd1, *pd2;
    int i, pos = 0;

    std::string exec_name = "gridlabd";
    global_gl_executable = findExecutable_x("gridlabd", exec_name, getenv("PATH"));
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
    started_at = realtime_starttime(); /* mark start */

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
    strcpy(global_execname, exec_name.c_str());
    strcpy(global_execdir, exec_name.c_str());
    pd1 = strrchr(global_execdir, '/');
    pd2 = strrchr(global_execdir, '\\');
    if (pd1 > pd2) *pd1 = '\0';
    else if (pd2 > pd1) *pd2 = '\0';

    /* determine current working directory */
    char *result = getcwd(global_workdir, 1024);

    if(setup_before_load() == GLD_OPERATION_FAILED)
    {
        exit(XC_INIERR);
    }
    /* see if newer version is available */
    if (global_check_version)
        check_version(1);

    /* enable profiling for performance analysis */
    global_profiler = 1;
    global_mt_analysis = 1;

    /* setup the random number generator */
    random_init();
}

// Set configuration file
GLDErrorCode GridLabD::set_config_file(const std::string& config_file) {
    printf("Setting config file: %s\n", config_file.c_str());
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

void set_clocks(std::optional<double> start_time, std::optional<double> stop_time) {
    if (start_time.has_value()) {
        printf("Setting start_time: %.2f\n", start_time.value());
        global_starttime = start_time.value();
    } else {
        printf("Using previous start_time: %lld\n", (long long)global_starttime);
    }
    if (stop_time.has_value()) {
        printf("Setting stop_time: %.2f\n", stop_time.value());
        global_stoptime = stop_time.value();
    } else {
        printf("Using previous stop_time: %lld\n", (long long)global_stoptime);
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
    global_profiler = 1;
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

    // Put reporting stuff here, probably not the xml dump

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

    report_performance_after_run(started_at, passes, tsteps);

    /* compute elapsed runtime */
    output_verbose("elapsed runtime %d seconds", realtime_runtime());
    output_verbose("exit code %d", exec_getexitcode());
    exit(exec_getexitcode());
    return GLD_SUCCESS;
}

// Retrieve GLM data based on a query, optionally save to filepath
nlohmann::ordered_json GridLabD::get_checkpoint_json(const std::string& filepath) {
    nlohmann::ordered_json checkpoint;
    
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
    }
    
    // Set the internal gld_model representation to be equal to checkpoint
    gld_model = nlohmann::ordered_json(checkpoint);
    
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
    nlohmann::json checkpoint = do_checkpoint(save_path.c_str()); // Use provided directory
    
    // Set the internal gld_model representation to be equal to checkpoint
    gld_model = nlohmann::json(checkpoint);
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
    
    if (exec_start(&passes, &tsteps) == FAILED) {
        return handle_simulation_failure("exec_start failed");
    }
    
    gld_model = get_checkpoint_json();

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
    
    STATUS result = exec_step(&passes, &tsteps);
    
    if (result == FAILED) {
        printf("Error occurred during simulation step\n");
        simulation_time = (double)global_clock;
        return GLD_OPERATION_FAILED;
    }
    
    // Update the simulation time
    simulation_time = (double)global_clock;
    
    printf("Stepped from time %.2f to %.2f (total passes: %lld, total tsteps: %lld)\n", 
           (double)prev_clock, simulation_time, passes, tsteps);
    
    // Not creating the checkpoint after every step, user can do that manually.
    // gld_model = get_checkpoint_json();
    
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

// Simple object finding method implementation

void* GridLabD::find_object_by_name(const std::string& object_name) {
    // Traverse the object list directly using the safe object_get_first() function
    OBJECT* obj = object_get_first();
    while (obj != nullptr) {
        // Check if this object has the name we're looking for
        if (obj->name && std::string(obj->name) == object_name) {
            return static_cast<void*>(obj);
        }
        obj = obj->next;
    }
    
    return nullptr; // Object not found
}

// Property access methods implementation

GLDErrorCode GridLabD::get_property_value(void* object_ptr, const std::string& property_name, std::string& value) {
    if (!object_ptr) {
        return GLD_OBJECT_NOT_FOUND;
    }
    
    OBJECT* obj = static_cast<OBJECT*>(object_ptr);
    char buffer[1024];
    
    // Use the safe object_get_value_by_name function
    int result = object_get_value_by_name(obj, property_name.c_str(), buffer, sizeof(buffer));
    if (result >= 0) {
        value = std::string(buffer);
        return GLD_SUCCESS;
    }
    
    return GLD_OPERATION_FAILED;
}

GLDErrorCode GridLabD::set_property_value(void* object_ptr, const std::string& property_name, const std::string& value) {
    if (!object_ptr) {
        printf("set_property_value: object_ptr is null\n");
        return GLD_OBJECT_NOT_FOUND;
    }
    
    OBJECT* obj = static_cast<OBJECT*>(object_ptr);
    printf("set_property_value: Setting %s.%s = %s\n", obj->name ? obj->name : "unnamed", property_name.c_str(), value.c_str());
    
    // Create a non-const copy for the GridLAB-D API
    std::string val_copy = value;
    
    // Use the safe object_set_value_by_name function
    int result = object_set_value_by_name(obj, const_cast<char*>(property_name.c_str()), const_cast<char*>(val_copy.c_str()));
    printf("set_property_value: object_set_value_by_name returned %d\n", result);
    
    if (result > 0) {
        printf("set_property_value: SUCCESS - Property set successfully\n");
        return GLD_SUCCESS;
    }
    
    printf("set_property_value: FAILED - Property not set (result=%d)\n", result);
    return GLD_OPERATION_FAILED;
}

// Validation structures and functions
namespace {
    struct TestResult {
        std::string test_path;
        std::string test_name;
        std::string module;
        bool success;
        std::string error_message;
        double duration_seconds;
    };

    struct TestSummary {
        int total_tests = 0;
        int passed_tests = 0;
        int failed_tests = 0;
        std::vector<TestResult> results;
    };

    int run_single_test(const std::filesystem::path& test_dir, const std::string& test_name) {
        namespace fs = std::filesystem;
        
        try {
            fs::current_path(test_dir);
            
            GridLabD gld;
            
            std::string glm_filename = test_name + ".glm";
            std::vector<const char*> args = {"gridlabd", glm_filename.c_str()};
            int test_argc = static_cast<int>(args.size());
            std::vector<char*> test_argv;
            for (const auto& arg : args) {
                test_argv.push_back(const_cast<char*>(arg));
            }
            
            if (gld.load_glm(test_argc, test_argv.data()) != GLD_SUCCESS) {
                return 1;
            }
            
            if (gld.run() != GLD_SUCCESS) {
                return 1;
            }
            
            return 0;
            
        } catch (...) {
            return 1;
        }
    }

    void print_test_summary(const TestSummary& summary) {
        std::cout << "=============================================================\n";
        std::cout << "                    TEST SUMMARY\n";
        std::cout << "=============================================================\n\n";
        
        std::cout << "Total Tests:  " << summary.total_tests << "\n";
        std::cout << "Passed:       " << summary.passed_tests 
                  << " (" << std::fixed << std::setprecision(2) 
                  << (summary.total_tests > 0 ? (100.0 * summary.passed_tests / summary.total_tests) : 0.0) 
                  << "%)\n";
        std::cout << "Failed:       " << summary.failed_tests 
                  << " (" << std::fixed << std::setprecision(2) 
                  << (summary.total_tests > 0 ? (100.0 * summary.failed_tests / summary.total_tests) : 0.0) 
                  << "%)\n\n";
        
        // Group results by module
        std::map<std::string, std::pair<int, int>> module_stats;
        for (const auto& result : summary.results) {
            if (result.success) {
                module_stats[result.module].first++;
            }
            module_stats[result.module].second++;
        }
        
        std::cout << "Results by Module:\n";
        std::cout << std::string(60, '-') << "\n";
        for (const auto& [module, stats] : module_stats) {
            std::cout << "  " << module << ": " << stats.first << "/" << stats.second << " passed\n";
        }
        
        // Show failed tests
        std::vector<TestResult> failed_tests;
        for (const auto& result : summary.results) {
            if (!result.success) {
                failed_tests.push_back(result);
            }
        }
        
        if (!failed_tests.empty()) {
            std::cout << "\nFailed Tests:\n";
            std::cout << std::string(60, '-') << "\n";
            for (const auto& result : failed_tests) {
                std::cout << "  ✗ " << result.module << "/" << result.test_name << "\n";
                if (!result.error_message.empty()) {
                    std::cout << "    " << result.error_message << "\n";
                }
            }
        }
        
        std::cout << "\n=============================================================\n\n";
    }
}

GLDErrorCode GridLabD::validate(const std::string& repo_root, const std::vector<std::string>& modules) {
    namespace fs = std::filesystem;
    TestSummary summary;
    
    std::vector<std::string> all_modules = {
        "assert", "climate", "commercial", "connection", "generators",
        "market", "mysql", "network", "plc", "powerflow", 
        "reliability", "residential", "rest", "tape", "taxonomy_feeders"
    };
    
    std::vector<std::string> modules_to_test = modules.empty() ? all_modules : modules;
    
    // Determine the search path for autotests
    // Autotests are located in the source tree at MODULE/autotest/ (e.g., residential/autotest/)
    fs::path search_root;
    if (!repo_root.empty() && fs::exists(repo_root)) {
        search_root = repo_root;
    } else {
        // Default to current working directory (typically the source tree root)
        search_root = fs::current_path();
    }
    
    std::cout << "=============================================================\n";
    std::cout << "         GridLAB-D Autotest Suite Runner\n";
    std::cout << "=============================================================\n";
    std::cout << "Search path: " << search_root << "\n\n";
    
    int modules_found = 0;
    for (const auto& module : modules_to_test) {
        fs::path autotest_dir = search_root / module / "autotest";
        
        if (!fs::exists(autotest_dir) || !fs::is_directory(autotest_dir)) {
            continue;
        }
        
        modules_found++;
        std::cout << "Module: " << module << "\n";
        std::cout << std::string(60, '-') << "\n";
        
        std::vector<fs::path> test_dirs;
        for (const auto& entry : fs::directory_iterator(autotest_dir)) {
            if (entry.is_directory()) {
                std::string dirname = entry.path().filename().string();
                if (dirname.rfind("test_", 0) == 0) {
                    fs::path glm_file = entry.path() / (dirname + ".glm");
                    if (fs::exists(glm_file)) {
                        test_dirs.push_back(entry.path());
                    }
                }
            }
        }
        
        std::sort(test_dirs.begin(), test_dirs.end());
        
        for (const auto& test_dir : test_dirs) {
            std::string test_name = test_dir.filename().string();
            
            TestResult result;
            result.test_path = test_dir.string();
            result.test_name = test_name;
            result.module = module;
            result.success = false;
            
            std::cout << "  Running: " << test_name << " ... " << std::flush;
            
            fs::path original_cwd = fs::current_path();
            
            auto start_time = std::chrono::high_resolution_clock::now();
            
            pid_t pid = fork();
            
            if (pid == -1) {
                result.error_message = "Failed to fork process";
                result.success = false;
                summary.failed_tests++;
            } else if (pid == 0) {
                freopen("/dev/null", "w", stdout);
                freopen("/dev/null", "w", stderr);
                
                int test_result = run_single_test(test_dir, test_name);
                exit(test_result);
            } else {
                int status;
                waitpid(pid, &status, 0);
                
                bool is_error_test = test_name.find("_err") != std::string::npos;
                
                if (WIFEXITED(status)) {
                    int exit_code = WEXITSTATUS(status);
                    bool test_passed = is_error_test ? (exit_code != 0) : (exit_code == 0);
                    
                    if (test_passed) {
                        result.success = true;
                        summary.passed_tests++;
                    } else {
                        if (is_error_test) {
                            result.error_message = "Error test unexpectedly succeeded (exit code 0)";
                        } else {
                            result.error_message = "Test returned exit code " + std::to_string(exit_code);
                        }
                        summary.failed_tests++;
                    }
                } else if (WIFSIGNALED(status)) {
                    if (is_error_test) {
                        result.success = true;
                        summary.passed_tests++;
                    } else {
                        result.error_message = "Test terminated by signal " + std::to_string(WTERMSIG(status));
                        summary.failed_tests++;
                    }
                } else {
                    result.error_message = "Test terminated abnormally";
                    summary.failed_tests++;
                }
            }
            
            auto end_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> duration = end_time - start_time;
            result.duration_seconds = duration.count();
            
            fs::current_path(original_cwd);
            
            if (result.success) {
                std::cout << "✓ PASS (" << std::fixed << std::setprecision(2) 
                         << result.duration_seconds << "s)\n";
            } else {
                std::cout << "✗ FAIL (" << std::fixed << std::setprecision(2) 
                         << result.duration_seconds << "s)\n";
                std::cout << "    Error: " << result.error_message << "\n";
            }
            
            summary.results.push_back(result);
            summary.total_tests++;
        }
        
        std::cout << "\n";
    }
    
    if (modules_found == 0) {
        std::cout << "\n*** WARNING: No autotest directories found ***\n";
        std::cout << "Searched in: " << search_root << "\n";
        std::cout << "\nAutotests are located in the source tree at MODULE/autotest/ directories.\n";
        std::cout << "To fix this issue:\n";
        std::cout << "  - Specify the repo_root parameter pointing to the GridLAB-D source tree\n";
        std::cout << "    Example: gld.validate('/path/to/gridlab-d')\n";
        std::cout << "  - Or run from the GridLAB-D source tree root directory\n\n";
        return GLD_FILE_NOT_FOUND;
    }
    
    print_test_summary(summary);
    
    return (summary.failed_tests > 0) ? GLD_OPERATION_FAILED : GLD_SUCCESS;
}

/**
 * Self-contained API health check that validates core functionality.
 * Creates a temporary test model with a residential house object and runs
 * a series of tests to verify: GLM loading, object lookup, property access,
 * simulation execution, and in-memory JSON checkpoint generation.
 * 
 * @param verbose If true, prints detailed test results to stdout
 * @return GLD_SUCCESS if all tests pass, GLD_OPERATION_FAILED otherwise
 */
GLDErrorCode GridLabD::validate_api(bool verbose) {
    namespace fs = std::filesystem;
    
    if (verbose) {
        std::cout << "=============================================================\n";
        std::cout << "         GridLAB-D API Health Check\n";
        std::cout << "=============================================================\n\n";
    }
    
    int passed = 0;
    int failed = 0;
    
    // Create a simple test file with a house object
    fs::path temp_dir = fs::temp_directory_path() / "gldapi_health_check";
    fs::create_directories(temp_dir);
    fs::path test_file = temp_dir / "health_check.glm";
    
    std::ofstream glm_file(test_file);
    glm_file << "#set suppress_repeat_messages=1\n"
             << "#set checkpoint_type=SIM\n"
             << "clock {\n"
             << "  timezone PST+8PDT;\n"
             << "  starttime '2000-01-01 0:00:00';\n"
             << "  stoptime '2000-01-01 0:15:00';\n"
             << "}\n"
             << "module residential {\n"
             << "  implicit_enduses NONE;\n"
             << "}\n"
             << "object house {\n"
             << "  name test_house;\n"
             << "  floor_area 2000;\n"
             << "}\n";
    glm_file.close();
    
    // Use this instance for all tests - keep the path as a string to ensure it stays valid
    std::string test_file_str = test_file.string();
    char* argv[] = {const_cast<char*>("gridlabd"), const_cast<char*>(test_file_str.c_str())};
    int argc = 2;
    
    // Declare test_house_obj before any gotos to avoid C++ scoping issues
    void* test_house_obj = nullptr;
    
    // Test 1: Basic GLM loading
    if (verbose) std::cout << "Test 1: Basic GLM loading... " << std::flush;
    try {
        if (this->load_glm(argc, argv) == GLD_SUCCESS) {
            if (verbose) std::cout << "✓ PASS\n";
            passed++;
        } else {
            if (verbose) std::cout << "✗ FAIL\n";
            failed++;
            goto cleanup;
        }
    } catch (...) {
        if (verbose) std::cout << "✗ FAIL (exception)\n";
        failed++;
        goto cleanup;
    }
    
    // Test 2: Object finding
    if (verbose) std::cout << "Test 2: Object lookup... " << std::flush;
    try {
        test_house_obj = this->find_object_by_name("test_house");
        if (test_house_obj != nullptr) {
            if (verbose) std::cout << "✓ PASS\n";
            passed++;
        } else {
            if (verbose) std::cout << "✗ FAIL\n";
            failed++;
        }
    } catch (...) {
        if (verbose) std::cout << "✗ FAIL (exception)\n";
        failed++;
    }
    
    // Test 3: Property get
    if (verbose) std::cout << "Test 3: Property get... " << std::flush;
    try {
        if (test_house_obj) {
            std::string floor_area;
            GLDErrorCode result = this->get_property_value(test_house_obj, "floor_area", floor_area);
            // Just check if we can get a property value successfully (don't validate exact format)
            if (result == GLD_SUCCESS && !floor_area.empty()) {
                if (verbose) std::cout << "✓ PASS\n";
                passed++;
            } else {
                if (verbose) std::cout << "✗ FAIL\n";
                failed++;
            }
        } else {
            if (verbose) std::cout << "✗ FAIL (no object from Test 2)\n";
            failed++;
        }
    } catch (...) {
        if (verbose) std::cout << "✗ FAIL (exception)\n";
        failed++;
    }
    
    // Test 4: Simulation run
    if (verbose) std::cout << "Test 4: Simulation execution... " << std::flush;
    try {
        if (this->run() == GLD_SUCCESS) {
            if (verbose) std::cout << "✓ PASS\n";
            passed++;
        } else {
            if (verbose) std::cout << "✗ FAIL\n";
            failed++;
        }
    } catch (...) {
        if (verbose) std::cout << "✗ FAIL (exception)\n";
        failed++;
    }
    
    // Test 5: In-memory JSON checkpoint
    if (verbose) std::cout << "Test 5: JSON checkpoint (in-memory)... " << std::flush;
    try {
        // Get checkpoint JSON in memory (no file writing since no directory specified)
        nlohmann::ordered_json checkpoint = this->get_checkpoint_json();
        
        // Validate the checkpoint has expected structure
        // Note: checkpoint["objects"] is an object/dictionary of classes, not an array
        bool valid = !checkpoint.empty() && 
                     checkpoint.contains("globals") &&
                     checkpoint.contains("objects") &&
                     checkpoint["objects"].is_object()&&
                     checkpoint.contains("modules") &&
                     checkpoint["modules"].is_object()&& 
                     checkpoint.contains("clock") &&
                     checkpoint["clock"].is_object();                                         
        
        if (valid) {
            if (verbose) std::cout << "✓ PASS\n";
            passed++;
        } else {
            if (verbose) std::cout << "✗ FAIL\n";
            failed++;
        }
    } catch (...) {
        if (verbose) std::cout << "✗ FAIL (exception)\n";
        failed++;
    }
    
cleanup:
    fs::remove_all(temp_dir);
    
    if (verbose) {
        std::cout << "\n=============================================================\n";
        std::cout << "                    HEALTH CHECK SUMMARY\n";
        std::cout << "=============================================================\n\n";
        std::cout << "Total Tests:  " << (passed + failed) << "\n";
        std::cout << "Passed:       " << passed << " (" << std::fixed << std::setprecision(2)
                  << (100.0 * passed / (passed + failed)) << "%)\n";
        std::cout << "Failed:       " << failed << " (" << std::fixed << std::setprecision(2)
                  << (100.0 * failed / (passed + failed)) << "%)\n\n";
        
        if (failed == 0) {
            std::cout << "✓ GridLAB-D API is functioning correctly\n";
        } else {
            std::cout << "✗ GridLAB-D API has issues - some tests failed\n";
        }
        std::cout << "\n=============================================================\n\n";
    }
    
    return (failed == 0) ? GLD_SUCCESS : GLD_OPERATION_FAILED;
}

