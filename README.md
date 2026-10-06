# Building GridLAB-D

Instructions for building the Python package, including native Windows wheels
and editable installs, are in [python_bindings/README.md](python_bindings/README.md).

## Prerequisites

CMake  
CCMake or CMake-gui (optional)   
Visual Studio 2022 or newer (Windows), gcc/g++ (Linux), or Clang (MacOS)

## Installation

### Windows

#### Git

Clone the git repository for GridLAB-D and update submodules:

```powershell
git clone https://github.com/gridlab-d/gridlab-d.git
cd gridlab-d
git submodule update --init
```

#### Prepare out-of-source build directory

Create build directory and move into it:

```powershell
mkdir cmake-build
cd cmake-build
```

#### Generate the build system

CMake flags can be added using the `-D` prefix. GridLAB-D relies on vcpkg in Windows to retrieve prerequisite packages and thus requires the build system to be configured with `VCPKG_TARGET_TRIPLET` and `CMAKE_TOOLCHAIN_FILE` set by the user. 

Below is a general format guide, and an actual viable build command for most platforms.

```shell script
# Format:
cmake <flags> ..

# Full Example: 
cmake -B ./ -S ../ -DVCPKG_TARGET_TRIPLET=x64-windows -DCMAKE_TOOLCHAIN_FILE="C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\vcpkg\\scripts\\buildsystems\\vcpkg.cmake"
```
Please note that `VCPKG_TARGET_TRIPLET` must be assigned the value x64-windows and `CMAKE_TOOLCHAIN_FILE` must be assigned the path to your installed Visual Studio's vcpkg.cmake file location. Usually this is "C:\\Program Files\\Microsoft Visual Studio\\{your installed version}\\Communitiy\\VC\\vcpkg\\scripts\\buildsystems\\vcpkg.cmake".

#### Build and install the application

CMake can directly invoke the build and install process by running the below command. Multiprocess build is enabled
through the `-j#` flag (`-j8` in the included example).

```powershell
# Run the build system and install the application
cmake --build . -j8 --config Release
cmake --install . -j8 --config Release
```

### Linux and MacOS

#### Git

Clone the git repository for GridLAB-D and update submodules:

```bash
git clone https://github.com/gridlab-d/gridlab-d.git
cd gridlab-d
git submodule update --init
```

#### Prepare out-of-source build directory

Create build directory and move into it:

```bash
mkdir cmake-build
cd cmake-build
```

#### Generate the build system

CMake flags can be added using the `-D` prefix.

Below is a general format guide, and an actual viable build command for most platforms.

```bash
# Format:
cmake <flags> ..

# Full Example: 
cmake -B ./ -S ../
```

#### Build and install the application

CMake can directly invoke the build and install process by running the below command. Multiprocess build is enabled
through the `-j#` flag (`-j8` in the included example).

```bash
# Run the build system and install the application
cmake --build . -j8 --config Release
cmake --install . -j8 --config Release
```

## CMake Variables

The following variables affect the build process and can be changed using the `-D` flag at build generation or by
updating the cache using ccmake or cmake-gui (default values are shown).

| Variable             | Valid Values                                       | Description                                     | Linux/Mac Default | Windows Default |
|----------------------|----------------------------------------------------|-------------------------------------------------|-------------------|-----------------|
| CMAKE_BUILD_TYPE     | 'Debug', 'RelWithDebInfo', 'MinSizeRel', 'Release' | Compiler optimizer configuration                | Debug             | Debug           |
| CMAKE_INSTALL_PREFIX | Any path                                           | Install location                                | /usr/local        | %ProgramFiles%  |
| GLD_USE_HELICS       | ON/OFF                                             | Enables detection and use of HELICS             | OFF               | OFF             |
| GLD_HELICS_DIR       | Any path                                           | Hint indicating HELICS install directory        |                   |                 |
| GLD_DO_CLEANUP       | ON/OFF                                             | Remove files from old GridLAB-D build processes | OFF               | OFF             |

### Enable building with HELICS

To enable HELICS set the `GLD_USE_HELICS` flag to `ON`
if HELICS is in a custom path set `GLD_HELICS_DIR` to the install location in CMake or as an environmental variable

### Enable build debugging

To output all build commands during build, set following flag to `ON`

```
CMAKE_VERBOSE_MAKEFILE=OFF 
```
