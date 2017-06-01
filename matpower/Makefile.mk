pkglib_LTLIBRARIES = matpower/matpower.la

matpower_matpower_la_CPPFLAGS =
matpower_matpower_la_CPPFLAGS += $(MATLAB_CPPFLAGS)
matpower_matpower_la_CPPFLAGS += $(AM_CPPFLAGS)

matpower_matpower_la_LDFLAGS =
matpower_matpower_la_LDFLAGS += -L$(top_srcdir)/mcc64
matpower_matpower_la_LDFLAGS += $(MATLAB_LDFLAGS)
matpower_matpower_la_LDFLAGS += $(AM_LDFLAGS)

matpower_matpower_la_LIBADD =
matpower_matpower_la_LIBADD += -lopf
matpower_matpower_la_LIBADD += -lmwmclmcrrt
matpower_matpower_la_LIBADD += $(MATLAB_LIBS)

matpower_matpower_la_SOURCES =
matpower_matpower_la_SOURCES += matpower/areas.cpp
matpower_matpower_la_SOURCES += matpower/areas.h
matpower_matpower_la_SOURCES += matpower/baseMVA.cpp
matpower_matpower_la_SOURCES += matpower/baseMVA.h
matpower_matpower_la_SOURCES += matpower/branch.cpp
matpower_matpower_la_SOURCES += matpower/branch.h
matpower_matpower_la_SOURCES += matpower/bus.cpp
matpower_matpower_la_SOURCES += matpower/bus.h
matpower_matpower_la_SOURCES += matpower/gen.cpp
matpower_matpower_la_SOURCES += matpower/gen.h
matpower_matpower_la_SOURCES += matpower/include/mclcppclass.h
matpower_matpower_la_SOURCES += matpower/include/mclmcrrt.h
matpower_matpower_la_SOURCES += matpower/init.cpp
matpower_matpower_la_SOURCES += matpower/main.cpp
matpower_matpower_la_SOURCES += matpower/matpower.cpp
matpower_matpower_la_SOURCES += matpower/matpower.h
