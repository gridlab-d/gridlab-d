if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    MESSAGE("Found Clang compiler")
    set(WARNING_FLAGS
            -Weverything
#            -Werror
            CACHE INTERNAL "WARNING_FLAGS")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    MESSAGE("Found GNU compiler")
    set(WARNING_FLAGS
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wnon-virtual-dtor  # makes non-virtual destructors in base classes a warning
            -Wold-style-cast
            -Wcast-align
            -Wunused
            -Wmisleading-indentation
            -Wduplicated-cond
            -Wduplicated-branches
            -Wlogical-op
            -Wnull-dereference
            -Wuseless-cast
            -Wdouble-promotion
            -Wformat=2
            -Wimplicit-fallthrough
            -Wconversion  # makes narrowing conversions (i.e. double -> int) a warning
            -Wsign-conversion
            -Warith-conversion
            -Wenum-conversion
#            -Werror  # just me being pedantic
            CACHE INTERNAL "WARNING_FLAGS")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    MESSAGE("Found MSVC compiler")
    SET(WARNING_FLAGS /permissive /W4 /w14640 /WX CACHE INTERNAL "WARNING_FLAGS")
endif()