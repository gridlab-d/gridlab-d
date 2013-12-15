pkglib_LTLIBRARIES += rest/rest.la

# HTML files to install
guirootdir = $(pkgdatadir)
nobase_dist_guiroot_DATA = 
nobase_dist_guiroot_DATA += rest/gui_root/*.html
nobase_dist_guiroot_DATA += rest/gui_root/gsimages/*

rest_rest_la_CPPFLAGS =
rest_rest_la_CPPFLAGS += -I$(top_srcdir)/third_party/superLU_MT
rest_rest_la_CPPFLAGS += -I$(top_srcdir)/core
rest_rest_la_CPPFLAGS = $(AM_CPPFLAGS)

rest_rest_la_LDFLAGS =
rest_rest_la_LDFLAGS = $(AM_LDFLAGS)
rest_rest_la_LDFLAGS += -ldl

rest_rest_la_LIBADD = 
rest_rest_la_LIBADD += core
rest_rest_la_LIBADD += third_party/superLU_MT/libsuperlu.la
rest_rest_la_LIBADD += $(PTHREAD_CFLAGS)
rest_rest_la_LIBADD += $(PTHREAD_LIBS) 

rest_rest_la_SOURCES =
rest_rest_la_SOURCES += rest/init.cpp
rest_rest_la_SOURCES += rest/server.cpp
rest_rest_la_SOURCES += rest/server.h
rest_rest_la_SOURCES += rest/main.cpp
rest_rest_la_SOURCES += rest/rest.h
rest_rest_la_SOURCES += rest/mongoose.c
rest_rest_la_SOURCES += rest/mongoose.h
