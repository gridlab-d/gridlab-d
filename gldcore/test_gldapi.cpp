#include "gldapi.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include "globals.h"
#include <json/json.h> //jsoncpp library

// Safe value extraction with type conversion
template<typename T>
T safeGetValue(const Json::Value& value, const T& defaultValue) {
    if (value.isNull()) return defaultValue;
    
    try {
        if constexpr (std::is_same_v<T, std::string>) {
            return value.isString() ? value.asString() : defaultValue;
        } else if constexpr (std::is_same_v<T, double>) {
            return value.isNumeric() ? value.asDouble() : defaultValue;
        } else if constexpr (std::is_same_v<T, int>) {
            return value.isInt() ? value.asInt() : defaultValue;
        } else if constexpr (std::is_same_v<T, long long> || std::is_same_v<T, int64_t>) {
            return value.isInt64() ? value.asInt64() : defaultValue;
        } else if constexpr (std::is_same_v<T, bool>) {
            return value.isBool() ? value.asBool() : defaultValue;
        }
    } catch (const std::exception&) {
        // Return default on any conversion error
    }
    
    return defaultValue;
}

int main(int argc, char* argv[]) {
    const char* fileName = "test_HVAC_balance.glm";
    GridLabD gld;

    // Test set_config_file
    // gld.set_config_file("config.cfg");
    
    // Test load_glm
    std::vector<const char*> args = {"gridlabd", fileName, "--verbose"};
    int test_argc = static_cast<int>(args.size());
    char* test_argv[] = { const_cast<char*>(args[0]), const_cast<char*>(args[1]), const_cast<char*>(args[2])};
    gld.load_glm(test_argc, test_argv);


    TIMESTAMP start_time = convert_to_timestamp("2000-04-01 0:00:00");
    TIMESTAMP stop_time = convert_to_timestamp("2000-06-01 0:00:00");
    
    // gld.set_time_step(900); // 15 minutes in seconds
    // Test run examples
    gld.run(start_time, stop_time);
    // gld.run();

    // Stepping through the simulation examples, check sim_time for each step if needed. 
    // double sim_time;
    // for (int i = 0; i < 6; i++) {
    //     gld.step(sim_time);
    //     std::cout << "Simulation time after step " << (i+1) << ": " << sim_time << std::endl;
    // }

    
    // Get all info for GLD
    Json::Value checkpoint = gld.get_checkpoint_json("/mnt/c/dev/gridlab-d_fork/_test_results/");
    
    std::cout << "\n=== Example Property Access ===" << std::endl;
    auto air_temp = checkpoint["objects"]["house"]["instances"][0]["air_temperature"];
    std::cout << "House air temp: " << air_temp << std::endl;

    // Test exit_gld
    gld.exit_gld(fileName);

    return 0;
}
