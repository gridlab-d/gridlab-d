#include "gldapi.h"
#include "cmdarg.h"
#include "environment.h"
#include "exec.h"
#include "gldrandom.h"
#include "globals.h"
#include "gridlabd.h"
#include "kml.h"
#include "legal.h"
#include "local.h"
// #include <module.h>

// External declarations for message capture functions in output.cpp
extern std::vector<std::map<std::string, std::string>>
output_get_captured_messages();
extern void output_clear_captured_messages();
extern void output_enable_capture(bool enable);
extern void output_set_message_capture_limit(size_t limit);
extern size_t output_get_message_capture_limit();
// #include <module.h>

#include "globals.h"
//#include "kill.h"
#include "load.h"
#include "object.h"
#include "save.h"
#include "cpp_threadpool.h"
#include <array>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <map>
#include <chrono>
#include <sys/wait.h>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>


#ifdef _WIN32
#include <direct.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace {

std::optional<fs::path> g_install_root_override;
std::optional<fs::path> g_executable_override;

std::string object_identifier(const OBJECT *obj) {
  if (obj == nullptr) {
    return "";
  }

  if (obj->name != nullptr && obj->name[0] != '\0') {
    return std::string(obj->name);
  }

  char id_str[32];
  snprintf(id_str, sizeof(id_str), "%d", obj->id);
  return std::string(id_str);
}

std::map<std::string, std::string>
collect_property_map_for_object(OBJECT *obj) {
  std::map<std::string, std::string> property_map;

  if (obj == nullptr || obj->oclass == nullptr ||
      obj->oclass->name == nullptr) {
    return property_map;
  }

  property_map["__class__"] = std::string(obj->oclass->name);
  property_map["__id__"] = std::to_string(obj->id);
  if (obj->name != nullptr && obj->name[0] != '\0') {
    property_map["__name__"] = std::string(obj->name);
    // Also add 'name' as a regular property for user convenience
    property_map["name"] = std::string(obj->name);
  }

  PROPERTY *prop = class_get_first_property(obj->oclass);
  while (prop != nullptr) {
    char buffer[1024];
    int result =
        object_get_value_by_name(obj, prop->name, buffer, sizeof(buffer));
    if (result != 0) {
      property_map[std::string(prop->name)] = std::string(buffer);
    }
    prop = prop->next;
  }

  return property_map;
}

fs::path weakly_canonical_or_self(const fs::path &candidate) {
  std::error_code ec;
  auto canonical = fs::weakly_canonical(candidate, ec);
  return ec ? candidate : canonical;
}

fs::path locate_exec_from_root(const fs::path &root) {
  const std::array<fs::path, 4> candidates = {
      root / "bin" / "gridlabd", root / "bin" / "gridlabd.exe",
      root / "gridlabd", root / "gridlabd.exe"};
  for (const auto &candidate : candidates) {
    if (!candidate.empty() && fs::exists(candidate)) {
      return weakly_canonical_or_self(candidate);
    }
  }
  return {};
}

bool validate_gridlabd_installation(const fs::path &root) {
  // Check for required directory structure
  // At minimum, we need either:
  // - share/ directory (for data files like tzinfo.txt)
  // - lib/ directory (for modules)
  // - gldcore/ directory (for development mode)

  fs::path share_dir = root / "share";
  fs::path lib_dir = root / "lib";
  fs::path gldcore_dir = root / "gldcore";

  bool has_share = fs::exists(share_dir) && fs::is_directory(share_dir);
  bool has_lib = fs::exists(lib_dir) && fs::is_directory(lib_dir);
  bool has_gldcore = fs::exists(gldcore_dir) && fs::is_directory(gldcore_dir);

  // Valid if it has share or gldcore (for data files) and optionally lib (for
  // modules)
  return has_share || has_gldcore || has_lib;
}

void apply_runtime_paths(const fs::path &exec_path) {
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
  if (const char *env_glpath = std::getenv("GLPATH")) {
    if (*env_glpath != '\0') {
      segments.emplace_back(env_glpath);
    }
  }
  for (const auto &p :
       {global_gl_lib, global_gl_share, global_gl_include, global_gl_bin}) {
    std::string s = p.string();
    if (!s.empty()) {
      segments.emplace_back(s);
    }
  }

  std::string combined;
  for (size_t i = 0; i < segments.size(); ++i) {
    if (i > 0)
      combined += env_delim;
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

void GridLabD::set_install_root(const std::string &install_root) {
  fs::path candidate(install_root);

  // If it's an executable file, use it directly
  if (fs::exists(candidate) && !fs::is_directory(candidate)) {
    g_install_root_override = candidate.parent_path();
    g_executable_override = candidate;
    apply_runtime_paths(candidate);
    return;
  }

  // If it's a directory, validate it has required structure
  if (fs::is_directory(candidate)) {
    if (!validate_gridlabd_installation(candidate)) {
      throw std::runtime_error(
          "Invalid GridLAB-D installation: " + install_root +
          " (missing required directories: share/, lib/, or gldcore/)");
    }

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

  throw std::runtime_error("Invalid install root: " + install_root +
                           " (path does not exist)");
}

std::string GridLabD::get_install_root() {
  if (g_install_root_override.has_value()) {
    return g_install_root_override.value().string();
  }
  return global_gl_bin.parent_path().string();
}

// constructor
GridLabD::GridLabD() : selected_timestep(0) {
  strcpy(global_environment, "batch");
  char *browser = getenv("GLBROWSER");

  /* determine current working directory */
  if (!getcwd(global_workdir, 1024)) {
    global_workdir[0] = 0;
  }

  // Auto-discover paths if not already set (must happen before timestamp_set_tz
  // so that find_file() can locate tzinfo.txt via global_gl_path)
  if (!g_install_root_override.has_value()) {
    // Try environment variables in priority order: GRIDLABD_HOME >
    // GRIDLABD_ROOT GRIDLABD_HOME: For custom GridLAB-D installations
    // (modules/data) GRIDLABD_ROOT: For bundled package installations (backward
    // compatibility)
    const char *override_path = std::getenv("GRIDLABD_HOME");
    if (override_path == nullptr || *override_path == '\0') {
      override_path = std::getenv("GRIDLABD_ROOT");
    }

    if (override_path != nullptr && *override_path != '\0') {
      try {
        set_install_root(override_path);
      } catch (...) {
        // If environment variable path is invalid, continue without setting
        // paths. The find_file() function will search GLPATH.
      }
    }
  }

  /* set the default timezone */
  timestamp_set_tz(nullptr);

  exec_clock();         /* initialize the wall clock */
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

  if (setup_before_load() == GLD_OPERATION_FAILED) {
    exit(XC_INIERR);
  }
  /* see if newer version is available */
  if (global_check_version)
    check_version(1);

  /* setup the random number generator */
  random_init();
}

// Set configuration file
GLDErrorCode GridLabD::set_config_file(const std::string &config_file) {
  output_verbose("Setting config file: %s", config_file.c_str());
  return GLD_SUCCESS;
}

// Set working directory
GLDErrorCode GridLabD::set_working_directory(const std::string &dir) {
  if (chdir(dir.c_str()) != 0) {
    output_error("Failed to change working directory to '%s': %s", dir.c_str(),
                 strerror(errno));
    return GLD_OPERATION_FAILED;
  }

  // Update global_workdir
  if (!getcwd(global_workdir, sizeof(global_workdir))) {
    output_error("Failed to get current working directory after chdir");
    return GLD_OPERATION_FAILED;
  }

  output_verbose("Working directory set to: %s", global_workdir);
  return GLD_SUCCESS;
}

// Load a GLM file
GLDErrorCode GridLabD::load_glm(int argc, char *argv[]) {
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
  /* load the GLM file */
  if (argc > 0 && argv[0] != nullptr) {
    if (loadall(argv[0]) == FAILED) {
      output_fatal("failed to load model file: %s", argv[0]);
      return GLD_FAILED_TO_START;
    }
  }
  setup_after_load();

  return GLD_SUCCESS;
}

GLDErrorCode GridLabD::load_glm(const std::string &filepath) {
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

  std::vector<char *> argv;
  argv.reserve(argument_storage.size());
  for (auto &arg : argument_storage) {
    argv.push_back(arg.data());
  }

  return load_glm(static_cast<int>(argv.size()), argv.data());
}

void set_clocks(std::optional<double> start_time,
                std::optional<double> stop_time) {
  if (start_time.has_value()) {
    output_verbose("Setting start_time: %.2f", start_time.value());
    global_starttime = start_time.value();
  } else {
    output_verbose("Using previous start_time: %.2f", global_starttime);
  }
  if (stop_time.has_value()) {
    output_verbose("Setting stop_time: %.2f", stop_time.value());
    global_stoptime = stop_time.value();
  } else {
    output_verbose("Using previous stop_time: %.2f", global_stoptime);
  }
  global_clock = global_starttime;
}
// Load a GLM file
GLDErrorCode GridLabD::setup_before_load() {

  /* set the default timezone */
  timestamp_set_tz(nullptr);

  exec_clock();         /* initialize the wall clock */
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
GLDErrorCode GridLabD::exit_gld(const std::string &filepath) {
  glm_file_path = filepath;
  output_verbose("Exit GLD: %s", filepath.c_str());
  /* do legal stuff */
#ifdef LEGAL_NOTICE
  if (strcmp(global_pidfile, "") == 0 && legal_notice() == FAILED)
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

  // report_performance_after_run(started_at, passes, tsteps);

  /* compute elapsed runtime */
  output_verbose("elapsed runtime %d seconds", realtime_runtime());
  output_verbose("exit code %d", exec_getexitcode());
  exit(exec_getexitcode());
  return GLD_SUCCESS;
}

// Retrieve GLM data based on a query, optionally save to filepath
nlohmann::ordered_json GridLabD::get_checkpoint_json(const std::string& filename) {
    nlohmann::ordered_json checkpoint;
    
    // If no filepath provided
    checkpoint = do_checkpoint(filename.c_str()); // Use provided directory or default if empty

    // Set the internal gld_model representation to be equal to checkpoint
    gld_model = nlohmann::ordered_json(checkpoint);
    
    return checkpoint;
}

// Set the GLM model with provided data
GLDErrorCode GridLabD::set_glm_data(const GLDData &data) {
  output_verbose("Setting GLM data with %zu fields.", data.size());
  return GLD_SUCCESS;
}

// Save simulation checkpoint
GLDErrorCode GridLabD::save_checkpoint(const std::string &save_path,
                                       GLDCheckPointMode mode) {
  output_verbose("Saving checkpoint to %s with mode %d", save_path.c_str(),
                 static_cast<int>(mode));
  nlohmann::json checkpoint =
      do_checkpoint(save_path.c_str()); // Use provided directory

  // Set the internal gld_model representation to be equal to checkpoint
  gld_model = nlohmann::json(checkpoint);
  return GLD_SUCCESS;
}

// Load simulation checkpoint
// Add an object
GLDErrorCode GridLabD::add_object(GLDData &object_data) {
  output_verbose("Adding object with %zu fields.", object_data.size());
  return GLD_SUCCESS;
}

// Delete an object
GLDErrorCode GridLabD::delete_object(const std::string &name) {
  output_verbose("Deleting object named: %s", name.c_str());
  return GLD_SUCCESS;
}

// Edit an object
GLDErrorCode GridLabD::edit_object(const std::string &name,
                                   const GLDData &updated_data) {
  output_verbose("Editing object: %s with %zu fields.", name.c_str(),
                 updated_data.size());
  return GLD_SUCCESS;
}

// Common helper to check environment and handle failures
GLDErrorCode check_environment_and_handle_failure() {
  if (strcmp(global_environment, "batch") != 0) {
    output_fatal("%s environment not recognized or supported",
                 global_environment);
    /*	TROUBLESHOOT
        The environment specified isn't supported. Currently only
        the <b>batch</b> environment is normally supported, although
        some builds can support other environments, such as <b>matlab</b>.
    */
    FILE *f = fopen("/tmp/gld_debug.log", "a");
    if (f) {
      fprintf(f,
              "DEBUG: Failed in check_environment_and_handle_failure due to "
              "unsupported environment: %s\n",
              global_environment);
      fclose(f);
    }
    return GLD_FAILED_TO_START;
  }
  FILE *f2 = fopen("/tmp/gld_debug.log", "a");
  if (f2) {
    fprintf(f2,
            "DEBUG: Succeeded in check_environment_and_handle_failure. "
            "Environment: %s\n",
            global_environment);
    fclose(f2);
  }
  return GLD_SUCCESS;
}

// Common helper to handle simulation failure with optional dump
GLDErrorCode handle_simulation_failure(const char *context_message) {
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
    output_verbose("Simulation not initialized, attempting to initialize...");

    GLDErrorCode env_check = check_environment_and_handle_failure();
    if (env_check != GLD_SUCCESS) {
      return env_check;
    }

    if (run_preparation() == FAILED) {
      output_error("Failed to initialize simulation for stepping");
      return GLD_OPERATION_FAILED;
    }

    output_verbose("Simulation initialized successfully");
  }
  return GLD_SUCCESS;
}

// Run simulation from start to end
GLDErrorCode GridLabD::run(std::optional<double> start_time,
                           std::optional<double> stop_time) {
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
GLDErrorCode GridLabD::step(double &simulation_time) {
  if(global_clock >= global_stoptime || global_clock == TS_NEVER) {
    output_verbose("Simulation has already reached or exceeded stoptime (%.2f >= %.2f), no stepping performed",
                   (double)global_clock, (double)global_stoptime);
    simulation_time = (double)global_clock;
    return GLD_SUCCESS;
  }

  output_verbose("Stepping simulation forward");

  // Ensure simulation is initialized
  GLDErrorCode init_result = ensure_simulation_initialized();
  if (init_result != GLD_SUCCESS) {
    simulation_time = (double)global_clock;
    return init_result;
  }

  // If selected_timestep is 0, use default event-driven behavior (single step)
  if (selected_timestep == 0) {
    output_verbose(
        "Using default event-driven stepping (selected_timestep = 0)");
    TIMESTAMP prev_clock = global_clock;

    STATUS result = exec_step();

    if (result == FAILED) {
      output_error("Error occurred during simulation step");
      simulation_time = (double)global_clock;
      return GLD_OPERATION_FAILED;
    }

    simulation_time = (double)global_clock;
    output_verbose("Stepped from %.2f to %.2f (advanced to next event)",
                   (double)prev_clock, simulation_time);
    return GLD_SUCCESS;
  }

  // Otherwise, use the selected_timestep to advance by fixed duration
  TIMESTAMP start_clock = global_clock;
  TIMESTAMP target_clock = start_clock + selected_timestep;

  // Respect global stoptime when using fixed timesteps
  if (global_stoptime != TS_NEVER && target_clock > global_stoptime) {
    target_clock = global_stoptime;
  }

  output_verbose(
      "Stepping from time %.2f to target %.2f (step size: %d seconds)",
      (double)start_clock, (double)target_clock, selected_timestep);

  // Cap the next event time so exec_step doesn't overshoot the target.
  global_step_time = target_clock;

  // Keep stepping until we reach the target time
  while (global_clock < target_clock) {
    TIMESTAMP prev_clock = global_clock;

    // Execute a single simulation step
    STATUS result = exec_step();

    if (result == FAILED) {
      output_error("Error occurred during simulation step");
      simulation_time = (double)global_clock;
      global_step_time = TS_NEVER;
      return GLD_OPERATION_FAILED;
    }

    //         printf("  Internal step: %.2f -> %.2f\n", (double)prev_clock,
    //         (double)global_clock);

    // Check if we've reached or passed the target
    if (global_clock >= target_clock) {
      break;
    }
  }

  if (global_clock > target_clock) {
    // Clamp to the exact target time to avoid overshoot.
    if (exec_force_sync_to_time(target_clock) == FAILED) {
      output_error("Failed to force sync to target time");
      simulation_time = (double)global_clock;
      global_step_time = TS_NEVER;
      return GLD_OPERATION_FAILED;
    }
  }

  global_step_time = TS_NEVER;

  // Update the simulation time
  simulation_time = (double)global_clock;

  output_verbose("Completed step: advanced from %.2f to %.2f",
                 (double)start_clock, simulation_time);

  return GLD_SUCCESS;
}

// Set pre-step callback
GLDErrorCode GridLabD::set_prestep_callback(GLDCallback callback) {
  output_verbose("Setting pre-step callback");
  return GLD_SUCCESS;
}

// Set post-step callback
GLDErrorCode GridLabD::set_poststep_callback(GLDCallback callback) {
  output_verbose("Setting post-step callback");
  return GLD_SUCCESS;
}

// Reset timestep
GLDErrorCode GridLabD::reset_step(double &current_time) {
  output_verbose("Resetting simulation step");
  current_time = 0.0;
  return GLD_SUCCESS;
}

// Set time manually
GLDErrorCode GridLabD::set_time(const std::string &timestamp) {
  output_verbose("Setting time to: %s", timestamp.c_str());
  return GLD_SUCCESS;
}

// Get current simulation time
GLDErrorCode GridLabD::get_time(std::string &current_time) {
  char buffer[64];
  if (convert_from_timestamp(global_clock, buffer, sizeof(buffer)) > 0) {
    current_time = buffer;
    output_verbose("Getting current time: %s", current_time.c_str());
    return GLD_SUCCESS;
  } else {
    output_error("Failed to convert timestamp");
    return GLD_OPERATION_FAILED;
  }
}

// Set application mode
GLDErrorCode GridLabD::set_application_mode(GLDApplicationType mode) {
  output_verbose("Setting application mode: %d", static_cast<int>(mode));
  return GLD_SUCCESS;
}

// Set timestep
GLDErrorCode GridLabD::set_time_step(double time_step) {
  if (time_step <= 0) {
    output_error("Time step must be positive, got: %.2f", time_step);
    return GLD_OPERATION_FAILED;
  }

  // Store the user-selected timestep
  // This is separate from global_minimum_timestep to avoid interfering with
  // GridLAB-D's core behavior
  selected_timestep = static_cast<int>(time_step);

  output_verbose("Setting API timestep to: %d seconds (global_minimum_timestep "
                 "unchanged at %d)",
                 selected_timestep, global_minimum_timestep);
  return GLD_SUCCESS;
}

// Step the simulation to a specific target timestamp
GLDErrorCode GridLabD::step_to(const std::string &target_time_str,
                               double &simulation_time) {
  output_verbose("Stepping to target time: %s", target_time_str.c_str());

  // Ensure simulation is initialized
  GLDErrorCode init_result = ensure_simulation_initialized();
  if (init_result != GLD_SUCCESS) {
    simulation_time = (double)global_clock;
    return init_result;
  }

  // Convert the ISO 8601 string to a TIMESTAMP with sub-second support
  unsigned int target_nanoseconds = 0;
  double target_time_dbl = 0.0;
  TIMESTAMP target_clock = convert_to_timestamp_delta(
      target_time_str.c_str(), &target_nanoseconds, &target_time_dbl);

  output_verbose("Formatted target time: %lld", target_clock);

  if (target_clock == TS_INVALID) {
    output_error("Invalid timestamp string: %s", target_time_str.c_str());
    simulation_time = (double)global_clock;
    return GLD_OPERATION_FAILED;
  }

  TIMESTAMP start_clock = global_clock;

  // Check if target is in the past (compare as doubles for sub-second
  // precision)
  double current_time_dbl =
      (double)global_clock + (double)global_api_clock_nanoseconds / 1e9;
  if (target_time_dbl <= current_time_dbl) {
    char start_buffer[64];
    convert_from_timestamp(start_clock, start_buffer, sizeof(start_buffer));
    output_warning("Target time %s is not after current time %s",
                   target_time_str.c_str(), start_buffer);
    simulation_time = current_time_dbl;
    global_step_time = TS_NEVER;
    return GLD_SUCCESS;
  }

  // Cap the next event time so exec_step doesn't overshoot the target.
  global_step_time = target_clock;

  char start_buffer[64];
  convert_from_timestamp(start_clock, start_buffer, sizeof(start_buffer));
  output_verbose("Stepping from %s to target %s", start_buffer,
                 target_time_str.c_str());

  // Keep stepping until we reach or pass the target time (with sub-second
  // precision)
  while (true) {
    current_time_dbl =
        (double)global_clock + (double)global_api_clock_nanoseconds / 1e9;

    if (current_time_dbl >= target_time_dbl) {
      break;
    }

    // Execute a single simulation step
    STATUS result = exec_step();

    if (result == FAILED) {
      output_error("Error occurred during simulation step");
      simulation_time = current_time_dbl;
      global_step_time = TS_NEVER;
      return GLD_OPERATION_FAILED;
    }
  }

  simulation_time =
      (double)global_clock + (double)global_api_clock_nanoseconds / 1e9;

  if (global_clock > target_clock) {
    // Clamp to the exact target time to avoid overshoot.
    if (exec_force_sync_to_time(target_clock) == FAILED) {
      output_error("Failed to force sync to target time");
      global_step_time = TS_NEVER;
      return GLD_OPERATION_FAILED;
    }
    global_api_clock_nanoseconds = target_nanoseconds;
    simulation_time =
        (double)global_clock + (double)global_api_clock_nanoseconds / 1e9;
  }

  char final_buffer[64];
  convert_from_timestamp(global_clock, final_buffer, sizeof(final_buffer));
  output_verbose("Reached time %s (target was %s)", final_buffer,
                 target_time_str.c_str());

  global_step_time = TS_NEVER;

  return GLD_SUCCESS;
}

// Get all objects of a specific class
std::vector<std::string>
GridLabD::get_objects_by_class(const std::string &class_name) {
  std::vector<std::string> object_names;

  // Find the class by name
  CLASS *oclass = class_get_class_from_classname(class_name.c_str());
  if (oclass == nullptr) {
    output_warning("Class '%s' not found", class_name.c_str());
    return object_names;
  }

  // Iterate through all objects
  OBJECT *obj = object_get_first();
  while (obj != nullptr) {
    // Check if object belongs to the requested class
    if (obj->oclass == oclass) {
      // Get object name (use ID if name is empty)
      if (obj->name != nullptr && obj->name[0] != '\0') {
        object_names.push_back(std::string(obj->name));
      } else {
        // Use object ID if no name
        char id_str[32];
        snprintf(id_str, sizeof(id_str), "%d", obj->id);
        object_names.push_back(std::string(id_str));
      }
    }
    obj = object_get_next(obj);
  }

  output_verbose("Found %zu objects of class '%s'", object_names.size(),
                 class_name.c_str());
  return object_names;
}

// Get a property value from an object
GLDErrorCode GridLabD::get_property(const std::string &object_name,
                                    const std::string &property_name,
                                    std::string &value) {
  // Find the object by name
  OBJECT *obj = nullptr;

  // Try to find by name first
  obj = object_find_name(object_name.c_str());

  // If not found by name, try to parse as ID
  if (obj == nullptr) {
    char *endptr;
    long id = strtol(object_name.c_str(), &endptr, 10);
    if (*endptr == '\0') { // Valid integer
      obj = object_find_by_id(static_cast<OBJECTNUM>(id));
    }
  }

  if (obj == nullptr) {
    output_error("Object '%s' not found", object_name.c_str());
    return GLD_OPERATION_FAILED;
  }

  // Get the property value
  char buffer[1024];
  int result = object_get_value_by_name(obj, property_name.c_str(), buffer,
                                        sizeof(buffer));

  if (result == 0) {
    output_error("Failed to get property '%s' from object '%s'",
                 property_name.c_str(), object_name.c_str());
    return GLD_OPERATION_FAILED;
  }

  value = std::string(buffer);
  return GLD_SUCCESS;
}

// Get property metadata (type, units, description)
GLDErrorCode GridLabD::get_property_info(const std::string &object_name,
                                         const std::string &property_name,
                                         int &prop_type, std::string &unit_str,
                                         std::string &description) {
  // Find the object by name
  OBJECT *obj = nullptr;

  // Try to find by name first
  obj = object_find_name(object_name.c_str());

  // If not found by name, try to parse as ID
  if (obj == nullptr) {
    char *endptr;
    long id = strtol(object_name.c_str(), &endptr, 10);
    if (*endptr == '\0') { // Valid integer
      obj = object_find_by_id(static_cast<OBJECTNUM>(id));
    }
  }

  if (obj == nullptr) {
    output_error("Object '%s' not found", object_name.c_str());
    return GLD_OPERATION_FAILED;
  }

  // Find the property
  PROPERTY *prop = object_get_property(obj, property_name.c_str(), nullptr);

  if (prop == nullptr) {
    output_error("Property '%s' not found on object '%s'",
                 property_name.c_str(), object_name.c_str());
    return GLD_OPERATION_FAILED;
  }

  // Get property type
  prop_type = static_cast<int>(prop->ptype);

  // Get unit string
  if (prop->unit != nullptr && prop->unit->name != nullptr) {
    unit_str = std::string(prop->unit->name);
  } else {
    unit_str = "";
  }

  // Get description
  if (prop->description != nullptr) {
    description = std::string(prop->description);
  } else {
    description = "";
  }

  return GLD_SUCCESS;
}

// Set a property value on an object
GLDErrorCode GridLabD::set_property(const std::string &object_name,
                                    const std::string &property_name,
                                    const std::string &value) {
  // Find the object by name
  OBJECT *obj = nullptr;

  // Try to find by name first
  obj = object_find_name(object_name.c_str());

  // If not found by name, try to parse as ID
  if (obj == nullptr) {
    char *endptr;
    long id = strtol(object_name.c_str(), &endptr, 10);
    if (*endptr == '\0') { // Valid integer
      obj = object_find_by_id(static_cast<OBJECTNUM>(id));
    }
  }

  if (obj == nullptr) {
    output_error("Object '%s' not found", object_name.c_str());
    return GLD_OPERATION_FAILED;
  }

  // Set the property value
  // Set the property value
  char value_copy[1024];
  strncpy(value_copy, value.c_str(), sizeof(value_copy) - 1);
  value_copy[sizeof(value_copy) - 1] = '\0';

  int result = object_set_value_by_name(
      obj, const_cast<char *>(property_name.c_str()), value_copy);

  if (result == 0) {
    output_error("Failed to set property '%s' on object '%s' to value '%s'",
                 property_name.c_str(), object_name.c_str(), value.c_str());
    return GLD_OPERATION_FAILED;
  }

  output_verbose("Set property '%s.%s' = '%s'", object_name.c_str(),
                 property_name.c_str(), value.c_str());
  return GLD_SUCCESS;
}

void *GridLabD::find_object_by_name(const std::string &object_name) {
  if (object_name.empty()) {
    output_error("Object name cannot be empty");
    return nullptr;
  }

  OBJECT *obj = object_find_name(object_name.c_str());
  if (obj != nullptr) {
    return static_cast<void *>(obj);
  }

  char *endptr = nullptr;
  long id = strtol(object_name.c_str(), &endptr, 10);
  if (endptr != nullptr && *endptr == '\0') {
    obj = object_find_by_id(static_cast<OBJECTNUM>(id));
    if (obj != nullptr) {
      return static_cast<void *>(obj);
    }
  }

  output_error("Object '%s' not found", object_name.c_str());
  return nullptr;
}

GLDErrorCode GridLabD::get_property_value(void *object_ptr,
                                         const std::string &property_name,
                                         std::string &value) {
  if (object_ptr == nullptr) {
    output_error("Null object pointer");
    return GLD_OBJECT_NOT_FOUND;
  }
  if (property_name.empty()) {
    output_error("Property name cannot be empty");
    return GLD_OPERATION_FAILED;
  }

  OBJECT *obj = static_cast<OBJECT *>(object_ptr);

  char buffer[1024];
  int result = object_get_value_by_name(obj, property_name.c_str(), buffer,
                                        sizeof(buffer));
  if (result == 0) {
    output_error("Failed to get property '%s' from object", property_name.c_str());
    return GLD_OPERATION_FAILED;
  }

  value = std::string(buffer);
  return GLD_SUCCESS;
}

GLDErrorCode GridLabD::set_property_value(void *object_ptr,
                                         const std::string &property_name,
                                         const std::string &value) {
  if (object_ptr == nullptr) {
    output_error("Null object pointer");
    return GLD_OBJECT_NOT_FOUND;
  }
  if (property_name.empty()) {
    output_error("Property name cannot be empty");
    return GLD_OPERATION_FAILED;
  }

  OBJECT *obj = static_cast<OBJECT *>(object_ptr);

  char value_copy[1024];
  strncpy(value_copy, value.c_str(), sizeof(value_copy) - 1);
  value_copy[sizeof(value_copy) - 1] = '\0';

  int result = object_set_value_by_name(
      obj, const_cast<char *>(property_name.c_str()), value_copy);
  if (result == 0) {
    output_error("Failed to set property '%s' on object to '%s'",
                 property_name.c_str(), value.c_str());
    return GLD_OPERATION_FAILED;
  }

  return GLD_SUCCESS;
}

// Set a property value on all objects of a specific class
GLDErrorCode GridLabD::set_property_by_class(const std::string &class_name,
                                             const std::string &property_name,
                                             const std::string &value) {
  // Find the class by name
  CLASS *oclass = class_get_class_from_classname(class_name.c_str());
  if (oclass == nullptr) {
    output_error("Class '%s' not found", class_name.c_str());
    return GLD_OPERATION_FAILED;
  }

  // Prepare value buffer
  // Prepare value buffer
  char value_copy[1024];
  strncpy(value_copy, value.c_str(), sizeof(value_copy) - 1);
  value_copy[sizeof(value_copy) - 1] = '\0';

  int success_count = 0;
  int failure_count = 0;

  // Iterate through all objects and set property for matching class
  OBJECT *obj = object_get_first();
  while (obj != nullptr) {
    if (obj->oclass == oclass) {
      int result = object_set_value_by_name(
          obj, const_cast<char *>(property_name.c_str()), value_copy);
      if (result != 0) {
        success_count++;
      } else {
        failure_count++;
        const char *obj_name = (obj->name != nullptr && obj->name[0] != '\0')
                                   ? obj->name
                                   : "(unnamed)";
        output_warning("Failed to set property '%s' on object '%s' (id=%d)",
                       property_name.c_str(), obj_name, obj->id);
      }
    }
    obj = object_get_next(obj);
  }

  output_verbose(
      "Set property '%s.%s' = '%s' on %d objects (%d succeeded, %d failed)",
      class_name.c_str(), property_name.c_str(), value.c_str(),
      success_count + failure_count, success_count, failure_count);

  if (success_count == 0 && failure_count > 0) {
    return GLD_OPERATION_FAILED;
  }

  return GLD_SUCCESS;
}

// Get property values from all objects of a specific class
std::map<std::string, std::string>
GridLabD::get_properties_by_class(const std::string &class_name,
                                  const std::string &property_name) {
  std::map<std::string, std::string> property_map;

  // Find the class by name
  CLASS *oclass = class_get_class_from_classname(class_name.c_str());
  if (oclass == nullptr) {
    output_warning("Class '%s' not found", class_name.c_str());
    return property_map;
  }

  // Iterate through all objects
  OBJECT *obj = object_get_first();
  while (obj != nullptr) {
    if (obj->oclass == oclass) {
      // Get object name or ID
      std::string obj_name;
      if (obj->name != nullptr && obj->name[0] != '\0') {
        obj_name = std::string(obj->name);
      } else {
        char id_str[32];
        snprintf(id_str, sizeof(id_str), "%d", obj->id);
        obj_name = std::string(id_str);
      }

      // Get property value
      char buffer[1024];
      int result = object_get_value_by_name(obj, property_name.c_str(), buffer,
                                            sizeof(buffer));

      if (result != 0) {
        property_map[obj_name] = std::string(buffer);
      } else {
        output_warning("Failed to get property '%s' from object '%s'",
                       property_name.c_str(), obj_name.c_str());
      }
    }
    obj = object_get_next(obj);
  }

  output_verbose("Retrieved property '%s.%s' from %zu objects",
                 class_name.c_str(), property_name.c_str(),
                 property_map.size());

  return property_map;
}

// Get all available class names
std::vector<std::string> GridLabD::get_all_classes() {
  std::vector<std::string> class_names;

  // Iterate through all classes
  CLASS *oclass = class_get_first_class();
  while (oclass != nullptr) {
    if (oclass->name != nullptr) {
      class_names.push_back(std::string(oclass->name));
    }
    oclass = oclass->next;
  }

  output_verbose("Found %zu classes in the model", class_names.size());
  return class_names;
}

// Get all properties of a specific object
std::map<std::string, std::string>
GridLabD::get_object_properties(const std::string &object_name) {
  std::map<std::string, std::string> property_map;

  // Find the object by name
  OBJECT *obj = nullptr;

  // Try to find by name first
  obj = object_find_name(object_name.c_str());

  // If not found by name, try to parse as ID
  if (obj == nullptr) {
    char *endptr;
    long id = strtol(object_name.c_str(), &endptr, 10);
    if (*endptr == '\0') { // Valid integer
      obj = object_find_by_id(static_cast<OBJECTNUM>(id));
    }
  }

  if (obj == nullptr) {
    output_error("Object '%s' not found", object_name.c_str());
    return property_map;
  }

  property_map = collect_property_map_for_object(obj);

  size_t meta_fields = 0;
  meta_fields += property_map.count("__class__");
  meta_fields += property_map.count("__id__");
  meta_fields += property_map.count("__name__");
  size_t reported_properties = property_map.size() >= meta_fields
                                   ? property_map.size() - meta_fields
                                   : 0;

  output_verbose("Retrieved %zu properties from object '%s' (class: %s)",
                 reported_properties, object_name.c_str(), obj->oclass->name);

  return property_map;
}

// Get all objects of a specific class with their properties
std::vector<std::map<std::string, std::string>>
GridLabD::get_all_objects(const std::string &class_name) {
  std::vector<std::map<std::string, std::string>> objects;

  CLASS *oclass = class_get_class_from_classname(class_name.c_str());
  if (oclass == nullptr) {
    output_warning("Class '%s' not found", class_name.c_str());
    return objects;
  }

  OBJECT *obj = object_get_first();
  while (obj != nullptr) {
    if (obj->oclass == oclass) {
      auto property_map = collect_property_map_for_object(obj);
      if (!property_map.empty()) {
        objects.push_back(std::move(property_map));
      }
    }
    obj = object_get_next(obj);
  }

  output_verbose("Collected %zu objects for class '%s'", objects.size(),
                 class_name.c_str());
  return objects;
}

// Get the entire model with all objects and properties organized by class
std::map<std::string, std::vector<std::map<std::string, std::string>>>
GridLabD::get_model() {
  std::map<std::string, std::vector<std::map<std::string, std::string>>> model;

  // Get all classes
  std::vector<std::string> classes = get_all_classes();

  // For each class, get all objects
  size_t total_objects = 0;
  for (const auto &class_name : classes) {
    auto objects = get_all_objects(class_name);
    if (!objects.empty()) {
      model[class_name] = std::move(objects);
      total_objects += model[class_name].size();
    }
  }

  output_verbose("Retrieved entire model: %zu classes with %zu total objects",
                 model.size(), total_objects);

  return model;
}

std::vector<std::map<std::string, std::string>> GridLabD::get_messages() {
  return output_get_captured_messages();
}

void GridLabD::clear_messages() { output_clear_captured_messages(); }

void GridLabD::enable_message_capture(bool enable) {
  output_enable_capture(enable);
}

void GridLabD::set_message_capture_limit(size_t limit) {
  output_set_message_capture_limit(limit);
}

size_t GridLabD::get_message_capture_limit() {
  return output_get_message_capture_limit();
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

