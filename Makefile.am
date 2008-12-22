# $Id: Makefile.am,v 1.21 2008/01/22 22:01:09 d3p181 Exp $
# Copyright (C) 2008 Battelle Memorial Institute
# This file is distributed under the same terms as GridLAB-D.

ACLOCAL_AMFLAGS = -I m4
SUBDIRS = core climate commercial network powerflow residential tape generators plc tape_file tape_plot market reliability smartgrid

include aminclude.am

EXTRA_DIST = doxygen/*.*

mostlyclean-local: run-clean

clean-local: run-clean

distclean-local: run-clean

maintainer-clean-local: run-clean
	-find $$SUBDIRS -iname Makefile.in -delete

over: clean all

repo-clean:
	cvs ls `find . -type d ! -name CVS -print` 2>/dev/null | grep '^? ' | \
		sed 's/^? \(.*\)$$/\1/' | xargs -d \\n /bin/rm -rf
	find . -maxdepth 1 -type l -execdir rm -rf \{\} \+

#BEGIN run

run-clean:
	-rm -rf run

run:
	mkdir $@

run/gridlabd$(EXEEXT): core/gridlabd$(EXEEXT)
	(cd run && $(LN_S) ../$< ../$@)

run/libclimate$(LIBEXT): climate/.libs/libclimate$(LIBEXT)
	(cd run && $(LN_S) ../$< ../$@)

run/libcommercial$(LIBEXT): commercial/.libs/libcommercial$(LIBEXT)
	(cd run && $(LN_S) ../$< ../$@)

run/libnetwork$(LIBEXT): network/.libs/libnetwork$(LIBEXT)
	(cd run && $(LN_S) ../$< ../$@)

run/libpowerflow$(LIBEXT): powerflow/.libs/libpowerflow$(LIBEXT)
	(cd run && $(LN_S) ../$< ../$@)

run/libresidential$(LIBEXT): residential/.libs/libresidential$(LIBEXT)
	(cd run && $(LN_S) ../$< ../$@)

run/libtape$(LIBEXT): tape/.libs/libtape$(LIBEXT)
	(cd run && $(LN_S) ../$< ../$@)

run/dryer.txt: test/shapes/dryer.txt
	(cd run && $(LN_S) ../$< ../$@)

run/test1.txt: test/input/test1.txt
	(cd run && $(LN_S) ../$< ../$@)

run/ieee14bus.cdf: test/externals/ieee14bus.cdf
	(cd run && $(LN_S) ../$< ../$@)

run/ieee300bus.cdf: test/externals/ieee300bus.cdf
	(cd run && $(LN_S) ../$< ../$@)

run/pnnl2bus.cdf: test/externals/pnnl2bus.cdf
	(cd run && $(LN_S) ../$< ../$@)

run/speed.tape: test/benchmark/speed.tape
	(cd run && $(LN_S) ../$< ../$@)

input = test/input/example.glm

run-input:
	(cd run && $(LN_S) -f ../${input} input.glm)

run-all: all ./run run/gridlabd$(EXEEXT) run/libclimate$(LIBEXT) \
	run/libcommercial$(LIBEXT) run/libnetwork$(LIBEXT) \
	run/libpowerflow$(LIBEXT) run/libresidential$(LIBEXT) \
	run/libtape$(LIBEXT)

run-support: run-all run/dryer.txt run/test1.txt \
	run/ieee14bus.cdf run/ieee300bus.cdf run/pnnl2bus.cdf

test: run-all
	(cd run && GLPATH=../core LD_LIBRARY_PATH=$(PWD)/run \
		./gridlabd --test $(test))

test-run: run-all run-support run-input
	(cd run && GLPATH=../core LD_LIBRARY_PATH=$(PWD)/run \
		./gridlabd --check --verbose --warn --profile --dumpall -o gridlabd.xml $(args) input.glm)

run-debug: run-all run-support run-input
	(cd run && GLPATH=../core LD_LIBRARY_PATH=$(PWD)/run \
		./gridlabd --check --debug --verbose --warn --profile --dumpall -o gridlabd.xml $(args) input.glm)

run-gdb: run-all run-support run-input
	(cd run && GLPATH=../core LD_LIBRARY_PATH=$(PWD)/run \
		gdb --args ./gridlabd --check --verbose --warn --profile --dumpall -o gridlabd.xml $(args) input.glm)

compare-source:
	bash ./compare-sources $(SUBDIRS)

#END run
