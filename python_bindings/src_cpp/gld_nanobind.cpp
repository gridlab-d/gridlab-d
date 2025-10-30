#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>

#include <optional>
#include <string>
#include <vector>

#include <json/json.h>

#include "../../gldcore/gldapi.h"

#define NB_STRINGIFY_HELPER(x) #x
#define NB_STRINGIFY(x) NB_STRINGIFY_HELPER(x)

namespace nb = nanobind;

namespace {

GLDErrorCode load_glm_with_arguments(GridLabD& instance, const std::vector<std::string>& arguments) {
    if (arguments.empty()) {
        return GLD_INVALID_FORMAT;
    }

    // Copy to ensure pointers remain valid for the lifetime of the call
    std::vector<std::string> local = arguments;
    std::vector<char*> argv;
    argv.reserve(local.size());
    for (auto& arg : local) {
        argv.push_back(arg.data());
    }

    return instance.load_glm(static_cast<int>(argv.size()), argv.data());
}

nb::dict runtime_info() {
    nb::dict info;
    info["install_root"] = GridLabD::get_install_root();
    info["executable_path"] = GridLabD::get_executable_path();
    return info;
}

} // namespace

NB_MODULE(gridlabd_core, m) {
    m.doc() = "GridLAB-D Python bindings using the public C++ API";

    m.def("hello", []() {
        return "Hello from GridLAB-D Python bindings!";
    }, "Quick sanity check helper");

    m.def("set_install_root", [](std::optional<std::string> install_root) {
        GridLabD::set_install_root(install_root.value_or(std::string()));
    }, nb::arg("install_root") = nb::none(), "Override the GridLAB-D installation root or executable path");

    m.def("get_install_root", []() {
        return GridLabD::get_install_root();
    }, "Resolved GridLAB-D installation root");

    m.def("get_executable_path", []() {
        return GridLabD::get_executable_path();
    }, "Resolved GridLAB-D executable path");

    m.def("runtime_info", runtime_info, "Return runtime path information as a dictionary");

    nb::enum_<GLDErrorCode>(m, "GLDErrorCode")
        .value("SUCCESS", GLD_SUCCESS)
        .value("FILE_NOT_FOUND", GLD_FILE_NOT_FOUND)
        .value("INVALID_FORMAT", GLD_INVALID_FORMAT)
        .value("OPERATION_FAILED", GLD_OPERATION_FAILED)
        .value("OBJECT_NOT_FOUND", GLD_OBJECT_NOT_FOUND)
        .value("TIME_STEP_ERROR", GLD_TIME_STEP_ERROR)
        .value("FAILED_TO_START", GLD_FAILED_TO_START)
        .export_values();

    nb::enum_<GLDCheckPointMode>(m, "GLDCheckPointMode")
        .value("NONE", GLD_CHECKPOINT_MODE_NONE)
        .value("SAVE", GLD_CHECKPOINT_MODE_SAVE)
        .value("LOAD", GLD_CHECKPOINT_MODE_LOAD)
        .export_values();

    nb::enum_<GLDApplicationType>(m, "GLDApplicationType")
        .value("UNKNOWN", GLD_APPLICATION_TYPE_UNKNOWN)
        .value("GRIDLABD", GLD_APPLICATION_TYPE_GRIDLABD)
        .value("OTHER", GLD_APPLICATION_TYPE_OTHER)
        .export_values();

    nb::class_<GridLabD>(m, "GridLabD")
        .def(nb::init<>(), "Create a GridLAB-D runtime instance")
        .def_static("set_install_root", [](std::optional<std::string> install_root) {
            GridLabD::set_install_root(install_root.value_or(std::string()));
        }, nb::arg("install_root") = nb::none(), "Override the GridLAB-D installation root or executable path")
        .def_static("get_install_root", &GridLabD::get_install_root, "Resolved GridLAB-D installation root")
        .def_static("get_executable_path", &GridLabD::get_executable_path, "Resolved GridLAB-D executable path")
        .def("set_config_file", &GridLabD::set_config_file, nb::arg("config_file"), "Set the configuration file path")
        .def("setup_before_load", &GridLabD::setup_before_load, "Initialise modules prior to loading a model")
        .def("setup_after_load", &GridLabD::setup_after_load, "Finalise setup after model loading")
        .def("load_glm", [](GridLabD& self, const std::string& filepath) {
            return self.load_glm(filepath);
        }, nb::arg("filepath"), "Load a GridLAB-D model (.glm) file")
        .def("load_glm_with_arguments", [](GridLabD& self, const std::vector<std::string>& arguments) {
            return load_glm_with_arguments(self, arguments);
        }, nb::arg("arguments"), "Load a model using argv-style arguments")
        .def("run", [](GridLabD& self, std::optional<double> start_time, std::optional<double> stop_time) {
            return self.run(start_time, stop_time);
        }, nb::arg("start_time") = nb::none(), nb::arg("stop_time") = nb::none(), "Run the simulation optionally bounding time interval")
        .def("step", [](GridLabD& self) {
            double simulation_time = 0.0;
            GLDErrorCode code = self.step(simulation_time);
            return nb::make_tuple(code, simulation_time);
        }, "Advance the simulation by one time step")
        .def("set_time", &GridLabD::set_time, nb::arg("timestamp"), "Set the simulation time")
        .def("get_time", [](GridLabD& self) {
            std::string current_time;
            GLDErrorCode code = self.get_time(current_time);
            return nb::make_tuple(code, current_time);
        }, "Get the current simulation time")
        .def("set_time_step", &GridLabD::set_time_step, nb::arg("time_step"), "Set the simulation time step")
        .def("save_checkpoint", &GridLabD::save_checkpoint, nb::arg("save_path"), nb::arg("mode") = GLD_CHECKPOINT_MODE_SAVE, "Save the simulation state")
        .def("load_checkpoint", &GridLabD::load_checkpoint, nb::arg("file_path"), "Load a previously saved simulation state")
        .def("exit_gld", &GridLabD::exit_gld, nb::arg("filepath"), "Shutdown the simulation")
        .def("start", &GridLabD::setup_before_load, "Compatibility alias for setup_before_load")
        .def("finalize", [](GridLabD& self, const std::string& filepath) {
            return self.exit_gld(filepath);
        }, nb::arg("filepath") = std::string(), "Compatibility alias for exit_gld")
        .def("is_initialized", [](GridLabD&) {
            return true;
        }, "Return True once the object exists")
        .def("get_checkpoint_json", [](GridLabD& self, const std::string& filepath) {
            Json::Value value = self.get_checkpoint_json(filepath);
            Json::StreamWriterBuilder builder;
            builder["indentation"] = "";
            return Json::writeString(builder, value);
        }, nb::arg("filepath") = std::string(), "Return checkpoint data as a JSON string");

#ifdef VERSION_INFO
    m.attr("__version__") = NB_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "0.0.0";
#endif
}
