pkgsysconfdir = $(sysconfdir)/$(PACKAGE)
pkgrtdir = $(pkgsysconfdir)/rt
dist_pkgrt_DATA = gridlabd.conf debugger.conf gnuplot.conf gridlabd.h

install-data-local:
	[ -d "$(DESTDIR)$(pkgrtdir)" ] || $(MKDIR_P) $(DESTDIR)$(pkgrtdir)
	echo "GLROOT=''" > $(DESTDIR)$(pkgrtdir)/gridlabd.sh
	echo "#export GLROOT=$(DESTDIR)" >> $(DESTDIR)$(pkgrtdir)/gridlabd.sh
	echo "#export PATH=\$$PATH:\$$GLROOT$(bindir)" >> $(DESTDIR)$(pkgrtdir)/gridlabd.sh
	echo "export GRIDLABD=\$$GLROOT$(pkgsysconfdir)" >> $(DESTDIR)$(pkgrtdir)/gridlabd.sh
	echo "export GLPATH=\$$GLROOT$(pkgsysconfdir):\$$GLROOT$(pkglibdir):\$$GLROOT$(pkgrtdir):\$$GLROOT$(pkgsysconfdir)/tmy" >> $(DESTDIR)$(pkgrtdir)/gridlabd.sh
	if [ -d "$(DESTDIR)$(sysconfdir)/profile.d" ]; then \
		$(LN_S) $(pkgsysconfdir)/gridlabd.sh $(DESTDIR)$(sysconfdir)/profile.d/gridlabd.sh; \
	elif [ -f "$(DESTDIR)$(sysconfdir)/profile" ]; then \
		echo ". $(pkgsysconfdir)/gridlabd.sh" >> $(DESTDIR)$(sysconfdir)/profile; \
	fi

uninstall-local:
	-[ ! -f "$(DESTDIR)$(sysconfdir)/profile" ] || $(SED) -i '\,^. $(pkgsysconfdir)/gridlabd\.sh$$,d' $(DESTDIR)$(sysconfdir)/profile
	-[ ! -d "$(DESTDIR)$(sysconfdir)/profile.d" ] || rm -f $(DESTDIR)$(sysconfdir)/profile.d/gridlabd.sh
	-rm -f $(DESTDIR)$(pkgrtdir)/gridlabd.sh

uninstall-hook:
	-rmdir $(DESTDIR)$(pkgrtdir)
	-rmdir $(DESTDIR)$(pkgsysconfdir)

