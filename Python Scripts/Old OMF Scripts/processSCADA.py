'''Process user-inputted SCADA data.

Currently, this is just a place holder for a function that will take the name of the SCADA file and return the necessary measurement points for use in the calibration function.

'''

def getValues(SCADAinput):
	if SCADAinput == None:
		# From the SCADA available, choose three days representing summer, winter, and spring (shoulder).
		# Ideally, summer and winter days are seasonal peak days.
		summer_day = '2012-06-29'
		winter_day = '2012-01-19'
		shoulder_day = '2012-04-10'
		# summer day measurements
		su_peak_value = 5931.36  # kW
		su_total_energy = 107380.8 # kWh
		su_peak_time = 17  # hour
		su_minimum_value = 2988  # kW
		su_minimum_time = 6  # hour
		# winter day measurements
		wi_peak_value = 3646.08  # kW
		wi_total_energy = 75604.32  # kWh
		wi_peak_time = 21  # hour
		wi_minimum_value = 2469.60  # kW
		wi_minimum_time = 1  # hour
		# spring day measurements
		sh_peak_value = 2518.56  # kW
		sh_total_energy = 52316.64  #kWh
		sh_peak_time = 21  # hour
		sh_minimum_value = 1738.08  # kW
		sh_minimum_time = 2  # hour
	else:
		summer_day = SCADAinput['summerDay']
		winter_day = SCADAinput['winterDay']
		shoulder_day = SCADAinput['shoulderDay']
		# summer day measurements
		su_peak_value = SCADAinput['summerPeakKW']  # kW
		su_total_energy = SCADAinput['summerTotalEnergy'] # kWh
		su_peak_time = SCADAinput['summerPeakHour']  # hour
		su_minimum_value = SCADAinput['summerMinimumKW']  # kW
		su_minimum_time = SCADAinput['summerMinimumHour']  # hour
		# winter day measurements
		wi_peak_value = SCADAinput['winterPeakKW']  # kW
		wi_total_energy = SCADAinput['winterTotalEnergy']  # kWh
		wi_peak_time = SCADAinput['winterPeakHour']  # hour
		wi_minimum_value = SCADAinput['winterMinimumKW']  # kW
		wi_minimum_time = SCADAinput['winterMinimumHour']  # hour
		# spring day measurements
		sh_peak_value = SCADAinput['shoulderPeakKW']  # kW
		sh_total_energy = SCADAinput['shoulderTotalEnergy']  #kWh
		sh_peak_time = SCADAinput['shoulderPeakHour']  # hour
		sh_minimum_value = SCADAinput['shoulderMinimumKW']  # kW
		sh_minimum_time = SCADAinput['shoulderMinimumHour']  # hour
	# It is important that values are returned in the correct order.
	return [summer_day, winter_day, shoulder_day], [ [su_peak_value, su_peak_time, su_total_energy, su_minimum_value, su_minimum_time],[wi_peak_value, wi_peak_time, wi_total_energy, wi_minimum_value, wi_minimum_time], [sh_peak_value, sh_peak_time, sh_total_energy, sh_minimum_value, sh_minimum_time] ]
	
def main():
	days, SCADA = getValues(None);
	print (__doc__)
	print ("Three chosen days are "+str(days))
	print ("Peak Value (kw), Peak Time (hour), Total Energy (kwh), Minimum Value (kw), Minimum Time (hour) for: ")
	print ("Summer " +str(SCADA[0]))
	print ("Winter " +str(SCADA[1]))
	print ("Shoulder " +str(SCADA[2]))

if __name__ ==  '__main__':
	main();