pkglib_LTLIBRARIES = libassert.la
libassert_la_SOURCES =                 \
	complex_assert.cpp complex_assert.h \
	double_assert.cpp  double_assert.h  \
	enum_assert.cpp    enum_assert.h    \
	init.cpp                            \
	main.cpp 
libassert_la_LDFLAGS = -no-undefined -version-info 1:0:0

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)

