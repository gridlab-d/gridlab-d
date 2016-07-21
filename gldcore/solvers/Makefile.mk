pkglib_LTLIBRARIES += gldcore/solvers/glsolvers.la

gldcore_solvers_glsolvers_la_CPPFLAGS =
gldcore_solvers_glsolvers_la_CPPFLAGS += $(AM_CPPFLAGS)

gldcore_solvers_glsolvers_la_LDFLAGS =
gldcore_solvers_glsolvers_la_LDFLAGS += $(AM_LDFLAGS)

gldcore_solvers_glsolvers_la_LIBADD =

gldcore_solvers_glsolvers_la_SOURCES =
gldcore_solvers_glsolvers_la_SOURCES += gldcore/solvers/etp.cpp
gldcore_solvers_glsolvers_la_SOURCES += gldcore/solvers/main.cpp
gldcore_solvers_glsolvers_la_SOURCES += gldcore/solvers/modified_euler.cpp
