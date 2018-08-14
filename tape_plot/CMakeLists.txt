project(tape_plot)

include_directories(../${GLDCORE_LOC})

add_library(${PROJECT_NAME} SHARED
        tape_plot/tape_plot.cpp
        tape_plot/tape_plot.h
        )

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )

target_link_libraries(${PROJECT_NAME} ${OS_SPECIFIC_LIBRARIES})
