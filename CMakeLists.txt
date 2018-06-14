cmake_minimum_required(VERSION 2.8.12)
project(gridlabd)
#set(CMAKE_CXX_STANDARD 11)
#SET(CMAKE_CXX_STANDARD_REQUIRED ON)

# Package information for Gridlab-d
SET(DLEXT ${CMAKE_SHARED_LIBRARY_SUFFIX})
SET(PACKAGE "${PROJECT_NAME}")
SET(PACKAGE_BUGREPORT "gridlabd@pnnl.gov")
SET(PACKAGE_NAME "GridLAB-D")
SET(PACKAGE_STRING "GridLAB-D 4.0.0")
SET(PACKAGE_TARNAME "${PROJECT_NAME}")
SET(PACKAGE_URL "")
SET(PACKAGE_VERSION "4.0.0")

# Sets default compile scheme to Release (Maximum optimization, no debugger data)
# To compile as Debug outside of IDE, run cmake with "-DCMAKE_BUILD_TYPE=Debug" flag.
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

# Allows specific compiler optimizations to be disabled for all build types.
set(DISABLED_OPTIMIZATIONS "")
#set(DISABLED_OPTIMIZATIONS "${DISABLED_OPTIMIZATIONS} -fno-inline-functions")

# Compiler/Linker flags for all build types
set(CMAKE_CXX_FLAGS "-w")
set(CMAKE_C_FLAGS "-w")
set(CMAKE_LD_FLAGS "-w")

# Compiler/Linker flags for debug specific build types
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 ${DISABLED_OPTIMIZATIONS}")
set(CMAKE_C_FLAGS_DEBUG "-g -O0 ${DISABLED_OPTIMIZATIONS}")
set(CMAKE_LD_FLAGS_DEBUG "-g -O0 ${DISABLED_OPTIMIZATIONS}")

# Compiler/Linker flags for release specific build types (default)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 ${DISABLED_OPTIMIZATIONS}")
set(CMAKE_C_FLAGS_RELEASE "-O3 ${DISABLED_OPTIMIZATIONS}")
set(CMAKE_LD_FLAGS_RELEASE "-O3 ${DISABLED_OPTIMIZATIONS}")

# Set up platform specific library linking
IF (WIN32 OR MSYS OR MINGW OR CYGWIN)
#    find_library(MYLIB
#            NAMES "dl"
#            HINTS "C:\\MinGW\\msys\\1.0\\opt\\windows_64\\mingw\\bin"
#            NO_DEFAULT_PATH
#            )

#    message(${MYLIB})
    SET(OS_SPECIFIC_LIBRARIES ws2_32 wsock32 dl) # ${MYLIB})
ELSEIF (LINUX)
    SET(OS_SPECIFIC_LIBRARIES ${CMAKE_DL_LIBS})
ELSEIF (APPLE)
    SET(OS_SPECIFIC_LIBRARIES ${CMAKE_DL_LIBS})
ELSE ()
    SET(OS_SPECIFIC_LIBRARIES ${CMAKE_DL_LIBS})
ENDIF ()
LIST(APPEND CMAKE_REQUIRED_LIBRARIES ${OS_SPECIFIC_LIBRARIES})

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
        gldcore/build.h
        gldcore/class.h
        gldcore/complex.h
        gldcore/debug.h
        gldcore/enduse.h
        gldcore/exception.h
        gldcore/loadshape.h
        gldcore/lock.h
        gldcore/module.h
        gldcore/object.h
        gldcore/property.h
        gldcore/schedule.h
        gldcore/test.h
        gldcore/version.h
        )
SET(GLD_SHARE
        gldcore/tzinfo.txt
        gldcore/unitfile.txt
        gldcore/rt/about.htm
        gldcore/rt/COPYRIGHT
        gldcore/rt/debugger.conf
        gldcore/rt/eula.htm
        gldcore/rt/favicon.ico
        gldcore/rt/gnuplot.conf
        gldcore/rt/gridlabd.conf
        gldcore/rt/gridlabd.css
        gldcore/rt/gridlabd.h
        gldcore/rt/gridlabd.htm
        gldcore/rt/gridlabd.jpg
        gldcore/rt/gridlabd.js
        gldcore/rt/gridlabd.syn
        gldcore/rt/LICENSE
        gldcore/rt/mingw.conf
        gldcore/rt/STATUS
        gldcore/rt/capacitor_b.png
        gldcore/rt/capacitor_g.png
        gldcore/rt/capacitor_k.png
        gldcore/rt/capacitor_r.png
        gldcore/rt/load_b.png
        gldcore/rt/load_g.png
        gldcore/rt/load_k.png
        gldcore/rt/load_r.png
        gldcore/rt/meter_g.png
        gldcore/rt/node_b.png
        gldcore/rt/node_g.png
        gldcore/rt/node_k.png
        gldcore/rt/node_r.png
        gldcore/rt/regulator_b.png
        gldcore/rt/regulator_g.png
        gldcore/rt/regulator_k.png
        gldcore/rt/regulator_r.png
        gldcore/rt/switch_b.png
        gldcore/rt/switch_g.png
        gldcore/rt/switch_k.png
        gldcore/rt/switch_r.png
        gldcore/rt/transformer_b.png

        gldcore/rt/transformer_g.png
        gldcore/rt/transformer_k.png
        gldcore/rt/transformer_r.png
        gldcore/rt/triplex_meter_b.png
        gldcore/rt/triplex_meter_g.png
        gldcore/rt/triplex_meter_k.png
        gldcore/rt/triplex_meter_r.png

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
        gldcore/gridlabd.htm
        )

find_package(Threads)
find_package(Curses)

SET(HAVE_CURSES CURSES_FOUND)

# Link required libraries for symbol checks
list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
list(APPEND CMAKE_REQUIRED_LIBRARIES m dl)

# CMake Module includes
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckFunctionExists)

# Check header files
CHECK_INCLUDE_FILES(arpa/inet.h HAVE_ARPA_INET_H)
CHECK_INCLUDE_FILES(fcntl.h HAVE_FCNTL_H)
CHECK_INCLUDE_FILES(float.h HAVE_FLOAT_H)
CHECK_INCLUDE_FILES(inttypes.h HAVE_INTTYPES_H)
CHECK_INCLUDE_FILES(limits.h HAVE_LIMITS_H)
CHECK_INCLUDE_FILES(malloc.h HAVE_MALLOC_H)
CHECK_INCLUDE_FILES(math.h HAVE_MATH_H)
CHECK_INCLUDE_FILES(memory.h HAVE_MEMORY_H)
CHECK_INCLUDE_FILES(netdb.h HAVE_NETDB_H)
CHECK_INCLUDE_FILES(netinet/in.h HAVE_NETINET_IN_H)
CHECK_INCLUDE_FILES(sched.h HAVE_SCHED_H)
CHECK_INCLUDE_FILES(stddef.h HAVE_STDDEF_H)
CHECK_INCLUDE_FILES(stdint.h HAVE_STDINT_H)
CHECK_INCLUDE_FILES(stdlib.h HAVE_STDLIB_H)
CHECK_INCLUDE_FILES(string.h HAVE_STRING_H)
CHECK_INCLUDE_FILES(sys/ioctl.h HAVE_IOCTL_H)
CHECK_INCLUDE_FILES(sys/param.h HAVE_PARAM_H)
CHECK_INCLUDE_FILES(sys/socket.h HAVE_SOCKET_H)
CHECK_INCLUDE_FILES(sys/timeb.h HAVE_TIMEB_H)
CHECK_INCLUDE_FILES(sys/time.h HAVE_TIME_H)
CHECK_INCLUDE_FILES(unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILES(wchar.h HAVE_WCHAR_H)
CHECK_INCLUDE_FILES(curses.h HAVE_CURSES_H)
CHECK_INCLUDE_FILES(ncursesw/curses.h HAVE_NCURSESW_CURSES_H)
CHECK_INCLUDE_FILES(ncursesw.h HAVE_NCURSESW_H)
CHECK_INCLUDE_FILES(ncurses/curses.h HAVE_NCURSES_CURSES_H)
CHECK_INCLUDE_FILES(ncurses.h HAVE_NCURSES_H)
CHECK_INCLUDE_FILES(dlfcn.h HAVE_DLFCN_H)

CHECK_FUNCTION_EXISTS(madvise HAVE_MADVISE)
CHECK_FUNCTION_EXISTS(error_at_line HAVE_ERROR_AT_LINE) #AC_FUNC_ERROR_AT_LINE
CHECK_FUNCTION_EXISTS(malloc HAVE_MALLOC) #AC_FUNC_MALLOC
CHECK_FUNCTION_EXISTS(mktime HAVE_MKTIME) #AC_FUNC_MKTIME
CHECK_FUNCTION_EXISTS(realloc HAVE_REALLOC) #AC_FUNC_REALLOC
CHECK_FUNCTION_EXISTS(strftime HAVE_STRFTIME) #AC_FUNC_STRFTIME
CHECK_FUNCTION_EXISTS(strtod HAVE_STRTOD) #AC_FUNC_STRTOD
CHECK_FUNCTION_EXISTS(vprintf HAVE_VPRINTF) #AC_FUNC_VPRINTF
CHECK_FUNCTION_EXISTS(alarm HAVE_ALARM)
CHECK_FUNCTION_EXISTS(atexit HAVE_ATEXIT)
CHECK_FUNCTION_EXISTS(ftime HAVE_FTIME)
CHECK_FUNCTION_EXISTS(getcwd HAVE_GETCWD)
CHECK_FUNCTION_EXISTS(gethostbyname HAVE_GETHOSTBYNAME)
CHECK_FUNCTION_EXISTS(gettimeofday HAVE_GETTIMEOFDAY)
CHECK_FUNCTION_EXISTS(inet_ntoa HAVE_INET_NTOA)
CHECK_FUNCTION_EXISTS(memset HAVE_MEMSET)
CHECK_FUNCTION_EXISTS(mkdir HAVE_MKDIR)
CHECK_FUNCTION_EXISTS(putenv HAVE_PUTENV)
CHECK_FUNCTION_EXISTS(select HAVE_SELECT)
CHECK_FUNCTION_EXISTS(setenv HAVE_SETENV)
CHECK_FUNCTION_EXISTS(socket HAVE_SOCKET)
CHECK_FUNCTION_EXISTS(strchr HAVE_STRCHR)
CHECK_FUNCTION_EXISTS(strcspn HAVE_STRCSPN)
CHECK_FUNCTION_EXISTS(strerror HAVE_STRERROR)
CHECK_FUNCTION_EXISTS(strpbrk HAVE_STRPBRK)
CHECK_FUNCTION_EXISTS(strrchr HAVE_STRRCHR)
CHECK_FUNCTION_EXISTS(strstr HAVE_STRSTR)
CHECK_FUNCTION_EXISTS(strtol HAVE_STRTOL)
CHECK_FUNCTION_EXISTS(tzset HAVE_TZSET)
CHECK_FUNCTION_EXISTS(gethrtime HAVE_GETHRTIME)
CHECK_FUNCTION_EXISTS(floor HAVE_FLOOR)
CHECK_FUNCTION_EXISTS(pow HAVE_POW)
CHECK_FUNCTION_EXISTS(sqrt HAVE_SQRT)

# Test for compiler builtin functions.
FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/HAVE_DYN_PROC_AFFINITY.cc "
#include <sched.h>
int main() {
    cpu_set_t *cpu = CPU_ALLOC(2);
    return 0;
}")
FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/HAVE___SYNC_ADD_AND_FETCH.cc "
int main() {
    volatile unsigned int value = 0;
    __sync_add_and_fetch(&value, 1);
    return 0;
}")
FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/HAVE___SYNC_BOOL_COMPARE_AND_SWAP.cc "
int main() {
    volatile unsigned int value = 0;
    __sync_bool_compare_and_swap(&value, value, 1);
    return 0;
}")
TRY_COMPILE(DYN_PROC_AFFINITY ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/HAVE_DYN_PROC_AFFINITY.cc)
TRY_COMPILE(HAVE___SYNC_ADD_AND_FETCH ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/HAVE___SYNC_ADD_AND_FETCH.cc)
TRY_COMPILE(HAVE___SYNC_BOOL_COMPARE_AND_SWAP ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/HAVE___SYNC_BOOL_COMPARE_AND_SWAP.cc)

# Generate the config.h header file.
configure_file(gldcore/cmake_config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/gldcore/config.h @ONLY)
add_definitions(-DHAVE_CONFIG_H)

# Load module directories. When adding new module, add it here.
include_directories(gldcore)
include_directories(third_party/superLU_MT)
include_directories(third_party/jsonCpp)

include(third_party/superLU_MT/CMakeLists.txt)
include(third_party/jsonCpp/CMakeLists.txt)

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
include(gldcore/solvers/CMakeLists.txt)
include(gldcore/link/engine/CMakeLists.txt)


# Load and link C/C++ files for the executeable
add_executable(gridlabd
        gldcore/globals.c
        gldcore/globals.h
        gldcore/gridlabd.h
        gldcore/aggregate.c
        gldcore/aggregate.h
        gldcore/build.h
        gldcore/class.c
        gldcore/class.h
        gldcore/cmdarg.c
        gldcore/cmdarg.h
        #        gldcore/cmex.c
        #gldcore/cmex.h
        gldcore/compare.c
        gldcore/compare.h
        gldcore/complex.h
        gldcore/config.h
        gldcore/console.h
        gldcore/convert.cpp
        gldcore/convert.h
        gldcore/debug.c
        gldcore/debug.h
        gldcore/deltamode.c
        gldcore/deltamode.h
        gldcore/enduse.c
        gldcore/enduse.h
        gldcore/environment.c
        gldcore/environment.h
        gldcore/exception.c
        gldcore/exception.h
        gldcore/exec.c
        gldcore/exec.h
        gldcore/find.c
        gldcore/find.h
        gldcore/gld_sock.h
        gldcore/gui.c
        gldcore/gui.h
        gldcore/http_client.c
        gldcore/http_client.h
        gldcore/index.c
        gldcore/index.h
        gldcore/instance.c
        gldcore/instance.h
        gldcore/instance_cnx.c
        gldcore/instance_cnx.h
        gldcore/instance_slave.c
        gldcore/instance_slave.h
        gldcore/interpolate.c
        gldcore/interpolate.h
        gldcore/job.cpp
        gldcore/job.h
        gldcore/kill.c
        gldcore/kill.h
        gldcore/kml.c
        gldcore/kml.h
        gldcore/legal.c
        gldcore/legal.h
        gldcore/link.cpp
        gldcore/link.h
        gldcore/linkage.c
        gldcore/linkage.h
        gldcore/list.c
        gldcore/list.h
        gldcore/load.c
        gldcore/load.h
        gldcore/load_xml.cpp
        gldcore/load_xml.h
        gldcore/load_xml_handle.cpp
        gldcore/load_xml_handle.h
        gldcore/loadshape.c
        gldcore/loadshape.h
        gldcore/local.c
        gldcore/local.h
        gldcore/lock.cpp
        gldcore/lock.h
        gldcore/main.c
        gldcore/match.c
        gldcore/match.h
        gldcore/matlab.c
        gldcore/matlab.h
        gldcore/module.c
        gldcore/module.h
        gldcore/object.c
        gldcore/object.h
        gldcore/output.c
        gldcore/output.h
        gldcore/platform.h
        gldcore/property.c
        gldcore/property.h
        gldcore/random.c
        gldcore/random.h
        gldcore/realtime.c
        gldcore/realtime.h
        gldcore/sanitize.cpp
        gldcore/sanitize.h
        gldcore/save.c
        gldcore/save.h
        gldcore/schedule.c
        gldcore/schedule.h
        gldcore/server.c
        gldcore/server.h
        gldcore/setup.cpp
        gldcore/setup.h
        gldcore/stream.cpp
        gldcore/stream.h
        gldcore/stream_type.h
        gldcore/test.c
        gldcore/test.h
        gldcore/test_callbacks.h
        gldcore/test_framework.cpp
        gldcore/test_framework.h
        gldcore/threadpool.c
        gldcore/threadpool.h
        gldcore/timestamp.c
        gldcore/timestamp.h
        gldcore/transform.c
        gldcore/transform.h
        gldcore/ufile.c
        gldcore/ufile.h
        gldcore/unit.c
        gldcore/unit.h
        gldcore/validate.cpp
        gldcore/validate.h
        gldcore/version.c
        gldcore/version.h
        #        gldcore/xcore.cpp
        #        gldcore/xcore.h
        )

add_dependencies(gridlabd libjsoncpp libsuperlu)

target_link_libraries(gridlabd ${CMAKE_THREAD_LIBS_INIT})
if (CURSES_FOUND)
    target_link_libraries(gridlabd ${CURSES_LIBRARIES})
endif ()

target_link_libraries(gridlabd ${GL_MODULES})
target_link_libraries(gridlabd libjsoncpp libsuperlu)

target_link_libraries(gridlabd ${OS_SPECIFIC_LIBRARIES})

SET(FILE_PERMISSIONS PERMISSIONS
        OWNER_EXECUTE OWNER_READ OWNER_WRITE
        GROUP_EXECUTE GROUP_READ
        WORLD_EXECUTE WORLD_READ)

install(TARGETS gridlabd
        ${FILE_PERMISSIONS}
        RUNTIME DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install64/bin
        )
install(TARGETS ${GL_MODULES}
        ${FILE_PERMISSIONS}
        RUNTIME DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install64/lib/gridlabd
        LIBRARY DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install64/lib/gridlabd
        ARCHIVE DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install64/lib/gridlabd
        COMPONENT library)

install(FILES ${HEADER_FILE_NAMES}
        ${FILE_PERMISSIONS}
        DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install64/include/gridlabd
        )

install(FILES ${GLD_SHARE}
        ${FILE_PERMISSIONS}
        DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install64/share/gridlabd)
install(FILES COPYRIGHT LICENSE
        ${FILE_PERMISSIONS}
        DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install64/share/doc/gridlabd)


#install(FILES ${MYLIB}
#        ${FILE_PERMISSIONS}
#        DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install64/bin
#        )
