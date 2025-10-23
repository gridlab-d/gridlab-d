"""
Basic tests for GridLAB-D Python bindings
"""

import pytest
import sys
import os

# Add the src directory to the path for testing
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'src'))

def test_import():
    """Test that we can import the gridlabd module."""
    try:
        import gridlabd
        assert True
    except ImportError:
        pytest.skip("gridlabd module not built yet")

def test_hello():
    """Test the hello function."""
    try:
        import gridlabd
        result = gridlabd.hello()
        assert isinstance(result, str)
        assert "GridLAB-D" in result
    except ImportError:
        pytest.skip("gridlabd module not built yet")

def test_version():
    """Test the version function."""
    try:
        import gridlabd
        version = gridlabd.version()
        assert isinstance(version, str)
        assert len(version) > 0
    except ImportError:
        pytest.skip("gridlabd module not built yet")

if __name__ == "__main__":
    # Run basic tests manually
    print("Running basic tests...")
    
    try:
        import gridlabd
        print("✓ Import successful")
        
        hello_result = gridlabd.hello()
        print(f"✓ Hello function: {hello_result}")
        
        version_result = gridlabd.version()
        print(f"✓ Version function: {version_result}")
        
        print("\n--- GridLAB-D Info ---")
        gridlabd.info()
        
    except ImportError as e:
        print(f"✗ Import failed: {e}")
        print("The module needs to be built first with 'pip install -e .'")
