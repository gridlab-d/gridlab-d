cmake_minimum_required(VERSION 3.15)
project(gridlabd_python_bindings)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Determine library extension based on platform
if(APPLE)
    set(LIB_EXT "dylib")
elseif(WIN32)
    set(LIB_EXT "dll")
else()
    set(LIB_EXT "so")
endif()

# Find required packages
find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)

# Find or fetch nlohmann/json
find_package(nlohmann_json 3.12.0 QUIET)
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json not found, downloading...")
    include(FetchContent)
    FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
    FetchContent_MakeAvailable(json)
endif()
find_package(nanobind CONFIG REQUIRED)

# For PyPI distribution, we expect GridLAB-D to be pre-built
# The build process should ensure libgldapi.so exists before calling this

# Determine paths based on build context
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../build/lib/libgldapi.so")
    # Development mode - use pre-built GridLAB-D from parent directory
    set(GRIDLABD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
    set(GRIDLABD_LIB_DIR ${GRIDLABD_ROOT}/build/lib)
    message(STATUS "Development mode: using pre-built GridLAB-D at ${GRIDLABD_ROOT}")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/lib/libgldapi.so")
    # PyPI mode - use prebuilt libraries included in the package
    set(GRIDLABD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
    set(GRIDLABD_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/lib)
    message(STATUS "PyPI mode: using prebuilt GridLAB-D libraries")
else()
    message(FATAL_ERROR "GridLAB-D library not found. Please build GridLAB-D first or include prebuilt libraries.")
endif()

# Set include directories based on the build mode
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../build/lib/libgldapi.so")
    # Development mode paths
    set(GRIDLABD_INCLUDE_DIR ${GRIDLABD_ROOT}/gldcore)
else()
    # PyPI mode paths (using copied headers)
    set(GRIDLABD_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/gldcore)
endif()

# Verify GridLAB-D library exists (should be pre-built for PyPI)

# Define HAVE_CONFIG_H like main build does
add_definitions(-DHAVE_CONFIG_H)
if(NOT EXISTS ${GRIDLABD_LIB_DIR}/libgldapi.so)
    message(FATAL_ERROR "GridLAB-D API library not found at ${GRIDLABD_LIB_DIR}/libgldapi.so. Please build GridLAB-D first or ensure prebuilt libraries are included.")

endif()

message(STATUS "Found GridLAB-D API library at ${GRIDLABD_LIB_DIR}/libgldapi.${LIB_EXT}")

# Create the Python module
nanobind_add_module(
    gridlabd_core
    src_cpp/gld_nanobind.cpp
)

# Add include directories
target_include_directories(gridlabd_core PRIVATE 
    ${GRIDLABD_ROOT}/build/headers
    ${GRIDLABD_INCLUDE_DIR}
    ${GRIDLABD_ROOT}/third_party
    ${json_SOURCE_DIR}/include
)


# Link against GridLAB-D API library and dependencies
target_link_libraries(gridlabd_core PRIVATE 
    ${GRIDLABD_LIB_DIR}/libgldapi.so
)

# Set RPATH for the module to find the shared library
set_target_properties(gridlabd_core PROPERTIES
    INSTALL_RPATH "$ORIGIN"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# Install the module
install(TARGETS gridlabd_core LIBRARY DESTINATION gridlabd)

# Install the GridLAB-D shared library alongside the Python module
install(FILES ${GRIDLABD_LIB_DIR}/libgldapi.${LIB_EXT}
        DESTINATION gridlabd)

# Install essential data files (tzinfo.txt, unitfile.txt) alongside the Python package
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../build/lib/libgldapi.so")
    # Development mode - use original locations
    set(GRIDLABD_DATA_FILES
        ${GRIDLABD_ROOT}/gldcore/tzinfo.txt
        ${GRIDLABD_ROOT}/gldcore/unitfile.txt
    )
else()
    # PyPI mode - use prebuilt locations
    set(GRIDLABD_DATA_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/share/tzinfo.txt
        ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/share/unitfile.txt
    )
endif()

install(FILES ${GRIDLABD_DATA_FILES}
        DESTINATION gridlabd/share)

# Install dummy bin/gridlabd so set_install_root() can work automatically
# The C++ code expects bin/gridlabd to exist for proper path resolution
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/bin/gridlabd")
    install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/bin/gridlabd
            DESTINATION gridlabd/bin)
endif()

# Install all GridLAB-D module libraries (.so files) so users can load modules
# These are needed for simulations (residential, climate, powerflow, etc.)
file(GLOB GRIDLABD_MODULES "${GRIDLABD_LIB_DIR}/*.so")
if(GRIDLABD_MODULES)
    install(PROGRAMS ${GRIDLABD_MODULES}
            DESTINATION gridlabd/lib)
endif()


# Create a custom target to fix RPATHs after installation
if(UNIX AND NOT APPLE)
    install(CODE "
        message(STATUS \"Fixing RPATH for all .so files...\")
        file(GLOB_RECURSE SO_FILES \"\${CMAKE_INSTALL_PREFIX}/gridlabd/*.so\")
        foreach(SO_FILE IN LISTS SO_FILES)
            get_filename_component(SO_NAME \"\${SO_FILE}\" NAME)
            execute_process(
                COMMAND patchelf --set-rpath \"\$ORIGIN:\$ORIGIN/..:\" \"\${SO_FILE}\"
                RESULT_VARIABLE PATCHELF_RESULT
                ERROR_VARIABLE PATCHELF_ERROR
            )
            if(PATCHELF_RESULT EQUAL 0)
                message(STATUS \"  Fixed RPATH for \${SO_NAME}\")
            else()
                message(WARNING \"  Failed to fix RPATH for \${SO_NAME}: \${PATCHELF_ERROR}\")
            endif()
        endforeach()
        message(STATUS \"RPATH fixes complete\")
    ")
endif()

# macOS-specific RPATH handling
if(APPLE)
    install(CODE "
        message(STATUS \"Fixing install names for all .dylib files...\")
        file(GLOB_RECURSE DYLIB_FILES \"\${CMAKE_INSTALL_PREFIX}/gridlabd/*.dylib\")
        foreach(DYLIB_FILE IN LISTS DYLIB_FILES)
            get_filename_component(DYLIB_NAME \"\${DYLIB_FILE}\" NAME)
            execute_process(
                COMMAND install_name_tool -add_rpath @loader_path \"\${DYLIB_FILE}\"
                RESULT_VARIABLE INSTALL_NAME_RESULT
                ERROR_VARIABLE INSTALL_NAME_ERROR
            )
            if(INSTALL_NAME_RESULT EQUAL 0)
                message(STATUS \"  Fixed install name for \${DYLIB_NAME}\")
            else()
                message(WARNING \"  Failed to fix install name for \${DYLIB_NAME}: \${INSTALL_NAME_ERROR}\")
            endif()
        endforeach()
        message(STATUS \"Install name fixes complete\")
    ")
endif()
