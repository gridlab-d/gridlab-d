"""Regression tests for thermostat setpoint normalization behavior."""

from __future__ import annotations

from pathlib import Path

import pytest


def _load_house_with_solar(gld_instance):
    model_dir = Path(__file__).parent.parent / "house_with_solar"
    gld_instance.set_working_directory(str(model_dir))
    assert gld_instance.load("houses.glm") == 0
    assert gld_instance.setup_after_load() == 0


def test_set_property_detailed_reports_normalization(gld_instance):
    _load_house_with_solar(gld_instance)

    houses = gld_instance.get_objects_by_class("house")
    if "house5" not in houses:
        pytest.skip("house5 not available in house_with_solar model")

    heat_code, heating = gld_instance.get_property("house5", "heating_setpoint")
    deadband_code, deadband = gld_instance.get_property("house5", "thermostat_deadband")
    assert heat_code == 0
    assert deadband_code == 0

    requested_value = float(heating) - 9.0
    expected_value = float(heating) + float(deadband)

    details = gld_instance.set_property(
        "house5",
        "cooling_setpoint",
        requested_value,
        detailed=True,
    )

    assert details["code"] == 0
    assert details["normalized"] is True
    assert float(details["applied_value"]) == pytest.approx(expected_value)

    get_code, actual_cooling = gld_instance.get_property("house5", "cooling_setpoint")
    assert get_code == 0
    assert float(actual_cooling) == pytest.approx(expected_value)


def test_repeated_normalized_set_property_allows_stepping(gld_instance):
    _load_house_with_solar(gld_instance)

    houses = gld_instance.get_objects_by_class("house")
    if "house5" not in houses:
        pytest.skip("house5 not available in house_with_solar model")

    # Keep applying an intentionally low cooling setpoint while stepping.
    # This used to trigger fatal thermostat overlap in longer workflows.
    step_codes = []
    for _ in range(20):
        details = gld_instance.set_property(
            "house5",
            "cooling_setpoint",
            55,
            detailed=True,
        )
        assert details["code"] == 0

        step_code, _ = gld_instance.step()
        step_codes.append(step_code)

        # TIME_STEP_ERROR indicates stop-time blocking in the wrapper; that is
        # acceptable for this stability regression as long as the worker lives.
        if step_code == 5:
            break

    assert step_codes, "Expected at least one step() call"
    assert all(code in (0, 5) for code in step_codes)
