# Lights

!!! warning

	This page contains features that are unfinished, were never implemented, or have since been deprecated. We preserve these pages for archival purposes, and also as a foundational resource for prospective developers who may wish to implement the same or similar feature. Many of these pages provide robust explanations of the theory behind a particular module or feature that we hope readers will find useful. 
	
	**This page does not reflect the current state of GridLAB-D™**

Residential lighting (explicit model) 

## Synopsis
    
    
    class lights {
    	parent residential_enduse;
    	class residential_enduse {
    		loadshape shape;
    		end use load; // the end use load description
    		complex energy[kVAh]; // the total energy consumed since the last meter reading
    		complex power[kVA]; // the total power consumption of the load
    		complex peak_demand[kVA]; // the peak power consumption since the last meter reading
    		double heatgain[Btu/h]; // the heat transferred from the end use to the parent
    		double heatgain_fraction[pu]; // the fraction of the heat that goes to the parent
    		double current_fraction[pu]; // the fraction of total power that is constant current
    		double impedance_fraction[pu]; // the fraction of total power that is constant impedance
    		double power_fraction[pu]; // the fraction of the total power that is constant power
    		double power_factor; // the power factor of the load
    		complex constant_power[kVA]; // the constant power portion of the total load
    		complex constant_current[kVA]; // the constant current portion of the total load
    		complex constant_admittance[kVA]; // the constant admittance portion of the total load
    		double voltage_factor[pu]; // the voltage change factor
    		double breaker_amps[A]; // the rated breaker amperage
    		set {IS220=1} configuration; // the load configuration options
    		enumeration {OFF=4294967295, NORMAL=0, ON=1} override;
    		enumeration {ON=1, OFF=0, UNKNOWN=4294967295} power_state;
    	}
    
    	enumeration {HID=4, SSL=3, CFL=2, FLUORESCENT=1, INCANDESCENT=0} type; // lighting type (affects power_factor)
    	enumeration {OUTDOOR=1, INDOOR=0} placement; // lighting location (affects where heatgains go)
    	double installed_power[kW]; // installed lighting capacity
    	double power_density[W/sf]; // installed power density
    	double curtailment[pu]; // lighting curtailment factor
    	double demand[pu]; // the current lighting demand
    	complex actual_power[kVA]; // actual power demand of lights object
    }
    

## Related Concepts:

  * Residential module
    * User's Guide
    * Appliances
    * house class – Single-family home model.
    * residential_enduse class – Abstract residential end use class.
    * occupantload – Residential occupants (sensible and latent heat).
    * ZIPload – Generic constant impedance/current/power end use load.
  * Technical Documents 
    * Requirements
    * Specifications
    * Developer notes
    * Technical support document
    * Validation
