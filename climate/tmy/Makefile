# $Id$
#
# Makefile

SOURCE=rredc.nrel.gov

tmy:
	wget -N http://$(SOURCE)/solar/old_data/nsrdb/tmy2/compressed/tmy2_pc.zip -o wget.log -O tmy2_pc.zip
	unzip -u tmy2_pc
	. ./extract_tmy *.tm2

clean:
	rm *.glm *.tmy2
