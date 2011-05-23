AM_CPPFLAGS = -I../third_party/superLU_MT
pkglib_LTLIBRARIES = powerflow.la 
powerflow_la_SOURCES =                                         \
	billdump.cpp                   billdump.h                   \
	capacitor.cpp                  capacitor.h                  \
	currdump.cpp                   currdump.h                   \
	emissions.cpp				   emissions.h					\
	fault_check.cpp                fault_check.h                \
	frequency_gen.cpp              frequency_gen.h              \
	fuse.cpp                       fuse.h                       \
	init.cpp                                                    \
	line_configuration.cpp         line_configuration.h         \
	line.cpp                       line.h                       \
	line_spacing.cpp               line_spacing.h               \
	link.cpp                       link.h                       \
	load.cpp                       load.h                       \
	main.cpp                                                    \
	meter.cpp                      meter.h                      \
	meter_test.h                                                \
	motor.cpp                      motor.h                      \
	node.cpp                       node.h                       \
	overhead_line_conductor.cpp    overhead_line_conductor.h    \
	overhead_line.cpp              overhead_line.h              \
	overheadline_test.h                                         \
	powerflow.cpp                  powerflow.h                  \
	powerflow_library.cpp          powerflow_library.h          \
	powerflow_object.cpp           powerflow_object.h           \
	power_metrics.cpp              power_metrics.h              \
	pqload.cpp                     pqload.h                     \
	recloser.cpp			       recloser.h					\
	regulator_configuration.cpp    regulator_configuration.h    \
	regulator.cpp                  regulator.h                  \
	regulator_test.h                                            \
	relay.cpp                      relay.h                      \
	relay_test.h                                                \
	restoration.cpp                restoration.h                \
	sectionalizer.cpp			   sectionalizer.h			    \
	series_reactor.cpp             series_reactor.h             \
	solver_nr.cpp                  solver_nr.h                  \
	substation.cpp                 substation.h                 \
	switch_object.cpp              switch_object.h              \
	test.cpp                       test.h                       \
	transformer_configuration.cpp  transformer_configuration.h  \
	transformer.cpp                transformer.h                \
	transformer_test.h                                          \
	triplex_line_conductor.cpp     triplex_line_conductor.h     \
	triplex_line_configuration.cpp triplex_line_configuration.h \
	triplex_line.cpp               triplex_line.h               \
	triplexline_test.h                                          \
	triplex_meter.cpp              triplex_meter.h              \
	triplex_node.cpp               triplex_node.h               \
	underground_line_conductor.cpp underground_line_conductor.h \
	underground_line.cpp           underground_line.h           \
	undergroundline_test.h                                      \
	voltdump.cpp voltdump.h                                     \
	volt_var_control.cpp           volt_var_control.h
powerflow_la_LDFLAGS = -module -no-undefined -avoid-version -version-info 0:0:0 -ldl
powerflow_la_LIBADD = ../third_party/superLU_MT/libsuperlu.la

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)

