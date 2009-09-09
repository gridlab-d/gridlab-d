install-data-local:
		$(INSTALL_DATA) $(srcdir)/gridlabd.conf $(prefix)/etc/gridlabd/rt
		$(INSTALL_DATA) $(srcdir)/debugger.conf $(prefix)/etc/gridlabd/rt
		$(INSTALL_DATA) $(srcdir)/gnuplot.conf $(prefix)/etc/gridlabd/rt
		$(INSTALL_DATA) $(srcdir)/gridlabd.h $(prefix)/etc/gridlabd/rt
		echo "export GRIDLABD=$(prefix)/etc/gridlabd" > /etc/profile.d/gridlabd.sh
		echo "export GLPATH=$(prefix)/etc/gridlabd:$(libdir)/gridlabd:$(prefix)/etc/gridlabd/rt:$(prefix)/etc/gridlabd/tmy" >> /etc/profile.d/gridlabd.sh