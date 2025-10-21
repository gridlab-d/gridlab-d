#include "gldapi.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <json/json.h> //jsoncpp library
#include "timestamp.h"

int main(int argc, char* argv[]) {
    // const char* fileName = "test_balanced_stepup_D-D_phAB";
    const char* fileName = "test_HVAC_balance";
    // Instantiate GridLabD via exectuable path
    GridLabD gld;
    
    // Test load_glm
    std::vector<const char*> args = {"gridlabd", fileName, "--verbose"};
    int test_argc = static_cast<int>(args.size());
    char* test_argv[] = { const_cast<char*>(args[0]), const_cast<char*>(args[1]), const_cast<char*>(args[2])};
    gld.load_glm(test_argc, test_argv);

    TIMESTAMP start_time = convert_to_timestamp("2000-04-01 0:00:00");
    TIMESTAMP stop_time = convert_to_timestamp("2000-06-01 0:00:00");
    
    // Test run examples
    // gld.run();
    // gld.run(start_time, stop_time);

    // Stepping through the simulation examples, check sim_time for each step if needed. 
    // gld.set_time_step(900); // 15 minutes in seconds
    // Need to support actually setting the timestep value, not just the minimum. Recorders do this. Tell it what the synchronization time is. 
    double sim_time;
    void* house = gld.find_object_by_name("This_old_house");
    for (int i = 0; i < 10; i++) {
        printf("\n=== Step %d ===\n", i+1);
        
        // Check current value before setting
        std::string current_setpoint;
        if (gld.get_property_value(house, "number_of_doors", current_setpoint) == GLD_SUCCESS) {
            printf("Current number_of_doors before set: %s\n", current_setpoint.c_str());
        }
        
        // Set new value
        GLDErrorCode set_result = gld.set_property_value(house, "number_of_doors", std::to_string(i).c_str());

        gld.step(sim_time);
        
        Json::Value checkpoint = gld.get_checkpoint_json("/mnt/c/dev/gridlab-d_fork/_test_results/");
    }
    
    // Get all info for GLD
    Json::Value checkpoint = gld.get_checkpoint_json("/mnt/c/dev/gridlab-d_fork/_test_results/");
    // OR access it directly
    // gld.get_checkpoint_json();
    // Json::Value checkpoint = gld.gld_model;

    // Test exit_gld
    gld.exit_gld(fileName);

    return 0;
}
