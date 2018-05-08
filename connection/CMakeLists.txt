cmake_minimum_required(VERSION 3.10)
project(connection)
set(CMAKE_CXX_STANDARD 11)

include_directories(../gldcore)

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
        )

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )

#install(TARGETS ${PROJECT_NAME}
#        LIBRARY DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
#        )