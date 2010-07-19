# $Id: Makefile.am,v 1.21 2008/01/22 22:01:09 d3p181 Exp $
# Copyright (C) 2008 Battelle Memorial Institute
# This file is distributed under the same terms as GridLAB-D.

ACLOCAL_AMFLAGS = -I m4
SUBDIRS = third_party/CBLAS third_party/superLU_MT core assert climate \
			 commercial network powerflow residential tape generators plc \
			 tape_file tape_plot market reliability

include aminclude.am

distdir = $(PACKAGE)_$(VERSION)

dist_doc_DATA = COPYRIGHT LICENSE

EXTRA_DIST = debian doxygen/*.* gridlabd.spec VERSION README-LINUX README-MACOSX

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)
	-rmdir $(DESTDIR)$(docdir)

over: clean all

clean-wc:
	@echo -e "\nThis removes all unversioned files and directories in the working copy."
	@unset REPLY && read -t 60 -p "Clean working copy (type 'yes' to proceed)? " && test "`echo "$$REPLY" | tr '[:upper:]' '[:lower:]'`" = "yes"
	. utilities/cleanwc

validate: 
	python validate.py

dist-deb: distdir
	echo -e "gridlabd ($(VERSION)-1) unstable; urgency=low\n\n  * Version $(VERSION) release.\n  * View recent changes at: https://sourceforge.net/apps/mediawiki/gridlab-d/index.php?title=Special:RecentChanges\n\n -- GridLAB-D Team <gridlabd@pnl.gov> `date -R`\n" > $(distdir)/debian/changelog
	ln -s $(distdir) $(distdir)-1
	(cd $(distdir)-1 && dpkg-buildpackage -tc -us -uc)
	rm -f $(distdir)-1
	$(am__remove_distdir)

dist-rpm: dist-gzip
	rpmbuild -tb $(distdir).tar.gz

RPMBUILDDIR = $(HOME)/rpmbuild

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

#.PHONY: over clean-wc validate

