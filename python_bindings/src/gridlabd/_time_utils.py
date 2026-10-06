"""Time conversion utilities for GridLAB-D Python bindings.

Converts GridLAB-D's native time format to ISO 8601 strings with timezone preservation.
GLD format:    "2024-01-01 00:00:00 EST+5EDT" or "2024-01-01 00:00:00"
ISO 8601:      "2024-01-01T00:00:00-08:00" or "2024-01-01T00:00:00"
"""

import re
from typing import Optional

# Sentinel values returned by GridLAB-D's convert_from_timestamp()
_SENTINELS = frozenset({"INIT", "NEVER", "INVALID", ""})

_TZ_OFFSETS = {
    "PST": "-08:00",
    "PDT": "-07:00",
    "MST": "-07:00",
    "MDT": "-06:00",
    "CST": "-06:00",
    "CDT": "-05:00",
    "EST": "-05:00",
    "EDT": "-04:00",
}


def _tz_to_offset(value: str) -> Optional[str]:
    """Convert timezone labels/specs into an ISO 8601 offset when possible."""
    tz = value.strip()
    if not tz:
        return None

    if tz in _TZ_OFFSETS:
        return _TZ_OFFSETS[tz]

    if re.match(r"^[+-]\d{2}:\d{2}$", tz):
        return tz

    # Handle POSIX-style timezone specs like PST+8PDT by using the standard-zone token.
    m = re.match(r"^([A-Z]{3,4})[+-]\d{1,2}(?:[A-Z]{3,4})?$", tz)
    if m:
        return _TZ_OFFSETS.get(m.group(1))

    return None


def gld_to_iso(time_str: str, timezone_hint: Optional[str] = None) -> Optional[str]:
    """Convert a GridLAB-D time string to ISO 8601 format, preserving timezone info.

    Args:
        time_str: Time string in various formats:
            - "2024-01-01 00:00:00 EST+5EDT" (GLD timezone name)
            - "2024-01-01T00:00:00-08:00" (already ISO with offset)
            - "2024-01-01 00:00:00" (no timezone)
        timezone_hint: Optional timezone spec (for example "PST+8PDT")
            used when time_str has no explicit timezone.

    Returns:
        ISO 8601 string with timezone preserved when present, or None for
        sentinel values like "INIT", "NEVER", or "INVALID".
    """
    if time_str in _SENTINELS:
        return None

    value = time_str.strip()

    # Already in ISO 8601 format with optional timezone offset.
    if re.match(r"^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(?:[+-]\d{2}:\d{2})?$", value):
        return value

    match = re.match(
        r"^(\d{4}-\d{2}-\d{2})[ T](\d{2}:\d{2}:\d{2})(?:\s+([^\s]+))?$",
        value,
    )
    if match:
        date_part, time_part, tz_part = match.groups()
        iso = f"{date_part}T{time_part}"
        offset = _tz_to_offset(tz_part) if tz_part else _tz_to_offset(timezone_hint or "")
        if offset:
            return f"{iso}{offset}"
        return iso

    return value
