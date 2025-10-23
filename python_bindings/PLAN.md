# GridLAB-D Python Bindings: Implementation Plan & Reference

## Project Overview

This document outlines the comprehensive plan for implementing Python bindings for GridLAB-D using nanobind, enabling PyPI distribution with automated CI/CD integration.

**Primary Goal**: Expose GridLAB-D APIs from `gldapi.cpp` with nanobind for PyPI distribution  
**Approach**: Integrated approach where entire GridLAB-D core is linked and CI/CD pipeline rebuilds the PyPI library automatically

## Strategic Decisions Made

### 1. Technology Stack Selection
- **Binding Framework**: Nanobind v1.3.2+ (chosen over pybind11)
  - **Rationale**: Better performance, ABI stability, modern C++17 support
  - **Benefits**: Smaller binary sizes, faster compilation, stable ABI across Python versions
- **Build System**: scikit-build-core v0.4.3+ with CMake
  - **Rationale**: Modern Python packaging standard, excellent CMake integration
  - **Benefits**: Cross-platform builds, wheel generation, pip compatibility
- **Integration Strategy**: Full GridLAB-D core linking
  - **Rationale**: gldapi.cpp has extensive dependencies (exec.h, output.h, globals.h, etc.)
  - **Benefits**: Complete API access, maintains existing functionality

### 2. Project Structure Layout
```
python_bindings/
├── pyproject.toml          # Modern Python packaging configuration
├── CMakeLists.txt          # CMake build configuration  
├── README.md               # User documentation
├── PLAN.md                 # This reference document
├── src/
│   └── gridlabd/
│       └── __init__.py     # Python package interface
├── src_cpp/
│   └── gld_nanobind.cpp    # C++ nanobind wrapper
└── tests/
    └── test_basic.py       # Test framework
```

**Design Rationale**:
- **Separated C++ sources** (`src_cpp/`) from Python sources (`src/`) for clarity
- **Standard Python packaging** following PEP 517/518 standards
- **Modular structure** allowing incremental API exposure
- **Integrated within main repository** for unified CI/CD and version management

### 3. CI/CD Integration Strategy
- **GitHub Actions Pipeline**: Automated builds on push/PR
- **Cross-Platform Wheels**: Windows, macOS, Linux builds
- **PyPI Deployment**: Automatic publishing from main branch
- **Version Synchronization**: GridLAB-D version drives PyPI version
- **Build Matrix**: Multiple Python versions (3.8-3.12)

## Implementation Phases

### Phase 1: Foundation Setup ✅ COMPLETED
**Objective**: Create minimal working Python bindings structure

**Tasks Completed**:
- [x] Created `python_bindings/` directory structure
- [x] Configured `pyproject.toml` with modern Python packaging
- [x] Set up CMakeLists.txt for standalone builds
- [x] Implemented basic nanobind wrapper with hello world function
- [x] Created Python package interface (`__init__.py`)
- [x] Added basic test framework
- [x] Successfully built and tested minimal bindings

**Deliverables**:
- Working `import gridlabd` functionality
- Basic function exposure demonstration
- Build system ready for expansion

### Phase 2: Core API Integration 🔄 NEXT
**Objective**: Expose actual GridLAB-D API functions from `gldapi.cpp`

**Tasks**:
- [ ] Update CMakeLists.txt to link against GridLAB-D core library
- [ ] Include necessary GridLAB-D headers (gldapi.h, exec.h, etc.)
- [ ] Expose GridLabD class constructor and basic methods
- [ ] Implement error handling and exception mapping
- [ ] Add type conversions for GridLAB-D specific types

**Target API Functions**:
```cpp
// Priority 1: Basic simulation control
GridLabD::GridLabD()           // Constructor
GridLabD::run()               // Run simulation
GridLabD::step()              // Single step
GridLabD::get_global()        // Get global variables

// Priority 2: Model management  
GridLabD::load_glm()          // Load model files
GridLabD::get_object_count()  // Object queries
GridLabD::get_object()        // Object access

// Priority 3: Advanced features
GridLabD::get_checkpoint_json() // Checkpointing
GridLabD::run_until()          // Conditional execution
```

### Phase 3: Python API Design 🔄 FUTURE
**Objective**: Create Pythonic interface layer

**Tasks**:
- [ ] Design high-level Python classes wrapping C++ APIs
- [ ] Implement property access patterns
- [ ] Add context managers for simulation lifecycle
- [ ] Create NumPy integration for data arrays
- [ ] Add pandas DataFrame export capabilities

**Target Python Interface**:
```python
import gridlabd

# High-level simulation control
sim = gridlabd.Simulation("model.glm")
with sim:
    sim.run(until="2024-01-01 12:00:00")
    results = sim.get_results_dataframe()

# Object-oriented access
for obj in sim.objects:
    print(f"{obj.name}: {obj.properties}")
```

### Phase 4: Testing & Documentation 🔄 FUTURE
**Objective**: Comprehensive testing and user documentation

**Tasks**:
- [ ] Unit tests for all API functions
- [ ] Integration tests with real GLM models
- [ ] Performance benchmarking
- [ ] API documentation generation
- [ ] User guide and tutorials
- [ ] Example notebooks

### Phase 5: CI/CD Implementation 🔄 FUTURE  
**Objective**: Automated build and deployment pipeline

**Tasks**:
- [ ] GitHub Actions workflow for cross-platform builds
- [ ] Wheel building for multiple Python versions
- [ ] Automated testing on multiple platforms
- [ ] PyPI deployment automation
- [ ] Version management integration

## Debugging Guide

### Common Build Issues

#### 1. CMake Configuration Errors
**Problem**: `add_subdirectory given source which is not an existing directory`
```bash
CMake Error: add_subdirectory given source "/path/to/gldcore/solvers" which is not an existing directory
```
**Solution**: CMakeLists.txt is trying to include main GridLAB-D build system
- Use standalone CMakeLists.txt for Phase 1
- Add proper GridLAB-D core linking only in Phase 2
- Don't include `add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/.. ...)` until ready

#### 2. Nanobind Header Issues  
**Problem**: `fatal error: nanobind/stl.h: No such file or directory`
**Solution**: Remove `#include <nanobind/stl.h>` - not all nanobind installations include STL bindings
- Use `#include <nanobind/nanobind.h>` only for basic functionality
- For STL types, use `const char*` instead of `std::string` or install nanobind[stl]

#### 3. Module Import Errors
**Problem**: `ModuleNotFoundError: No module named 'gridlabd.gridlabd_core'`
**Solution**: CMake install configuration issue
- Ensure `install(TARGETS gridlabd_core LIBRARY DESTINATION gridlabd)` in CMakeLists.txt
- Module must be installed to correct package directory
- Check with `find /path/to/venv -name "*gridlabd_core*"`

#### 4. Type Conversion Errors
**Problem**: `Unable to convert function return value to a Python type`
**Solution**: Nanobind type system requirements
- Use `const char*` instead of `std::string` for simple strings
- Include appropriate nanobind headers for complex types
- Consider explicit type casting in nanobind definitions

### Build Command Reference

```bash
# Clean build
pip install -e . --force-reinstall

# Verbose build (for debugging)
pip install -e . --verbose

# Development install 
pip install -e . 

# Test installation
python -c "import gridlabd; gridlabd.info()"
```

### File System Issues

#### Windows WSL Path Problems
**Problem**: Files created but not visible in Windows Explorer
**Solution**: 
- Use terminal commands instead of file creation tools
- Verify paths with `pwd` and `ls -la`
- Check WSL2 vs WSL1 file system integration

## Dependencies & Requirements

### Build Dependencies
```toml
[build-system]
requires = ["scikit-build-core >=0.4.3", "nanobind >=1.3.2"]
```

### Runtime Dependencies  
```toml
[project]
requires-python = ">=3.8"
dependencies = ["numpy>=1.19.0"]
```

### Development Dependencies
- CMake >= 3.15
- C++17 compatible compiler
- Python development headers
- GridLAB-D core library (for Phase 2+)

## Quality Gates

### Phase 1 Acceptance Criteria ✅
- [x] `import gridlabd` succeeds without errors
- [x] Basic function calls work (`gridlabd.hello()`)
- [x] Version information accessible
- [x] Clean build process without warnings
- [x] Cross-platform compatibility (Linux verified)

### Phase 2 Acceptance Criteria 🔄
- [ ] GridLabD class instantiation works
- [ ] Basic simulation functions exposed
- [ ] Error handling for invalid inputs
- [ ] Memory management verified
- [ ] Integration tests pass

## Resource References

### Key Files in GridLAB-D Repository
- `gldcore/gldapi.cpp` - Main API implementation
- `gldcore/gldapi.h` - API header definitions  
- `gldcore/exec.h` - Execution engine dependencies
- `reference/gridlab-d-python_bindings/` - Previous implementation reference
- `reference/nanobind-master/` - Nanobind examples

### External Documentation
- [Nanobind Documentation](https://nanobind.readthedocs.io/)
- [scikit-build-core Guide](https://scikit-build-core.readthedocs.io/)
- [GridLAB-D Documentation](https://gridlab-d.shoutwiki.com/wiki/Main_Page)

## Decision Log

### Why Nanobind over Pybind11?
- **Performance**: Faster compilation, smaller binaries
- **Stability**: Better ABI stability across Python versions  
- **Modern**: Built for C++17, better type system
- **Future-proof**: Active development, industry adoption

### Why Integrated Approach vs Separate Repository?
- **Dependency Management**: Ensures version compatibility
- **CI/CD Simplification**: Single build pipeline
- **Development Workflow**: Easier testing with core changes
- **Distribution**: Automatic PyPI updates with GridLAB-D releases

### Why scikit-build-core vs setuptools?
- **Modern Standard**: PEP 517/518 compliance
- **CMake Integration**: Native CMake support
- **Cross-platform**: Better Windows/macOS support
- **Wheel Building**: Automatic binary distribution

---

**Document Version**: 1.0  
**Last Updated**: October 20, 2025  
**Status**: Phase 1 Complete, Phase 2 Ready to Begin
