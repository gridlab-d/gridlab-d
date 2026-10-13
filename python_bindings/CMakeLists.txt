cmake_minimum_required(VERSION 3.26)
project(gridlabd_python_bindings)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Determine library extension based on platform
if(APPLE)
    set(LIB_EXT "dylib")
elseif(WIN32)
    set(LIB_EXT "dll")
else()
    set(LIB_EXT "so")
endif()

# Shared module libraries use the same extension as other shared libs on this project.
set(MODULE_LIB_EXT ${LIB_EXT})

# Find required packages
find_package(Python 3.10 COMPONENTS Interpreter Development.Module Development.SABIModule REQUIRED)

# Find or fetch nlohmann/json
find_package(nlohmann_json 3.12.0 QUIET)
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json not found, downloading...")
    include(FetchContent)
    FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
    FetchContent_MakeAvailable(json)
endif()
find_package(nanobind 3.0 CONFIG REQUIRED)

# For PyPI distribution, we expect GridLAB-D to be pre-built
# The build process should ensure libgldapi.${LIB_EXT} exists before calling this

# Accept an explicit core build directory, including Visual Studio multi-config builds.
set(GRIDLABD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(GRIDLABD_BUILD_DIR "" CACHE PATH "Directory containing the pre-built GridLAB-D core")
set(PYPI_MODE FALSE)
if(WIN32)
    set(GRIDLABD_API_NAME "gldapi.dll")
else()
    set(GRIDLABD_API_NAME "libgldapi.${LIB_EXT}")
endif()

if(GRIDLABD_BUILD_DIR)
    set(_gld_candidates "${GRIDLABD_BUILD_DIR}")
else()
    set(_gld_candidates
        "${GRIDLABD_ROOT}/cmake-build"
        "${GRIDLABD_ROOT}/build"
        "${CMAKE_CURRENT_SOURCE_DIR}/prebuilt")
endif()
foreach(_gld_build IN LISTS _gld_candidates)
    foreach(_gld_lib IN ITEMS "lib" "bin/${CMAKE_BUILD_TYPE}" "bin/Release" "bin")
        if(EXISTS "${_gld_build}/${_gld_lib}/${GRIDLABD_API_NAME}")
            set(GRIDLABD_BUILD_DIR "${_gld_build}")
            set(GRIDLABD_LIB_DIR "${_gld_build}/${_gld_lib}")
            break()
        endif()
    endforeach()
    if(GRIDLABD_LIB_DIR)
        break()
    endif()
endforeach()
if(NOT GRIDLABD_LIB_DIR)
    message(FATAL_ERROR "GridLAB-D library not found. Build the core and set GRIDLABD_BUILD_DIR to its build directory.")
endif()
if(GRIDLABD_BUILD_DIR STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}/prebuilt")
    set(PYPI_MODE TRUE)
    set(GRIDLABD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
endif()
set(GRIDLABD_API_LIBRARY "${GRIDLABD_LIB_DIR}/${GRIDLABD_API_NAME}")
if(WIN32)
    # MSVC links against the import library, not the runtime DLL.
    # Select the import library from the same configuration as the runtime DLL.
    get_filename_component(_gld_runtime_config "${GRIDLABD_LIB_DIR}" NAME)
    if(_gld_runtime_config STREQUAL "bin")
        set(_gld_runtime_config "")
    endif()
    set(_lib_candidates
        "${GRIDLABD_BUILD_DIR}/lib/static/${_gld_runtime_config}/gldapi.lib"
        "${GRIDLABD_BUILD_DIR}/lib/static/gldapi.lib"
        "${GRIDLABD_BUILD_DIR}/lib/gldapi.lib")
    
    set(GRIDLABD_API_IMPORT_LIBRARY "")
    foreach(_lib_path IN LISTS _lib_candidates)
        if(EXISTS "${_lib_path}")
            set(GRIDLABD_API_IMPORT_LIBRARY "${_lib_path}")
            message(STATUS "Found GridLAB-D import library: ${GRIDLABD_API_IMPORT_LIBRARY}")
            break()
        endif()
    endforeach()
    
    if(NOT GRIDLABD_API_IMPORT_LIBRARY)
        message(FATAL_ERROR "GridLAB-D import library (gldapi.lib) not found in:\n${_lib_candidates}\nBuild directory: ${GRIDLABD_BUILD_DIR}")
    endif()
    
    set(GRIDLABD_API_LINK_LIBRARY "${GRIDLABD_API_IMPORT_LIBRARY}")
else()
    set(GRIDLABD_API_LINK_LIBRARY "${GRIDLABD_API_LIBRARY}")
endif()

# Set include directories based on the build mode
if(PYPI_MODE)
    # PyPI mode paths (using copied headers)
    set(GRIDLABD_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/gldcore)
    set(GRIDLABD_BUILD_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/headers)
else()
    # Development mode paths
    set(GRIDLABD_INCLUDE_DIR ${GRIDLABD_ROOT}/gldcore)
    set(GRIDLABD_BUILD_INCLUDE_DIR ${GRIDLABD_BUILD_DIR}/headers)
endif()

# Verify GridLAB-D library exists (should be pre-built for PyPI)

# Define HAVE_CONFIG_H like main build does
add_definitions(-DHAVE_CONFIG_H)
if(NOT EXISTS ${GRIDLABD_API_LIBRARY})
    message(FATAL_ERROR "GridLAB-D API library not found at ${GRIDLABD_API_LIBRARY}. Please build GridLAB-D first or ensure prebuilt libraries are included.")

endif()

message(STATUS "Found GridLAB-D API library at ${GRIDLABD_API_LIBRARY}")

# Create the Python module
nanobind_add_module(
    gridlabd_core
    src_cpp/gld_nanobind.cpp
    BACKEND_MODULE nanobind_backend
)

# Add include directories
target_include_directories(gridlabd_core PRIVATE 
    ${GRIDLABD_INCLUDE_DIR}
    ${GRIDLABD_BUILD_INCLUDE_DIR}
    ${GRIDLABD_ROOT}/third_party
)

target_compile_definitions(gridlabd_core PRIVATE HAVE_CONFIG_H)
if(DEFINED SKBUILD_PROJECT_VERSION_FULL)
    target_compile_definitions(gridlabd_core PRIVATE VERSION_INFO=${SKBUILD_PROJECT_VERSION_FULL})
endif()
if(WIN32)
    target_compile_definitions(gridlabd_core PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

# Find jsoncpp library (could be in different locations)

# Link against GridLAB-D API library and dependencies
target_link_libraries(gridlabd_core PRIVATE 
    ${GRIDLABD_API_LINK_LIBRARY}
    nlohmann_json::nlohmann_json
)

# Set RPATH for the module to find the shared library
if(APPLE)
    set(GRIDLABD_MODULE_RPATH "@loader_path")
else()
    set(GRIDLABD_MODULE_RPATH "$ORIGIN")
endif()

set_target_properties(gridlabd_core PROPERTIES
    INSTALL_RPATH "${GRIDLABD_MODULE_RPATH}"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# Install the module
install(TARGETS gridlabd_core LIBRARY DESTINATION gridlabd RUNTIME DESTINATION gridlabd)

# Install the GridLAB-D shared library alongside the Python module
install(FILES ${GRIDLABD_API_LIBRARY}
        DESTINATION gridlabd)

# Install essential data files (tzinfo.txt, unitfile.txt) alongside the Python package
if(PYPI_MODE)
    # PyPI mode - use prebuilt locations
    set(GRIDLABD_DATA_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/share/tzinfo.txt
        ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/share/unitfile.txt
    )
else()
    # Development mode - use original locations
    set(GRIDLABD_DATA_FILES
        ${GRIDLABD_ROOT}/gldcore/tzinfo.txt
        ${GRIDLABD_ROOT}/gldcore/unitfile.txt
    )
endif()

install(FILES ${GRIDLABD_DATA_FILES}
        DESTINATION gridlabd/share)

if(WIN32)
    # Bundle runtime dependencies next to the extension for Python's DLL loader.
    file(GLOB GRIDLABD_RUNTIME_DLLS "${GRIDLABD_LIB_DIR}/*.dll")
    install(FILES ${GRIDLABD_RUNTIME_DLLS} DESTINATION gridlabd)
    if(EXISTS "${GRIDLABD_LIB_DIR}/gridlabd.exe")
        install(PROGRAMS "${GRIDLABD_LIB_DIR}/gridlabd.exe" DESTINATION gridlabd/bin)
    endif()
endif()

# Install dummy bin/gridlabd so set_install_root() can work automatically
# The C++ code expects bin/gridlabd to exist for proper path resolution
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/bin/gridlabd")
    install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/bin/gridlabd
            DESTINATION gridlabd/bin)
endif()

# Install all GridLAB-D module libraries so users can load modules
# These are needed for simulations (residential, climate, powerflow, etc.)
file(GLOB GRIDLABD_MODULES "${GRIDLABD_LIB_DIR}/*.${MODULE_LIB_EXT}")
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
