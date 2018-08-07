project(mysql)

include_directories(../${GLDCORE_LOC})

#TODO: MySQL imports

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

target_link_libraries(${PROJECT_NAME} ${OS_SPECIFIC_LIBRARIES})
