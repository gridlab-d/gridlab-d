BUILDING GRIDLAB WITH THE AUTO-TOOLSET UNDER LINUX (AND OTHER UNIX VARIANTS)
$Id: BUILD 1182 2008-12-22 22:08:36Z dchassin $
Copyright (C) 2008 Battelle Memorial Institute

You must have autoconf, automake, and libtool installed on your system for
this process to work.  You must also have xerces-c and cppunit installed on
your system.

First time builds:

	linux% cd <source-folder>
	linux% ./configure
	linux% make

Installs to system folders:

	linux% cd <source-folder>
	linux% sudo make install

Installs to non-system folders:

	linux% cd <source-folder>
	linux% sudo make install prefix=<install-path>

You will have to use the following exports for non-system installs to work
properly:

	linux% export PATH=<install-path>/bin:$PATH
	linux% export GLPATH=<install-path>/etc/gridlabd
	linux% export LD_LIBRARY_PATH=<install-path>/lib/gridlabd

You can test the install by running the residential sample model:

	linux% cd <source-folder>/models
	linux% gridlabd residential_loads

Rebuilding after changing makefiles and/or modules configurations

	linux% cd <source-folder>
	linux% autoreconf -isf
	linux% ./configure
	linux% make

To create a source-based distribution file(named gridlabd-src-#_#_###.tar.gz):

	linux% cd <source-folder>/..
	linux% source/utilities/release-src -t . -s <source-folder>

See README-LINUX file for details.
