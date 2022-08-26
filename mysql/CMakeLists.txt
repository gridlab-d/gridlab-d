project(mysql)

if (GLD_USE_MYSQL)
    find_package(MySQL REQUIRED HINTS ${GLD_MYSQL_DIR} ${MYSQL_DIR} $ENV{GLD_MYSQL_DIR} $ENV{MYSQL_DIR})
endif ()

add_library(${PROJECT_NAME}
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
    target_link_libraries(${PROJECT_NAME} PRIVATE ${MYSQL_LIBRARY})
endif ()

if (HAVE_MYSQL)
    target_include_directories(${PROJECT_NAME} PRIVATE
            ${MYSQL_INCLUDE_DIRECTORIES}
            ${MYSQL_INCLUDE_DIRECTORIES}/include
            )
    target_link_libraries(${PROJECT_NAME}
            ${MYSQL_LIBRARY}
            )
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_HELICS)
endif ()

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/gldcore")
target_link_libraries(${PROJECT_NAME} PRIVATE ${OS_SPECIFIC_LIBRARIES})

install(TARGETS ${PROJECT_NAME}
        ${FILE_PERMISSIONS}
        RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT Libraries)

if (GLD_USE_CLANG_TIDY)
    set_target_properties(${PROJECT_NAME} PROPERTIES
            CXX_CLANG_TIDY "${CMAKE_CXX_CLANG_TIDY}"
            )
endif ()