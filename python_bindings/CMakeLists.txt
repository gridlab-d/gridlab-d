cmake_minimum_required(VERSION 3.15)
project(gridlabd_python_bindings)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
install(PROGRAMS ${GRIDLABD_LIB_DIR}/libgldapi.so
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

# Create bundle directories (for future use)
install(CODE "file(MAKE_DIRECTORY \${CMAKE_INSTALL_PREFIX}/gridlabd_bundle/bin)")
install(CODE "file(MAKE_DIRECTORY \${CMAKE_INSTALL_PREFIX}/gridlabd_bundle/lib)")

# Fix RPATH for all installed .so files so they can find each other at runtime
# This is critical for the wheel to work when installed via pip
if(UNIX AND NOT APPLE)
    # Find patchelf (required for fixing RPATH on Linux)
    find_program(PATCHELF_EXECUTABLE patchelf)
    if(PATCHELF_EXECUTABLE)
        message(STATUS "Found patchelf: ${PATCHELF_EXECUTABLE}")
        install(CODE "
            message(STATUS \"Fixing RPATH for all .so files...\")
            
            # Fix RPATH for libgldapi.so
            execute_process(
                COMMAND ${PATCHELF_EXECUTABLE} --set-rpath \"\\\$ORIGIN\" \"\${CMAKE_INSTALL_PREFIX}/gridlabd/libgldapi.so\"
                RESULT_VARIABLE result
            )
            if(result EQUAL 0)
                message(STATUS \"  Fixed RPATH for libgldapi.so\")
            else()
                message(WARNING \"  Failed to fix RPATH for libgldapi.so\")
            endif()
            
            # Fix RPATH for all module libraries in lib/
            file(GLOB MODULE_LIBS \"\${CMAKE_INSTALL_PREFIX}/gridlabd/lib/*.so\")
            foreach(lib \${MODULE_LIBS})
                execute_process(
                    COMMAND ${PATCHELF_EXECUTABLE} --set-rpath \"\\\$ORIGIN:\\\$ORIGIN/..\" \"\${lib}\"
                    RESULT_VARIABLE result
                )
                if(result EQUAL 0)
                    get_filename_component(lib_name \"\${lib}\" NAME)
                    message(STATUS \"  Fixed RPATH for \${lib_name}\")
                else()
                    message(WARNING \"  Failed to fix RPATH for \${lib}\")
                endif()
            endforeach()
            
            message(STATUS \"RPATH fixes complete\")
        ")
    else()
        message(WARNING "patchelf not found. RPATH will not be fixed. Install patchelf: sudo apt-get install patchelf")
    endif()
elseif(APPLE)
    # On macOS, use install_name_tool
    find_program(INSTALL_NAME_TOOL install_name_tool)
    if(INSTALL_NAME_TOOL)
        message(STATUS "Found install_name_tool: ${INSTALL_NAME_TOOL}")
        install(CODE "
            message(STATUS \"Fixing RPATH for all .so files (macOS)...\")
            
            # Fix RPATH for libgldapi.so
            execute_process(
                COMMAND ${INSTALL_NAME_TOOL} -add_rpath \"@loader_path\" \"\${CMAKE_INSTALL_PREFIX}/gridlabd/libgldapi.so\"
                RESULT_VARIABLE result
            )
            if(result EQUAL 0)
                message(STATUS \"  Fixed RPATH for libgldapi.so\")
            endif()
            
            # Fix RPATH for all module libraries
            file(GLOB MODULE_LIBS \"\${CMAKE_INSTALL_PREFIX}/gridlabd/lib/*.so\")
            foreach(lib \${MODULE_LIBS})
                execute_process(
                    COMMAND ${INSTALL_NAME_TOOL} -add_rpath \"@loader_path\" \"\${lib}\"
                    RESULT_VARIABLE result
                )
                execute_process(
                    COMMAND ${INSTALL_NAME_TOOL} -add_rpath \"@loader_path/..\" \"\${lib}\"
                    RESULT_VARIABLE result2
                )
                if(result EQUAL 0 OR result2 EQUAL 0)
                    get_filename_component(lib_name \"\${lib}\" NAME)
                    message(STATUS \"  Fixed RPATH for \${lib_name}\")
                endif()
            endforeach()
            
            message(STATUS \"RPATH fixes complete\")
        ")
    else()
        message(WARNING "install_name_tool not found")
    endif()
endif()
