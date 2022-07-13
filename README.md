# Building GridLAB-D

## Prerequisites

CMake  
CCMake or CMake-gui (optional)   
g++ or Clang

## Installation

### Git

Clone the git repository for GridLAB-D, and update submodules:

```shell script
git clone https://github.com/gridlab-d/gridlab-d.git
git submodule update --init
```

### Prepare out-of-source build directory

Change directory into folder, and create build directory. Change to build directory and invoke `cmake` as described
below:

```shell script 
cd gridlab-d
mkdir cmake-build
cd cmake-build
```

### Generate the build system

CMake flags can be added using the `-D` prefix, and different build systems can be selected using `-G`.

Below is a general format guide, and an actual viable build command for most platforms.

```shell script
# Format:
cmake <flags> ..

# Full Example: 
cmake -DCMAKE_INSTALL_PREFIX=~/software/GridLAB-D -DCMAKE_BUILD_TYPE=Release -G "CodeBlocks - Unix Makefiles" ..
```

### Build and install the application

CMake can directly invoke the build and install process by running the below command. Multiprocess build is enabled
through the `-j#` flag (`-j8` in the included example).

```shell script
# Run the build system and install the application
cmake --build . -j8 --target install
```

## CMake Variables

The following variables affect the build process and can be changed using the `-D` flag at build generation or by
updating the cache using ccmake or cmake-gui (default values are shown).

| Variable             | Valid Values                                       | Description                                                       | Linux/Mac Default | Windows Default |
|----------------------|----------------------------------------------------|-------------------------------------------------------------------|-------------------|-----------------|
| CMAKE_BUILD_TYPE     | 'Debug', 'RelWithDebInfo', 'MinSizeRel', 'Release' | Compiler optimizer configuration                                  | Debug             | Debug           |
| CMAKE_INSTALL_PREFIX | Any path                                           | Install location                                                  | /usr/local        | %ProgramFiles%  |
| GLD_USE_HELICS       | ON/OFF                                             | Enables detection and use of HELICS                               | OFF               | OFF             |
| HELICS_DIR           | Any path                                           | Hint indicating HELICS install directory                          |                   |                 |
| GLD_USE_MYSQL        | ON/OFF                                             | Enables detection and use of MySQL                                | OFF               | OFF             |
| MYSQL_DIR            | Any path                                           | Hint indicating MySQL install directory                           |                   |                 |

### Enable building with HELICS

To enable HELICS set the `GLD_USE_HELICS` flag to `ON`
if HELICS is in a custom path set `HELICS_DIR` to the install location in CMake or as an environmental variable

### Enable building with MySQL

To enable MySQL support set the `GLD_USE_MYSQL` flag to `ON`
if MySQL is in a custom path set `MYSQL_DIR` to the install location in CMake or as an environmental variable

### Enable build debugging

To output all build commands during build, set following flag to `ON`

```
CMAKE_VERBOSE_MAKEFILE=OFF 
```
