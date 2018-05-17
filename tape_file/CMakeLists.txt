project(tape_file)

include_directories(../gldcore)

add_library(${PROJECT_NAME} SHARED
        tape_file/tape_file.cpp
        tape_file/tape_file.h
)

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )

#install(TARGETS ${PROJECT_NAME}
#        LIBRARY DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
#        )