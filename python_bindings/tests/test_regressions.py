"""Regression tests for reported GridLAB-D Python binding issues."""

from __future__ import annotations

from datetime import datetime, timedelta
from pathlib import Path

import pytest


def _parse_gld_time(time_str: str) -> datetime:
    parts = time_str.rsplit(" ", 1)
    if len(parts) == 2 and parts[1].isalpha():
        time_str = parts[0]
    return datetime.strptime(time_str, "%Y-%m-%d %H:%M:%S")


def test_step_respects_fixed_timestep(gld_instance, test_models_dir):
    """Ensure step() advances exactly by the configured timestep."""
    model_path = test_models_dir / "minimal.glm"
    assert gld_instance.load(str(model_path)) == 0

    assert gld_instance.set_time_step(900) == 0

    status1, time1 = gld_instance.get_time()
    assert status1 >= 0

    status_step, _ = gld_instance.step()
    assert status_step >= 0

    status2, time2 = gld_instance.get_time()
    assert status2 >= 0

    delta_seconds = (_parse_gld_time(time2) - _parse_gld_time(time1)).total_seconds()
    assert delta_seconds == pytest.approx(900.0, abs=1e-6)


def test_step_to_accepts_iso8601_and_hits_target(gld_instance):
    """Ensure step_to accepts ISO 8601 and lands on the target time."""
    model_dir = Path(__file__).resolve().parents[1] / "house_with_solar"
    gld_instance.set_working_directory(str(model_dir))
    assert gld_instance.load("houses.glm") == 0

    status1, time1 = gld_instance.get_time()
    assert status1 >= 0

    base_time = _parse_gld_time(time1)
    target_time = base_time + timedelta(minutes=30)
    target_str = target_time.isoformat(timespec="seconds")

    status_step, _ = gld_instance.step_to(target_str)
    assert status_step >= 0

    status2, time2 = gld_instance.get_time()
    assert status2 >= 0

    final_time = _parse_gld_time(time2)
    assert (final_time - target_time).total_seconds() == pytest.approx(0.0, abs=1e-6)
