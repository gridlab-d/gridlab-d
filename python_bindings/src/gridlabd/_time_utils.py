"""Time conversion utilities for GridLAB-D Python bindings.

Converts GridLAB-D's native time format to ISO 8601 strings with timezone preservation.
GLD format:    "2024-01-01 00:00:00 EST+5EDT" or "2024-01-01 00:00:00"
ISO 8601:      "2024-01-01T00:00:00-08:00" or "2024-01-01T00:00:00"
"""

import re
from typing import Optional

# Sentinel values returned by GridLAB-D's convert_from_timestamp()
_SENTINELS = frozenset({"INIT", "NEVER", "INVALID", ""})


def gld_to_iso(time_str: str) -> Optional[str]:
    """Convert a GridLAB-D time string to ISO 8601 format, preserving timezone info.

    Args:
        time_str: Time string in various formats:
            - "2024-01-01 00:00:00 EST+5EDT" (GLD timezone name)
            - "2024-01-01T00:00:00-08:00" (already ISO with offset)
            - "2024-01-01 00:00:00" (no timezone)

    Returns:
        ISO 8601 string with timezone preserved when present, or None for
        sentinel values like "INIT", "NEVER", or "INVALID".
    """
    if time_str in _SENTINELS:
        return None

    # Already in ISO 8601 format with timezone offset (e.g., "2024-01-01T00:00:00-08:00")
    if re.match(r'^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}[+-]\d{2}:\d{2}$', time_str):
        return time_str

    # Check if it has a GLD timezone suffix (alphabetic characters like "EST", "PST+8PDT")
    # vs an ISO timezone offset (like "-08:00", "+05:30")
    parts = time_str.rsplit(" ", 1)
    if len(parts) == 2:
        potential_tz = parts[1]
        # If it's an alphabetic timezone name (EST, PST+8PDT), strip it
        # Keep ISO timezone offsets like -08:00
        if any(c.isalpha() for c in potential_tz):
            time_str = parts[0]
        # If it's an ISO offset like "-08:00", we'll preserve it

    # Replace the space between date and time with 'T' for ISO 8601
    # "2024-01-01 00:00:00" -> "2024-01-01T00:00:00"
    # "2024-01-01 00:00:00 -08:00" -> "2024-01-01T00:00:00-08:00"
    if " " in time_str:
        # Split on first space only to preserve timezone offset
        parts = time_str.split(" ", 1)
        if len(parts) == 2:
            date_part = parts[0]
            # Check if there's a timezone offset in the time part
            time_and_tz = parts[1]
            if " " in time_and_tz:
                # "00:00:00 -08:00" -> "00:00:00-08:00"
                time_part, tz_offset = time_and_tz.split(" ", 1)
                return f"{date_part}T{time_part}{tz_offset}"
            else:
                return f"{date_part}T{time_and_tz}"

    return time_str
