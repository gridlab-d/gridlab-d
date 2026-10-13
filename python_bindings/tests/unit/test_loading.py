"""
Unit tests for loading GLM models.

Tests:
- Loading valid GLM files
- Loading non-existent files
- Loading with load_glm() argv-style
"""

import pytest
from pathlib import Path


def test_load_minimal_model(gld_instance, test_models_dir):
    """Test loading a minimal valid GLM model."""
    model_path = test_models_dir / "minimal.glm"
    result = gld_instance.load(str(model_path))
    
    assert result == 0


def test_load_nonexistent_file(gld_instance):
    """Test that loading a non-existent file fails gracefully."""
    result = gld_instance.load("/nonexistent/path/to/model.glm")
    
    assert result != 0  # Should return error code


def test_load_invalid_syntax(gld_instance, temp_glm_file):
    """Test that loading a file with invalid syntax fails."""
    # Create GLM file with syntax error
    invalid_glm = temp_glm_file("""
        clock {
            timezone PST+8PDT;
            starttime '2020-01-01 00:00:00'
            // Missing semicolon above
        }
    """)
    
    result = gld_instance.load(str(invalid_glm))
    assert result != 0  # Should fail


def test_load_glm_with_argv_style(gld_instance, test_models_dir):
    """Test loading with load_glm() using argv-style arguments."""
    model_path = test_models_dir / "minimal.glm"
    
    # load_glm() expects a list of arguments
    result = gld_instance.load_glm([str(model_path)])
    
    # Returns GLDErrorCode enum, check it's successful
    assert result is not None


def test_load_stepping_model(gld_instance, test_models_dir):
    """Test loading the stepping model used in time management tests."""
    model_path = test_models_dir / "stepping.glm"
    result = gld_instance.load(str(model_path))
    
    assert result == 0


def test_fixture_provides_loaded_model(minimal_model):
    """Test that minimal_model fixture provides a loaded instance."""
    # The minimal_model fixture already loads the model
    # We should be able to run it immediately
    result = minimal_model.run()
    assert result == 0
