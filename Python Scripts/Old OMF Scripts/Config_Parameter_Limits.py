'''The limits imposed on the calibration parameters.'''

# limits for Average House (VA)
res_high = 15000
res_low = 3000

# limits for Average Commercial (VA)
com_high = 35000
com_low = 8000

# limits for residential base load scaling (%)
# -- parameter value scales upward. i.e. scaling 100% will increase base load by 100%. Scaling by -50% will decrease base load by 50%.
base_scale_high = 1
base_scale_low = -0.50

# limits for decreasing gas heating penetration (%)
# -- parameter value scales gas heating downward. i.e. scaling 100% decreases penetration by 100% (no gas heat).
less_gas_low = -1
less_gas_high = 1

# band (plus or minus limit) limiting additional degrees added/subtracted from heating set points.
addDegreeBand = 4

# limits for schedule skew standard deviation (seconds)
schedSkewStd_low = 2700
schedSkewStd_high = 5400

# limits for scaling COP high and low values.
# -- parameter value scales upward
COPlim_high = 0.75
COPlim_low = 0

# band (plus or minus limit) limiting set point offsets (degrees F)
offset_band = 4

# limits for window_wall_ratio (%)
window_wall_low = 0
window_wall_high = 1

# band (plus or minus limit) limiting residential schedule skew shift (seconds)
schedskewband = 7200


def avgHouseLIM (x):
	'''Return true if average house VA is within limits.'''
	if x >= res_low and x <= res_high:
		return 1
	else:
		return 0
		
def avgCommercialLIM (x):
	'''Return true if average commercial VA is within limits.'''
	if x >= com_low and x <= com_high:
		return 1
	else:
		return 0
		
def resBaseLoadLIM (x):
	'''Return true if residential base load scalar is within limits.'''
	if x >= base_scale_low and x <= base_scale_high:
		return 1
	else:
		return 0
		
def OffsetsLIM (x,y):
	'''Return true if set point offsets are within limits and heating offset is greater than or equal to cooling offset.'''
	if x >= y and abs(x) <= offset_band and abs(y) <= offset_band:
		return 1
	else:
		return 0
		
def COPvalsLIM (x,y):
	'''Return true if COP high and low scalars are within limits and COP high scalar is greater than or equal to COP low scalar.'''
	if x >= COPlim_low and x <= COPlim_high and y >= COPlim_low and y <= COPlim_high and y <= x:
		return 1
	else:
		return 0
		
def resSchedSkewLIM (x):
	'''Return true if residential schedule skew is within limits.'''
	if abs(x) < schedskewband:
		return 1
	else:
		return 0
	
def gasHeatPercLIM (x):
	'''Return true if gas heating penetration scalar is within limits.'''
	if x >= less_gas_low and x <= less_gas_high:
		return 1
	else:
		return 0
		
def SchedSkewSTDLIM (x):
	'''Return true if schedule skew standard deviation is within limits.''' 
	if x >= schedSkewStd_low and x <= schedSkewStd_high:
		return 1
	else:
		return 0
		
def windowWallRatioLIM (x):
	'''Return true if window wall ratio is within limits.'''
	if x >= window_wall_low and x <= window_wall_high:
		return 1
	else:
		return 0

def addHeatDegreesLIM (x):
	'''Return true if additional heating set point degree value is within limits.'''
	if abs(x) < addDegreeBand:
		return 1
	else:
		return 0
		
if __name__ ==  '__main__':
	print(__doc__)
	print("Average House (VA) in ["+str(res_low)+","+str(res_high)+"].")
	print("Average Commercial (VA) in ["+str(com_low)+","+str(com_high)+"].")
	print("Residential base load scaled an additional percentage in ["+str(base_scale_low * 100)+"%,"+str(base_scale_high * 100)+"%].")
	print("Gas Heating penetration lessened by percentage in ["+str(less_gas_low*100)+"%,"+str(less_gas_high*100)+"%].")
	print("Additional degrees added to heating set points in ["+str(-addDegreeBand)+","+str(addDegreeBand)+"].")
	print("Schedule skew standard deviation (seconds) in ["+str(schedSkewStd_low)+","+str(schedSkewStd_high)+"].")
	print("COP high and COP low values scaled an additional percentage in ["+str(COPlim_low * 100)+"%,"+str(COPlim_high * 100)+"%].")
	print("Heating and Cooling set point offsets in ["+str(-offset_band)+","+str(offset_band)+"].")
	print("Window wall ratio in ["+str(window_wall_low)+","+str(window_wall_high)+"].")
	print("Residential schedule skew (seconds) in ["+str(-schedskewband)+","+str(schedskewband)+"].")
