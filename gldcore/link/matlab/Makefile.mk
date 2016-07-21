pkglib_LTLIBRARIES += gldcore/link/matlab/glxmatlab.la

gldcore_link_matlab_glxmatlab_la_CPPFLAGS =
gldcore_link_matlab_glxmatlab_la_CPPFLAGS += $(MATLAB_CPPFLAGS)
gldcore_link_matlab_glxmatlab_la_CPPFLAGS += $(AM_CPPFLAGS)

gldcore_link_matlab_glxmatlab_la_LDFLAGS =
gldcore_link_matlab_glxmatlab_la_LDFLAGS += $(MATLAB_LDFLAGS)
gldcore_link_matlab_glxmatlab_la_LDFLAGS += $(AM_LDFLAGS)

gldcore_link_matlab_glxmatlab_la_LIBADD =
gldcore_link_matlab_glxmatlab_la_LIBADD += $(MATLAB_LIBS)

gldcore_link_matlab_glxmatlab_la_SOURCES = gldcore/link/matlab/matlab.cpp
