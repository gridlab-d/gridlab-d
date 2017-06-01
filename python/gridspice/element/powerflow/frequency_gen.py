from .. import elementBase

class frequency_gen(elementBase.elementBase):
	def __init__(self):
		pass

	Frequency_Mode=None
	"""
	Description: Frequency object operations mode
	"""

	Frequency=None
	"""
	Units: Hz
	Description: Instantaneous frequency value
	"""

	FreqChange=None
	"""
	Units: Hz/s
	Description: Frequency change from last timestep
	"""

	Deadband=None
	"""
	Units: Hz
	Description: Frequency deadband of the governor
	"""

	Tolerance=None
	"""
	Units: %
	Description: % threshold a power difference must be before it is cared about
	"""

	M=None
	"""
	Units: pu
	Description: Inertial constant of the system
	"""

	D=None
	"""
	Units: %
	Description: Load-damping constant
	"""

	Rated_power=None
	"""
	Units: W
	Description: Rated power of system (base power)
	"""

	Gen_power=None
	"""
	Units: W
	Description: Mechanical power equivalent
	"""

	Load_power=None
	"""
	Units: W
	Description: Last sensed load value
	"""

	Gov_delay=None
	"""
	Units: s
	Description: Governor delay time
	"""

	Ramp_rate=None
	"""
	Units: W/s
	Description: Ramp ideal ramp rate
	"""

	Low_Freq_OI=None
	"""
	Units: Hz
	Description: Low frequency setpoint for GFA devices
	"""

	High_Freq_OI=None
	"""
	Units: Hz
	Description: High frequency setpoint for GFA devices
	"""

	avg24=None
	"""
	Units: Hz
	Description: Average of last 24 hourly instantaneous measurements
	"""

	std24=None
	"""
	Units: Hz
	Description: Standard deviation of last 24 hourly instantaneous measurements
	"""

	avg168=None
	"""
	Units: Hz
	Description: Average of last 168 hourly instantaneous measurements
	"""

	std168=None
	"""
	Units: Hz
	Description: Standard deviation of last 168 hourly instantaneous measurements
	"""

	Num_Resp_Eqs=None
	"""
	Description: Total number of equations the response can contain
	"""




