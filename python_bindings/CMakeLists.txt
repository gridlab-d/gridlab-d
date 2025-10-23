cmake_minimum_required(VERSION 3.15)
project(gridlabd_python_bindings)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)
find_package(nanobind CONFIG REQUIRED)

# Find and link GridLAB-D libraries
set(GRIDLABD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(GRIDLABD_LIB_DIR ${GRIDLABD_ROOT}/cmake-build/lib)
set(GRIDLABD_INCLUDE_DIR ${GRIDLABD_ROOT}/gldcore)
set(JSONCPP_INCLUDE_DIR ${GRIDLABD_ROOT}/third_party/jsoncpp_lib/include)

# Check if GridLAB-D library exists
if(NOT EXISTS ${GRIDLABD_LIB_DIR}/libgldapi.so)
    message(FATAL_ERROR "GridLAB-D API library not found at ${GRIDLABD_LIB_DIR}/libgldapi.so. Please build GridLAB-D first.")
endif()

# Check if jsoncpp library exists
if(NOT EXISTS ${GRIDLABD_LIB_DIR}/static/libjsoncpp.a)
    message(FATAL_ERROR "jsoncpp library not found at ${GRIDLABD_LIB_DIR}/static/libjsoncpp.a. Please build GridLAB-D first.")
endif()

# Create the Python module with GridLAB-D integration
nanobind_add_module(
    gridlabd_core
    src_cpp/gld_nanobind.cpp
)

# Add include directories for GridLAB-D headers
target_include_directories(gridlabd_core PRIVATE 
    ${GRIDLABD_INCLUDE_DIR}
    ${GRIDLABD_ROOT}/third_party
    ${JSONCPP_INCLUDE_DIR}
)

# Link against GridLAB-D API library and dependencies
target_link_libraries(gridlabd_core PRIVATE 
    ${GRIDLABD_LIB_DIR}/libgldapi.so
    ${GRIDLABD_LIB_DIR}/static/libjsoncpp.a
)

# Set module properties
target_compile_definitions(gridlabd_core PRIVATE VERSION_INFO=${SKBUILD_PROJECT_VERSION})

# Install the module to the gridlabd package directory
install(TARGETS gridlabd_core
    LIBRARY DESTINATION gridlabd
    RUNTIME DESTINATION gridlabd)
