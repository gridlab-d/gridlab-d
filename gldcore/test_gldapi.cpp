#include "gldapi.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <nlohmann/json.hpp>
#include "timestamp.h"
#include <filesystem>
#include <chrono>
#include <fstream>
#include <unistd.h>
#include <sys/wait.h>

namespace fs = std::filesystem;

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

/**
 * Run a single test in the current process
 * This function is called by child processes to run one test
 * Returns 0 on success, non-zero on failure
 */
int run_single_test(const std::filesystem::path& test_dir, const std::string& test_name) {
    namespace fs = std::filesystem;
    
    try {
        // Change to test directory
        fs::current_path(test_dir);
        
        // Initialize GridLabD
        GridLabD gld;
        
        // Prepare arguments
        std::string glm_filename = test_name + ".glm";
        std::vector<const char*> args = {"gridlabd", glm_filename.c_str()};
        int test_argc = static_cast<int>(args.size());
        std::vector<char*> test_argv;
        for (const auto& arg : args) {
            test_argv.push_back(const_cast<char*>(arg));
        }
        
        // Load GLM
        if (gld.load_glm(test_argc, test_argv.data()) != GLD_SUCCESS) {
            return 1;
        }
        
        // Run simulation
        if (gld.run() != GLD_SUCCESS) {
            return 1;
        }
        
        return 0;
        
    } catch (...) {
        return 1;
    }
}

/**
 * Run all autotests found in module directories
 * 
 * Searches for test directories under each module's autotest/ folder and runs them
 * using gld.run(). Each test is run with its directory as the CWD.
 * Each test is executed in a separate forked process to avoid global state conflicts.
 * 
 * @param repo_root Path to the GridLAB-D repository root
 * @param modules List of module names to test (empty = test all)
 * @return TestSummary containing results of all tests
 */
TestSummary run_all_autotests(const std::string& repo_root = "/mnt/c/dev/gridlab-d_fork", 
                               const std::vector<std::string>& modules = {}) {
    TestSummary summary;
    
    // List of known module directories
    std::vector<std::string> all_modules = {
        "assert", "climate", "commercial", "generators",
        "market", "mysql", "network", "powerflow", 
        "reliability", "residential", "rest", "tape", "taxonomy_feeders"
    };
    
    // Use provided modules or all modules
    const auto& modules_to_test = modules.empty() ? all_modules : modules;
    
    std::cout << "=============================================================\n";
    std::cout << "         GridLAB-D Autotest Suite Runner\n";
    std::cout << "=============================================================\n\n";
    
    for (const auto& module : modules_to_test) {
        fs::path autotest_dir = fs::path(repo_root) / module / "autotest";
        
        if (!fs::exists(autotest_dir) || !fs::is_directory(autotest_dir)) {
            continue;
        }
        
        std::cout << "Module: " << module << "\n";
        std::cout << std::string(60, '-') << "\n";
        
        // Find all test directories (directories starting with "test_")
        std::vector<fs::path> test_dirs;
        for (const auto& entry : fs::directory_iterator(autotest_dir)) {
            if (entry.is_directory()) {
                std::string dirname = entry.path().filename().string();
                if (dirname.rfind("test_", 0) == 0) {
                    // Check if there's a matching .glm file in the directory
                    fs::path glm_file = entry.path() / (dirname + ".glm");
                    if (fs::exists(glm_file)) {
                        test_dirs.push_back(entry.path());
                    }
                }
            }
        }
        
        // Sort test directories alphabetically
        std::sort(test_dirs.begin(), test_dirs.end());
        
        for (const auto& test_dir : test_dirs) {
            std::string test_name = test_dir.filename().string();
            fs::path glm_file = test_dir / (test_name + ".glm");
            
            TestResult result;
            result.test_path = test_dir.string();
            result.test_name = test_name;
            result.module = module;
            result.success = false;
            
            std::cout << "  Running: " << test_name << " ... " << std::flush;
            
            // Save current directory
            fs::path original_cwd = fs::current_path();
            
            auto start_time = std::chrono::high_resolution_clock::now();
            
            // Fork a child process to run the test
            pid_t pid = fork();
            
            if (pid == -1) {
                // Fork failed
                result.error_message = "Failed to fork process";
                result.success = false;
                summary.failed_tests++;
            } else if (pid == 0) {
                // Child process - run the test and exit
                // Redirect stdout/stderr to /dev/null to suppress verbose output
                freopen("/dev/null", "w", stdout);
                freopen("/dev/null", "w", stderr);
                
                int test_result = run_single_test(test_dir, test_name);
                exit(test_result);
            } else {
                // Parent process - wait for child to complete
                int status;
                waitpid(pid, &status, 0);
                
                // Check if this is an error test (should fail)
                bool is_error_test = test_name.find("_err") != std::string::npos;
                
                if (WIFEXITED(status)) {
                    int exit_code = WEXITSTATUS(status);
                    
                    // For error tests, we expect non-zero exit (failure)
                    // For normal tests, we expect zero exit (success)
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
                    // For error tests, termination by signal might be expected behavior
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
            
            // Restore original directory (parent only)
            fs::current_path(original_cwd);
            
            // Print result
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
    
    return summary;
}

/**
 * Print a detailed summary of test results
 */
void print_test_summary(const TestSummary& summary) {
    std::cout << "=============================================================\n";
    std::cout << "                    TEST SUMMARY\n";
    std::cout << "=============================================================\n\n";
    
    std::cout << "Total Tests:  " << summary.total_tests << "\n";
    std::cout << "Passed:       " << summary.passed_tests << " (" 
              << (summary.total_tests > 0 ? (summary.passed_tests * 100.0 / summary.total_tests) : 0.0) 
              << "%)\n";
    std::cout << "Failed:       " << summary.failed_tests << " (" 
              << (summary.total_tests > 0 ? (summary.failed_tests * 100.0 / summary.total_tests) : 0.0) 
              << "%)\n\n";
    
    if (!summary.results.empty()) {
        // Group by module
        std::map<std::string, std::vector<TestResult>> by_module;
        for (const auto& result : summary.results) {
            by_module[result.module].push_back(result);
        }
        
        std::cout << "Results by Module:\n";
        std::cout << std::string(60, '-') << "\n";
        for (const auto& [module, results] : by_module) {
            int passed = std::count_if(results.begin(), results.end(), 
                                      [](const TestResult& r) { return r.success; });
            std::cout << "  " << module << ": " << passed << "/" << results.size() << " passed\n";
        }
        std::cout << "\n";
    }
    
    // List failed tests
    if (summary.failed_tests > 0) {
        std::cout << "Failed Tests:\n";
        std::cout << std::string(60, '-') << "\n";
        for (const auto& result : summary.results) {
            if (!result.success) {
                std::cout << "  ✗ " << result.module << "/" << result.test_name << "\n";
                std::cout << "    " << result.error_message << "\n";
            }
        }
        std::cout << "\n";
    }
    
    std::cout << "=============================================================\n";
}

int main(int argc, char* argv[]) {
    // Check if user wants API health check
    if (argc > 1 && std::string(argv[1]) == "--validate-api") {
        GridLabD gld;
        GLDErrorCode result = gld.validate_api(true);
        return (result == GLD_SUCCESS) ? 0 : 1;
    }
    
    // Check if user wants to run all autotests
    if (argc > 1 && (std::string(argv[1]) == "--validate" || std::string(argv[1]) == "--autotest")) {
        std::string repo_root = "/mnt/c/dev/gridlab-d_fork";
        
        // Check if specific modules were provided
        std::vector<std::string> modules;
        if (argc > 2) {
            for (int i = 2; i < argc; i++) {
                modules.push_back(argv[i]);
            }
        }
        
        // Create GridLabD instance and run validation
        GridLabD gld;
        GLDErrorCode result = gld.validate(repo_root, modules);
        
        // Return exit code based on results
        return (result == GLD_SUCCESS) ? 0 : 1;
    }
    
    // Original single test mode
    if (argc < 2) {
        std::cerr << "Usage:\n";
        std::cerr << "  " << argv[0] << " <test_name>              Run a single test\n";
        std::cerr << "  " << argv[0] << " --validate-api           Run API health check (fast)\n";
        std::cerr << "  " << argv[0] << " --validate               Run all autotests (slow)\n";
        std::cerr << "  " << argv[0] << " --autotest [modules...]  Run autotests for specific modules\n";
        return 1;
    }
    
    // const char* fileName = "test_balanced_stepup_D-D_phAB";
    // const char* fileName = "test_HVAC_balance";
    // Instantiate GridLabD via exectuable path
    GridLabD gld;
    
    // Test load_glm
    std::vector<const char*> args = {"gridlabd", argv[1], "--verbose"};
    int test_argc = static_cast<int>(args.size());
    char* test_argv[] = { const_cast<char*>(args[0]), const_cast<char*>(args[1]), const_cast<char*>(args[2])};
    gld.load_glm(test_argc, test_argv);

    TIMESTAMP start_time = convert_to_timestamp("2000-04-01 0:00:00");
    TIMESTAMP stop_time = convert_to_timestamp("2000-06-01 0:00:00");
    
    // Test run examples
    gld.run();
    // gld.run(start_time, stop_time);

    // Stepping through the simulation examples, check sim_time for each step if needed. 
    // gld.set_time_step(900); // 15 minutes in seconds
    // Need to support actually setting the timestep value, not just the minimum. Recorders do this. Tell it what the synchronization time is. 
    // double sim_time;
    // void* house = gld.find_object_by_name("This_old_house");
    // for (int i = 0; i < 10; i++) {
    //     printf("\n=== Step %d ===\n", i+1);
        
    //     // Check current value before setting
    //     std::string current_setpoint;
    //     if (gld.get_property_value(house, "number_of_doors", current_setpoint) == GLD_SUCCESS) {
    //         printf("Current number_of_doors before set: %s\n", current_setpoint.c_str());
    //     }
        
    //     // Set new value
    //     GLDErrorCode set_result = gld.set_property_value(house, "number_of_doors", std::to_string(i).c_str());

    //     gld.step(sim_time);
        
    // nlohmann::json checkpoint = gld.get_checkpoint_json("/mnt/c/dev/gridlab-d_fork/_test_results/");
    // }
    
    // Get all info for GLD
    nlohmann::ordered_json checkpoint = gld.get_checkpoint_json("/mnt/c/dev/gridlab-d_fork/_test_results/");
    // OR access it directly
    // gld.get_checkpoint_json();
    // nlohmann::json checkpoint = gld.gld_model; // use nlohmann::json (JsonCpp removed)

    // Test exit_gld
    gld.exit_gld(argv[1]);

    return 0;
}
