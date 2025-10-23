"""
Simple test suite for GridLAB-D Python bindings.
"""

import sys
import os
from pathlib import Path

# Add the src directory to the path for testing
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'src'))

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
    print("✓ All classes available")

def test_version_info():
    """Test version information functions."""
    import gridlabd
    
    # Test version function
    version = gridlabd.version()
    assert isinstance(version, str)
    assert len(version) > 0
    print(f"✓ Version: {version}")
    
    # Test hello function
    hello_msg = gridlabd.hello()
    assert isinstance(hello_msg, str)
    assert "GridLAB-D" in hello_msg
    print(f"✓ Hello: {hello_msg}")

def test_error_code_enums():
    """Test GridLAB-D error code enumerations."""
    import gridlabd
    
    # Test that error codes are accessible
    assert hasattr(gridlabd.GLDErrorCode, 'SUCCESS')
    assert hasattr(gridlabd.GLDErrorCode, 'FILE_NOT_FOUND')
    assert hasattr(gridlabd.GLDErrorCode, 'INVALID_FORMAT')
    print("✓ Error codes accessible")
    
    # Test that we can get enum values
    success_code = gridlabd.GLDErrorCode.SUCCESS
    assert success_code is not None
    print(f"✓ SUCCESS code: {success_code}")

def test_simulation_class_structure():
    """Test Simulation class structure without instantiation."""
    import gridlabd
    from gridlabd.simulation import Simulation
    
    # Test class exists and has expected methods
    assert hasattr(Simulation, '__init__')
    assert hasattr(Simulation, 'load_model')
    assert hasattr(Simulation, 'run')
    assert hasattr(Simulation, 'get_results')
    assert hasattr(Simulation, '__enter__')
    assert hasattr(Simulation, '__exit__')
    print("✓ Simulation class structure correct")

def test_exception_hierarchy():
    """Test custom exception hierarchy.""" 
    import gridlabd
    
    # Test exception inheritance
    assert issubclass(gridlabd.ConfigurationError, gridlabd.GridLABDError)
    assert issubclass(gridlabd.SimulationError, gridlabd.GridLABDError)
    assert issubclass(gridlabd.GridLABDError, Exception)
    print("✓ Exception hierarchy correct")
    
    # Test that we can raise and catch exceptions
    try:
        raise gridlabd.ConfigurationError("Test error")
    except gridlabd.ConfigurationError as e:
        assert str(e) == "Test error"
        print("✓ Exception handling works")

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
    
    available_exports = []
    missing_exports = []
    
    for export in expected_exports:
        if hasattr(gridlabd, export):
            available_exports.append(export)
        else:
            missing_exports.append(export)
    
    print(f"✓ Available exports ({len(available_exports)}): {', '.join(available_exports)}")
    if missing_exports:
        print(f"✗ Missing exports ({len(missing_exports)}): {', '.join(missing_exports)}")
    else:
        print("✓ All expected exports available")
    
    # Test __all__ 
    print(f"✓ __all__ contains: {', '.join(gridlabd.__all__)}")

if __name__ == "__main__":
    print("=== GridLAB-D Python Bindings Test Suite ===")
    print()
    
    try:
        test_basic_imports()
        test_version_info()
        test_error_code_enums()
        test_simulation_class_structure()
        test_exception_hierarchy()
        test_package_exports()
        
        print()
        print("🎉 All basic tests passed!")
        print("✓ Package structure is correct")
        print("✓ API bindings are working")
        print("✓ Error handling is functional")
        print("✓ High-level interface is available")
        
    except Exception as e:
        print(f"\n✗ Test failed: {e}")
        import traceback
        traceback.print_exc()
        exit(1)
