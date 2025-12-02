# Microwave

!!! warning

	This page contains features that are unfinished, were never implemented, or have since been deprecated. We preserve these pages for archival purposes, and also as a foundational resource for prospective developers who may wish to implement the same or similar feature. Many of these pages provide robust explanations of the theory behind a particular module or feature that we hope readers will find useful. 
	
	**This page does not reflect the current state of GridLAB-D™**

## Synopsis
    
    
    object microwave {
    	// residential_enduse properties
    	shape "type: unknown";
    	load "power_factor: 0.000000; power.r: 0.000000";
    	energy +0+0i kVAh;
    	power +0+0j kVA;
    	peak_demand +0+0i kVA;
    	heatgain +0 Btu/h;
    	cumulative_heatgain +0 Btu;
    	heatgain_fraction +0 pu;
    	current_fraction +0 pu;
    	impedance_fraction +0 pu;
    	power_fraction +0 pu;
    	power_factor +0;
    	constant_power +0+0j kVA;
    	constant_current +0+0i kVA;
    	constant_admittance +0+0i kVA;
    	voltage_factor +1 pu;
    	breaker_amps +0 A;
    	configuration IS110;
    	override NORMAL;
    	power_state OFF;
    	// microwave properties
    	installed_power +0 kW;
    	standby_power +0 kW;
    	circuit_split +0;
    	state OFF;
    	cycle_length +0 s;
    	runtime +0 s;
    	state_time +0 s;
    }
    

## Related Concepts:

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

