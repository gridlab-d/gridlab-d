pkglib_LTLIBRARIES += core/link/engine/glxengine.la

core_link_engine_glxengine_la_CPPFLAGS =
core_link_engine_glxengine_la_CPPFLAGS += $(AM_CPPFLAGS)

core_link_engine_glxengine_la_LDFLAGS =
core_link_engine_glxengine_la_LDFLAGS += -module -no-undefined -avoid-version -version-info 1:0:0
core_link_engine_glxengine_la_LDFLAGS += $(AM_LDFLAGS)

core_link_engine_glxengine_la_LIBADD =

core_link_engine_glxengine_la_SOURCES = 
core_link_engine_glxengine_la_SOURCES += core/link/engine/engine.cpp
core_link_engine_glxengine_la_SOURCES += core/link/engine/engine.h
core_link_engine_glxengine_la_SOURCES += core/link/engine/udpconnection.cpp

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)
