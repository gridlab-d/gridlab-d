# $Id: Makefile.am,v 1.21 2008/01/22 22:01:09 d3p181 Exp $
# Copyright (C) 2008 Battelle Memorial Institute
# This file is distributed under the same terms as GridLAB-D.

ACLOCAL_AMFLAGS = -I m4
SUBDIRS =  core assert climate commercial network powerflow residential tape \
			  generators plc tape_file tape_plot market reliability
# previously:  core assert climate commercial network powerflow residential tape generators plc tape_file tape_plot market reliability 

include aminclude.am

docdir = ${datarootdir}/doc/${PACKAGE_TARNAME}

dist_doc_DATA = BUILD COPYRIGHT LICENSE README-LINUX README-MACOSX

EXTRA_DIST = doxygen/*.* VERSION third_party/CBLAS third_party/superLU_MT

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

#.PHONY: over clean-wc validate

