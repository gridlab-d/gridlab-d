"""
Isolated GridLabD wrapper using process isolation.

This module provides a GridLabD interface that runs each instance in a separate
subprocess, preventing global state conflicts in the C++ core.
"""

import subprocess
import sys
import os
from subprocess import PIPE
from typing import Any, Optional

from ._protocol import Command, Message, Response


class IsolatedGridLabD:
    """
    GridLabD wrapper that runs in an isolated subprocess.
    
    Each instance spawns its own worker process, ensuring complete isolation
    of global state in the C++ core.
    """
    
    def __init__(self):
        """Create a new isolated GridLabD instance."""
        self._process: Optional[subprocess.Popen] = None
        self._spawn_worker()
        # Initialize the GridLabD instance in the worker
        self._send_command(Command.INIT, {})
    
    def _spawn_worker(self):
        """Spawn a new worker subprocess."""
        self._process = subprocess.Popen(
            [sys.executable, "-m", "gridlabd._worker"],
            stdin=PIPE,
            stdout=PIPE,
            stderr=PIPE,
            text=True,
            bufsize=1
        )
    
    def _send_command(self, command: Command, args: dict[str, Any]) -> Response:
        """Send a command to the worker and get the response."""
        if self._process is None or self._process.poll() is not None:
            raise RuntimeError("Worker process is not running")
        
        message = Message(command=command, args=args)
        self._process.stdin.write(message.to_json() + "\n")
        self._process.stdin.flush()
        
        response_line = self._process.stdout.readline()
        if not response_line:
            raise RuntimeError("Worker process closed unexpectedly")
        
        return Response.from_json(response_line.strip())
    
    def __del__(self):
        """Clean up the worker process."""
        if self._process and self._process.poll() is None:
            self._process.terminate()
            self._process.wait(timeout=5)
    
    # Static methods
    @staticmethod
    def set_install_root(path: str):
        """Set the GridLAB-D installation root directory."""
        # This would need to be sent to worker, but static methods are tricky with process isolation
        # For now, set it as environment variable
        os.environ["GRIDLABD_INSTALL_ROOT"] = path
    
    @staticmethod
    def get_install_root() -> str:
        """Get the GridLAB-D installation root directory."""
        return os.environ.get("GRIDLABD_INSTALL_ROOT", "")
    
    @staticmethod
    def get_executable_path() -> str:
        """Get the GridLAB-D executable path."""
        return sys.executable
    
    # Configuration methods
    def set_config_file(self, config_file: str) -> int:
        """Set the configuration file path."""
        response = self._send_command(Command.SET_CONFIG_FILE, {"config_file": config_file})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
    
    def set_working_directory(self, dir: str) -> int:
        """Set working directory for resolving relative paths in GLM files."""
        response = self._send_command(Command.SET_WORKING_DIRECTORY, {"dir": dir})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
    
    # Setup methods
    def setup_before_load(self) -> int:
        """Initialize modules prior to loading a model."""
        response = self._send_command(Command.SETUP_BEFORE_LOAD, {})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
    
    def setup_after_load(self) -> int:
        """Finalize setup after model loading."""
        response = self._send_command(Command.SETUP_AFTER_LOAD, {})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
    
    def start(self) -> int:
        """Compatibility alias for setup_before_load."""
        return self.setup_before_load()
    
    # Loading methods
    def load_glm(self, arguments: list[str]) -> int:
        """Load a model using argv-style arguments (list of strings)."""
        response = self._send_command(Command.LOAD_GLM, {"arguments": arguments})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
    
    def load(self, filename: str) -> int:
        """Load a GLM file (simple version)."""
        response = self._send_command(Command.LOAD, {"filename": filename})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
    
    # Execution methods
    def run(self, start_time: Optional[float] = None, stop_time: Optional[float] = None) -> int:
        """Run the simulation optionally bounding time interval."""
        if self.get_object_count() == 0:
            raise RuntimeError("Cannot run simulation: no objects loaded in model")
        response = self._send_command(Command.RUN, {
            "start_time": start_time,
            "stop_time": stop_time
        })
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
    
    def run_test(self) -> int:
        """Run the simulation."""
        response = self._send_command(Command.RUN_TEST, {})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
    
    def step(self) -> tuple[int, float]:
        """Advance the simulation by one time step."""
        if self.get_object_count() == 0:
            raise RuntimeError("Cannot step simulation: no objects loaded in model")
  
        response = self._send_command(Command.STEP, {})
        if not response.success:
            raise RuntimeError(response.error)
        
        return response.result["code"], response.result["time"]
    
    def step_to(self, target_time_str: str) -> tuple[int, float]:
        """Step the simulation to a specific timestamp (ISO 8601 string)."""
        response = self._send_command(Command.STEP_TO, {"target_time": target_time_str})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result["code"], response.result["time"]
    
    # Time management methods
    def set_time(self, timestamp: str) -> int:
        """Set the simulation time."""
        response = self._send_command(Command.SET_TIME, {"timestamp": timestamp})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
    
    def get_time(self) -> tuple[int, str]:
        """Get the current simulation time."""
        response = self._send_command(Command.GET_TIME, {})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result["code"], response.result["time"]
    
    def set_time_step(self, time_step: int) -> int:
        """Set the simulation time step."""
        response = self._send_command(Command.SET_TIME_STEP, {"time_step": time_step})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
    
    # Checkpoint methods
    def save_checkpoint(self, save_path: str, mode: Optional[int] = None) -> int:
        """Save the simulation state."""
        args = {"save_path": save_path}
        if mode is not None:
            args["mode"] = mode
        response = self._send_command(Command.SAVE_CHECKPOINT, args)
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
    
    def load_checkpoint(self, file_path: str) -> int:
        """Load a previously saved simulation state."""
        response = self._send_command(Command.LOAD_CHECKPOINT, {"file_path": file_path})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
    
    def get_checkpoint_json(self, filepath: str = "") -> str:
        """Return checkpoint data as a JSON string."""
        response = self._send_command(Command.GET_CHECKPOINT_JSON, {"filepath": filepath})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
    
    # Lifecycle methods
    def stop(self) -> int:
        """Stop the simulation."""
        response = self._send_command(Command.STOP, {})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
    
    def exit_gld(self, filepath: str = "") -> int:
        """Shutdown the simulation."""
        response = self._send_command(Command.EXIT_GLD, {"filepath": filepath})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
    
    def finalize(self, filepath: str = "") -> int:
        """Compatibility alias for exit_gld."""
        return self.exit_gld(filepath)
    
    def is_initialized(self) -> bool:
        """Return True once the object exists."""
        response = self._send_command(Command.IS_INITIALIZED, {})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
    
    # Object and property access methods
    def get_all_classes(self) -> list[str]:
        """Get all class names in the model."""
        response = self._send_command(Command.GET_ALL_CLASSES, {})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
    
    def get_object_count(self) -> int:
        """Get the number of objects in the loaded model."""
        response = self._send_command(Command.GET_OBJECT_COUNT, {})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result

    def get_objects_by_class(self, class_name: str) -> list[str]:
        """Get all object names of a specific class."""
        response = self._send_command(Command.GET_OBJECTS_BY_CLASS, {"class_name": class_name})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
    
    def get_object_properties(self, object_name: str) -> dict[str, str]:
        """Get all properties of an object as a dictionary."""
        response = self._send_command(Command.GET_OBJECT_PROPERTIES, {"object_name": object_name})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
    
    def get_all_objects(self, class_name: str) -> list[dict[str, str]]:
        """Get all objects (and their properties) of a specific class."""
        response = self._send_command(Command.GET_ALL_OBJECTS, {"class_name": class_name})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
    
    def get_model(self) -> dict[str, list[dict[str, str]]]:
        """Get the entire model with all objects and properties organized by class."""
        response = self._send_command(Command.GET_MODEL, {})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
    
    def get_property(self, object_name: str, property_name: str) -> tuple[int, str]:
        """Get a property value from an object."""
        response = self._send_command(Command.GET_PROPERTY, {
            "object_name": object_name,
            "property_name": property_name
        })
        if not response.success:
            raise RuntimeError(response.error)
        return response.result["code"], response.result["value"]
    
    def set_property(self, object_name: str, property_name: str, value: str) -> int:
        """Set a property value on an object."""
        response = self._send_command(Command.SET_PROPERTY, {
            "object_name": object_name,
            "property_name": property_name,
            "value": value
        })
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
    
    def get_properties_by_class(self, class_name: str, property_name: str) -> dict[str, str]:
        """Get property values from all objects of a class."""
        response = self._send_command(Command.GET_PROPERTIES_BY_CLASS, {
            "class_name": class_name,
            "property_name": property_name
        })
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
    
    def set_property_by_class(self, class_name: str, property_name: str, value: str) -> int:
        """Set property value on all objects of a class."""
        response = self._send_command(Command.SET_PROPERTY_BY_CLASS, {
            "class_name": class_name,
            "property_name": property_name,
            "value": value
        })
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
    
    # Legacy global variable methods (for backward compatibility)
    def global_setvar(self, name: str, value: str) -> int:
        """Set a global variable (legacy support)."""
        response = self._send_command(Command.SET_GLOBAL, {"name": name, "value": value})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
    
    def global_getvar(self, name: str) -> str:
        """Get a global variable (legacy support)."""
        response = self._send_command(Command.GET_GLOBAL, {"name": name})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
