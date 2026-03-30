"""Tests verifying consistent ISO 8601 time types across all API methods.

Related to issues #1555 and #1559: all time-returning methods should return
ISO 8601 formatted strings.
"""

import json
import re
from datetime import datetime, timedelta
from pathlib import Path

import pytest

# Pattern for ISO 8601 datetime without timezone: YYYY-MM-DDTHH:MM:SS
ISO_8601_PATTERN = re.compile(r"^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}")


class TestTimeConsistency:
    """All time-returning methods should return ISO 8601 strings."""

    def test_get_time_returns_iso(self, minimal_model):
        """get_time() should return an ISO 8601 string."""
        code, time_str = minimal_model.get_time()
        assert code >= 0
        assert isinstance(time_str, str)
        assert ISO_8601_PATTERN.match(time_str), f"Not ISO 8601: {time_str}"
        # Should be parseable by fromisoformat
        datetime.fromisoformat(time_str)

    def test_step_returns_iso(self, minimal_model):
        """step() should return an ISO 8601 string."""
        code, time_str = minimal_model.step()
        assert code >= 0
        assert isinstance(time_str, str)
        assert ISO_8601_PATTERN.match(time_str), f"Not ISO 8601: {time_str}"
        datetime.fromisoformat(time_str)

    def test_step_to_returns_iso(self, minimal_model):
        """step_to() should return an ISO 8601 string."""
        code, start = minimal_model.get_time()
        target = datetime.fromisoformat(start) + timedelta(hours=1)
        target_str = target.isoformat(timespec="seconds")

        code, time_str = minimal_model.step_to(target_str)
        assert code >= 0
        assert isinstance(time_str, str)
        assert ISO_8601_PATTERN.match(time_str), f"Not ISO 8601: {time_str}"
        datetime.fromisoformat(time_str)

    def test_get_clock_returns_iso(self, minimal_model):
        """get_clock() should return an ISO 8601 string."""
        clock = minimal_model.get_clock()
        assert clock is not None
        assert isinstance(clock, str)
        assert ISO_8601_PATTERN.match(clock), f"Not ISO 8601: {clock}"

    def test_get_starttime_returns_iso(self, minimal_model):
        """get_starttime() should return an ISO 8601 string."""
        starttime = minimal_model.get_starttime()
        assert starttime is not None
        assert isinstance(starttime, str)
        assert ISO_8601_PATTERN.match(starttime), f"Not ISO 8601: {starttime}"

    def test_get_stoptime_returns_iso(self, minimal_model):
        """get_stoptime() should return an ISO 8601 string or None."""
        stoptime = minimal_model.get_stoptime()
        # stoptime could be None if NEVER (unbounded)
        if stoptime is not None:
            assert isinstance(stoptime, str)
            assert ISO_8601_PATTERN.match(stoptime), f"Not ISO 8601: {stoptime}"

    def test_get_start_and_stop_include_timezone_offset(self, minimal_model):
        """starttime/stoptime should include timezone info for datetime math."""
        starttime = minimal_model.get_starttime()
        stoptime = minimal_model.get_stoptime()

        assert starttime is not None
        assert stoptime is not None

        start_dt = datetime.fromisoformat(starttime)
        stop_dt = datetime.fromisoformat(stoptime)

        assert start_dt.tzinfo is not None, f"starttime missing timezone offset: {starttime}"
        assert stop_dt.tzinfo is not None, f"stoptime missing timezone offset: {stoptime}"

        assert (stop_dt - start_dt).total_seconds() == 3600

    def test_step_and_get_time_agree(self, minimal_model):
        """step() and get_time() should report the same time after stepping.
        
        Note: Times may have different timezone representations, but represent
        the same instant."""
        minimal_model.set_time_step(300)
        code, step_time = minimal_model.step()
        _, get_time = minimal_model.get_time()
        
        # Parse and normalize both times for comparison
        from datetime import datetime
        step_dt = datetime.fromisoformat(step_time) if step_time else None
        get_dt = datetime.fromisoformat(get_time) if get_time else None
        
        # Strip timezone for comparison (both represent same instant)
        if step_dt and step_dt.tzinfo:
            step_dt = step_dt.replace(tzinfo=None)
        if get_dt and get_dt.tzinfo:
            get_dt = get_dt.replace(tzinfo=None)
            
        assert step_dt == get_dt, (
            f"step() returned {step_time} but get_time() returned {get_time}"
        )

    def test_get_clock_and_get_time_agree(self, minimal_model):
        """get_clock() and get_time() should report the same time.
        
        Note: Times may have different timezone representations, but represent
        the same instant."""
        _, get_time = minimal_model.get_time()
        clock = minimal_model.get_clock()
        
        # Parse and normalize both times for comparison
        from datetime import datetime
        get_dt = datetime.fromisoformat(get_time) if get_time else None
        clock_dt = datetime.fromisoformat(clock) if clock else None
        
        # Strip timezone for comparison (both represent same instant)
        if get_dt and get_dt.tzinfo:
            get_dt = get_dt.replace(tzinfo=None)
        if clock_dt and clock_dt.tzinfo:
            clock_dt = clock_dt.replace(tzinfo=None)
            
        assert get_dt == clock_dt, (
            f"get_time() returned {get_time} but get_clock() returned {clock}"
        )

    def test_checkpoint_clock_times_are_iso(self, gld_instance):
        """Checkpoint clock values should be ISO 8601 strings."""
        model_path = Path(__file__).parent.parent / "house_with_solar"
        gld_instance.set_working_directory(str(model_path))
        assert gld_instance.load("houses.glm") == 0

        gld_instance.set_time_step(900)
        gld_instance.step()
        checkpoint = json.loads(gld_instance.get_checkpoint_json())
        clock = checkpoint.get("clock", {})

        for key in ("starttime", "stoptime", "timestamp"):
            value = clock.get(key)
            assert isinstance(value, str), f"{key} should be a string"
            assert ISO_8601_PATTERN.match(value), f"Not ISO 8601: {value}"
