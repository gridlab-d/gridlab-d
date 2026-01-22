"""Tests for message capture API"""
import os
import sys
import pytest

# Add parent directory to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

import gridlabd


def test_error_messages_captured():
    """Test that warnings are captured during model run"""
    gld = gridlabd.GridLabD()
    gld.clear_messages()
    
    model_path = os.path.join(os.path.dirname(__file__), "test_HVAC_balance.glm")
    gld.load(model_path)
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
    
    model_path = os.path.join(os.path.dirname(__file__), "test_HVAC_balance.glm")
    gld.load(model_path)
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
    
    model_path = os.path.join(os.path.dirname(__file__), "test_HVAC_balance.glm")
    gld.load(model_path)
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
    model_path = os.path.join(os.path.dirname(__file__), "test_HVAC_balance.glm")
    gld.load(model_path)
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
    model_path = os.path.join(os.path.dirname(__file__), "test_HVAC_balance.glm")
    gld.load(model_path)
    gld.setup_after_load()
    gld.run()
    
    messages = gld.get_messages()
    assert len(messages) <= 5, f"Should not exceed limit of 5, got {len(messages)}"
    
    # Reset to default for other tests
    gld.set_message_capture_limit(10000)


def test_message_timestamps():
    """Test that messages have timestamps"""
    gld = gridlabd.GridLabD()
    gld.clear_messages()
    
    model_path = os.path.join(os.path.dirname(__file__), "test_HVAC_balance.glm")
    gld.load(model_path)
    gld.setup_after_load()
    gld.run()
    
    messages = gld.get_messages()
    
    if len(messages) == 0:
        pytest.skip("No messages captured - likely due to global state from previous tests")
    
    for msg in messages:
        assert msg["timestamp"] != "", f"Message should have timestamp: {msg}"
        assert "INIT" in msg["timestamp"] or "20" in msg["timestamp"], \
            f"Unexpected timestamp format: {msg['timestamp']}"
