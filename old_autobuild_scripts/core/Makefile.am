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
gridlabd_bin_SOURCES = \
	aggregate.c aggregate.h \
	build.h \
	class.c  class.h \
	cmdarg.c cmdarg.h \
	compare.c compare.h \
	complex.h \
	console.h \
	convert.cpp convert.h \
	debug.c  debug.h \
	deltamode.c deltamode.h \
	enduse.c enduse.h \
	environment.c environment.h \
	exception.c exception.h \
	exec.c exec.h \
	find.c find.h \
	gld_sock.h \
	globals.c globals.h \
	gridlabd.h \
	gui.c gui.h \
	http_client.c http_client.h \
	index.c index.h \
	instance.c instance.h \
	instance_cnx.c instance_cnx.h \
	instance_slave.c instance_slave.h \
	interpolate.c interpolate.h \
	job.cpp job.h \
	kill.c kill.h \
	kml.c kml.h \
	legal.c legal.h \
	linkage.c linkage.h \
	list.c list.h \
	link.cpp link.h \
	load.c load.h \
	load_xml.cpp load_xml.h \
	load_xml_handle.cpp load_xml_handle.h \
	loadshape.c loadshape.h \
	local.c local.h \
	lock.cpp lock.h \
	main.c \
	match.c match.h \
	matlab.c matlab.h \
	module.c module.h \
	object.c object.h \
	output.c output.h \
	platform.h \
	property.c property.h \
	random.c random.h \
	realtime.c realtime.h \
	sanitize.cpp sanitize.h \
	save.c save.h \
	setup.cpp setup.h \
	schedule.c schedule.h \
	server.c server.h \
	stream.cpp stream.h stream_type.h \
	test.c test.h \
	test_callbacks.h \
	test_framework.cpp test_framework.h \
	threadpool.c threadpool.h \
	timestamp.c timestamp.h \
	transform.c transform.h \
	unit.c unit.h \
	validate.cpp validate.h \
	version.h \
	deltamode.c deltamode.h\
	# xcore.cpp xcore.h 
EXTRA_gridlabd_bin_SOURCES = \
	cmex.c cmex.h xcore.cpp xcore.h
BUILT_SOURCES = build.h
CLEANFILES = build.h
pkginclude_HEADERS = \
	build.h class.h complex.h debug.h enduse.h exception.h loadshape.h lock.h \
	module.h object.h property.h schedule.h test.h version.h

build.h: buildnum

buildnum:
	if test -f ../utilities/buildnum.sh; then \
		. ../utilities/buildnum.sh ..; \
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

