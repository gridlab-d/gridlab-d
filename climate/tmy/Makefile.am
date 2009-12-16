
pkgsysconfdir = $(sysconfdir)/$(PACKAGE)
tmydir = $(pkgsysconfdir)/tmy
dist_tmy_DATA = ??.zip
dist_noinst_SCRIPTS = extract_tmy build_pkgs

TMY_SITE = rredc.nrel.gov

install-data-hook:
	mkdir -p $(DESTDIR)$(tmydir)
	cd $(DESTDIR)$(tmydir) && for f in ??.zip; do unzip -qq $$f; done

uninstall-local:
	-cd $(DESTDIR)$(tmydir) && rm ??-*.glm ??-*.tmy2

uninstall-hook:
	-rmdir $(DESTDIR)$(tmydir)
	-rmdir $(DESTDIR)$(pkgsysconfdir)

tmy: tmy2_pc.zip
	unzip -qq -u tmy2_pc \*.tm2
	. ./extract_tmy *.tm2
	. ./build_pkgs
	rm -f *.glm *.tmy2 *.tm2*

tmy2_pc.zip:
	wget -N http://$(TMY_SITE)/solar/old_data/nsrdb/tmy2/compressed/tmy2_pc.zip -o wget.log -O tmy2_pc.zip

clean-tmy:
	rm -f *.glm *.tmy2 *.zip

