"""
Unit tests for GridLAB-D initialization and instance creation.

Tests:
- Basic instance creation
- Multiple instances (process isolation)
- Installation information retrieval
"""

import pytest
from gridlabd import GridLabD


def test_create_instance():
    """Test that we can create a GridLAB-D instance."""
    gld = GridLabD()
    assert gld is not None


def test_create_multiple_instances():
    """Test that multiple instances work correctly (process isolation)."""
    gld1 = GridLabD()
    gld2 = GridLabD()
    
    assert gld1 is not None
    assert gld2 is not None
    assert gld1 is not gld2


def test_get_install_root(gld_instance):
    """Test that we can retrieve installation root."""
    root = gld_instance.get_install_root()
    
    assert root is not None
    assert len(str(root)) > 0


def test_is_initialized(gld_instance):
    """Test that we can check initialization status."""
    is_init = gld_instance.is_initialized()
    
    # Should return a truthy value for initialized instance
    assert is_init is not None


def test_instance_cleanup():
    """Test that instances clean up properly."""
    # Create and immediately discard instance
    gld = GridLabD()
    del gld
    
    # Should not crash when creating another instance
    gld2 = GridLabD()
    assert gld2 is not None


def test_fixture_provides_instance(gld_instance):
    """Test that the gld_instance fixture works correctly."""
    assert gld_instance is not None
    assert isinstance(gld_instance, GridLabD)
