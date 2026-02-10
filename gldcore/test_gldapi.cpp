#include "gldapi.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <nlohmann/json.hpp>
#include "timestamp.h"

// Helper function to get filename without path and without extension
std::string get_base_filename(const std::string& filepath) {
    // Get filename without path
    size_t pos = filepath.find_last_of("/\\");
    std::string filename = (pos == std::string::npos) ? filepath : filepath.substr(pos + 1);
    
    // Remove extension
    size_t dot = filename.find_last_of('.');
    if (dot != std::string::npos) {
        filename = filename.substr(0, dot);
    }
    return filename;
}

int main(int argc, char* argv[]) {
    std::string fileName = argv[1];
    GridLabD gld;
    
    // Parse flags
    bool checkpoint_mode = false;
    bool restore_mode = false;
    int num_steps = 2;
    
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--checkpoint") {
            checkpoint_mode = true;
        } else if (arg == "--restore") {
            restore_mode = true;
        } else if (arg == "--steps" && i + 1 < argc) {
            num_steps = std::stoi(argv[++i]);
        }
    }
    
    // Load GLM file
    if(restore_mode) {
        fileName = get_base_filename(fileName) + "_checkpoint.json";
    }
    std::vector<const char*> args = {"notneeded", fileName.c_str(), "--verbose"};
    int test_argc = static_cast<int>(args.size());
    char* test_argv[] = { const_cast<char*>(args[0]), const_cast<char*>(args[1]), const_cast<char*>(args[2])};
    gld.load_glm(test_argc, test_argv);
    
    if (checkpoint_mode) {
        // Run N steps and save checkpoint
        double sim_time;
        for (int i = 0; i < num_steps; i++) {
            gld.step(sim_time);
        }
        printf("Completed %d steps. Simulation time: %.2f\n", num_steps, sim_time);
        
        // Get checkpoint and save it
        nlohmann::json checkpoint = gld.get_checkpoint_json();
        printf("Checkpoint saved.\n");
    } 
    else if (restore_mode) {
        printf("Checkpoint loaded.\n");
        gld.run();
    }
    else {
        // Default: just run to completion
        gld.run();
    }
    
    // Exit
    gld.exit_gld(fileName.c_str());
    
    return 0;
}
