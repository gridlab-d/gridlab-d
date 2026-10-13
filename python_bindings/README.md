# GridLAB-D Python Bindings

This package provides Python bindings for GridLAB-D, a power system simulation platform.

## Building and installing on Windows

Windows builds use 64-bit MSVC and a Release core. The Python extension and the
core must use the same architecture and build configuration.

### Prerequisites

- 64-bit Python 3.10 or newer
- Visual Studio 2022 or Build Tools 2022 with **Desktop development with C++**
- CMake available from a native Windows terminal
- Git submodules initialized with `git submodule update --init --recursive`

Use Command Prompt, PowerShell, or the x64 Native Tools prompt. Ensure `cmake`
does not resolve to an MSYS2 installation (`where cmake` in Command Prompt or
`Get-Command cmake` in PowerShell).

### Build the native core

Run these commands from the repository root:

```cmd
cmake -S . -B out/build/windows-native -G "Visual Studio 17 2022" -A x64
cmake --build out/build/windows-native --config Release --parallel 4
```

The build must produce both of these files:

```text
out/build/windows-native/bin/Release/gldapi.dll
out/build/windows-native/lib/static/Release/gldapi.lib
```

### Install for development

Creating a virtual environment is recommended:

```cmd
python -m venv .venv-windows-api
.venv-windows-api\Scripts\activate
python -m pip install --upgrade pip
python -m pip install pytest pytest-timeout
```

Install the package from the repository root and explicitly identify the core
build. Forward slashes in the CMake path work in both Command Prompt and
PowerShell:

```cmd
python -m pip install --force-reinstall --config-settings=cmake.define.GRIDLABD_BUILD_DIR=C:/path/to/gridlab-d/out/build/windows-native ./python_bindings
```

For an editable install, use:

```cmd
python -m pip install --force-reinstall -e ./python_bindings --config-settings=cmake.define.GRIDLABD_BUILD_DIR=C:/path/to/gridlab-d/out/build/windows-native
```

Replace `C:/path/to/gridlab-d` with the repository's absolute path. Reinstall
the package after rebuilding the core so the installed `gldapi.dll` and module
DLLs are refreshed.

`GRIDLABD_BUILD_DIR` prevents CMake from selecting artifacts from another build
tree. The package links to the Release `gldapi.lib` and installs the matching
`gldapi.dll`, model-module DLLs, and runtime data beside the Python extension.

### Verify the installation

From the repository root:

```cmd
python -X faulthandler -c "import gridlabd; print(gridlabd.version()); print(gridlabd.GridLabD.get_install_root())"
python -X faulthandler -m pytest python_bindings/tests -v --tb=short
```

When the current directory is `python_bindings`, use `tests` instead of
`python_bindings/tests` in the pytest command.

### Runtime environment variables

An installed package normally finds its bundled runtime without setting
`GRIDLABD_HOME`, `GRIDLABD_ROOT`, or `GLPATH`. Old values pointing at a source
tree can select unrelated native modules. To diagnose an older installation,
clear them before testing:

```cmd
set GRIDLABD_HOME=
set GRIDLABD_ROOT=
set GLPATH=
```

PowerShell equivalents are:

```powershell
Remove-Item Env:GRIDLABD_HOME -ErrorAction SilentlyContinue
Remove-Item Env:GRIDLABD_ROOT -ErrorAction SilentlyContinue
Remove-Item Env:GLPATH -ErrorAction SilentlyContinue
```

Current builds place their bundled `share` and `lib` directories first in
`GLPATH`, which prevents a source-root override from mixing incompatible module
DLLs with the installed extension.

## Building Wheels for Manual Distribution

Wheels use nanobind 3 split mode and target the CPython 3.10 stable ABI.
A Windows x64 build produces a cp310-abi3-win_amd64 wheel for standard
CPython 3.10 and newer. pip installs the matching nanobind-backend dependency.
Windows builds require MSVC with the shared Release CRT (/MD). Test the same
wheel on Python 3.10 through 3.14 before release. Free-threaded Python requires
a separate build and is not covered by this abi3 wheel.

This preparation step is handled by the GitHub Actions build pipeline. For manual builds:

### Linux & macOS

1. **Run the preparation script** (copies built libraries into the package):
   ```bash
   cd python_bindings
   ./prepare_pypi_build.sh
   ```

2. **Build the distribution files**:
   ```bash
   python3 -m build
   ```

   This creates both files in the `dist/` directory.

### Windows (MSVC x64)

After building the Release core as described above, create a standalone wheel
from the repository root:

```cmd
python -m pip wheel ./python_bindings --no-deps -w out/wheels --config-settings=cmake.define.GRIDLABD_BUILD_DIR=C:/path/to/gridlab-d/out/build/windows-native
```

Replace <version> below with the built package version. Test the wheel in a clean virtual environment and from outside the repository,
so the checkout cannot shadow its installed Python files:

```cmd
python -m venv out/wheel-test-env
out\wheel-test-env\Scripts\python -m pip install "out\wheels\gridlabd-<version>-cp310-abi3-win_amd64.whl"
cd %TEMP%
C:\path\to\gridlab-d\out\wheel-test-env\Scripts\python -c "import gridlabd; print(gridlabd.version()); print(gridlabd.GridLabD().get_clock())"
```

GitHub Actions performs the same native Release build on `windows-2022` before
running cibuildwheel. The workflow explicitly initializes the MSVC x64 developer
environment, so it does not depend on an interactive Native Tools prompt.
Windows 32-bit wheels are not supported.

## API Usage Examples

### Basic Usage

```python
import gridlabd

# Create a GridLAB-D instance
gld = gridlabd.GridLabD()

# Load a model file
result = gld.load("path/to/model.glm")
assert result == 0, "Failed to load model"

# Initialize the model
result = gld.setup_after_load()
assert result == 0, "Failed to initialize"

# Run the simulation
result = gld.run()
assert result == 0, "Simulation failed"

print("Simulation completed successfully!")
```

### Querying Objects and Properties

```python
import gridlabd

# Load and initialize model
gld = gridlabd.GridLabD()
gld.load("test_HVAC_balance.glm")
gld.setup_after_load()

# Get all classes in the model
classes = gld.get_all_classes()
print(f"Classes in model: {classes}")

# Get all objects of a specific class
houses = gld.get_object_names_by_class("house")
print(f"Found {len(houses)} houses")

# Get properties from a single object
if houses:
    props = gld.get_object_properties(houses[0])
    print(f"Floor area: {props.get('floor_area')}")

# Get all objects with all their properties
all_houses = gld.get_all_objects("house")
for house in all_houses:
    house_name = house.get("__name__", house.get("__id__", "(unnamed)"))
    floor_area = house.get("floor_area", "N/A")
    print(f"House {house_name}: floor_area={floor_area}")

# Get entire model as nested dictionary
model = gld.get_model()
for class_name, objects in model.items():
    print(f"{class_name}: {len(objects)} objects")
```

### Setting Properties

```python
import gridlabd

gld = gridlabd.GridLabD()
gld.load("model.glm")
gld.setup_after_load()

# Get objects
houses = gld.get_object_names_by_class("house")

# Set a property value
if houses:
    result = gld.set_property(houses[0], "air_temperature", "72 degF")
    print(f"Set temperature result: {result}")
    
    # Verify the change
    result, new_value = gld.get_property(houses[0], "air_temperature")
    print(f"New temperature: {new_value}")
```

### Stepping Through Simulation

```python
import gridlabd

gld = gridlabd.GridLabD()
gld.load("model.glm")
gld.setup_after_load()

# Step through simulation timestep by timestep
for i in range(10):
    status, timestamp = gld.step()
    if status < 0:
        print("Simulation complete")
        break
    
    # Query state at each timestep
    houses = gld.get_all_objects("house")
    if houses:
        temp = houses[0].get('air_temperature')
        print(f"Timestep {i}, Time {timestamp}: Temperature = {temp}")
```

### Message Capture

GridLAB-D messages (warnings, errors, debug output) are automatically captured and can be retrieved programmatically. By default, C++ output is suppressed to keep your console clean.

```python
import gridlabd

# Default: C++ output suppressed, clean console
gld = gridlabd.GridLabD()

# Load and run model
gld.load("model.glm")
gld.setup_after_load()
gld.run()

# Get captured messages programmatically
messages = gld.get_messages()
for msg in messages:
    print(f"[{msg['type']}] {msg['timestamp']}: {msg['message']}")

# Filter for errors only
errors = [m for m in messages if m['type'] == 'ERROR']
print(f"Found {len(errors)} errors")
```

**Verbose mode** - Enable C++ console output for debugging:

```python
# Show C++ output on stderr (useful for debugging)
gld = gridlabd.GridLabD(verbose=True)
gld.load("model.glm")
gld.setup_after_load()
gld.run()

# Messages are still captured even in verbose mode
messages = gld.get_messages()
```

**Message capture controls**:

```python
# Disable message capture (not recommended)
gld.enable_message_capture(False)

# Clear captured messages
gld.clear_messages()

# Set message limit (default: 10000)
gld.set_message_capture_limit(5000)
limit = gld.get_message_capture_limit()
```
