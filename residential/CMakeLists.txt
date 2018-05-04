cmake_minimum_required(VERSION 3.10)
project(residential)
set(CMAKE_CXX_STANDARD 11)

include_directories(../gldcore)

add_library(residential SHARED
        appliance.cpp
        appliance.h
        clotheswasher.cpp
        clotheswasher.h
        dishwasher.cpp
        dishwasher.h
        dryer.cpp
        dryer.h
        elcap1990.h
        elcap2010.h
        evcharger.cpp
        evcharger.h
        evcharger_det.cpp
        evcharger_det.h
        freezer.cpp
        freezer.h
        house_a.cpp
        house_a.h
        house_e.cpp
        house_e.h
        init.cpp
        lights.cpp
        lights.h
        main.cpp
        microwave.cpp
        microwave.h
        occupantload.cpp
        occupantload.h
        plugload.cpp
        plugload.h
        range.cpp
        range.h
        rbsa2014.h
        refrigerator.cpp
        refrigerator.h
        residential.h
        residential_enduse.cpp
        residential_enduse.h
        solvers.cpp
        solvers.h
        thermal_storage.cpp
        thermal_storage.h
        waterheater.cpp
        waterheater.h
        zipload.cpp
        zipload.h
)

set_target_properties(residential
        PROPERTIES PREFIX ""
        )

install(TARGETS residential
        LIBRARY DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        )