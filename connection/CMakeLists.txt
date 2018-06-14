project(connection)

include_directories(../gldcore)

include_directories(../third_party/jsonCpp)
#SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I../third_party/jsonCpp -lwsock32 -lws2_32" )


add_library(${PROJECT_NAME} SHARED
        connection/init.cpp
        connection/jsmn.c
        connection/jsmn.h
        connection/json.cpp
        connection/json.h
        connection/main.cpp
        connection/message.h
        connection/native.cpp
        connection/native.h
        connection/server.cpp
        connection/server.h
        connection/socket.cpp
        connection/socket.h
        connection/tcp.cpp
        connection/tcp.h
        connection/transport.cpp
        connection/transport.h
        connection/udp.cpp
        connection/udp.h
        connection/varmap.cpp
        connection/varmap.h
        connection/xml.cpp
        connection/xml.h

        connection/connection.cpp
        connection/connection.h
        connection/cache.cpp
        connection/cache.h
        connection/client.cpp
        connection/client.h
        )

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )
add_dependencies(${PROJECT_NAME} libjsoncpp)
target_link_libraries(${PROJECT_NAME} libjsoncpp)

target_link_libraries(${PROJECT_NAME} ${OS_SPECIFIC_LIBRARIES})
