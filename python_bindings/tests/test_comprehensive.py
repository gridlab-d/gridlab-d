"""
Comprehensive test suite for GridLAB-D Python bindings.

This test suite covers:
- Low-level C++ API bindings
- High-level Python interface
- Error handling and edge cases
- Configuration and environment detection
- Context management
"""

import pytest
import sys
import os
from pathlib import Path
from unittest.mock import patch

# Add the src directory to the path for testing
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'src'))

# Test imports
def test_basic_imports():
    """Test that all modules can be imported successfully."""
    import gridlabd
    
    # Test that all expected classes are available
    assert hasattr(gridlabd, 'Simulation')
    assert hasattr(gridlabd, 'GridLabD')
    assert hasattr(gridlabd, 'GLDErrorCode')
    assert hasattr(gridlabd, 'GLDCheckPointMode')
    assert hasattr(gridlabd, 'GridLABDError')
    assert hasattr(gridlabd, 'ConfigurationError')
    assert hasattr(gridlabd, 'SimulationError')


def test_version_info():
    """Test version information functions."""
    import gridlabd
    
    # Test version function
    version = gridlabd.version()
    assert isinstance(version, str)
    assert len(version) > 0
    
    # Test hello function
    hello_msg = gridlabd.hello()
    assert isinstance(hello_msg, str)
    assert "GridLAB-D" in hello_msg


def test_error_code_enums():
    """Test GridLAB-D error code enumerations."""
    import gridlabd
    
    # Test that error codes are accessible
    assert hasattr(gridlabd.GLDErrorCode, 'SUCCESS')
    assert hasattr(gridlabd.GLDErrorCode, 'FILE_NOT_FOUND')
    assert hasattr(gridlabd.GLDErrorCode, 'INVALID_FORMAT')
    
    # Test that error codes have correct values
    assert int(gridlabd.GLDErrorCode.SUCCESS) == 0
    assert int(gridlabd.GLDErrorCode.FILE_NOT_FOUND) >= 1


def test_checkpoint_mode_enums():
    """Test GridLAB-D checkpoint mode enumerations."""
    import gridlabd
    
    # Test that checkpoint modes are accessible
    assert hasattr(gridlabd.GLDCheckPointMode, 'NONE')
    assert hasattr(gridlabd.GLDCheckPointMode, 'SAVE')
    assert hasattr(gridlabd.GLDCheckPointMode, 'LOAD')
    
    # Test that modes have correct values
    assert int(gridlabd.GLDCheckPointMode.NONE) == 0


def test_exception_hierarchy():
    """Test custom exception hierarchy.""" 
    import gridlabd
    
    # Test exception inheritance
    assert issubclass(gridlabd.ConfigurationError, gridlabd.GridLABDError)
    assert issubclass(gridlabd.SimulationError, gridlabd.GridLABDError)
    assert issubclass(gridlabd.GridLABDError, Exception)
    
    # Test that we can raise and catch exceptions
    with pytest.raises(gridlabd.ConfigurationError):
        raise gridlabd.ConfigurationError("Test error")


class TestSimulationClass:
    """Test suite for the high-level Simulation class."""
    
    def test_simulation_path_validation(self):
        """Test GridLAB-D executable path validation."""
        import gridlabd
        
        # Test with non-existent path
        with pytest.raises(gridlabd.ConfigurationError) as exc_info:
            gridlabd.Simulation(gridlabd_path="/nonexistent/path")
        
        assert "not found" in str(exc_info.value)
    
    def test_simulation_path_detection(self):
        """Test automatic GridLAB-D executable detection."""
        import gridlabd
        from gridlabd.simulation import Simulation
        
        # Create a mock executable file for testing
        test_dir = Path("/tmp/test_gridlabd")
        test_dir.mkdir(exist_ok=True)
        test_exe = test_dir / "gridlabd"
        test_exe.touch()
        test_exe.chmod(0o755)
        
        try:
            # Test with explicit valid path
            sim = Simulation(gridlabd_path=str(test_exe))
            assert sim.gridlabd_path == str(test_exe.absolute())
            assert sim.is_configured == True
            
        finally:
            # Cleanup
            test_exe.unlink(missing_ok=True)
            test_dir.rmdir()
    
    def test_simulation_properties(self):
        """Test Simulation class properties."""
        import gridlabd
        from gridlabd.simulation import Simulation
        
        # Create a temporary executable for testing
        test_dir = Path("/tmp/test_gridlabd_props")
        test_dir.mkdir(exist_ok=True)
        test_exe = test_dir / "gridlabd"
        test_exe.touch()
        test_exe.chmod(0o755)
        
        try:
            sim = Simulation(gridlabd_path=str(test_exe))
            
            # Test properties
            assert isinstance(sim.is_configured, bool)
            assert isinstance(sim.gridlabd_path, str)
            assert sim.gridlabd_path == str(test_exe.absolute())
            
            # Test string representation
            repr_str = repr(sim)
            assert "Simulation" in repr_str
            assert str(test_exe.absolute()) in repr_str
            
        finally:
            # Cleanup
            test_exe.unlink(missing_ok=True)
            test_dir.rmdir()
    
    def test_simulation_context_manager(self):
        """Test Simulation context manager functionality."""
        import gridlabd
        from gridlabd.simulation import Simulation
        
        # Create a temporary executable for testing
        test_dir = Path("/tmp/test_gridlabd_context")
        test_dir.mkdir(exist_ok=True)
        test_exe = test_dir / "gridlabd"
        test_exe.touch()
        test_exe.chmod(0o755)
        
        try:
            # Test context manager
            with Simulation(gridlabd_path=str(test_exe)) as sim:
                assert sim is not None
                assert sim.is_configured == True
                assert hasattr(sim, 'load_model')
                assert hasattr(sim, 'run')
                assert hasattr(sim, 'get_results')
                
        finally:
            # Cleanup
            test_exe.unlink(missing_ok=True)
            test_dir.rmdir()
    
    def test_load_model_validation(self):
        """Test model file validation."""
        import gridlabd
        from gridlabd.simulation import Simulation
        
        # Create a temporary executable for testing
        test_dir = Path("/tmp/test_gridlabd_model")
        test_dir.mkdir(exist_ok=True)
        test_exe = test_dir / "gridlabd"
        test_exe.touch()
        test_exe.chmod(0o755)
        
        try:
            sim = Simulation(gridlabd_path=str(test_exe))
            
            # Test with non-existent model file
            with pytest.raises(gridlabd.SimulationError) as exc_info:
                sim.load_model("/nonexistent/model.glm")
            
            assert "not found" in str(exc_info.value)
            
        finally:
            # Cleanup
            test_exe.unlink(missing_ok=True)
            test_dir.rmdir()


class TestLowLevelAPI:
    """Test suite for low-level C++ API."""
    
    def test_gridlabd_class_availability(self):
        """Test that GridLabD C++ class is available."""
        import gridlabd
        
        # Test class exists
        assert hasattr(gridlabd, 'GridLabD')
        
        # Note: We can't instantiate without proper environment,
        # but we can verify the class structure
        gld_class = gridlabd.GridLabD
        assert callable(gld_class)


def test_package_exports():
    """Test that all expected symbols are exported."""
    import gridlabd
    
    expected_exports = [
        'Simulation',
        'GridLABDError', 
        'ConfigurationError',
        'SimulationError',
        'GridLabD',
        'GLDErrorCode',
        'GLDCheckPointMode',
        'hello',
        '__version__',
        'version',
        'info'
    ]
    
    for export in expected_exports:
        assert hasattr(gridlabd, export), f"Missing export: {export}"
        
    # Test __all__ contains expected exports
    for export in gridlabd.__all__:
        assert hasattr(gridlabd, export), f"__all__ contains {export} but it's not available"


def test_environment_detection():
    """Test environment detection functions."""
    import gridlabd
    from gridlabd.simulation import Simulation
    
    # Test auto-detection search paths
    sim_class = Simulation
    test_paths = [
        "/usr/local/bin/gridlabd",
        "/usr/bin/gridlabd", 
        "/opt/gridlabd/bin/gridlabd"
    ]
    
    # This tests the path detection logic without requiring actual files
    # The method should return a ConfigurationError if no paths are found
    with pytest.raises(gridlabd.ConfigurationError):
        Simulation()  # No gridlabd_path provided, should try auto-detection and fail


if __name__ == "__main__":
    # Run tests manually if pytest is not available
    print("Running GridLAB-D Python bindings tests...")
    
    try:
        test_basic_imports()
        print("✓ Basic imports test passed")
        
        test_version_info()
        print("✓ Version info test passed")
        
        test_error_code_enums()
        print("✓ Error code enums test passed")
        
        test_checkpoint_mode_enums()
        print("✓ Checkpoint mode enums test passed")
        
        test_exception_hierarchy()
        print("✓ Exception hierarchy test passed")
        
        test_package_exports()
        print("✓ Package exports test passed")
        
        # Simulation class tests
        sim_tests = TestSimulationClass()
        sim_tests.test_simulation_path_validation()
        print("✓ Simulation path validation test passed")
        
        sim_tests.test_simulation_path_detection()
        print("✓ Simulation path detection test passed")
        
        sim_tests.test_simulation_properties()
        print("✓ Simulation properties test passed")
        
        sim_tests.test_simulation_context_manager()
        print("✓ Simulation context manager test passed")
        
        sim_tests.test_load_model_validation()
        print("✓ Load model validation test passed")
        
        # Low-level API tests
        ll_tests = TestLowLevelAPI()
        ll_tests.test_gridlabd_class_availability()
        print("✓ Low-level API availability test passed")
        
        test_environment_detection()
        print("✓ Environment detection test passed")
        
        print("\n🎉 All tests passed successfully!")
        
    except Exception as e:
        print(f"\n✗ Test failed: {e}")
        import traceback
        traceback.print_exc()
        exit(1)
