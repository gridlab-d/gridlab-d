pkglib_LTLIBRARIES = matpower.la
matpower_la_SOURCES = init.cpp main.cpp matpower.h \
	matpower.cpp include/mclmcrrt.h include/mclcppclass.h\
	bus.cpp bus.h \
	gen.cpp gen.h \
	areas.cpp areas.h \
	baseMVA.cpp baseMVA.h \
	branch.cpp branch.h 
# NEWCLASS 
# DO NOT EDIT THE PREVIOUS LINE

#matpower_la_CPPFLAGS = -I/$(MATLAB_LDPATH)/extern/include
#matpower_la_CPPFLAGS = -I./include

# TODO set the version info as desired
#matpower_la_LDFLAGS = -L/mnt/yizheng/gridspice.simulator/matpower -lopf -lmwmclmcrrt -lmwmclmcrrt -L/mnt/matlab/runtime/glnx86 -module -no-undefined -avoid-version -version-info 0:0:0
matpower_la_LDFLAGS =  -L$(srcdir)/mcc64 -lopf -module -lmwmclmcrrt -no-undefined -avoid-version -version-info 0:0:0


uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)

