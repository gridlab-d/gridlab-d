# Range
**TODO**:  This page needs to be completed. 

Residential Range (explicit model) 

## Synopsis
    
    
    class range {
    	parent residential_enduse;
    	class residential_enduse {
    		loadshape shape;
    		enduse load; // the enduse load description
    		complex energy[kVAh]; // the total energy consumed since the last meter reading
    		complex power[kVA]; // the total power consumption of the load
    		complex peak_demand[kVA]; // the peak power consumption since the last meter reading
    		double heatgain[Btu/h]; // the heat transferred from the enduse to the parent
    		double cumulative_heatgain[Btu]; // the cumulative heatgain from the enduse to the parent
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
    		set {IS220=1, IS110=0} configuration; // the load configuration options
    		enumeration {OFF=2, ON=1, NORMAL=0} override;
    		enumeration {UNKNOWN=2, ON=1, OFF=0} power_state;
    	}
     	double oven_volume[gal]; // the volume of the oven
    	double oven_UA[Btu/degF*h]; // the UA of the oven (surface area divided by R-value)
    	double oven_diameter[ft]; // the diameter of the oven
    	double oven_demand[gpm]; // the hot food take out from the oven
    	double heating_element_capacity[kW]; // the power of the heating element
    	double inlet_food_temperature[degF]; // the inlet temperature of the food
    	enumeration {GASHEAT=1, ELECTRIC=0} heat_mode; // the energy source for heating the oven
    	enumeration {GARAGE=1, INSIDE=0} location; // whether the range is inside or outside
    	double oven_setpoint[degF]; // the temperature around which the oven will heat its contents
    	double thermostat_deadband[degF]; // the degree to heat the food in the oven, when needed
    	double temperature[degF]; // the outlet temperature of the oven
    	double height[ft]; // the height of the oven
    	double food_density; // food density
    	double specificheat_food;
    	double queue_cooktop[unit]; // number of loads accumulated
    	double queue_oven[unit]; // number of loads accumulated
    	double queue_min[unit];
    	double queue_max[unit];
    	double time_cooktop_operation;
    	double time_cooktop_setting;
    	double cooktop_run_prob;
    	double oven_run_prob;
    	double cooktop_coil_setting_1[kW];
    	double cooktop_coil_setting_2[kW];
    	double cooktop_coil_setting_3[kW];
    	double total_power_oven[kW];
    	double total_power_cooktop[kW];
    	double total_power_range[kW];
    	double demand_cooktop[unit/day]; // number of loads accumulating daily
    	double demand_oven[unit/day]; // number of loads accumulating daily
    	double stall_voltage[V];
    	double start_voltage[V];
    	complex stall_impedance[Ohm];
    	double trip_delay[s];
    	double reset_delay[s];
    	double time_oven_operation[s];
    	double time_oven_setting[s];
    	enumeration {CT_TRIPPED=6, CT_STALLED=5, STAGE_8_ONLY=4, STAGE_7_ONLY=3, STAGE_6_ONLY=2, CT_STOPPED=1} state_cooktop;
    	double cooktop_energy_baseline[kWh];
    	double cooktop_energy_used;
    	double Toff;
    	double Ton;
    	double cooktop_interval_setting_1[s];
    	double cooktop_interval_setting_2[s];
    	double cooktop_interval_setting_3[s];
    	double cooktop_energy_needed[kWh];
    	bool heat_needed;
    	bool oven_check;
    	bool remainon;
    	bool cooktop_check;
    	double actual_load[kW]; // the actual load based on the current voltage across the coils
    	double previous_load[kW]; // the actual load based on current voltage stored for use in controllers
    	complex actual_power[kVA]; // the actual power based on the current voltage across the coils
    	double is_range_on; // simple logic output to determine state of range (1-on, 0-off)
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

