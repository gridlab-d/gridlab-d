"""Integration tests for the copied deltamode model in tests/models."""

from __future__ import annotations

from pathlib import Path
import re

import pytest


def _deltamode_model_paths() -> tuple[Path, Path]:
    tests_dir = Path(__file__).parent
    model_path = tests_dir / "models" / "test_deltamode.glm"
    player_csv = tests_dir / "test_deltamode_house_player.csv"
    return model_path, player_csv


def _require_deltamode_assets() -> Path:
    model_path, player_csv = _deltamode_model_paths()

    if not model_path.exists():
        pytest.skip(f"Missing model file: {model_path}")
    if not player_csv.exists():
        pytest.skip(
            "Missing player CSV required by test_deltamode.glm: "
            f"{player_csv}"
        )

    return model_path


def test_deltamode_model_run_succeeds(gld_instance):
    """The copied deltamode model should load and run via the Python API."""
    model_path = _require_deltamode_assets()

    assert gld_instance.set_working_directory(str(model_path.parent)) == 0
    assert gld_instance.load(str(model_path)) == 0
    assert gld_instance.run() == 0


def test_deltamode_fractional_step_to_returns_fractional_time(gld_instance):
    """Fractional step_to should preserve sub-second precision in returned time."""
    model_path = _require_deltamode_assets()

    assert gld_instance.set_working_directory(str(model_path.parent)) == 0
    assert gld_instance.load(str(model_path)) == 0

    code, time_str = gld_instance.step_to("2001-01-01T00:00:01.250000")
    assert code >= 0
    assert isinstance(time_str, str)
    assert re.search(r"\.\d+", time_str), f"Expected fractional time, got: {time_str}"


def test_deltamode_fractional_step_to_increments_delta_counter(gld_instance):
    """Fractional step_to should enter the delta branch at least once."""
    model_path = _require_deltamode_assets()

    assert gld_instance.set_working_directory(str(model_path.parent)) == 0
    assert gld_instance.load(str(model_path)) == 0

    before = int(gld_instance.global_getvar("api_delta_trigger_count") or "0")
    code, _ = gld_instance.step_to("2001-01-01T00:00:01.250000")
    assert code >= 0
    after = int(gld_instance.global_getvar("api_delta_trigger_count") or "0")

    assert after > before, (
        "Expected delta trigger counter to increase for fractional step_to, "
        f"but before={before}, after={after}"
    )
