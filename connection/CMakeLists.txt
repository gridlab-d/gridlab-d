project(connection)

if (GLD_USE_HELICS)
    FIND_PACKAGE(HELICS 3 REQUIRED CONFIG HINTS ${HELICS_DIR} $ENV{HELICS_DIR})
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
        ${HELICS_CONNECTION}
        )

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
