"""
Utilities for managing bundled GridLAB-D installation.
"""

import os
import sys
from pathlib import Path
from typing import Optional


def get_bundled_gridlabd_path() -> Optional[str]:
    """
    Get the path to the bundled GridLAB-D executable.
    
    Returns:
        Path to gridlabd executable if found, None otherwise
    """
    # Get the package installation directory
    package_dir = Path(__file__).parent.parent  # Go up from gridlabd/bundle_utils.py
    
    # Look for bundled GridLAB-D in several possible locations
    possible_locations = [
        package_dir / "gridlabd_bundle" / "bin" / "gridlabd",  # Installed package
        package_dir / ".." / "gridlabd_bundle" / "bin" / "gridlabd",  # Development
        package_dir / ".." / ".." / "cmake-build" / "bin" / "gridlabd",  # Local build
    ]
    
    for path in possible_locations:
        if path.exists() and os.access(path, os.X_OK):
            return str(path.absolute())
    
    return None


def get_bundled_lib_path() -> Optional[str]:
    """
    Get the path to the bundled GridLAB-D libraries.
    
    Returns:
        Path to library directory if found, None otherwise
    """
    package_dir = Path(__file__).parent.parent
    
    possible_locations = [
        package_dir / "gridlabd_bundle" / "lib",  # Installed package
        package_dir / ".." / "gridlabd_bundle" / "lib",  # Development
        package_dir / ".." / ".." / "cmake-build" / "lib",  # Local build
    ]
    
    for path in possible_locations:
        if path.exists():
            return str(path.absolute())
    
    return None


def setup_bundled_environment() -> bool:
    """
    Set up environment variables to use bundled GridLAB-D.
    
    Returns:
        True if setup successful, False otherwise
    """
    gridlabd_path = get_bundled_gridlabd_path()
    lib_path = get_bundled_lib_path()
    
    if not gridlabd_path:
        return False

    exec_path = Path(gridlabd_path).resolve()
    os.environ['GRIDLABD_EXECUTABLE'] = str(exec_path)
    os.environ['GRIDLABD_ROOT'] = str(exec_path.parent.parent)

    # Add GridLAB-D executable directory to PATH
    gridlabd_dir = str(Path(gridlabd_path).parent)
    current_path = os.environ.get('PATH', '')
    if gridlabd_dir not in current_path:
        os.environ['PATH'] = f"{gridlabd_dir}:{current_path}"
    
    # Add library directory to LD_LIBRARY_PATH (Linux) or DYLD_LIBRARY_PATH (macOS)
    if lib_path:
        if sys.platform.startswith('linux'):
            lib_env = 'LD_LIBRARY_PATH'
        elif sys.platform == 'darwin':
            lib_env = 'DYLD_LIBRARY_PATH'
        else:
            lib_env = 'PATH'  # Windows
        
        current_lib_path = os.environ.get(lib_env, '')
        if lib_path not in current_lib_path:
            os.environ[lib_env] = f"{lib_path}:{current_lib_path}"
    
    return True


def get_gridlabd_info() -> dict:
    """
    Get information about the GridLAB-D installation.
    
    Returns:
        Dictionary with GridLAB-D installation info
    """
    bundled_path = get_bundled_gridlabd_path()
    lib_path = get_bundled_lib_path()
    
    info = {
        'bundled_executable': bundled_path,
        'bundled_libraries': lib_path,
        'environment_setup': bundled_path is not None,
        'in_path': 'gridlabd' in os.environ.get('PATH', ''),
    }
    
    return info
