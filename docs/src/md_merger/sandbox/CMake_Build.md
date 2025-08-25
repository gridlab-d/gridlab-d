# CMake Build

**Source URL:** https://GridLAB-D™.shoutwiki.com/wiki/CMake_Build

---
 
 



[Ostrander (Version 5.0)] 

## Contents

  * 1 Building GridLAB-D
    * 1.1 Prerequisites
      * 1.1.1 Packages
    * 1.2 Installation
      * 1.2.1 Git
      * 1.2.2 Prepare out-of-source build directory
      * 1.2.3 Generate the build system
      * 1.2.4 Build and install the application
    * 1.3 CMake Variables
      * 1.3.1 Enable building with HELICS
      * 1.3.2 Enable building with MySQL
      * 1.3.3 Enable build debugging
      * 1.3.4 Developer Build Flags
    * 1.4 Building on ARM-Based macOS (M1, M2, M3....)
      * 1.4.1 Switch the architecture of the Terminal shell
      * 1.4.2 Install HomeBrew
      * 1.4.3 Install build dependencies
      * 1.4.4 (HELICS-only) Find the location of the HELICS library file libhelics.dylib
      * 1.4.5 Clone in the GridLAB-D™ repo and set up the build directory (as in above instructions)
      * 1.4.6 Configure the build
      * 1.4.7 Build and install (as in above instructions)
# Building GridLAB-D

## Prerequisites

These instructions should be executed in your terminal of choice. This may be MSYS2, a WSL instance, or the built-in terminal for your OS of choice (note: Windows CMD does not work). 

If you are on Windows and are using MSYS2, see [this page for setting up MSYS2] to run the commands below. 

If you have an Apple-ARM-based Mac, be sure to skip to the [ARM-Based Section] for those instructions. 

### Packages
    
    
    CMake  
    CCMake or CMake-gui (optional)   
    g++ or Clang
    

## Installation

### Git

Clone the git repository for GridLAB-D™ and update submodules: 
    
    
    git clone <https://github.com/gridlab-d/GridLAB-D™.git>
    cd gridlab-d
    git submodule update --init
    

### Prepare out-of-source build directory

Create build directory and move into it: 
    
    
    mkdir cmake-build
    cd cmake-build
    

### Generate the build system

CMake flags can be added using the `-D` prefix, and different build systems can be selected using `-G`. 

Below is a general format guide, and an actual viable build command for most platforms. 
    
    
    # Format:
    cmake <flags> ..
    
    
    
    # Full Example: 
    cmake -DCMAKE_INSTALL_PREFIX=~/software/GridLAB-D™ -DCMAKE_BUILD_TYPE=Release -G "CodeBlocks - Unix Makefiles" ..
    

### Build and install the application

CMake can directly invoke the build and install process by running the below command. Multiprocess build is enabled through the `-j#` flag (`-j8` in the included example). 
    
    
    # Run the build system and install the application
    cmake --build . -j8 --target install
    

## CMake Variables

The following variables affect the build process and can be changed using the `-D` flag at build generation or by updating the cache using ccmake or cmake-gui (default values are shown). 

Variable | Valid Values | Description | Linux/Mac Default | Windows Default   
---|---|---|---|---  
Example | Example | Example | Example | Example   
CMAKE_BUILD_TYPE | 'Debug', 'RelWithDebInfo', 'MinSizeRel', 'Release' | Compiler optimizer configuration | Debug | Debug   
CMAKE_INSTALL_PREFIX | Any path | Install location | /usr/local | %ProgramFiles%   
GLD_USE_HELICS | ON/OFF | Enables detection and use of HELICS | OFF | OFF   
HELICS_DIR | Any path | Hint indicating HELICS install directory |  |   
GLD_USE_MYSQL | ON/OFF | Enables detection and use of MySQL | OFF | OFF   
MYSQL_DIR | Any path | Hint indicating MySQL install directory |  |   
  
### Enable building with HELICS

To enable HELICS set the `GLD_USE_HELICS` flag to `ON` if HELICS is in a custom path set `HELICS_DIR` to the install location in CMake or as an environmental variable 
    
    
    GLD_USE_HELICS=OFF
    

### Enable building with MySQL

To enable MySQL support set the `GLD_USE_MYSQL` flag to `ON` if MySQL is in a custom path set `MYSQL_DIR` to the install location in CMake or as an environmental variable 

### Enable build debugging

To output all build commands during build, set following flag to `ON` 
    
    
    CMAKE_VERBOSE_MAKEFILE=OFF
    

### Developer Build Flags

GridLAB-D™ Developers may want to enable additional C++ code style and bugprone checks, which have been made available through the use of the clang-tidy tool at compile time. To enable these build-time checks, a flag to enable these checks is provided 
    
    
    GLD_USE_CLANG_TIDY=ON
    

WARNING: Enabling clang-tidy checks will significantly increase the number of build warnings for portions of the code base which have not yet been updated 

## Building on ARM-Based macOS (M1, M2, M3....)

As of this writing (Sept 2024), GridLAB-D™ functions best when built as a x86-64 (Intel-chip) binary where macOS can use [Rosetta 2](https://support.apple.com/en-us/102527) to translate the machine code to run on an ARM-based processor. 

### Switch the architecture of the Terminal shell
    
    
    gld_user2@mymac ~ % arch -x86_64 zsh
    

This launches a new zsh shell as an x86-64 process so the compilation/build run in this shell will produce x86-64 binary. 

### Install HomeBrew

[Homebrew](https://brew.sh/) is a package manager for macOS and is the easiest way to install many tools from the Linux world. As of this writing (Sept 2024), you can install Homebrew with the following command: 
    
    
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    

### Install build dependencies
    
    
    brew install cmake # installs cmake and ccmake
    

Optionally, if you plan on including HELICS compatibility in GridLAB-D™, install the ZeroMQ library 
    
    
    brew install zeromq
    

### (HELICS-only) Find the location of the HELICS library file libhelics.dylib

Depending on how you installed HELICS (building from source, installing pre-built binaries, using `pip`) the location of the libhelics.dylib on your system will vary. If you did install via `pip` you can use 
    
    
    pip show helics
    

as a starting point for your search. Other good places to look include `/usr/lib`, `/usr/local/lib`. 

Once you find libhelics.dylib, copy the path to the folder containing it as it will be needed in configuring the build. 

### Clone in the GridLAB-D™ repo and set up the build directory (as in above instructions)

No special macOS instructions, here. The next step comes when configuring the build by running 
    
    
    ccmake .
    

### Configure the build

The following environment variables are the most likely ones you may want to change: 
    
    
    CMAKE_INSTALL_PREFIX = <<Location in which to install. If in a virtual or conda environment you'll want to set this to the path used in that environment.>>
    
    
    
    GLD_HELICS_DIR = <<Directory where libhelics.dylib resides.>>
    
    
    
    GLD_USE_HELICS = ON
    
    
    
    GLD_HELICS_DIR = <<Directory where libhelics.dylib resides.>>
    

Once all of these have been set, press `c` to configure and `g` to generate the configuration file. Due to a bug in ccmake it may take multiple attempts with `c` to get the `g` option to appear 

### Build and install (as in above instructions)


