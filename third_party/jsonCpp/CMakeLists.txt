project(libjsoncpp)

add_library(${PROJECT_NAME} STATIC #SHARED
        ../jsonCpp/jsoncpp.cpp
        ../jsonCpp/json/json.h
        ../jsonCpp/json/json-forwards.h
        )

set_target_properties(${PROJECT_NAME}
        PROPERTIES
        PREFIX ""
        POSITION_INDEPENDENT_CODE ON
        )

if ( CMAKE_COMPILER_IS_GNUCC )
    target_compile_options(libjsoncpp PRIVATE "-w")
endif()
if ( MSVC )
    target_compile_options(libjsoncpp PRIVATE "/w")
endif()

install(TARGETS ${PROJECT_NAME}
        PERMISSIONS
        OWNER_EXECUTE OWNER_READ OWNER_WRITE
        GROUP_EXECUTE GROUP_READ
        WORLD_EXECUTE WORLD_READ
        LIBRARY DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}
        ARCHIVE DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}
        )
