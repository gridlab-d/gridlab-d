cmake_minimum_required(VERSION 3.10)
project(mysql)
set(CMAKE_CXX_STANDARD 11)

include_directories(../gldcore)

add_library(mysql SHARED
        collector.cpp
        collector.h
        database.cpp
        database.h
        init.cpp
        main.cpp
        player.cpp
        player.h
        recorder.cpp
        recorder.h
)

set_target_properties(mysql
        PROPERTIES PREFIX ""
        )

install(TARGETS mysql
        LIBRARY DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        )