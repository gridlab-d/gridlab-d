project(mysql)

include_directories(../${GLDCORE_LOC})
IF (HAVE_MYSQL)
    include_directories(${MYSQL_INCLUDE_DIRECTORIES} ${MYSQL_INCLUDE_DIRECTORIES}/include)
ENDIF ()

#TODO: MySQL imports

add_library(${PROJECT_NAME}
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
        mysql/query_engine.h
        mysql/table_manager.cpp
        mysql/query_engine.cpp
        mysql/group_recorder.cpp
        mysql/group_recorder.h
        )

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )

target_link_libraries(${PROJECT_NAME} ${OS_SPECIFIC_LIBRARIES})

IF (HAVE_MYSQL)
    target_link_libraries(${PROJECT_NAME} ${MYSQL_LIBRARY})
ENDIF ()