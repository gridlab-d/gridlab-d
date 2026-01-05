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
# Import low-level C++ API (enums and utilities only)
from .gridlabd_core import (
    hello,
    __version__,
    GLDErrorCode,
    GLDCheckPointMode,
)

# Import isolated GridLabD wrapper (process isolation for multiple instances)
from ._isolated import IsolatedGridLabD

# Always use isolated wrapper for safety and multi-instance support
GridLabD = IsolatedGridLabD

__all__ = [
    # Main API
    "GridLabD",
    "GLDErrorCode",
    "GLDCheckPointMode",
    
    # Utility functions
    "hello",
    "__version__",
    "version",
    "info",
    
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
    print("GridLabD Mode: Isolated (multiple instances supported)")

