#!/anaconda/bin/python
import os
import gridlabd
import random
import json

#
# Get the time from the environment (must be exported by GLM)
#
t = os.getenv("clock")
print '\n\nDate/Time {:20}'.format(t)
print '================================='
print 'Object       Tair  Theat  Tcool    Tdb   Taux Mode'
print '---------- ------ ------ ------ ------ ------ ----'
#
# Process the list of houses
#
houses = gridlabd.get('houselist')
for name in houses['houselist'].split(';') :

	# Get the house data
	house = gridlabd.get(name,'*')
	
	# thermostat control
	Ta = gridlabd.get_double(house,'air_temperature')
	Th = gridlabd.get_double(house,'heating_setpoint')
	Tc = gridlabd.get_double(house,'cooling_setpoint')
	Td = gridlabd.get_double(house,'thermostat_deadband')
	Tx = gridlabd.get_double(house,'aux_heat_deadband')
	M = house['system_mode']
	
	# show values
	print '{:10} {:6.1f} {:6.1f} {:6.1f} {:6.1f} {:6.1f} {:4}'.format(name,Ta,Th,Tc,Td,Tx,M)
	
	#
	# Run thermostat logic
	#
	if M=='OFF':
		if Ta > Tc + Td/2 :
			gridlabd.set(name,'system_mode','COOL')
		elif Ta < Th - Td/2 :
			gridlabd.set(name,'system_mode','HEAT')
	elif M == 'COOL' :
		if Ta < Tc - Td/2 :
			gridlabd.set(name,'system_mode','OFF')
	elif M == 'HEAT' :
		if Ta > Th + Td/2 :
			gridlabd.set(name,'system_mode','OFF')
		elif Ta < Th - Tx/2 :
			gridlabd.set(name,'system_mode','AUX')
	elif M == 'AUX' :
		if Ta > Th + Td/2 :
			gridlabd.set(name,'system_mode','OFF')
	else :
		print "WARNING: tstat mode {} is invalid, changing mode to OFF".format(M)
		set(name,'system_mode','OFF')

