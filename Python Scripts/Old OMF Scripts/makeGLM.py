'''Writes the necessary .glm files for a calibration round. Define recording interval and MySQL schema name here.'''
from __future__ import division
import datetime 
import re
import math
import feeder
import Milsoft_GridLAB_D_Feeder_Generation
import os

# recording interval (seconds)
interval = 300  

# flag whether we're using mysql for recorder output or not ( implies using .csv files instead )
# Make sure this flag is the same in gleanMetrics.py or there'll be problems. 
use_mysql = 0 # 0 => .csv files; 1 => mysql database

# MySQL schema name
schema = 'CalibrationDB'


def makeGLM(clock, calib_file, baseGLM, case_flag, wdir):
	'''Create populated dict and write it to .glm file
	
	- clock (dictionary) links the three seasonal dates with start and stop timestamps (start simulation full 24 hour before day we're recording)
	- calib_file (string) -- filename of one of the calibration files generated during a calibration round 
	- baseGLM (dictionary) -- orignal base dictionary for use in Milsoft_GridLAB_D_Feeder_Generation.py
	- case_flag (int) -- flag technologies to test
	- feeder_config (string TODO: this is future work, leave as 'None')-- feeder configuration file (weather, sizing, etc)
	- dir(string)-- directory in which to store created .glm files
	'''
	# Create populated dictionary.
	glmDict, last_key = Milsoft_GridLAB_D_Feeder_Generation.GLD_Feeder(baseGLM,case_flag,calib_file) 
	
	fnames =  []
	for i in clock.keys():
		# Simulation start
		starttime = clock[i][0]
		# Recording start
		rec_starttime = i
		# Simulation and Recording stop
		stoptime = clock[i][1]
		
		# Calculate limit.
		j = datetime.datetime.strptime(rec_starttime,'%Y-%m-%d %H:%M:%S')
		k = datetime.datetime.strptime(stoptime,'%Y-%m-%d %H:%M:%S')
		diff = (k - j).total_seconds()
		limit = int(math.ceil(diff / interval))
		
		populated_dict = glmDict
		
		# Name the file.
		if calib_file == None:
			id = 'DefaultCalibration'
		else:
			m = re.compile( '\.txt$' )
			id = m.sub('',calib_file.get('name', ''))
		date = re.sub('\s.*$','',rec_starttime)
		filename = id + '_' + date + '.glm'
		
		# Get into clock object and set start and stop timestamp.
		for i in populated_dict.keys():
			if 'clock' in populated_dict[i].keys():
				populated_dict[i]['starttime'] = "'{:s}'".format(starttime)
				populated_dict[i]['stoptime'] = "'{:s}'".format(stoptime)
		
		lkey = last_key
		
		if use_mysql == 1:
			# Add GridLAB-D objects for recording into MySQL database.
			populated_dict[lkey] = { 'module' : 'mysql' }
			lkey += 1
			populated_dict[lkey] = {'object' : 'database',
										'name' : '{:s}'.format(schema),
										'schema' : '{:s}'.format(schema) }
			lkey += 1
			populated_dict[lkey] = {'object' : 'mysql.recorder',
										'table' : 'network_node_recorder',
										'parent' : 'network_node',
										'property' : 'measured_real_power,measured_real_energy',
										'interval' : '{:d}'.format(interval),
										'limit' : '{:d}'.format(limit),
										'start': "'{:s}'".format(rec_starttime),
										'connection': schema,
										'mode': 'a'}
		else:
			# Add GridLAB-D object for recording into *.csv files.
			try:
				os.mkdir(os.path.join(wdir,'csv_output'))
			except:
				pass
			# Add GridLAB-D object for recording into *.csv files.
			populated_dict[lkey] = {'object' : 'tape.recorder',
										'file' : './csv_output/{:s}_{:s}_network_node_recorder.csv'.format(id,date),
										'parent' : 'network_node',
										'property' : 'measured_real_power,measured_real_energy',
										'interval' : '{:d}'.format(interval),
										'limit' : '{:d}'.format(limit),
										'in': "'{:s}'".format(rec_starttime) }
										
		# Turn dictionary into a *.glm string and print it to a file in the given directory.
		glmstring = feeder.sortedWrite(populated_dict)
		file = open(os.path.join(wdir, filename), 'w')
		file.write(glmstring)
		file.close()
		print ("\t"+filename+ " is ready.")
		
		fnames.append(filename)
	return fnames

def main():
	print (__doc__)
	print (makeGLM.__doc__)
if __name__ ==  '__main__':
	 main();