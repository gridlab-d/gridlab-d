pkglib_LTLIBRARIES += core/link/matlab/glxmatlab.la

core_link_matlab_glxmatlab_la_CPPFLAGS =
core_link_matlab_glxmatlab_la_CPPFLAGS += $(MATLAB_CPPFLAGS)
core_link_matlab_glxmatlab_la_CPPFLAGS += $(AM_CPPFLAGS)

core_link_matlab_glxmatlab_la_LDFLAGS =
core_link_matlab_glxmatlab_la_LDFLAGS += $(MATLAB_LDFLAGS)
core_link_matlab_glxmatlab_la_LDFLAGS += $(AM_LDFLAGS)

core_link_matlab_glxmatlab_la_LIBADD =
core_link_matlab_glxmatlab_la_LIBADD += $(MATLAB_LIBS)

core_link_matlab_glxmatlab_la_SOURCES = core/link/matlab/matlab.cpp
