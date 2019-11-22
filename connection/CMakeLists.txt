project(connection)

include_directories(../${GLDCORE_LOC})

include_directories(../third_party/jsonCpp)
#SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I../third_party/jsonCpp -lwsock32 -lws2_32" )

IF (HAVE_FNCS)
    SET(FNCS_CONNECTION "connection/fncs_msg.cpp connection/fncs_msg.h")
ELSE ()
    SET(FNCS_CONNECTION "")
ENDIF ()

IF (HAVE_HELICS)
    SET(HELICS_CONNECTION "helics_msg.cpp helics_msg.h")
ELSE ()
    SET(HELICS_CONNECTION "")
ENDIF ()

add_library(${PROJECT_NAME}
        connection/connection.cpp
        connection/connection.h
        connection/socket.cpp
        connection/socket.h
        connection/server.cpp
        connection/server.h
        connection/cache.cpp
        connection/cache.h
        connection/client.cpp
        connection/client.h
        connection/native.cpp
        connection/native.h
        connection/tcp.cpp
        connection/tcp.h
        connection/udp.cpp
        connection/udp.h
        connection/xml.cpp
        connection/xml.h
        connection/json.cpp
        connection/json.h

        ${FNCS_CONNECTION}
		${HELICS_CONNECTION}

        connection/transport.cpp
        connection/transport.h
        connection/message.h
        connection/varmap.cpp
        connection/varmap.h
        connection/init.cpp
        connection/main.cpp
        )

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )
add_dependencies(${PROJECT_NAME} libjsoncpp)
target_link_libraries(${PROJECT_NAME} libjsoncpp)

target_link_libraries(${PROJECT_NAME} ${OS_SPECIFIC_LIBRARIES})
