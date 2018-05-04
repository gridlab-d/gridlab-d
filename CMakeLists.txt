cmake_minimum_required(VERSION 3.10)
project(gridlabd)
set(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

# CMake Module includes
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckFunctionExists)


# Check header files
CHECK_INCLUDE_FILES(arpa/inet.h HAVE_INET_H)
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

CHECK_FUNCTION_EXISTS(madvise HAVE_MADVISE)

#configure_file(cmake_config.h.in config.h)
add_definitions(-DHAVE_CONFIG_H)


# Load module directories. When adding new module, add it here.
include_directories(third_party/superLU_MT)
include_directories(third_party/jsonCpp)
include_directories(gldcore/link/engine)
include_directories(gldcore/solvers)
include_directories(gldcore)

SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/install64/lib)

#add_subdirectory(third_party/superLU_MT)
#add_subdirectory(third_party/jsonCpp)
add_subdirectory(assert)
add_subdirectory(climate)
add_subdirectory(commercial)
add_subdirectory(connection)
#add_subdirectory(gldcore/link/engine)
#add_subdirectory(gldcore/solvers)
add_subdirectory(generators)
add_subdirectory(market)
add_subdirectory(mysql)
add_subdirectory(optimize)
add_subdirectory(powerflow)
add_subdirectory(reliability)
add_subdirectory(residential)
add_subdirectory(tape_file)
add_subdirectory(tape)
add_subdirectory(tape_plot)
#add_subdirectory(gldcore)


# Load and link C/C++ files for the executeable
add_executable(gridlabd
        gldcore/link/engine/engine.cpp
        gldcore/link/engine/engine.h
        gldcore/link/engine/udpconnection.cpp
        #gldcore/link/matlab/matlab.cpp
        #gldcore/odbc++/callablestatement.h
        #gldcore/odbc++/config-win32.h
        #gldcore/odbc++/config.h
        #gldcore/odbc++/connection.h
        #gldcore/odbc++/databasemetadata.h
        #gldcore/odbc++/drivermanager.h
        #gldcore/odbc++/errorhandler.h
        #gldcore/odbc++/preparedstatement.h
        #gldcore/odbc++/resultset.h
        #gldcore/odbc++/resultsetmetadata.h
        #gldcore/odbc++/setup.h
        #gldcore/odbc++/statement.h
        #gldcore/odbc++/threads.h
        #gldcore/odbc++/types.h
        #gldcore/rt/gridlabd.h
        gldcore/globals.c
        gldcore/globals.h
        gldcore/gridlabd.h
        gldcore/solvers/etp.cpp
        gldcore/solvers/main.cpp
        gldcore/solvers/modified_euler.cpp
        gldcore/solvers/newton_method.cpp
        gldcore/aggregate.c
        gldcore/aggregate.h
        gldcore/build.h
        gldcore/class.c
        gldcore/class.h
        gldcore/cmdarg.c
        gldcore/cmdarg.h
        #gldcore/cmex.c
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
        #gldcore/xcore.cpp
        #gldcore/xcore.h
        )


find_package(Threads)
find_package(Curses)
target_link_libraries(gridlabd ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(gridlabd ${CMAKE_DL_LIBS})
target_link_libraries(gridlabd ${CURSES_LIBRARIES})



#TARGET_LINK_LIBRARIES(gridlabd
#        gridlabd_assert
#        gridlabd_climate
#        gridlabd_commercial
#        gridlabd_connection
#        gridlabd_generators
#        gridlabd_market
#        gridlabd_mysql
#        gridlabd_optimize
#        gridlabd_powerflow
#        gridlabd_reliability
#        gridlabd_residential
#        gridlabd_tape_file
#        gridlabd_tape
#        gridlabd_tape_plot
#        )
#find_library()


install(TARGETS gridlabd
        RUNTIME DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install64/bin
        LIBRARY DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install64/lib
        ARCHIVE DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install64/lib/static)
#install(TARGETS gridlabd_assert DESTINATION /some/full/path)