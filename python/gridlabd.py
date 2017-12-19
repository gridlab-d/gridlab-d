import os
import pycurl
from StringIO import StringIO
import json

###############################################################################
# START
###############################################################################
def start(glmfile,options='') :
	""" Start a GridLAB-D simulation"
	Syntax: 
		start(glmfile)
		start(glmfile,options)
	
	Arguments:
		glmfile(string): the name of the glm file
		options(string): simulation options (see 
				         http://gridlab-d.shoutwiki.com/wiki/Command_options 
						 for details)
		
	Returns:
		When the simulation completes successfully the return value is 0.
		When the simulation fails, the return values are as follows
            -1: exec/wait failure - per system(3)
             1: error processing command line arguments
             2: bad environment startup
             3: test failed
             4: user reject terms of use
             5: simulation did not complete as desired
             6: initialization failed 
             7: process control error
             8: server killed
             9: I/O error
           127: shell failure - per system(3)
           128: signal caught - or with SIG value if known, e.g.,
               (XC_SIGNAL|SIGINT): SIGINT caught
           255: unhandled exception caught
	"""
	return os.system("gridlabd --server {} {}".format(options,glmfile))

###############################################################################
# GET
###############################################################################
def get(name,property=[]) :
	""" Get a GridLAB-D global variables or object properties
	
	Syntax:
		get(global-name): get the value of a global variable
		get(object-name,property): get the value of an object property
		get(object-name,'*'): get all the properties of an object
	
	Arguments:
		global-name(string): the name of the global variable
		object-name(string): the name of the object
		property(string): the name of the object property ([] for globals)
	
	Returns:
		When a global variable value is obtained, the result is returned
		as a list containing the following entities:
		{	u'<name>': u'<value>' }
		where <name> is the name of the global variable, and <value> is the
		value of the global variable.
		
		When a list of object properties is obtained, the result is returned
		as a list containing the following entities:
		{
			u'<property_1>': u'<value_1>',
			u'<property_2>': u'<value_2>',
			...
			u'<property_N>': u'<value_N>',
		}
		where <property_n> is the name of the nth property, and <value_n> is
		the value of the nth property.
		
		When a single value is obtained for an object property, the result 
		is returned as a list containing the following entities:
		{	u'object': u'<name>',
			u'type': u'<type>',
			u'name': u'<property>',
			u'value': u'<value>'
		}
		where <name> is the name of the object or global variable, <type> is
		the GridLAB-D property type, <property> is the name of the property
		
		When an error is encountered, the return list will include the "error"
		property, with the remaining properties describing the nature and 
		origin of the error.
	"""
	buffer = StringIO()
	c = pycurl.Curl()
	if property == [] : # global property get
		c.setopt(c.URL,'http://localhost:6267/json/{}'.format(name))
	elif property == '*' : # object property get all
			c.setopt(c.URL,'http://localhost:6267/json/{}/*[dict]'.format(name))
	else : # object property get
		c.setopt(c.URL,'http://localhost:6267/json/{}/{}'.format(name,property))
	c.setopt(c.WRITEDATA,buffer)
	c.perform()
	c.close()
	body = buffer.getvalue()
	return json.loads(body)

###############################################################################
# SET
###############################################################################
def set(name,property,value=[]) :
	""" Set a GridLAB-D global variable or object property
	
	Syntax:
		set(global-name,value): set a global variable
		set(object-name,property,value): set a property of a object
	
	Arguments:
		global-name(string): the name of a global variable
	
	Returns:
		When the set() command is successful, the previous value of the global 
		variable or object property is returned in the same format as the 
		get() command. 
		
		When an error is encountered, the return list will include the "error"
		property, with the remaining properties describing the nature and 
		origin of the error.
	"""
	buffer = StringIO()
	c = pycurl.Curl()
	if value == [] : # global property set
		c.setopt(c.URL,'http://localhost:6267/json/{}={}'.format(name,property))
	else : # object property set
		c.setopt(c.URL,'http://localhost:6267/json/{}/{}={}'.format(name,property,value))
	c.setopt(c.WRITEDATA,buffer)
	c.perform()
	c.close()
	body = buffer.getvalue()
	return json.loads(body)

###############################################################################
# CONTROL
###############################################################################
def find(search) :
	""" GridLAB-D object search command
	
	Syntax: 
		find(filter)
		
	Arguments:
		filter(string): search filter, see 
			http://gridlab-d.shoutwiki.com/wiki/Collections for details
	
	Returns:
		When the query succeed a list of object names is returned.
	"""
	buffer = StringIO()
	c = pycurl.Curl()
	c.setopt(c.URL,'http://localhost:6267/find/{}'.format(search))
	c.setopt(c.WRITEDATA,buffer)
	c.perform()
	c.close()
	body = buffer.getvalue()
	return json.loads(body)

###############################################################################
# CONTROL
###############################################################################
def control(command) :
	""" GridLAB-D simulation control commands
		
	Syntax:
		control(command): send a control command to the active simulation
		
	Arguments:
		command(string): the control command, see
						 http://gridlab-d.shoutwiki.com/wiki/Control
						 for details.
			Frequently used commands:
			"pause": pauses the simulation at the current time, use "resume" to
				to continue.
			"pause_wait": pauses the simulation and sleeps for 0.1 seconds
			    before returning.
			"pauseat=<yyyy-mm-dd HH:MM:SS zzz[z]>": schedules a pause at the
				specified time.
			"resume": resumes the simulation after a pause.
			"shutdown": shuts the server down and stops the simulation.
			"stop": stops the simulation but leaves the server running to 
				allow further queries. Only used for realtime simulation.
	Returns:
		The return value depends on the command issued. Most commands return
		an empty string on success.
	"""
	buffer = StringIO()
	c = pycurl.Curl()
	c.setopt(c.URL,'http://localhost:6267/control/{}'.format(command))
	c.setopt(c.WRITEDATA,buffer)
	c.perform()
	c.close()
	body = buffer.getvalue()
	print body
	return body

###############################################################################
# UTILITIES
###############################################################################
def get_double(data,key) :
	""" Get float value from a GridLAB-D double property in a list
	
	Syntax:
		x = get_double(data,name)
		
	Arguments:
		data(dict): data returned by get()
		key(string): name of item to extract from the data
		
	Returns:
		A float value
		
	Example:
		import gridlabd
		data = gridlabd.get('house','*')
		T = gridlabd.get_double(data,'air_temperature')
	"""
	try :
		value = data[key].split(' ')
		return float(value[0])
	except :
		print "ERROR: can't find double value for key='{}' in data={}".format(key,json.dumps(data,sort_keys=True,indent=4, separators=(',', ': ')))

def get_complex(data,key) :
	""" Get complex value from a GridLAB-D cmplex property in a list
	
	Syntax:
		x = get_complex(data,name)
		
	Arguments:
		data(dict): data returned by get()
		key(string): name of item to extract from the data
		
	Returns:
		A complex value
		
	Example:
		import gridlabd
		data = gridlabd.get('house','*')
		P = gridlabd.get_complex(data,'panel.power')
	
	Note: 
		By default GridLAB-D using the 'i' notation for complex value but
		Python users the 'j' notation.  To enable the 'j' notation in GridLAB-D
		the following must be included in the GLM file:
		
			#set complex_format="%d+%dj"
	"""
	try :
		value = data[key].split(' ')
		return complex(value[0])
	except :
		print "ERROR: can't find complex value for key='{}' in data={}".format(key,json.dumps(data,sort_keys=True,indent=4, separators=(',', ': ')))

def get_unit(data,key) :
	""" Get units from from a GridLAB-D double or complex property in a list
	
	Syntax:
		x = get_unit(data,name)
		
	Arguments:
		data(dict): data returned by get()
		key(string): name of item to extract from the data
		
	Returns:
		A unit string if one is present, otherwise an empty string
		
	Example:
		import gridlabd
		data = gridlabd.get('house','*')
		unit = gridlabd.get_unit(data,'air_temperature')
	"""
	try :
		value = data[key].split(' ')
		if len(value) > 1 :
			return value[1]
		else :
			return ''	
	except :
		print "ERROR: can't find unit for key='{}' in data={}".format(key,json.dumps(data,sort_keys=True,indent=4, separators=(',', ': ')))

