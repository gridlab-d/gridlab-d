cmake_minimum_required(VERSION 3.10)
project(mysql)
set(CMAKE_CXX_STANDARD 11)

include_directories(../gldcore)

add_library(${PROJECT_NAME} SHARED
        mysql/collector.cpp
        mysql/collector.h
        mysql/database.cpp
        mysql/database.h
        mysql/init.cpp
        mysql/main.cpp
        mysql/player.cpp
        mysql/player.h
        mysql/recorder.cpp
        mysql/recorder.h
)

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )

#install(TARGETS ${PROJECT_NAME}
#        LIBRARY DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
#        )