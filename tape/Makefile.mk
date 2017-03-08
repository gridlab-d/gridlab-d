pkglib_LTLIBRARIES += tape/tape.la

tape_tape_la_CPPFLAGS =
tape_tape_la_CPPFLAGS += -I$(top_srcdir)/third_party/jsonCpp
tape_tape_la_CPPFLAGS += $(AM_CPPFLAGS)

tape_tape_la_LDFLAGS =
tape_tape_la_LDFLAGS += $(AM_LDFLAGS)

tape_tape_la_LIBADD =
tape_tape_la_LIBADD += third_party/jsonCpp/libjsoncpp.la
tape_tape_la_LIBADD += -ldl

tape_tape_la_SOURCES =
tape_tape_la_SOURCES += tape/collector.c
tape_tape_la_SOURCES += tape/file.c
tape_tape_la_SOURCES += tape/file.h
tape_tape_la_SOURCES += tape/group_recorder.h
tape_tape_la_SOURCES += tape/group_recorder.cpp
tape_tape_la_SOURCES += tape/metrics_collector.cpp
tape_tape_la_SOURCES += tape/metrics_collector.h
tape_tape_la_SOURCES += tape/metrics_collector_writer.cpp
tape_tape_la_SOURCES += tape/metrics_collector_writer.h
tape_tape_la_SOURCES += tape/violation_recorder.h
tape_tape_la_SOURCES += tape/violation_recorder.cpp
tape_tape_la_SOURCES += tape/histogram.cpp
tape_tape_la_SOURCES += tape/histogram.h
tape_tape_la_SOURCES += tape/loadshape.cpp
tape_tape_la_SOURCES += tape/loadshape.h
tape_tape_la_SOURCES += tape/main.cpp
tape_tape_la_SOURCES += tape/memory.c
tape_tape_la_SOURCES += tape/memory.h
tape_tape_la_SOURCES += tape/multi_recorder.c
tape_tape_la_SOURCES += tape/odbc.c
tape_tape_la_SOURCES += tape/odbc.h
tape_tape_la_SOURCES += tape/player.c
tape_tape_la_SOURCES += tape/recorder.c
tape_tape_la_SOURCES += tape/shaper.c
tape_tape_la_SOURCES += tape/tape.c
tape_tape_la_SOURCES += tape/tape.h
