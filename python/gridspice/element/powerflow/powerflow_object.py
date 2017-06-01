from .. import elementBase

class powerflow_object(elementBase.elementBase):
	def __init__(self):
		super(powerflow_object,self).__init__()
		self.phases=None
		"""
		"""
	
		self.nominal_voltage=None
		"""
		Units: V
		"""




class node(powerflow_object):
	def __init__(self):
		super(node, self).__init__()
		self.bustype=None
		"""
		"""
	
		self.busflags=None
		"""
		"""
	
		self.reference_bus=None
		"""
		"""
	
		self.maximum_voltage_error=None
		"""
		Units: V
		"""
	
		self.voltage_A=None
		"""
		Units: V
		"""
	
		self.voltage_B=None
		"""
		Units: V
		"""
	
		self.voltage_C=None
		"""
		Units: V
		"""
	
		self.voltage_AB=None
		"""
		Units: V
		"""
	
		self.voltage_BC=None
		"""
		Units: V
		"""
	
		self.voltage_CA=None
		"""
		Units: V
		"""
	
		self.current_A=None
		"""
		Units: A
		"""
	
		self.current_B=None
		"""
		Units: A
		"""
	
		self.current_C=None
		"""
		Units: A
		"""
	
		self.power_A=None
		"""
		Units: VA
		"""
	
		self.power_B=None
		"""
		Units: VA
		"""
	
		self.power_C=None
		"""
		Units: VA
		"""
	
		self.shunt_A=None
		"""
		Units: S
		"""
	
		self.shunt_B=None
		"""
		Units: S
		"""
	
		self.shunt_C=None
		"""
		Units: S
		"""
	
		self.NR_mode=None
		"""
		"""
	
		self.mean_repair_time=None
		"""
		Units: s
		Description: Time after a fault clears for the object to be back in service
		"""
	
		self.service_status=None
		"""
		"""




class link(powerflow_object):
	def __init__(self):
		pass

	status=None
	"""
	"""

	fromNode=None
	"""
	"""

	toNode=None
	"""
	"""

	power_in=None
	"""
	Units: VA
	"""

	power_out=None
	"""
	Units: VA
	"""

	power_out_real=None
	"""
	Units: W
	"""

	power_losses=None
	"""
	Units: VA
	"""

	power_in_A=None
	"""
	Units: VA
	"""

	power_in_B=None
	"""
	Units: VA
	"""

	power_in_C=None
	"""
	Units: VA
	"""

	power_out_A=None
	"""
	Units: VA
	"""

	power_out_B=None
	"""
	Units: VA
	"""

	power_out_C=None
	"""
	Units: VA
	"""

	power_losses_A=None
	"""
	Units: VA
	"""

	power_losses_B=None
	"""
	Units: VA
	"""

	power_losses_C=None
	"""
	Units: VA
	"""

	current_out_A=None
	"""
	Units: A
	"""

	current_out_B=None
	"""
	Units: A
	"""

	current_out_C=None
	"""
	Units: A
	"""

	current_in_A=None
	"""
	Units: A
	"""

	current_in_B=None
	"""
	Units: A
	"""

	current_in_C=None
	"""
	Units: A
	"""

	fault_current_in_A=None
	"""
	Units: A
	"""

	fault_current_in_B=None
	"""
	Units: A
	"""

	fault_current_in_C=None
	"""
	Units: A
	"""

	fault_current_out_A=None
	"""
	Units: A
	"""

	fault_current_out_B=None
	"""
	Units: A
	"""

	fault_current_out_C=None
	"""
	Units: A
	"""

	flow_direction=None
	"""
	"""

	mean_repair_time=None
	"""
	Units: s
	Description: Time after a fault clears for the object to be back in service
	"""




class capacitor(node):
	def __init__(self):
		pass

	pt_phase=None
	"""
	"""

	phases_connected=None
	"""
	"""

	switchA=None
	"""
	"""

	switchB=None
	"""
	"""

	switchC=None
	"""
	"""

	control=None
	"""
	"""

	voltage_set_high=None
	"""
	Units: V
	"""

	voltage_set_low=None
	"""
	Units: V
	"""

	VAr_set_high=None
	"""
	Units: VAr
	"""

	VAr_set_low=None
	"""
	Units: VAr
	"""

	current_set_low=None
	"""
	Units: A
	"""

	current_set_high=None
	"""
	Units: A
	"""

	capacitor_A=None
	"""
	Units: VAr
	"""

	capacitor_B=None
	"""
	Units: VAr
	"""

	capacitor_C=None
	"""
	Units: VAr
	"""

	cap_nominal_voltage=None
	"""
	Units: V
	"""

	time_delay=None
	"""
	Units: s
	"""

	dwell_time=None
	"""
	Units: s
	"""

	lockout_time=None
	"""
	Units: s
	"""

	remote_sense=None
	"""
	"""

	remote_sense_B=None
	"""
	"""

	control_level=None
	"""
	"""




class fuse(link):
	def __init__(self):
		pass

	phase_A_status=None
	"""
	"""

	phase_B_status=None
	"""
	"""

	phase_C_status=None
	"""
	"""

	repair_dist_type=None
	"""
	"""

	current_limit=None
	"""
	Units: A
	"""

	mean_replacement_time=None
	"""
	Units: s
	"""




class meter(node):
	def __init__(self):
		self.measured_real_energy=None
		"""
		Units: Wh
		"""

		self.measured_reactive_energy=None
		"""
		Units: VAh
		"""

		measured_power=None
		"""
		Units: VA
		"""
	
		measured_power_A=None
		"""
		Units: VA
		"""
	
		measured_power_B=None
		"""
		Units: VA
		"""
	
		self.measured_power_C=None
		"""
		Units: VA
		"""
	
		self.measured_demand=None
		"""
		Units: W
		"""
	
		self.measured_real_power=None
		"""
		Units: W
		"""
	
		self.measured_reactive_power=None
		"""
		Units: VAr
		"""
	
		self.meter_power_consumption=None
		"""
		Units: VA
		"""
	
		self.measured_voltage_A=None
		"""
		Units: V
		"""
	
		self.measured_voltage_B=None
		"""
		Units: V
		"""
	
		self.measured_voltage_C=None
		"""
		Units: V
		"""
	
		self.measured_voltage_AB=None
		"""
		Units: V
		"""
	
		self.measured_voltage_BC=None
		"""
		Units: V
		"""
	
		self.measured_voltage_CA=None
		"""
		Units: V
		"""
	
		self.measured_current_A=None
		"""
		Units: A
		"""
	
		self.measured_current_B=None
		"""
		Units: A
		"""
	
		self.measured_current_C=None
		"""
		Units: A
		"""
	
		self.customer_interrupted=None
		"""
		"""
	
		self.customer_interrupted_secondary=None
		"""
		"""
	
		self.monthly_bill=None
		"""
		"""
	
		self.previous_monthly_bill=None
		"""
		"""
	
		self.previous_monthly_energy=None
		"""
		Units: kWh
		"""
	
		self.monthly_fee=None
		"""
		"""
	
		self.monthly_energy=None
		"""
		Units: kWh
		"""
	
		self.bill_mode=None
		"""
		"""
	
		self.power_market=None
		"""
		"""
	
		self.bill_day=None
		"""
		"""
	
		self.price=None
		"""
		"""
	
		self.price_base=None
		"""
		Description: Used only in TIERED_RTP mode to describe the price before the first tier
		"""
	
		self.first_tier_price=None
		"""
		"""
	
		self.first_tier_energy=None
		"""
		Units: kWh
		"""
	
		self.second_tier_price=None
		"""
		"""
	
		self.second_tier_energy=None
		"""
		Units: kWh
		"""
	
		self.third_tier_price=None
		"""
		"""
	
		self.third_tier_energy=None
		"""
		Units: kWh
		"""




class line(link):
	def __init__(self):
	
		self.configuration=None
		"""
		"""
	
		self.length=None
		"""
		Units: ft
		"""




class overhead_line(line):
	def __init__(self):
		pass




class underground_line(line):
	def __init__(self):
		pass




class relay(link):
	def __init__(self):
		pass

	time_to_change=None
	"""
	Units: s
	"""

	recloser_delay=None
	"""
	Units: s
	"""

	recloser_tries=None
	"""
	"""

	recloser_limit=None
	"""
	"""

	recloser_event=None
	"""
	"""




class transformer(link):
	def __init__(self):

		self.configuration=None
		"""
		"""




class load(node):
	def __init__(self):
		pass

	load_class=None
	"""
	"""

	constant_power_A=None
	"""
	Units: VA
	"""

	constant_power_B=None
	"""
	Units: VA
	"""

	constant_power_C=None
	"""
	Units: VA
	"""

	constant_power_A_real=None
	"""
	Units: W
	"""

	constant_power_B_real=None
	"""
	Units: W
	"""

	constant_power_C_real=None
	"""
	Units: W
	"""

	constant_power_A_reac=None
	"""
	Units: VAr
	"""

	constant_power_B_reac=None
	"""
	Units: VAr
	"""

	constant_power_C_reac=None
	"""
	Units: VAr
	"""

	constant_current_A=None
	"""
	Units: A
	"""

	constant_current_B=None
	"""
	Units: A
	"""

	constant_current_C=None
	"""
	Units: A
	"""

	constant_current_A_real=None
	"""
	Units: A
	"""

	constant_current_B_real=None
	"""
	Units: A
	"""

	constant_current_C_real=None
	"""
	Units: A
	"""

	constant_current_A_reac=None
	"""
	Units: A
	"""

	constant_current_B_reac=None
	"""
	Units: A
	"""

	constant_current_C_reac=None
	"""
	Units: A
	"""

	constant_impedance_A=None
	"""
	Units: Ohm
	"""

	constant_impedance_B=None
	"""
	Units: Ohm
	"""

	constant_impedance_C=None
	"""
	Units: Ohm
	"""

	constant_impedance_A_real=None
	"""
	Units: Ohm
	"""

	constant_impedance_B_real=None
	"""
	Units: Ohm
	"""

	constant_impedance_C_real=None
	"""
	Units: Ohm
	"""

	constant_impedance_A_reac=None
	"""
	Units: Ohm
	"""

	constant_impedance_B_reac=None
	"""
	Units: Ohm
	"""

	constant_impedance_C_reac=None
	"""
	Units: Ohm
	"""

	measured_voltage_A=None
	"""
	"""

	measured_voltage_B=None
	"""
	"""

	measured_voltage_C=None
	"""
	"""

	measured_voltage_AB=None
	"""
	"""

	measured_voltage_BC=None
	"""
	"""

	measured_voltage_CA=None
	"""
	"""

	phase_loss_protection=None
	"""
	Description: Trip all three phases of the load if a fault occurs
	"""

	base_power_A=None
	"""
	Units: VA
	"""

	base_power_B=None
	"""
	Units: VA
	"""

	base_power_C=None
	"""
	Units: VA
	"""

	power_pf_A=None
	"""
	Units: pu
	"""

	current_pf_A=None
	"""
	Units: pu
	"""

	impedance_pf_A=None
	"""
	Units: pu
	"""

	power_pf_B=None
	"""
	Units: pu
	"""

	current_pf_B=None
	"""
	Units: pu
	"""

	impedance_pf_B=None
	"""
	Units: pu
	"""

	power_pf_C=None
	"""
	Units: pu
	"""

	current_pf_C=None
	"""
	Units: pu
	"""

	impedance_pf_C=None
	"""
	Units: pu
	"""

	power_fraction_A=None
	"""
	Units: pu
	"""

	current_fraction_A=None
	"""
	Units: pu
	"""

	impedance_fraction_A=None
	"""
	Units: pu
	"""

	power_fraction_B=None
	"""
	Units: pu
	"""

	current_fraction_B=None
	"""
	Units: pu
	"""

	impedance_fraction_B=None
	"""
	Units: pu
	"""

	power_fraction_C=None
	"""
	Units: pu
	"""

	current_fraction_C=None
	"""
	Units: pu
	"""

	impedance_fraction_C=None
	"""
	Units: pu
	"""




class regulator(link):
	def __init__(self):
		pass

	configuration=None
	"""
	"""

	tap_A=None
	"""
	"""

	tap_B=None
	"""
	"""

	tap_C=None
	"""
	"""

	sense_node=None
	"""
	"""




class triplex_node(powerflow_object):
	def __init__(self):
		pass

	bustype=None
	"""
	"""

	busflags=None
	"""
	"""

	reference_bus=None
	"""
	"""

	maximum_voltage_error=None
	"""
	Units: V
	"""

	voltage_1=None
	"""
	Units: V
	"""

	voltage_2=None
	"""
	Units: V
	"""

	voltage_N=None
	"""
	Units: V
	"""

	voltage_12=None
	"""
	Units: V
	"""

	voltage_1N=None
	"""
	Units: V
	"""

	voltage_2N=None
	"""
	Units: V
	"""

	current_1=None
	"""
	Units: A
	"""

	current_2=None
	"""
	Units: A
	"""

	current_N=None
	"""
	Units: A
	"""

	current_1_real=None
	"""
	Units: A
	"""

	current_2_real=None
	"""
	Units: A
	"""

	current_N_real=None
	"""
	Units: A
	"""

	current_1_reac=None
	"""
	Units: A
	"""

	current_2_reac=None
	"""
	Units: A
	"""

	current_N_reac=None
	"""
	Units: A
	"""

	current_12=None
	"""
	Units: A
	"""

	current_12_real=None
	"""
	Units: A
	"""

	current_12_reac=None
	"""
	Units: A
	"""

	residential_nominal_current_1=None
	"""
	Units: A
	"""

	residential_nominal_current_2=None
	"""
	Units: A
	"""

	residential_nominal_current_12=None
	"""
	Units: A
	"""

	residential_nominal_current_1_real=None
	"""
	Units: A
	"""

	residential_nominal_current_1_imag=None
	"""
	Units: A
	"""

	residential_nominal_current_2_real=None
	"""
	Units: A
	"""

	residential_nominal_current_2_imag=None
	"""
	Units: A
	"""

	residential_nominal_current_12_real=None
	"""
	Units: A
	"""

	residential_nominal_current_12_imag=None
	"""
	Units: A
	"""

	power_1=None
	"""
	Units: VA
	"""

	power_2=None
	"""
	Units: VA
	"""

	power_12=None
	"""
	Units: VA
	"""

	power_1_real=None
	"""
	Units: W
	"""

	power_2_real=None
	"""
	Units: W
	"""

	power_12_real=None
	"""
	Units: W
	"""

	power_1_reac=None
	"""
	Units: VAr
	"""

	power_2_reac=None
	"""
	Units: VAr
	"""

	power_12_reac=None
	"""
	Units: VAr
	"""

	shunt_1=None
	"""
	Units: S
	"""

	shunt_2=None
	"""
	Units: S
	"""

	shunt_12=None
	"""
	Units: S
	"""

	impedance_1=None
	"""
	Units: Ohm
	"""

	impedance_2=None
	"""
	Units: Ohm
	"""

	impedance_12=None
	"""
	Units: Ohm
	"""

	impedance_1_real=None
	"""
	Units: Ohm
	"""

	impedance_2_real=None
	"""
	Units: Ohm
	"""

	impedance_12_real=None
	"""
	Units: Ohm
	"""

	impedance_1_reac=None
	"""
	Units: Ohm
	"""

	impedance_2_reac=None
	"""
	Units: Ohm
	"""

	impedance_12_reac=None
	"""
	Units: Ohm
	"""

	house_present=None
	"""
	"""

	NR_mode=None
	"""
	"""




class triplex_meter(triplex_node):
	def __init__(self):
		pass

	measured_real_energy=None
	"""
	Units: Wh
	"""

	measured_reactive_energy=None
	"""
	Units: VAh
	"""

	measured_power=None
	"""
	Units: VA
	"""

	indiv_measured_power_1=None
	"""
	Units: VA
	"""

	indiv_measured_power_2=None
	"""
	Units: VA
	"""

	indiv_measured_power_N=None
	"""
	Units: VA
	"""

	measured_demand=None
	"""
	Units: W
	"""

	measured_real_power=None
	"""
	Units: W
	"""

	measured_reactive_power=None
	"""
	Units: VAr
	"""

	meter_power_consumption=None
	"""
	Units: VA
	"""

	measured_voltage_1=None
	"""
	Units: V
	"""

	measured_voltage_2=None
	"""
	Units: V
	"""

	measured_voltage_N=None
	"""
	Units: V
	"""

	measured_current_1=None
	"""
	Units: A
	"""

	measured_current_2=None
	"""
	Units: A
	"""

	measured_current_N=None
	"""
	Units: A
	"""

	customer_interrupted=None
	"""
	"""

	customer_interrupted_secondary=None
	"""
	"""

	monthly_bill=None
	"""
	"""

	previous_monthly_bill=None
	"""
	"""

	previous_monthly_energy=None
	"""
	Units: kWh
	"""

	monthly_fee=None
	"""
	"""

	monthly_energy=None
	"""
	Units: kWh
	"""

	bill_mode=None
	"""
	"""

	power_market=None
	"""
	"""

	bill_day=None
	"""
	"""

	price=None
	"""
	"""

	price_base=None
	"""
	Description: Used only in TIERED_RTP mode to describe the price before the first tier
	"""

	first_tier_price=None
	"""
	"""

	first_tier_energy=None
	"""
	Units: kWh
	"""

	second_tier_price=None
	"""
	"""

	second_tier_energy=None
	"""
	Units: kWh
	"""

	third_tier_price=None
	"""
	"""

	third_tier_energy=None
	"""
	Units: kWh
	"""




class triplex_line(line):
	def __init__(self):
		pass




class switch(link):
	def __init__(self):
		pass

	phase_A_state=None
	"""
	"""

	phase_B_state=None
	"""
	"""

	phase_C_state=None
	"""
	"""

	operating_mode=None
	"""
	"""




class substation(node):
	def __init__(self):
		pass

	distribution_energy=None
	"""
	Units: Wh
	"""

	distribution_power=None
	"""
	Units: VA
	"""

	distribution_demand=None
	"""
	Units: W
	"""

	distribution_voltage_A=None
	"""
	Units: V
	"""

	distribution_voltage_B=None
	"""
	Units: V
	"""

	distribution_voltage_C=None
	"""
	Units: V
	"""

	distribution_current_A=None
	"""
	Units: A
	"""

	distribution_current_B=None
	"""
	Units: A
	"""

	distribution_current_C=None
	"""
	Units: A
	"""

	Network_Node_Base_Power=None
	"""
	Units: MVA
	"""

	Network_Node_Base_Voltage=None
	"""
	Units: V
	"""




class pqload(load):
	def __init__(self):
		pass

	weather=None
	"""
	"""

	T_nominal=None
	"""
	Units: degF
	"""

	Zp_T=None
	"""
	Units: ohm/degF
	"""

	Zp_H=None
	"""
	Units: ohm/%
	"""

	Zp_S=None
	"""
	Units: ohm
	"""

	Zp_W=None
	"""
	Units: ohm/mph
	"""

	Zp_R=None
	"""
	Units: ohm
	"""

	Zp=None
	"""
	Units: ohm
	"""

	Zq_T=None
	"""
	Units: F/degF
	"""

	Zq_H=None
	"""
	Units: F/%
	"""

	Zq_S=None
	"""
	Units: F
	"""

	Zq_W=None
	"""
	Units: F/mph
	"""

	Zq_R=None
	"""
	Units: F
	"""

	Zq=None
	"""
	Units: F
	"""

	Im_T=None
	"""
	Units: A/degF
	"""

	Im_H=None
	"""
	Units: A/%
	"""

	Im_S=None
	"""
	Units: A
	"""

	Im_W=None
	"""
	Units: A/mph
	"""

	Im_R=None
	"""
	Units: A
	"""

	Im=None
	"""
	Units: A
	"""

	Ia_T=None
	"""
	Units: deg/degF
	"""

	Ia_H=None
	"""
	Units: deg/%
	"""

	Ia_S=None
	"""
	Units: deg
	"""

	Ia_W=None
	"""
	Units: deg/mph
	"""

	Ia_R=None
	"""
	Units: deg
	"""

	Ia=None
	"""
	Units: deg
	"""

	Pp_T=None
	"""
	Units: W/degF
	"""

	Pp_H=None
	"""
	Units: W/%
	"""

	Pp_S=None
	"""
	Units: W
	"""

	Pp_W=None
	"""
	Units: W/mph
	"""

	Pp_R=None
	"""
	Units: W
	"""

	Pp=None
	"""
	Units: W
	"""

	Pq_T=None
	"""
	Units: VAr/degF
	"""

	Pq_H=None
	"""
	Units: VAr/%
	"""

	Pq_S=None
	"""
	Units: VAr
	"""

	Pq_W=None
	"""
	Units: VAr/mph
	"""

	Pq_R=None
	"""
	Units: VAr
	"""

	Pq=None
	"""
	Units: VAr
	"""




class series_reactor(link):
	def __init__(self):
		pass

	phase_A_impedance=None
	"""
	Units: Ohm
	Description: Series impedance of reactor on phase A
	"""

	phase_A_resistance=None
	"""
	Units: Ohm
	Description: Resistive portion of phase A's impedance
	"""

	phase_A_reactance=None
	"""
	Units: Ohm
	Description: Reactive portion of phase A's impedance
	"""

	phase_B_impedance=None
	"""
	Units: Ohm
	Description: Series impedance of reactor on phase B
	"""

	phase_B_resistance=None
	"""
	Units: Ohm
	Description: Resistive portion of phase B's impedance
	"""

	phase_B_reactance=None
	"""
	Units: Ohm
	Description: Reactive portion of phase B's impedance
	"""

	phase_C_impedance=None
	"""
	Units: Ohm
	Description: Series impedance of reactor on phase C
	"""

	phase_C_resistance=None
	"""
	Units: Ohm
	Description: Resistive portion of phase C's impedance
	"""

	phase_C_reactance=None
	"""
	Units: Ohm
	Description: Reactive portion of phase C's impedance
	"""

	rated_current_limit=None
	"""
	Units: A
	Description: Rated current limit for the reactor
	"""




class motor(node):
	def __init__(self):
		pass




class recloser(switch):
	def __init__(self):

		self.retry_time=None
		"""
		Units: s
		Description: the amount of time in seconds to wait before the recloser attempts to close
		"""
	
		self.max_number_of_tries=None
		"""
		Description: the number of times the recloser will try to close before permanently opening
		"""
	
		self.number_of_tries=None
		"""
		Description: Current number of tries recloser has attempted
		"""




class sectionalizer(switch):
	def __init__(self):
		pass




