project(climate)

include_directories(../gldcore)

add_library(${PROJECT_NAME} SHARED
        climate/climate.cpp
        climate/climate.h
        climate/csv_reader.cpp
        climate/csv_reader.h
        climate/init.cpp
        climate/main.cpp
        climate/solar_angles.cpp
        climate/solar_angles.h
        climate/test.cpp
        climate/test.h
        climate/weather.cpp
        climate/weather.h
        climate/weather_reader.cpp
        climate/weather_reader.h
        )

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )

target_link_libraries(${PROJECT_NAME} ${OS_SPECIFIC_LIBRARIES})
