SET(GLD_MODULE_NAME mysql)

if (GLD_USE_MYSQL)
    find_package(MySQL REQUIRED HINTS ${GLD_MYSQL_DIR} ${MYSQL_DIR} $ENV{GLD_MYSQL_DIR} $ENV{MYSQL_DIR})
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

install(TARGETS ${GLD_MODULE_NAME}
        ${FILE_PERMISSIONS}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static
        COMPONENT Libraries)

if (GLD_USE_CLANG_TIDY)
    set_target_properties(${GLD_MODULE_NAME} PROPERTIES
            CXX_CLANG_TIDY "${CMAKE_CXX_CLANG_TIDY}"
            )
endif ()