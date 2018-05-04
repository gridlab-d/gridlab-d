cmake_minimum_required(VERSION 3.10)
project(climate)
set(CMAKE_CXX_STANDARD 11)

include_directories(../gldcore)

add_library(climate SHARED
        climate.cpp
        climate.h
        csv_reader.cpp
        csv_reader.h
        init.cpp
        main.cpp
        solar_angles.cpp
        solar_angles.h
        test.cpp
        test.h
        weather.cpp
        weather.h
        weather_reader.cpp
        weather_reader.h
)

set_target_properties(climate
        PROPERTIES PREFIX ""
        )

install(TARGETS climate
        LIBRARY DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        )