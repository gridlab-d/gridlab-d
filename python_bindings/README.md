# GridLAB-D Python Bindings

This package provides Python bindings for GridLAB-D, a power system simulation platform.

## Features

- **Self-contained package**: All required data files (`tzinfo.txt`, `unitfile.txt`) are bundled
- **Automatic path configuration**: No manual setup needed
- **Low-level API**: Direct access to GridLAB-D C++ API
- **High-level interface**: Pythonic simulation management

## Installation

### From Source (Development)

1. **Build the GridLAB-D core** (if not already built):
   ```bash
   # Create out-of-source build directory
   mkdir cmake-build
   cd cmake-build
   
   # Configure the build (use ccmake for interactive configuration)
   ccmake ../
   
   # Build and install (use sudo if installing to default system location)
   sudo cmake --build . --target install
   
   # Verify GridLAB-D installation
   cd ../
   gridlabd -T 0 --validate
   ```

2. **Install the Python package in development mode**:
   ```bash
   cd python_bindings
   pip install -e .
   ```
   
   **Note**: In development mode (`-e`), the package links to your source tree. It automatically finds:
   - GridLAB-D libraries in `cmake-build/lib/` or `build/lib/`
   - Data files in `gldcore/` (tzinfo.txt, unitfile.txt)
   - Modules (residential.so, climate.so, etc.)

3. **Verify installation**:
   ```python
   import gridlabd
   gld = gridlabd.GridLabD()
   print("✓ Package working correctly!")
   ```

### From PyPI (future)

```bash
pip install gridlabd
```

## Basic Usage

### Quick Start

```python
import gridlabd

# Option 1: Use the convenience function (recommended)
# Automatically changes to the model's directory so relative paths work
gld, result = gridlabd.load_model("path/to/model.glm")
if result == gridlabd.GLDErrorCode.SUCCESS:
    gld.run()
    gld.exit_gld("")

# Option 2: Manual approach
gld = gridlabd.GridLabD()
gld.load_glm(["gridlabd", "model.glm"])
gld.run()
gld.exit_gld("")
```

### High-Level Interface

```python
import gridlabd

# Use the high-level Simulation class
with gridlabd.Simulation() as sim:
    sim.load_model("model.glm")
    results = sim.run()
```

### Handling File References in GLM Files

GLM files often reference external files (climate data, CSV files, etc.) using relative paths. GridLAB-D resolves these paths from the **current working directory**.

**Solution 1: Use the convenience function (recommended)**:
```python
import gridlabd

# Automatically changes to model's directory before loading
gld, result = gridlabd.load_model("models/my_model.glm")
# Now relative paths in my_model.glm work correctly!
```

**Solution 2: Manually set working directory**:
```python
import gridlabd
from pathlib import Path

gld = gridlabd.GridLabD()

# Change to the model's directory
model_path = Path("models/my_model.glm").resolve()
gld.set_working_directory(str(model_path.parent))

# Now load the model
gld.load_glm(["gridlabd", str(model_path)])
```

**Solution 3: Use absolute paths in your GLM file**:
```python
# If your GLM uses absolute paths, you don't need to change directories
gld = gridlabd.GridLabD()
gld.load_glm(["gridlabd", "/absolute/path/to/model.glm"])
```

### Path Configuration (Advanced)

The package automatically discovers paths, but you can override if needed:

```python
import gridlabd

# Set custom GridLAB-D installation
gridlabd.GridLabD.set_install_root("/path/to/gridlabd")

# Query paths
print("Install root:", gridlabd.GridLabD.get_install_root())
print("Executable:", gridlabd.GridLabD.get_executable_path())
```

### Using Custom GridLAB-D Installations

If you want to use a different GridLAB-D installation (e.g., for custom modules or data files) alongside the bundled PyPI package, use the `GRIDLABD_HOME` environment variable:

#### Quick Start

```bash
# Point to your custom GridLAB-D installation
export GRIDLABD_HOME=/usr/local/custom-gridlabd

# Run your Python code
python my_simulation.py
```

#### What Gets Used

When `GRIDLABD_HOME` is set, the package uses a **hybrid approach**:
- ✅ **Core API**: Uses bundled `libgldapi.so` (from PyPI package)
- ✅ **Modules**: Uses custom modules from `$GRIDLABD_HOME/lib/` (e.g., `residential.so`)
- ✅ **Data files**: Uses custom data from `$GRIDLABD_HOME/share/` (e.g., `tzinfo.txt`)

This allows you to:
- Use custom or modified GridLAB-D modules
- Override built-in data files
- Experiment with development builds
- Support multiple GridLAB-D configurations

#### Directory Structure Requirements

Your custom installation must have this structure:

```
/usr/local/custom-gridlabd/
├── lib/                    # Required: GridLAB-D modules
│   ├── residential.so
│   ├── powerflow.so
│   └── ... (other modules)
└── share/                  # Required: Data files
    ├── tzinfo.txt
    └── unitfile.txt
```

Or for development builds:

```
/path/to/gridlabd-dev/
├── gldcore/               # Data files location
│   ├── tzinfo.txt
│   └── unitfile.txt
└── build/lib/             # Compiled modules
    ├── residential.so
    └── ...
```

#### Usage Examples

**Example 1: Use Custom Modules**
```bash
# Build custom GridLAB-D with modified modules
cd /path/to/custom-gridlabd
cmake -B build
cmake --build build --parallel

# Use custom installation
export GRIDLABD_HOME=/path/to/custom-gridlabd
python -c "import gridlabd; print(gridlabd.GridLabD.get_install_root())"
# Output: /path/to/custom-gridlabd
```

**Example 2: Switch Between Installations**
```bash
# Use default bundled installation
unset GRIDLABD_HOME
python my_sim.py

# Use custom installation
export GRIDLABD_HOME=/usr/local/gridlabd-dev
python my_sim.py
```

**Example 3: Per-Project Configuration**
```python
import os
os.environ['GRIDLABD_HOME'] = '/path/to/project/gridlabd'
import gridlabd  # Will use custom installation

gld = gridlabd.GridLabD()
# Now using modules from /path/to/project/gridlabd/lib/
```

#### Important Limitations

⚠️ **Core API Version**: The bundled `libgldapi.so` (core API) is compiled into the PyPI package and **cannot be swapped** via `GRIDLABD_HOME`. Only modules and data files can be customized.

**If you need a different core API version**, you must:
1. **Rebuild the Python package** from source against the desired GridLAB-D version
2. **Use development mode** with `pip install -e python_bindings/`

#### Validation

The package validates that your custom installation has the required structure:

```python
import gridlabd
import os

os.environ['GRIDLABD_HOME'] = '/invalid/path'

try:
    gridlabd.GridLabD.set_install_root(os.environ['GRIDLABD_HOME'])
except RuntimeError as e:
    print(f"Validation failed: {e}")
    # Output: Invalid GridLAB-D installation: /invalid/path 
    #         (missing required directories: share/, lib/, or gldcore/)
```

#### Environment Variable Priority

The package checks environment variables in this order:
1. **`GRIDLABD_HOME`** - Custom installation (highest priority)
2. **`GRIDLABD_ROOT`** - Backward compatibility with older versions
3. **Auto-detection** - Bundled package files (default)

```bash
# GRIDLABD_HOME takes precedence
export GRIDLABD_ROOT=/old/path
export GRIDLABD_HOME=/new/path
python -c "import gridlabd; print(gridlabd.GridLabD.get_install_root())"
# Output: /new/path
```

## Package Structure

After installation:
```
site-packages/gridlabd/
├── __init__.py              # Auto-configures paths
├── gridlabd_core.*.so       # C++ extension module
├── libgldapi.so            # GridLAB-D API library
├── share/                  # Bundled data files
│   ├── tzinfo.txt         # Timezone database
│   └── unitfile.txt       # Units database
├── lib/                    # GridLAB-D modules (self-contained!)
│   ├── residential.so     # Residential loads module
│   ├── commercial.so      # Commercial loads module
│   ├── powerflow.so       # Power flow solver
│   ├── climate.so         # Climate/weather module
│   ├── tape.so            # Data recording module
│   └── ... (all other GridLAB-D modules)
├── simulation.py          # High-level interface
└── bundle_utils.py        # Path utilities
```

**Note**: All necessary GridLAB-D modules are bundled with the package, making it fully self-contained. Users do not need a separate GridLAB-D installation!

## What Gets Bundled

When you install the package (not in development mode), it includes:

### Core Components
- **gridlabd_core.so** - Python bindings (nanobind module)
- **libgldapi.so** - GridLAB-D C++ API library

### Data Files
- **share/tzinfo.txt** - Timezone definitions (600+ timezones worldwide)
- **share/unitfile.txt** - Unit conversion database

### All GridLAB-D Modules (17 modules, ~27 MB total)
- **residential.so** - Residential building loads and HVAC
- **commercial.so** - Commercial building loads
- **powerflow.so** - Power flow analysis and distribution systems
- **climate.so** - Weather data and climate modeling
- **tape.so** - Data recording and playback
- **assert.so** - Test assertions
- **generators.so** - Generator models (diesel, solar, wind, etc.)
- **reliability.so** - Reliability analysis
- **market.so** - Economic dispatch and markets
- **connection.so** - External system connections
- **optimize.so** - Optimization algorithms
- **tape_file.so** - File-based data recording
- **tape_plot.so** - Plotting and visualization
- **mysql.so** - MySQL database integration
- And more...

**Total package size**: ~33 MB (compressed to ~13 MB in wheel)

**Result**: A fully self-contained Python package that works without any external GridLAB-D installation!

**Note**: Climate data files (TMY2/TMY3 weather files) are not bundled. You need to provide these separately if your models use them. See the "Handling File References in GLM Files" section above for how to reference external data files.

## Development

### Building

The bindings use:
- **nanobind**: Modern Python binding framework
- **scikit-build-core**: Python packaging with CMake
- **CMake**: Build system integration

### Rebuilding After Changes

```bash
cd python_bindings
pip install -e .
```

That's it! The package will:
- Automatically find the GridLAB-D core library
- Bundle required data files
- Configure paths on import

### Testing

Run the test suite:
```bash
cd python_bindings
python test_tzinfo_fix.py
```

Or test manually:
```python
import gridlabd
from pathlib import Path

# Verify data files are bundled
pkg_dir = Path(gridlabd.__file__).parent
print("tzinfo.txt exists:", (pkg_dir / "share" / "tzinfo.txt").exists())

# Test instance creation
gld = gridlabd.GridLabD()
print("✓ Instance created successfully!")
```

## Troubleshooting

### tzinfo.txt not found error?

This should not happen with the current version, as data files are bundled. If you see this:

1. **Check installation**:
   ```python
   import gridlabd
   from pathlib import Path
   share_dir = Path(gridlabd.__file__).parent / "share"
   print("Share directory exists:", share_dir.exists())
   print("Files:", list(share_dir.glob("*")) if share_dir.exists() else "None")
   ```

2. **Reinstall package**:
   ```bash
   pip uninstall -y gridlabd
   pip install -e python_bindings/
   ```

3. **Verify core library**:
   ```bash
   ls -la build/lib/libgldapi.so
   ```
   If missing, rebuild core: `cd build && cmake --build . --parallel`

### Import errors?

Make sure GridLAB-D core is built:
```bash
cd build
cmake --build . --parallel
```

Then reinstall Python package:
```bash
cd ../python_bindings
pip install -e .
```

## Building for PyPI Distribution

This section describes how to create distributable packages for uploading to PyPI.

### Overview

The PyPI build process creates self-contained wheels that include all necessary GridLAB-D libraries and modules. Users can install with just `pip install gridlabd` without needing to build GridLAB-D from source.

### Prerequisites

1. **GridLAB-D must be built first**:
   ```bash
   cd build
   cmake --build . --parallel
   ```
   This creates `libgldapi.so` and all GridLAB-D modules in `build/lib/`

2. **Install build tools**:
   ```bash
   pip install build twine
   ```

### Build Process

#### Step 1: Prepare Prebuilt Files

Run the preparation script to copy necessary files:

```bash
cd python_bindings
./prepare_pypi_build.sh
```

**What this script does:**
- Creates the `prebuilt/` directory structure
- Copies `libgldapi.so` and all GridLAB-D module libraries (`.so` files)
- Copies essential data files (`tzinfo.txt`, `unitfile.txt`)  
- Copies required header files for compilation
- Copies jsoncpp include files

**Generated structure:**
```
python_bindings/
├── prebuilt/
│   ├── lib/
│   │   ├── libgldapi.so           # Core API library
│   │   ├── residential.so         # All GridLAB-D modules
│   │   ├── powerflow.so
│   │   ├── climate.so
│   │   └── ... (17 modules total)
│   └── share/
│       ├── tzinfo.txt            # Timezone database
│       └── unitfile.txt          # Units database
├── gldcore/                      # Copied header files
└── third_party/jsoncpp_lib/      # Copied jsoncpp headers
```

#### Step 2: Build the Package

```bash
python -m build
```

This creates:
- **Source distribution** (`.tar.gz`) — Contains source code and prebuilt files
- **Wheel** (`.whl`) — Binary distribution ready for installation

#### Step 3: Test the Package

```bash
# Install the wheel locally to test
pip install --force-reinstall dist/gridlabd-5.0.0-*.whl

# Verify it works
python -c "import gridlabd; print('Version:', gridlabd.version()); print('Test:', gridlabd.hello())"
```

#### Step 4: Upload to PyPI

```bash
# Upload to Test PyPI first (recommended)
twine upload --repository testpypi dist/*

# Test install from Test PyPI
pip install --index-url https://test.pypi.org/simple/ --extra-index-url https://pypi.org/simple/ gridlabd

# If everything works, upload to real PyPI
twine upload dist/*
```

### Architecture Details

#### Why Prebuild Libraries?

The PyPI approach uses **prebuilt libraries** rather than building GridLAB-D during `pip install` because:

1. **Fast installs** — No compilation needed (seconds vs minutes)
2. **Reliable** — Works consistently across environments  
3. **User-friendly** — No build dependencies required
4. **Self-contained** — Everything needed is included

#### Two-Mode CMakeLists.txt

The CMakeLists.txt automatically detects the build context:

```cmake
# Development mode — uses source tree
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../build/lib/libgldapi.so")
    set(GRIDLABD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
    
# PyPI mode — uses prebuilt files  
else()
    set(GRIDLABD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
```

#### What Gets Packaged

**Core Components:**
- Python bindings module (`gridlabd_core.so`)
- GridLAB-D API library (`libgldapi.so`)

**GridLAB-D Modules (~17 modules):**
- `residential.so`, `commercial.so`, `powerflow.so`
- `climate.so`, `tape.so`, `generators.so`
- `reliability.so`, `market.so`, `optimize.so`
- And more...

**Data Files:**
- `tzinfo.txt` — Timezone definitions (600+ timezones)
- `unitfile.txt` — Unit conversion database

**Total Size:** ~13 MB compressed wheel, ~33 MB installed

### Troubleshooting PyPI Builds

#### GridLAB-D Not Built Error
```
GridLAB-D API library not found at build/lib/libgldapi.so
```
**Solution:** Build GridLAB-D first by running: `cd build && cmake --build . --parallel`

#### Missing Prebuilt Files Error
```
PyPI mode: using prebuilt GridLAB-D libraries
CMake Error: libgldapi.so not found
```
**Solution:** Run `./prepare_pypi_build.sh` first

#### Import Errors After Install
```
ImportError: No module named 'gridlabd.gridlabd_core'
```
**Solution:** Check that the wheel was built correctly and installed from the right location

### Development vs PyPI Workflow

**Development Workflow:**
```bash
# Make changes to source
# Build and test quickly
pip install -e .
```

**PyPI Release Workflow:**
```bash
# 1. Ensure GridLAB-D is built
cd build && cmake --build . --parallel

# 2. Prepare PyPI package
cd ../python_bindings
./prepare_pypi_build.sh

# 3. Build distributable package
python -m build

# 4. Test locally
pip install --force-reinstall dist/gridlabd-5.0.0-*.whl

# 5. Upload to PyPI
twine upload dist/*
```

### File Management

The preparation script and build process create several directories that should **not** be committed to Git:

- `prebuilt/` — Copied libraries and data files
- `gldcore/` — Copied header files  
- `third_party/` — Copied jsoncpp files
- `dist/` — Built packages

These are automatically ignored via `.gitignore` entries.

## Documentation

- **[QUICK_REFERENCE.md](QUICK_REFERENCE.md)** — Quick start guide and common tasks
- **[TZINFO_FIX_SUMMARY.md](TZINFO_FIX_SUMMARY.md)** — Technical details on path configuration
- **[PACKAGE_DATA_FIX.md](PACKAGE_DATA_FIX.md)** — Implementation details

## API Reference

### Low-Level API

```python
# GridLabD class
gld = gridlabd.GridLabD()

# Static methods
gld.set_install_root(path)       # Set installation root
gld.get_install_root()            # Get installation root  
gld.get_executable_path()         # Get executable path

# Instance methods
gld.set_working_directory(dir)    # Set working directory for file resolution
gld.load_glm(args)                # Load GLM file
gld.run(start_time, stop_time)    # Run simulation
gld.step()                        # Step simulation
gld.get_checkpoint_json(path)     # Get state as JSON
gld.exit_gld(filepath)            # Exit simulation

# Convenience function
gld, result = gridlabd.load_model(path)  # Load model, auto-change to its directory

# Error codes
gridlabd.GLDErrorCode.SUCCESS
gridlabd.GLDErrorCode.OPERATION_FAILED
# ... etc
```

### High-Level API

```python
# Simulation class
sim = gridlabd.Simulation()
sim.load_model(path)
sim.run()
```

## License

This project follows the same license as GridLAB-D (Battelle Open Source License).
