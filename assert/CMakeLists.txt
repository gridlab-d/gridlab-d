project(assert)

include_directories(../${GLDCORE_LOC})

add_library(${PROJECT_NAME}
        assert/assert.cpp
        assert/assert.h
        assert/complex_assert.cpp
        assert/complex_assert.h
        assert/double_assert.cpp
        assert/double_assert.h
        assert/enum_assert.cpp
        assert/enum_assert.h
        assert/int_assert.cpp
        assert/int_assert.h
        assert/init.cpp
        assert/main.cpp
        )

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )

target_link_libraries(${PROJECT_NAME} ${OS_SPECIFIC_LIBRARIES})
