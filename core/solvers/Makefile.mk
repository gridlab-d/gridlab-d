pkglib_LTLIBRARIES += core/solvers/glsolvers.la

core_solvers_glsolvers_la_CPPFLAGS =
core_solvers_glsolvers_la_CPPFLAGS += $(AM_CPPFLAGS)

core_solvers_glsolvers_la_LDFLAGS =
core_solvers_glsolvers_la_LDFLAGS += $(AM_LDFLAGS)

core_solvers_glsolvers_la_LIBADD =

core_solvers_glsolvers_la_SOURCES =
core_solvers_glsolvers_la_SOURCES += core/solvers/etp.cpp
core_solvers_glsolvers_la_SOURCES += core/solvers/main.cpp
core_solvers_glsolvers_la_SOURCES += core/solvers/modified_euler.cpp
