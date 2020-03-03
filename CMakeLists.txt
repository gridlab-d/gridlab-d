#cmake_minimum_required(VERSION 2.8.12)
cmake_minimum_required(VERSION 3.0)
project(gridlabd)

if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++14")
    endif ()
else ()
    SET(CMAKE_CXX_STANDARD 14)
    SET(CMAKE_CXX_STANDARD_REQUIRED ON)
endif ()

set(CMAKE_INSTALL_BINDIR "${CMAKE_INSTALL_PREFIX}/bin" CACHE PATH "" FORCE)
set(CMAKE_INSTALL_LIBDIR "${CMAKE_INSTALL_PREFIX}/lib/gridlabd" CACHE PATH "" FORCE)

#SET(CMAKE_VERBOSE_MAKEFILE ON)

# CMake Module includes
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckSymbolExists)

# Setup Gridlab-d Version information.
include(CMakeScripts/SetVersion.cmake)
INCLUDE(CMakeScripts/CheckDependancies.cmake)
INCLUDE(CMakeScripts/ExternalLibraries.cmake)
INCLUDE(CMakeScripts/Build_number.cmake)

configure_file(gldcore/cmake_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/headers/config.h @ONLY)
configure_file(gldcore/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/headers/version.h @ONLY)
add_definitions(-DHAVE_CONFIG_H)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/headers)
#include(CMakeScripts/ExternalLibraries.cmake)

# use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib/gridlabd")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
SET(GLDCORE_LOC gldcore)
OPTION(BUILD_SHARED_LIBS "Sets Gridlab-d libraries to be built as shared. *DANGER - will currently break build if changed.*" ON)

# Forces certain warnings to trigger errors.
SET(FORCE_ERROR "-Werror=return-type" "-Werror=write-strings")

# Allows specific compiler optimizations to be disabled for all build types.
set(DISABLED_OPTIMIZATIONS "") # "-fno-inline-functions")

# Compiler/Linker flags for all build types
add_compile_options(${FORCE_ERROR} ${DISABLED_OPTIMIZATIONS})

# Set up platform specific library linking
IF (WIN32 OR MSYS OR MINGW OR CYGWIN)
    SET(OS_SPECIFIC_LIBRARIES ws2_32 wsock32 dl m)
ELSEIF (LINUX)
    SET(OS_SPECIFIC_LIBRARIES "")
ELSEIF (APPLE)
    SET(OS_SPECIFIC_LIBRARIES "")
ELSE ()
    SET(OS_SPECIFIC_LIBRARIES "")
ENDIF ()
LIST(APPEND CMAKE_REQUIRED_LIBRARIES ${OS_SPECIFIC_LIBRARIES})
IF (CMAKE_REQUIRED_LIBRARIES)
    LIST(REMOVE_DUPLICATES CMAKE_REQUIRED_LIBRARIES)
ENDIF ()

# Set the list of modules and files which should be copied into the install directory.
SET(GL_MODULES
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
SET(HEADER_FILE_NAMES
        ${GLDCORE_LOC}/class.h
        ${GLDCORE_LOC}/complex.h
        ${GLDCORE_LOC}/debug.h
        ${GLDCORE_LOC}/enduse.h
        ${GLDCORE_LOC}/exception.h
        ${GLDCORE_LOC}/loadshape.h
        ${GLDCORE_LOC}/lock.h
        ${GLDCORE_LOC}/module.h
        ${GLDCORE_LOC}/object.h
        ${GLDCORE_LOC}/property.h
        ${GLDCORE_LOC}/schedule.h
        ${GLDCORE_LOC}/test.h

        ${CMAKE_CURRENT_BINARY_DIR}/headers/build.h
        ${CMAKE_CURRENT_BINARY_DIR}/headers/version.h
        )
SET(GLD_SHARE
        ${GLDCORE_LOC}/tzinfo.txt
        ${GLDCORE_LOC}/unitfile.txt
        ${GLDCORE_LOC}/rt/about.htm
        ${GLDCORE_LOC}/rt/COPYRIGHT
        ${GLDCORE_LOC}/rt/debugger.conf
        ${GLDCORE_LOC}/rt/eula.htm
        ${GLDCORE_LOC}/rt/favicon.ico
        ${GLDCORE_LOC}/rt/gnuplot.conf
        ${GLDCORE_LOC}/rt/gridlabd.conf
        ${GLDCORE_LOC}/rt/gridlabd.css
        ${GLDCORE_LOC}/rt/gridlabd.h
        ${GLDCORE_LOC}/rt/gridlabd.htm
        ${GLDCORE_LOC}/rt/gridlabd.jpg
        ${GLDCORE_LOC}/rt/gridlabd.js
        ${GLDCORE_LOC}/rt/gridlabd.syn
        ${GLDCORE_LOC}/rt/LICENSE
        ${GLDCORE_LOC}/rt/mingw.conf
        ${GLDCORE_LOC}/rt/STATUS
        ${GLDCORE_LOC}/rt/capacitor_b.png
        ${GLDCORE_LOC}/rt/capacitor_g.png
        ${GLDCORE_LOC}/rt/capacitor_k.png
        ${GLDCORE_LOC}/rt/capacitor_r.png
        ${GLDCORE_LOC}/rt/load_b.png
        ${GLDCORE_LOC}/rt/load_g.png
        ${GLDCORE_LOC}/rt/load_k.png
        ${GLDCORE_LOC}/rt/load_r.png
        ${GLDCORE_LOC}/rt/meter_g.png
        ${GLDCORE_LOC}/rt/node_b.png
        ${GLDCORE_LOC}/rt/node_g.png
        ${GLDCORE_LOC}/rt/node_k.png
        ${GLDCORE_LOC}/rt/node_r.png
        ${GLDCORE_LOC}/rt/regulator_b.png
        ${GLDCORE_LOC}/rt/regulator_g.png
        ${GLDCORE_LOC}/rt/regulator_k.png
        ${GLDCORE_LOC}/rt/regulator_r.png
        ${GLDCORE_LOC}/rt/switch_b.png
        ${GLDCORE_LOC}/rt/switch_g.png
        ${GLDCORE_LOC}/rt/switch_k.png
        ${GLDCORE_LOC}/rt/switch_r.png
        ${GLDCORE_LOC}/rt/transformer_b.png

        ${GLDCORE_LOC}/rt/transformer_g.png
        ${GLDCORE_LOC}/rt/transformer_k.png
        ${GLDCORE_LOC}/rt/transformer_r.png
        ${GLDCORE_LOC}/rt/triplex_meter_b.png
        ${GLDCORE_LOC}/rt/triplex_meter_g.png
        ${GLDCORE_LOC}/rt/triplex_meter_k.png
        ${GLDCORE_LOC}/rt/triplex_meter_r.png

        models/climate_csvreader_example.glm
        models/collector_example.glm
        models/diesel_deltamode_load_player_A.csv
        models/diesel_deltamode_load_player_B.csv
        models/diesel_deltamode_load_player_C.csv
        models/house_HVAC_example.glm
        models/IEEE_13_Node_Test_Feeder.glm
        models/IEEE_13_Node_With_Houses.glm
        models/passive_controller_example.glm
        models/random_fault_generator_example.glm
        models/residential_zipload_example.glm
        models/Standard_Weather.csv
        models/subsecond_diesel_generator_example.glm
        models/taxonomy_feeder_R1-12.47-1.glm
        models/transactive_controller_example.glm
        models/WA-Seattle.tmy2
        models/WA-Yakima.tmy2
        models/waterheater_example.glm
        models/weather.csv
        models/wind_turbine_example.glm

        #        ${GLDCORE_LOC}/gridlabd.htm
        ${GLDCORE_LOC}/gridlabd.htm
        )

# Load module directories. When adding new module, add it here.
include_directories(gldcore)

include_directories(third_party/superLU_MT)
include_directories(third_party/jsoncpp_lib/include)

add_subdirectory(third_party/superLU_MT)

option(JSON_CPP_WITH_TESTS "Build the JSON CPP test suite" OFF)
add_subdirectory(third_party/jsoncpp_lib)

include(assert/CMakeLists.txt)
include(climate/CMakeLists.txt)
include(commercial/CMakeLists.txt)
include(connection/CMakeLists.txt)
include(generators/CMakeLists.txt)
include(market/CMakeLists.txt)
include(mysql/CMakeLists.txt)
include(optimize/CMakeLists.txt)
include(powerflow/CMakeLists.txt)
include(reliability/CMakeLists.txt)
include(residential/CMakeLists.txt)
include(tape/CMakeLists.txt)
include(tape_file/CMakeLists.txt)
include(tape_plot/CMakeLists.txt)
include(${GLDCORE_LOC}/solvers/CMakeLists.txt)
include(${GLDCORE_LOC}/link/engine/CMakeLists.txt)

# Load and link C/C++ files for the executeable
add_executable(gridlabd
        ${GLDCORE_LOC}/gridlabd.h
        ${GLDCORE_LOC}/globals.cpp
        ${GLDCORE_LOC}/globals.h
        ${GLDCORE_LOC}/object.cpp
        ${GLDCORE_LOC}/object.h
        ${GLDCORE_LOC}/schedule.cpp
        ${GLDCORE_LOC}/schedule.h
        ${GLDCORE_LOC}/aggregate.cpp
        ${GLDCORE_LOC}/aggregate.h
        ${GLDCORE_LOC}/class.cpp
        ${GLDCORE_LOC}/class.h
        ${GLDCORE_LOC}/cmdarg.cpp
        ${GLDCORE_LOC}/cmdarg.h
        #${GLDCORE_LOC}/cmex.c
        #${GLDCORE_LOC}/cmex.h
        ${GLDCORE_LOC}/compare.cpp
        ${GLDCORE_LOC}/compare.h
        ${GLDCORE_LOC}/complex.h
        ${GLDCORE_LOC}/console.h
        ${GLDCORE_LOC}/convert.cpp
        ${GLDCORE_LOC}/convert.h
        ${GLDCORE_LOC}/cpp_threadpool.cpp
        ${GLDCORE_LOC}/cpp_threadpool.h
        ${GLDCORE_LOC}/debug.cpp
        ${GLDCORE_LOC}/debug.h
        ${GLDCORE_LOC}/deltamode.cpp
        ${GLDCORE_LOC}/deltamode.h
        ${GLDCORE_LOC}/enduse.cpp
        ${GLDCORE_LOC}/enduse.h
        ${GLDCORE_LOC}/environment.cpp
        ${GLDCORE_LOC}/environment.h
        ${GLDCORE_LOC}/exception.cpp
        ${GLDCORE_LOC}/exception.h
        ${GLDCORE_LOC}/exec.cpp
        ${GLDCORE_LOC}/exec.h
        ${GLDCORE_LOC}/find.cpp
        ${GLDCORE_LOC}/find.h
        ${GLDCORE_LOC}/gld_sock.h
        ${GLDCORE_LOC}/gui.cpp
        ${GLDCORE_LOC}/gui.h
        ${GLDCORE_LOC}/http_client.cpp
        ${GLDCORE_LOC}/http_client.h
        ${GLDCORE_LOC}/index.cpp
        ${GLDCORE_LOC}/index.h
        ${GLDCORE_LOC}/instance.cpp
        ${GLDCORE_LOC}/instance.h
        ${GLDCORE_LOC}/instance_cnx.cpp
        ${GLDCORE_LOC}/instance_cnx.h
        ${GLDCORE_LOC}/instance_slave.cpp
        ${GLDCORE_LOC}/instance_slave.h
        ${GLDCORE_LOC}/interpolate.cpp
        ${GLDCORE_LOC}/interpolate.h
        ${GLDCORE_LOC}/job.cpp
        ${GLDCORE_LOC}/job.h
        ${GLDCORE_LOC}/kill.cpp
        ${GLDCORE_LOC}/kill.h
        ${GLDCORE_LOC}/kml.cpp
        ${GLDCORE_LOC}/kml.h
        ${GLDCORE_LOC}/legal.cpp
        ${GLDCORE_LOC}/legal.h
        ${GLDCORE_LOC}/link.cpp
        ${GLDCORE_LOC}/link.h
        ${GLDCORE_LOC}/linkage.cpp
        ${GLDCORE_LOC}/linkage.h
        ${GLDCORE_LOC}/list.cpp
        ${GLDCORE_LOC}/list.h
        ${GLDCORE_LOC}/load.cpp
        ${GLDCORE_LOC}/load.h
        ${GLDCORE_LOC}/load_xml.cpp
        ${GLDCORE_LOC}/load_xml.h
        ${GLDCORE_LOC}/load_xml_handle.cpp
        ${GLDCORE_LOC}/load_xml_handle.h
        ${GLDCORE_LOC}/loadshape.cpp
        ${GLDCORE_LOC}/loadshape.h
        ${GLDCORE_LOC}/local.cpp
        ${GLDCORE_LOC}/local.h
        ${GLDCORE_LOC}/lock.cpp
        ${GLDCORE_LOC}/lock.h
        ${GLDCORE_LOC}/main.cpp
        ${GLDCORE_LOC}/match.cpp
        ${GLDCORE_LOC}/match.h
        ${GLDCORE_LOC}/matlab.cpp
        ${GLDCORE_LOC}/matlab.h
        ${GLDCORE_LOC}/module.cpp
        ${GLDCORE_LOC}/module.h
        ${GLDCORE_LOC}/output.cpp
        ${GLDCORE_LOC}/output.h
        ${GLDCORE_LOC}/platform.h
        ${GLDCORE_LOC}/property.cpp
        ${GLDCORE_LOC}/property.h
        ${GLDCORE_LOC}/random.cpp
        ${GLDCORE_LOC}/gldrandom.h
        ${GLDCORE_LOC}/realtime.cpp
        ${GLDCORE_LOC}/realtime.h
        ${GLDCORE_LOC}/sanitize.cpp
        ${GLDCORE_LOC}/sanitize.h
        ${GLDCORE_LOC}/save.cpp
        ${GLDCORE_LOC}/save.h
        ${GLDCORE_LOC}/server.cpp
        ${GLDCORE_LOC}/server.h
        ${GLDCORE_LOC}/setup.cpp
        ${GLDCORE_LOC}/setup.h
        ${GLDCORE_LOC}/stream.cpp
        ${GLDCORE_LOC}/stream.h
        ${GLDCORE_LOC}/stream_type.h
        ${GLDCORE_LOC}/test.cpp
        ${GLDCORE_LOC}/test.h
        ${GLDCORE_LOC}/test_callbacks.h
        ${GLDCORE_LOC}/test_framework.cpp
        ${GLDCORE_LOC}/test_framework.h
        ${GLDCORE_LOC}/threadpool.cpp
        ${GLDCORE_LOC}/threadpool.h
        ${GLDCORE_LOC}/timestamp.cpp
        ${GLDCORE_LOC}/timestamp.h
        ${GLDCORE_LOC}/transform.cpp
        ${GLDCORE_LOC}/transform.h
        ${GLDCORE_LOC}/ufile.cpp
        ${GLDCORE_LOC}/ufile.h
        ${GLDCORE_LOC}/unit.cpp
        ${GLDCORE_LOC}/unit.h
        ${GLDCORE_LOC}/validate.cpp
        ${GLDCORE_LOC}/validate.h
        ${GLDCORE_LOC}/version.cpp
        #        ${GLDCORE_LOC}/xcore.cpp
        #        ${GLDCORE_LOC}/xcore.h

        ${CMAKE_CURRENT_BINARY_DIR}/headers/version.h
        ${CMAKE_CURRENT_BINARY_DIR}/headers/build.h
        ${CMAKE_CURRENT_BINARY_DIR}/headers/config.h
        )

mark_as_advanced(FORCE
        OS_SPECIFIC_LIBRARIES
        DISABLED_OPTIMIZATIONS
        BUILD_SHARED_LIBS
        GL_MODULES
        HEADER_FILE_NAMES
        GLD_SHARE
        )

#MESSAGE(${CMAKE_CURRENT_SOURCE_DIR}/third_party/mysql-connector-c++)
#
#add_custom_target(
#        mysql_connector
#        COMMAND ./install.sh
#        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mysql-connector-c++
#)
#add_dependencies(gridlabd mysql_connector)
if (HAVE_MYSQL)
    include_directories(${MYSQL_INCLUDE_DIRECTORIES} ${MYSQL_INCLUDE_DIRECTORIES}/include)
    target_link_libraries(gridlabd ${MYSQL_LIBRARY})
ENDIF ()

if (HAVE_HELICS)
    target_link_libraries(gridlabd ${GL_HELICS_LIBRARIES})
ENDIF ()

if (HAVE_FNCS)
    target_link_libraries(gridlabd ${GL_FNCS_LIBRARIES})
ENDIF ()

add_dependencies(gridlabd jsoncpp_lib libsuperlu)

target_link_libraries(gridlabd ${CMAKE_THREAD_LIBS_INIT})
if (CURSES_FOUND)
    target_link_libraries(gridlabd ${CURSES_LIBRARIES})
endif ()

target_link_libraries(gridlabd ${GL_MODULES})
target_link_libraries(gridlabd jsoncpp_lib libsuperlu)

IF (WIN32 OR MSYS OR MINGW OR CYGWIN)
    target_link_libraries(gridlabd ${OS_SPECIFIC_LIBRARIES})
ELSE ()
    target_link_libraries(gridlabd ${CMAKE_DL_LIBS})
ENDIF ()

SET(FILE_PERMISSIONS PERMISSIONS
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
