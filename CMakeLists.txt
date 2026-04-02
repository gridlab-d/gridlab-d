cmake_minimum_required(VERSION 3.15)


set(CMAKE_CXX_VISIBILITY_PRESET default)
set(CMAKE_VISIBILITY_INLINES_HIDDEN OFF)



# --- Global MSVC settings to eliminate C1041 ---
if (MSVC)
  # Use embedded debug info in Debug and RelWithDebInfo (/Z7)
  add_compile_options(
    $<$<CONFIG:Debug>:/Z7>
    $<$<CONFIG:RelWithDebInfo>:/Z7>
  )

  # Force synchronous PDB writes as extra safety (/FS)
  add_compile_options(
    $<$<CONFIG:Debug>:/FS>
    $<$<CONFIG:RelWithDebInfo>:/FS>
  )
endif ()


if(APPLE)
  set(CMAKE_MACOSX_RPATH ON)
endif()


# Set the architecture BEFORE project() on Windows
if(MSVC OR CMAKE_GENERATOR MATCHES "Visual Studio")
    # Ensure we have a valid platform
    if(NOT CMAKE_GENERATOR_PLATFORM)
        set(CMAKE_GENERATOR_PLATFORM "x64" CACHE STRING "Generator platform" FORCE)
    endif()
endif()


if(MSVC)
  # Let targets select the MSVC runtime via property (CMP0091 must be NEW)
  cmake_policy(SET CMP0091 NEW)
  # Default: dynamic runtime (/MD for Release, /MDd for Debug)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
  # If you prefer static runtime, uncomment the next line:
  # set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

  # Ensure architecture macros are defined
    # This shouldn't be necessary normally, but helps with edge cases
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        add_compile_definitions(_AMD64_ AMD64)
    else()
        add_compile_definitions(_X86_ _WIN32)
    endif()
    
endif()
project(GridLAB-D)




if(WIN32)
    # Avoid legacy includes and macro collisions from Windows headers
    add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)
    # Ensure _WIN32 is defined (should be automatic, but be explicit)
    if(MSVC)
        add_compile_definitions(_WINDOWS)
    endif()
endif()

set(CMAKE_CXX_STANDARD 23) # Or 14, 11 if 17 is not supported
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(EIGEN_HAS_STD_RESULT_OF=0)


if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()


# Include FetchContent module
include(FetchContent)

# Try to find system Eigen3 first (version 3.3 or higher)
find_package(Eigen3 3.3 QUIET)

if(NOT Eigen3_FOUND)
    # Fetch Eigen3 from release tarball (faster than git clone)
    FetchContent_Declare(
      Eigen3
      URL https://gitlab.com/libeigen/eigen/-/archive/3.3.9/eigen-3.3.9.tar.gz
      URL_HASH SHA256=7985975b787340124786f092b3a07d594b2e9cd53bbfe5f3d9b1daee7d55f56f
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    
    # Use Populate instead of MakeAvailable to include headers only (avoids BLAS/LAPACK conflicts)
    FetchContent_GetProperties(Eigen3)
    if(NOT eigen3_POPULATED)
      FetchContent_Populate(Eigen3)
    endif()
    
    include_directories(${eigen3_SOURCE_DIR})
else()
    message(STATUS "Using system Eigen3 (version ${Eigen3_VERSION})")
    include_directories(${EIGEN3_INCLUDE_DIR})
endif()



include(cmake/Warnings.cmake)
OPTION(GLD_EXTRA_WARNINGS "Make warnings more pedantic" OFF)
if (GLD_EXTRA_WARNINGS)
    set(GLD_COMPILE_OPTIONS ${WARNING_FLAGS} CACHE INTERNAL "GLD_COMPILE_OPTIONS")
else ()
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # TODO: resolve these warnings and un-supress them
        SET(GLD_COMPILE_OPTIONS
                "-Wno-format-overflow"
                "-Wno-format-truncation"
                "-Wno-stringop-overflow"
                CACHE INTERNAL
                "GLD_COMPILE_OPTIONS"
        )
    endif ()
endif ()


set(GLD_PACKAGE gridlabd)
set(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION OFF) # This shouldn't be needed, but seems to fix issues

option(CMAKE_VERBOSE_MAKEFILE "CMake outputs all build instructions" OFF)
option(GLD_DO_CLEANUP "Automatically removes files from old GridLAB-D build processes" OFF)
option(GLD_CI_BUILD "Build is being performed by a CI platform" OFF)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# CMake Module includes
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CMakeDependentOption)

# Clean up remnant files from the old autotools build system
include(cmake/Cleanup.cmake)
# Setup Gridlab-d Version information.
include(cmake/SetVersion.cmake)
# Perform dependency checks
include(cmake/CheckDependencies.cmake)
# Generate current build number
include(cmake/BuildNumber.cmake)
include(cmake/SanitizePaths.cmake)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib/static)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

option(GLD_USE_EIGEN "[NOT YET SUPPORTED] - Use Eigen package for LU Decomposition in Powerflow" OFF)
option(GLD_USE_SUPERLU "Use SuperLU package for LU Decomposition in Powerflow" ON)
option(GLD_USE_HELICS "Link with HELICS" OFF)
option(GLD_USE_FNCS "Link with FNCS" OFF)
option(GLD_USE_HDF5 "Link with HDF5" OFF)
option(GLD_USE_MYSQL "Link with MySQL" OFF)
set(GLD_HELICS_DIR "" CACHE PATH "Search path for HELICS library")
set(GLD_MYSQL_DIR "" CACHE PATH "Search path for MySQL library")
set(GLD_FNCS_DIR "" CACHE PATH "Search path for FNCS library")
set(GLD_ZeroMQ_DIR "" CACHE PATH "Search path for ZeroMQ library (FNCS dependency)")
set(GLD_CZMQ_DIR "" CACHE PATH "Search path for CZMQ library (FNCS dependency)")

set(CMAKE_POSITION_INDEPENDENT_CODE ON) # TODO: only required for SuperLU_MT support with Clang. Future removal recommended.
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)
if (CMAKE_USE_PTHREADS_INIT)
    option(HAVE_PTHREADS "pthreads library is present on the platform" ON)
endif ()

find_package(XercesC 3.2.0)
if (NOT XercesC_FOUND)
    message(STATUS "XercesC library could not be located on the platform. XML Loading is disabled.")
endif ()

if(UNIX)
    find_package(Curses)
    if (CURSES_FOUND)
        option(HAVE_CURSES "Curses library is present on the platform" ON)
    endif ()
endif()

# Link required libraries for symbol checks

if (NOT MSVC)
list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
endif ()

configure_file(gldcore/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/headers/config.h @ONLY)
configure_file(gldcore/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/headers/version.h @ONLY)
add_definitions(-DHAVE_CONFIG_H)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/headers)
#include(cmake/ExternalLibraries.cmake)

# use, i.e. don't skip the full RPATH for the build tree
set(CMAKE_SKIP_BUILD_RPATH FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib/${GLD_PACKAGE}")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
option(BUILD_SHARED_LIBS "Sets Gridlab-d libraries to be built as shared. *DANGER - will currently break build if changed.*" ON)



# --- CMakeLists.txt patch ---
function(set_portable_rpath tgt)
  # Determine the target type: EXECUTABLE, MODULE_LIBRARY, SHARED_LIBRARY, STATIC_LIBRARY, etc.
  get_target_property(_type ${tgt} TYPE)

  # Always point build-time rpath at the directory holding gldapi (so build tree runs work)
  set_target_properties(${tgt} PROPERTIES
    BUILD_RPATH "$<TARGET_FILE_DIR:gldapi>"
  )

  if(APPLE)
    if(_type STREQUAL "EXECUTABLE")
      # Executable in bin must find libs in ../lib (and ../lib/gridlabd for modules under that subtree)
      set_target_properties(${tgt} PROPERTIES
        INSTALL_RPATH "@loader_path/../lib;@loader_path/../lib/gridlabd"
      )
    else() # modules/libraries installed into lib
      set_target_properties(${tgt} PROPERTIES
        INSTALL_RPATH "@loader_path"
      )
    endif()
  elseif(UNIX) # Linux and other ELF platforms (excluding Windows)
    if(_type STREQUAL "EXECUTABLE")
      set_target_properties(${tgt} PROPERTIES
        INSTALL_RPATH "$ORIGIN/../lib;$ORIGIN/../lib/gridlabd"
      )
    else() # modules/libraries in lib
      set_target_properties(${tgt} PROPERTIES
        INSTALL_RPATH "$ORIGIN"
      )
    endif()
  endif()
endfunction()

# Forces certain warnings to trigger errors.
if (NOT MSVC)
    set(FORCE_ERROR "-Werror=return-type" "-Werror=write-strings")
endif ()

if (NOT MSVC)
    # Compiler/Linker flags for all build types
    add_compile_options(${FORCE_ERROR} ${DISABLED_OPTIMIZATIONS})
endif ()

## Set up platform specific library linking and platform identification
if (WIN32 OR MSYS OR MINGW OR CYGWIN)
    FetchContent_Declare(
             dlfcn-win32
             GIT_REPOSITORY https://github.com/dlfcn-win32/dlfcn-win32.git
             GIT_TAG v1.4.2
             DOWNLOAD_EXTRACT_TIMESTAMP true
     )
     FetchContent_MakeAvailable(dlfcn-win32)
     include_directories(${dlfcn-win32_SOURCE_DIR}/src)
     set(DL_NAME dl)
     set(DL_LIB dl)

   
    # Winsock 2: only Ws2_32.lib is needed.
    if(WIN32)
      set(OS_SPECIFIC_LIBRARIES ws2_32 ${DL_NAME})
    else()
      set(OS_SPECIFIC_LIBRARIES ws2_32 ${DL_NAME} m)
    endif()


    set(FOUND_WINDOWS yes CACHE INTERNAL "Tell script generator we're Windows")

    if(NOT MSVC)
        # Enable static linking for GCC on Windows
        add_compile_options(-static-libgcc -static-libstdc++)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
        add_compile_options(-static-libgcc -static-libstdc++ -static)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++ -static")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
    endif()

elseif (APPLE)
    set(FOUND_MACOS yes CACHE INTERNAL "Tell script generator we're MacOS")
    # macOS: use the platform dl libs provided by CMake
    set(OS_SPECIFIC_LIBRARIES ${CMAKE_DL_LIBS})

else ()
    set(OS_SPECIFIC_LIBRARIES ${CMAKE_DL_LIBS})
endif ()
LIST(APPEND CMAKE_REQUIRED_LIBRARIES ${OS_SPECIFIC_LIBRARIES})
if (CMAKE_REQUIRED_LIBRARIES)
    LIST(REMOVE_DUPLICATES CMAKE_REQUIRED_LIBRARIES)
endif ()

# Generate runtime wrapper script
set(GENFILE_WARNING
        "WARNING: This is a CMake generated file, modifications to this file will be overwritten."
        CACHE INTERNAL
        "Warn user that gridlabd.sh is a generated file."
)

find_program(
        GLD_CLANG_TIDY_EXE
        NAMES
        "clang-tidy"
        "clang-tidy-9"
        "clang-tidy-10"
        "clang-tidy-11"
)

cmake_dependent_option(GLD_USE_CLANG_TIDY "Run Clang-tidy" OFF "GLD_CLANG_TIDY_EXE" OFF)

if (NOT GLD_CLANG_TIDY_EXE)
    message(STATUS "clang-tidy not found.")
elseif (NOT GLD_USE_CLANG_TIDY)
    message(STATUS "clang-tidy disabled.")
else ()
    message(STATUS "clang-tidy found: ${GLD_CLANG_TIDY_EXE}")
    set(CMAKE_CXX_CLANG_TIDY ${GLD_CLANG_TIDY_EXE};-checks=-*,bugprone-*,modernize-*,-modernize-use-trailing-return-type,-modernize-avoid-c-arrays;-warnings-as-errors=-*;)
endif ()

# Set the list of modules and files which should be copied into the install directory.
set(GLD_MODULES
        assert
        climate
        commercial
        connection
        generators
        market
        mysql
        optimize
        powerflow
        reliability
        residential
        tape
        tape_file
        tape_plot
        glsolvers
        glxengine
)

set(HEADER_FILE_NAMES
        ${CMAKE_SOURCE_DIR}/gldcore/class.h
        ${CMAKE_SOURCE_DIR}/gldcore/gld_complex.h
        ${CMAKE_SOURCE_DIR}/gldcore/debug.h
        ${CMAKE_SOURCE_DIR}/gldcore/enduse.h
        ${CMAKE_SOURCE_DIR}/gldcore/exception.h
        ${CMAKE_SOURCE_DIR}/gldcore/loadshape.h
        ${CMAKE_SOURCE_DIR}/gldcore/lock.h
        ${CMAKE_SOURCE_DIR}/gldcore/module.h
        ${CMAKE_SOURCE_DIR}/gldcore/object.h
        ${CMAKE_SOURCE_DIR}/gldcore/property.h
        ${CMAKE_SOURCE_DIR}/gldcore/schedule.h
        ${CMAKE_SOURCE_DIR}/gldcore/test.h
        ${CMAKE_SOURCE_DIR}/gldcore/gldapi.h
)

file(COPY ${HEADER_FILE_NAMES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/headers)
list(APPEND HEADER_FILE_NAMES
        ${CMAKE_CURRENT_BINARY_DIR}/headers/build.h
        ${CMAKE_CURRENT_BINARY_DIR}/headers/version.h
)

set(GLD_SHARE
        ${CMAKE_SOURCE_DIR}/gldcore/tzinfo.txt
        ${CMAKE_SOURCE_DIR}/gldcore/unitfile.txt
        ${CMAKE_SOURCE_DIR}/gldcore/rt/about.htm
        ${CMAKE_SOURCE_DIR}/gldcore/rt/COPYRIGHT
        ${CMAKE_SOURCE_DIR}/gldcore/rt/debugger.conf
        ${CMAKE_SOURCE_DIR}/gldcore/rt/eula.htm
        ${CMAKE_SOURCE_DIR}/gldcore/rt/favicon.ico
        ${CMAKE_SOURCE_DIR}/gldcore/rt/gnuplot.conf
        ${CMAKE_SOURCE_DIR}/gldcore/rt/gridlabd.conf
        ${CMAKE_SOURCE_DIR}/gldcore/rt/gridlabd.css
        ${CMAKE_SOURCE_DIR}/gldcore/rt/gridlabd.h
        ${CMAKE_SOURCE_DIR}/gldcore/rt/gridlabd.htm
        ${CMAKE_SOURCE_DIR}/gldcore/rt/gridlabd.jpg
        ${CMAKE_SOURCE_DIR}/gldcore/rt/gridlabd.js
        ${CMAKE_SOURCE_DIR}/gldcore/rt/gridlabd.syn
        ${CMAKE_SOURCE_DIR}/gldcore/rt/LICENSE
        ${CMAKE_SOURCE_DIR}/gldcore/rt/mingw.conf
        ${CMAKE_SOURCE_DIR}/gldcore/rt/STATUS
        ${CMAKE_SOURCE_DIR}/gldcore/rt/capacitor_b.png
        ${CMAKE_SOURCE_DIR}/gldcore/rt/capacitor_g.png
        ${CMAKE_SOURCE_DIR}/gldcore/rt/capacitor_k.png
        ${CMAKE_SOURCE_DIR}/gldcore/rt/capacitor_r.png
        ${CMAKE_SOURCE_DIR}/gldcore/rt/load_b.png
        ${CMAKE_SOURCE_DIR}/gldcore/rt/load_g.png
        ${CMAKE_SOURCE_DIR}/gldcore/rt/load_k.png
        ${CMAKE_SOURCE_DIR}/gldcore/rt/load_r.png
        ${CMAKE_SOURCE_DIR}/gldcore/rt/meter_g.png
        ${CMAKE_SOURCE_DIR}/gldcore/rt/node_b.png
        ${CMAKE_SOURCE_DIR}/gldcore/rt/node_g.png
        ${CMAKE_SOURCE_DIR}/gldcore/rt/node_k.png
        ${CMAKE_SOURCE_DIR}/gldcore/rt/node_r.png
        ${CMAKE_SOURCE_DIR}/gldcore/rt/regulator_b.png
        ${CMAKE_SOURCE_DIR}/gldcore/rt/regulator_g.png
        ${CMAKE_SOURCE_DIR}/gldcore/rt/regulator_k.png
        ${CMAKE_SOURCE_DIR}/gldcore/rt/regulator_r.png
        ${CMAKE_SOURCE_DIR}/gldcore/rt/switch_b.png
        ${CMAKE_SOURCE_DIR}/gldcore/rt/switch_g.png
        ${CMAKE_SOURCE_DIR}/gldcore/rt/switch_k.png
        ${CMAKE_SOURCE_DIR}/gldcore/rt/switch_r.png
        ${CMAKE_SOURCE_DIR}/gldcore/rt/transformer_b.png
        ${CMAKE_SOURCE_DIR}/gldcore/rt/transformer_g.png
        ${CMAKE_SOURCE_DIR}/gldcore/rt/transformer_k.png
        ${CMAKE_SOURCE_DIR}/gldcore/rt/transformer_r.png
        ${CMAKE_SOURCE_DIR}/gldcore/rt/triplex_meter_b.png
        ${CMAKE_SOURCE_DIR}/gldcore/rt/triplex_meter_g.png
        ${CMAKE_SOURCE_DIR}/gldcore/rt/triplex_meter_k.png
        ${CMAKE_SOURCE_DIR}/gldcore/rt/triplex_meter_r.png
        ${CMAKE_SOURCE_DIR}/models/climate_csvreader_example.glm
        ${CMAKE_SOURCE_DIR}/models/collector_example.glm
        ${CMAKE_SOURCE_DIR}/models/diesel_deltamode_load_player_A.csv
        ${CMAKE_SOURCE_DIR}/models/diesel_deltamode_load_player_B.csv
        ${CMAKE_SOURCE_DIR}/models/diesel_deltamode_load_player_C.csv
        ${CMAKE_SOURCE_DIR}/models/house_HVAC_example.glm
        ${CMAKE_SOURCE_DIR}/models/IEEE_13_Node_Test_Feeder.glm
        ${CMAKE_SOURCE_DIR}/models/IEEE_13_Node_With_Houses.glm
        ${CMAKE_SOURCE_DIR}/models/passive_controller_example.glm
        ${CMAKE_SOURCE_DIR}/models/random_fault_generator_example.glm
        ${CMAKE_SOURCE_DIR}/models/residential_zipload_example.glm
        ${CMAKE_SOURCE_DIR}/models/Standard_Weather.csv
        ${CMAKE_SOURCE_DIR}/models/subsecond_diesel_generator_example.glm
        ${CMAKE_SOURCE_DIR}/models/taxonomy_feeder_R1-12.47-1.glm
        ${CMAKE_SOURCE_DIR}/models/transactive_controller_example.glm
        ${CMAKE_SOURCE_DIR}/models/WA-Seattle.tmy2
        ${CMAKE_SOURCE_DIR}/models/WA-Yakima.tmy2
        ${CMAKE_SOURCE_DIR}/models/waterheater_example.glm
        ${CMAKE_SOURCE_DIR}/models/weather.csv
        ${CMAKE_SOURCE_DIR}/models/wind_turbine_example.glm
        #        ${CMAKE_SOURCE_DIR}/gldcore/gridlabd.htm
        ${CMAKE_SOURCE_DIR}/gldcore/gridlabd.htm
)

file(COPY ${GLD_SHARE} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/share)

set(BUILD_SHARED_CACHE ${BUILD_SHARED_LIBS} CACHE INTERNAL "Cache the shared library state")
set(BUILD_SHARED_LIBS OFF)
if (GLD_USE_SUPERLU)
    set(THIRD_PARTY_PATH ${CMAKE_SOURCE_DIR}/third_party)

    list(APPEND CMAKE_MODULE_PATH
            ${CMAKE_BINARY_DIR}/third_party/SuperLU/SRC
            ${CMAKE_BINARY_DIR}/third_party/SuperLU/CBLAS
    )

    # Use SuperLU's internal BLAS (faster build, no external dependencies)
    # The CBLAS version provided by SuperLU does not build in MSYS2
    if (WIN32 OR MSYS OR MINGW OR CYGWIN)
        set(enable_internal_blaslib OFF CACHE BOOL "Build the CBLAS library" FORCE)
    else()
        set(enable_internal_blaslib ON CACHE BOOL "Build the CBLAS library" FORCE)
    endif()

    # SuperLU's generic names here can cause issues, make sure other projects do not cross these names
    set(enable_examples OFF CACHE BOOL "Build SuperLU examples" FORCE)
    set(enable_tests OFF CACHE BOOL "Build SuperLU tests" FORCE)


    include_directories(third_party/SuperLU/SRC)

    #Choose 64-bit indices for SuperLU at configure time
    set(XSDK_INDEX_SIZE 32 CACHE STRING "SuperLU index size" FORCE)

    add_subdirectory(third_party/SuperLU)


    target_include_directories(superlu BEFORE PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/SuperLU/SRC
        ${CMAKE_BINARY_DIR}/third_party/SuperLU/SRC  # if superlu_config.h is generated here
    )

    set(SUPERLU_LIB superlu)
endif ()

find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json not found, downloading...")
    FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
    FetchContent_MakeAvailable(json)
endif()


if (GLD_USE_EIGEN)
    message(FATAL_ERROR "Eigen implementation is not complete, and should not be used at this time.")
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/third_party/Eigen/cmake) # Gives us FindKLU.cmake without replicating code
    include_directories(third_party/Eigen)
    find_package(Eigen3 3.3 REQUIRED NO_MODULE)
endif ()

set(BUILD_SHARED_LIBS ${BUILD_SHARED_CACHE})

# Configure module targets
add_subdirectory(assert)
add_subdirectory(climate)
add_subdirectory(commercial)
add_subdirectory(connection)
add_subdirectory(generators)
add_subdirectory(market)
add_subdirectory(mysql)
add_subdirectory(optimize)
add_subdirectory(powerflow)
add_subdirectory(reliability)
add_subdirectory(residential)
add_subdirectory(tape)
add_subdirectory(tape_file)
add_subdirectory(tape_plot)
add_subdirectory(${CMAKE_SOURCE_DIR}/gldcore/solvers)
add_subdirectory(${CMAKE_SOURCE_DIR}/gldcore/link/engine)
add_subdirectory(gldcore)

# Add test_gldapi executable
add_executable(test_gldapi gldcore/test_gldapi.cpp)
target_link_libraries(test_gldapi PRIVATE gldapi ${DL_LIB} ${JSONCPP_LIB} ${SUPERLU_LIB} Threads::Threads)
add_dependencies(test_gldapi gldapi)  



##
## Apply portable RPATH and ensure modules link to the shared core
## (SAFE: no effect on Windows RPATH; additional PRIVATE links to gldapi are additive)
##

# Core + host:
if(TARGET gldapi)
  set_portable_rpath(gldapi)
endif()
if(TARGET gridlabd)
  set_portable_rpath(gridlabd)
endif()

# Test host:
set_portable_rpath(test_gldapi)

# Modules: bind to shared core and apply portable RPATH (if targets exist)
foreach(mod ${GLD_MODULES})
  if(TARGET ${mod})
    # Ensure the module resolves symbols from the single shared core library
    target_link_libraries(${mod} PRIVATE gldapi)
    # Ensure the loader finds the shared core next to the module/exe on UNIX
    set_portable_rpath(${mod})
  endif()
endforeach()



set_source_files_properties(
  ${CMAKE_SOURCE_DIR}/third_party/SuperLU/SRC/slu_util.c
  PROPERTIES LANGUAGE C
)

if(MSVC)
    set_target_properties(gldapi PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )
endif()


if (WIN32)
  target_link_libraries(gridlabd PRIVATE Ws2_32)
endif()


# Link Curses only on Unix/Linux platforms
if(UNIX AND CURSES_FOUND)
    target_link_libraries(test_gldapi PRIVATE ${CURSES_LIBRARIES})
endif()

set_target_properties(test_gldapi PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin
)


get_target_property(GLD_COMPILE_OPTIONS gridlabd COMPILE_OPTIONS)
string(JOIN " " GLD_COMPILE_OPTIONS ${GLD_COMPILE_OPTIONS})
configure_file(cmake/gridlabd.sh.in ${CMAKE_CURRENT_BINARY_DIR}/script/${GLD_PACKAGE}.sh @ONLY)
if (WIN32 OR MSYS OR MINGW OR CYGWIN)
    configure_file(cmake/gridlabd.bat.in ${CMAKE_CURRENT_BINARY_DIR}/script/${GLD_PACKAGE}.bat @ONLY)
endif ()

set(GLD_BINARIES gridlabd)
set(GLD_LIBS
        assert
        climate
        commercial
        connection
	    gldapi
        generators
        market
        mysql
        optimize
        powerflow
        reliability
        residential
        tape
        tape_file
        tape_plot
        glsolvers
        glxengine
)
set(GLD_DEPS
        ${SUPERLU_LIB}
        ${JSONCPP_LIB}
        ${DL_LIB}
)

mark_as_advanced(FORCE
        OS_SPECIFIC_LIBRARIES
        DISABLED_OPTIMIZATIONS
        BUILD_SHARED_LIBS
        GLD_MODULES
        HEADER_FILE_NAMES
        GLD_SHARE
        GLD_CI_BUILD
)

set(FILE_PERMISSIONS PERMISSIONS
        OWNER_EXECUTE OWNER_READ OWNER_WRITE
        GROUP_EXECUTE GROUP_READ
        WORLD_EXECUTE WORLD_READ
)

include(GNUInstallDirs) 

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/script/${GLD_PACKAGE}.sh
        #${FILE_PERMISSIONS}
        DESTINATION ${CMAKE_INSTALL_BINDIR} #bin
        COMPONENT Runtime
)
if (WIN32 OR MSYS OR MINGW OR CYGWIN)
    install(FILES
            ${CMAKE_CURRENT_BINARY_DIR}/script/${GLD_PACKAGE}.bat
            ${FILE_PERMISSIONS}
            DESTINATION bin
            COMPONENT Runtime
    )
endif ()
#install(FILES ${HEADER_FILE_NAMES}
#        ${FILE_PERMISSIONS}
#        DESTINATION include
#        COMPONENT Runtime
#)


install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/headers/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        COMPONENT Runtime
		OPTIONAL
        FILES_MATCHING PATTERN "*.h"
        )
install(FILES ${GLD_SHARE}
        ${FILE_PERMISSIONS}
        DESTINATION share
        COMPONENT Runtime
)
install(FILES COPYRIGHT LICENSE
        ${FILE_PERMISSIONS}
        DESTINATION share/doc
        COMPONENT Runtime
)
install(TARGETS
        ${GLD_LIBS}
        ${GLD_BINARIES}
        ${GLD_DEPS}

        ${FILE_PERMISSIONS}
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
        #        BUNDLE DESTINATION bundle
        COMPONENT Runtime
        ARCHIVE DESTINATION lib/static
        COMPONENT Development
)


# Tell CPack not to re-build before packaging (the CI workflow already built everything)
set(CPACK_SET_DESTDIR ON)
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY ON)

include(cmake/CPackConfig.cmake)

