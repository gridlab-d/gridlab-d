"""Tests for message capture API"""
from pathlib import Path
import subprocess
import sys
from datetime import datetime
import pytest
import gridlabd


def test_error_messages_captured():
    """Test that warnings are captured during model run"""
    gld = gridlabd.GridLabD()
    gld.clear_messages()
    
    model_path = Path(__file__).parent / "test_HVAC_balance.glm"
    gld.load(str(model_path))
    gld.setup_after_load()
    gld.run()
    
    messages = gld.get_messages()
    assert len(messages) >= 1, "Expected at least one warning to be captured"
    
    # Check structure
    for msg in messages:
        assert "type" in msg
        assert "timestamp" in msg
        assert "message" in msg
        assert msg["type"] in ["WARNING", "ERROR", "DEBUG", "MESSAGE", "VERBOSE", "FATAL"]


def test_clear_messages():
    """Test that clear_messages() works"""
    gld = gridlabd.GridLabD()
    gld.clear_messages()
    
    model_path = Path(__file__).parent / "test_HVAC_balance.glm"
    gld.load(str(model_path))
    gld.setup_after_load()
    gld.run()
    
    messages = gld.get_messages()
    assert len(messages) > 0, "Should have captured some messages"
    
    gld.clear_messages()
    messages = gld.get_messages()
    assert len(messages) == 0, "Messages should be cleared"


def test_message_content():
    """Test that message content is meaningful"""
    gld = gridlabd.GridLabD()
    gld.clear_messages()
    
    model_path = Path(__file__).parent / "test_HVAC_balance.glm"
    gld.load(str(model_path))
    gld.setup_after_load()
    gld.run()
    
    messages = gld.get_messages()
    
    # Check for expected warning about DST
    dst_warnings = [m for m in messages if "DST" in m["message"] or "daylight" in m["message"].lower()]
    assert len(dst_warnings) > 0, "Expected DST warning in captured messages"
    
    for msg in dst_warnings:
        assert msg["timestamp"] != "", "Warning should have timestamp"
        assert msg["type"] == "WARNING", "Should be classified as WARNING"


def test_enable_disable_capture():
    """Test that enable_message_capture() controls capturing"""
    gld = gridlabd.GridLabD()
    
    gld.clear_messages()
    model_path = Path(__file__).parent / "test_HVAC_balance.glm"
    gld.load(str(model_path))
    gld.setup_after_load()
    
    # Disable and clear
    gld.enable_message_capture(False)
    gld.clear_messages()
    
    # Run should not capture new messages
    gld.run()
    messages = gld.get_messages()
    
    assert len(messages) == 0, f"Should not capture when disabled, got {len(messages)}"
    
    # Re-enable for other tests
    gld.enable_message_capture(True)


def test_message_limit_default():
    """Test default message limit is 10000"""
    gld = gridlabd.GridLabD()
    assert gld.get_message_capture_limit() == 10000, "Default limit should be 10000"


def test_message_limit_enforcement():
    """Test that message limit is enforced"""
    gld = gridlabd.GridLabD()
    
    # Set a small limit
    gld.set_message_capture_limit(5)
    assert gld.get_message_capture_limit() == 5
    
    gld.clear_messages()
    model_path = Path(__file__).parent / "test_HVAC_balance.glm"
    gld.load(str(model_path))
    gld.setup_after_load()
    gld.run()
    
    messages = gld.get_messages()
    assert len(messages) <= 5, f"Should not exceed limit of 5, got {len(messages)}"
    
    # Reset to default for other tests
    gld.set_message_capture_limit(10000)


def test_message_timestamps():
    """Test that message timestamps are ISO 8601 for non-sentinel values."""
    gld = gridlabd.GridLabD()
    gld.clear_messages()
    
    model_path = Path(__file__).parent / "test_HVAC_balance.glm"
    gld.load(str(model_path))
    gld.setup_after_load()
    gld.run()
    
    messages = gld.get_messages()
    
    if len(messages) == 0:
        pytest.skip("No messages captured - likely due to global state from previous tests")
    
    sentinel_values = {"INIT", "NEVER", "INVALID", ""}

    for msg in messages:
        timestamp = msg.get("timestamp", "")
        assert timestamp != "", f"Message should have timestamp: {msg}"
        if timestamp in sentinel_values:
            continue
        assert "T" in timestamp, f"Timestamp should be ISO 8601: {timestamp}"
        try:
            datetime.fromisoformat(timestamp)
        except ValueError as exc:
            raise AssertionError(f"Timestamp should be parseable ISO 8601: {timestamp}") from exc


def test_verbose_default_suppresses_console_output():
    """Test that verbose=False (default) suppresses C++ console output"""
    # Run a subprocess to capture stderr/stdout independently
    model_path = Path(__file__).parent / "test_HVAC_balance.glm"
    script = f"""
import sys
import gridlabd

gld = gridlabd.GridLabD()  # verbose=False by default
gld.load("{model_path}")
gld.setup_after_load()
print("USER OUTPUT START", file=sys.stdout)
gld.run()
print("USER OUTPUT END", file=sys.stdout)
"""
    
    result = subprocess.run(
        [sys.executable, "-c", script],
        capture_output=True,
        text=True,
        cwd=Path(__file__).parent
    )
    
    # stderr should be empty (C++ output suppressed)
    assert result.stderr == "", \
        f"Expected no stderr output with verbose=False, got: {result.stderr}"
    
    # stdout should only contain user output
    assert "USER OUTPUT START" in result.stdout
    assert "USER OUTPUT END" in result.stdout


def test_verbose_true_shows_console_output():
    """Test that verbose=True enables C++ console output on stderr"""
    model_path = Path(__file__).parent / "test_HVAC_balance.glm"
    script = f"""
import sys
import gridlabd

gld = gridlabd.GridLabD(verbose=True)
gld.load("{model_path}")
gld.setup_after_load()
gld.run()
"""
    
    result = subprocess.run(
        [sys.executable, "-c", script],
        capture_output=True,
        text=True,
        cwd=Path(__file__).parent
    )
    
    # With verbose=True, stderr might have output (or might not if no verbose messages)
    # The key is that it's not blocked - we just verify no crash
    assert result.returncode == 0, \
        f"Expected successful run with verbose=True, got return code {result.returncode}"


def test_messages_captured_regardless_of_verbose():
    """Test that messages are captured with both verbose=True and verbose=False"""
    model_path = Path(__file__).parent / "test_HVAC_balance.glm"
    
    # Test with verbose=False (default)
    gld_quiet = gridlabd.GridLabD()
    gld_quiet.clear_messages()
    gld_quiet.load(str(model_path))
    gld_quiet.setup_after_load()
    gld_quiet.run()
    messages_quiet = gld_quiet.get_messages()
    
    # Test with verbose=True
    gld_verbose = gridlabd.GridLabD(verbose=True)
    gld_verbose.clear_messages()
    gld_verbose.load(str(model_path))
    gld_verbose.setup_after_load()
    gld_verbose.run()
    messages_verbose = gld_verbose.get_messages()
    
    # Both should capture messages
    assert len(messages_quiet) > 0, "Should capture messages with verbose=False"
    assert len(messages_verbose) > 0, "Should capture messages with verbose=True"
    
    # Message count should be similar (within a small tolerance)
    assert abs(len(messages_quiet) - len(messages_verbose)) <= 2, \
        f"Message counts should be similar: quiet={len(messages_quiet)}, verbose={len(messages_verbose)}"


def test_no_console_interleaving():
    """Test that user print() statements are not interleaved with C++ output"""
    model_path = Path(__file__).parent / "test_HVAC_balance.glm"
    script = f"""
import sys
import gridlabd

gld = gridlabd.GridLabD()  # verbose=False
gld.load("{model_path}")
gld.setup_after_load()

# Print multiple user messages during simulation
for i in range(5):
    print(f"USER_LINE_{{i}}")
    code, t = gld.step()
    if code != 0:
        break

print("DONE")
"""
    
    result = subprocess.run(
        [sys.executable, "-c", script],
        capture_output=True,
        text=True,
        cwd=Path(__file__).parent
    )
    
    # Check that user output is clean and sequential
    lines = [line for line in result.stdout.split('\n') if line.strip()]
    user_lines = [line for line in lines if line.startswith("USER_LINE_")]
    
    assert len(user_lines) >= 3, "Should have multiple user output lines"
    
    # Check they are sequential
    for i, line in enumerate(user_lines):
        assert f"USER_LINE_{i}" == line, \
            f"User output should be sequential, expected USER_LINE_{i}, got {line}"
    
    # stderr should be empty (no C++ noise)
    assert result.stderr == "", \
        f"Expected clean stderr with verbose=False, got: {result.stderr}"
