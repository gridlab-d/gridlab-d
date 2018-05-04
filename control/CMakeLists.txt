cmake_minimum_required(VERSION 3.10)
project(gridlabd_assert)

include_directories(BEFORE ../gldcore)

add_library(gridlabd_assert SHARED
        assert.cpp
        assert.h
        complex_assert.cpp
        complex_assert.h
        double_assert.cpp
        double_assert.h
        enum_assert.cpp
        enum_assert.h
        int_assert.cpp
        int_assert.h
        init.cpp
        main.cpp
        )

INCLUDE(GNUInstallDirs)

install(TARGETS gridlabd_assert
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )


