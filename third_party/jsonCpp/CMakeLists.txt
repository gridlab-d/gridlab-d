project(libjsoncpp)

add_library(${PROJECT_NAME} STATIC #SHARED
        third_party/jsonCpp/jsoncpp.cpp
        third_party/jsonCpp/json/json.h
        third_party/jsonCpp/json/json-forwards.h
        )

set_target_properties(${PROJECT_NAME}
        PROPERTIES
        PREFIX ""
        POSITION_INDEPENDENT_CODE ON
        )

install(TARGETS ${PROJECT_NAME}
        PERMISSIONS
        OWNER_EXECUTE OWNER_READ OWNER_WRITE
        GROUP_EXECUTE GROUP_READ
        WORLD_EXECUTE WORLD_READ
        LIBRARY DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}
        ARCHIVE DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}
        )
