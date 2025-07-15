cmake_minimum_required(VERSION 3.14)
project(GLDAPI_Test CXX)

#set(CMAKE_CXX_STANDARD 17)

# Tell CMake where to find custom FindGridLABD.cmake
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

# Find GridLAB-D package
find_package(GridLABD REQUIRED)

# Include GridLAB-D headers
include_directories(${GridLABD_INCLUDE_DIRS})
link_directories(${GridLABD_LIBRARIES})

message(STATUS "GridLABD_INCLUDE_DIRS=${GridLABD_INCLUDE_DIRS}")
message(STATUS "GridLABD_LIBRARIES=${GridLABD_LIBRARIES}")

# Test executable
add_executable(test_gldapi test_gldapi.cpp)
target_link_libraries(test_gldapi ${GridLABD_LIBRARIES})

install(TARGETS test_gldapi
        RUNTIME DESTINATION bin)
