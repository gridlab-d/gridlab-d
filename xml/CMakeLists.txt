cmake_minimum_required(VERSION 3.10)
project(xml)
set(CMAKE_CXX_STANDARD 11)

include_directories(../gldcore)

add_library(xml SHARED
        loader/Xerces_loader.cpp
)

set_target_properties(xml
        PROPERTIES PREFIX ""
        )

install(TARGETS xml
        LIBRARY DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        )