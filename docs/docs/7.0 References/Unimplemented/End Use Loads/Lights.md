# Lights

**TODO**:  This document needs to be completed. 

Residential lighting (explicit model) 

## Synopsis
    
    
    class lights {
    	parent residential_enduse;
    	class residential_enduse {
    		loadshape shape;
    		enduse load; // the enduse load description
    		complex energy[kVAh]; // the total energy consumed since the last meter reading
    		complex power[kVA]; // the total power consumption of the load
    		complex peak_demand[kVA]; // the peak power consumption since the last meter reading
    		double heatgain[Btu/h]; // the heat transferred from the enduse to the parent
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
