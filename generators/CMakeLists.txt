project(generators)

include_directories(../${GLDCORE_LOC})

add_library(${PROJECT_NAME} SHARED
        generators/battery.cpp
        generators/battery.h
        generators/central_dg_control.cpp
        generators/central_dg_control.h
        generators/diesel_dg.cpp
        generators/diesel_dg.h
        generators/energy_storage.cpp
        generators/energy_storage.h
        generators/generators.h
        generators/init.cpp
        generators/inverter.cpp
        generators/inverter.h
        generators/main.cpp
        generators/rectifier.cpp
        generators/rectifier.h
        generators/solar.cpp
        generators/solar.h
        generators/windturb_dg.cpp
        generators/windturb_dg.h
        generators/controller_dg.cpp
        generators/controller_dg.h
)

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )
