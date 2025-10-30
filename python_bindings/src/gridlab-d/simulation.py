"""
High-level Python interface for GridLAB-D simulations.

This module provides user-friendly classes that wrap the low-level GridLabD C++ API
and handle configuration, environment setup, and error handling automatically.
"""

import os
import sys
from pathlib import Path
from typing import Optional, Union, Dict, Any
from .gridlabd_core import (
    GridLabD,
    GLDErrorCode,
    GLDCheckPointMode,
    set_install_root,
    get_install_root,
    get_executable_path,
)
from .bundle_utils import setup_bundled_environment, get_gridlabd_info


class GridLABDError(Exception):
    """Base exception for GridLAB-D Python binding errors."""
    pass


class ConfigurationError(GridLABDError):
    """Raised when GridLAB-D configuration is invalid or missing."""
    pass


class SimulationError(GridLABDError):
    """Raised when simulation execution fails."""
    pass


class SimulationResults:
    """Container for simulation results."""
    
    def __init__(self, success: bool = False, duration: float = 0.0, message: str = ""):
        self.success = success
        self.duration = duration
        self.message = message
    
    def __repr__(self):
        status = "SUCCESS" if self.success else "FAILED"
        return f"SimulationResults(status={status}, duration={self.duration:.2f}s)"


class Simulation:
    """
    High-level interface for GridLAB-D simulations.
    
    This class provides a user-friendly API that handles common simulation tasks,
    automatic configuration detection, and proper error handling.
    
    Example:
        with Simulation() as sim:
            sim.load_model("model.glm")
            results = sim.run()
            print(f"Simulation completed: {results}")
    """
    
    def __init__(self, gridlabd_path: Optional[str] = None, working_dir: Optional[str] = None, debug_mode: bool = False):
        """
        Initialize the GridLAB-D simulation interface.
        
        Args:
            gridlabd_path: Path to GridLAB-D executable (auto-detected if None)
            working_dir: Working directory for simulation (current dir if None)
            debug_mode: Enable debug output
        """
        self.gridlabd_path = str(gridlabd_path) if gridlabd_path else None
        self.working_dir = Path(working_dir) if working_dir else Path.cwd()
        self.debug_mode = debug_mode
        self._gld = None
        self._is_initialized = False
        self._model_loaded = False
        
        self._initialize()
    
    def _initialize(self):
        """Initialize the GridLAB-D API with bundled GridLAB-D."""
        try:
            env_configured = False

            if self.gridlabd_path:
                resolved = Path(self.gridlabd_path).resolve()
                if self.debug_mode:
                    print(f"Using explicit GridLAB-D path: {resolved}")
                set_install_root(str(resolved))
                self.gridlabd_path = str(resolved)
                gridlabd_dir = str(resolved.parent)
                current_path = os.environ.get('PATH', '')
                if gridlabd_dir and gridlabd_dir not in current_path:
                    os.environ['PATH'] = f"{gridlabd_dir}:{current_path}" if current_path else gridlabd_dir
                env_configured = True
            elif setup_bundled_environment():
                env_configured = True
                if self.debug_mode:
                    info = get_gridlabd_info()
                    print(f"Using bundled GridLAB-D: {info['bundled_executable']}")
                    print(f"Bundled libraries: {info['bundled_libraries']}")
            elif self.debug_mode:
                print("Warning: Could not find bundled GridLAB-D, using system installation")

            self._gld = GridLabD()

            resolved_exec = get_executable_path()
            if resolved_exec:
                self.gridlabd_path = resolved_exec

            if hasattr(self._gld, 'is_initialized') and self._gld.is_initialized():
                self._is_initialized = True
                if self.debug_mode:
                    print("✓ GridLAB-D API initialized successfully")
            else:
                self._is_initialized = True
                if self.debug_mode:
                    print("✓ GridLAB-D object created (initialization status unknown)")

            if self.debug_mode:
                install_root = get_install_root()
                if install_root:
                    print(f"Install root: {install_root}")
                if self.gridlabd_path:
                    print(f"Executable: {self.gridlabd_path}")
                if not env_configured:
                    print("Using environment defaults for GridLAB-D")

        except Exception as e:
            raise ConfigurationError(f"Failed to initialize GridLAB-D API: {e}")

    def load_model(self, model_path: Union[str, Path]):
        """
        Load a GridLAB-D model file.
        
        Args:
            model_path: Path to the GLM model file
            
        Raises:
            ConfigurationError: If model file not found
            SimulationError: If model loading fails
        """
        model_file = Path(model_path)
        if not model_file.exists():
            raise ConfigurationError(f"Model file not found: {model_path}")
        
        if not self._is_initialized:
            raise SimulationError("GridLAB-D API not initialized")
        
        try:
            # Convert to absolute path string for C++ API
            model_path_str = str(model_file.absolute())
            
            if self.debug_mode:
                print(f"Loading model: {model_path_str}")
            
            # Use the actual load_file method from our C++ API
            result = self._gld.load_glm(model_path_str)

            if result == GLDErrorCode.SUCCESS:
                self._model_loaded = True
                if self.debug_mode:
                    print("✓ Model loaded successfully")
            else:
                error_label = getattr(result, "name", str(result))
                raise SimulationError(f"Failed to load model {model_path} (error: {error_label})")
            
        except Exception as e:
            raise SimulationError(f"Failed to load model {model_path}: {e}")
    
    def run(self) -> SimulationResults:
        """
        Run the loaded simulation.
        
        Returns:
            SimulationResults: Results of the simulation
            
        Raises:
            SimulationError: If simulation fails or no model loaded
        """
        if not self._is_initialized:
            raise SimulationError("GridLAB-D API not initialized")
        
        if not self._model_loaded:
            raise SimulationError("No model loaded - call load_model() first")
        
        try:
            import time
            start_time = time.time()
            
            if self.debug_mode:
                print("Starting simulation...")
            
            # Use the new GridLAB-D API run method
            result = self._gld.run()
            
            end_time = time.time()
            duration = end_time - start_time
            
            if result == GLDErrorCode.SUCCESS:
                if self.debug_mode:
                    print(f"✓ Simulation completed successfully in {duration:.2f} seconds")
                
                return SimulationResults(
                    success=True,
                    duration=duration,
                    message="Simulation completed successfully"
                )
            else:
                error_label = getattr(result, "name", str(result))
                raise SimulationError(f"Simulation failed with error code: {error_label}")
            
        except Exception as e:
            return SimulationResults(
                success=False, 
                duration=0.0,
                message=f"Simulation failed: {e}"
            )
    
    def set_global(self, name: str, value: Any):
        """
        Set a global variable in the simulation.
        
        Args:
            name: Global variable name
            value: Variable value
        """
        if self.debug_mode:
            print(f"Setting global {name} = {value}")
        # TODO: Implement global variable setting with real API
    
    def get_results(self) -> Dict[str, Any]:
        """
        Get simulation results.
        
        Returns:
            Dictionary containing simulation results
        """
        if self.debug_mode:
            print("Getting simulation results")
        # TODO: Implement result retrieval with real API
        return {"status": "completed", "note": "Full results retrieval not yet implemented"}
    
    def cleanup(self):
        """Clean up simulation resources."""
        if self._gld and hasattr(self._gld, 'exit_gld'):
            try:
                self._gld.exit_gld("")
            except:
                pass  # Ignore cleanup errors
        
        self._model_loaded = False
        if self.debug_mode:
            print("✓ Simulation cleanup completed")
    
    def __enter__(self):
        """Context manager entry."""
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit with automatic cleanup."""
        self.cleanup()
        return False  # Don't suppress exceptions
