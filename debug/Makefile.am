pkglib_LTLIBRARIES = assert.la
assert_la_SOURCES =                    \
	complex_assert.cpp complex_assert.h \
	double_assert.cpp  double_assert.h  \
	enum_assert.cpp    enum_assert.h    \
	assert.cpp assert.h \
	init.cpp                            \
	main.cpp 
assert_la_LDFLAGS = -module -no-undefined -avoid-version -version-info 1:0:0

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)

