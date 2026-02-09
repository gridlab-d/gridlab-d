"""Tests verifying consistent ISO 8601 time types across all API methods.

Related to issues #1555 and #1559: all time-returning methods should return
ISO 8601 formatted strings.
"""

import re
from datetime import datetime, timedelta

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

    def test_step_and_get_time_agree(self, minimal_model):
        """step() and get_time() should report the same time after stepping."""
        minimal_model.set_time_step(300)
        code, step_time = minimal_model.step()
        _, get_time = minimal_model.get_time()
        assert step_time == get_time, (
            f"step() returned {step_time} but get_time() returned {get_time}"
        )

    def test_get_clock_and_get_time_agree(self, minimal_model):
        """get_clock() and get_time() should report the same time."""
        _, get_time = minimal_model.get_time()
        clock = minimal_model.get_clock()
        assert get_time == clock, (
            f"get_time() returned {get_time} but get_clock() returned {clock}"
        )
