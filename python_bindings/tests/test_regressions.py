"""Regression tests for reported GridLAB-D Python binding issues."""

from __future__ import annotations

from datetime import datetime, timedelta
import re
from pathlib import Path

import pytest
import gridlabd


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

    dt1 = datetime.fromisoformat(time1)
    dt2 = datetime.fromisoformat(time2)
    delta_seconds = (dt2 - dt1).total_seconds()
    assert delta_seconds == pytest.approx(900.0, abs=1e-6)


def test_step_to_accepts_iso8601_and_hits_target(gld_instance):
    """Ensure step_to accepts ISO 8601 and lands on the target time."""
    model_dir = Path(__file__).resolve().parents[1] / "house_with_solar"
    gld_instance.set_working_directory(str(model_dir))
    assert gld_instance.load("houses.glm") == 0

    status1, time1 = gld_instance.get_time()
    assert status1 >= 0

    base_time = datetime.fromisoformat(time1)
    target_time = base_time + timedelta(minutes=30)
    # Strip timezone for step_to - C++ convert_to_timestamp_delta doesn't support TZ offsets
    target_str = target_time.replace(tzinfo=None).isoformat(timespec="seconds")

    status_step, _ = gld_instance.step_to(target_str)
    assert status_step >= 0

    status2, time2 = gld_instance.get_time()
    assert status2 >= 0

    final_time = datetime.fromisoformat(time2)
    # Strip timezone for comparison if present
    if final_time.tzinfo:
        final_time = final_time.replace(tzinfo=None)
    if target_time.tzinfo:
        target_time = target_time.replace(tzinfo=None)
    assert (final_time - target_time).total_seconds() == pytest.approx(0.0, abs=1e-6)


def test_step_does_not_exceed_stoptime(gld_instance, test_models_dir):
    """Ensure step() does not advance beyond the clock stoptime."""
    model_path = test_models_dir / "minimal.glm"
    assert gld_instance.load(str(model_path)) == 0

    # Force a step larger than the 1-hour window to verify it clamps to stoptime.
    assert gld_instance.set_time_step(4000) == 0

    status, _ = gld_instance.step()
    assert status >= 0

    status_time, time_str = gld_instance.get_time()
    assert status_time >= 0

    final_time = datetime.fromisoformat(time_str)
    expected_stop = datetime(2020, 1, 1, 1, 0, 0)
    assert final_time == expected_stop


def test_get_time_returns_iso8601(gld_instance, test_models_dir):
    """Ensure get_time() returns an ISO 8601 timestamp string."""
    model_path = test_models_dir / "minimal.glm"
    assert gld_instance.load(str(model_path)) == 0

    status, time_str = gld_instance.get_time()
    assert status >= 0

    # Expect ISO 8601 like 2020-01-01T00:00:00 or with timezone offset
    iso_pattern = r"^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(?:Z|[+-]\d{2}:\d{2})?$"
    assert re.match(iso_pattern, time_str), f"Non-ISO timestamp: {time_str}"


def test_step_past_stoptime_returns_error_and_warns(gld_instance, test_models_dir, capsys):
    """Stepping at stoptime should return TIME_STEP_ERROR and print a warning."""
    model_path = test_models_dir / "minimal.glm"
    assert gld_instance.load(str(model_path)) == 0

    assert gld_instance.set_time_step(4000) == 0

    status1, stop_time = gld_instance.step()
    assert status1 >= 0

    status2, time2 = gld_instance.step()
    assert status2 == int(gridlabd.GLDErrorCode.TIME_STEP_ERROR.value)
    assert time2 == stop_time

    captured = capsys.readouterr()
    assert "blocked at stoptime" in captured.err.lower()


def test_step_to_subseconds_triggers_delta_branch(gld_instance, test_models_dir):
    """Fractional step_to should trigger at least one delta branch entry."""
    model_path = test_models_dir / "minimal.glm"
    assert gld_instance.load(str(model_path)) == 0

    before = int(gld_instance.global_getvar("api_delta_trigger_count") or "0")

    status, _ = gld_instance.step_to("2020-01-01T00:00:05.250000")
    assert status >= 0

    after = int(gld_instance.global_getvar("api_delta_trigger_count") or "0")
    assert after > before, (
        f"Expected api_delta_trigger_count to increase, but before={before}, after={after}"
    )


def test_step_does_not_exceed_stoptime(gld_instance, test_models_dir):
    """Ensure step() does not advance beyond the clock stoptime."""
    model_path = test_models_dir / "minimal.glm"
    assert gld_instance.load(str(model_path)) == 0

    # Force a step larger than the 1-hour window to verify it clamps to stoptime.
    assert gld_instance.set_time_step(4000) == 0

    status, _ = gld_instance.step()
    assert status >= 0

    status_time, time_str = gld_instance.get_time()
    assert status_time >= 0

    final_time = datetime.fromisoformat(time_str)
    # Strip timezone for comparison if present
    if final_time.tzinfo:
        final_time = final_time.replace(tzinfo=None)
    expected_stop = datetime(2020, 1, 1, 1, 0, 0)
    assert final_time == expected_stop


def test_get_time_returns_iso8601(gld_instance, test_models_dir):
    """Ensure get_time() returns an ISO 8601 timestamp string."""
    model_path = test_models_dir / "minimal.glm"
    assert gld_instance.load(str(model_path)) == 0

    status, time_str = gld_instance.get_time()
    assert status >= 0

    # Expect ISO 8601 like 2020-01-01T00:00:00 or with timezone offset
    iso_pattern = r"^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(?:Z|[+-]\d{2}:\d{2})?$"
    assert re.match(iso_pattern, time_str), f"Non-ISO timestamp: {time_str}"
