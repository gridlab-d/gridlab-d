# powernet/Makefile.mk
# Copyright (C) 2016, Stanford University
# Author: dchassin@slac.stanford.edu

pkglib_LTLIBRARIES += powernet/powernet.la

powernet_powernet_la_CPPFLAGS =
powernet_powernet_la_CPPFLAGS += $(AM_CPPFLAGS)

powernet_powernet_la_LDFLAGS =
powernet_powernet_la_LDFLAGS += $(AM_LDFLAGS)

powernet_powernet_la_LIBADD = 

powernet_powernet_la_SOURCES = powernet/main.cpp
powernet_powernet_la_SOURCES += powernet/agent.cpp powernet/agent.h
powernet_powernet_la_SOURCES += powernet/house.cpp powernet/house.h
powernet_powernet_la_SOURCES += powernet/message.cpp powernet/message.h
