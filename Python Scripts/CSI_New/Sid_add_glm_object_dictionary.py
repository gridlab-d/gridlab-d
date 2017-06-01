'''
Created on Oct 7, 2013

@author: fish334
'''

def create_glm_object_dictionary(glm_dict,glm_object,glm_parameters):
	if glm_dict is None:
		glm_dict = {}
	#glm_dict is the dictionary you wish to add the object too
	#glm_object is the GridLAB-D object you wish to add. Valid arguments are
		#'node'
		#'triplex_node'
		#'load'
		#'triplex_load'
		#'capacitor'
		#'meter'
		#'triplex_meter'
		#'line_configuration'
		#'line_spacing'
		#'overhead_line_conductor'
		#'underground_line_conductor'
		#'transformer_configuration'
		#'regulation_configuration'
		#'overhead_line'
		#'underground_line'
		#'transformer'
		#'regulator'
		#'fuse'
		#'switch'
		#'climate' 
		#'csvreader' 
		#'inverter' 
		#'solar' 
		#'auction'
		#'controller'
		#'passive_controller'
		#'voltdump' 
		#'volt_var_controller'
		#'billdump' 
		#'house_e' 
		#'zipload' 
		#'waterheater' 
		#'player' 
		#'recorder' 
		#'multi_recorder'
		#'collector' 
		
	#glm_parameters is a list containing all the user inputs for the object in a certain order as strings
		# node
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'parent'
		#    3 : 'bustype'
		#    4 : 'phases'
		#    5 : 'nominal_voltage'
		#    6 : 'comments'
		
		# meter
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'parent'
		#    3 : 'bustype'
		#    4 : 'phases'
		#    5 : 'nominal_voltage'
		#    6 : 'comments'
		
		# load
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'parent'
		#    3 : 'bustype'
		#    4 : 'phases'
		#    5 : 'nominal_voltage'
		#    6 : 'load_class'
		#    7 : 'constant_power_A'
		#    8 : 'constant_power_B'
		#    9 : 'constant_power_C'
		#    10 : 'constant_current_A'
		#    11 : 'constant_current_B'
		#    12 : 'constant_current_C'
		#    13 : 'constant_impedance_A'
		#    14 : 'constant_impedance_B'
		#    15 : 'constant_impedance_C'
		#    16 : 'base_power_A'
		#    17 : 'base_power_B'
		#    18 : 'base_power_C'
		#    19 : 'power_pf_A'
		#    20 : 'power_pf_B'
		#    21 : 'power_pf_C'
		#    22 : 'current_pf_A'
		#    23 : 'current_pf_B'
		#    24 : 'current_pf_C'
		#    25 : 'impedance_pf_A'
		#    26 : 'impedance_pf_B'
		#    27 : 'impedance_pf_C'
		#    28 : 'power_fraction_A'
		#    29 : 'power_fraction_B'
		#    30 : 'power_fraction_C'
		#    31 : 'current_fraction_A'
		#    32 : 'current_fraction_B'
		#    33 : 'current_fraction_C'
		#    34 : 'impedance_fraction_A'
		#    35 : 'impedance_fraction_B'
		#    36 : 'impedance_fraction_C'
		#    37 : 'comments'
		
		# triplex_node
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'parent'
		#    3 : 'bustype'
		#    4 : 'phases'
		#    5 : 'nominal_voltage'
		#    6 : 'power_12'
		#    7 : 'comments'
		#    8 : 'load_class'
		
		# triplex_meter
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'parent'
		#    3 : 'bustype'
		#    4 : 'phases'
		#    5 : 'nominal_voltage'
		#    6 : 'comments'
		
		# triplex_load
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'parent'
		#    3 : 'bustype'
		#    4 : 'phases'
		#    5 : 'nominal_voltage'
		#    6 : 'load_class'
		#    7 : 'constant_power_1'
		#    8 : 'constant_power_2'
		#    9 : 'constant_power_12'
		#    10 : 'constant_current_1'
		#    11 : 'constant_current_2'
		#    12 : 'constant_current_12'
		#    13 : 'constant_impedance_1'
		#    14 : 'constant_impedance_2'
		#    15 : 'constant_impedance_12'
		#    16 : 'base_power_1'
		#    17 : 'base_power_2'
		#    18 : 'base_power_12'
		#    19 : 'power_pf_1'
		#    20 : 'power_pf_2'
		#    21 : 'power_pf_12'
		#    22 : 'current_pf_1'
		#    23 : 'current_pf_2'
		#    24 : 'current_pf_12'
		#    25 : 'impedance_pf_1'
		#    26 : 'impedance_pf_2'
		#    27 : 'impedance_pf_12'
		#    28 : 'power_fraction_1'
		#    29 : 'power_fraction_2'
		#    30 : 'power_fraction_12'
		#    31 : 'current_fraction_1'
		#    32 : 'current_fraction_2'
		#    33 : 'current_fraction_12'
		#    34 : 'impedance_fraction_1'
		#    35 : 'impedance_fraction_2'
		#    36 : 'impedance_fraction_12'
		#    37 : 'comments'
		
		# capacitor
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'parent'
		#    3 : 'phases'
		#    4 : 'nominal_voltage'
		#    5 : 'pt_phase'
		#    6 : 'phases_connected'
		#    7 : 'switchA'
		#    8 : 'switchB'
		#    9 : 'switchC'
		#    10 : 'control'
		#    11 : 'voltage_set_high'
		#    12 : 'voltage_set_low'
		#    13 : 'VAr_set_high'
		#    14 : 'VAr_set_low'
		#    15 : 'current_set_high'
		#    16 : 'current_set_low'
		#    17 : 'capacitor_A'
		#    18 : 'capacitor_B'
		#    19 : 'capacitor_C'
		#    20 : 'cap_nominal_voltage'
		#    21 : 'time_delay'
		#    22 : 'dwell_time'
		#    23 : 'lockout_time'
		#    24 : 'remote_sense'
		#    25 : 'remote_sense_B'
		#    26 : 'control_level'
		#    27 : 'comments'
		
		# fuse
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'phases'
		#    3 : 'from'
		#    4 : 'to'
		#    5 : 'phase_A_status'
		#    6 : 'phase_B_status'
		#    7 : 'phase_C_status'
		#    8 : 'repair_dist_type'
		#    9 : 'current_limit'
		#    10 : 'mean_replacement_time'
		#    11 : 'comments'
		#    12 : 'status'
		
		# switch
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'phases'
		#    3 : 'from'
		#    4 : 'to'
		#    5 : 'phase_A_state'
		#    6 : 'phase_B_state'
		#    7 : 'phase_C_state'
		#    8 : 'operating_mode'
		#    9 : 'comments'
		
		# overhead_line
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'phases'
		#    3 : 'from'
		#    4 : 'to'
		#    5 : 'length'
		#    6 : 'configuration'
		#    7 : 'comments'
		
		# underground_line
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'phases'
		#    3 : 'from'
		#    4 : 'to'
		#    5 : 'length'
		#    6 : 'configuration'
		#    7 : 'comments'
		
		# triplex_line
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'phases'
		#    3 : 'from'
		#    4 : 'to'
		#    5 : 'length'
		#    6 : 'configuration'
		#    7 : 'comments'
		
		# transformer
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'phases'
		#    3 : 'from'
		#    4 : 'to'
		#    5 : 'configuration'
		#    6 : 'climate'
		#    7 : 'aging_constant'
		#    8 : 'use_thermal_model'
		#    9 : 'aging_granularity'
		#    10 : 'comments'
		
		# regulator
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'phases'
		#    3 : 'from'
		#    4 : 'to'
		#    5 : 'configuration'
		#    6 : 'sense_node'
		#    7 : 'comments'
		
		# line_configuration
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'conductor_A'
		#    3 : 'conductor_B'
		#    4 : 'conductor_C'
		#    5 : 'conductor_N'
		#    6 : 'spacing'
		#    7 : 'z11'
		#    8 : 'z12'
		#    9 : 'z13'
		#    10 : 'z21'
		#    11 : 'z22'
		#    12 : 'z23'
		#    13 : 'z31'
		#    14 : 'z32'
		#    15 : 'z33'
		#    16 : 'comments'
		
		# triplex_line_configuration
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'conductor_1'
		#    3 : 'conductor_2'
		#    4 : 'conductor_N'
		#    5 : 'insulation_thickness'
		#    6 : 'diameter'
		#    7 : 'spacing'
		#    8 : 'z11'
		#    9 : 'z12'
		#    10 : 'z21'
		#    11 : 'z22'
		#    12 : 'comments'
		
		# transformer_configuration
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'connect_type'
		#    3 : 'install_type'
		#    4 : 'coolant_type'
		#    5 : 'cooling_type'
		#    6 : 'primary_voltage'
		#    7 : 'secondary_voltage'
		#    8 : 'power_rating'
		#    9 : 'powerA_rating'
		#    10 : 'powerB_rating'
		#    11 : 'powerC_rating'
		#    12 : 'impedance'
		#    13 : 'shunt_impedance'
		#    14 : 'core_coil_weight'
		#    15 : 'tank_fittings_weight'
		#    16 : 'oil_volume'
		#    17 : 'rated_winding_time_constant'
		#    18 : 'rated_winding_hot_spot_rise'
		#    19 : 'rated_top_oil_rice'
		#    20 : 'no_load_loss'
		#    21 : 'full_load_loss'
		#    22 : 'reactance_resistance_ratio'
		#    23 : 'installed_insulation_life'
		#    24 : 'comments'
		
		# regulator_configuration
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'connect_type'
		#    3 : 'band_center'
		#    4 : 'band_width'
		#    5 : 'time_delay'
		#    6 : 'dwell_time'
		#    7 : 'raise_taps'
		#    8 : 'lower_taps'
		#    9 : 'current_transducer_ratio'
		#    10 : 'power_transducer_ratio'
		#    11 : 'compensator_r_setting_A'
		#    12 : 'compensator_x_setting_A'
		#    13 : 'compensator_r_setting_B'
		#    14 : 'compensator_x_setting_B'
		#    15 : 'compensator_r_setting_C'
		#    16 : 'compensator_x_setting_C'
		#    17 : 'CT_phase'
		#    18 : 'PT_phase'
		#    19 : 'regulation'
		#    20 : 'control_level'
		#    21 : 'Control'
		#    22 : 'Type'
		#    23 : 'tap_pos_A'
		#    24 : 'tap_pos_B'
		#    25 : 'tap_pos_C'
		#    26 : 'comments'
		
		# line_spacing
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'distance_AB'
		#    3 : 'distance_AC'
		#    4 : 'distance_AN'
		#    5 : 'distance_AE'
		#    6 : 'distance_BC'
		#    7 : 'distance_BN'
		#    8 : 'distance_BE'
		#    9 : 'distance_CN'
		#    10 : 'distance_CE'
		#    11 : 'distance_NE'
		#    12 : 'comments'
		
		# overhead_line_conductor
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'geometric_mean_radius'
		#    3 : 'resistance'
		#    4 : 'diameter'
		#    5 : 'rating.summer.continuous'
		#    6 : 'rating.summer.emergency'
		#    7 : 'rating.winter.continuous'
		#    8 : 'rating.winter.emergency'
		#    9 : 'comments'
		
		# underground_line_conductor
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'outer_diameter'
		#    3 : 'conductor_gmr'
		#    4 : 'conductor_diameter'
		#    5 : 'conductor_resistance'
		#    6 : 'neutral_gmr'
		#    7 : 'neutral_diameter'
		#    8 : 'neutral_resistance'
		#    9 : 'neutral_strands'
		#    10 : 'insulation_relative_permativitty'
		#    11 : 'shield_gmr'
		#    12 : 'shield_resistance'
		#    13 : 'rating.summer.continuous'
		#    14: 'rating.summer.emergency'
		#    15 : 'rating.winter.continuous'
		#    16 : 'rating.winter.emergency'
		#    17 : 'comments'
		
		# overhead_line_conductor
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'resistance'
		#    3 : 'geometric_mean_radius'
		#    4 : 'rating.summer.continuous'
		#    5 : 'rating.summer.emergency'
		#    6 : 'rating.winter.continuous'
		#    7 : 'rating.winter.emergency'
		#    8 : 'comments'
		
		# climate
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'temperature'
		#    3 : 'tmyfile'
		#    4 : 'interpolate'
		#    5 : 'comments' 
		
		# csvreader
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'filename'
		#    3 : 'timefmt'
		#    4 : 'timezone'
		#    5 : 'comments'
		
		# inverter
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'parent'
		#    3 : 'generator_status'
		#    4 : 'generator_mode'
		#    5 : 'inverter_type'
		#    6 : 'four_quadrant_control_mode'
		#    7 : 'coupling_inductance_type'
		#    8 : 'control_mode_switch'
		#    9 : 'V_In'
		#    10 : 'I_In'
		#    11 : 'power_factor'
		#    12 : 'P_Out'
		#    13 : 'Q_Out'
		#    14 : 'use_multipoint_efficiency'
		#    15 : 'inverter_manufacturer'
		#    16 : 'maximum_dc_power'
		#    17 : 'maximum_dc_voltage'
		#    18 : 'minimum_dc_power'
		#    19 : 'c_o'
		#    20 : 'c_1'
		#    21 : 'c_2'
		#    22 : 'c_3'
		#    23 : 'nominal_frequency'
		#    24 : 'initial_inverter_frequency'
		#    25 : 'PLL_measured_frequency'
		#    26 : 'nominal_VRMSLG'
		#    27 : 'measured_VRMSLL'
		#    28 : 'coupling_L1'
		#    29 : 'coupling_L2'
		#    30 : 'coupling_C'
		#    31 : 'constant_PQ_KPPLL'
		#    32 : 'constant_PQ_KIPLL'
		#    33 : 'constant_PQ_KPP'
		#    34 : 'constant_PQ_KIP'
		#    35 : 'constant_PQ_KPQ'
		#    36 : 'constant_PQ_KIQ'
		#    37 : 'constant_PQ_T_MeasP'
		#    38 : 'constant_PQ_T_MeasQ'
		#    39 : 'constant_PQ_T_MeasV'
		#    40 : 'constant_PQ_TPRefFilter'
		#    41 : 'constant_PQ_TQRefFilter'
		#    42 : 'constant_PQ_vt_max_angle_ref'
		#    43 : 'constant_PQ_vt_min_angle_ref'
		#    44 : 'constant_PQ_vt_max_mag_ref'
		#    45 : 'constant_PQ_vt_min_mag_ref'
		#    46 : 'droop_PQ_KPPLL'
		#    47 : 'droop_PQ_KIPLL'
		#    48 : 'droop_PQ_KPP'
		#    49 : 'droop_PQ_KIP'
		#    50 : 'droop_PQ_KPQ'
		#    51 : 'droop_PQ_KIQ'
		#    52 : 'droop_PQ_T_MeasP'
		#    53 : 'droop_PQ_T_MeasQ'
		#    54 : 'droop_PQ_T_MeasV'
		#    55 : 'droop_PQ_TPRefFilter'
		#    56 : 'droop_PQ_TQRefFilter'
		#    57 : 'droop_PQ_vt_max_angle_ref'
		#    58 : 'droop_PQ_vt_min_angle_ref'
		#    59 : 'droop_PQ_vt_max_mag_ref'
		#    60 : 'droop_PQ_vt_min_mag_ref'
		#    61 : 'droop_P1'
		#    62 : 'droop_P2'
		#    63 : 'droop_Q1'
		#    64 : 'droop_Q2'
		#    65 : 'droop_f1'
		#    66 : 'droop_f2'
		#    67 : 'droop_V1'
		#    68 : 'droop_V2'
		#    69 : 'islanded_state'
		
		# solar
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'parent'
		#    3 : 'generator_status'
		#    4 : 'generator_mode'
		#    5 : 'panel_type'
		#    6 : 'power_type'
		#    7 : 'INSTALLATION_TYPE'
		#    8 : 'SOLAR_TILT_MODEL'
		#    9 : 'SOLAR_POWER_MODEL'
		#    10 : 'a_coeff'
		#    11 : 'b_coeff'
		#    12 : 'dT_coeff'
		#    13 : 'T_coeff'
		#    14 : 'NOCT'
		#    15 : 'Tmodule'
		#    16 : 'Tambient'
		#    17 : 'wind_speed'
		#    18 : 'ambient_temperature'
		#    19 : 'Insolation'
		#    20 : 'Rinternal'
		#    21 : 'Rated_Insolation'
		#    22 : 'Pmax_temp_coeff'
		#    23 : 'Voc_temp_coeff'
		#    24 : 'V_Max'
		#    25 : 'Voc_Max'
		#    26 : 'Voc'
		#    27 : 'efficiency'
		#    28 : 'area'
		#    29 : 'soiling'
		#    30 : 'derating'
		#    31 : 'Rated_kVA'
		#    32 : 'weather'
		#    33 : 'shading_factor'
		#    34 : 'tilt_angle'
		#    35 : 'orientation_azimuth'
		#    36 : 'latitude_angle_fix'
		#    37 : 'orientation'
		#    38 : 'phases'
		#    39 : 'comments'
		
		# auction
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'type'
		#    3 : 'unit'
		#    4 : 'period'
		#    5 : 'latency'
		#    6 : 'market_id'
		#    7 : 'price_cap'
		#    8 : 'special_mode'
		#    9 : 'statistic_mode'
		#    10 : 'fixed_price'
		#    11 : 'fixed_quantity'
		#    12 : 'capacity_reference_object'
		#    13 : 'capacity_reference_property'
		#    14 : 'capacity_reference_bid_price'
		#    15 : 'capacity_reference_bid_quantity'
		#    16 : 'max_capacity_reference_bid_quantity'
		#    17 : 'init_price'
		#    18 : 'init_stdev'
		#    19 : 'future_mean_price'
		#    20 : 'use_future_mean_price'
		#    21 : 'margin_mode'
		#    22 : 'warmup'
		#    23 : 'transaction_log_file'
		#    24 : 'curve_log_file'
		#    25 : 'curve_log_info'
		#    26 : 'comments'
		
		# controller
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'simple_mode'
		#    3 : 'bid_mode'
		#    4 : 'use_override'
		#    5 : 'control_mode'
		#    6 : 'resolve_mode'
		#    7 : 'ramp_low'
		#    8 : 'ramp_high'
		#    9 : 'range_low'
		#    10 : 'range_high'
		#    11 : 'target'
		#    12 : 'setpoint' 
		#    13 : 'demand'
		#    14 : 'load'
		#    15 : 'total'
		#    16 : 'market'
		#    17 : 'state'
		#    18 : 'avg_target'
		#    19 : 'std_target'
		#    20 : 'base_setpoint'
		#    21 : 'period'
		#    22 : 'slider_setting'
		#    23 : 'slider_setting_heat'
		#    24 : 'slider_setting_cool'
		#    25 : 'override'
		#    26 : 'heating_range_high'
		#    27 : 'heating_range_low'
		#    28 : 'heating_ramp_high'
		#    29 : 'heating_ramp_low'
		#    30 : 'cooling_range_high'
		#    31 : 'cooling_range_low'
		#    32 : 'cooling_ramp_high' 
		#    33 : 'cooling_ramp_low'
		#    34 : 'heating_base_setpoint'
		#    35 : 'cooling_base_setpoint'
		#    36 : 'deadband'
		#    37 : 'heating_setpoint'
		#    38 : 'heating_demand'
		#    39 : 'cooling_setpoint'
		#    40 : 'cooling_demand'
		#    41 : 'sliding_time_delay'
		#    42 : 'use_predictive_bidding' 
		#    43 : 'comments' 
		
		# passive_controller
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'control_mode'
		#    3 : 'dlc_mode'
		#    4 : 'input_state'
		#    5 : 'input_setpoint'
		#    6 : 'input_chained'
		#    7 : 'sensitivity'
		#    8 : 'period'
		#    9 : 'expectation_prop'
		#    10 : 'expectation_obj'
		#    11 : 'setpoint_prop'
		#    12 : 'state_prop'
		#    13 : 'observation_obj'
		#    14 : 'observation_prop'
		#    15 : 'mean_observation_prop'
		#    16 : 'stdev_observation_prop'
		#    17 : 'cycle_length'
		#    18 : 'base_setpoint'
		#    19 : 'ramp_high'
		#    20 : 'ramp_low'
		#    21 : 'range_high'
		#    22 : 'range_low'
		#    23 : 'critical_day'
		#    24 : 'two_tier_cpp'
		#    25 : 'daily_elasticity'
		#    26 : 'sub_elasticity_first_second'
		#    27 : 'sub_elasticity_first_third'
		#    28 : 'second_tier_hours'
		#    29 : 'first_tier_hours'
		#    30 : 'first_tier_price'
		#    31 : 'second_tier_price'
		#    32 : 'third_tier_price'
		#    33 : 'linearize_elasticity'
		#    34 : 'price_offset'
		#    35 : 'pool_pump_model'
		#    36 : 'base_duty_cycle'
		#    37 : 'distribution_type'
		#    38 : 'comfort_level'
		#    39 : 'cycle_length_off'
		#    40 : 'cycle_length_on'
		#    41 : 'comments'
		
		# voltdump
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'runtime'
		#    3 : 'file'
		#    4 : 'runcount'
		#    5 : 'mode'
		#    6 : 'comments'
		
		# volt_var_control
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'control_method'
		#    3 : 'capacitor_delay'
		#    4 : 'regulator_delay'
		#    5 : 'desired_pf'
		#    6 : 'pf_signed'
		#    7 : 'd_max'
		#    8 : 'd_min'
		#    9 : 'substation_link'
		#    10 : 'pf_phase'
		#    11 : 'regulator_list'
		#    12 : 'capacitor_list'
		#    13 : 'voltage_measurements'
		#    14 : 'minimum_voltages'
		#    15 : 'maximum_voltages'
		#    16 : 'desired_voltages'
		#    17 : 'max_vdrop'
		#    18 : 'high_load_deadband'
		#    19 : 'low_load_deadband'
		#    20 : 'comments'
		
		# billdump
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'runtime'
		#    3 : 'filename'
		#    4 : 'runcount'
		#    5 : 'meter_type'
		#    6 : 'comments'
		
		# house_e
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'parent'
		#    3 : 'system_type'
		#    4 : 'heating_system_type'
		#    5 : 'cooling_system_type'
		#    6 : 'auxiliary_system_type'
		#    7 : 'auxiliary_strategy'
		#    8 : 'system_mode'
		#    9 : 'fan_type'
		#    10 : 'thermal_integrity_level'
		#    11 : 'glass_type'
		#    12 : 'window_frame'
		#    13 : 'glazing_treatment'
		#    14 : 'glazing_layers'
		#    15 : 'motor_model'
		#    16 : 'motor_efficiency'
		#    17 : 'weather'
		#    18 : 'floor_area'
		#    19 : 'gross_wall_area'
		#    20 : 'ceiling_height'
		#    21 : 'aspect_ratio'
		#    22 : 'envelope_UA'
		#    23 : 'window_wall_ratio'
		#    24 : 'number_of_doors'
		#    25 : 'exterior_wall_fraction'
		#    26 : 'interior_exterior_wall_ratio'
		#    27 : 'exterior_ceiling_fraction'
		#    28 : 'exterior_floor_fraction'
		#    29 : 'number_of_stories'
		#    30 : 'Rroof'
		#    31 : 'Rwall'
		#    32 : 'Rfloor'
		#    33 : 'Rwindows'
		#    34 : 'Rdoors'
		#    35 : 'window_shading'
		#    36 : 'window_exterior_transmission_coefficient'
		#    37 : 'cooling_design_temperature'
		#    38 : 'heating_design_temperature'
		#    39 : 'design_peak_solar'
		#    40 : 'design_internal_gains'
		#    41 : 'cooling_supply_air_temp'
		#    42 : 'heating_supply_air_temp'
		#    43 : 'duct_pressure_drop'
		#    44 : 'heating_COP'
		#    45 : 'cooling_COP'
		#    46 : 'design_heating_capacity'
		#    47 : 'design_cooling_capacity'
		#    48 : 'design_heating_setpoint'
		#    49 : 'design_cooling_setpoint'
		#    50 : 'auxiliary_heat_capacity'
		#    51 : 'over_sizing_factor'
		#    52 : 'solar_heatgain_factor'
		#    53 : 'airchange_per_hour'
		#    54 : 'internal_gain'
		#    55 : 'solar_gain'
		#    56 : 'incident_solar_radiation'
		#    57 : 'heat_cool_gain'
		#    58 : 'air_heat_fraction'
		#    59 : 'mass_heat_capacity'
		#    60 : 'mass_heat_coeff'
		#    61 : 'air_heat_capacity'
		#    62 : 'total_thermal_mass_per_floor_area'
		#    63 : 'interior_surface_heat_transfer_coeff'
		#    64 : 'design_internal_gain_density'
		#    65 : 'fan_design_power'
		#    66 : 'fan_low_power_fraction'
		#    67 : 'fan_power'
		#    68 : 'fan_design_airflow'
		#    69 : 'fan_impedance_fraction'
		#    70 : 'fan_power_fraction'
		#    71 : 'hvac_motor_efficiency'
		#    72 : 'hvac_motor_loss_power_factor'
		#    73 : 'heating_setpoint'
		#    74 : 'cooling_setpoint'
		#    75 : 'aux_heat_deadband'
		#    76 : 'aux_heat_temperature_lockout'
		#    77 : 'aux_heat_time_delay'
		#    78 : 'thermostat_deadband'
		#    79 : 'thermostat_cycle_time'
		#    80 : 'thermostat_last_cycle_time'
		#    81 : 'last_mode_timer'
		#    82 : 'panel'
		#    83 : 'hvac_breaker_rating'
		#    84 : 'hvac_power_factor'
		#    85 : 'hvac_load'
		#    86 : 'total_load'
		#    87 : 'comments'
		
		# zipload
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'parent'
		#    3 : 'heat_fraction'
		#    4 : 'base_power'
		#    5 : 'power_pf'
		#    6 : 'current_pf'
		#    7 : 'impedance_pf'
		#    8 : 'is_240'
		#    9 : 'breaker_val'
		#    10 : 'actual_power'
		#    11 : 'multiplier'
		#    12 : 'heatgain_only'
		#    13 : 'demand_response_mode'
		#    14 : 'number_of_devices'
		#    15 : 'thermostatic_control_range'
		#    16 : 'number_of_devices_off'
		#    17 : 'number_of_devices_on'
		#    18 : 'rate_of_cooling'
		#    19 : 'rate_of_heating'
		#    20 : 'temperature'
		#    21 : 'phi'
		#    22 : 'demand_rate'
		#    23 : 'duty_cycle'
		#    24 : 'recovery_duty_cycle'
		#    25 : 'period'
		#    26 : 'power_fraction'
		#    27 : 'current_fraction'
		#    28 : 'impedance_fraction'
		#    29 : 'phase'
		#    30 : 'comments'
		
		# waterheater
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'parent'
		#    3 : 'heat_mode'
		#    4 : 'location'
		#    5 : 'tank_volume'
		#    6 : 'tank_UA'
		#    7 : 'tank_diameter'
		#    8 : 'water_demand'
		#    9 : 'heating_element_capacity'
		#    10 : 'inlet_water_temperature'
		#    11 : 'tank_setpoint'
		#    12 : 'thermostat_deadband'
		#    13 : 'temperature'
		#    14 : 'height'
		#    15 : 'demand'
		#    16 : 'actual_load'
		#    17 : 'previous_load'
		#    18 : 'actual_power'
		#    19 : 'is_waterheater_on'
		#    20 : 'gas_fan_power'
		#    21 : 'gas_standby_power'
		#    22 : 'comments'
		
		# player
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'parent'
		#    3 : 'property'
		#    4 : 'file'
		#    5 : 'loop'
		#    6 : 'comments'
		
		# recorder
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'parent'
		#    3 : 'property'
		#    4 : 'file'
		#    5 : 'trigger'
		#    6 : 'interval'
		#    7 : 'limit'
		#    8 : 'comments'
		
		# multi_recorder
		#    0 : 'name'
		#    1 : 'groupid'
		#    2 : 'parent'
		#    3 : 'property'
		#    4 : 'file'
		#    5 : 'trigger'
		#    6 : 'interval'
		#    7 : 'limit'
		#    8 : 'comments'
		
		# collector
		#    0 : 'name'
		#    1 : 'group'
		#    2 : 'property'
		#    3 : 'file'
		#    4 : 'comments'
		
		# violation
		#    0 : file
		#    1 : summary
		#    2 : interval
		#    3 : strict
		#    4 : echo
		#    5 : violation_flag
		#    6 : xfmr_thermal_limit_upper
		#    7 : xfmr_thermal_limit_lower
		#    8 : line_thermal_limit_upper
		#    9 : line_thermal_limit_lower
		#    10 : node_instantaneous_voltage_limit_upper
		#    11 : node_instantaneous_voltage_limit_lower
		#    12 : node_continuous_voltage_limit_upper
		#    13 : node_continuous_voltage_limit_lower
		#    14 : node_continuous_voltage_limit_interal
		#    15 : substation_breaker_A_limit
		#    16 : substation_breaker_B_limit
		#    17 : substation_breaker_C_limit
		#    18 : virtual_substation
		#    19 : inverter_v_chng_per_interval_upper_bound
		#    20 : inverter_v_chng_per_interval_lower_bound
		#    21 : inverter_v_chng_interval
		#    22 : secondary_dist_voltage_rise_upper_limit
		#    23 : secondary_dist_voltage_rise_lower_limit
		
		

	# Function that creates the object dictionary
	def create_object(glm_props, obj):
		# Create an dictionary where (key : value) corresponds to (GridLAB-D object : list of GridLAB-D parameters)
######### ALL OBJECTS WE WISH TO CREATE SHOULD HAVE A STORED (KEY : VALUE) IN THIS DICTIONARY!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		glm_objects={'node' : ['name','groupid','parent','bustype','phases','nominal_voltage','comments'],
							#'meter' : ['name','groupid','parent','bustype','phases','voltage_A','voltage_B','voltage_C','nominal_voltage','comments'],
							'meter' : ['name','groupid','parent','phases','nominal_voltage'],
						    'load' : ['name','groupid','parent','bustype','phases','nominal_voltage','load_class','constant_power_A','constant_power_B','constant_power_C','constant_current_A','constant_current_B','constant_current_C','constant_impedance_A','constant_impedance_B','constant_impedance_C','base_power_A','base_power_B','base_power_C','power_pf_A','power_pf_B','power_pf_C','current_pf_A','current_pf_B','current_pf_C','impedance_pf_A','impedance_pf_B','impedance_pf_C','power_pf_A','power_fraction_B','power_fraction_C','current_fraction_A','current_fraction_B','current_fraction_C','impedance_fraction_A','impedance_fraction_B','impedance_fraction_C','comments'],
						    'triplex_node' : ['name','groupid','parent','bustype','phases','nominal_voltage','power_12','comments','load_class'],
						    #'triplex_meter' : ['name','groupid','parent','bustype','phases','nominal_voltage','comments'],
						    'triplex_meter': ['name','groupid','parent','bustype','phases','nominal_voltage'],
						    'triplex_load' : ['name','groupid','parent','bustype','phases','nominal_voltage','load_class','constant_power_1','constant_power_2','constant_power_12','constant_current_1','constant_current_2','constant_current_12','constant_impedance_1','constant_impedance_2','constant_impedance_12','base_power_1','base_power_2','base_power_12','power_pf_1','power_pf_2','power_pf_12','current_pf_1','current_pf_2','current_pf_12','impedance_pf_1','impedance_pf_2','impedance_pf_12','power_pf_1','power_fraction_2','power_fraction_12','current_fraction_1','current_fraction_2','current_fraction_12','impedance_fraction_1','impedance_fraction_2','impedance_fraction_12','comments'],
						    'capacitor' : ['name','groupid','parent','phases','nominal_voltage','pt_phase','phases_connected','switchA','switchB','switchC','control','voltage_set_high','voltage_set_low','VAr_set_high','VAr_set_low','current_set_high','current_set_low','capacitor_A','capacitor_B','capacitor_C','cap_nominal_voltage','time_delay','dwell_time','lockout_time','remote_sense','remote_sense_B','control_level','comments'],
						    'fuse' : ['name','groupid','phases','from','to','phase_A_status','phase_B_status','phase_C_status','repair_dist_type','current_limit','mean_replacement_time','comments','status'],
						    'switch' : ['name','groupid','phases','from','to','phase_A_state','phase_B_state','phase_C_state','operating_mode','comments'],
						    'overhead_line' : ['name','groupid','phases','from','to','length','configuration','comments'],
						    'underground_line' : ['name','groupid','phases','from','to','length','configuration','comments'],
						    'triplex_line' : ['name','groupid','phases','from','to','length','configuration','comments'],						    
						    'transformer' : ['name','groupid','phases','from','to','configuration','climate','aging_constant','use_thermal_model','aging_granularity','comments'],
						    'regulator' : ['name','groupid','phases','from','to','configuration','sense_node','comments'],
						    'line_configuration' : ['name','groupid','conductor_A','conductor_B','conductor_C','conductor_N','spacing','z11','z12','z13','z21','z22','z23','z31','z32','z33','comments'],
						    'triplex_line_configuration' : ['name','groupid','conductor_1','conductor_2','conductor_N','insulation_thickness','diameter','spacing','z11','z12','z21','z22','comments'],
						    'transformer_configuration' : ['name','groupid','connect_type','install_type','coolant_type','cooliing_type','primary_voltage','secondary_voltage','power_rating','powerA_rating','powerB_rating','powerC_rating','impedance','shunt_impedance','core_coil_weight','tank_fittings_weight','oil_volume','rated_winding_time_constant','rated_winding_hot_spot_rise','rated_top_oil_rise','no_load_loss','full_load_loss','reactance_resistance_ratio','installed_insulation_life','comments'],
						    'regulator_configuration' : ['name','groupid','connect_type','band_center','band_width','time_delay','dwell_time','raise_taps','lower_taps','current_transducer_ratio','power_transducer_ratio','compensator_r_setting_A','compensator_x_setting_A','compensator_r_setting_B','compensator_x_setting_B','compensator_r_setting_C','compensator_x_setting_C','CT_phase','PT_phase','regulation','control_level','Control','Type','tap_pos_A','tap_pos_B','tap_pos_C','comments'],
						    'line_spacing' : ['name','groupid','distance_AB','distance_AC','distance_AN','distance_AE','distance_BC','distance_BN','distance_BE','distance_CN','distance_CE','distance_NE','comments'],
						    'overhead_line_conductor' : ['name','groupid','geometric_mean_radius','resistance','diameter','rating.summer.continuous','rating.summer.emergency','rating.winter.continuous','rating.winter.emergency','comments'],
						    'underground_line_conductor' : ['name','groupid','outer_diameter','conductor_gmr','conductor_diameter','conductor_resistance','neutral_gmr','neutral_diameter','neutral_resistance','neutral_strands','insulation_relative_permitivitty','shield_gmr','shield_resistance','rating.summer.continuous','rating.summer.emergency','rating.winter.continuous','rating.winter.emergency','comments'],
						    'triplex_line_conductor' : ['name','groupid','resistance','geometric_mean_radius','rating.summer.continuous','rating.summer.emergency','rating.winter.continuous','rating.winter.emergency','comments'],
						    'climate' : ['name','groupid','temperature','tmyfile','interpolate','comments'],
						    'csvreader' : ['name','groupid','filename','timefmt','timezone','comments'],
						    #'inverter' : ['name','groupid','parent','generator_status','generator_mode','inverter_type','four_quadrant_control_mode','coupling_inductance_type','control_mode_switch','V_In','I_In','power_factor','P_Out','Q_Out','use_multipoint_efficiency','inverter_manufacturer','maximum_dc_power','maximum_dc_voltage','minimum_dc_power','c_o','c_1','c_2','c_3','nominal_frequency','initial_inverter_frequency','PLL_measured_frequency','nominal_VRMSLG','measured_VRMSLL','coupling_L1','coupling_L2','coupling_C','constant_PQ_KPPLL','constant_PQ_KIPLL','constant_PQ_KPP','constant_PQ_KIP','constant_PQ_KPQ','constant_PQ_KIQ','constant_PQ_T_MeasP','constant_PQ_T_MeasQ','constant_PQ_T_MeasV','constant_PQ_TPRefFilter','constant_PQ_TQRefFilter','constant_PQ_vt_max_angle_ref','constant_PQ_vt_min_angle_ref','constant_PQ_vt_max_mag_ref','constant_PQ_vt_min_mag_ref','droop_PQ_KPPLL','droop_PQ_KIPLL','droop_PQ_KPP','droop_PQ_KIP','droop_PQ_KPQ','droop_PQ_KIQ','droop_PQ_T_MeasP','droop_PQ_T_MeasQ','droop_PQ_T_MeasV','droop_PQ_TPRefFilter','droop_PQ_TQRefFilter','droop_PQ_vt_max_angle_ref','droop_PQ_vt_min_angle_ref','droop_PQ_vt_max_mag_ref','droop_PQ_vt_min_mag_ref','droop_P1','droop_P2','droop_Q1','droop_Q2','droop_f1','droop_f2','droop_V1','droop_V2','islanded_state','comments'],
						    #'inverter' : ['name','parent','generator_status','inverter_type','four_quadrant_control_mode','power_factor', 'rated_power'],
							'inverter' : ['name','parent','generator_mode','generator_status','inverter_efficiency','inverter_type','power_factor', 'rated_power'],
						    #'solar' : ['name','groupid','parent','generator_status','generator_mode','panel_type','power_type','INSTALLATION_TYPE','SOLAR_TILT_MODEL','SOLAR_POWER_MODEL','a_coeff','b_coeff','dT_coeff','T_coeff','NOCT','Tmodule','Tambient','wind_speed','ambient_temperature','Insolation','Rinternal','Rated_Insolation','Pmax_temp_coeff','Voc_temp_coeff','V_Max','Voc_Max','Voc','efficiency','area','soiling','derating','Rated_kVA','weather','shading_factor','tilt_angle','orientation_azimuth','latitude_angle_fix','orientation','phases','comments'],
						    'solar' : ['name','SOLAR_POWER_MODEL', 'parent','generator_status','generator_mode','panel_type','efficiency','rated_power','tilt_angle','orientation_azimuth','orientation'],
						    'auction' : ['name','groupid','type','unit','period','latency','market_id','price_cap','special_mode','statistic_mode','fixed_price','fixed_quantity','capacity_reference_object','capacity_reference_property','capacity_reference_bid_price','capacity_reference_bid_quantity','max_capacity_reference_bid_quantity','init_price','init_stdev','future_mean_price','use_future_mean_price','margin_mode','warmup','transaction_log_file','curve_log_file','curve_log_info','comments'],
						    'controller' : ['name','groupid','simple_mode','bid_mode','use_override','control_mode','resolve_mode','ramp_low','ramp_high','range_low','range_high','target','setpoint','demand','load','total','market','state','avg_target','std_target','base_setpoint','period','slider_setting','slider_setting_heat','slider_setting_cool','override','heating_range_high','heating_range_low','heating_ramp_high','heating_ramp_low','cooling_range_high','cooling_range_low','cooling_ramp_high','cooling_ramp_low','heating_base_setpoint','cooling_base_setpoint','deadband','heating_setpoint','heating_demand','cooling_setpoint','cooling_demand','sliding_time_delay','use_predictive_bidding','comments'],
						    'passive_controller' : ['name','groupid','control_mode','dlc_mode','input_state','input_setpoint','input_chained','sensitivity','period','expectation_prop','expectation_obj','setpoint_prop','state_prop','observation_obj','observation_prop','mean_observation_prop','stdev_observation_prop','cycle_length','base_setpoint','ramp_high','ramp_low','range_high','range_low','critical_day','two_tier_cpp','daily_elasticity','sub_elasticity_first_second','sub_elasticity_first_third','second_tier_hours','first_tier_hours','first_tier_price','second_tier_price','third_tier_price','linearize_elasticity','price_offset','pool_pump_model','base_duty_cycle','distribution_type','comfort_level','cycle_length_off','cycle_length_on','comments'],
						    'voltdump' : ['name','groupid','runtime','file','runcount','mode','comments'],
						    'volt_var_control' : ['name','groupid','control_method','capacitor_delay','regulator_delay','desired_pf','pf_signed','d_max','d_min','substation_link','pf_phase','regulator_list','capacitor_list','voltage_measurements','minimum_voltages','maximum_voltages','desired_voltages','max_vdrop','high_load_deadband','low_load_deadband','comments'],
						    'billdump' : ['name','groupid','runtime','filename','runcount','meter_type','comments'],
						    'house_e' : ['name','groupid','parent','system_type','heating_system_type','cooling_system_type','auxiliary_system_type','auxiliary_strategy','system_mode','fan_type','thermal_integrity_level','glass_type','window_frame','glazing_treatment','glazing_layers','motor_model','motor_efficiency','weather','floor_area','gross_wall_area','ceiling_height','aspect_ratio','envelope_UA','window_wall_ratio','number_of_doors','exterior_wall_fraction','interior_exterior_wall_ratio','exterior_ceiling_fraction','exterior_floor_fraction','number_of_stories','Rroof','Rwall','Rfloor','Rwindows','Rdoors','window_shading','window_exterior_transmission_coefficient','cooling_design_temperature','heating_design_temperature','design_peak_solar','design_internal_gains','cooling_supply_air_temp','heating_supply_air_temp','duct_pressure_drop','heating_COP','cooling_COP','design_heating_capacity','design_cooling_capacity','design_heating_setpoint','design_cooling_setpoint','auxiliary_heat_capacity','over_sizing_factor','solar_heatgain_factor','airchange_per_hour','internal_gain','solar_gain','incident_solar_radiation','heat_cool_gain','air_heat_fraction','mass_heat_capacity','mass_heat_coeff','air_heat_capacity','total_thermal_mass_per_floor_area','interior_surface_heat_transfer_coeff','design_internal_gain_density','fan_design_power','fan_low_power_fraction','fan_power','fan_design_airflow','fan_impedance_fraction','fan_power_fraction','hvac_motor_efficiency','hvac_motor_loss_power_factor','heating_setpoint','cooling_setpoint','aux_heat_deadband','aux_heat_temperature_lockout','aux_heat_time_delay','thermostat_deadband','thermostat_cycle_time','thermostat_last_cycle_time','last_mode_timer','panel','hvac_breaker_rating','hvac_power_factor','hvac_load','total_load','comments'],
						    'zipload' : ['name','groupid','parent','heat_fraction','base_power','power_pf','current_pf','impedance_pf','is_240','breaker_val','actual_power','multiplier','heatgain_only','demand_response_mode','number_of_devices','termostatic_control_range','number_of_devices_off','number_of_devices_on','rate_of_cooling','rate_of_heating','temperature','phi','demand_rate','duty_cycle','recovery_duty_cycle','period','power_fraction','current_fraction','impedance_fraction','phase','comments'],
						    'waterheater' : ['name','groupid','parent','heat_mode','location','tank_volume','tank_UA','tank_diameter','water_demand','heating_element_capacity','inlet_water_temperature','tank_setpoint','thermostat_deadband','temperature','height','demand','actual_load','previous_load','actual_power','is_waterheater_on','gas_fan_power','gas_standby_power','comments'],
						    'player' : ['name','groupid','parent','property','file','loop','flags','comments'],
						    'recorder' : ['name','groupid','parent','property','file','trigger','interval','limit','comments'],
						    'multi_recorder' : ['name','groupid','parent','property','file','trigger','interval','limit','comments'],
						    'collector' : ['name','group','property','file','comments'],
						    'violation_recorder': ['file','summary','interval','strict','echo','violation_flag','xfrmr_thermal_limit_upper','xfrmr_thermal_limit_lower', 'line_thermal_limit_upper','line_thermal_limit_lower','node_instantaneous_voltage_limit_upper','node_instantaneous_voltage_limit_lower','node_continuous_voltage_limit_upper', 'node_continuous_voltage_limit_lower','node_continuous_voltage_interval','substation_breaker_A_limit','substation_breaker_B_limit','substation_breaker_C_limit','virtual_substation','inverter_v_chng_per_interval_upper_bound','inverter_v_chng_per_interval_lower_bound','inverter_v_chng_interval','secondary_dist_voltage_rise_upper_limit','secondary_dist_voltage_rise_lower_limit', 'substation_pf_lower_limit', 'violation_delay']}

		# Check to see if the object we want to create is a supported object
		if obj not in glm_objects:
			raise RuntimeError("Unsupported GridLAB-D object given: {:s}.".format(obj))

		# Make sure glm_parameters is the appropriate size
		if len(glm_props) != len(glm_objects[obj]):
			print glm_props
			print glm_objects[obj]
			print glm_object, len(glm_props), len(glm_objects[obj])
			
			raise RuntimeError("Incorrect number of parameters specified for the {:s} object.".format(obj))

		#find an unused key in the glm dictionary
		key = len(glm_dict)
		while key in glm_dict.keys():
			key += 1

		#Create object dictionary in glm dictionary    
		glm_dict[key] = {'object' : obj}
		for x in xrange(len(glm_props)):
			if glm_props[x] != None:
				if glm_objects[obj][x] == 'comments':
					glm_objects[obj][x] = '//'
				glm_dict[key][glm_objects[obj][x]] = glm_props[x]
	# End create_object()

	# Function that finds the proper rating for a transformer
	def find_SPCT_rating(load_str):
		spot_load = abs(complex(load_str))                           
		spct_rating = [5,10,15,25,30,37.5,50,75,87.5,100,112.5,125,137.5,150,162.5,175,187.5,200,225,250,262.5,300,337.5,400,412.5,450,500,750,1000,1250,1500,2000,2500,3000,4000,5000]
		past_rating = max(spct_rating)
		for rating in spct_rating:
			if rating >= spot_load and rating < past_rating:
				past_rating = rating

		return str(past_rating)


	# Begin to add the object dictionary to the glm dictionary

	# Check some cases
	if glm_object == 'load':
		if glm_parameters[2] == None: # All loads must have a parent node for our population scripts
			# Create node parameters list
			parent_node = [glm_parameters[0], None, None, glm_parameters[3], glm_parameters[4], glm_parameters[5]]
			
			# Create parent node dictionary
			create_object(parent_node, 'node')

			# Update load with parent info
			glm_parameters[2] = parent_node[0]
			glm_parameters[0] = 'ld_' + parent_node[0]

		if glm_parameters[6] in ['C','6','7','8']:
			# Create load dictionary
			create_object(glm_parameters, glm_object)
		else: # The load was classified as a residential load so we have to convert the load to a triplex_node for our population scripts
			# Create the transformer_configureation, transformer, tripelx_meter, and triplex_nodes parameters lists for every phase of the load 
			xfmrconfig_list = [None]*25
			xfmr_list = [None]*11
			tpm_list = [None]*7
			tpn_list = [None]*9

			# Transformer Configuration
			xfmrconfig_list[2] = 'SINGLE_PHASE_CENTER_TAPPED' # connect_type
			xfmrconfig_list[3] = 'POLETOP' # install_type
			xfmrconfig_list[6] = glm_parameters[5] # primary_voltage
			xfmrconfig_list[7] = '120' # secondary_voltage
			xfmrconfig_list[12] = '0.00033+0.0022j' # impedance

			# tpm Nominal voltage
			tpm_list[5] = '120'

			# tpn Nominal voltage
			tpn_list[5] = '120'

			# xfmr From
			xfmr_list[3] = glm_parameters[2]

			if 'A' in glm_parameters[4] and glm_parameters[7] != None:
				xfmrconfig_list[0] = 'SPCT_cfg_' + glm_parameters[0] + '_A' # name
				xfmrconfig_list[8] = find_SPCT_rating(glm_parameters[7]) # power_rating
				xfmrconfig_list[9] = xfmrconfig_list[8] # powerA_rating
				xfmrconfig_list[10] = None # powerB_rating
				xfmrconfig_list[11] = None # powerC_rating
				# Create A phase SPCT transformer configuration dictionary
				create_object(xfmrconfig_list, 'transformer_configuration')

				# tpm Name
				tpm_list[0] = 'tpm_' + glm_parameters[0] + '_A'
				# tpm Phases
				tpm_list[4] = 'AS'
				# Create A phase triplex meter dictionary
				create_object(tpm_list, 'triplex_meter')

				# tpn Name
				tpn_list[0] = glm_parameters[0] + '_A'
				# tpn load_class (original classID)
				tpn_list[8] = glm_parameters[1]
				# tpn groupid (original classID)
				tpn_list[1] = glm_parameters[1]
				# tpn Parent
				tpn_list[2] = tpm_list[0]
				# tpn Phases
				tpn_list[4] = 'AS'
				# tpn Power 12
				tpn_list[6] = glm_parameters[7]
				# Create A phase triplex node dictionary
				create_object(tpn_list, 'triplex_node')

				# xfmr Name
				xfmr_list[0] = 'SPCT_' + glm_parameters[0] + '_A'
				# xfmr Phases
				xfmr_list[2] = 'AS'
				# xfmr To
				xfmr_list[4] = tpm_list[0]
				# xfmr Configuration
				xfmr_list[5] = xfmrconfig_list[0]
				# Create transformer dictionary
				create_object(xfmr_list, 'transformer')

			if 'B' in glm_parameters[4] and glm_parameters[8] != None:
				# xfmrconfig Name
				xfmrconfig_list[0] = 'SPCT_cfg_' + glm_parameters[0] + '_B'
				# xfmrconfig Power rating
				xfmrconfig_list[8] = find_SPCT_rating(glm_parameters[8])
				# xfmrconfig Phase power rating
				xfmrconfig_list[9] = None
				xfmrconfig_list[10] = xfmrconfig_list[8]
				xfmrconfig_list[11] = None
				# Create B phase SPCT transformer configuration dictionary
				create_object(xfmrconfig_list, 'transformer_configuration')

				# tpm Name
				tpm_list[0] = 'tpm_' + glm_parameters[0] + '_B'
				# tpm Phases
				tpm_list[4] = 'BS'
				# Create A phase triplex meter dictionary
				create_object(tpm_list, 'triplex_meter')
				
				# tpn Name
				tpn_list[0] = glm_parameters[0] + '_B'
				# tpn load_class (original classID)
				tpn_list[8] = glm_parameters[1]
				# tpn groupid (original classID)
				tpn_list[1] = glm_parameters[1]
				# tpn Parent
				tpn_list[2] = tpm_list[0]
				# tpn Phases
				tpn_list[4] = 'BS'
				# tpn Power 12
				tpn_list[6] = glm_parameters[8]
				# Create A phase triplex node dictionary
				create_object(tpn_list, 'triplex_node')

				# xfmr Name
				xfmr_list[0] = 'SPCT_' + glm_parameters[0] + '_B'
				# xfmr Phases
				xfmr_list[2] = 'BS'
				# xfmr To
				xfmr_list[4] = tpm_list[0]
				# xfmr Configuration
				xfmr_list[5] = xfmrconfig_list[0]
				# Create transformer dictionary
				create_object(xfmr_list, 'transformer')

			if 'C' in glm_parameters[4] and glm_parameters[9] != None:
				# xfmrconfig Name
				xfmrconfig_list[0] = 'SPCT_cfg_' + glm_parameters[0] + '_C'
				# xfmrconfigPower rating
				xfmrconfig_list[8] = find_SPCT_rating(glm_parameters[9])
				# Phase power rating
				xfmrconfig_list[9] = None
				xfmrconfig_list[10] = None
				xfmrconfig_list[11] = xfmrconfig_list[8]
				# Create C phase SPCT transformer configuration dictionary
				create_object(xfmrconfig_list, 'transformer_configuration')

				# tpm Name
				tpm_list[0] = 'tpm_' + glm_parameters[0] + '_C'
				# tpm Phases
				tpm_list[4] = 'CS'
				# Create A phase triplex meter dictionary
				create_object(tpm_list, 'triplex_meter')

				# tpn Name
				tpn_list[0] = glm_parameters[0] + '_C'
				# tpn load_class (original classID)
				tpn_list[8] = glm_parameters[1]
				# tpn groupid (original classID)
				tpn_list[1] = glm_parameters[1]
				# tpn Parent
				tpn_list[2] = tpm_list[0]
				# tpn Phases
				tpn_list[4] = 'CS'
				# tpn Power 12
				tpn_list[6] = glm_parameters[9]
				# Create A phase triplex node dictionary
				create_object(tpn_list, 'triplex_node')

				# xfmr Name
				xfmr_list[0] = 'SPCT_' + glm_parameters[0] + '_C'
				# xfmr Phases
				xfmr_list[2] = 'CS'
				# xfmr To
				xfmr_list[4] = tpm_list[0]
				# xfmr Configuration
				xfmr_list[5] = xfmrconfig_list[0]
				# Create transformer dictionary
				create_object(xfmr_list, 'transformer')

	else:
		# Create the glm object dictionary
		create_object(glm_parameters, glm_object)

	return glm_dict

def main():
	pass
if __name__ == '__main__':
	main()