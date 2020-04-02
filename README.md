## Building GridLAB-D
Instructions for building GridLAB-D can be found on our wiki under [Builds](http://gridlab-d.shoutwiki.com/wiki/Builds).
# Building Gridlab-d
## Prerequisites

CMake  
CCMake or CMake-gui (optional)   
g++ or Clang

## Installation

```shell script
git clone https://github.com/gridlab-d/gridlab-d.git
```

### Prepare out-of-source build directory
Change directory into folder, and create build directory. Change to build directory and invoke `cmake` as described below:
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

Sets prefix for GridLAB-D install. It is strongly recommended to change this variable.
```
CMAKE_INSTALL_PREFIX=/usr/local  # linux/MacOS default
CMAKE_INSTALL_PREFIX=%ProgramFiles%  # Windows default
```

Sets build type. Options are 'Debug', 'RelWithDebInfo', 'MinSizeRel', and 'Release'. Options are case sensitive.  
Empty defaults to 'Debug'
```
CMAKE_BUILD_TYPE
```

### Enable building with FNCS
To enable FNCS set the following flag to `ON`
```
GLD_USE_FNCS=OFF
```
To enable FNCS, several libraries are required. The following variables can be set in CMake or as environmental 
variables pointed to the install location of these libraries. 

Libraries on the global include path may be detected automatically:  
```
GLD_ZeroMQ_DIR
GLD_CZMQ_DIR
GLD_FNCS_DIR
```

### Enable building with HELICS
To enable HELICS set the following flag to `ON`
 if HELICS is in a custom path set `HELICS_DIR` to the install location in CMake or as an environmental variable
```
GLD_USE_HELICS=OFF
```

### Enable building with MySQL
To enable MySQL support set the following flag to `ON`
```
GLD_USE_MYSQL=OFF
```
These fields will be automatically populated if MySQL is detected. 
They can be manually specified if MySQL is installed in a non-standard location.
```
MYSQL_INCLUDE_DIRECTORIES
MYSQL_LIBRARY
MYSQL_EXTRA_LIBRARIES
```

### Enable build debugging
To output all build commands during build, set following flag to `ON`
```
CMAKE_VERBOSE_MAKEFILE=OFF 
```
