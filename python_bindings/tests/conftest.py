"""
pytest configuration and shared fixtures for GridLAB-D Python bindings tests.

This module provides reusable fixtures that eliminate boilerplate in test files.
"""

import os
import pytest
from pathlib import Path
from gridlabd import GridLabD


@pytest.fixture
def gld_instance():
    """
    Create a fresh GridLAB-D instance for testing.
    
    Yields:
        GridLabD: A new instance that will be automatically cleaned up after the test.
        
    Example:
        def test_something(gld_instance):
            assert gld_instance.load("test.glm") == 0
    """
    instance = GridLabD()
    yield instance
    # Cleanup happens automatically via __del__


@pytest.fixture
def test_models_dir():
    """
    Get the path to the test models directory.
    
    Returns:
        Path: Absolute path to tests/models/
        
    Example:
        def test_load_model(gld_instance, test_models_dir):
            model_path = test_models_dir / "minimal.glm"
            assert gld_instance.load(str(model_path)) == 0
    """
    return Path(__file__).parent / "models"


@pytest.fixture
def minimal_model(gld_instance, test_models_dir):
    """
    Pre-loaded minimal GridLAB-D model (not yet run).
    
    Provides:
        - Single clock object
        - Simple configuration
        - Ready to call run() or step()
        
    Yields:
        GridLabD: Instance with minimal.glm loaded
        
    Example:
        def test_run(minimal_model):
            assert minimal_model.run() == 0
    """
    model_path = test_models_dir / "minimal.glm"
    result = gld_instance.load(str(model_path))
    assert result == 0, f"Failed to load {model_path}, error code: {result}"
    yield gld_instance


@pytest.fixture
def stepped_model(gld_instance, test_models_dir):
    """
    Pre-loaded and initialized model ready for stepping.
    
    Provides:
        - Model loaded and initialized (step() called once)
        - At first simulation timestamp
        - Ready for get_objects_by_class(), set_property(), etc.
        
    Yields:
        GridLabD: Instance with model loaded and stepped once
        
    Example:
        def test_objects(stepped_model):
            objects = stepped_model.get_objects_by_class("house")
            assert len(objects) > 0
    """
    model_path = test_models_dir / "minimal.glm"
    result = gld_instance.load(str(model_path))
    assert result == 0, f"Failed to load {model_path}, error code: {result}"
    
    # Step once to initialize
    status, timestamp = gld_instance.step()
    assert status >= 0, f"Failed to initialize model with step(), status: {status}"
    
    yield gld_instance


@pytest.fixture
def residential_model(gld_instance, test_models_dir):
    """
    Pre-loaded residential model with house and solar.
    
    Provides:
        - Climate object
        - Triplex meter
        - House with HVAC
        - Solar panel
        
    Yields:
        GridLabD: Instance with residential.glm loaded
    """
    model_path = test_models_dir / "residential.glm"
    result = gld_instance.load(str(model_path))
    assert result == 0, f"Failed to load {model_path}, error code: {result}"
    yield gld_instance


@pytest.fixture
def gld_with_model(gld_instance):
    """
    Pre-loaded and initialized HVAC model for property and object tests.
    
    Provides:
        - test_HVAC_balance.glm loaded
        - Model initialized via setup_after_load()
        - Ready for get_all_objects(), get_property(), etc.
        
    Yields:
        GridLabD: Instance with HVAC model loaded and initialized
    """
    model_path = Path(__file__).parent / "test_HVAC_balance.glm"
    result = gld_instance.load(str(model_path))
    assert result == 0, f"Failed to load model: {result}"
    
    result = gld_instance.setup_after_load()
    assert result == 0, f"Failed to initialize model: {result}"
    
    yield gld_instance


@pytest.fixture
def temp_glm_file(tmp_path):
    """
    Create a temporary GLM file for testing.
    
    Args:
        tmp_path: pytest's built-in temporary directory fixture
        
    Returns:
        function: Callable that creates a GLM file with given content
        
    Example:
        def test_custom_model(gld_instance, temp_glm_file):
            glm_path = temp_glm_file("clock { starttime '2020-01-01 00:00:00'; }")
            assert gld_instance.load(str(glm_path)) == 0
    """
    def _create_glm(content: str, filename: str = "test.glm") -> Path:
        glm_file = tmp_path / filename
        glm_file.write_text(content)
        return glm_file
    
    return _create_glm
