from __future__ import division
import Config_Parameter_Limits as limits

# Main function takeAction(action,vals,diffs) takes action code, list of previously used calibration parameter values, and list of calculated differences for four main metrics. 
# Returns a list of lists, each is a set of calibration parameter values to try. 

def loadLevel (a, a_count, avg_house, avg_comm, base_load_scalar, avg_diff):
	options = [None] * 7
	for i in xrange(7):
		options[i] = [base_load_scalar, avg_house, avg_comm]
	#change = 0.10 + (abs(avg_diff) - (abs(avg_diff) % 0.10))
	if abs(avg_diff) > 0.50:
		scalar = 3
	elif abs(avg_diff) > 0.25:
		scalar = 2
	else:
		scalar = 1
	if a_count > 1 and scalar > 1:
		scalar -= 1
	change_r = 1000 * scalar
	change_c = 4000 
	change_base_load = 0.05;
	if a < 0:
		a0 = "To lower load overall, we can ";
		a1 = "increase";
		a2 = "decrease";
		change_base_load = -change_base_load
		ch = 1
	else:
		a0 = "To increase load overall, we can ";
		a1 = "decrease";
		a2 = "increase";
		#change = -change
		ch = -1
	print (a0 + a1 + " average house VA and/or average commercial VA, and/or "+ a2 + " residential base load scalar.");
	options[0][0] = round(base_load_scalar + change_base_load,2) # change base load scalar only
	# options[1][1] = int(avg_house + (avg_house * change)) # change avg house only
	# options[2][2] = int(avg_comm + (avg_comm * change)) # change avg comm only
	# options[3][1] = int(avg_house + (avg_house * change)) # change avg res and avg comm equally
	# options[3][2] = int(avg_comm + (avg_comm * change))
	# options[4][1] = int(avg_house + (avg_house * 1.5 * change)) # change avg res more than avg comm
	# options[4][2] = int(avg_comm + (avg_comm * 0.5 * change))
	# options[5][2] = int(avg_comm + (avg_comm * 1.5 * change)) # change avg com more than avg res
	# options[5][1] = int(avg_house + (avg_house * 0.5 * change))
	options[6][0] = round(base_load_scalar + (change_base_load * 2),2) # change base load double
	options[1][1] = int(avg_house + ch * change_r) # change avg house only
	options[2][2] = int(avg_comm + ch * change_c) # change avg comm only
	options[3][1] = int(avg_house + ch * change_r) # change avg res and avg comm
	options[3][2] = int(avg_comm + ch * change_c)
	options[4][1] = int(avg_house + (2 * ch * change_r)) # change avg res double
	options[5][2] = int(avg_comm + (2 * ch *change_c)) # change avg com double
	# constrain the values within our set limits
	for i in xrange(7):
		if not limits.resBaseLoadLIM(options[i][0]):
			if a < 0:
				options[i][0] = limits.base_scale_low
			else:
				options[i][0] = limits.base_scale_high
		if not limits.avgHouseLIM(options[i][1]):
			if a < 0:
				options[i][1] = limits.res_high
			else:
				options[i][1] = limits.res_low
		if not limits.avgCommercialLIM(options[i][2]):
			if a < 0:
				options[i][2] = limits.com_high
			else:
				options[i][2] = limits.com_low
	# get rid of any duplicates
	final = list(map(list, set(map(tuple,options))))
	return final

def winterLoad (a, decrease_gas, add_heat_degree):
	options = [None] * 6
	for i in xrange(6):
		options[i] = [decrease_gas, add_heat_degree]
	if a < 0:
		a0 = "To lower winter load only, we can ";
		a1 = "increase";
		a2 = "fewer";
		ch = -1
	else:
		a0 = "To increase winter load only, we can ";
		a1 = "decrease";
		a2 = "more";
		ch = 1
	print (a0 + a1 + " gas heating penetration, and/or add " + a2 + " degrees to the heating set points.");
	options[0][0] = round(decrease_gas + (ch * 0.05),2)   # change gas heat only
	options[1][0] = round(decrease_gas + (ch * 0.10),2)   # change gas heat double
	options[2][1] = add_heat_degree + (ch * 0.5) # change heat set only
	options[3][1] = add_heat_degree + (ch * 1)   # change heat set double
	options[4][0] = round(decrease_gas + (ch * 0.05),2)
	options[4][1] = add_heat_degree + (ch * 0.5) # change gas and heat
	options[5][0] = round(decrease_gas + (ch * 0.10),2)
	options[5][1] = add_heat_degree + (ch * 1)   # change gas and heat double
	# constrain the values within our set limits
	for i in xrange(6):
		if not limits.gasHeatPercLIM(options[i][0]):
			if a < 0:
				options[i][0] = limits.less_gas_low
			else:
				options[i][0] = limits.less_gas_high
		if not limits.addHeatDegreesLIM(options[i][1]):
			if a < 0:
				options[i][1] = -(limits.addDegreeBand)
			else:
				options[i][1] = limits.addDegreeBand
	# get rid of any duplicates
	final = list(map(list, set(map(tuple,options))))
	return final

def winterPeak (a, decrease_gas, sched_skew_std, add_heat_degree):
	winterload_ops = winterLoad(a, decrease_gas, add_heat_degree)[::2]
	options = []
	if a < 0:
		a0 = "To lower winter peak, we can "
		a1 = "increase"
		ch = 1
	else:
		a0 = "To increase winter peak, we can "
		a1 = "decrease"
		ch = -1
	print (a0 + a1 + " schedule skew standard deviation.")
	# test two different changes 
	val1 = sched_skew_std + ch * 900
	val2 = sched_skew_std + ch * 1800
	# constrain the values within our set limits
	if not limits.SchedSkewSTDLIM(val1):
		if a < 0:
			val1 = limits.schedSkewStd_high
		else:
			val1 = limits.schedSkewStd_low
	if not limits.SchedSkewSTDLIM(val2):
		if a < 0:
			val2 = limits.schedSkewStd_high
		else:
			val1 = limits.schedSkewStd_low
	# for each option from winterLoad, test each schedule skew std value
	for i in winterload_ops:
		options.extend([i + [val1],i + [val2]])
	# get rid of any duplicates
	final = list(map(list, set(map(tuple,options))))
	return final

def summerPeak (a, window_wall_ratio):
	options = [None] * 2
	for i in xrange(2):
		options[i] = [window_wall_ratio]
	if a < 0:
		a0 = "To lower summer peak, we can "
		a1 = "decrease"
		ch = -1
	else:
		a0 = "To increase summer peak, we can "
		a1 = "increase"
		ch = 1
	print (a0 + a1 + " window wall ratio.");
	options[0][0] = round(window_wall_ratio + ch * 0.05, 2)
	options[1][0] = round(window_wall_ratio + ch * 0.10, 2)
	for i in xrange(2):
		if not limits.windowWallRatioLIM(options[i][0]):
			if a < 0:
				options[i][0] = limits.window_wall_low;
			else:
				options[i][0] = limits.window_wall_high;
	final = list(map(list, set(map(tuple,options))))
	return final
	
def peakLevel (a, cool_offset, heat_offset, cop_high, cop_low, sched_skew_std):
	options = [None] * 7
	for i in xrange(7):
		options[i] = [cool_offset, heat_offset, cop_high, cop_low, sched_skew_std]
	if a < 0:
		a0 = "To lower peaks, we can "
		a1 = "decrease"
		a2 = "increase"
		a3 = "increase"
		ch = -1
	else:
		a0 = "To increase peaks, we can "
		a1 = "increase"
		a2 = "decrease"
		a3 = "decrease"
		ch = 1
	print (a0 + a1 + " set point offsets, " + a2 + " COP values, " + a3 + " schedule skew standard deviation.")
	
	cool_offset_new = cool_offset + ch * 0.5
	heat_offset_new = heat_offset + ch * 0.5
	cop_high_new = round(cop_high - ch * 0.05,2)
	cop_low_new = round(cop_low - ch * 0.05,2)
	sched_skew_std_new = sched_skew_std - ch * 900
	
	# Changing COP values together and offsets together for now.	
	options[0][0:2] = cool_offset_new, heat_offset_new  # change offsets 
	options[1][2:4] = cop_high_new, cop_low_new  # change COP scaling
	options[2][4] = sched_skew_std_new  # change schedule skew std
	options[3][0:4] = cool_offset_new, heat_offset_new, cop_high_new, cop_low_new  # change offsets and COP scaling
	options[4][0], options[4][1], options[4][4] = cool_offset_new, heat_offset_new, sched_skew_std_new  # change offsets and schedule skew std
	options[5][2:5] = cop_high_new, cop_low_new, sched_skew_std_new  # change COP scaling and schedule skew std
	options[6] = [cool_offset_new, heat_offset_new, cop_high_new, cop_low_new, sched_skew_std_new]  # change COP scaling, offsets, and schedule skew std
	
	for i in xrange(7):
		if not limits.OffsetsLIM(options[i][1],options[i][0]):  # Will need modification if changing offsets independently.
			if a < 0:
				options[i][0],options[i][1] = -limits.offset_band, -limits.offset_band
			else:
				options[i][0],options[i][1] = limits.offset_band, limits.offset_band
		if not limits.COPvalsLIM(options[i][2],options[i][3]):  # Will need modification if changing COP high and COP low independently.
			if a < 0:
				options[i][2], options[i][3] = limits.COPlim_high, limits.COPlim_high
			else:
				options[i][2], options[i][3] = limits.COPlim_low, limits.COPlim_low
		if not limits.SchedSkewSTDLIM(options[i][4]):
			if a < 0:
				options[i][4] = limits.schedSkewStd_high
			else:
				options[i][4] = limits.schedSkewStd_low
	# Get rid of any duplicates that were created after limits imposed. 
	final = list(map(list, set(map(tuple,options))))
	return final

def winterPeaksummerOK (a, cop_high, cop_low, decrease_gas, add_heat_degree ):
	#peakLevel(a, cool_offset, heat_offset, cop_high, cop_low, sched_skew_std ) # if we have the computing capacity, this would be best
	options = []
	winterload_ops = winterLoad(a, decrease_gas, add_heat_degree)[::2]
	if a < 0:
		a0 = "Since we can also lower summer peak, we can "
		a1 = "increase" # COP values
		ch = -1
	else:
		a0 = "Since we can also raise summer peak, we can "
		a1 = "decrease" # COP values
		ch = 1
	print (a0 + a1 + " COP values.")
	cop_high_new = round(cop_high - ch * 0.05,2)
	cop_low_new = round(cop_low - ch * 0.05,2)
	if not limits.COPvalsLIM(cop_high_new,cop_low_new):  # Will need modification if changing COP high and COP low independently.
		if a < 0:
			cop_high_new, cop_low_new = limits.COPlim_high, limits.COPlim_high
		else:
			cop_high_new, cop_low_new = limits.COPlim_low, limits.COPlim_low
	for i in winterload_ops:
		options.extend([i + [cop_high_new, cop_low_new], i + [cop_high, cop_low]])
	final = list(map(list, set(map(tuple,options))))
	return final
		
def summerPeakwinterOK (a, cop_high, cop_low, window_wall_ratio ):
	options = [None] * 6
	for i in xrange(6):
		options[i] = [cop_high, cop_low, window_wall_ratio]
	if a < 0:
		a0 = "To lower summer peak (lowering winter is OK too), we can "
		a1 = "increase" # COP values
		a2 = "decrease" # window wall ratio
		ch = -1
	else:
		a0 = "To increase summer peak (increasing winter is OK too), we can "
		a1 = "decrease" # COP values
		a2 = "increase" # window wall ratio
		ch = 1
	print (a0 + a1 + " COP values, " + a2 + " window wall ratio.")
	
	cop_high_new = round(cop_high - ch * 0.05,2)
	cop_high_dub = round(cop_high - ch * 0.10,2)
	cop_low_new = round(cop_low - ch * 0.05,2)
	cop_low_dub = round(cop_low - ch * 0.10,2)
	window_wall_ratio_new = round(window_wall_ratio + ch * 0.05, 2)
	window_wall_ratio_dub = round(window_wall_ratio + ch * 0.10, 2)
	
	options[0][0:2] =  cop_high_new, cop_low_new  # change COP high and low
	options[1][0:2] = cop_high_dub, cop_low_dub   # change COP high and low double
	options[2] = [cop_high_new, cop_low_new, window_wall_ratio_new]  # change COP high and low and window wall ratio
	options[3] = [cop_high_dub, cop_low_dub, window_wall_ratio_dub]  # change COP high and low and window wall ratio double
	options[4][2] = window_wall_ratio_new  # change window wall ratio
	options[5][2] = window_wall_ratio_dub  # change window wall ratio double
	for i in xrange(6):
		if not limits.COPvalsLIM(options[i][0],options[i][1]):  # Will need modification if changing COP high and COP low independently.
			if a < 0:
				options[i][0], options[i][1] = limits.COPlim_high, limits.COPlim_high
			else:
				options[i][0], options[i][1] = limits.COPlim_low, limits.COPlim_low
		if not limits.windowWallRatioLIM(options[i][2]):
			if a < 0:
				options[i][2] = limits.window_wall_low;
			else:
				options[i][2] = limits.window_wall_high;
	# Get rid of any duplicates that were created after limits imposed. 
	final = list(map(list, set(map(tuple,options))))
	return final
	
def peaksOpposite (a, decrease_gas, window_wall_ratio, add_heat_degree):
	winterload_ops = winterLoad(-a, decrease_gas, add_heat_degree)[::2]  # take half of the options ( the ones where "change" wasn't doubled ) This may not be perfect if limits are being reached, but should still give us something reasonable to work with. 
	summerpeak_ops = summerPeak(a, window_wall_ratio);
	options = []
	for i in winterload_ops:
		for j in summerpeak_ops:
			options.extend([i + j])
	# There shouldn't be duplicates, but it's quicker to check now than run a whole simulation
	final = list(map(list, set(map(tuple,options))))
	return final

def resSchedSkew(seconds):
	options = [None] * 7
	for i in xrange(7):
		options[i] = [seconds]
	test = [-3600, -1800, -900, 0, 900, 1800, 3600]
	for j in xrange(len(options)):
		options[j][0] += test[j]
	for i in xrange(7):
		if not limits.resSchedSkewLIM(options[i][0]):
			if options[i][0] < seconds:
				options[i][0] = - limits.schedskewband
			else:
				options[i][0] = limits.schedskewband
	final = list(map(list, set(map(tuple,options))))
	return final
	

def takeAction(action,action_count,vals,diffs):
	'''Calculate changes to certain calibration parameters based on the action code supplied (from chooseAction.py)
	
	- action (int)-- Action code from chooseAction.py. Determines which calibration parameters to alter.
	- action_count (int) -- How many times this action has been attempted. 
	- vals (list)-- List of previously used calibration parameters. (Order is important.)
	- diffs (list)-- Calculated differences for the four main metrics [pv_s,te_s,pv_w,te_w]. 
	
	'''
	# a : avg house,			b : avg comm,			c : base load scalar,
	# d : cooling offset,		e : heating offset,		f : cop high,
	# g : cop low,				h : sched skew,			j : decrease gas,
	# k : schedule skew std,	l : window wall ratio,	m : additional heating degrees	o: load shape scalar (if used)
	
	[a,b,c,d,e,f,g,h,j,k,l,m] = vals[0:12]
	[pv_s,te_s,pv_w,te_w] = diffs 
	avg_diff = sum(diffs)/4
	i = abs(action);
	calibrations = []
	if i == 1:
		options = loadLevel(action, action_count, a, b, c, avg_diff)
		for n in xrange(len(options)):
			calibrations.append([options[n][1], options[n][2], options[n][0], d, e, f, g, h, j, k, l, m])
	elif i == 2:
		options = winterLoad(action, j, m)
		for n in xrange(len(options)):
			calibrations.append([a, b, c, d, e, f, g, h, options[n][0], k, l, options[n][1]])
	elif i == 3:
		options = winterPeak(action, j, k, m)
		for n in xrange(len(options)):
			calibrations.append([a, b, c, d, e, f, g, h, options[n][0], options[n][2], l, options[n][1]])
	elif i == 4:
		options = winterPeaksummerOK(action, f, g, j, m)
		for n in xrange(len(options)):
			calibrations.append([a, b, c, d, e, options[n][2], options[n][3], h, options[n][0], k, l, options[n][1]])
	elif i == 5:
		options = summerPeak(action, l)
		for n in xrange(len(options)):
			calibrations.append([a, b, c, d, e, f, g, h, j, k, options[n][0], m])
	elif i == 6:
		options = summerPeakwinterOK(action, f, g, l)
		for n in xrange(len(options)):
			calibrations.append([a, b, c, d, e, options[n][0], options[n][1], h, j, k, options[n][2], m])
	elif i == 7:
		options = peakLevel(action, d, e, f, g, k)
		for n in xrange(len(options)):
			calibrations.append([a, b, c, options[n][0], options[n][1], options[n][2], options[n][3], h, j, options[n][4], l, m])
	elif i == 8:
		options = peaksOpposite(action, j, l, m)
		for n in xrange(len(options)):
			calibrations.append([a, b, c, d, e, f, g, h, options[n][0], k, options[n][2], options[n][1]])
	elif i == 9:
		options = resSchedSkew(h)
		for n in xrange(len(options)):
			calibrations.append([a, b, c, d, e, f, g, options[n][0], j, k, l, m])
	else:
		print ("Action ID doesn't match any defined!");
	#print (options)
	print ("Calibrations to test:")
	for x in calibrations:
		print (str(x))
	return calibrations

def main():
	print ("Testing values:")
	takeAction(9,[57000,30000,.5,3.5,3.5,0.5,0.5,3600,.50,2700,.15,3.5],[-0.01,-0.009,-0.1245,-0.08]); # example numbers for testing
		
if __name__ ==  '__main__':
	main();