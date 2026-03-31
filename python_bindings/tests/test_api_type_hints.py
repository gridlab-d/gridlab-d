"""Regression checks for public API type hints in the isolated wrapper."""

import inspect
import types
import typing
from typing import Optional, get_args, get_origin

import gridlabd


def _is_tuple_int_optional_str(annotation) -> bool:
    origin = get_origin(annotation)
    args = get_args(annotation)
    if origin is not tuple or len(args) != 2:
        return False
    if args[0] is not int:
        return False
    second_origin = get_origin(args[1])
    second_args = set(get_args(args[1]))
    return second_origin in (typing.Union, types.UnionType) and second_args == {str, type(None)}


def test_time_api_signatures_use_iso_strings():
    """step/get_time/step_to should expose ISO-string time values in signatures."""
    step_ret = inspect.signature(gridlabd.GridLabD.step).return_annotation
    get_time_ret = inspect.signature(gridlabd.GridLabD.get_time).return_annotation
    step_to_ret = inspect.signature(gridlabd.GridLabD.step_to).return_annotation

    assert _is_tuple_int_optional_str(step_ret)
    assert _is_tuple_int_optional_str(get_time_ret)
    assert _is_tuple_int_optional_str(step_to_ret)


def test_run_signature_remains_numeric_bounds():
    """run() should remain numeric-bound to match core C++ API."""
    sig = inspect.signature(gridlabd.GridLabD.run)
    assert sig.parameters["start_time"].annotation == Optional[float]
    assert sig.parameters["stop_time"].annotation == Optional[float]
