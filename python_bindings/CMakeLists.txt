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
set(GRIDLABD_LIB_DIR ${GRIDLABD_ROOT}/build/lib)
set(GRIDLABD_INCLUDE_DIR ${GRIDLABD_ROOT}/gldcore)
set(JSONCPP_INCLUDE_DIR ${GRIDLABD_ROOT}/third_party/jsoncpp_lib/include)

# Check if GridLAB-D library exists
if(NOT EXISTS ${GRIDLABD_LIB_DIR}/libgldapi.so)
    message(FATAL_ERROR "GridLAB-D API library not found at ${GRIDLABD_LIB_DIR}/libgldapi.so. Please build GridLAB-D first.")
endif()

# Create the Python module
nanobind_add_module(
    gridlabd_core
    src_cpp/gld_nanobind.cpp
)

# Add include directories
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

# Set RPATH for the module to find the shared library
set_target_properties(gridlabd_core PROPERTIES
    INSTALL_RPATH_USE_LINK_PATH FALSE
    INSTALL_RPATH "\$ORIGIN"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# Set module properties
target_compile_definitions(gridlabd_core PRIVATE VERSION_INFO=${SKBUILD_PROJECT_VERSION})

# Install the module and the shared library to the same directory
install(TARGETS gridlabd_core
    LIBRARY DESTINATION gridlabd
    RUNTIME DESTINATION gridlabd)

# Install the shared library alongside the module
install(FILES ${GRIDLABD_LIB_DIR}/libgldapi.so
        DESTINATION gridlabd)

# Install essential data files (tzinfo.txt, unitfile.txt) alongside the Python package
set(GRIDLABD_DATA_FILES
    ${GRIDLABD_ROOT}/gldcore/tzinfo.txt
    ${GRIDLABD_ROOT}/gldcore/unitfile.txt
)

install(FILES ${GRIDLABD_DATA_FILES}
        DESTINATION gridlabd/share)

# Install all GridLAB-D module libraries (.so files) so users can load modules
# These are needed for simulations (residential, climate, powerflow, etc.)
file(GLOB GRIDLABD_MODULES "${GRIDLABD_LIB_DIR}/*.so")
if(GRIDLABD_MODULES)
    install(FILES ${GRIDLABD_MODULES}
            DESTINATION gridlabd/lib)
endif()

# Also create a modules directory as an alias (GridLAB-D may look in multiple places)
install(CODE "file(MAKE_DIRECTORY \${CMAKE_INSTALL_PREFIX}/gridlabd/lib)")

# Create bundle directories (for future use)
install(CODE "file(MAKE_DIRECTORY \${CMAKE_INSTALL_PREFIX}/gridlabd_bundle/bin)")
install(CODE "file(MAKE_DIRECTORY \${CMAKE_INSTALL_PREFIX}/gridlabd_bundle/lib)")