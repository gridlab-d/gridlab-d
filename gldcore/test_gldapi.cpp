#include "gldapi.h"
#include "timestamp.h"
#include <algorithm>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

int main(int argc, char *argv[]) {
  // const char* fileName = "test_balanced_stepup_D-D_phAB";
  const char *fileName = "test_HVAC_balance";
  // Instantiate GridLabD via exectuable path
  GridLabD gld;

  // Test load_glm
  std::vector<const char *> args = {"gridlabd", fileName, "--verbose"};
  int test_argc = static_cast<int>(args.size());
  char *test_argv[] = {const_cast<char *>(args[0]), const_cast<char *>(args[1]),
                       const_cast<char *>(args[2])};
  gld.load_glm(test_argc, test_argv);

  TIMESTAMP start_time = convert_to_timestamp("2000-04-01 0:00:00");
  TIMESTAMP stop_time = convert_to_timestamp("2000-06-01 0:00:00");

  // Test run examples
  // gld.run();
  // gld.run(start_time, stop_time);

  // Stepping through the simulation examples, check sim_time for each step if
  // needed. gld.set_time_step(900); // 15 minutes in seconds Need to support
  // actually setting the timestep value, not just the minimum. Recorders do
  // this. Tell it what the synchronization time is.
  double sim_time;

  // Get the house object properties using get_object_properties()
  auto house_props = gld.get_object_properties("This_old_house");

  for (int i = 0; i < 10; i++) {
    printf("\n=== Step %d ===\n", i + 1);

    // Get current value before setting
    house_props = gld.get_object_properties("This_old_house");
    auto it = house_props.find("number_of_doors");
    if (it != house_props.end()) {
      printf("Current number_of_doors before set: %s\n", it->second.c_str());
    }

    // Note: Setting properties requires direct API calls - this test
    // demonstrates reading For setting values, would need to use the core
    // object_set_value_by_name() directly

    gld.step(sim_time);

    nlohmann::json checkpoint =
        gld.get_checkpoint_json("/mnt/c/dev/gridlab-d_fork/_test_results/");
  }

  // Get all info for GLD
  nlohmann::json checkpoint =
      gld.get_checkpoint_json("/mnt/c/dev/gridlab-d_fork/_test_results/");
  // OR access it directly
  // gld.get_checkpoint_json();
  // nlohmann::json checkpoint = gld.gld_model; // use nlohmann::json (JsonCpp
  // removed)

  // Test exit_gld
  gld.exit_gld(fileName);

  return 0;
}
