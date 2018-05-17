project(xml)

include_directories(../gldcore)

add_library(${PROJECT_NAME} SHARED
        loader/Xerces_loader.cpp
)

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )

#install(TARGETS ${PROJECT_NAME}
#        LIBRARY DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
#        )