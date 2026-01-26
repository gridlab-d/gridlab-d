# Battery Energy storage

GridLAB-D™ contains both a generic energy storage object as well as a battery energy storage object. Both are described here.

## Synopsis

A battery energy storage device is defined using the **battery** object.

		module generators;
	class battery {
		enumeration {UNKNOWN, CONSTANT_V, CONSTANT_PQ, CONSTANT_PF, SUPPLY_DRIVEN, POWER_DRIVEN, VOLTAGE_CONTROLLED, POWER_VOLTAGE_HYBRID} generator_mode ;
		enumeration {NONE=0, LINEAR_TEMPERATURE=1} additional_controls;
		enumeration {OFFLINE=1, ONLINE=2} generator_status;
		enumeration {HOUSEHOLD, SMALL=1, MED_COMMERCIAL, MED_HIGH_ENERGY, LARGE} rfb_size;
		enumeration {AC=2, DC=1} power_type;
		enumeration {CHARGING=1, DISCHARGING=2, WAITING=0, FULL=3, EMPTY=4, CONFLICTED=5} battery_state;
		double number_battery_state_changes;
		double monitored_power[W];
		double power_set_high[W];
		double power_set_low[W];
		double power_set_high_highT[W];
		double power_set_low_highT[W];
		double check_power_low[W];
		double check_power_high[W];
		double voltage_set_high[V];
		double voltage_set_low[V];
		double deadband[V];
		double sensitivity;
		double high_temperature;
		double midpoint_temperature;
		double low_temperature;
		double scheduled_power[W];
		double Rinternal[Ohm];
		double V_Max[V];
		double I_Max[A];
		double E_Max[Wh];
		double P_Max[W];
		double power_factor;
		double Energy[Wh];
		double efficiency[unit];
		double base_efficiency[unit];
		double parasitic_power_draw[W];
		double Rated_kVA[kVA];
		complex V_Out[V];
		complex I_Out[A];
		complex VA_Out[VA];
		complex V_In[V];
		complex I_In[I];
		complex V_Internal[V];
		complex I_Internal[A];
		complex I_Prev[A];
		double power_transfered;
		set {A,B,C,N,S} phases;
		bool use_internal_battery_model; 
		enumeration {UNKNOWN, LI_ION, LEAD_ACID} battery_type; 
		double nominal_voltage[V];
		double rated_power[W];
		double battery_capacity[Wh];
		double round_trip_efficiency[pu];
		double state_of_charge[pu];
		double battery_load[W];
		double reserve_state_of_charge[pu];
	}


A generic, technology agnostic energy storage device is defined with the **energy_storage** object.     
    
    module generators;
    class energy_storage {
    	enumeration {SUPPLY_DRIVEN=4, CONSTANT_PF=3, CONSTANT_PQ=2, CONSTANT_V=1, UNKNOWN=0} generator_mode;
    	enumeration {ONLINE=2, OFFLINE=1} generator_status;
    	enumeration {DC=0, AC=1} power_type;
    	double Rinternal;
    	double V_Max[V];
    	complex I_Max[A];
    	double E_Max;
    	double Energy;
    	double efficiency;
    	double Rated_kVA[kVA];
    	complex V_Out[V];
    	complex I_Out[A];
    	complex VA_Out[VA];
    	complex V_In[V];
    	complex I_In[A];
    	complex V_Internal[V];
    	complex I_Internal[A];
    	complex I_Prev[A];
    	set {S=112, N=8, C=4, B=2, A=1} phases;
    }
    

## Properties

#### Property Table

Property name |	Type |	Unit |	Description
-- | -- | -- | --
**generator_mode**	| enumeration | 	none | 	(UNKNOWN, CONSTANT_V, CONSTANT_PQ, CONSTANT_PF, SUPPLY_DRIVEN, POWER_DRIVEN, VOLTAGE_CONTROLLED, POWER_VOLTAGE_HYBRID) All modes are implemented except for VOLTAGE_CONTROLLED.
**generator_status** |	enumeration |	none |	(ONLINE, OFFLINE)
**additional_controls**	 | enumeration |	none |	(NONE, LINEAR_TEMPERATURE) Allows for additional controls to account for the climate region battery is in.
**rfb_size** |	enumeration	| none |	(HOUSEHOLD, SMALL,MED_COMMERCIAL, MED_HIGH_ENERGY, LARGE) Selection sets different defaults for maximum voltage, power, energy capacity, current, and the battery efficiency.
**power_type** |	enumeration |	none |	(AC, DC) Not currently used
**battery_state** |	enumeration |	none |	(CHARGING, DISCHARGING, WAITING, FULL, EMPTY, CONFLICTED)
**number_battery_state_changes** |	double | |		
**monitored_power** |	double |	W	
**power_set_high** |	double |	W	
**power_set_low** |	double	| W	
**power_set_high_highT** |	double |	W	
**power_set_low_highT** |	double |	W	
**check_power_low** |	double	| W	
**check_power_high** |	double	| W	
**voltage_set_high** |	double	| V	
**voltage_set_low** |	double	| V	
**deadband** |	double	| V	
**sensitivity** |	double	|	
**high_temperature** |	double	| degF	
**midpoint_temperature** |	double	| degF	
**low_temperature**	| double |	 degF	
**scheduled_power** |	double	| W	
**Rinternal** |	double	| Ohm	
**V_Max** |	double	| V	
**I_Max** |	complex	| A	
**E_Max** |	double	| Wh	
**P_Max** |	double	| W	
**power_factor** |	double |	pu	
**Energy** |	double	| Wh	
**efficiency** |	double	| unit	
**base_efficency** |	double	| unit	
**parasitic_power_draw** |	double	| W	
**rated_kVA** |	double	kVA	
**V_Out**	| complex	| V	
**I_Out**	| complex	| A	
**VA_Out**	| complex	| VA	
**V_In**	| complex	| V	
**V_Internal** |	complex |	V	
**I_Internal**	| complex	| A	
**I_Prev**	| complex	| A	
**power_transfered**	| double		
**phases**	| set	| none	| (A, B, C, N, S)


#### Table 2. Properties only used for internal battery module

Property Name	| Type	| Unit | 	Description
-- | -- | -- | --
**use_internal_battery_model** | bool | none | 	Boolean flag to use internal battery model. Default is FALSE.
**nominal_voltage**	| double	| V	| Battery's nominal voltage
**rated_power**	| double	| W	| Maximum rated power of battery. Can be set in inverter in rated_battery_power.
**battery_capacity**	| double| 	Wh | 	Maximum energy capacity of battery
**round_trip_efficiency**	| double | 	pu | 	Round-trip efficiency of battery
**state_of_charge**	| double | 	pu | 	Battery's current state of charge (SOC)
**battery_load**	| double | 	W | 	Battery's current load. This is the value of power_in from the parent inverter.
**reserve_state_of_charge**	| double | 	pu	| The state of charge which the battery may not go below

## Remarks

**TODO**: 

## Example

**TODO**: 

# Related Concepts:

* Modules
	* Generators
		* inverter
		* Centralized DG Controller