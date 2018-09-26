project(tape)

include_directories(../${GLDCORE_LOC})
include_directories(../third_party/jsonCpp)
SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I../third_party/jsonCpp" )

add_library(${PROJECT_NAME} SHARED
        tape/collector.cpp
        tape/collector.h
        tape/file.cpp
        tape/file.h
        tape/group_recorder.h
        tape/group_recorder.cpp
        tape/metrics_collector.cpp
        tape/metrics_collector.h
        tape/metrics_collector_writer.cpp
        tape/metrics_collector_writer.h
        tape/violation_recorder.h
        tape/violation_recorder.cpp
        tape/histogram.cpp
        tape/histogram.h
        tape/loadshape.cpp
        tape/loadshape.h
        tape/main.cpp
        tape/memory.cpp
        tape/memory.h
        tape/multi_recorder.cpp
        tape/multi_recorder.h
        tape/odbc.cpp
        tape/odbc.h
        tape/player.cpp
        tape/player.h
        tape/recorder.cpp
        tape/recorder.h
        tape/shaper.cpp
        tape/shaper.h
        tape/tape.cpp
        tape/tape.h
        )

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )

target_link_libraries(${PROJECT_NAME} libjsoncpp)

target_link_libraries(${PROJECT_NAME} ${OS_SPECIFIC_LIBRARIES})
