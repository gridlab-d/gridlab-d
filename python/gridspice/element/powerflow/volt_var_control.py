from .. import elementBase

class volt_var_control(elementBase.elementBase):
	def __init__(self):
		pass

	control_method=None
	"""
	Description: IVVC activated or in standby
	"""

	capacitor_delay=None
	"""
	Units: s
	Description: Default capacitor time delay - overridden by local defintions
	"""

	regulator_delay=None
	"""
	Units: s
	Description: Default regulator time delay - overriden by local definitions
	"""

	desired_pf=None
	"""
	Description: Desired power-factor magnitude at the substation transformer or regulator
	"""

	d_max=None
	"""
	Description: Scaling constant for capacitor switching on - typically 0.3 - 0.6
	"""

	d_min=None
	"""
	Description: Scaling constant for capacitor switching off - typically 0.1 - 0.4
	"""

	substation_link=None
	"""
	Description: Substation link, transformer, or regulator to measure power factor
	"""

	pf_phase=None
	"""
	Description: Phase to include in power factor monitoring
	"""

	regulator_list=None
	"""
	Description: List of voltage regulators for the system, separated by commas
	"""

	capacitor_list=None
	"""
	Description: List of controllable capacitors on the system separated by commas
	"""

	voltage_measurements=None
	"""
	Description: List of voltage measurement devices, separated by commas
	"""

	minimum_voltages=None
	"""
	Description: Minimum voltages allowed for feeder, separated by commas
	"""

	maximum_voltages=None
	"""
	Description: Maximum voltages allowed for feeder, separated by commas
	"""

	desired_voltages=None
	"""
	Description: Desired operating voltages for the regulators, separated by commas
	"""

	max_vdrop=None
	"""
	Description: Maximum voltage drop between feeder and end measurements for each regulator, separated by commas
	"""

	high_load_deadband=None
	"""
	Description: High loading case voltage deadband for each regulator, separated by commas
	"""

	low_load_deadband=None
	"""
	Description: Low loading case voltage deadband for each regulator, separated by commas
	"""




