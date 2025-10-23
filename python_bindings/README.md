# GridLAB-D Python Bindings

This package provides Python bindings for GridLAB-D, a power system simulation platform.

## Installation

```bash
pip install gridlabd
```

## Basic Usage

```python
import gridlabd

# Test the installation
gridlabd.info()

# Get version
print(f"Version: {gridlabd.version()}")
```

## Development

This is a minimal initial implementation. The bindings are built using:
- **nanobind**: Modern Python binding framework
- **scikit-build-core**: Modern Python packaging with CMake integration
- **CMake**: Build system integration with GridLAB-D core

## Next Steps

1. Expose GridLAB-D API functions from `gldapi.cpp`
2. Add comprehensive Python interfaces for simulation control
3. Implement error handling and type safety
4. Add comprehensive documentation and examples
5. Set up automated testing and CI/CD pipeline

## License

This project follows the same license as GridLAB-D.
