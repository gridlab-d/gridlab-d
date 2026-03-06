cmake_minimum_required(VERSION 3.13)
project(gldcore LANGUAGES C CXX)

SET(GLD_MODULE_NAME gridlabd)

# ---------------------------------------------------------------------------
# NOTE: We build the GridLAB-D core once as a SHARED library (gldapi),
# and keep the host executable (gridlabd) thin (only main.cpp). This
# avoids duplicate class registry/typeinfo across DSO boundaries and
# fixes the macOS ARM64 plugin type-identity mismatch.
# ---------------------------------------------------------------------------

# If the top-level adds definitions/visiblity, we inherit them automatically.
# (e.g., CMAKE_CXX_VISIBILITY_PRESET=default, VISIBILITY_INLINES_HIDDEN=OFF)

# --- Core sources ------------------------------------------------------------
# Load and link C/C++ files for the executable
set(SOURCE_LIST
        gridlabd.h
	    gldapi.h
	    gldapi.cpp
        globals.cpp
        globals.h
        object.cpp
        object.h
        schedule.cpp
        schedule.h
        aggregate.cpp
        aggregate.h
        class.cpp
        class.h
        cmdarg.cpp
        cmdarg.h
        #        cmex.c
        #        cmex.h
        compare.cpp
        compare.h
        gld_complex.h
        console.h
        convert.cpp
        convert.h
        cpp_threadpool.cpp
        cpp_threadpool.h
        debug.cpp
        debug.h
        deltamode.cpp
        deltamode.h
        enduse.cpp
        enduse.h
        environment.cpp
        environment.h
        exception.cpp
        exception.h
        exec.cpp
        exec.h
        find.cpp
        find.h
        gld_sock.h
        gui.cpp
        gui.h
        http_client.cpp
        http_client.h
        index.cpp
        index.h
        instance.cpp
        instance.h
        instance_cnx.cpp
        instance_cnx.h
        instance_slave.cpp
        instance_slave.h
        interpolate.cpp
        interpolate.h
        job.cpp
        job.h
        kill.cpp
        kill.h
        kml.cpp
        kml.h
        legal.cpp
        legal.h
        link.cpp
        link.h
        linkage.cpp
        linkage.h
        list.cpp
        list.h
        load.cpp
        load.h
        loader.cpp
        loader.h
        load_xml.cpp
        load_xml.h
        load_xml_handle.cpp
        load_xml_handle.h
        loadshape.cpp
        loadshape.h
        local.cpp
        local.h
        lock.cpp
        lock.h
        # main.cpp
        match.cpp
        match.h
        matlab.cpp
        matlab.h
        module.cpp
        module.h
        output.cpp
        output.h
        parser.cpp
        parser.h
        platform.h
        property.cpp
        property.h
        random.cpp
        gldrandom.h
        realtime.cpp
        realtime.h
        sanitize.cpp
        sanitize.h
        save.cpp
        save.h
        server.cpp
        server.h
        setup.cpp
        setup.h
        stream.cpp
        stream.h
        stream_type.h
        test.cpp
        test.h
        test_callbacks.h
        test_framework.cpp
        test_framework.h
        threadpool.cpp
        threadpool.h
        timestamp.cpp
        timestamp.h
        transform.cpp
        transform.h
        ufile.cpp
        ufile.h
        unit.cpp
        unit.h
        validate.cpp
        validate.h
        version.cpp
        #        xcore.cpp
        #        xcore.h
        )



# Create library source list (exclude main.cpp)
set(LIBRARY_SOURCE_LIST ${SOURCE_LIST})
list(REMOVE_ITEM LIBRARY_SOURCE_LIST main.cpp)

# Build the single shared core (SHARED library) from library sources
add_library(gldapi SHARED ${LIBRARY_SOURCE_LIST})
 

# globals.cpp carries core singletons/registry; define only for this TU
set_source_files_properties(globals.cpp PROPERTIES COMPILE_DEFINITIONS _MAIN_C)

# Optional: PIC & visibility (MSVC ignores visibility)
set_target_properties(gldapi PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  CXX_VISIBILITY_PRESET default
  VISIBILITY_INLINES_HIDDEN OFF
)


target_link_libraries(gldapi PUBLIC nlohmann_json::nlohmann_json)


# If Xerces/Curses/etc. are found, link them here (keep existing blocks if you have them)
if (XercesC_FOUND)
  target_link_libraries(gldapi PRIVATE ${XercesC_LIBRARIES})
endif ()
if (UNIX AND CURSES_FOUND)
  target_link_libraries(gldapi PRIVATE ${CURSES_LIBRARIES})
endif ()

# Windows: export all symbols for DLL (preserve existing MSVC behavior)
if (MSVC)
  set_target_properties(gldapi PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif ()

# --- Thin host executable ----------------------------------------------------
# The host must compile only main.cpp and link to gldapi
add_executable(gridlabd main.cpp)
target_link_libraries(gridlabd PRIVATE gldapi Threads::Threads)

# Keep any platform-specific flags you already use
if (MSVC)
  # Example: stack size & debug flags (if you use them here)
  target_link_options(gridlabd PRIVATE /STACK:16777216)
  target_compile_options(gridlabd PRIVATE $<$<CONFIG:Debug>:/Zi /Od>)
else()
  target_compile_options(gridlabd PRIVATE $<$<CONFIG:Debug>:-g -O0>)
endif()

# --- Portable RPATH on UNIX (macOS/Linux) -----------------------------------
# Use the helper function defined at the top-level CMakeLists.txt
if (COMMAND set_portable_rpath)
  set_portable_rpath(gldapi)
  set_portable_rpath(gridlabd)
endif()

# --- Safety check (prevents future duplicate-core regression) ---------------
# Fail if someone accidentally adds more sources to the executable
get_target_property(_exe_srcs gridlabd SOURCES)
list(LENGTH _exe_srcs _exe_count)
if(_exe_count GREATER 1)
  message(FATAL_ERROR "gridlabd must be a thin host (only main.cpp); core lives in gldapi.")
endif()


