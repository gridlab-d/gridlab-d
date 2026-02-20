"""
IPC Protocol definitions for isolated GridLabD instances.

This module defines the message protocol used for communication between
the main process and worker subprocesses running isolated GridLabD instances.
"""

from dataclasses import dataclass
from enum import Enum
from typing import Any, Optional
import json


class Command(Enum):
    """Commands that can be sent to worker processes."""
    
    # Initialization and configuration
    INIT = "init"
    SET_INSTALL_ROOT = "set_install_root"
    GET_INSTALL_ROOT = "get_install_root"
    GET_EXECUTABLE_PATH = "get_executable_path"
    SET_CONFIG_FILE = "set_config_file"
    SET_WORKING_DIRECTORY = "set_working_directory"
    
    # Setup and loading
    SETUP_BEFORE_LOAD = "setup_before_load"
    SETUP_AFTER_LOAD = "setup_after_load"
    LOAD_GLM = "load_glm"
    LOAD = "load"
    
    # Execution
    RUN = "run"
    RUN_TEST = "run_test"
    STEP = "step"
    STEP_TO = "step_to"
    
    # Time management
    SET_TIME = "set_time"
    GET_TIME = "get_time"
    SET_TIME_STEP = "set_time_step"
    
    # Checkpoints
    SAVE_CHECKPOINT = "save_checkpoint"
    LOAD_CHECKPOINT = "load_checkpoint"
    GET_CHECKPOINT_JSON = "get_checkpoint_json"
    
    # Lifecycle
    START = "start"
    STOP = "stop"
    EXIT_GLD = "exit_gld"
    FINALIZE = "finalize"
    IS_INITIALIZED = "is_initialized"
    
    # Globals
    SET_GLOBAL = "set_global"
    GET_GLOBAL = "get_global"
    
    # Object and property access
    GET_ALL_CLASSES = "get_all_classes"
    GET_OBJECTS_BY_CLASS = "get_objects_by_class"
    GET_OBJECT_PROPERTIES = "get_object_properties"
    GET_ALL_OBJECTS = "get_all_objects"
    GET_MODEL = "get_model"
    GET_OBJECT_COUNT = "get_object_count"
    GET_PROPERTY = "get_property"
    GET_PROPERTY_INFO = "get_property_info"
    SET_PROPERTY = "set_property"
    GET_PROPERTIES_BY_CLASS = "get_properties_by_class"
    SET_PROPERTY_BY_CLASS = "set_property_by_class"
    
    # Message capture
    GET_MESSAGES = "get_messages"
    CLEAR_MESSAGES = "clear_messages"
    ENABLE_MESSAGE_CAPTURE = "enable_message_capture"
    SET_MESSAGE_CAPTURE_LIMIT = "set_message_capture_limit"
    GET_MESSAGE_CAPTURE_LIMIT = "get_message_capture_limit"


@dataclass
class Message:
    """A message sent between main process and worker."""
    
    command: Command
    args: dict[str, Any]
    
    def to_json(self) -> str:
        """Serialize message to JSON string."""
        return json.dumps({
            "command": self.command.value,
            "args": self.args
        })
    
    @classmethod
    def from_json(cls, json_str: str) -> "Message":
        """Deserialize message from JSON string."""
        data = json.loads(json_str)
        return cls(
            command=Command(data["command"]),
            args=data["args"]
        )


@dataclass
class Response:
    """A response from a worker process."""
    
    success: bool
    result: Any = None
    error: Optional[str] = None
    
    def to_json(self) -> str:
        """Serialize response to JSON string."""
        return json.dumps({
            "success": self.success,
            "result": self.result,
            "error": self.error
        })
    
    @classmethod
    def from_json(cls, json_str: str) -> "Response":
        """Deserialize response from JSON string."""
        data = json.loads(json_str)
        return cls(
            success=data["success"],
            result=data.get("result"),
            error=data.get("error")
        )
