pkglib_LTLIBRARIES += assert/assert.la

assert_assert_la_CPPFLAGS =
assert_assert_la_CPPFLAGS += $(AM_CPPFLAGS)

assert_assert_la_LDFLAGS =
assert_assert_la_LDFLAGS += $(AM_LDFLAGS)

assert_assert_la_LIBADD = 

assert_assert_la_SOURCES =
assert_assert_la_SOURCES += assert/assert.cpp
assert_assert_la_SOURCES += assert/assert.h
assert_assert_la_SOURCES += assert/complex_assert.cpp
assert_assert_la_SOURCES += assert/complex_assert.h
assert_assert_la_SOURCES += assert/double_assert.cpp
assert_assert_la_SOURCES += assert/double_assert.h
assert_assert_la_SOURCES += assert/enum_assert.cpp
assert_assert_la_SOURCES += assert/enum_assert.h
assert_assert_la_SOURCES += assert/init.cpp
assert_assert_la_SOURCES += assert/main.cpp
