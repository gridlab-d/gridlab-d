cmake_minimum_required(VERSION 3.10)
project(tape)
set(CMAKE_CXX_STANDARD 11)

include_directories(../gldcore)

add_library(tape SHARED
        collector.c
        file.c
        file.h
        group_recorder.cpp
        group_recorder.h
        histogram.cpp
        histogram.h
        loadshape.cpp
        loadshape.h
        main.cpp
        memory.c
        memory.h
        metrics_collector.cpp
        metrics_collector.h
        metrics_collector_writer.cpp
        metrics_collector_writer.h
        multi_recorder.c
        odbc.c
        odbc.h
        player.c
        recorder.c
        schedule.cpp
        schedule.h
        shaper.c
        tape.c
        tape.h
        violation_recorder.cpp
        violation_recorder.h
        )

set_target_properties(tape
        PROPERTIES PREFIX ""
        )

install(TARGETS tape
        LIBRARY DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        )