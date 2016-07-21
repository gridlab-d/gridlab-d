pkglib_LTLIBRARIES += gldcore/link/engine/glxengine.la

gldcore_link_engine_glxengine_la_CPPFLAGS =
gldcore_link_engine_glxengine_la_CPPFLAGS += $(AM_CPPFLAGS)

gldcore_link_engine_glxengine_la_LDFLAGS =
gldcore_link_engine_glxengine_la_LDFLAGS += -module -no-undefined -avoid-version -version-info 1:0:0
gldcore_link_engine_glxengine_la_LDFLAGS += $(AM_LDFLAGS)

gldcore_link_engine_glxengine_la_LIBADD =

gldcore_link_engine_glxengine_la_SOURCES = 
gldcore_link_engine_glxengine_la_SOURCES += gldcore/link/engine/engine.cpp
gldcore_link_engine_glxengine_la_SOURCES += gldcore/link/engine/engine.h
gldcore_link_engine_glxengine_la_SOURCES += gldcore/link/engine/udpconnection.cpp

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)
