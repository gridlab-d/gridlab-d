project(tape_plot)

include_directories(../gldcore)

add_library(${PROJECT_NAME} SHARED
        tape_plot/tape_plot.cpp
        tape_plot/tape_plot.h
        )

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )

#install(TARGETS ${PROJECT_NAME}
#        LIBRARY DESTINATION ${CMAKE_LIBRARY_PATH}
#        )