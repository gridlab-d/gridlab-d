SUBDIRS = rt
dist_pkglib_DATA = tzinfo.txt unitfile.txt
pkglib_PROGRAMS = gridlabd.bin
dist_pkglib_SCRIPTS = gridlabd
CC = $(PTHREAD_CC)
AM_CFLAGS = $(PTHREAD_CFLAGS)
# Add -pg to AM_CXXFLAGS and AM_CPPFLAGS to enable profiling.
# Debug symbols must also be included with -g if not done elsewhere.
AM_CPPFLAGS = $(XERCES_CPPFLAGS)
AM_CXXFLAGS = $(PTHREAD_CFLAGS)
#AM_LDFLAGS = $(EXTRA_gridlabd_LDFLAGS) -ldl -lX11 -L/usr/lib/X11 $(XERCES_LDFLAGS) $(XERCES_LIBS) $(PTHREAD_LIBS) $(PTHREAD_CFLAGS)
AM_LDFLAGS = $(EXTRA_gridlabd_LDFLAGS) -ldl -L/usr/lib/X11 $(XERCES_LDFLAGS) $(XERCES_LIBS) $(PTHREAD_LIBS) $(PTHREAD_CFLAGS)
gridlabd_bin_SOURCES = main.c build.h \
	aggregate.c aggregate.h \
	class.c  class.h \
	cmdarg.c cmdarg.h \
	complex.h \
	convert.cpp convert.h \
	debug.c  debug.h \
	enduse.c enduse.h \
	environment.c environment.h \
	exception.c exception.h \
	exec.c exec.h \
	find.c find.h \
	globals.c globals.h \
	gridlabd.h \
	gui.c gui.h \
	index.c index.h \
	interpolate.c interpolate.h \
	kill.c kill.h \
	kml.c kml.h \
	legal.c legal.h \
	list.c list.h \
	load.c load.h \
	loadshape.c loadshape.h \
	load_xml.cpp load_xml.h \
	load_xml_handle.cpp load_xml_handle.h \
	local.c local.h \
	lock.h \
	match.c match.h \
	matlab.c matlab.h \
	module.c module.h \
	object.c object.h \
	output.c output.h \
	platform.h \
	random.c random.h \
	realtime.c realtime.h \
	save.c save.h \
	schedule.c schedule.h \
	server.c server.h \
	stream.c stream.h \
	test.c test.h \
	test_callbacks.h \
	test_framework.cpp test_framework.h \
	threadpool.c threadpool.h \
	timestamp.c timestamp.h \
	unit.c unit.h \
	version.h \
	# xcore.cpp xcore.h 
EXTRA_gridlabd_bin_SOURCES = \
	cmex.c cmex.h xcore.cpp xcore.h
BUILT_SOURCES = build.h
CLEANFILES = build.h
pkginclude_HEADERS = \
	build.h class.h complex.h debug.h enduse.h exception.h loadshape.h lock.h \
	module.h object.h schedule.h test.h version.h

build.h: buildnum

buildnum:
	if test -f ../utilities/build_number; then \
		if test -f build.h; then \
			. ../utilities/build_number -b .. > buildnum.h; \
			test "`cat buildnum.h`" = "`cat build.h`" || mv -f buildnum.h build.h; \
			test -f buildnum.h && rm buildnum.h || true; \
		else \
			. ../utilities/build_number -b .. > build.h; \
		fi; \
	elif test ! -f build.h; then \
		echo '#define "$$Revision: 0"' > build.h; \
	fi

.PHONY: buildnum

install-data-local:
	test -d "$(DESTDIR)$(bindir)" || mkdir -p "$(DESTDIR)$(bindir)"
	test -d "$(DESTDIR)$(libdir)" || mkdir -p "$(DESTDIR)$(libdir)"
	if test "`dirname '$(DESTDIR)$(bindir)'`" -ef "`dirname '$(DESTDIR)$(libdir)'`"; then \
		$(LN_S) -f ../`basename $(libdir)`/@PACKAGE@/gridlabd "$(DESTDIR)$(bindir)/gridlabd"; \
	else \
		$(LN_S) -f "$(DESTDIR)$(pkglibdir)/gridlabd" "$(DESTDIR)$(bindir)/gridlabd"; \
	fi

uninstall-local:
	-rm $(DESTDIR)$(bindir)/gridlabd

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)

