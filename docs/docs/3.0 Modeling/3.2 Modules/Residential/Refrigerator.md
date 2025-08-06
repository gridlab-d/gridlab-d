# Refrigerator

**TODO**:  This page needs to be completed. 

Refrigerator \- Residential Refrigerator (explicit model) 

## Synopsis
    
    
    object refrigerator {
    	// residential_enduse properties
    	shape "type: unknown";
    	load "power_factor: 0.950000; power.r: 0.000000";
    	energy +0+0i kVAh;
    	power +0+0j kVA;
    	peak_demand +0+0i kVA;
    	heatgain +0 Btu/h;
    	cumulative_heatgain +0 Btu;
    	heatgain_fraction +0 pu;
    	current_fraction +0 pu;
    	impedance_fraction +0 pu;
    	power_fraction +0 pu;
    	power_factor +0.95;
    	constant_power +0+0i kVA;
    	constant_current +0+0i kVA;
    	constant_admittance +0+0i kVA;
    	voltage_factor +0 pu;
    	breaker_amps +0 A;
    	configuration IS110;
    	override NORMAL;
    	power_state OFF;
    	// refrigerator properties
    	size +23.9374 cf;
    	rated_capacity +816.743 Btu/h;
    	temperature +38.1227 degF;
    	setpoint +38.3484 degF;
    	deadband +2.4032 degF;
    	cycle_time +0 s;
    	output +0;
    	event_temp +0;
    	UA +0.6 Btu/degF*h;
    	compressor_off_normal_energy +40500;
    	compressor_off_normal_power +15 W;
    	compressor_on_normal_energy +252000;
    	compressor_on_normal_power +120 W;
    	defrost_energy +1.32e+06;
    	defrost_power +550 W;
    	icemaking_energy +18000;
    	icemaking_power +300 W;
    	ice_making_probability +0.02;
    	FF_Door_Openings 0;
    	door_opening_energy 0;
    	door_opening_power 0;
    	DO_Thershold +1.18576e-322;
    	dr_mode_double +0;
    	energy_needed +0;
    	energy_used +0;
    	refrigerator_power +0;
    	icemaker_running FALSE;
    	check_DO 0;
    	is_240 FALSE;
    	defrostDelayed +0;
    	long_compressor_cycle_due FALSE;
    	long_compressor_cycle_time +0;
    	long_compressor_cycle_power +120;
    	long_compressor_cycle_energy +720000;
    	long_compressor_cycle_threshold +0.05;
    	defrost_criterion TIMED;
    	run_defrost FALSE;
    	door_opening_criterion +0;
    	compressor_defrost_time +0;
    	delay_defrost_time +28800;
    	daily_door_opening 0;
    	state 0;
    }
    

## See also

  * [Residential module]
    * [User's Guide]
    * [Appliances]
    * [house] class – Single-family home model.
    * residential_enduse class – Abstract residential end-use class.
    * [occupantload] – Residential occupants (sensible and latent heat).
    * [ZIPload] – Generic constant impedance/current/power end-use load.
  * Technical Documents 
    * [Requirements]
    * [Specifications]
    * [Developer notes]
    * [Technical support document]
    * [Validation]

