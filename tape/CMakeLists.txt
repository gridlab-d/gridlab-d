project(tape)

include_directories(../gldcore)
include_directories(../third_party/jsonCpp)
SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I../third_party/jsonCpp" )

add_library(${PROJECT_NAME} SHARED
        tape/collector.c
        tape/file.c
        tape/file.h
        tape/group_recorder.cpp
        tape/group_recorder.h
        tape/histogram.cpp
        tape/histogram.h
        tape/loadshape.cpp
        tape/loadshape.h
        tape/main.cpp
        tape/memory.c
        tape/memory.h
        tape/metrics_collector.cpp
        tape/metrics_collector.h
        tape/metrics_collector_writer.cpp
        tape/metrics_collector_writer.h
        tape/multi_recorder.c
        tape/odbc.c
        tape/odbc.h
        tape/player.c
        tape/recorder.c
        tape/schedule.cpp
        tape/schedule.h
        tape/shaper.c
        tape/tape.c
        tape/tape.h
        tape/violation_recorder.cpp
        tape/violation_recorder.h
        )

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )

#find_library(libjsoncpp PATHS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/jsonCpp)
target_link_libraries(${PROJECT_NAME} libjsoncpp)

#install(TARGETS ${PROJECT_NAME}
#        LIBRARY DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
#        )