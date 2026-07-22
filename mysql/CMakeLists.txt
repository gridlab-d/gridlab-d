SET(GLD_MODULE_NAME mysql)

if (GLD_USE_MYSQL)
    find_package(MySQL QUIET HINTS ${GLD_MYSQL_DIR} ${MYSQL_DIR} $ENV{GLD_MYSQL_DIR} $ENV{MYSQL_DIR})

    if (NOT MYSQL_FOUND)
        find_package(PkgConfig QUIET)
        if (PkgConfig_FOUND)
            pkg_search_module(MYSQL_PC QUIET mysqlclient libmariadb mariadb)
        endif ()

        if (MYSQL_PC_FOUND)
            set(MYSQL_FOUND TRUE)
            set(MYSQL_INCLUDE_DIRECTORIES ${MYSQL_PC_INCLUDE_DIRS})
            set(MYSQL_LIBRARY ${MYSQL_PC_LIBRARIES})
            link_directories(${MYSQL_PC_LIBRARY_DIRS})
        endif ()
    endif ()

    if (NOT MYSQL_FOUND)
        message(FATAL_ERROR
            "MySQL not found via FindMySQL.cmake or pkg-config (mysqlclient/mariadb/libmariadb). "
            "Set GLD_MYSQL_DIR, MYSQL_DIR, or the matching environment variables, "
            "or install the MySQL/MariaDB client development package.")
    endif ()

    set(HAVE_MYSQL ${MYSQL_FOUND})
endif ()

add_library(${GLD_MODULE_NAME}
        collector.cpp
        collector.h
        database.cpp
        database.h
        init.cpp
        main.cpp
        player.cpp
        player.h
        recorder.cpp
        recorder.h
        query_engine.h
        table_manager.cpp
        query_engine.cpp
        group_recorder.cpp
        group_recorder.h
        )

if (HAVE_MYSQL)
    include_directories(${MYSQL_INCLUDE_DIRECTORIES} ${MYSQL_INCLUDE_DIRECTORIES}/include)
    target_link_libraries(${GLD_MODULE_NAME} PRIVATE ${MYSQL_LIBRARY})
endif ()

if (HAVE_MYSQL)
    target_include_directories(${GLD_MODULE_NAME} PRIVATE
            ${MYSQL_INCLUDE_DIRECTORIES}
            ${MYSQL_INCLUDE_DIRECTORIES}/include
            )
    target_link_libraries(${GLD_MODULE_NAME}
            ${MYSQL_LIBRARY}
            )
    target_compile_definitions(${GLD_MODULE_NAME} PRIVATE HAVE_HELICS)
endif ()

target_compile_options(${GLD_MODULE_NAME} PRIVATE ${GLD_COMPILE_OPTIONS})
set_target_properties(${GLD_MODULE_NAME} PROPERTIES PREFIX "")
target_include_directories(${GLD_MODULE_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/gldcore")
target_link_libraries(${GLD_MODULE_NAME} PRIVATE ${OS_SPECIFIC_LIBRARIES})
if (GLD_USE_CLANG_TIDY)
    set_target_properties(${GLD_MODULE_NAME} PROPERTIES
            CXX_CLANG_TIDY "${CMAKE_CXX_CLANG_TIDY}"
            )
endif ()
