from .. import elementBase

class billdump(elementBase.elementBase):
	def __init__(self):
		pass

	group=None
	"""
	Description: the group ID to output data for (all nodes if empty)
	"""

	runtime=None
	"""
	Description: the time to check voltage data
	"""

	filename=None
	"""
	Description: the file to dump the voltage data into
	"""

	meter_type=None
	"""
	Description: describes whether to collect from 3-phase or S-phase meters
	"""




