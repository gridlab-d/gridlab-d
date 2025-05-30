cmake_minimum_required(VERSION 3.13)
project(GridLAB-D)

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

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
include(FetchContent)

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

find_package(Curses)
if (CURSES_FOUND)
    option(HAVE_CURSES "Curses library is present on the platform" ON)
endif ()

# Link required libraries for symbol checks
list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)

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

# Forces certain warnings to trigger errors.
if (NOT MSVC)
    set(FORCE_ERROR "-Werror=return-type" "-Werror=write-strings")
endif ()

# Compiler/Linker flags for all build types
add_compile_options(${FORCE_ERROR} ${DISABLED_OPTIMIZATIONS})

## Set up platform specific library linking and platform identification
if (WIN32 OR MSYS OR MINGW OR CYGWIN)
    FetchContent_Declare(
            dlfcn-win32
            GIT_REPOSITORY https://github.com/dlfcn-win32/dlfcn-win32.git
            GIT_TAG v1.3.1
    )
    FetchContent_MakeAvailable(dlfcn-win32)
    include_directories(${dlfcn-win32_SOURCE_DIR}/src)
    set(DL_NAME dl)
    set(DL_LIB dl)

    set(OS_SPECIFIC_LIBRARIES ws2_32 wsock32 ${DL_NAME} m)
    set(FOUND_WINDOWS yes CACHE INTERNAL "Tell script generator we're Windows")
    add_compile_options(-static-libgcc -static-libstdc++ -static)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++ -static")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
elseif (APPLE)
    set(OS_SPECIFIC_LIBRARIES ${CMAKE_DL_LIBS})
    set(FOUND_MACOS yes CACHE INTERNAL "Tell script generator we're MacOS")
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

    if (WIN32 OR MSYS OR MINGW OR CYGWIN)
        # The CBLAS version provided by SuperLU does not build in MSYS2.
        option(enable_internal_blaslib "Build the CBLAS library" OFF)
    else ()
        option(enable_internal_blaslib "Build the CBLAS library" ON)
    endif ()

    # SuperLU's generic names here can cause issues, make sure other projects do not cross these names
    option(enable_examples "Build SuperLU examples" OFF)
    option(enable_tests "Build SuperLU tests" OFF)

    include_directories(third_party/SuperLU/SRC)
    add_subdirectory(third_party/SuperLU)
    set(SUPERLU_LIB superlu)
endif ()

option(JSONCPP_WITH_TESTS "Build the JSON CPP test suite" OFF)
add_subdirectory(third_party/jsoncpp_lib)
get_target_property(JSON_INC_PATH jsoncpp_static INTERFACE_INCLUDE_DIRECTORIES)
include_directories(${JSON_INC_PATH})
set(JSONCPP_LIB jsoncpp_static)

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
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/script/${GLD_PACKAGE}.sh
        ${FILE_PERMISSIONS}
        DESTINATION bin
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
install(FILES ${HEADER_FILE_NAMES}
        ${FILE_PERMISSIONS}
        DESTINATION include
        COMPONENT Runtime
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

include(cmake/CPackConfig.cmake)