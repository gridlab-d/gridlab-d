cmake_minimum_required(VERSION 3.10)
project(assert)
set(CMAKE_CXX_STANDARD 11)

include_directories(../gldcore)

add_library(assert SHARED
        assert.cpp
        assert.h
        complex_assert.cpp
        complex_assert.h
        double_assert.cpp
        double_assert.h
        enum_assert.cpp
        enum_assert.h
        init.cpp
        int_assert.cpp
        int_assert.h
        main.cpp
        )

set_target_properties(assert
        PROPERTIES PREFIX ""
        )

install(TARGETS assert
        LIBRARY DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        )