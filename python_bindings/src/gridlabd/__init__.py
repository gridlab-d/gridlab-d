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

import os
from pathlib import Path

# Set up GRIDLABD_ROOT to point to data files
# Priority: 1) Already set, 2) Package share/ dir, 3) Development source tree
_package_dir = Path(__file__).parent
_share_dir = _package_dir / "share"

if "GRIDLABD_ROOT" not in os.environ:
    # For installed package: use package's share and lib directories
    _lib_dir = _package_dir / "lib"
    if _share_dir.exists() and (_share_dir / "tzinfo.txt").exists():
        os.environ["GRIDLABD_ROOT"] = str(_package_dir)
        # Include both share (for data files) and lib (for modules)
        glpath = [str(_share_dir)]
        if _lib_dir.exists():
            glpath.append(str(_lib_dir))
    else:
        # For development mode: try to find the source tree
        # Go up from src/gridlabd/__init__.py to find gldcore/
        repo_root = _package_dir.parent.parent.parent  # src/gridlabd -> src -> python_bindings -> repo
        gldcore_dir = repo_root / "gldcore"
        build_lib_dir = repo_root / "build" / "lib"
        
        if gldcore_dir.exists() and (gldcore_dir / "tzinfo.txt").exists():
            # Development mode: point to repo root
            os.environ["GRIDLABD_ROOT"] = str(repo_root)
            # Include both gldcore (for tzinfo.txt) and build/lib (for modules)
            glpath_components = [str(gldcore_dir)]
            if build_lib_dir.exists():
                glpath_components.append(str(build_lib_dir))
            glpath = glpath_components
        else:
            # Last resort: just set GLPATH to empty and hope find_file() can locate it
            glpath = None
    
    # Set GLPATH to help find_file() locate tzinfo.txt and modules
    if glpath:
        glpath_components = []
        if "GLPATH" in os.environ:
            glpath_components.append(os.environ["GLPATH"])
        if isinstance(glpath, list):
            glpath_components.extend(glpath)
        else:
            glpath_components.append(glpath)
        
        # Use appropriate path separator for the platform
        path_sep = ";" if os.name == "nt" else ":"
        os.environ["GLPATH"] = path_sep.join(glpath_components)

# Import low-level C++ API
from .gridlabd_core import (
    hello,
    __version__,
    GridLabD,
    GLDErrorCode,
    GLDCheckPointMode,
)

# Set install root immediately after importing GridLabD class
# This allows the C++ code to find tzinfo.txt and other data files
if "GRIDLABD_ROOT" in os.environ:
    try:
        root = os.environ["GRIDLABD_ROOT"]
        GridLabD.set_install_root(root)
    except Exception:
        # If setting install root fails, continue anyway
        # The C++ code will use GLPATH environment variable
        pass

# Import high-level Python API
from .simulation import (
    Simulation,
    GridLABDError,
    ConfigurationError, 
    SimulationError
)

def load_model(glm_path, change_to_model_dir=True):
    """
    Load a GridLAB-D model, optionally changing to its directory first.
    
    This is a convenience function that handles the common pattern of:
    1. Creating a GridLabD instance
    2. Changing to the model's directory (so relative paths in the GLM work)
    3. Loading the model
    
    Args:
        glm_path: Path to the GLM file
        change_to_model_dir: If True, change working directory to the GLM file's directory
                            before loading (recommended). This allows relative paths in the
                            GLM file (like climate data files) to work correctly.
    
    Returns:
        tuple: (GridLabD instance, GLDErrorCode)
    
    Example:
        gld, result = gridlabd.load_model("path/to/model.glm")
        if result == gridlabd.GLDErrorCode.SUCCESS:
            gld.run()
    """
    from pathlib import Path
    
    gld = GridLabD()
    glm_file = Path(glm_path).resolve()
    
    if not glm_file.exists():
        return gld, GLDErrorCode.FILE_NOT_FOUND
    
    if change_to_model_dir:
        model_dir = str(glm_file.parent)
        result = gld.set_working_directory(model_dir)
        if result != GLDErrorCode.SUCCESS:
            return gld, result
    
    result = gld.load_glm(["gridlabd", str(glm_file)])
    return gld, result

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
    
    # Utility functions
    "hello",
    "__version__",
    "version",
    "info",
    "load_model",  # Convenience function
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
        sim = Simulation()
        print(f"GridLAB-D executable: ✓ Found at {sim.gridlabd_path}")
    except Exception as e:
        print(f"GridLAB-D executable: ✗ {e}")

# Bundle utilities
from .bundle_utils import get_gridlabd_info, setup_bundled_environment

# Add to __all__ exports
__all__.extend(['get_gridlabd_info', 'setup_bundled_environment'])
