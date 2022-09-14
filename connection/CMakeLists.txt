project(connection)

if (GLD_USE_FNCS)
    find_library(GLD_ZMQ_LIBRARY NAMES zmq REQUIRED PATHS ${GLD_ZeroMQ_DIR} $ENV{GLD_ZeroMQ_DIR})
    find_library(GLD_CZMQ_LIBRARY NAMES czmq REQUIRED PATHS ${GLD_CZMQ_DIR} $ENV{GLD_CZMQ_DIR})
    find_library(GLD_FNCS_LIBRARY NAMES fncs REQUIRED PATHS ${GLD_FNCS_DIR} $ENV{GLD_FNCS_DIR})

    set(GLD_FNCS_LIBRARIES
            ${GLD_FNCS_LIBRARY}
            ${GLD_CZMQ_LIBRARY}
            ${GLD_ZMQ_LIBRARY}
            )
    if (GLD_ZMQ_LIBRARY AND GLD_CZMQ_LIBRARY AND GLD_FNCS_LIBRARY)
        option(FNCS_FOUND "FNCS library found" ON)
        set(FNCS_CONNECTION fncs_msg.cpp fncs_msg.h)
    else ()
        option(FNCS_FOUND "FNCS library found" OFF)
        set(FNCS_CONNECTION)
        message(WARNING "One of the required FNCS libraries could not be located. Check your configuration for "
                "GLD_ZeroMQ_DIR, "
                "GLD_CZMQ_DIR, "
                "and GLD_FNCS_DIR.")
    endif ()
else ()
    set(FNCS_CONNECTION)
endif ()

if (GLD_USE_HELICS)
    FIND_PACKAGE(HELICS 3 REQUIRED CONFIG HINTS ${GLD_HELICS_DIR} ${HELICS_DIR} $ENV{GLD_HELICS_DIR} $ENV{HELICS_DIR})
    set(HELICS_CONNECTION helics_msg.cpp helics_msg.h)
else ()
    set(HELICS_CONNECTION)
endif ()

add_library(${PROJECT_NAME}
        connection.cpp
        connection.h
        socket.cpp
        socket.h
        server.cpp
        server.h
        cache.cpp
        cache.h
        client.cpp
        client.h
        native.cpp
        native.h
        tcp.cpp
        tcp.h
        udp.cpp
        udp.h
        xml.cpp
        xml.h
        json.cpp
        json.h
        transport.cpp
        transport.h
        message.h
        varmap.cpp
        varmap.h
        init.cpp
        main.cpp
        ${FNCS_CONNECTION}
        ${HELICS_CONNECTION}
        )

if (FNCS_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE
            ${GLD_FNCS_LIBRARIES}
            )
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_FNCS)
endif ()

if (HELICS_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE HELICS::helics)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_HELICS)
endif ()

target_include_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_SOURCE_DIR}/third_party/jsoncpp/include"
        )

target_link_libraries(${PROJECT_NAME} PRIVATE jsoncpp_lib)

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/gldcore")
target_link_libraries(${PROJECT_NAME} PRIVATE ${OS_SPECIFIC_LIBRARIES})

install(TARGETS ${PROJECT_NAME}
        ${FILE_PERMISSIONS}
        RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT Libraries)

if (GLD_USE_CLANG_TIDY)
    set_target_properties(${PROJECT_NAME} PROPERTIES
            CXX_CLANG_TIDY "${CMAKE_CXX_CLANG_TIDY}"
            )
endif ()
