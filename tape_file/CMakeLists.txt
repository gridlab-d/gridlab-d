project(tape_file)

include_directories(../gldcore)

add_library(${PROJECT_NAME} SHARED
        tape_file/tape_file.cpp
        tape_file/tape_file.h
)

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )

target_link_libraries(${PROJECT_NAME} ${OS_SPECIFIC_LIBRARIES})
