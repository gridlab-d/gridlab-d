#  This file is part of libodbc++.
#  
#  Copyright (C) 1999-2000 Manush Dodunekov <manush@stendahls.net>
#   
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Library General Public
#  License as published by the Free Software Foundation; either
#  version 2 of the License, or (at your option) any later version.
#   
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Library General Public License for more details.
#  
#  You should have received a copy of the GNU Library General Public License
#  along with this library; see the file COPYING.  If not, write to
#  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
#  Boston, MA 02111-1307, USA.

pkgincludedir=@includedir@/odbc++

pkginclude_HEADERS=\
config.h \
setup.h \
types.h \
threads.h \
errorhandler.h \
drivermanager.h \
connection.h \
databasemetadata.h \
statement.h \
preparedstatement.h \
callablestatement.h \
resultset.h \
resultsetmetadata.h

config.h: $(CONFIG_HEADER) config.h.in
	cat $(srcdir)/config.h.in > $@
	sed -e 's/#\([^ ][^ ]*\) \([^ ][^ ]*\)/#\1 ODBCXX_\2/g' >> $@ < $(CONFIG_HEADER)


EXTRA_DIST=config.h.in config-win32.h

CLEANFILES=config.h

# nuke our generated config file
dist-hook:
	rm -f $(distdir)/config.h

uninstall-local:
	test -d $(DESTDIR)$(pkgincludedir) && \
	rmdir $(DESTDIR)$(pkgincludedir) || true
