# Building Gridlab-d

Gridlab-d is a ...  

## Prerequisites

CMake \
CCMake or CMake-gui \
gcc or Clang

## Installation

```bash
git clone *Insert clone link here*
```

Change directory into folder, and create build directory. Change to build directory and invoke `cmake` as described below:

```bash 
cd gridlab-d
mkdir cmake-build
cd cmake-build
cmake ..
```

After running cmake, run either `ccmake .` or `cmake-gui .` to execute the graphical interface. 

Adjust settings as needed, then run `configure` and `generate` either from the interface, or by clicking `c` then `g` on the keyboard.


Run `make install` from the command line. Multithreaded builds are supported via the `-j#` flag.

```bash
make -j8 install
```

## CMake Variables

The following variables affect the build process (default values are shown):
```
// Sets prefix for gridlab-d install. It is strongly recommended to change this variable.
CMAKE_INSTALL_PREFIX=/usr/local

// Sets build type. Options are 'Debug', 'RelWithDebInfo', 'MinSizeRel', and 'Release'. Options are case sensitive. 
Empty defaults to 'Debug'
CMAKE_BUILD_TYPE

// Enable building with FNCS
USE_FNCS=OFF
    // To enable FNCS, the following path variables must be set to the install paths of their respective applications:
    GL_ZeroMQ_DIR
    GL_CZMQ_DIR
    GL_FNCS_DIR
    
// Enable building with HELICS
USE_HELIC=OFF
    // To enable HELICS, the following path variables must be set to the install paths of their respective applications:
    GL_ZeroMQ_DIR
    GL_HELICS_DIR
    // If a non-system standard version of Boost is desired, the following variable must also be set:
    GL_Boost_DIR

// Enable building with MySQL
USE_MYSQL=OFF
    // These fields will be automatically populated if MySQL is detected. They can be manually specified if MySQL is installed in a non-standard location.
    MYSQL_INCLUDE_DIRECTORIES
    MYSQL_LIBRARY
    MYSQL_EXTRA_LIBRARIES
    
// Enable building with OpenMP (No current use support)
USE_OPENMP=OFF

// Setting true will output more information during build process.
CMAKE_VERBOSE_MAKEFILE=FALSE 
```
