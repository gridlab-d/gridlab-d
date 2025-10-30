#ifndef GLD_API_H
#define GLD_API_H

#include <string>
#include <map>
#include <any>
#include <memory>
#include <vector>
#include <optional>
#include <json/json.h> //jsoncpp library

// typedefs for GLD data types
typedef std::map<std::string, std::any> GLDData;


enum GLDErrorCode {
    GLD_SUCCESS = 0,
    GLD_FILE_NOT_FOUND = 1,
    GLD_INVALID_FORMAT = 2,
    GLD_OPERATION_FAILED = 3,
    GLD_OBJECT_NOT_FOUND = 4,
    GLD_TIME_STEP_ERROR = 5,
    GLD_FAILED_TO_START = 6
};

enum GLDApplicationType {
    GLD_APPLICATION_TYPE_UNKNOWN = 0,
    GLD_APPLICATION_TYPE_GRIDLABD = 1,
    GLD_APPLICATION_TYPE_OTHER = 2
};

enum GLDCheckPointMode {
    GLD_CHECKPOINT_MODE_NONE = 0,
    GLD_CHECKPOINT_MODE_SAVE = 1,
    GLD_CHECKPOINT_MODE_LOAD = 2
};

// Forward declaration of GridLabD class
class GridLabD;

// typedef for Callback function type
typedef GLDErrorCode (*GLDCallback)(GridLabD* gld);

class GridLabD {
public:
     // Default constructor
    GridLabD();

    ~GridLabD() {
        // Cleanup code goes here
    }

    // Explicitly set the GridLAB-D installation root (directory or executable)
    static void set_install_root(const std::string& install_root);

    // Retrieve the resolved installation root directory
    static std::string get_install_root();

    // Retrieve the resolved GridLAB-D executable path
    static std::string get_executable_path();

    // Set the configuration file path
    GLDErrorCode set_config_file(const std::string& config_file);

    // Load a GLM and return an error code
    GLDErrorCode load_glm(int argc, char* argv[]);

    // Load a GLM from a file path
    GLDErrorCode load_glm(const std::string& filepath);

    // Setup GLD and return an error code
    //GLDErrorCode setup_before_load(const std::string& filepath) ;
    GLDErrorCode setup_before_load();

    // Setup GLD and return an error code
    GLDErrorCode setup_after_load() ;

    // Get the GLM data based on a query, optionally save to filepath
    Json::Value get_checkpoint_json(const std::string& filepath = "");

    // Set the GLM based on input data
    GLDErrorCode set_glm_data(const GLDData& data);

    // Save simulation state
    GLDErrorCode save_checkpoint(const std::string& save_path, GLDCheckPointMode mode = GLD_CHECKPOINT_MODE_SAVE);

    // Load simulation state
    GLDErrorCode load_checkpoint(const std::string& file_path);

    // Add a new object to the model
    GLDErrorCode add_object(GLDData& object_data);

    // Delete an object from the model
    GLDErrorCode delete_object(const std::string& name);

    // Edit an object in the model
    GLDErrorCode edit_object(const std::string& name, const GLDData& updated_data);

    // Run the simulation for a specified time range (optional). If not provided, previous values are used.
    GLDErrorCode run(std::optional<double> start_time = std::nullopt, std::optional<double> stop_time = std::nullopt);

    // Run the simulation by one time step and return the simulation time
    GLDErrorCode step(double& simulation_time);

    // Set prestep callback function
    GLDErrorCode set_prestep_callback(GLDCallback callback);

    // Set poststep callback function
    GLDErrorCode set_poststep_callback(GLDCallback callback);

    // Reset the simulation time step and returns the current time
    GLDErrorCode reset_step(double& current_time);

    // Set the simulation time manually (use with caution)
    GLDErrorCode set_time(const std::string& timestamp);

    // Get the current simulation time
    GLDErrorCode get_time(std::string& current_time);

    // Set the application mode (e.g., POWERFLOW, TIMESERIES, VVO)
    GLDErrorCode set_application_mode(GLDApplicationType mode); //Not needed anymore

    // Set the time step for the simulation
    GLDErrorCode set_time_step(double time_step);

    //Exit simulation
    GLDErrorCode exit_gld(const std::string& filepath);

    private:
        std::string glm_file_path;  // Path to the GLM file
};

#endif // gldapi.hpp
