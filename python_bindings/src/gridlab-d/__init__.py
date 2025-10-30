"""
GridLAB-D Python Bindings

This package provides Python bindings for the GridLAB-D power system simulation platform.

Example usage:
    # High-level interface (recommended)
    import gridlabd
    
    with gridlabd.Simulation() as sim:
        sim.load_model("model.glm")
        results = sim.run()
    
    # Low-level interface (advanced)
    gld = gridlabd.GridLabD()
    gld.setup_before_load()
"""

# Import low-level C++ API
from .gridlabd_core import (
    hello,
    __version__,
    GridLabD,
    GLDErrorCode,
    GLDCheckPointMode,
    set_install_root,
    get_install_root,
    get_executable_path,
    runtime_info,
)

# Import high-level Python API
from .simulation import (
    Simulation,
    GridLABDError,
    ConfigurationError, 
    SimulationError
)

__all__ = [
    # High-level interface (recommended for most users)
    "Simulation",
    "GridLABDError",
    "ConfigurationError",
    "SimulationError",
    
    # Low-level interface (advanced users)
    "GridLabD",
    "GLDErrorCode",
    "GLDCheckPointMode",
    "set_install_root",
    "get_install_root",
    "get_executable_path",
    
    # Utility functions
    "hello",
    "__version__",
    "version",
    "info",
    "runtime_info",
]

def version():
    """Return the GridLAB-D Python bindings version."""
    return __version__

def info():
    """Print information about the GridLAB-D Python bindings."""
    print(f"GridLAB-D Python Bindings v{__version__}")
    print("Built with nanobind")
    print(hello())
    print("GridLAB-D API integration: ✓ Available")

    try:
        details = runtime_info()
        install_root = details.get("install_root") or "(unknown)"
        executable = details.get("executable_path") or "(unknown)"
        print(f"Install root: {install_root}")
        print(f"Executable: {executable}")
    except Exception as e:
        print(f"Runtime info unavailable: {e}")

    try:
        sim = Simulation()
        print(f"GridLAB-D executable: ✓ Found at {sim.gridlabd_path}")
    except Exception as e:
        print(f"GridLAB-D executable: ✗ {e}")

# Bundle utilities
from .bundle_utils import get_gridlabd_info, setup_bundled_environment

# Add to __all__ exports
__all__.extend(['get_gridlabd_info', 'setup_bundled_environment'])
