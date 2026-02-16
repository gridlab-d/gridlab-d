#include "gldapi.h"
#include <cassert>
#include <string>

GLDErrorCode dummy_callback(GridLabD* gld) {
    return GLD_SUCCESS;
}

int main() {
    GridLabD sim;

    assert(sim.set_config_file("config.cfg") == GLD_SUCCESS);
    assert(sim.load_glm("example.glm") == GLD_SUCCESS);

    GLDData data = {
        {"name", std::string("object1")},
        {"type", std::string("generator")}
    };

        assert(sim.add_object(data) == GLD_SUCCESS);
    assert(sim.delete_object("object1") == GLD_SUCCESS);
    assert(sim.edit_object("object1", data) == GLD_SUCCESS);
    assert(sim.set_glm_data(data) == GLD_SUCCESS);

    GLDData query_result;
    assert(sim.get_glm_data("SELECT * FROM all", query_result) == GLD_SUCCESS);
    assert(query_result.count("status") > 0);

    assert(sim.save_checkpoint("state.chk", GLD_CHECKPOINT_MODE_SAVE) == GLD_SUCCESS);
    assert(sim.load_checkpoint("state.chk") == GLD_SUCCESS);

    assert(sim.set_prestep_callback(dummy_callback) == GLD_SUCCESS);
    assert(sim.set_poststep_callback(dummy_callback) == GLD_SUCCESS);

    assert(sim.set_time("2025-06-18T10:00:00") == GLD_SUCCESS);

    std::string current_time;
    assert(sim.get_time(current_time) == GLD_SUCCESS);

    assert(sim.set_application_mode(GLD_APPLICATION_TYPE_GRIDLABD) == GLD_SUCCESS);
    assert(sim.set_time_step(60.0) == GLD_SUCCESS);

    double sim_time = 0.0;
    assert(sim.run(0.0, 3600.0, sim_time) == GLD_SUCCESS);
    assert(sim.step(sim_time) == GLD_SUCCESS);
    assert(sim.reset_step(sim_time) == GLD_SUCCESS);

    printf("✅ All GLD API tests passed successfully.\n");
    return 0;
}
