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
    set(JSONCPP_INCLUDE_DIR ${GRIDLABD_ROOT}/third_party/jsoncpp_lib/include)
else()
    # PyPI mode paths (using copied headers)
    set(GRIDLABD_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/gldcore)
    set(JSONCPP_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/jsoncpp_lib/include)
endif()

# Verify GridLAB-D library exists (should be pre-built for PyPI)
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
    ${GRIDLABD_INCLUDE_DIR}
    ${GRIDLABD_ROOT}/third_party
    ${JSONCPP_INCLUDE_DIR}
)

# Find jsoncpp library (could be in different locations)
set(JSONCPP_SEARCH_PATHS
    ${GRIDLABD_LIB_DIR}/static/libjsoncpp.a
    ${GRIDLABD_BUILD_DIR}/third_party/jsoncpp_lib/src/lib_json/libjsoncpp.a
    ${GRIDLABD_BUILD_DIR}/lib/libjsoncpp.a
)

set(JSONCPP_LIB_PATH "")
foreach(SEARCH_PATH ${JSONCPP_SEARCH_PATHS})
    if(EXISTS ${SEARCH_PATH})
        set(JSONCPP_LIB_PATH ${SEARCH_PATH})
        message(STATUS "Found jsoncpp at: ${JSONCPP_LIB_PATH}")
        break()
    endif()
endforeach()

if(NOT JSONCPP_LIB_PATH)
    message(FATAL_ERROR "Could not find libjsoncpp.a. Searched in: ${JSONCPP_SEARCH_PATHS}")
endif()

# Link against GridLAB-D API library and dependencies
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../build/lib/libgldapi.so")
    # Development mode - use build directory
    target_link_libraries(gridlabd_core PRIVATE 
        ${GRIDLABD_LIB_DIR}/libgldapi.so
        ${GRIDLABD_LIB_DIR}/static/libjsoncpp.a
    )
else()
    # PyPI mode - use prebuilt libraries
    target_link_libraries(gridlabd_core PRIVATE 
        ${GRIDLABD_LIB_DIR}/libgldapi.so
        ${GRIDLABD_LIB_DIR}/static/libjsoncpp.a
    )
endif()

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

# Install all GridLAB-D module libraries (.so files) so users can load modules
# These are needed for simulations (residential, climate, powerflow, etc.)
file(GLOB GRIDLABD_MODULES "${GRIDLABD_LIB_DIR}/*.so")
if(GRIDLABD_MODULES)
    install(PROGRAMS ${GRIDLABD_MODULES}
            DESTINATION gridlabd/lib)
endif()

# Also create a modules directory as an alias (GridLAB-D may look in multiple places)
install(CODE "file(MAKE_DIRECTORY \${CMAKE_INSTALL_PREFIX}/gridlabd/lib)")

# Install all GridLAB-D module libraries
file(GLOB MODULE_LIBS "${GRIDLABD_LIB_DIR}/*.${LIB_EXT}")
install(FILES ${MODULE_LIBS}
        DESTINATION gridlabd/lib)

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
