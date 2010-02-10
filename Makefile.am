# $Id: Makefile.am,v 1.21 2008/01/22 22:01:09 d3p181 Exp $
# Copyright (C) 2008 Battelle Memorial Institute
# This file is distributed under the same terms as GridLAB-D.

ACLOCAL_AMFLAGS = -I m4
SUBDIRS =  core assert climate commercial network powerflow residential tape \
			  generators plc tape_file tape_plot market reliability
# previously:  core assert climate commercial network powerflow residential tape generators plc tape_file tape_plot market reliability 

include aminclude.am

dist_doc_DATA = BUILD COPYRIGHT LICENSE README-LINUX README-MACOSX

EXTRA_DIST = doxygen/*.* VERSION third_party/CBLAS third_party/superLU_MT

over: clean all

repo-clean:
	svn status | grep '^?' | cut -c 8- | xargs rm

uninstall-hook:
	-rmdir $(DESTDIR)$(sysconfdir)/$(PACKAGE)/rt
	-rmdir $(DESTDIR)$(sysconfdir)/$(PACKAGE)
	-rmdir $(DESTDIR)$(pkglibdir)

validate: 
	python validate.py

compare-source:
	bash ./compare-sources $(SUBDIRS)

