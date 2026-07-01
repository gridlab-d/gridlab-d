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


def _load_house_with_solar_for_step(gld, model_dir: Path):
    """Load and initialize a house_with_solar* model for step/get_property tests."""
    if not model_dir.exists():
        pytest.skip(f"Model directory not found: {model_dir}")
    gld.set_working_directory(str(model_dir))
    assert gld.load("houses.glm") == 0
    assert gld.set_time_step(300) == 0
    assert gld.setup_after_load() == 0
    status, _ = gld.step()
    assert status >= 0


def test_network_node_measured_real_power_falls_back_to_distribution_power(gld_instance):
    """Regression: measured_real_power should succeed on feeder network_node objects."""
    model_dir = Path(__file__).resolve().parents[2] / "docs" / "examples" / "api" / "house_with_solar"
    _load_house_with_solar_for_step(gld_instance, model_dir)

    # This model reproduces the regression because measured_real_power is not
    # directly published, while a phase-specific distribution_power_* is.
    dist_a_code, dist_a_value = gld_instance.get_property(
        "network_node", "distribution_power_A", typed=False
    )
    assert dist_a_code == 0

    # These are intentionally absent in this model for network_node.
    for missing_prop in ("distribution_power", "measured_power", "power"):
        missing_code, _ = gld_instance.get_property("network_node", missing_prop, typed=False)
        assert missing_code != 0

    measured_code, measured_value = gld_instance.get_property(
        "network_node", "measured_real_power", typed=False
    )

    # Prior to the regression fix this was code=3 on house_with_solar.
    assert measured_code == 0
    assert measured_value == dist_a_value


def test_network_node_measured_real_power_typed_uses_fallback_type(gld_instance):
    """Regression: typed get_property should use the fallback property's type metadata."""
    model_dir = Path(__file__).resolve().parents[2] / "docs" / "examples" / "api" / "house_with_solar"
    _load_house_with_solar_for_step(gld_instance, model_dir)

    distribution_code, distribution_value = gld_instance.get_property(
        "network_node", "distribution_power_A", typed=True
    )
    assert distribution_code == 0

    measured_code, measured_value = gld_instance.get_property(
        "network_node", "measured_real_power", typed=True
    )
    assert measured_code == 0
    assert measured_value == distribution_value
    assert isinstance(measured_value, complex)


def test_measured_real_power_fallback_works_across_multiple_instances():
    """Regression: measured_real_power should work across parallel-style model runs."""
    model_dirs = [
        Path(__file__).resolve().parents[2] / "docs" / "examples" / "api" / "house_with_solar",
        Path(__file__).resolve().parents[2] / "docs" / "examples" / "api" / "house_with_solar2",
        Path(__file__).resolve().parents[2] / "docs" / "examples" / "api" / "house_with_solar3",
    ]
    missing = [str(p) for p in model_dirs if not p.exists()]
    if missing:
        pytest.skip(f"Parallel reproducer model folders missing: {missing}")

    instances = [gridlabd.GridLabD(), gridlabd.GridLabD(), gridlabd.GridLabD()]
    try:
        for gld, model_dir in zip(instances, model_dirs):
            _load_house_with_solar_for_step(gld, model_dir)
            code, value = gld.get_property("network_node", "measured_real_power", typed=True)
            assert code == 0
            assert value not in (None, "")
    finally:
        for gld in instances:
            del gld


def test_get_properties_by_class_name_returns_object_names(gld_instance):
    """Regression: class-level query for property 'name' should not return empty."""
    model_dir = Path(__file__).resolve().parents[2] / "docs" / "examples" / "api" / "house_with_solar"
    if not model_dir.exists():
        pytest.skip(f"Model directory not found: {model_dir}")

    gld_instance.set_working_directory(str(model_dir))
    assert gld_instance.load("houses.glm") == 0

    house_names = gld_instance.get_properties_by_class(class_name="house", property_name="name")
    assert isinstance(house_names, dict)
    assert len(house_names) > 0

    # Ensure the map is object_name -> name and values are non-empty.
    for obj_name, value in house_names.items():
        assert isinstance(value, str)
        assert value != ""
        assert value == obj_name


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
