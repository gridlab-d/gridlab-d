"""Regression tests for run_bug.py workflow."""

from __future__ import annotations

from datetime import datetime
import re
from pathlib import Path


def _parse_gld_time(time_str: str) -> datetime:
    match = re.search(r"(\d{4}-\d{2}-\d{2})[T ](\d{2}:\d{2}:\d{2})", time_str)
    if not match:
        raise ValueError(f"Unrecognized time format: {time_str}")
    return datetime.strptime(f"{match.group(1)} {match.group(2)}", "%Y-%m-%d %H:%M:%S")


def test_run_house_with_solar_one_hour(gld_instance):
    model_dir = Path(__file__).parent.parent / "house_with_solar"
    gld_instance.set_working_directory(str(model_dir))

    assert gld_instance.load("houses.glm") == 0

    start_code, start_time = gld_instance.get_time()
    assert start_code == 0

    run_result = gld_instance.run()
    assert run_result == 0

    stop_code, stop_time = gld_instance.get_time()
    assert stop_code == 0

    start_dt = _parse_gld_time(start_time)
    stop_dt = _parse_gld_time(stop_time)
    assert (stop_dt - start_dt).total_seconds() == 3600
