# cmake_minimum_required(VERSION 3.14)
# project(GLDAPI_Test CXX)

# #set(CMAKE_CXX_STANDARD 17)

# # Tell CMake where to find custom FindGridLABD.cmake
# set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

# # Find GridLAB-D package
# find_package(GridLABD REQUIRED)

# # Include GridLAB-D headers
# include_directories(${GridLABD_INCLUDE_DIRS})
# link_directories(${GridLABD_LIBRARIES})

# # find_package(Curses)
# # Only find Curses on Unix/Linux platforms (not Windows)
# if(UNIX)
#     find_package(Curses)
#     if(CURSES_FOUND)
#         message(STATUS "Curses found on Unix platform")
#     endif()
# endif()

# # Test executable
# add_executable(test_gldapi test_gldapi.cpp)
# target_sources(test_gldapi PRIVATE ${CMAKE_SOURCE_DIR}/gldcore/globals.cpp)
# target_link_libraries(test_gldapi ${GridLABD_LIBRARIES})


# message(STATUS "GridLABD_INCLUDE_DIRS=${GridLABD_INCLUDE_DIRS}")
# message(STATUS "GridLABD_LIBRARIES=${GridLABD_LIBRARIES}")

# if (UNIX AND CURSES_FOUND)
#   target_link_libraries(test_gldapi PRIVATE ${CURSES_LIBRARIES})
#   target_include_directories(test_gldapi PRIVATE ${CURSES_INCLUDE_DIR})
# endif()


# install(TARGETS test_gldapi
#         RUNTIME DESTINATION bin)
