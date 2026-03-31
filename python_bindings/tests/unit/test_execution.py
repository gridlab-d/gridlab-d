"""
Unit tests for model execution (run and step).

Tests:
- Basic run() execution
- Checking run success/failure
- Step-by-step execution
- Execution without loading model
"""

from datetime import datetime

import pytest


def test_run_minimal_model(minimal_model):
    """Test running a minimal model to completion."""
    result = minimal_model.run()
    assert result == 0  # 0 = success


def test_run_without_loading(gld_instance):
    """Test that running without loading a model fails."""
    with pytest.raises(RuntimeError, match="no objects loaded"):
        gld_instance.run()


def test_step_minimal_model(minimal_model):
    """Test stepping through a model."""
    # First step initializes the simulation
    status, timestamp = minimal_model.step()
    assert status >= 0  # Non-negative status
    assert isinstance(timestamp, str)
    # Verify ISO 8601 format (contains 'T' separator)
    assert "T" in timestamp
    
    # Should be able to step again
    status2, timestamp2 = minimal_model.step()
    assert status2 >= 0


def test_step_until_completion(gld_instance, test_models_dir):
    """Test stepping through entire simulation."""
    model_path = test_models_dir / "minimal.glm"
    assert gld_instance.load(str(model_path)) == 0
    
    step_count = 0
    max_steps = 100  # Safety limit
    
    while step_count < max_steps:
        status, timestamp = gld_instance.step()
        if status < 0:  # Error
            break
        if status == 0:  # Simulation complete
            break
        step_count += 1
    
    # Should complete in reasonable number of steps
    assert step_count >= 0
    assert step_count < max_steps


def test_step_without_loading(gld_instance):
    """Test that stepping without loading a model fails."""
    with pytest.raises(RuntimeError, match="no objects loaded"):
        gld_instance.step()


def test_stepped_fixture_is_initialized(stepped_model):
    """Test that stepped_model fixture has already been stepped."""
    # The fixture should have already called step() once
    # So we should be able to get objects immediately
    objects = stepped_model.get_objects_by_class("house")
    assert isinstance(objects, list)
    assert len(objects) > 0


def test_run_returns_status_code(minimal_model):
    """Test that run() returns integer status code."""
    result = minimal_model.run()
    assert isinstance(result, int)
    assert result == 0  # Success


def test_step_respects_fixed_timestep(gld_instance, test_models_dir):
    """Test that step() advances by the configured fixed timestep."""
    model_path = test_models_dir / "minimal.glm"
    assert gld_instance.load(str(model_path)) == 0

    assert gld_instance.set_time_step(900) == 0

    status1, time1 = gld_instance.step()
    assert status1 >= 0

    status2, time2 = gld_instance.step()
    assert status2 >= 0

    dt1 = datetime.fromisoformat(time1)
    dt2 = datetime.fromisoformat(time2)
    assert (dt2 - dt1).total_seconds() == pytest.approx(900.0, abs=1e-6)
