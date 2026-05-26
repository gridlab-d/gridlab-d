"""
Isolated GridLabD wrapper using process isolation.

This module provides a GridLabD interface that runs each instance in a separate
subprocess, preventing global state conflicts in the C++ core.
"""

import atexit
import subprocess
import sys
import os
import weakref
import re
import json
from datetime import datetime
from subprocess import PIPE
from typing import Any, Optional

from ._protocol import Command, Message, Response
from ._time_utils import gld_to_iso

_TZ_OFFSETS = {
    "PST": "-08:00",
    "PDT": "-07:00",
    "MST": "-07:00",
    "MDT": "-06:00",
    "CST": "-06:00",
    "CDT": "-05:00",
    "EST": "-05:00",
    "EDT": "-04:00",
}


def _to_iso8601(time_str: str) -> str:
    value = time_str.strip()
    if len(value) >= 2 and value[0] == value[-1] and value[0] in ("'", '"'):
        value = value[1:-1].strip()
    if re.match(r"^\d{4}-\d{2}-\d{2}T", value):
        return value

    parts = value.split()
    if len(parts) < 2:
        return value

    date_part = parts[0]
    time_part = parts[1]
    tz_part = parts[2] if len(parts) >= 3 else None

    iso = f"{date_part}T{time_part}"
    if tz_part in _TZ_OFFSETS:
        iso = f"{iso}{_TZ_OFFSETS[tz_part]}"
    return iso

def _normalize_time_input(value: str) -> str:
    """Normalize ISO 8601 time strings into GridLAB-D friendly format.

    GridLAB-D core APIs generally accept "YYYY-MM-DD HH:MM:SS" without
    timezone offsets. If an ISO string is provided, strip the offset and
    replace the T separator.
    """
    if not isinstance(value, str):
        return value

    if "T" in value:
        try:
            parsed = datetime.fromisoformat(value)
            # GridLAB-D parsers used by step_to don't support timezone offsets,
            # so strip tzinfo but preserve fractional seconds.
            parsed = parsed.replace(tzinfo=None)
            text = parsed.isoformat(sep=" ", timespec="microseconds")
            if "." in text:
                text = text.rstrip("0").rstrip(".")
            return text
        except ValueError:
            return value.replace("T", " ", 1)

    return value


def _parse_iso_datetime(value: Optional[str]) -> Optional[datetime]:
    """Parse an ISO 8601 datetime string, normalizing timezone-aware values."""
    if not value or not isinstance(value, str):
        return None

    text = value.strip()
    if not text:
        return None

    if text.endswith("Z"):
        text = text[:-1] + "+00:00"

    try:
        dt = datetime.fromisoformat(text)
    except ValueError:
        return None

    if dt.tzinfo is not None:
        dt = dt.replace(tzinfo=None)
    return dt


def _parse_iso_datetime_with_tz(value: Optional[str]) -> Optional[datetime]:
    """Parse an ISO 8601 datetime string and preserve timezone when present."""
    if not value or not isinstance(value, str):
        return None

    text = value.strip()
    if not text:
        return None

    if text.endswith("Z"):
        text = text[:-1] + "+00:00"

    try:
        return datetime.fromisoformat(text)
    except ValueError:
        return None


def _coerce_run_bound_to_timestamp(
    value: Optional[float | str],
    reference_tz,
) -> Optional[float]:
    """Convert run() bound inputs (float or ISO string) to numeric timestamps."""
    if value is None:
        return None

    if isinstance(value, (int, float)):
        return float(value)

    if isinstance(value, str):
        text = value.strip()
        if not text:
            return None

        dt = _parse_iso_datetime_with_tz(text)
        if dt is None:
            raise ValueError(f"Invalid ISO 8601 time value for run bound: {value!r}")

        if dt.tzinfo is None and reference_tz is not None:
            dt = dt.replace(tzinfo=reference_tz)

        return float(dt.timestamp())

    raise TypeError(
        f"run bounds must be float, str, or None; got {type(value).__name__}"
    )


def _is_stoptime_blocked_step(
    before_step_time: Optional[str],
    after_step_time: Optional[str],
    stop_time: Optional[str],
) -> bool:
    """Return True when step() is a no-op because simulation is already at/after stoptime."""
    before_dt = _parse_iso_datetime(before_step_time)
    after_dt = _parse_iso_datetime(after_step_time)
    stop_dt = _parse_iso_datetime(stop_time)
    if before_dt is None or after_dt is None or stop_dt is None:
        return False

    return before_dt >= stop_dt and after_dt == before_dt


class IsolatedGridLabD:
    """
    GridLabD wrapper that runs in an isolated subprocess.
    
    Each instance spawns its own worker process, ensuring complete isolation
    of global state in the C++ core.
    """

    _instances: "weakref.WeakSet[IsolatedGridLabD]" = weakref.WeakSet()
    _atexit_registered = False
    
    def __init__(self, verbose: bool = False):
        """Create a new isolated GridLabD instance.

        Args:
            verbose: If True, pass C++ debug/warning output through to stderr.
                     If False (default), suppress console output. Use
                     get_messages() to retrieve warnings and errors.
        """
        self._process: Optional[subprocess.Popen] = None
        self._verbose = verbose
        self._spawn_worker()
        # Initialize the GridLabD instance in the worker
        response = self._send_command(Command.INIT, {})
        if not response.success:
            raise RuntimeError(f"Failed to initialize worker: {response.error}")

        # Track instances to ensure clean shutdown on interpreter exit
        IsolatedGridLabD._instances.add(self)
        if not IsolatedGridLabD._atexit_registered:
            atexit.register(IsolatedGridLabD._shutdown_all)
            IsolatedGridLabD._atexit_registered = True
    
    def _spawn_worker(self):
        """Spawn a new worker subprocess and wait for it to be ready."""
        self._process = subprocess.Popen(
            [sys.executable, "-m", "gridlabd._worker"],
            stdin=PIPE,
            stdout=PIPE,
            stderr=sys.stderr if self._verbose else subprocess.DEVNULL,
            text=True,
            bufsize=1,
            start_new_session=True
        )
        
        # Read the READY signal (worker sends it immediately on startup)
        # This will block until worker sends READY or closes stdout
        try:
            ready_line = self._process.stdout.readline().strip()
            if ready_line != "READY":
                # Process sent something unexpected
                self._process.terminate()
                raise RuntimeError(f"Worker sent unexpected startup message: {ready_line!r}")
        except Exception as e:
            # Check if process died
            if self._process.poll() is not None:
                raise RuntimeError(f"Worker process exited with code {self._process.returncode}")
            raise RuntimeError(f"Failed to read READY signal from worker: {e}")
    
    def _send_command(self, command: Command, args: dict[str, Any]) -> Response:
        """Send a command to the worker and get the response."""
        if self._process is None or self._process.poll() is not None:
            raise RuntimeError("Worker process is not running")
        
        message = Message(command=command, args=args)
        try:
            self._process.stdin.write(message.to_json() + "\n")
            self._process.stdin.flush()
        except Exception as e:
            exit_code = self._process.poll()
            raise RuntimeError(f"Failed to send {command.name} to worker (exit code: {exit_code}): {e}")
        
        non_protocol_lines: list[str] = []
        while True:
            response_line = self._process.stdout.readline()
            if not response_line:
                # Check if worker died
                exit_code = self._process.poll()
                if command in (Command.EXIT_GLD, Command.FINALIZE):
                    # EXIT_GLD may close stdout before replying; treat as success and
                    # ensure the worker is terminated.
                    if exit_code is None:
                        try:
                            self._process.wait(timeout=2)
                        except subprocess.TimeoutExpired:
                            try:
                                self._process.kill()
                                self._process.wait(timeout=3)
                            except Exception:
                                pass
                    self._process = None
                    return Response(success=True, result=0)

                details = ""
                if non_protocol_lines:
                    details = f" Last worker output: {non_protocol_lines[-1][:200]}"
                if exit_code is not None:
                    raise RuntimeError(
                        f"Worker process exited unexpectedly with code {exit_code} while processing {command.name}.{details}")
                raise RuntimeError(f"Worker process closed stdout while processing {command.name}.{details}")

            response_line = response_line.strip()
            if not response_line:
                continue

            try:
                return Response.from_json(response_line)
            except Exception:
                # GridLAB-D may still emit console output; ignore non-JSON lines
                # and keep reading until a protocol response is found.
                non_protocol_lines.append(response_line)
                if self._verbose:
                    print(
                        f"Worker non-protocol stdout during {command.name}: {response_line}",
                        file=sys.stderr,
                        flush=True,
                    )

    @classmethod
    def _shutdown_all(cls):
        """Ensure all worker processes are cleanly stopped at interpreter exit."""
        for inst in list(cls._instances):
            try:
                inst._shutdown_worker()
            except Exception:
                pass

    def _shutdown_worker(self):
        """Attempt clean worker shutdown without sending SIGTERM."""
        if self._process and self._process.poll() is None:
            try:
                self._process.kill()
                self._process.wait(timeout=3)
            except Exception:
                pass
    
    def __del__(self):
        """Clean up the worker process."""
        self._shutdown_worker()
    
    # Static methods
    @staticmethod
    def set_install_root(path: str):
        """Set the GridLAB-D installation root directory."""
        # Set it as environment variable so worker processes can pick it up
        os.environ["GRIDLABD_ROOT"] = path
        # Also try to validate it using the C++ class directly
        from .gridlabd_core import GridLabD as CppGridLabD
        CppGridLabD.set_install_root(path)
    
    @staticmethod
    def get_install_root() -> str:
        """Get the GridLAB-D installation root directory."""
        # Get from the C++ class which may have been set before this wrapper
        from .gridlabd_core import GridLabD as CppGridLabD
        return CppGridLabD.get_install_root()
    
    @staticmethod
    def get_executable_path() -> str:
        """Get the GridLAB-D executable path."""
        from .gridlabd_core import GridLabD as CppGridLabD
        return CppGridLabD.get_executable_path()
    
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
    def load_glm(
        self, 
        filename_or_args: str | list[str],
        *,
        defines: dict[str, str] | None = None,
        verbose: bool = False,
        warn: bool = False,
        quiet: bool = False,
        debug: bool = False,
        debugger: bool = False,
        check: bool = False,
        workdir: str | None = None,
        threads: int | None = None,
        compile_only: bool = False,
        save: str | None = None,
        **kwargs
    ) -> int:
        """Load a model using either argv-style arguments or Pythonic kwargs.
        
        Args:
            filename_or_args: Either a GLM filename (str) for Pythonic style,
                            or a list of argv-style arguments for backward compatibility
            defines: Dictionary of global variables to define (e.g., {"VAR": "value"})
            verbose: Enable verbose output messages
            warn: Enable warning messages
            quiet: Suppress all but error and fatal messages
            debug: Enable debug messages
            debugger: Enable the debugger
            check: Perform module checks before starting
            workdir: Set the working directory for resolving relative paths
            threads: Set maximum number of threads allowed
            compile_only: Enable compile-only mode
            save: Enable save of output to specified file
            **kwargs: Additional arguments (ignored for forward compatibility)
        
        Returns:
            Error code (0 for success)
            
        Examples:
            # Old style (backward compatible)
            gld.load_glm(["model.glm"])
            gld.load_glm(["model.glm", "-D", "VAR=123", "--verbose"])
            
            # New Pythonic style
            gld.load_glm("model.glm")
            gld.load_glm("model.glm", verbose=True)
            gld.load_glm("model.glm", defines={"VAR": "123", "PARAM": "value"})
            gld.load_glm("model.glm", workdir="/path/to/dir", warn=True)
            gld.load_glm("model.glm", defines={"X": "10"}, threads=4, verbose=True)
        """
        # Backward compatibility: if first arg is a list, use old behavior
        if isinstance(filename_or_args, list):
            arguments = filename_or_args
        else:
            # Build argv-style arguments from kwargs
            # GridLAB-D expects: [filename, options...]
            arguments = [filename_or_args]
            
            # Add options after filename
            if check:
                arguments.append("--check")
            
            if debug:
                arguments.append("--debug")
            
            if debugger:
                arguments.append("--debugger")
            
            if verbose:
                arguments.append("--verbose")
            
            if warn:
                arguments.append("--warn")
            
            if quiet:
                arguments.append("--quiet")
            
            if workdir is not None:
                arguments.append("-W")
                arguments.append(workdir)
            
            if threads is not None:
                arguments.append("--threadcount")
                arguments.append(str(threads))
            
            if defines:
                for key, value in defines.items():
                    arguments.append("-D")
                    arguments.append(f"{key}={value}")
            
            if compile_only:
                arguments.append("--compile")
            
            if save is not None:
                arguments.append("--save")
                arguments.append(save)
        
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
    def run(
        self,
        start_time: Optional[float | str] = None,
        stop_time: Optional[float | str] = None,
    ) -> int:
        """Run the simulation, optionally bounding the timestamp interval.

        Args:
            start_time: Optional bound as either numeric GridLAB-D timestamp
                or ISO 8601 string.
            stop_time: Optional bound as either numeric GridLAB-D timestamp
                or ISO 8601 string.

        Note:
            The underlying C++ run API accepts numeric timestamps. String
            inputs are converted to timestamps in the wrapper before dispatch.
        """
        if self.get_object_count() == 0:
            raise RuntimeError("Cannot run simulation: no objects loaded in model")

        reference_tz = None
        if isinstance(start_time, str) or isinstance(stop_time, str):
            _, current_time = self.get_time()
            current_dt = _parse_iso_datetime_with_tz(current_time)
            reference_tz = current_dt.tzinfo if current_dt is not None else None

        start_ts = _coerce_run_bound_to_timestamp(start_time, reference_tz)
        stop_ts = _coerce_run_bound_to_timestamp(stop_time, reference_tz)

        response = self._send_command(Command.RUN, {
            "start_time": start_ts,
            "stop_time": stop_ts,
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
    
    def step(self) -> tuple[int, Optional[str]]:
        """Advance the simulation by one time step.

        Returns:
            tuple: (error_code, simulation_time) where simulation_time is an
                   ISO 8601 string in the simulation's local timezone, or None
                   if the time is a sentinel value (INIT, NEVER).
        """
        if self.get_object_count() == 0:
            raise RuntimeError("Cannot step simulation: no objects loaded in model")

        _, before_step_time = self.get_time()
        stop_time = self.get_stoptime()

        try:
            response = self._send_command(Command.STEP, {})
        except RuntimeError as exc:
            text = str(exc)
            if "processing STEP" in text and "Worker process" in text:
                # If GridLAB-D terminates the worker during stepping, surface this as
                # a step-level error so callers can handle it in loop control logic.
                print(
                    "GridLAB-D error: worker exited during step(); "
                    "returning TIME_STEP_ERROR and preserving last known time. "
                    f"Details: {text}",
                    file=sys.stderr,
                    flush=True,
                )
                from .gridlabd_core import GLDErrorCode

                return int(GLDErrorCode.TIME_STEP_ERROR.value), before_step_time
            raise
        if not response.success:
            raise RuntimeError(response.error)

        code = response.result["code"]
        step_time = gld_to_iso(response.result["time"])

        if code == 0 and _is_stoptime_blocked_step(before_step_time, step_time, stop_time):
            # Emit a default warning even when verbose=False so users can see the stop-time block.
            print(
                "GridLAB-D warning: step() was blocked at stoptime; "
                f"simulation remains at {step_time}.",
                file=sys.stderr,
                flush=True,
            )
            from .gridlabd_core import GLDErrorCode

            return int(GLDErrorCode.TIME_STEP_ERROR.value), step_time

        return code, step_time
    
    def step_to(self, target_time_str: str) -> tuple[int, Optional[str]]:
        """Step the simulation to a specific timestamp.

        Args:
            target_time_str: Target time as an ISO 8601 string.

        Returns:
            tuple: (error_code, simulation_time) where simulation_time is an
                   ISO 8601 string in the simulation's local timezone, or None
                   if the time is a sentinel value.
        """
        normalized_time = _normalize_time_input(target_time_str)
        response = self._send_command(Command.STEP_TO, {"target_time": normalized_time})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result["code"], gld_to_iso(response.result["time"])
    
    # Time management methods
    def set_time(self, timestamp: str) -> int:
        """Set the simulation time."""
        normalized_time = _normalize_time_input(timestamp)
        response = self._send_command(Command.SET_TIME, {"timestamp": normalized_time})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
    
    def get_time(self) -> tuple[int, Optional[str]]:
        """Get the current simulation time.

        Returns:
            tuple: (error_code, current_time) where current_time is an
                   ISO 8601 string in the simulation's local timezone, or None
                   if the time represents INIT or NEVER.
        """
        response = self._send_command(Command.GET_TIME, {})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result["code"], gld_to_iso(response.result["time"])
    
    def set_time_step(self, time_step: int) -> int:
        """Set the simulation time step."""
        response = self._send_command(Command.SET_TIME_STEP, {"time_step": time_step})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result

    def maintain_transient(self, enable: bool) -> int:
        """Toggle persistent transient behavior (global deltamode_forced_always)."""
        response = self._send_command(
            Command.MAINTAIN_TRANSIENT,
            {"enable": bool(enable)},
        )
        if not response.success:
            raise RuntimeError(response.error)
        return response.result

    def trigger_transient(self) -> int:
        """Trigger one-shot transient behavior for the next step opportunity."""
        response = self._send_command(Command.TRIGGER_TRANSIENT, {})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result

    def exit_transient(self) -> int:
        """Force exit transient mode back to QSTS (not recommended)."""
        response = self._send_command(Command.EXIT_TRANSIENT, {})
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
        try:
            payload = json.loads(response.result)
        except (TypeError, json.JSONDecodeError):
            return response.result

        clock = payload.get("clock") if isinstance(payload, dict) else None
        if isinstance(clock, dict):
            for key in ("starttime", "stoptime", "timestamp"):
                value = clock.get(key)
                if isinstance(value, str):
                    clock[key] = _to_iso8601(value)

        return json.dumps(payload)
    
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

    def get_object_names_by_class(self, class_name: str) -> list[str]:
        """Get all object names/IDs of a specific class."""
        response = self._send_command(Command.GET_OBJECTS_BY_CLASS, {"class_name": class_name})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result

    def get_objects_by_class(self, class_name: str) -> list[str]:
        """Compatibility alias for get_object_names_by_class()."""
        return self.get_object_names_by_class(class_name)
    
    def get_object_properties(self, object_name: str, typed: bool = True) -> dict[str, Any]:
        """Get all properties of an object as a dictionary.

        Args:
            object_name: Name or ID of the object.
            typed: When True, return native Python values with units stripped.
        """
        response = self._send_command(
            Command.GET_OBJECT_PROPERTIES,
            {"object_name": object_name, "typed": bool(typed)},
        )
        if not response.success:
            raise RuntimeError(response.error)
        if typed:
            return self._reconstruct_complex_deep(response.result)
        return response.result
    
    def get_all_objects(self, class_name: str, typed: bool = True) -> list[dict[str, Any]]:
        """Get all objects (and their properties) of a specific class.

        Args:
            class_name: Name of the class.
            typed: When True, return native Python values with units stripped.
        """
        response = self._send_command(
            Command.GET_ALL_OBJECTS,
            {"class_name": class_name, "typed": bool(typed)},
        )
        if not response.success:
            raise RuntimeError(response.error)
        if typed:
            return self._reconstruct_complex_deep(response.result)
        return response.result
    
    def get_model(self, typed: bool = True) -> dict[str, list[dict[str, Any]]]:
        """Get the entire model with all objects and properties organized by class.

        Args:
            typed: When True, return native Python values with units stripped.
        """
        response = self._send_command(Command.GET_MODEL, {"typed": bool(typed)})
        if not response.success:
            raise RuntimeError(response.error)
        if typed:
            return self._reconstruct_complex_deep(response.result)
        return response.result
    
    def get_property(self, object_name: str, property_name: str, typed: bool = True) -> tuple[int, Any]:
        """Get a property value from an object.

        Args:
            object_name: Name or ID of the object.
            property_name: Name of the property.
            typed: When True, return native Python value with units stripped.
        """
        response = self._send_command(Command.GET_PROPERTY, {
            "object_name": object_name,
            "property_name": property_name,
            "typed": bool(typed),
        })
        if not response.success:
            raise RuntimeError(response.error)
        value = response.result["value"]
        if typed:
            value = self._reconstruct_complex_deep(value)
        return response.result["code"], value
    
    def get_property_info(self, object_name: str, property_name: str) -> tuple[int, dict]:
        """Get property metadata (type, unit, description, access).
        
        Args:
            object_name: Name or ID of the object
            property_name: Name of the property
            
        Returns:
            tuple: (error_code, info_dict) where info_dict contains:
                - type: PropertyType enum value (int)
                - unit: Unit string (str, empty if no unit)
                - description: Property description (str)
                - access: Access flags (int)
        """
        response = self._send_command(Command.GET_PROPERTY_INFO, {
            "object_name": object_name,
            "property_name": property_name
        })
        if not response.success:
            raise RuntimeError(response.error)
        return response.result["code"], response.result["info"]
    
    @staticmethod
    def _reconstruct_complex(value):
        """Reconstruct a complex number from its JSON-safe dict representation."""
        if isinstance(value, dict) and value.get("__complex__"):
            return complex(value["real"], value["imag"])
        return value

    @classmethod
    def _reconstruct_complex_deep(cls, value):
        """Reconstruct complex markers recursively in lists and dictionaries."""
        value = cls._reconstruct_complex(value)
        if isinstance(value, list):
            return [cls._reconstruct_complex_deep(v) for v in value]
        if isinstance(value, dict):
            return {k: cls._reconstruct_complex_deep(v) for k, v in value.items()}
        return value

    def get_object_property_value(self, object_name: str, property_name: str):
        """Get a property value as its native Python data type (no units).
        
        Unlike :meth:`get_property` which returns a raw string with units
        appended (e.g. ``"+68.1022 degF"``), this method strips the unit
        suffix and converts the value to the appropriate Python type:
        
        - ``PT_double`` / ``PT_float`` / ``PT_real`` → ``float``
        - ``PT_complex`` → ``complex``
        - ``PT_int16`` / ``PT_int32`` / ``PT_int64`` → ``int``
        - ``PT_bool`` → ``bool``
        - ``PT_enumeration`` / ``PT_set`` / ``PT_char*`` → ``str``
        
        Args:
            object_name: Name or ID of the object
            property_name: Name of the property
            
        Returns:
            The property value as the appropriate Python data type.
            
        Raises:
            RuntimeError: If the object or property is not found.
            
        Examples:
            >>> temp = gld.get_object_property_value("house1", "air_temperature")
            >>> type(temp)
            <class 'float'>
            >>> power = gld.get_object_property_value("meter1", "measured_power")
            >>> type(power)
            <class 'complex'>
        """
        response = self._send_command(Command.GET_OBJECT_PROPERTY_VALUE, {
            "object_name": object_name,
            "property_name": property_name
        })
        if not response.success:
            raise RuntimeError(response.error)
        return self._reconstruct_complex(response.result["value"])
    
    def get_object_properties_detailed(self, object_name: str) -> dict[str, dict]:
        """Get all properties of an object with rich metadata.
        
        Returns a dictionary keyed by property name, where each value is a
        metadata dictionary with the following structure::
        
            {
                "value":       <appropriate Python type>,
                "unit":        "degF",          # empty string if unitless
                "type":        "double",         # GridLAB-D type name
                "access":      "read-only",      # or "read-write"
                "description": "indoor air temperature"
            }
        
        Values are converted to their native Python types (float, int,
        complex, bool, or str) — see :meth:`get_object_property_value`.
        
        Args:
            object_name: Name or ID of the object
            
        Returns:
            dict[str, dict]: Property metadata keyed by property name.
            
        Raises:
            RuntimeError: If the object is not found.
            
        Examples:
            >>> props = gld.get_object_properties_detailed("house1")
            >>> props["air_temperature"]["value"]
            68.1022
            >>> props["air_temperature"]["unit"]
            'degF'
            >>> props["air_temperature"]["access"]
            'read-only'
        """
        response = self._send_command(Command.GET_OBJECT_PROPERTIES_DETAILED, {
            "object_name": object_name
        })
        if not response.success:
            raise RuntimeError(response.error)
        # Reconstruct complex values in the result
        result = response.result
        for prop_name, meta in result.items():
            meta["value"] = self._reconstruct_complex(meta["value"])
        return result
    
    def set_property(self, object_name: str, property_name: str, value) -> int:
        """Set a property value on an object.
        
        Args:
            object_name: Name or ID of the object
            property_name: Name of the property
            value: Value to set - can be str, int, float, bool, or complex
                   Native Python types are automatically converted to GridLAB-D format
        
        Returns:
            Error code (0 for success)
        """
        # Convert Python native types to strings for C++ binding
        if isinstance(value, bool):
            # Handle bool before int since bool is subclass of int
            str_value = "TRUE" if value else "FALSE"
        elif isinstance(value, complex):
            # Convert complex to GridLAB-D format: "real+imagj" or "real-imagj"
            if value.imag >= 0:
                str_value = f"{value.real}+{value.imag}j"
            else:
                str_value = f"{value.real}{value.imag}j"
        elif isinstance(value, (int, float)):
            str_value = str(value)
        elif isinstance(value, str):
            str_value = value
        else:
            # Let it through and see what happens
            str_value = str(value)
        
        response = self._send_command(Command.SET_PROPERTY, {
            "object_name": object_name,
            "property_name": property_name,
            "value": str_value
        })
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
    
    def get_properties_by_class(self, class_name: str, property_name: str, typed: bool = True) -> dict[str, Any]:
        """Get property values from all objects of a class.

        Args:
            class_name: Name of the class.
            property_name: Name of the property.
            typed: When True, return native Python values with units stripped.
        """
        response = self._send_command(Command.GET_PROPERTIES_BY_CLASS, {
            "class_name": class_name,
            "property_name": property_name,
            "typed": bool(typed),
        })
        if not response.success:
            raise RuntimeError(response.error)
        if typed:
            return self._reconstruct_complex_deep(response.result)
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
        """Set a global variable."""
        response = self._send_command(Command.SET_GLOBAL, {"name": name, "value": value})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
    
    def global_getvar(self, name: str) -> str:
        """Get a global variable."""
        response = self._send_command(Command.GET_GLOBAL, {"name": name})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
    
    # Convenience methods for clock properties
    def get_clock(self) -> Optional[str]:
        """Get the current simulation time.

        Returns:
            ISO 8601 string in the simulation's local timezone, or None
            if the clock has not been initialized.
        """
        return gld_to_iso(self.global_getvar("clock"), timezone_hint=self.get_timezone())

    def get_starttime(self) -> Optional[str]:
        """Get the simulation start time.

        Returns:
            ISO 8601 string in the simulation's local timezone, or None
            if start time is not set.
        """
        return gld_to_iso(self.global_getvar("starttime"), timezone_hint=self.get_timezone())

    def get_stoptime(self) -> Optional[str]:
        """Get the simulation stop time.

        Returns:
            ISO 8601 string in the simulation's local timezone, or None
            if stop time is NEVER (unbounded simulation).
        """
        return gld_to_iso(self.global_getvar("stoptime"), timezone_hint=self.get_timezone())
    
    def set_starttime(self, value: str) -> int:
        """Set the simulation start time.
        
        Args:
            value: Start time as string (ISO 8601 format or timestamp)
            
        Returns:
            Error code (0 for success)
        """
        return self.global_setvar("starttime", _normalize_time_input(value))
    
    def set_stoptime(self, value: str) -> int:
        """Set the simulation stop time.
        
        Args:
            value: Stop time as string (ISO 8601 format or timestamp)
            
        Returns:
            Error code (0 for success)
        """
        return self.global_setvar("stoptime", _normalize_time_input(value))
    
    def get_timezone(self) -> str:
        """Get the current timezone.
        
        Returns:
            Timezone string
        """
        return self.global_getvar("timezone")
    
    # Message capture methods
    def get_messages(self) -> list[dict[str, str]]:
        """Get all captured warning/error/debug messages."""
        response = self._send_command(Command.GET_MESSAGES, {})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
    
    def clear_messages(self) -> None:
        """Clear all captured messages."""
        response = self._send_command(Command.CLEAR_MESSAGES, {})
        if not response.success:
            raise RuntimeError(response.error)
    
    def enable_message_capture(self, enable: bool) -> None:
        """Enable or disable message capture."""
        response = self._send_command(Command.ENABLE_MESSAGE_CAPTURE, {"enable": enable})
        if not response.success:
            raise RuntimeError(response.error)
    
    def set_message_capture_limit(self, limit: int) -> None:
        """Set maximum number of messages to capture."""
        response = self._send_command(Command.SET_MESSAGE_CAPTURE_LIMIT, {"limit": limit})
        if not response.success:
            raise RuntimeError(response.error)
    
    def get_message_capture_limit(self) -> int:
        """Get current message capture limit."""
        response = self._send_command(Command.GET_MESSAGE_CAPTURE_LIMIT, {})
        if not response.success:
            raise RuntimeError(response.error)
        return response.result
