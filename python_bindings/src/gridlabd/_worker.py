"""
Worker process for isolated GridLabD instances.

This module runs in a subprocess and handles commands from the main process,
executing them against a real GridLabD C++ instance and returning results.
"""

import sys
import json
from typing import Any

from ._protocol import Command, Message, Response

# Import the direct C++ binding
from .gridlabd_core import GridLabD as DirectGridLabD, GLDErrorCode


# Global instance for this worker

def _convert_error_code(code):
    """Convert GLDErrorCode enum or int to int."""
    if isinstance(code, int):
        return code
    # It's a GLDErrorCode enum
    return int(code.value)

_gld_instance: DirectGridLabD | None = None


def handle_init(message: Message) -> Response:
    """Initialize a new GridLabD instance."""
    global _gld_instance
    try:
        # Set install root from environment before creating instance
        import os
        if "GRIDLABD_ROOT" in os.environ:
            DirectGridLabD.set_install_root(os.environ["GRIDLABD_ROOT"])
        
        _gld_instance = DirectGridLabD()
        return Response(success=True, result=True)
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_set_install_root(message: Message) -> Response:
    """Set the installation root."""
    try:
        DirectGridLabD.set_install_root(message.args["path"])
        return Response(success=True, result=None)
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_get_install_root(message: Message) -> Response:
    """Get the installation root."""
    try:
        result = DirectGridLabD.get_install_root()
        return Response(success=True, result=result)
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_get_executable_path(message: Message) -> Response:
    """Get the executable path."""
    try:
        result = DirectGridLabD.get_executable_path()
        return Response(success=True, result=result)
    except Exception as e:
        return Response(success=False, error=str(e))

def handle_get_object_count(msg: Message) -> Response:
    """Get the number of objects in the model."""
    try:
        count = _gld_instance.object_get_count()
        return Response(success=True, result=count)
    except Exception as e:
        return Response(success=False, error=str(e))
    
def handle_set_config_file(message: Message) -> Response:
    """Set the configuration file."""
    try:
        code = _gld_instance.set_config_file(message.args["config_file"])
        return Response(success=True, result=int(code) if isinstance(code, int) else int(code.value))
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_set_working_directory(message: Message) -> Response:
    """Set the working directory."""
    try:
        code = _gld_instance.set_working_directory(message.args["dir"])
        return Response(success=True, result=int(code) if isinstance(code, int) else int(code.value))
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_setup_before_load(message: Message) -> Response:
    """Setup before loading a model."""
    try:
        code = _gld_instance.setup_before_load()
        return Response(success=True, result=int(code) if isinstance(code, int) else int(code.value))
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_setup_after_load(message: Message) -> Response:
    """Setup after loading a model."""
    try:
        code = _gld_instance.setup_after_load()
        return Response(success=True, result=int(code) if isinstance(code, int) else int(code.value))
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_load_glm(message: Message) -> Response:
    """Load a GLM file with arguments."""
    try:
        code = _gld_instance.load_glm(message.args["arguments"])
        return Response(success=True, result=int(code) if isinstance(code, int) else int(code.value))
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_load(message: Message) -> Response:
    """Load a GLM file (simple version)."""
    try:
        # Convert filename to arguments list
        filename = message.args["filename"]
        code = _gld_instance.load_glm([filename])
        return Response(success=True, result=int(code) if isinstance(code, int) else int(code.value))
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_run(message: Message) -> Response:
    """Run the simulation."""
    try:
        start_time = message.args.get("start_time")
        stop_time = message.args.get("stop_time")
        code = _gld_instance.run(start_time, stop_time)
        return Response(success=True, result=int(code) if isinstance(code, int) else int(code.value))
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_run_test(message: Message) -> Response:
    """Run the simulation (test mode)."""
    try:
        code = _gld_instance.run_test()
        return Response(success=True, result=int(code) if isinstance(code, int) else int(code.value))
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_step(message: Message) -> Response:
    """Step the simulation."""
    try:
        code, simulation_time = _gld_instance.step()
        return Response(success=True, result={"code": int(code) if isinstance(code, int) else int(code.value), "time": simulation_time})
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_step_to(message: Message) -> Response:
    """Step to a specific time."""
    try:
        target_time = message.args["target_time"]
        code, simulation_time = _gld_instance.step_to(target_time)
        return Response(success=True, result={"code": int(code) if isinstance(code, int) else int(code.value), "time": simulation_time})
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_set_time(message: Message) -> Response:
    """Set the simulation time."""
    try:
        code = _gld_instance.set_time(message.args["timestamp"])
        return Response(success=True, result=int(code) if isinstance(code, int) else int(code.value))
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_get_time(message: Message) -> Response:
    """Get the current simulation time."""
    try:
        code, current_time = _gld_instance.get_time()
        return Response(success=True, result={"code": int(code) if isinstance(code, int) else int(code.value), "time": current_time})
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_set_time_step(message: Message) -> Response:
    """Set the simulation time step."""
    try:
        code = _gld_instance.set_time_step(message.args["time_step"])
        return Response(success=True, result=int(code) if isinstance(code, int) else int(code.value))
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_save_checkpoint(message: Message) -> Response:
    """Save a checkpoint."""
    try:
        save_path = message.args["save_path"]
        mode = message.args.get("mode")
        if mode is not None:
            code = _gld_instance.save_checkpoint(save_path, mode)
        else:
            code = _gld_instance.save_checkpoint(save_path)
        return Response(success=True, result=int(code) if isinstance(code, int) else int(code.value))
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_load_checkpoint(message: Message) -> Response:
    """Load a checkpoint."""
    try:
        code = _gld_instance.load_checkpoint(message.args["file_path"])
        return Response(success=True, result=int(code) if isinstance(code, int) else int(code.value))
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_get_checkpoint_json(message: Message) -> Response:
    """Get checkpoint as JSON."""
    try:
        filepath = message.args.get("filepath", "")
        result = _gld_instance.get_checkpoint_json(filepath)
        return Response(success=True, result=result)
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_start(message: Message) -> Response:
    """Start the simulation (alias for setup_before_load)."""
    return handle_setup_before_load(message)


def handle_stop(message: Message) -> Response:
    """Stop the simulation."""
    try:
        # GridLabD doesn't have an explicit stop, just return success
        return Response(success=True, result=0)
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_exit_gld(message: Message) -> Response:
    """Exit GridLabD."""
    try:
        filepath = message.args.get("filepath", "")
        code = _gld_instance.exit_gld(filepath)
        return Response(success=True, result=int(code) if isinstance(code, int) else int(code.value))
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_finalize(message: Message) -> Response:
    """Finalize (alias for exit_gld)."""
    return handle_exit_gld(message)


def handle_is_initialized(message: Message) -> Response:
    """Check if initialized."""
    return Response(success=True, result=_gld_instance is not None)


def handle_get_all_classes(message: Message) -> Response:
    """Get all class names."""
    try:
        result = _gld_instance.get_all_classes()
        return Response(success=True, result=result)
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_get_objects_by_class(message: Message) -> Response:
    """Get all objects of a class."""
    try:
        result = _gld_instance.get_objects_by_class(message.args["class_name"])
        return Response(success=True, result=result)
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_get_object_properties(message: Message) -> Response:
    """Get all properties of an object."""
    try:
        result = _gld_instance.get_object_properties(message.args["object_name"])
        return Response(success=True, result=result)
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_get_property(message: Message) -> Response:
    """Get a property value."""
    try:
        code, value = _gld_instance.get_property(
            message.args["object_name"],
            message.args["property_name"]
        )
        return Response(success=True, result={"code": int(code) if isinstance(code, int) else int(code.value), "value": value})
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_set_property(message: Message) -> Response:
    """Set a property value."""
    try:
        code = _gld_instance.set_property(
            message.args["object_name"],
            message.args["property_name"],
            message.args["value"]
        )
        return Response(success=True, result=int(code) if isinstance(code, int) else int(code.value))
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_get_properties_by_class(message: Message) -> Response:
    """Get property values from all objects of a class."""
    try:
        result = _gld_instance.get_properties_by_class(
            message.args["class_name"],
            message.args["property_name"]
        )
        return Response(success=True, result=result)
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_set_property_by_class(message: Message) -> Response:
    """Set property value on all objects of a class."""
    try:
        code = _gld_instance.set_property_by_class(
            message.args["class_name"],
            message.args["property_name"],
            message.args["value"]
        )
        return Response(success=True, result=int(code) if isinstance(code, int) else int(code.value))
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_set_global(message: Message) -> Response:
    """Set a global variable (legacy support)."""
    try:
        # For now, just return success - globals are handled differently
        return Response(success=True, result=0)
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_get_global(message: Message) -> Response:
    """Get a global variable (legacy support)."""
    try:
        # For now, return empty string
        return Response(success=True, result="")
    except Exception as e:
        return Response(success=False, error=str(e))


# Command handler mapping
COMMAND_HANDLERS = {
    Command.INIT: handle_init,
    Command.SET_INSTALL_ROOT: handle_set_install_root,
    Command.GET_INSTALL_ROOT: handle_get_install_root,
    Command.GET_EXECUTABLE_PATH: handle_get_executable_path,
    Command.SET_CONFIG_FILE: handle_set_config_file,
    Command.SET_WORKING_DIRECTORY: handle_set_working_directory,
    Command.SETUP_BEFORE_LOAD: handle_setup_before_load,
    Command.SETUP_AFTER_LOAD: handle_setup_after_load,
    Command.LOAD_GLM: handle_load_glm,
    Command.LOAD: handle_load,
    Command.RUN: handle_run,
    Command.RUN_TEST: handle_run_test,
    Command.STEP: handle_step,
    Command.STEP_TO: handle_step_to,
    Command.SET_TIME: handle_set_time,
    Command.GET_TIME: handle_get_time,
    Command.SET_TIME_STEP: handle_set_time_step,
    Command.SAVE_CHECKPOINT: handle_save_checkpoint,
    Command.LOAD_CHECKPOINT: handle_load_checkpoint,
    Command.GET_CHECKPOINT_JSON: handle_get_checkpoint_json,
    Command.START: handle_start,
    Command.STOP: handle_stop,
    Command.EXIT_GLD: handle_exit_gld,
    Command.FINALIZE: handle_finalize,
    Command.IS_INITIALIZED: handle_is_initialized,
    Command.GET_ALL_CLASSES: handle_get_all_classes,
    Command.GET_OBJECTS_BY_CLASS: handle_get_objects_by_class,
    Command.GET_OBJECT_COUNT: handle_get_object_count,
    Command.GET_OBJECT_PROPERTIES: handle_get_object_properties,
    Command.GET_PROPERTY: handle_get_property,
    Command.SET_PROPERTY: handle_set_property,
    Command.GET_PROPERTIES_BY_CLASS: handle_get_properties_by_class,
    Command.SET_PROPERTY_BY_CLASS: handle_set_property_by_class,
    Command.SET_GLOBAL: handle_set_global,
    Command.GET_GLOBAL: handle_get_global,
}


def main():
    """Main worker loop - reads commands from stdin, executes them, writes responses to stdout."""
    for line in sys.stdin:
        try:
            message = Message.from_json(line.strip())
            handler = COMMAND_HANDLERS.get(message.command)
            
            if handler is None:
                response = Response(success=False, error=f"Unknown command: {message.command}")
            else:
                response = handler(message)
            
            print(response.to_json(), flush=True)
        except Exception as e:
            response = Response(success=False, error=f"Worker error: {str(e)}")
            print(response.to_json(), flush=True)


if __name__ == "__main__":
    main()
