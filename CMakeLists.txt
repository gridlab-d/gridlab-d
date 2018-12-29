cmake_minimum_required(VERSION 2.8.12)
project(gridlabd)

if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
    endif ()
else ()
    SET(CMAKE_CXX_STANDARD 11)
    SET(CMAKE_CXX_STANDARD_REQUIRED ON)
endif ()

# Get the current working branch
execute_process(
        COMMAND git rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get the latest abbreviated commit hash of the working branch
execute_process(
        COMMAND git log -1 --format=%h
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Package information for Gridlab-d
SET(DLEXT ${CMAKE_SHARED_LIBRARY_SUFFIX})
SET(PACKAGE "${PROJECT_NAME}")
SET(PACKAGE_BUGREPORT "gridlabd@pnnl.gov")
SET(PACKAGE_NAME "GridLAB-D")
SET(REV_MAJOR 4)
SET(REV_MINOR 1)
SET(REV_PATCH 0)
SET(PACKAGE_STRING "GridLAB-D ${REV_MAJOR}.${REV_MINOR}.${REV_PATCH} (Branch:${GIT_BRANCH} - ${GIT_COMMIT_HASH})")
SET(PACKAGE_TARNAME "${PROJECT_NAME}")
SET(PACKAGE_URL "")
SET(PACKAGE_VERSION ${REV_MAJOR}.${REV_MINOR}.${REV_PATCH})

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

# Allows specific compiler optimizations to be disabled for all build types.
set(DISABLED_OPTIMIZATIONS "${DISABLED_OPTIMIZATIONS} -fno-inline-functions")

# Compiler/Linker flags for all build types
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w ${DISABLED_OPTIMIZATIONS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w ${DISABLED_OPTIMIZATIONS}")
set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -w ${DISABLED_OPTIMIZATIONS}")

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

# Set configurable flags for runtime.
OPTION(USE_MYSQL "Enable building with MySQL" OFF)
OPTION(USE_HELIC "Enable building with HELICS" OFF)
OPTION(USE_FNCS "Enable building with FNCS" OFF)
OPTION(USE_OPENMP "Enable building with OpenMP" OFF)

# Configure MySQL to be configured and linked if available.
IF (USE_MYSQL)
    INCLUDE(CMakeScripts/FindMySQL.cmake)
    IF (FOUND_MYSQL)
        SET(HAVE_MYSQL ON)
    ENDIF ()
ENDIF ()

#message("Have mysql? -> ${HAVE_MYSQL}")

# Configure OpenMP to be compiled and linked if available.
if (WITH_OPENMP)
    include(FindOpenMP)
    if (OPENMP_FOUND)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
        set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} ${OpenMP_LD_FLAGS}")
    endif ()
ENDIF () # WITH_OPENMP

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
        ${GLDCORE_LOC}/build.h
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
        ${GLDCORE_LOC}/version.h
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

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)

if (CMAKE_USE_PTHREADS_INIT)
    set(HAVE_PTHREAD 1)
endif ()

find_package(Curses)

IF (CURSES_FOUND)
    SET(HAVE_CURSES 1)
ENDIF ()

# Link required libraries for symbol checks
list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
#list(APPEND CMAKE_REQUIRED_LIBRARIES m dl)
# CMake Module includes
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckSymbolExists)

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
CHECK_INCLUDE_FILES(strings.h HAVE_STRINGS_H)
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
CHECK_INCLUDE_FILES(sys/ioctl.h HAVE_SYS_IOCTL_H)
CHECK_INCLUDE_FILES(sys/param.h HAVE_SYS_PARAM_H)
CHECK_INCLUDE_FILES(sys/socket.h HAVE_SYS_SOCKET_H)
CHECK_INCLUDE_FILES(sys/stat.h HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILES(sys/timeb.h HAVE_SYS_TIMEB_H)
CHECK_INCLUDE_FILES(sys/time.h HAVE_SYS_TIME_H)
CHECK_INCLUDE_FILES(sys/types.h HAVE_SYS_TYPES_H)

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
CHECK_FUNCTION_EXISTS(bindprocessor HAVE_BINDPROCESSOR)
CHECK_FUNCTION_EXISTS(thread_policy_set HAVE_THREAD_POLICY_SET)

CHECK_SYMBOL_EXISTS(sched_setaffinity sched.h HAVE_SCHED_SETAFFINITY)
CHECK_SYMBOL_EXISTS(sched_getaffinity sched.h HAVE_SCHED_GETAFFINITY)

CHECK_SYMBOL_EXISTS(THREAD_AFFINITY_POLICY mach/thread_policy.h HAVE_OSX_THREAD_AFFINITY_POLICY)
CHECK_SYMBOL_EXISTS(THREAD_AFFINITY_TAG_NULL mach/thread_policy.h HAVE_OSX_THREAD_AFFINITY_TAG_NULL)
IF (HAVE_OSX_THREAD_AFFINITY_POLICY AND HAVE_OSX_THREAD_AFFINITY_TAG_NULL)
    SET(HAVE_OSX_THREAD_AFFINITY 1)
ENDIF ()

# Check if _GNU_SOURCE is available.
if (NOT _GNU_SOURCE)
    CHECK_SYMBOL_EXISTS(__GNU_LIBRARY__ "features.h" _GNU_SOURCE)

    if (NOT _GNU_SOURCE)
        unset(_GNU_SOURCE CACHE)
        CHECK_SYMBOL_EXISTS(_GNU_SOURCE "features.h" _GNU_SOURCE)
    endif ()
endif ()

if (_GNU_SOURCE)
    add_definitions(-D_GNU_SOURCE)
endif ()

# Test for compiler builtin functions.
FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/HAVE_CPU_SET_T.cc "
#include <sched.h>
int main() {
    cpu_set_t t;
    return 0;
}")
FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/HAVE_DYN_PROC_AFFINITY.cc "
#include <sched.h>
int main() {
    cpu_set_t *cpu = CPU_ALLOC(2);
    return 0;
}")
FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/HAVE_CPU_SET_MACROS.cc "
#include <sched.h>
int main() {
    cpu_set_t t;
    CPU_ZERO(&t);
    CPU_SET(1,&t);
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
FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/HAVE_GET_NPROCS.cc "
#include <sys/sysinfo.h>
int main() {
    get_nprocs();
}")
FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/HAVE_ISFINITE.cc "
#include <math.h>
int main() {
    int result = isfinite(1.0);
}")
FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/HAVE_PTHREAD_PRIO_INHERIT.cc "
#include <pthread.h>
int main() {
    int i = PTHREAD_PRIO_INHERIT;
}")

TRY_COMPILE(HAVE_CPU_SET_T ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/HAVE_CPU_SET_T.cc)
TRY_COMPILE(DYN_PROC_AFFINITY ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/HAVE_DYN_PROC_AFFINITY.cc)
TRY_COMPILE(HAVE_CPU_SET_MACROS ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/HAVE_CPU_SET_MACROS.cc)
TRY_COMPILE(HAVE___SYNC_ADD_AND_FETCH ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/HAVE___SYNC_ADD_AND_FETCH.cc)
TRY_COMPILE(HAVE___SYNC_BOOL_COMPARE_AND_SWAP ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/HAVE___SYNC_BOOL_COMPARE_AND_SWAP.cc)
TRY_COMPILE(HAVE_GET_NPROCS ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/HAVE_GET_NPROCS.cc)
TRY_COMPILE(HAVE_ISFINITE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/HAVE_ISFINITE.cc)
TRY_COMPILE(HAVE_PTHREAD_PRIO_INHERIT ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/HAVE_PTHREAD_PRIO_INHERIT.cc)

# Generate the config.h header file.
INCLUDE(CMakeScripts/Build_number.cmake)
configure_file(gldcore/cmake_config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/gldcore/config.h @ONLY)
configure_file(gldcore/version.h.in ${CMAKE_CURRENT_SOURCE_DIR}/gldcore/version.h @ONLY)
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
        ${GLDCORE_LOC}/build.h
        ${GLDCORE_LOC}/class.cpp
        ${GLDCORE_LOC}/class.h
        ${GLDCORE_LOC}/cmdarg.cpp
        ${GLDCORE_LOC}/cmdarg.h
        #${GLDCORE_LOC}/cmex.c
        #${GLDCORE_LOC}/cmex.h
        ${GLDCORE_LOC}/compare.cpp
        ${GLDCORE_LOC}/compare.h
        ${GLDCORE_LOC}/complex.h
        ${GLDCORE_LOC}/config.h
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
        ${GLDCORE_LOC}/version.h
        #        ${GLDCORE_LOC}/xcore.cpp
        #        ${GLDCORE_LOC}/xcore.h
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

add_dependencies(gridlabd libjsoncpp libsuperlu)

target_link_libraries(gridlabd ${CMAKE_THREAD_LIBS_INIT})
if (CURSES_FOUND)
    target_link_libraries(gridlabd ${CURSES_LIBRARIES})
endif ()

target_link_libraries(gridlabd ${GL_MODULES})
target_link_libraries(gridlabd libjsoncpp libsuperlu)

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
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
        )
install(TARGETS ${GL_MODULES}
        ${FILE_PERMISSIONS}
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/gridlabd
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/gridlabd
        ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/gridlabd
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
