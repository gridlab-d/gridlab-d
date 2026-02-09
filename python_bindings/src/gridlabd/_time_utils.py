"""Time conversion utilities for GridLAB-D Python bindings.

Converts GridLAB-D's native time format to ISO 8601 strings.
GLD format: "2024-01-01 00:00:00 EST+5EDT"
ISO 8601:   "2024-01-01T00:00:00"
"""

from typing import Optional

# Sentinel values returned by GridLAB-D's convert_from_timestamp()
_SENTINELS = frozenset({"INIT", "NEVER", "INVALID", ""})


def gld_to_iso(time_str: str) -> Optional[str]:
    """Convert a GridLAB-D time string to ISO 8601 format.

    Args:
        time_str: GLD-format time string, e.g. "2024-01-01 00:00:00 EST+5EDT"

    Returns:
        ISO 8601 string (e.g. "2024-01-01T00:00:00") or None for sentinel
        values like "INIT", "NEVER", or "INVALID".
    """
    if time_str in _SENTINELS:
        return None

    # Strip the GLD timezone suffix. The suffix is the last space-separated
    # token when it contains alphabetic characters (e.g., "EST", "PST+8PDT").
    # The date-time portion is "YYYY-MM-DD HH:MM:SS[.nnnnnnnnn]".
    parts = time_str.rsplit(" ", 1)
    if len(parts) == 2 and any(c.isalpha() for c in parts[1]):
        time_str = parts[0]

    # Replace the space between date and time with 'T' for ISO 8601
    # "2024-01-01 00:00:00" -> "2024-01-01T00:00:00"
    if " " in time_str:
        date_part, time_part = time_str.split(" ", 1)
        return f"{date_part}T{time_part}"

    return time_str
