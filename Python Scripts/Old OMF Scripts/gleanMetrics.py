# After the .glms all run, of those that ran succesfully we want the five comparison metrics to plug into the WSM. 
#import pymysql
from __future__ import division
import re
import os
import datetime

# Flag whether we're using mysql for recorder output or not ( implies using .csv files instead )
# Make sure this flag is the same in makeGLM.py or there'll be problems. 
use_mysql = 0 # 0 => .csv files; 1 => mysql database

if use_mysql == 1:
	import MySQLdb
	# MySQL connection parameters
	u = 'gridlabd'
	p = ''
	schema = 'CalibrationDB'
	table = 'network_node_recorder'

# This is an example dictionary this script returns.
# 	Each dictionary index represents one of the feeder versions we're comparing, 
#	and the values are the percent differences between the measured metrics and the SCADA data. 
#	[peak val, peak time, total energy, minimum val, minimum time] for summer = 0, winter = 1, shoulder = 2

#raw_metrics = { 	'v1': [[0.0178,0.0033,-0.0027,-0.0437,0.0138],	[0.0325,-0.0071,-0.0744,-0.1742,0.0033],	[0.1310,-0.1492,0.0864,0.0586,0.0554]], 
#					'v2': [[-0.0119,0.0242,0.0105,0.1069,0.0383],	[-0.1028,0.0138,-0.0831,-0.1400,0.3854],	[-0.0856,-0.0138,0.0670,0.3922,-0.0383]],
#					'v3': [[0.0038,0.0138,-0.0022,0.0662,0.0696],	[-0.0152,0.0000,-0.0164,-0.0716,0.3854],	[0.1643,-0.1875,0.2364,0.4175,-0.0592]],
#					'v4': [[-0.0116,-0.0033,0.0150,0.2188,-0.0971],	[0.1735,-0.0175,0.0104,-0.1221,0.3992],		[0.4247,-0.1425,0.6542,0.8297,-0.0033]],
#					'v5': [[-0.0093,-0.0071,-0.0002,0.1427,0.0938],	[-0.0169,-0.0138,-0.0155,-0.1007,-0.0104],	[0.1862,-0.1458,0.1460,0.4128,-0.0383]],
#					'v6': [[-0.0091,0.0033,-0.0050,0.1273,0.0104],	[-0.1232,-0.0175,-0.0349,-0.1179,0.0033],	[0.1262,-0.2154,0.1218,0.3190,-0.0729]] };


def getValues(dir,glm_filenames,days):
	''' Take list of all .glms that were ran and return measurements for each metric.
	
	dir -- path to the working directory
	glm_filenames -- List of filenames for every .glm that was ran in GridLab-D 
	days -- List of dates for summer, winter, and spring. Should match the dates referenced in the .glm filenames
	
	This function also evaluates by querying the MySQL database whether a .glm ran successfully or not. If it was not successful, it is discounted.
	
	'''
	measurements = {}
	orig_len = len(glm_filenames)
	
	if use_mysql == 1:
		# open database connection
		dbh = MySQLdb.connect(user=u,passwd=p,db=schema)
		cursor = dbh.cursor()

	for i in glm_filenames:
		print ("Attempting to gather measurements from "+i)
		
		# Check that each feeder version has records for three days, and that each ran .glm has a complete (or nearly complete) record set (288 entries). 
		if use_mysql == 1:
			sql = "SELECT COUNT(*) FROM %s WHERE fname = '%s'" % (table, i)
			cursor.execute(sql)
			result = cursor.fetchone()
			if result[0] is None or result[0] == 0:
				print ("	Missing simulation output for "+i)
				continue
			elif result[0] < 288:
				print ("	Missing simulation output for "+i+". "+str(result[0])+"/288 five-minute intervals made it into the database.")
				continue
		else:
			c = 0
			filename = dir+'\\csv_output\\'+re.sub('\.glm$','_network_node_recorder.csv',i)
			if not os.path.isfile(filename):
				print ("	Missing simulation output for "+i)
				continue
			else:
				csv = open (filename, 'r')
				lines = csv.readlines()
				for l in lines:
					if l.startswith("#"):
						continue
					else:
						c += 1
				if c < 288:
					csv.close()
					print ("	Missing simulation output for "+i+". "+str(c)+"/288 five-minute intervals were recorded.")
					continue
		
		# Get annual .glm ID by stripping the date from the filename. 
		m = re.match(r'^Calib_ID(\d*)_Config_ID(\d*)',i)
		if m is not None:
			glm_ID = m.group()
		else:
			m = re.match(r'^DefaultCalibration',i)
			if m is not None:
				glm_ID = m.group()
			else:
				print ("	Can't recognize file name: "+str(i)+". Going to ignore it.")
				continue
		
		if glm_ID not in measurements.keys():
			measurements[glm_ID] = [ [None], [None], [None] ]
			
		# Determine what season this .glm was for.
		season = None
		for j in xrange(len(days)):
			if re.match(r'^.*'+days[j]+'.*$',i) is not None:
				season = j
		if season is None:
			print ("File "+str(i)+" isn't matching any of "+str(days)+".")
			measurements[glm_ID] = None
		else:
		
			if use_mysql == 1:
				# Get peak value & time of peak.
				sql1 = "SELECT t,measured_real_power FROM %s WHERE measured_real_power = (SELECT MAX(measured_real_power) FROM %s WHERE fname = '%s') AND fname = '%s'" % (table, table, i, i ) 
				cursor.execute(sql1)
				result1 = cursor.fetchone()
				pt = int(result1[0].strftime('%H')) + (int(result1[0].strftime('%M')) / 60)
				pv = result1[1]/1000
				# Get total energy.
				sql2 = "SELECT measured_real_energy FROM %s WHERE t = (SELECT MAX(t) FROM %s WHERE fname = '%s') AND fname = '%s'" % (table, table, i, i ) 
				cursor.execute(sql2)
				result2 = cursor.fetchone()
				te = result2[0]/1000
				# Get minimum value & time of minimum.
				sql3 = "SELECT t,measured_real_power from %s WHERE measured_real_power = (SELECT MIN(measured_real_power) FROM %s WHERE fname = '%s') AND fname = '%s'" % (table, table, i, i ) 
				cursor.execute(sql3)
				result3 = cursor.fetchone()
				mt = int(result3[0].strftime('%H')) + (int(result3[0].strftime('%M')) / 60)
				mv = result3[1]/1000
			else:
				measured_power_dict = {}
				measured_power = []
				measured_energy = []
				for l in lines:
					if l.startswith("#"):
						continue
					else:
						vals = l.split(',')
						measured_power_dict[vals[0]] = float(vals[1])/1000
						measured_power.append(float(vals[1])/1000)
						measured_energy.append(float(vals[2])/1000)
				csv.close()
				pv = max(measured_power)
				mv = min(measured_power)
				te = max(measured_energy)
				
				for i in measured_power_dict.keys():
					if measured_power_dict[i] == pv:
						pt_stamp = re.sub('\s\w{3}','',i)
					elif measured_power_dict[i] == mv:
						mt_stamp = re.sub('\s\w{3}','',i)
				
				pt_date = datetime.datetime.strptime(pt_stamp,'%Y-%m-%d %H:%M:%S')
				mt_date = datetime.datetime.strptime(mt_stamp,'%Y-%m-%d %H:%M:%S')
				pt = int(pt_date.strftime('%H')) + (int(pt_date.strftime('%M')) / 60)
				mt = int(mt_date.strftime('%H')) + (int(mt_date.strftime('%M')) / 60)
			measurements[glm_ID][season] = [pv,pt,te,mv,mt]
			#print (str(i) +": peak value: "+ str(pv)+ " peak time: "+ str(pt)+ " total energy: "+ str(te) + " minimum time: "+ str(mt) + " minimum value: " + str(mv)) 
	
	# Make sure we only continue evaluation with glms that have all three days completely recorded. 
	topop=[]
	for j in measurements.keys():
		if measurements[j] is None: 
			topop.append(j)
		else:
			for k in measurements[j]:
				if k[0] is None: 
					topop.append(j)
	if len(topop) != 0:
		for i in topop:
			if i in measurements.keys():
				print ("Ignoring "+i+" due to missing simulation output.")
				measurements.pop(i)
					
	if use_mysql == 1:
		dbh.close()
	print (str(len(measurements))+" of "+ str(orig_len/3)+ " calibrated models successfully ran.")
	return measurements
	

def calcDiffs (glm_vals, scada):
	'''Calculate percent differences for [peak val, peak time, total energy, minimum val, minimum time] from recorder output from simulation of given glm on given day.'''

	diffs = [];
	for i in xrange(len(glm_vals)):
		if i == 1 or i == 4: # time of peak or time of minimum
			j = round((glm_vals[i] - scada[i])/24,4);
		else: 
			j = round((glm_vals[i] - scada[i])/scada[i],4);
		diffs.append(j);
	return diffs;

# We'll need to calculate the 5 metrics for each season for each comparison run. 
def funcRawMetsDict (dir, glms,scada,days):
	'''Create dictionary of metrics for each .glm in question
	
	dir -- Path to the working directory
	glms -- A list of file names of .glms to compare
	scada -- The SCADA values to be compared with .glm simulation output
	days -- The list of dates for summer, winter, shoulder
	
	'''
	# Create dictionary with measurements for each successfuly ran *.glm.
	# Note: "Successfully ran" means that each of the three days ran successfully for a certain calibration.
	measurements = getValues(dir, glms,days)
	raw_metrics = {};
	for i in measurements.keys():
		# Set up an entry in the raw metrics dictionary for each calibrated *.glm.
		raw_metrics[i] = [];
	for j in raw_metrics.keys():
		raw_metrics[j].append(calcDiffs(measurements[j][0],scada[0]));
		raw_metrics[j].append(calcDiffs(measurements[j][1],scada[1]));
		raw_metrics[j].append(calcDiffs(measurements[j][2],scada[2]));
	return raw_metrics
	
def main():
	pass
	# pretend these are returned from a function gleaning val's from recorder output stored somewhere 
	# [peak val, peak time, total energy, minimum val, minimum time] for summer = 0, winter = 1, shoulder = 2
	# test_metrics = {	'2407': [[15010.97, 18.0, 253289.49, 6229.59, 4.25], [11754.59, 7.08, 2061549.63, 6228.17, 22.58], [8496.49, 17.09, 165030.59, 5197.83, 2.91]], # first entry are measurements from 2407
						# '2408': [[14572.94, 18.5, 256641.96, 7210.63, 4.84], [10214.25, 7.58, 204220.22, 6486.1, 31.75], [6869.31, 20.34, 162083.61, 6835.84, 0.66]],   # following are calculated such that combined with 2407 scada, give differences that are actually for other feeders (2408-2415)
						# '2410': [[14804.49, 18.25, 253416.47, 6945.50, 5.59], [11211.54, 7.25, 219076.25, 7001.97, 31.75], [8746.65, 16.17, 187816.47, 6960.07, 0.16]], 
						# '2412': [[14577.37, 17.84, 257784.85, 7939.58, 1.59], [13359.82, 6.83, 225045.38, 6621.1, 32.08], [10702.87, 17.25, 251282.77, 8984.01, 1.5]], 
						# '2414': [[14611.29, 17.75, 253924.42, 7443.84, 6.17], [11192.19, 6.92, 219276.70,6782.5, 22.25], [8911.17, 17.17, 174084.18, 6936.99, 0.66]], 
						# '2415': [[14614.24, 18.00, 252705.34, 7343.53, 4.17], [9982.01, 6.83, 214955.76, 6652.78, 22.58], [8460.43, 15.5, 170408.06, 6476.42, -0.17]]}

	#getValues('C:\\Users\\d3y051\\Documents\\NRECA_feeder_calibration_2-2013\\Calibration\\github\\omf\\calibration\\Feeder_Test',['DefaultCalibration_2012-06-29.glm','DefaultCalibration_2012-04-10.glm','DefaultCalibration_2012-01-19.glm'],['2012-06-29','2012-04-10','2012-01-19'])
	#names, mets = funcRawMetsDict(['2407','2408','2410','2412','2414','2415'],[[14748.45,17.92,3047702.62,6514.26,3.92],[11384.59,7.25,2672748.02,7541.98,22.50],[7512.37,20.67,1822870.98,4910.10,1.58]]);
	#getValues(['F2407_calib_1_0_2013-04-14.glm','F2407_calib_1_2_2013-04-14.glm','F2407_calib_1_0_2013-06-29.glm'],['2013-06-29', '2013-04-14'])
	#mets = funcRawMetsDict(['2407','2408','2410','2412','2414','2415'],[[14748.45,17.92,3047702.62,6514.26,3.92],[11384.59,7.25,2672748.02,7541.98,22.50],[7512.37,20.67,1822870.98,4910.10,1.58]]);
	# for i in (mets.keys()):
		# print ("ID: " + str(i));
		# print ("summer \t\t" + str(mets[i][0]));
		# print ("winter \t\t" + str(mets[i][1]));
		# print ("shoulder \t" + str(mets[i][2]));

if __name__ ==  '__main__':
	main();