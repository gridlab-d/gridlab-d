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
set(GRIDLABD_BUILD_DIR ${GRIDLABD_ROOT}/build)
set(GRIDLABD_LIB_DIR ${GRIDLABD_BUILD_DIR}/lib)
set(GRIDLABD_INCLUDE_DIR ${GRIDLABD_ROOT}/gldcore)
set(JSONCPP_INCLUDE_DIR ${GRIDLABD_ROOT}/third_party/jsoncpp_lib/include)

# Platform-specific library extension
if(APPLE)
    set(LIB_EXT "dylib")
else()
    set(LIB_EXT "so")
endif()

# Check if GridLAB-D library exists, if not build it
if(NOT EXISTS ${GRIDLABD_LIB_DIR}/libgldapi.${LIB_EXT})
    message(STATUS "GridLAB-D library not found at ${GRIDLABD_LIB_DIR}/libgldapi.${LIB_EXT}, building it first...")
    
    # Create build directory if it doesn't exist
    file(MAKE_DIRECTORY ${GRIDLABD_BUILD_DIR})
    
    # Configure GridLAB-D with CMake
    message(STATUS "Configuring GridLAB-D from ${GRIDLABD_ROOT}...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release ..
        WORKING_DIRECTORY ${GRIDLABD_BUILD_DIR}
        RESULT_VARIABLE CONFIGURE_RESULT
        OUTPUT_VARIABLE CONFIGURE_OUTPUT
        ERROR_VARIABLE CONFIGURE_ERROR
        ECHO_OUTPUT_VARIABLE
        ECHO_ERROR_VARIABLE
    )
    
    if(NOT CONFIGURE_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to configure GridLAB-D:\nOutput: ${CONFIGURE_OUTPUT}\nError: ${CONFIGURE_ERROR}")
    endif()
    
    # Build GridLAB-D
    message(STATUS "Building GridLAB-D...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} --build . --parallel
        WORKING_DIRECTORY ${GRIDLABD_BUILD_DIR}
        RESULT_VARIABLE BUILD_RESULT
        OUTPUT_VARIABLE BUILD_OUTPUT
        ERROR_VARIABLE BUILD_ERROR
        ECHO_OUTPUT_VARIABLE
        ECHO_ERROR_VARIABLE
    )
    
    if(NOT BUILD_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to build GridLAB-D:\nOutput: ${BUILD_OUTPUT}\nError: ${BUILD_ERROR}")
    endif()
    
    message(STATUS "GridLAB-D built successfully")
endif()

# Verify library exists after build
if(NOT EXISTS ${GRIDLABD_LIB_DIR}/libgldapi.${LIB_EXT})
    message(FATAL_ERROR "GridLAB-D API library still not found at ${GRIDLABD_LIB_DIR}/libgldapi.${LIB_EXT} after build attempt.")
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
target_link_libraries(gridlabd_core PRIVATE 
    ${GRIDLABD_LIB_DIR}/libgldapi.${LIB_EXT}
    ${JSONCPP_LIB_PATH}
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

# Install GridLAB-D data files
install(DIRECTORY ${GRIDLABD_BUILD_DIR}/share/
        DESTINATION gridlabd/share
        PATTERN "*.txt")

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
