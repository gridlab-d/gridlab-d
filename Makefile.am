# $Id: Makefile.am 4738 2014-07-03 00:55:39Z dchassin $
# Copyright (C) 2008 Battelle Memorial Institute
# This file is distributed under the same terms as GridLAB-D.

ACLOCAL_AMFLAGS = -I m4

CC = $(PTHREAD_CC)
CXX = $(PTHREAD_CXX)

AM_CPPFLAGS =
AM_CPPFLAGS += -I$(top_srcdir)/core
AM_CPPFLAGS += $(GLD_CPPFLAGS)

AM_CFLAGS =
AM_CFLAGS += $(PTHREAD_CFLAGS)
AM_CFLAGS += $(GLD_CFLAGS)

AM_CXXFLAGS =
AM_CXXFLAGS += $(PTHREAD_CFLAGS)
AM_CXXFLAGS += $(GLD_CXXFLAGS)

AM_LDFLAGS =
AM_LDFLAGS += -no-undefined
AM_LDFLAGS += -module
AM_LDFLAGS += -avoid-version
AM_LDFLAGS += -export-dynamic
AM_LDFLAGS += $(GLD_LDFLAGS)

# makefile fragments populate these variables
BUILT_SOURCES =
CLEANFILES =
EXTRA_DIST =
dist_pkgdata_DATA =
pkgdata_DATA =
noinst_LTLIBRARIES =
pkglib_LTLIBRARIES =
bin_PROGRAMS =
bin_SCRIPTS =
dist_doc_DATA =

# we're using non-recursive Make
# include Makefile fragments instead of using SUBDIRS
include assert/Makefile.mk
include climate/Makefile.mk
include commercial/Makefile.mk
#include rest/Makefile.mk
#include comm/Makefile.mk
#include control/Makefile.mk
include core/link/matlab/Makefile.mk
include core/Makefile.mk
include core/rt/Makefile.mk
include core/solvers/Makefile.mk
#include debug/Makefile.mk
include generators/Makefile.mk
include market/Makefile.mk
#include matlab/Makefile.mk
include mysql/Makefile.mk
#include network/Makefile.mk
include optimize/Makefile.mk
#include plc/Makefile.mk
include powerflow/Makefile.mk
include reliability/Makefile.mk
include residential/Makefile.mk
include tape_file/Makefile.mk
include tape/Makefile.mk
include tape_plot/Makefile.mk

# we combine the sources in CBLAS and superLU_MT in a single Makefile fragment
include third_party/superLU_MT/Makefile_combined.mk
#include third_party/CBLAS/Makefile.mk
#include third_party/superLU_MT/Makefile.mk

# for doxygen commands
include aminclude.mk

dist_doc_DATA += COPYRIGHT
dist_doc_DATA += LICENSE

EXTRA_DIST += $(top_srcdir)/debian/changelog
EXTRA_DIST += $(top_srcdir)/debian/compat
EXTRA_DIST += $(top_srcdir)/debian/control
EXTRA_DIST += $(top_srcdir)/debian/copyright
EXTRA_DIST += $(top_srcdir)/debian/docs
EXTRA_DIST += $(top_srcdir)/debian/gridlabd-dev.install
EXTRA_DIST += $(top_srcdir)/debian/gridlabd.install
EXTRA_DIST += $(top_srcdir)/debian/rules*
EXTRA_DIST += $(top_srcdir)/doxygen/build.bat
EXTRA_DIST += $(top_srcdir)/doxygen/COPYRIGHT
EXTRA_DIST += $(top_srcdir)/doxygen/doxygen-doc.vcproj
EXTRA_DIST += $(top_srcdir)/doxygen/footer.html
EXTRA_DIST += $(top_srcdir)/doxygen/gridlabd.conf
EXTRA_DIST += $(top_srcdir)/doxygen/LICENSE
EXTRA_DIST += $(top_srcdir)/doxygen/STATUS
EXTRA_DIST += $(top_srcdir)/doxygen/user_manual.h
EXTRA_DIST += $(top_srcdir)/gridlabd.spec
EXTRA_DIST += $(top_srcdir)/README-LINUX
EXTRA_DIST += $(top_srcdir)/README-MACOSX
EXTRA_DIST += $(top_srcdir)/Resources/Background.png
EXTRA_DIST += $(top_srcdir)/Resources/GridLAB-D.pmdoc/01core-contents.xml
EXTRA_DIST += $(top_srcdir)/Resources/GridLAB-D.pmdoc/01core.xml
EXTRA_DIST += $(top_srcdir)/Resources/GridLAB-D.pmdoc/02climate-contents.xml
EXTRA_DIST += $(top_srcdir)/Resources/GridLAB-D.pmdoc/02climate.xml
EXTRA_DIST += $(top_srcdir)/Resources/GridLAB-D.pmdoc/03libxerces-contents.xml
EXTRA_DIST += $(top_srcdir)/Resources/GridLAB-D.pmdoc/03libxerces.xml
EXTRA_DIST += $(top_srcdir)/Resources/GridLAB-D.pmdoc/04devel-contents.xml
EXTRA_DIST += $(top_srcdir)/Resources/GridLAB-D.pmdoc/04devel.xml
EXTRA_DIST += $(top_srcdir)/Resources/GridLAB-D.pmdoc/index.xml
EXTRA_DIST += $(top_srcdir)/Resources/License.rtf
EXTRA_DIST += $(top_srcdir)/Resources/Readme.rtf
EXTRA_DIST += $(top_srcdir)/Resources/Welcome.rtf
EXTRA_DIST += $(top_srcdir)/VERSION
EXTRA_DIST += $(top_srcdir)/models/climate_csvreader_example.glm
EXTRA_DIST += $(top_srcdir)/models/collector_example.glm
EXTRA_DIST += $(top_srcdir)/models/diesel_deltamode_load_player_A.csv
EXTRA_DIST += $(top_srcdir)/models/diesel_deltamode_load_player_B.csv
EXTRA_DIST += $(top_srcdir)/models/diesel_deltamode_load_player_C.csv
EXTRA_DIST += $(top_srcdir)/models/Four_quadrant_inverter_example.glm
EXTRA_DIST += $(top_srcdir)/models/house_HVAC_example.glm
EXTRA_DIST += $(top_srcdir)/models/IEEE_13_Node_Test_Feeder.glm
EXTRA_DIST += $(top_srcdir)/models/IEEE_13_Node_With_Houses.glm
EXTRA_DIST += $(top_srcdir)/models/passive_controller_example.glm
EXTRA_DIST += $(top_srcdir)/models/random_fault_generator_example.glm
EXTRA_DIST += $(top_srcdir)/models/residential_zipload_example.glm
EXTRA_DIST += $(top_srcdir)/models/Standard_Weather.csv
EXTRA_DIST += $(top_srcdir)/models/subsecond_diesel_generator_example.glm
EXTRA_DIST += $(top_srcdir)/models/taxonomy_feeder_R1-12.47-1.glm
EXTRA_DIST += $(top_srcdir)/models/transactive_controller_example.glm
EXTRA_DIST += $(top_srcdir)/models/WA-Seattle.tmy2
EXTRA_DIST += $(top_srcdir)/models/WA-Yakima.tmy2
EXTRA_DIST += $(top_srcdir)/models/waterheater_example.glm
EXTRA_DIST += $(top_srcdir)/models/weather.csv
EXTRA_DIST += $(top_srcdir)/models/wind_turbine_example.glm
EXTRA_DIST += $(top_srcdir)/core/gridlabd.htm
pkgdata_DATA += $(top_srcdir)/models/climate_csvreader_example.glm
pkgdata_DATA += $(top_srcdir)/models/collector_example.glm
pkgdata_DATA += $(top_srcdir)/models/diesel_deltamode_load_player_A.csv
pkgdata_DATA += $(top_srcdir)/models/diesel_deltamode_load_player_B.csv
pkgdata_DATA += $(top_srcdir)/models/diesel_deltamode_load_player_C.csv
pkgdata_DATA += $(top_srcdir)/models/Four_quadrant_inverter_example.glm
pkgdata_DATA += $(top_srcdir)/models/house_HVAC_example.glm
pkgdata_DATA += $(top_srcdir)/models/IEEE_13_Node_Test_Feeder.glm
pkgdata_DATA += $(top_srcdir)/models/IEEE_13_Node_With_Houses.glm
pkgdata_DATA += $(top_srcdir)/models/passive_controller_example.glm
pkgdata_DATA += $(top_srcdir)/models/random_fault_generator_example.glm
pkgdata_DATA += $(top_srcdir)/models/residential_zipload_example.glm
pkgdata_DATA += $(top_srcdir)/models/Standard_Weather.csv
pkgdata_DATA += $(top_srcdir)/models/subsecond_diesel_generator_example.glm
pkgdata_DATA += $(top_srcdir)/models/taxonomy_feeder_R1-12.47-1.glm
pkgdata_DATA += $(top_srcdir)/models/transactive_controller_example.glm
pkgdata_DATA += $(top_srcdir)/models/WA-Seattle.tmy2
pkgdata_DATA += $(top_srcdir)/models/WA-Yakima.tmy2
pkgdata_DATA += $(top_srcdir)/models/waterheater_example.glm
pkgdata_DATA += $(top_srcdir)/models/weather.csv
pkgdata_DATA += $(top_srcdir)/models/wind_turbine_example.glm
pkgdata_DATA += $(top_srcdir)/core/gridlabd.htm

if MISSING_XERCES
all-local:
	@echo
	@echo "  Missing dependency: libxerces-c"
	@echo ""
	@echo "    libxerces-c was not found during configuration and is a required dependency."
	@echo "    Install the libxerces-c development libraries and re-run ./configure or run"
	@echo "    ./configure with the --with-xerces option.  Try './configure --help' for"
	@echo "    more information."
	@echo
	@false
endif

help:
	@echo "Cleaning targets:"
	@echo "  clean        - Erase from the build tree the files built by 'make all'"
	@echo "  distclean    - Same as 'make clean' plus erase anything ./configure created"
	@echo "  mostlyclean  - Erase intermediate files (such as .o files)"
	@echo ""
	@echo "Building targets:"
	@echo "  all            - Build $(PACKAGE) (same as 'make')"
	@echo "  install        - Install $(PACKAGE) to $(prefix)"
	@echo "  install-strip  - Same as install, but follow by stripping debug symbols"
	@echo "  uninstall      - Erase files installed by 'make install'"
	@echo ""
	@echo "Testing targets:"
	@echo "  validate  - Run the test/validation suite (requires Python)"
	@echo ""
	@echo "Packaging targets:"
	@echo "  dist          - same as 'make dist-gzip'"
	@echo "  dist-bzip2    - Generate a bzip2 tar archive of the distribution"
	@echo "  dist-deb      - Create Debian binary and source deb packages"
	@echo "  dist-deb-bin  - Create Debian binary-only deb package"
	@echo "  dist-deb-src  - Create Debian source-only deb package"
	@echo "  dist-gzip     - Generate a gzip tar archive of the distribution"
	@echo "  dist-lzma     - Generate an lzma tar archive of the distribution"
	@echo "  dist-osx      - Create Mac OS X package installer (OS X w/ XCode only)"
	@echo "  dist-rpm      - Create binary-only RPM package"
	@echo "  dist-shar     - Generate a shar archive of the distribution"
	@echo "  dist-tarZ     - Generate a compressed tar archive of the distribution"
	@echo "  dist-xz       - Generate an xz tar archive of the distribution"
	@echo "  dist-zip      - Generate a zip archive of the distribution"
	@echo "  rpm-prep      - Create \$$HOME/.rpmmacros suitable for building RPMs in"
	@echo "                  \$$HOME/rpmbuild as non-root user"
	@echo ""
	@echo "Documentation targets:"
	@echo "  doxygen-doc  - Generate doxygen documentation (only if doxygen was found"
	@echo "                 by ./configure)"
	@echo ""
	@echo "Developer targets (may require special tools):"
	@echo "  clean-wc   - Recursively remove all files and directories not under"
	@echo "               revision control (will prompt to confirm first)"
	@echo "  ctags      - Generate vi-style tags files"
	@echo "  tags       - Generate emacs-style TAGS files"
	@echo "  maintainer-clean"

over: clean all

# Recursively remove all files and directories not under revision control
clean-wc:
	@echo -e "\nThis removes all unversioned files and directories in the working copy."
	@unset REPLY && read -t 60 -p "Clean working copy (type 'yes' to proceed)? " && test "`echo "$$REPLY" | tr '[:upper:]' '[:lower:]'`" = "yes"
	. utilities/cleanwc

check-local validate: 
	python validate.py

distdir = $(PACKAGE)_$(VERSION)
scratchdir = scratch
XERCES_TARNAME = xerces-c-3.1.1
XERCESCROOT = $(CURDIR)/$(scratchdir)/$(XERCES_TARNAME)

$(scratchdir)/.xerces.stamp: third_party/$(XERCES_TARNAME).tar.gz
	-rm -f $@
	test ! -e $(@D)/$(XERCES_TARNAME) || rm -rf $(@D)/$(XERCES_TARNAME)
	test -d $(@D) || mkdir -p $(@D)
	tar -xzf $< -C $(@D)
	export XERCESCROOT=$(CURDIR)/$(@D)/$(XERCES_TARNAME) && \
		cd $(@D)/$(XERCES_TARNAME) && \
		autoreconf -isf && ./configure && make
	touch $@

$(scratchdir)/pkg: $(scratchdir)/.xerces.stamp distdir
	test ! -e $@ || rm -rf $@
	test -d $@/core || mkdir -p $@/core
	cd $(distdir) && ./configure --with-xerces=$(CURDIR)/$(scratchdir)/$(XERCES_TARNAME) && make install DESTDIR=$(CURDIR)/$@/core
	cd $@ && mkdir -p climate-data/usr/lib/gridlabd devel/usr/lib/gridlabd libxerces-c/usr/lib
	mv $@/core/usr/lib/gridlabd/*.zip $@/climate-data/usr/lib/gridlabd/
	mv $@/core/usr/include $@/devel/usr/include
	mv $@/core/usr/lib/gridlabd/*.la $@/devel/usr/lib/gridlabd/
	cp $(scratchdir)/$(XERCES_TARNAME)/lib/libxerces-c.* $@/libxerces-c/usr/lib/
	$(am__remove_distdir)

$(scratchdir)/$(PACKAGE_TARNAME).mpkg: $(scratchdir)/pkg
	env PATH=$$PATH:/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS \
	PackageMaker --doc Resources/GridLAB-D.pmdoc --version "$(PACKAGE_VERSION)" --title "$(PACKAGE_NAME)" --out $@

$(scratchdir)/$(PACKAGE_TARNAME).dmg: $(scratchdir)/$(PACKAGE_TARNAME).mpkg
	test ! -e $@ || rm -f $@
	hdiutil create -size $$((`du -sm $< | cut -f 1` + 3))m -fs HFS+ -volname "$(PACKAGE_STRING)" $@
	hdiutil attach -mountpoint $(@D)/mnt $@
	mv $< $(@D)/mnt/
	hdiutil detach $(@D)/mnt

$(distdir).dmg: $(scratchdir)/$(PACKAGE_TARNAME).dmg
	test ! -e $@ || rm -rf $@
	hdiutil convert -format UDZO -o $@ $<
	rm -f $<
	-rmdir $(<D)

clean-dist-osx:
	-test ! -d $(scratchdir)/mnt || hdiutil detach $(scratchdir)/mnt
	-rm -rf $(scratchdir)

# Build a .mpkg installer package for Mac OS X
dist-osx:
	trap '$(MAKE) clean-dist-osx' EXIT && \
	$(MAKE) $(distdir).dmg

# Build deb (binary and source) packages
dist-deb: distdir
	echo -e "gridlabd ($(VERSION)-1) unstable; urgency=low\n\n  * Version $(VERSION) release.\n  * View recent changes at: https://sourceforge.net/apps/mediawiki/gridlab-d/index.php?title=Special:RecentChanges\n\n -- GridLAB-D Team <gridlabd@pnl.gov> `date -R`\n" > $(distdir)/debian/changelog
	ln -s $(distdir) $(distdir)-1
	(cd $(distdir)-1 && dpkg-buildpackage -tc -us -uc $(DEBFLAGS))
	rm -f $(distdir)-1
	$(am__remove_distdir)

# Build Debian deb binary package
dist-deb-bin:
	$(MAKE) dist-deb DEBFLAGS=-b

# Build Debian deb source package
dist-deb-src:
	$(MAKE) dist-deb DEBFLAGS=-S

# Build RPM binary package
dist-rpm: dist-gzip
	rpmbuild -tb $(distdir).tar.gz

RPMBUILDDIR = $(HOME)/rpmbuild

# Create $(HOME)/.rpmmacros file suitable for building RPMs in $(RPMBUILDDIR) as non-root user
rpm-prep:
	@DIR="$(RPMBUILDDIR)" && if [ "$${DIR:0:1}" != "/" ]; then \
		echo >&2 "ERROR: RPMBUILDDIR must be an absolute path"; exit 1; \
	fi
	@if [ -f "$$HOME/.rpmmacros" ]; then \
		sed -i 's/^\(%_topdir\)/#\1/' $$HOME/.rpmmacros; \
		echo >&2 "WARNING: $$HOME/.rpmmacros already exists; commenting out _topdir macros"; \
	fi
	@echo "%_topdir $(RPMBUILDDIR)" >> $$HOME/.rpmmacros
	@echo "RPMs will be built in $(RPMBUILDDIR)"
	@for DIR in $(RPMBUILDDIR)/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}; do \
		test -d "$$DIR" || mkdir -p "$$DIR"; \
	done


libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status libtool

