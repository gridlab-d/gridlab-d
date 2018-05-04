cmake_minimum_required(VERSION 3.10)
project(connection)
set(CMAKE_CXX_STANDARD 11)

include_directories(../gldcore)

add_library(connection SHARED
        init.cpp
        jsmn.c
        jsmn.h
        json.cpp
        json.h
        main.cpp
        message.h
        native.cpp
        native.h
        server.cpp
        server.h
        socket.cpp
        socket.h
        tcp.cpp
        tcp.h
        transport.cpp
        transport.h
        udp.cpp
        udp.h
        varmap.cpp
        varmap.h
        xml.cpp
        xml.h
        )

set_target_properties(connection
        PROPERTIES PREFIX ""
        )

install(TARGETS connection
        LIBRARY DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        )