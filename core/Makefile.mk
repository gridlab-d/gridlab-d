dist_pkgdata_DATA += core/tzinfo.txt
dist_pkgdata_DATA += core/unitfile.txt

bin_SCRIPTS += core/gridlabd

bin_PROGRAMS += gridlabd.bin

gridlabd_bin_CPPFLAGS =
gridlabd_bin_CPPFLAGS += $(XERCES_CPPFLAGS)
gridlabd_bin_CPPFLAGS += $(AM_CPPFLAGS)

gridlabd_bin_LDFLAGS =
gridlabd_bin_LDFLAGS += $(XERCES_LDFLAGS)
gridlabd_bin_LDFLAGS += $(AM_LDFLAGS)

gridlabd_bin_LDADD =
gridlabd_bin_LDADD += $(XERCES_LIB)
gridlabd_bin_LDADD += $(CURSES_LIB)
gridlabd_bin_LDADD += -ldl

gridlabd_bin_SOURCES =
gridlabd_bin_SOURCES += core/aggregate.c
gridlabd_bin_SOURCES += core/aggregate.h
gridlabd_bin_SOURCES += core/build.h
gridlabd_bin_SOURCES += core/class.c
gridlabd_bin_SOURCES += core/class.h
gridlabd_bin_SOURCES += core/cmdarg.c
gridlabd_bin_SOURCES += core/cmdarg.h
gridlabd_bin_SOURCES += core/compare.c
gridlabd_bin_SOURCES += core/compare.h
gridlabd_bin_SOURCES += core/complex.h
gridlabd_bin_SOURCES += core/console.h
gridlabd_bin_SOURCES += core/convert.cpp
gridlabd_bin_SOURCES += core/convert.h
gridlabd_bin_SOURCES += core/debug.c
gridlabd_bin_SOURCES += core/debug.h
gridlabd_bin_SOURCES += core/deltamode.c
gridlabd_bin_SOURCES += core/deltamode.h
gridlabd_bin_SOURCES += core/enduse.c
gridlabd_bin_SOURCES += core/enduse.h
gridlabd_bin_SOURCES += core/environment.c
gridlabd_bin_SOURCES += core/environment.h
gridlabd_bin_SOURCES += core/exception.c
gridlabd_bin_SOURCES += core/exception.h
gridlabd_bin_SOURCES += core/exec.c
gridlabd_bin_SOURCES += core/exec.h
gridlabd_bin_SOURCES += core/find.c
gridlabd_bin_SOURCES += core/find.h
gridlabd_bin_SOURCES += core/gld_sock.h
gridlabd_bin_SOURCES += core/globals.c
gridlabd_bin_SOURCES += core/globals.h
gridlabd_bin_SOURCES += core/gridlabd.h
gridlabd_bin_SOURCES += core/gui.c
gridlabd_bin_SOURCES += core/gui.h
gridlabd_bin_SOURCES += core/http_client.c
gridlabd_bin_SOURCES += core/http_client.h
gridlabd_bin_SOURCES += core/index.c
gridlabd_bin_SOURCES += core/index.h
gridlabd_bin_SOURCES += core/instance.c
gridlabd_bin_SOURCES += core/instance_cnx.c
gridlabd_bin_SOURCES += core/instance_cnx.h
gridlabd_bin_SOURCES += core/instance.h
gridlabd_bin_SOURCES += core/instance_slave.c
gridlabd_bin_SOURCES += core/instance_slave.h
gridlabd_bin_SOURCES += core/interpolate.c
gridlabd_bin_SOURCES += core/interpolate.h
gridlabd_bin_SOURCES += core/job.cpp
gridlabd_bin_SOURCES += core/job.h
gridlabd_bin_SOURCES += core/kill.c
gridlabd_bin_SOURCES += core/kill.h
gridlabd_bin_SOURCES += core/kml.c
gridlabd_bin_SOURCES += core/kml.h
gridlabd_bin_SOURCES += core/legal.c
gridlabd_bin_SOURCES += core/legal.h
gridlabd_bin_SOURCES += core/linkage.c
gridlabd_bin_SOURCES += core/linkage.h
gridlabd_bin_SOURCES += core/link.cpp
gridlabd_bin_SOURCES += core/link.h
gridlabd_bin_SOURCES += core/list.c
gridlabd_bin_SOURCES += core/list.h
gridlabd_bin_SOURCES += core/load.c
gridlabd_bin_SOURCES += core/load.h
gridlabd_bin_SOURCES += core/loadshape.c
gridlabd_bin_SOURCES += core/loadshape.h
gridlabd_bin_SOURCES += core/load_xml.cpp
gridlabd_bin_SOURCES += core/load_xml.h
gridlabd_bin_SOURCES += core/load_xml_handle.cpp
gridlabd_bin_SOURCES += core/load_xml_handle.h
gridlabd_bin_SOURCES += core/local.c
gridlabd_bin_SOURCES += core/local.h
gridlabd_bin_SOURCES += core/lock.cpp
gridlabd_bin_SOURCES += core/lock.h
gridlabd_bin_SOURCES += core/main.c
gridlabd_bin_SOURCES += core/match.c
gridlabd_bin_SOURCES += core/match.h
gridlabd_bin_SOURCES += core/matlab.c
gridlabd_bin_SOURCES += core/matlab.h
gridlabd_bin_SOURCES += core/module.c
gridlabd_bin_SOURCES += core/module.h
gridlabd_bin_SOURCES += core/object.c
gridlabd_bin_SOURCES += core/object.h
gridlabd_bin_SOURCES += core/output.c
gridlabd_bin_SOURCES += core/output.h
gridlabd_bin_SOURCES += core/platform.h
gridlabd_bin_SOURCES += core/property.c
gridlabd_bin_SOURCES += core/property.h
gridlabd_bin_SOURCES += core/random.c
gridlabd_bin_SOURCES += core/random.h
gridlabd_bin_SOURCES += core/realtime.c
gridlabd_bin_SOURCES += core/realtime.h
gridlabd_bin_SOURCES += core/sanitize.cpp
gridlabd_bin_SOURCES += core/sanitize.h
gridlabd_bin_SOURCES += core/save.c
gridlabd_bin_SOURCES += core/save.h
gridlabd_bin_SOURCES += core/schedule.c
gridlabd_bin_SOURCES += core/schedule.h
gridlabd_bin_SOURCES += core/server.c
gridlabd_bin_SOURCES += core/server.h
gridlabd_bin_SOURCES += core/setup.cpp
gridlabd_bin_SOURCES += core/setup.h
gridlabd_bin_SOURCES += core/stream.cpp
gridlabd_bin_SOURCES += core/stream.h
gridlabd_bin_SOURCES += core/stream_type.h
gridlabd_bin_SOURCES += core/test.c
gridlabd_bin_SOURCES += core/test_callbacks.h
gridlabd_bin_SOURCES += core/test_framework.cpp
gridlabd_bin_SOURCES += core/test_framework.h
gridlabd_bin_SOURCES += core/test.h
gridlabd_bin_SOURCES += core/threadpool.c
gridlabd_bin_SOURCES += core/threadpool.h
gridlabd_bin_SOURCES += core/timestamp.c
gridlabd_bin_SOURCES += core/timestamp.h
gridlabd_bin_SOURCES += core/transform.c
gridlabd_bin_SOURCES += core/transform.h
gridlabd_bin_SOURCES += core/unit.c
gridlabd_bin_SOURCES += core/unit.h
gridlabd_bin_SOURCES += core/validate.cpp
gridlabd_bin_SOURCES += core/validate.h
gridlabd_bin_SOURCES += core/version.h

EXTRA_gridlabd_bin_SOURCES =
EXTRA_gridlabd_bin_SOURCES += core/cmex.c
EXTRA_gridlabd_bin_SOURCES += core/cmex.h
EXTRA_gridlabd_bin_SOURCES += core/xcore.cpp
EXTRA_gridlabd_bin_SOURCES += core/xcore.h

BUILT_SOURCES += core/build.h

CLEANFILES += core/build.h

pkginclude_HEADERS =
pkginclude_HEADERS += core/build.h
pkginclude_HEADERS += core/class.h
pkginclude_HEADERS += core/complex.h
pkginclude_HEADERS += core/debug.h
pkginclude_HEADERS += core/enduse.h
pkginclude_HEADERS += core/exception.h
pkginclude_HEADERS += core/loadshape.h
pkginclude_HEADERS += core/lock.h
pkginclude_HEADERS += core/module.h
pkginclude_HEADERS += core/object.h
pkginclude_HEADERS += core/property.h
pkginclude_HEADERS += core/schedule.h
pkginclude_HEADERS += core/test.h
pkginclude_HEADERS += core/version.h

core/build.h: buildnum

.PHONY: buildnum
buildnum:
	$(AM_V_GEN)new=`svn info $(top_srcdir) | sed -ne 's/^Last Changed Rev: //p'`; \
	if test -f $(top_build_prefix)core/build.h; then \
		old=`cat $(top_build_prefix)core/build.h | sed -ne 's/^#define BUILDNUM //p'`; \
	else \
		old=0; \
	fi; \
	echo "New version is '$$new', old version is '$$old'"; \
	if test -z "$$old" -o $$new -ne $$old; then \
		echo "#define BUILDNUM $$new" > $(top_build_prefix)core/build.h; \
	fi;
