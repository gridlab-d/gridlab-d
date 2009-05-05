# $Id$
#
# Makefile

SOURCE=rredc.nrel.gov

tmy: tmy2_pc.zip
	unzip -qq -u tmy2_pc \*.tm2
	. ./extract_tmy *.tm2
	. ./build_pkgs
	rm -f *.glm *.tmy2 *.tm2*

tmy2_pc.zip:
	wget -N http://$(SOURCE)/solar/old_data/nsrdb/tmy2/compressed/tmy2_pc.zip -o wget.log -O tmy2_pc.zip

clean:
	rm -f *.glm *.tmy2 *.zip

