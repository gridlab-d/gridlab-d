# transactive/Makefile.mk
# Copyright (C) 2016, Stanford University
# Author: dchassin@slac.stanford.edu

pkglib_LTLIBRARIES += transactive/transactive.la

transactive_transactive_la_CPPFLAGS =
transactive_transactive_la_CPPFLAGS += $(AM_CPPFLAGS)

transactive_transactive_la_LDFLAGS =
transactive_transactive_la_LDFLAGS += $(AM_LDFLAGS)

transactive_transactive_la_LIBADD = 

transactive_transactive_la_SOURCES =
transactive_transactive_la_SOURCES += transactive/house.cpp 
transactive_transactive_la_SOURCES += transactive/house.h
transactive_transactive_la_SOURCES += transactive/agent.cpp 
transactive_transactive_la_SOURCES += transactive/agent.h
transactive_transactive_la_SOURCES += transactive/main.cpp
