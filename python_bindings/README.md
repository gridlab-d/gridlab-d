# GridLAB-D Python Bindings

This package provides Python bindings for GridLAB-D, a power system simulation platform.

## Features

- **Self-contained package**: All required data files (`tzinfo.txt`, `unitfile.txt`) are bundled
- **Automatic path configuration**: No manual setup needed
- **Low-level API**: Direct access to GridLAB-D C++ API
- **High-level interface**: Pythonic simulation management

## Installation

### From Source (Development)

1. **Build GridLAB-D core** (if not already built):
   ```bash
   cd build
   cmake --build . --parallel
   ```

2. **Install Python package in development mode**:
   ```bash
   cd python_bindings
   pip install -e .
   ```
   
   **Note**: In development mode (`-e`), the package links to your source tree. It automatically finds:
   - Data files in `gldcore/` (tzinfo.txt, unitfile.txt)
   - Modules in `build/lib/` (residential.so, climate.so, etc.)

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

# Using the high-level Simulation class
with gridlabd.Simulation() as sim:
    sim.load_model("model.glm")
    results = sim.run()
```

### Handling File References in GLM Files

GLM files often reference external files (climate data, CSV files, etc.) using relative paths. GridLAB-D resolves these paths from the **current working directory**.

**Solution 1: Use the convenience function** (recommended):
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

**Note**: All necessary GridLAB-D modules are bundled with the package, making it fully self-contained. Users don't need a separate GridLAB-D installation!

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

**Note**: Climate data files (TMY2/TMY3 weather files) are NOT bundled. You need to provide these separately if your models use them. See "Handling File References in GLM Files" section above for how to reference external data files.

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

## Documentation

- **[QUICK_REFERENCE.md](QUICK_REFERENCE.md)** - Quick start guide and common tasks
- **[TZINFO_FIX_SUMMARY.md](TZINFO_FIX_SUMMARY.md)** - Technical details on path configuration
- **[PACKAGE_DATA_FIX.md](PACKAGE_DATA_FIX.md)** - Implementation details

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
