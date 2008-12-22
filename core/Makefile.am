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
gridlabd_SOURCES =    \
	aggregate.c        \
	class.c            \
	cmdarg.c           \
	convert.c          \
	debug.c            \
	environment.c      \
	exception.c        \
	exec.c             \
	find.c             \
	globals.c          \
	index.c            \
	kill.c		\
	kml.c				    \
	legal.c            \
	list.c             \
	load.c             \
	load_xml.cpp       \
	load_xml_handle.cpp \
	local.c            \
	main.c             \
	matlab.c           \
	match.c            \
	module.c         \
	object.c           \
	output.c           \
	random.c           \
	realtime.c         \
	save.c             \
	test.c             \
	test_framework.cpp \
	threadpool.c       \
	timestamp.c        \
	unit.c             \
	aggregate.h        \
	class.h            \
	cmdarg.h           \
	complex.h          \
	convert.h          \
	debug.h            \
	environment.h      \
	exception.h        \
	exec.h             \
	find.h             \
	globals.h          \
	gridlabd.h         \
	index.h            \
	kill.h		\
	kml.h              \
	legal.h            \
	list.h             \
	load.h             \
	load_xml.h         \
	load_xml_handle.h  \
	local.h            \
	lock.h             \
	match.h		\
	matlab.h           \
	module.h           \
	object.h           \
	output.h           \
	platform.h         \
	random.h           \
	realtime.h         \
	save.h             \
	test_callbacks.h   \
	test_framework.h   \
	test.h             \
	threadpool.h       \
	timestamp.h        \
	unit.h             \
	version.h	\
	asm/cmpxchg.h      \
	asm/x86/cmpxchg.h  \
	asm/x86_64/cmpxchg.h
EXTRA_gridlabd_SOURCES = \
	cmex.c                \
	cmex.h

