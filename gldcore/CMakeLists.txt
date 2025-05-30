SET(GLD_MODULE_NAME gridlabd)

# Load and link C/C++ files for the executeable
set(SOURCE_LIST
        gridlabd.h
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
        main.cpp
        match.cpp
        match.h
        matlab.cpp
        matlab.h
        module.cpp
        module.h
        output.cpp
        output.h
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

#if(NOT APPLE)
add_executable(${GLD_MODULE_NAME} ${SOURCE_LIST})
#else()
#    add_executable(${GLD_MODULE_NAME} MACOSX_BUNDLE ${SOURCE_LIST})
#    set_target_properties(${GLD_MODULE_NAME} PROPERTIES
#            BUNDLE True
#            MACOSX_BUNDLE_GUI_IDENTIFIER gov.pnnl.gridlabd
#            MACOSX_BUNDLE_NAME ${GLD_MODULE_NAME}
#            )
#endif()

target_link_libraries(${GLD_MODULE_NAME} PUBLIC ${GL_MODULES})
target_link_libraries(${GLD_MODULE_NAME} PRIVATE ${CMAKE_THREAD_LIBS_INIT})
if (CURSES_FOUND)
    target_link_libraries(${GLD_MODULE_NAME} PRIVATE ${CURSES_LIBRARIES})
endif ()
if (XercesC_FOUND)
    target_link_libraries(${GLD_MODULE_NAME} PRIVATE ${XercesC_LIBRARIES})
endif ()

target_include_directories(${GLD_MODULE_NAME} PRIVATE
        "${CMAKE_SOURCE_DIR}/third_party/jsonCpp"
        )
target_link_libraries(${GLD_MODULE_NAME} PRIVATE ${JSONCPP_LIB})

target_compile_options(${GLD_MODULE_NAME} PRIVATE ${GLD_COMPILE_OPTIONS})
set_target_properties(${GLD_MODULE_NAME} PROPERTIES PREFIX "")
target_link_libraries(${GLD_MODULE_NAME} PRIVATE ${OS_SPECIFIC_LIBRARIES})
target_include_directories(${GLD_MODULE_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/headers)

install(TARGETS ${GLD_MODULE_NAME}
        ${FILE_PERMISSIONS}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT Runtime
        )

if (GLD_USE_CLANG_TIDY)
    set_target_properties(${GLD_MODULE_NAME} PROPERTIES
            CXX_CLANG_TIDY "${CMAKE_CXX_CLANG_TIDY}"
            )
endif ()
