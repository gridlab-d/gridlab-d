cmake_minimum_required(VERSION 3.10)
project(assert)
set(CMAKE_CXX_STANDARD 11)

include_directories(../gldcore)

add_library(${PROJECT_NAME} SHARED
        assert/assert.cpp
        assert/assert.h
        assert/complex_assert.cpp
        assert/complex_assert.h
        assert/double_assert.cpp
        assert/double_assert.h
        assert/enum_assert.cpp
        assert/enum_assert.h
        assert/init.cpp
        assert/int_assert.cpp
        assert/int_assert.h
        assert/main.cpp
        )

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )

#install(TARGETS ${PROJECT_NAME}
#LIBRARY DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
#        )