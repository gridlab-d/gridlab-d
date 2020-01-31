cmake_minimum_required(VERSION 3.6)
project(GridLAB-D)

set(COMPILE_FLAGS "${WARNING_FLAGS}")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INSTALL_BINDIR "${CMAKE_INSTALL_PREFIX}/bin" CACHE PATH "" FORCE)
set(CMAKE_INSTALL_LIBDIR "${CMAKE_INSTALL_PREFIX}/lib/gridlabd" CACHE PATH "" FORCE)

#SET(CMAKE_VERBOSE_MAKEFILE ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# CMake Module includes
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CMakeDependentOption)

# Setup Gridlab-d Version information.
include(cmake/SetVersion.cmake)
# Perform dependency checks
include(cmake/CheckDependancies.cmake)
# Generate current build number
include(cmake/Build_number.cmake)

option(GLD_USE_EIGEN "Use Eigen package for LU Decomposition in Powerflow" OFF)
option(GLD_USE_HELICS "Link with HELICS" OFF)
option(GLD_USE_FNCS "Link with FNCS" OFF)
option(GLD_USE_MYSQL "Link with MySQL" OFF)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)
if (CMAKE_USE_PTHREADS_INIT)
    option(HAVE_PTHREADS "pthreads library is present on the platform" ON)
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
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib/gridlabd")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
option(BUILD_SHARED_LIBS "Sets Gridlab-d libraries to be built as shared. *DANGER - will currently break build if changed.*" ON)

# Forces certain warnings to trigger errors.
set(FORCE_ERROR "-Werror=return-type")

# Compiler/Linker flags for all build types
add_compile_options(${FORCE_ERROR} ${DISABLED_OPTIMIZATIONS})

## Set up platform specific library linking
#if (WIN32 OR MSYS OR MINGW OR CYGWIN)
#    set(OS_SPECIFIC_LIBRARIES ws2_32 wsock32 dl m)
#elseif (LINUX)
#    set(OS_SPECIFIC_LIBRARIES "")
#elseif (APPLE)
#    set(OS_SPECIFIC_LIBRARIES "")
#else ()
#    set(OS_SPECIFIC_LIBRARIES "")
#endif ()
#LIST(APPEND CMAKE_REQUIRED_LIBRARIES ${OS_SPECIFIC_LIBRARIES})
#if (CMAKE_REQUIRED_LIBRARIES)
#    LIST(REMOVE_DUPLICATES CMAKE_REQUIRED_LIBRARIES)
#endif ()

find_program(
        GLD_CLANG_TIDY_EXE
        NAMES
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
    set(CMAKE_CXX_CLANG_TIDY "${GLD_CLANG_TIDY_EXE}" # "-fix"
            "-checks=-*,-modernize-avoid-c-arrays,modernize-*"
            )
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

# Load module directories. When adding new module, add it here.
include_directories(gldcore)

include_directories(third_party/superLU_MT)
include_directories(third_party/jsoncpp/include)
include_directories(third_party/eigen)
include_directories(third_party/dlfcn-win32)

add_subdirectory(third_party/superLU_MT)

option(JSON_CPP_WITH_TESTS "Build the JSON CPP test suite" OFF)
add_subdirectory(third_party/jsoncpp)
find_package (Eigen3 3.3 REQUIRED NO_MODULE)

if (WIN32 OR MSYS OR MINGW OR CYGWIN)
    find_package(dlfcn)
    message(STATUS "dlfcn_FOUND: ${dlfcn_FOUND}")
endif()

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

mark_as_advanced(FORCE
        OS_SPECIFIC_LIBRARIES
        DISABLED_OPTIMIZATIONS
        BUILD_SHARED_LIBS
        GLD_MODULES
        HEADER_FILE_NAMES
        GLD_SHARE
        )

set(FILE_PERMISSIONS PERMISSIONS
        OWNER_EXECUTE OWNER_READ OWNER_WRITE
        GROUP_EXECUTE GROUP_READ
        WORLD_EXECUTE WORLD_READ)

install(TARGETS gridlabd
        ${FILE_PERMISSIONS}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
install(TARGETS ${GL_MODULES}
        ${FILE_PERMISSIONS}
        RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT library)

install(FILES ${HEADER_FILE_NAMES}
        ${FILE_PERMISSIONS}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/gridlabd
        )

install(FILES ${GLD_SHARE}
        ${FILE_PERMISSIONS}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/gridlabd)
install(FILES COPYRIGHT LICENSE
        ${FILE_PERMISSIONS}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/gridlabd)
