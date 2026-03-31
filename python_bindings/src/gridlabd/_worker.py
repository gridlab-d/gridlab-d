"""
Worker process for isolated GridLabD instances.

This module runs in a subprocess and handles commands from the main process,
executing them against a real GridLabD C++ instance and returning results.
"""

import sys
import json
from typing import Any

from ._protocol import Command, Message, Response
import re

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
        
        # CRITICAL: Enable message capture to prevent GridLAB-D from writing to stdout
        # This would corrupt the JSON protocol between worker and parent
        _gld_instance.enable_message_capture(True)
        
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
        iso_time = _to_iso8601(simulation_time)
        return Response(success=True, result={"code": int(code) if isinstance(code, int) else int(code.value), "time": iso_time})
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_step_to(message: Message) -> Response:
    """Step to a specific time."""
    try:
        target_time = message.args["target_time"]
        code, simulation_time = _gld_instance.step_to(target_time)
        iso_time = _to_iso8601(simulation_time)
        return Response(success=True, result={"code": int(code) if isinstance(code, int) else int(code.value), "time": iso_time})
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
        iso_time = _to_iso8601(current_time)
        return Response(success=True, result={"code": int(code) if isinstance(code, int) else int(code.value), "time": iso_time})
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_set_time_step(message: Message) -> Response:
    """Set the simulation time step."""
    try:
        code = _gld_instance.set_time_step(message.args["time_step"])
        return Response(success=True, result=int(code) if isinstance(code, int) else int(code.value))
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_maintain_transient(message: Message) -> Response:
    """Toggle persistent transient (deltamode) behavior."""
    try:
        code = _gld_instance.maintain_transient(message.args["enable"])
        return Response(success=True, result=int(code) if isinstance(code, int) else int(code.value))
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_trigger_transient(message: Message) -> Response:
    """Trigger one-shot transient (deltamode) behavior."""
    try:
        code = _gld_instance.trigger_transient()
        return Response(success=True, result=int(code) if isinstance(code, int) else int(code.value))
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_exit_transient(message: Message) -> Response:
    """Force exit from transient (deltamode) behavior."""
    try:
        code = _gld_instance.exit_transient()
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
        print("DEBUG _worker: handle_get_checkpoint_json called", file=sys.stderr, flush=True)
        filepath = message.args.get("filepath", "")
        print(f"DEBUG _worker: filepath='{filepath}'", file=sys.stderr, flush=True)
        print("DEBUG _worker: calling _gld_instance.get_checkpoint_json", file=sys.stderr, flush=True)
        result = _gld_instance.get_checkpoint_json(filepath)
        print(f"DEBUG _worker: got result, length={len(result)}", file=sys.stderr, flush=True)
        return Response(success=True, result=result)
    except Exception as e:
        print(f"DEBUG _worker: Exception: {e}", file=sys.stderr, flush=True)
        import traceback
        traceback.print_exc(file=sys.stderr)
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
        object_name = message.args["object_name"]
        result = _gld_instance.get_object_properties(object_name)
        if bool(message.args.get("typed", False)) and isinstance(result, dict):
            result = _convert_typed_property_map(object_name, result)
        return Response(success=True, result=result)
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_get_all_objects(message: Message) -> Response:
    """Get all objects (and their properties) of a specific class."""
    try:
        result = _gld_instance.get_all_objects(message.args["class_name"])
        if bool(message.args.get("typed", False)) and isinstance(result, list):
            typed_objects = []
            for obj_props in result:
                if not isinstance(obj_props, dict):
                    typed_objects.append(obj_props)
                    continue
                obj_name = obj_props.get("__name__") or obj_props.get("__id__")
                if isinstance(obj_name, str) and obj_name:
                    typed_objects.append(_convert_typed_property_map(obj_name, obj_props))
                else:
                    typed_objects.append(obj_props)
            result = typed_objects
        return Response(success=True, result=result)
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_get_model(message: Message) -> Response:
    """Get the entire model with all objects and properties organized by class."""
    try:
        result = _gld_instance.get_model()
        if bool(message.args.get("typed", False)) and isinstance(result, dict):
            typed_model = {}
            for class_name, objects in result.items():
                if not isinstance(objects, list):
                    typed_model[class_name] = objects
                    continue
                typed_objects = []
                for obj_props in objects:
                    if not isinstance(obj_props, dict):
                        typed_objects.append(obj_props)
                        continue
                    obj_name = obj_props.get("__name__") or obj_props.get("__id__")
                    if isinstance(obj_name, str) and obj_name:
                        typed_objects.append(_convert_typed_property_map(obj_name, obj_props))
                    else:
                        typed_objects.append(obj_props)
                typed_model[class_name] = typed_objects
            result = typed_model
        return Response(success=True, result=result)
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_get_property(message: Message) -> Response:
    """Get a property value."""
    try:
        object_name = message.args["object_name"]
        property_name = message.args["property_name"]
        code, value = _gld_instance.get_property(
            object_name,
            property_name
        )
        code_int = int(code) if isinstance(code, int) else int(code.value)
        if bool(message.args.get("typed", False)) and code_int == 0:
            prop_type, unit = _get_prop_type_unit(object_name, property_name)
            value = _convert_value(value, prop_type, unit)
        return Response(success=True, result={"code": code_int, "value": value})
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_get_property_info(message: Message) -> Response:
    """Get property metadata (type, unit, description)."""
    try:
        # Check if the method exists on the C++ binding
        if not hasattr(_gld_instance, 'get_property_info'):
            return Response(success=False, error="'gridlabd.gridlabd_core.GridLabD' object has no attribute 'get_property_info'")
        
        code, info = _gld_instance.get_property_info(
            message.args["object_name"],
            message.args["property_name"]
        )
        return Response(success=True, result={"code": int(code) if isinstance(code, int) else int(code.value), "info": info})
    except Exception as e:
        return Response(success=False, error=str(e))


# --- Property type enum values (must match gldcore/property.h PROPERTYTYPE) ---
_PT_VOID        = 0
_PT_DOUBLE      = 1
_PT_COMPLEX     = 2
_PT_ENUMERATION = 3
_PT_SET         = 4
_PT_INT16       = 5
_PT_INT32       = 6
_PT_UINT32      = 7
_PT_INT64       = 8
_PT_CHAR8       = 9
_PT_CHAR32      = 10
_PT_CHAR256     = 11
_PT_CHAR1024    = 12
_PT_OBJECT      = 13
_PT_DELEGATED   = 14
_PT_BOOL        = 15
_PT_TIMESTAMP   = 16
_PT_DOUBLE_ARRAY = 17
_PT_COMPLEX_ARRAY = 18
_PT_REAL        = 19
_PT_FLOAT       = 20

_FLOAT_TYPES = {_PT_DOUBLE, _PT_REAL, _PT_FLOAT}
_INT_TYPES   = {_PT_INT16, _PT_INT32, _PT_UINT32, _PT_INT64}
_STR_TYPES   = {_PT_CHAR8, _PT_CHAR32, _PT_CHAR256, _PT_CHAR1024,
                _PT_OBJECT, _PT_DELEGATED, _PT_ENUMERATION, _PT_SET}

_PA_W = 0x02  # write access bit (from property.h)
_META_KEYS = {"__class__", "__id__", "__name__"}

_TYPE_NAMES = {
    _PT_VOID: "void",
    _PT_DOUBLE: "double",
    _PT_COMPLEX: "complex",
    _PT_ENUMERATION: "enumeration",
    _PT_SET: "set",
    _PT_INT16: "int16",
    _PT_INT32: "int32",
    _PT_UINT32: "uint32",
    _PT_INT64: "int64",
    _PT_CHAR8: "char8",
    _PT_CHAR32: "char32",
    _PT_CHAR256: "char256",
    _PT_CHAR1024: "char1024",
    _PT_OBJECT: "object",
    _PT_DELEGATED: "delegated",
    _PT_BOOL: "bool",
    _PT_TIMESTAMP: "timestamp",
    _PT_DOUBLE_ARRAY: "double_array",
    _PT_COMPLEX_ARRAY: "complex_array",
    _PT_REAL: "real",
    _PT_FLOAT: "float",
}


def _strip_unit_suffix(raw_value: str, unit: str) -> str:
    """Strip the unit suffix from a raw property value string.

    GridLAB-D's ``object_get_value_by_name`` appends the unit to the
    formatted value (e.g. ``"+68.1022 degF"``).  We strip it so the
    remaining text can be parsed as a number.
    """
    s = raw_value.strip()
    if unit and s.endswith(unit):
        s = s[: -len(unit)].rstrip()
    return s


def _convert_value(raw_value: str, prop_type: int, unit: str):
    """Convert a raw GridLAB-D value string to a native Python type.

    Returns a JSON-safe value (int, float, bool, str, or a dict for complex).
    """
    s = _strip_unit_suffix(raw_value, unit)

    try:
        if prop_type in _FLOAT_TYPES:
            return float(s)

        if prop_type == _PT_COMPLEX:
            # GridLAB-D complex formats:
            #   "+120+0.5j"  (rectangular)
            #   "+120-0.5j"
            #   "120+0.5d"   (polar degrees – rare in API output)
            # Remove any trailing notation char other than 'j'
            cleaned = s.rstrip()
            if cleaned.endswith(('d', 'r')):
                cleaned = cleaned[:-1] + 'j'
            if not cleaned.endswith('j'):
                cleaned += '+0j'
            c = complex(cleaned.replace(' ', ''))
            return {"__complex__": True, "real": c.real, "imag": c.imag}

        if prop_type in _INT_TYPES:
            # Values may contain a decimal point (e.g. "3.000"), truncate
            return int(float(s))

        if prop_type == _PT_BOOL:
            return s.upper() in ("TRUE", "1", "YES")

        # Strings, enumerations, sets, timestamps, etc.
        return s

    except (ValueError, TypeError):
        # Fall back to the raw (unit-stripped) string
        return s


def _get_prop_type_unit(obj_name: str, prop_name: str) -> tuple[int, str]:
    """Get the property type/unit metadata for one object property."""
    if not hasattr(_gld_instance, 'get_property_info'):
        return _PT_VOID, ""

    try:
        code_info, info = _gld_instance.get_property_info(obj_name, prop_name)
        info_int = int(code_info) if isinstance(code_info, int) else int(code_info.value)
        if info_int == 0:
            return info.get("type", _PT_VOID), info.get("unit", "")
    except Exception:
        pass

    return _PT_VOID, ""


def _convert_typed_property_map(obj_name: str, props: dict[str, str]) -> dict[str, Any]:
    """Convert property map values to native Python types without units."""
    result = {}
    for prop_name, raw_value in props.items():
        if prop_name in _META_KEYS:
            result[prop_name] = raw_value
            continue
        prop_type, unit = _get_prop_type_unit(obj_name, prop_name)
        result[prop_name] = _convert_value(raw_value, prop_type, unit)
    return result


def handle_get_object_property_value(message: Message) -> Response:
    """Get a single property value converted to its native Python type."""
    try:
        obj_name = message.args["object_name"]
        prop_name = message.args["property_name"]

        # 1. Get raw string value
        code_val, raw_value = _gld_instance.get_property(obj_name, prop_name)
        code_int = int(code_val) if isinstance(code_val, int) else int(code_val.value)
        if code_int != 0:
            return Response(success=False,
                            error=f"Failed to get property '{prop_name}' from object '{obj_name}'")

        # 2. Get property metadata (type, unit)
        prop_type = _PT_VOID
        unit = ""
        if hasattr(_gld_instance, 'get_property_info'):
            code_info, info = _gld_instance.get_property_info(obj_name, prop_name)
            info_int = int(code_info) if isinstance(code_info, int) else int(code_info.value)
            if info_int == 0:
                prop_type = info.get("type", _PT_VOID)
                unit = info.get("unit", "")

        # 3. Convert
        typed_value = _convert_value(raw_value, prop_type, unit)

        return Response(success=True, result={
            "code": 0,
            "value": typed_value,
        })
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_get_object_properties_detailed(message: Message) -> Response:
    """Get all properties of an object with metadata (value, unit, type, access)."""
    try:
        obj_name = message.args["object_name"]

        # 1. Get all raw property strings
        raw_props = _gld_instance.get_object_properties(obj_name)
        if not raw_props:
            return Response(success=False,
                            error=f"Object '{obj_name}' not found or has no properties")

        result = {}
        has_info = hasattr(_gld_instance, 'get_property_info')

        for prop_name, raw_value in raw_props.items():
            # Skip internal metadata keys
            if prop_name.startswith("__") and prop_name.endswith("__"):
                continue

            prop_type = _PT_VOID
            unit = ""
            description = ""
            access_flags = 0x0F  # default PA_PUBLIC (R|W|S|L)
            type_name = "unknown"

            if has_info:
                try:
                    code_info, info = _gld_instance.get_property_info(obj_name, prop_name)
                    info_int = int(code_info) if isinstance(code_info, int) else int(code_info.value)
                    if info_int == 0:
                        prop_type = info.get("type", _PT_VOID)
                        unit = info.get("unit", "")
                        description = info.get("description", "")
                        access_flags = info.get("access", 0x0F)
                        type_name = _TYPE_NAMES.get(prop_type, "unknown")
                except Exception:
                    pass

            typed_value = _convert_value(raw_value, prop_type, unit)

            access_str = "read-write" if (access_flags & _PA_W) else "read-only"

            result[prop_name] = {
                "value": typed_value,
                "unit": unit,
                "type": type_name,
                "access": access_str,
                "description": description,
            }

        return Response(success=True, result=result)
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
        if bool(message.args.get("typed", False)) and isinstance(result, dict):
            property_name = message.args["property_name"]
            typed_result = {}
            for obj_name, raw_value in result.items():
                prop_type, unit = _get_prop_type_unit(obj_name, property_name)
                typed_result[obj_name] = _convert_value(raw_value, prop_type, unit)
            result = typed_result
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
    """Set a global variable."""
    try:
        result = _gld_instance.set_global(
            message.args["name"],
            message.args["value"]
        )
        return Response(success=True, result=result)
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_get_global(message: Message) -> Response:
    """Get a global variable."""
    try:
        result = _gld_instance.get_global(message.args["name"])
        return Response(success=True, result=result)
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_get_messages(message: Message) -> Response:
    """Get all captured warning/error/debug messages."""
    try:
        result = _gld_instance.get_messages()
        if isinstance(result, list):
            for entry in result:
                if isinstance(entry, dict):
                    timestamp = entry.get("timestamp")
                    if isinstance(timestamp, str) and timestamp.strip():
                        entry["timestamp"] = _to_iso8601(timestamp)
        return Response(success=True, result=result)
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_clear_messages(message: Message) -> Response:
    """Clear all captured messages."""
    try:
        _gld_instance.clear_messages()
        return Response(success=True, result=None)
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_enable_message_capture(message: Message) -> Response:
    """Enable or disable message capture."""
    try:
        _gld_instance.enable_message_capture(message.args["enable"])
        return Response(success=True, result=None)
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_set_message_capture_limit(message: Message) -> Response:
    """Set maximum number of messages to capture."""
    try:
        _gld_instance.set_message_capture_limit(message.args["limit"])
        return Response(success=True, result=None)
    except Exception as e:
        return Response(success=False, error=str(e))


def handle_get_message_capture_limit(message: Message) -> Response:
    """Get current message capture limit."""
    try:
        result = _gld_instance.get_message_capture_limit()
        return Response(success=True, result=result)
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
    Command.MAINTAIN_TRANSIENT: handle_maintain_transient,
    Command.TRIGGER_TRANSIENT: handle_trigger_transient,
    Command.EXIT_TRANSIENT: handle_exit_transient,
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
    Command.GET_ALL_OBJECTS: handle_get_all_objects,
    Command.GET_MODEL: handle_get_model,
    Command.GET_PROPERTY: handle_get_property,
    Command.GET_PROPERTY_INFO: handle_get_property_info,
    Command.GET_OBJECT_PROPERTY_VALUE: handle_get_object_property_value,
    Command.GET_OBJECT_PROPERTIES_DETAILED: handle_get_object_properties_detailed,
    Command.SET_PROPERTY: handle_set_property,
    Command.GET_PROPERTIES_BY_CLASS: handle_get_properties_by_class,
    Command.SET_PROPERTY_BY_CLASS: handle_set_property_by_class,
    Command.SET_GLOBAL: handle_set_global,
    Command.GET_GLOBAL: handle_get_global,
    Command.GET_MESSAGES: handle_get_messages,
    Command.CLEAR_MESSAGES: handle_clear_messages,
    Command.ENABLE_MESSAGE_CAPTURE: handle_enable_message_capture,
    Command.SET_MESSAGE_CAPTURE_LIMIT: handle_set_message_capture_limit,
    Command.GET_MESSAGE_CAPTURE_LIMIT: handle_get_message_capture_limit,
}


def main():
    """Main worker loop - reads commands from stdin, executes them, writes responses to stdout."""
    # CRITICAL: Redirect C++ stdout to stderr to prevent GridLAB-D debug output
    # from corrupting the JSON protocol on stdout
    import os
    # Duplicate stdout to a safe place, then redirect stdout fd to stderr
    original_stdout_fd = os.dup(1)  # Save original stdout
    os.dup2(2, 1)  # Redirect stdout (fd 1) to stderr (fd 2)
    
    # Create a Python file object from the saved stdout for protocol communication
    protocol_out = os.fdopen(original_stdout_fd, 'w', buffering=1)
    
    # Send READY signal to parent to indicate worker is ready to receive commands
    protocol_out.write("READY\n")
    protocol_out.flush()
    
    for line in sys.stdin:
        try:
            message = Message.from_json(line.strip())
            handler = COMMAND_HANDLERS.get(message.command)
            
            if handler is None:
                response = Response(success=False, error=f"Unknown command: {message.command}")
            else:
                response = handler(message)
            
            protocol_out.write(response.to_json() + "\n")
            protocol_out.flush()
            if message.command in (Command.EXIT_GLD, Command.FINALIZE):
                break
        except Exception as e:
            response = Response(success=False, error=f"Worker error: {str(e)}")
            protocol_out.write(response.to_json() + "\n")
            protocol_out.flush()



if __name__ == "__main__":
    main()
