AM_CXXFLAGS = -I../third_party/SuperLU_3.1/SRC
libpowerflow_la_LIBADD = ../third_party/SuperLU_3.1/lib/libsuperlu_3.1.a
pkglib_LTLIBRARIES = libpowerflow.la 
libpowerflow_la_SOURCES = \
	init.cpp main.cpp \
	powerflow.cpp powerflow.h \
	capacitor.cpp capacitor.h \
	fuse.cpp fuse.h \
	line_configuration.cpp line_configuration.h \
	line_spacing.cpp line_spacing.h \
	line.cpp line.h \
	link.cpp link.h \
	load.cpp load.h \
	meter.cpp meter.h \
	node.cpp node.h \
	overhead_line_conductor.cpp overhead_line_conductor.h \
	overhead_line.cpp overhead_line.h \
	powerflow_object.cpp powerflow_object.h \
	regulator_configuration.cpp regulator_configuration.h \
	regulator.cpp regulator.h \
	relay.cpp relay.h \
	test.cpp test.h \
	transformer_configuration.cpp transformer_configuration.h \
	transformer.cpp transformer.h \
	triplex_line_conductor.cpp triplex_line_conductor.h \
	triplex_line_configuration.cpp triplex_line_configuration.h \
	triplex_line.cpp triplex_line.h \
	triplex_node.cpp triplex_node.h \
	underground_line_conductor.cpp underground_line_conductor.h \
	underground_line.cpp underground_line.h \
	load.h load.h \
	link.h link.h \
	meter.h meter.h \
	meter_test.h meter_test.h \
	powerflow_library.cpp powerflow_library.h \
	triplex_meter.cpp triplex_meter.h  \
	switch_object.cpp switch_object.h \
	substation.cpp substation.h \
	pqload.cpp pqload.h \
	voltdump.cpp voltdump.h \
	solver_nr.cpp solver_nr.h \
	frequency_gen.cpp frequency_gen.h \
	restoration.cpp frequency_gen.h \
	series_reactor.cpp series_reactor.h 

libpowerflow_la_LDFLAGS = -version-info 0:0:0

