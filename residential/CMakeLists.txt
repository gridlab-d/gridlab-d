project(residential)

include_directories(../${GLDCORE_LOC})

add_library(${PROJECT_NAME} SHARED
        residential/appliance.cpp
        residential/appliance.h
        residential/clotheswasher.cpp
        residential/clotheswasher.h
        residential/dishwasher.cpp
        residential/dishwasher.h
        residential/dryer.cpp
        residential/dryer.h
        residential/evcharger.cpp
        residential/evcharger_det.cpp
        residential/evcharger_det.h
        residential/evcharger.h
        residential/freezer.cpp
        residential/freezer.h
        residential/house_e.cpp
        residential/house_e.h
        residential/init.cpp
        residential/lights.cpp
        residential/lights.h
        residential/main.cpp
        residential/microwave.cpp
        residential/microwave.h
        residential/occupantload.cpp
        residential/occupantload.h
        residential/plugload.cpp
        residential/plugload.h
        residential/range.cpp
        residential/range.h
        residential/refrigerator.cpp
        residential/refrigerator.h
        residential/residential_enduse.cpp
        residential/residential_enduse.h
        residential/residential.h
        residential/rbsa2014.h
        residential/solvers.cpp
        residential/solvers.h
        residential/thermal_storage.cpp
        residential/thermal_storage.h
        residential/waterheater.cpp
        residential/waterheater.h
        residential/zipload.cpp
        residential/zipload.h
        residential/elcap1990.h
        residential/elcap2010.h
)

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )

target_link_libraries(${PROJECT_NAME} ${OS_SPECIFIC_LIBRARIES})
