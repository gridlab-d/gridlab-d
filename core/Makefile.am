SUBDIRS = rt
dist_pkglib_DATA = tzinfo.txt unitfile.txt
pkglib_PROGRAMS = gridlabd.bin
dist_pkglib_SCRIPTS = gridlabd
CC = $(PTHREAD_CC)
AM_CFLAGS = $(PTHREAD_CFLAGS)
# Add -pg to AM_CXXFLAGS and AM_CPPFLAGS to enable profiling.
# Debug symbols must also be included with -g if not done elsewhere.
AM_CXXFLAGS = $(PTHREAD_CFLAGS)
AM_LDFLAGS = $(EXTRA_gridlabd_LDFLAGS) -ldl -lxerces-c $(PTHREAD_LIBS) $(PTHREAD_CFLAGS)
gridlabd_bin_SOURCES = build.h main.c \
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
	version.h
EXTRA_gridlabd_bin_SOURCES = \
	cmex.c cmex.h

legal.c: buildnum build.h

build.h: buildnum

buildnum:
	if [ -f ../utilities/build_number ]; then \
		. ../utilities/build_number -b .. > build.h; \
	elif [ ! -f build.h ]; then \
		echo '#define "$$Revision: 0"' > build.h; \
	fi

install-data-local:
	test -d "$(DESTDIR)$(bindir)" || mkdir -p "$(DESTDIR)$(bindir)"
	test -d "$(DESTDIR)$(libdir)" || mkdir -p "$(DESTDIR)$(libdir)"
	if test "`dirname '$(DESTDIR)$(bindir)'`" -ef "`dirname '$(DESTDIR)$(libdir)'`"; then \
		$(LN_S) -f ../lib/@PACKAGE@/gridlabd "$(DESTDIR)$(bindir)/gridlabd"; \
	else \
		$(LN_S) -f "$(DESTDIR)$(pkglibdir)/gridlabd" "$(DESTDIR)$(bindir)/gridlabd"; \
	fi

uninstall-local:
	-rm $(DESTDIR)$(bindir)/gridlabd

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)

dist-hook:
	. ../utilities/build_number -b .. > $(distdir)/build.h

