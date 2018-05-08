cmake_minimum_required(VERSION 3.10)
project(commercial)
set(CMAKE_CXX_STANDARD 11)

include_directories(../gldcore)

add_library(${PROJECT_NAME} SHARED
        commercial/hvac.cpp
        commercial/hvac.h
        commercial/init.cpp
        commercial/main.cpp
        commercial/multizone.cpp
        commercial/multizone.h
        commercial/office.cpp
        commercial/office.h
        commercial/solvers.cpp
        commercial/solvers.h
        )

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )

#install(TARGETS ${PROJECT_NAME}
#LIBRARY DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
#        )