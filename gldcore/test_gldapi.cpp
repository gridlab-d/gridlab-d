#include "gldapi.h"
#include <iostream>
#include <vector>
#include <string>

int main(int argc, char* argv[]) {
    // Dummy command line arguments
    std::vector<const char*> args = {"gridlabd", "test_commercial_const_air_temp.glm", "--verbose"};
    int test_argc = static_cast<int>(args.size());
    char* test_argv[] = { const_cast<char*>(args[0]), const_cast<char*>(args[1]), const_cast<char*>(args[2])};

    // Instantiate GridLabD
    GridLabD gld(test_argc, test_argv);

    // Test set_config_file
    gld.set_config_file("config.cfg");
    
    // Test load_glm
    gld.load_glm("test_commercial_const_air_temp.glm", test_argc, test_argv);

    gld.setup_after_load();

    // Test run
    double sim_time = 0.0;
    gld.run(0.0, 10.0, sim_time, test_argc, test_argv);

    // Test step
    // gld.step(sim_time);

    // Test get_time
    // std::string current_time;
    // gld.get_time(current_time);
    // std::cout << "Current simulation time: " << current_time << std::endl;

    // Test set_time
    // gld.set_time("2025-06-12T12:00:00");

    // Test exit_gld
    gld.exit_gld("test.glm");

    return 0;
}
