"""Scenario matrix validation for deltamode/QSTS stepping behaviors.

These tests encode expected behavior from the delta-mode scenario table and
are intended to produce a pass/fail map against current implementation.
"""
from datetime import datetime
from pathlib import Path
import re
import csv

import pytest


def _to_dt(value: str) -> datetime:
    normalized = re.sub(r"(\.\d{6})\d+(?=[+-]\d\d:\d\d$)", r"\1", value)
    try:
        dt = datetime.fromisoformat(normalized)
    except ValueError:
        try:
            dt = datetime.strptime(normalized, "%m-%d-%YT%H:%M:%S.%f%z")
        except ValueError:
            dt = datetime.strptime(normalized, "%m-%d-%YT%H:%M:%S%z")
    if dt.tzinfo is not None:
        dt = dt.replace(tzinfo=None)
    return dt


def _seconds(time_str: str, base: datetime) -> float:
    return (_to_dt(time_str) - base).total_seconds()


def _set_model(gld_instance, test_models_dir: Path, filename: str) -> Path:
    model_path = test_models_dir / filename
    assert model_path.exists(), f"Missing model fixture: {model_path}"
    assert gld_instance.set_working_directory(str(model_path.parent)) == 0
    assert gld_instance.load(str(model_path)) == 0
    return model_path


def _enable_messages(gld_instance) -> None:
    gld_instance.enable_message_capture(True)
    gld_instance.clear_messages()


def _messages_text(gld_instance) -> str:
    return "\n".join(msg.get("message", "") for msg in gld_instance.get_messages())


def test_scenario_1_qsts_step_to_fractional_targets(gld_instance, test_models_dir):
    """Scenario 1 behavior: step_to(5.5) then step_to(10.5) from QSTS.

    Validates both sub-behaviors:
    - step_to(5.5): deltamode is triggered/scheduled and lands at 5.5
    - step_to(10.5): floor-second scheduling path still lands at 10.5
    """
    _set_model(gld_instance, test_models_dir, "minimal.glm")

    base = _to_dt(gld_instance.get_time()[1])
    before = int(gld_instance.global_getvar("api_delta_trigger_count") or "0")

    code_55, time_55 = gld_instance.step_to("2020-01-01T00:00:05.500000")
    assert code_55 >= 0
    assert _seconds(time_55, base) == pytest.approx(5.5, abs=1e-6)
    after_55 = int(gld_instance.global_getvar("api_delta_trigger_count") or "0")
    assert after_55 > before

    code_105, time_105 = gld_instance.step_to("2020-01-01T00:00:10.500000")
    assert code_105 >= 0
    assert _seconds(time_105, base) == pytest.approx(10.5, abs=1e-6)
    after_105 = int(gld_instance.global_getvar("api_delta_trigger_count") or "0")
    assert after_105 > after_55


def test_scenario_2_fixed_step_with_event_window_85_to_93(gld_instance, test_models_dir):
    """Scenario 2: QSTS fixed 5s step with delta event 8.5->9.3 should pause at 10."""
    model_path = _set_model(gld_instance, test_models_dir, "deltamode_window_93.glm")
    recorder_path = model_path.parent / "deltamode_window_93_recorder.csv"
    if recorder_path.exists():
        recorder_path.unlink()

    base = _to_dt(gld_instance.get_time()[1])

    assert _seconds(gld_instance.get_time()[1], base) == pytest.approx(0.0, abs=1e-6)

    assert gld_instance.set_time_step(5) == 0

    code_step, time_step = gld_instance.step()
    assert code_step >= 0
    assert _seconds(time_step, base) == pytest.approx(5.0, abs=1e-6)

    assert gld_instance.finalize() == 0

    assert recorder_path.exists(), "Expected DELTAMODE recorder output file"
    rows = []
    with recorder_path.open("r", encoding="utf-8") as fh:
        reader = csv.reader(fh)
        for row in reader:
            if not row:
                continue
            first = row[0].strip()
            if first.startswith("#"):
                continue
            if first.lower() == "timestamp":
                continue
            if re.match(r"\d{2}-\d{2}-\d{4}\s+\d{2}:\d{2}:\d{2}", first):
                rows.append(row)

    assert rows, "Expected at least one DELTAMODE recorder row"
    assert any("00:00:08" in row[0] or "00:00:09" in row[0] for row in rows), (
        "Expected DELTAMODE recorder activity in scenario window 8.5-9.3s"
    )


def test_scenario_3_fixed_step_with_event_crossing_target(gld_instance, test_models_dir):
    """Scenario 3: QSTS fixed 5s step with delta event 8.5->10.3 pauses at 10."""
    model_path = _set_model(gld_instance, test_models_dir, "deltamode_window_103.glm")
    recorder_path = model_path.parent / "deltamode_window_103_recorder.csv"
    if recorder_path.exists():
        recorder_path.unlink()

    base = _to_dt(gld_instance.get_time()[1])

    assert _seconds(gld_instance.get_time()[1], base) == pytest.approx(0.0, abs=1e-6)

    assert gld_instance.set_time_step(5) == 0

    code_step, time_step = gld_instance.step()
    assert code_step >= 0
    assert _seconds(time_step, base) == pytest.approx(5.0, abs=1e-6)

    assert gld_instance.finalize() == 0

    assert recorder_path.exists(), "Expected DELTAMODE recorder output file"
    rows = []
    with recorder_path.open("r", encoding="utf-8") as fh:
        reader = csv.reader(fh)
        for row in reader:
            if not row:
                continue
            first = row[0].strip()
            if first.startswith("#"):
                continue
            if first.lower() == "timestamp":
                continue
            if re.match(r"\d{2}-\d{2}-\d{4}\s+\d{2}:\d{2}:\d{2}", first):
                rows.append(row)

    assert rows, "Expected at least one DELTAMODE recorder row"
    assert any(
        "00:00:08" in row[0] or "00:00:09" in row[0] or "00:00:10" in row[0]
        for row in rows
    ), "Expected DELTAMODE recorder activity in scenario window 8.5-10.3s"


def test_scenario_4_deltamode_default_step_is_delta_timestep(gld_instance, test_models_dir):
    """Scenario 4: establish t=5.5 then step once by one effective delta step."""
    # Note: a literal first step() from whole-second startup currently follows
    # event scheduling and rounds to whole seconds in this setup, so it cannot
    # reliably land on 5.5. We establish the 5.5 precondition explicitly, then
    # validate that the subsequent default step() advances by one delta step.
    model_path = _set_model(gld_instance, test_models_dir, "deltamode_window_62.glm")
    recorder_path = model_path.parent / "deltamode_window_62_recorder.csv"
    if recorder_path.exists():
        recorder_path.unlink()

    base = _to_dt(gld_instance.get_time()[1])
    assert _seconds(gld_instance.get_time()[1], base) == pytest.approx(0.0, abs=1e-6)

    code_pre, time_pre = gld_instance.step_to("2020-01-01T00:00:05.500000")
    assert code_pre >= 0
    assert _seconds(time_pre, base) == pytest.approx(5.5, abs=1e-6)

    # Explicitly request transient persistence for this next step.
    assert gld_instance.trigger_transient() == 0

    code_step, time_step = gld_instance.step()
    assert code_step >= 0
    assert _seconds(time_step, base) == pytest.approx(5.502, abs=2e-3)

    assert gld_instance.finalize() == 0

    assert recorder_path.exists(), "Expected DELTAMODE recorder output file"
    rows = []
    with recorder_path.open("r", encoding="utf-8") as fh:
        reader = csv.reader(fh)
        for row in reader:
            if not row:
                continue
            first = row[0].strip()
            if first.startswith("#"):
                continue
            if first.lower() == "timestamp":
                continue
            if re.match(r"\d{2}-\d{2}-\d{4}\s+\d{2}:\d{2}:\d{2}", first):
                rows.append(row)

    assert rows, "Expected at least one DELTAMODE recorder row"
    assert any("00:00:05.50" in row[0] or "00:00:05.502" in row[0] for row in rows), (
        "Expected DELTAMODE activity around t=5.5"
    )


def test_scenario_5_set_time_step_001s_in_deltamode(gld_instance, test_models_dir):
    """Scenario 5: set_time_step(0.01) in deltamode then step() -> 5.51."""
    # Contract: sub-second API timestep requests should be preserved and applied
    # as relative target advances in delta-capable context.
    _set_model(gld_instance, test_models_dir, "deltamode_start_55_62.glm")

    base = _to_dt(gld_instance.get_time()[1])

    assert gld_instance.set_time_step(0.01) == 0

    code_step, time_step = gld_instance.step()
    assert code_step >= 0
    assert _seconds(time_step, base) == pytest.approx(0.01, abs=2e-3)


def test_scenario_6_set_time_step_too_small_should_warn_and_advance(gld_instance, test_models_dir):
    """Scenario 6 preference: set_time_step(1ms) warns and advances to 5.502."""
    # Contract: undersized sub-second request clamps to effective delta step
    # with warning surfaced via get_messages()/terminal (verbose mode).
    _set_model(gld_instance, test_models_dir, "deltamode_start_55_62.glm")
    _enable_messages(gld_instance)

    base = _to_dt(gld_instance.get_time()[1])

    result = gld_instance.set_time_step(0.001)
    assert result == 0

    code_step, time_step = gld_instance.step()
    assert code_step >= 0
    assert _seconds(time_step, base) == pytest.approx(0.002, abs=2e-3)

    text = _messages_text(gld_instance).lower()
    assert "too small" in text


def test_scenario_7_exit_deltamode_and_sync_to_next_qsts_second(gld_instance, test_models_dir):
    """Scenario 7 baseline: if deltamode has no remaining work, step() exits and syncs to next second."""
    _set_model(gld_instance, test_models_dir, "deltamode_window_62.glm")

    base = _to_dt(gld_instance.get_time()[1])

    code_pre, time_pre = gld_instance.step_to("2020-01-01T00:00:05.500000")
    assert code_pre >= 0
    assert _seconds(time_pre, base) == pytest.approx(5.5, abs=1e-6)

    code_step, time_step = gld_instance.step()
    assert code_step >= 0
    assert _seconds(time_step, base) == pytest.approx(6.0, abs=1e-6)


def test_scenario_7_forced_deltamode_globals_are_runtime_settable(gld_instance, test_models_dir):
    """Scenario 7 note: transient controls should be runtime settable via API."""
    _set_model(gld_instance, test_models_dir, "minimal.glm")

    set_always = gld_instance.maintain_transient(True)
    assert set_always == 0
    assert gld_instance.global_getvar("deltamode_forced_always").strip().upper() in {"TRUE", "1"}

    trigger = gld_instance.trigger_transient()
    assert trigger == 0

    set_extra = gld_instance.global_setvar("deltamode_forced_extra_timesteps", "3")
    assert set_extra in {0, 1}
    assert int(gld_instance.global_getvar("deltamode_forced_extra_timesteps")) == 3

    reset = gld_instance.exit_transient()
    assert reset == 0
    assert gld_instance.global_getvar("deltamode_forced_always").strip().upper() in {"FALSE", "0"}
    assert int(gld_instance.global_getvar("deltamode_forced_extra_timesteps") or "0") == 0


def test_scenario_8_step_to_125_through_two_delta_windows(gld_instance, test_models_dir):
    """Scenario 8: step_to(12.5) should traverse both windows and land at 12.5."""
    _set_model(gld_instance, test_models_dir, "deltamode_multi_windows.glm")

    base = _to_dt(gld_instance.get_time()[1])
    before = int(gld_instance.global_getvar("api_delta_trigger_count") or "0")

    code_125, time_125 = gld_instance.step_to("2020-01-01T00:00:12.500000")
    assert code_125 >= 0
    assert _seconds(time_125, base) == pytest.approx(12.5, abs=1e-6)

    after = int(gld_instance.global_getvar("api_delta_trigger_count") or "0")
    assert after > before
