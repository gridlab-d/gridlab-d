cmake_minimum_required(VERSION 3.10)
project(generators)
set(CMAKE_CXX_STANDARD 11)

include_directories(../gldcore)

add_library(generators SHARED
        battery.cpp
        battery.h
        central_dg_control.cpp
        central_dg_control.h
        dc_dc_converter.cpp
        dc_dc_converter.h
        diesel_dg.cpp
        diesel_dg.h
        energy_storage.cpp
        energy_storage.h
        generators.h
        init.cpp
        inverter.cpp
        inverter.h
        main.cpp
        microturbine.cpp
        microturbine.h
        power_electronics.cpp
        power_electronics.h
        rectifier.cpp
        rectifier.h
        solar.cpp
        solar.h
        windturb_dg.cpp
        windturb_dg.h
)

set_target_properties(generators
        PROPERTIES PREFIX ""
        )

install(TARGETS generators
        LIBRARY DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        )