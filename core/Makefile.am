etcdir=$(prefix)/etc
pkgetcdir=$(etcdir)/$(PACKAGE)
tmydir=$(etcdir)/tmy
pkgetc_DATA=tzinfo.txt unitfile.txt
tmy_DATA=
bin_PROGRAMS = gridlabd
CC = $(PTHREAD_CC)
AM_CFLAGS = $(PTHREAD_CFLAGS)
# Add -pg to AM_CXXFLAGS and AM_CPPFLAGS to enable profiling.
# Debug symbols must also be included with -g if not done elsewhere.
AM_CXXFLAGS = $(PTHREAD_CFLAGS)
AM_LDFLAGS = $(EXTRA_gridlabd_LDFLAGS) -ldl -lxerces-c $(PTHREAD_LIBS) $(PTHREAD_CFLAGS)
gridlabd_SOURCES = build.h main.c \
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
	index.c index.h \
	kill.c kill.h \
	kml.c kml.h \
	legal.c legal.h \
	list.c list.h \
	load.c  load.h \
	load_xml.cpp load_xml.h \
	load_xml_handle.cpp load_xml_handle.h \
	local.c local.h \
	lock.h \
	matlab.c matlab.h \
	match.c match.h \
	module.c module.h \
	object.c object.h \
	output.c output.h \
	platform.h \
	random.c random.h \
	realtime.c realtime.h \
	save.c save.h \
	server.c server.h \
	test_callbacks.h \
	test.c test.h \
	test_framework.cpp test_framework.h \
	threadpool.c threadpool.h \
	timestamp.c timestamp.h \
	unit.c unit.h \
	version.h \
	asm/cmpxchg.h asm/x86/cmpxchg.h asm/x86_64/cmpxchg.h \
	schedule.c schedule.h \
	loadshape.c loadshape.h
EXTRA_gridlabd_SOURCES = \
	cmex.c cmex.h

legal.c: buildnum build.h

build.h: buildnum

buildnum:
	. ../utilities/build_number -b .. >build.h

