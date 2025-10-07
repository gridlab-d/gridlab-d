# House E

  In this section, we describe the residential and small commercial building
  thermal load calculation, the modeling approach, major assumption and model
  testing and validation.

The major heat gains and losses that contribute to the building cooling or heating load consist of the following:

1. Conduction through exterior walls, roof, and glass
2. Heat gain/loss from infiltration of outside air through opening
3. Solar radiation through glass
4. Internal gains from lighting, people, equipment

Items 1, 2, and 3 are considered driven by the external source, while item 4 is consider internally generated. The thermal load can be either sensible or latent. Sensible load results in increase/decrease in the air temperature; latent load results in increase/decrease of water vapor, which increases/decreases the humidity. Items 1 and 3 are solely sensible, items 2 can be both sensible and latent, internal gains from lights are sensible and internal gains from people and equipment can be both sensible and latent.

In general, the amount of heat that must be removed (cooling load) or added (heating load) is not always equal to the amount of heat received or lost at a given time. The difference is a result of the heat storage and time lag effects \[citation]. Only a portion of the heat entering or leaving building actually heats/cools the room air immediately; the rest heats the building mass – the roof, walls, floors and building mass and air mass. The heat that is stored in the mass will result in heating/cooling load at a later time. So, the modeling approach will have to account for the storage effect.

To account for the storage effects when estimating the conduction gains from the exterior walls, roofs, and glass cooling load temperature difference method (CLTD) can be used:


## Synopsis
    
    
    class house {
    	parent residential_enduse;
    	function attach_enduse();
    	object weather; 
    	double floor_area[sf];
    	double gross_wall_area[sf]; 
    	double ceiling_height[ft];
    	double aspect_ratio;
    	double envelope_UA[Btu/degF];
    	double window_wall_ratio;
    	double number_of_doors; 
    	double exterior_wall_fraction;
    	double interior_exterior_wall_ratio;
    	double exterior_ceiling_fraction;
    	double exterior_floor_fraction; 
    	double window_shading;
    	double window_exterior_transmission_coefficient;
    	double solar_heatgain_factor; 
    	double airchange_per_hour; 
    	double airchange_UA[Btu/degF]; 
            double UA;
    	double internal_gain[Btu/h]; 
    	double solar_gain[Btu/h]; 
    	double incident_solar_radiation[Btu/h]; 
    	double heat_cool_gain[Btu/h]; 
            set {NONE=0,H=1,N=2,E=3,S=4,W=5} include_solar_quadrant;
            enumeration {DEFAULT=0,FLAT=1,LINEAR=2,CURVED=3} heating_cop_curve;
    	double thermostat_deadband[degF]; 
    	int16 thermostat_cycle_time; 
    	timestamp thermostat_last_cycle_time; 
    	double heating_setpoint[degF]; 
    	double cooling_setpoint[degF]; 
    	double design_heating_setpoint[degF]; 
    	double design_cooling_setpoint[degF]; 
    	double over_sizing_factor; 
    	double design_heating_capacity[Btu/h]; 
    	double design_cooling_capacity[Btu/h]; 
    	double cooling_design_temperature[degF]; 
    	double heating_design_temperature[degF]; 
    	double design_peak_solar[Btu/h]; 
    	double design_internal_gains[Btu/h]; 
    	double air_heat_fraction[pu]; 
    	double mass_solar_gain_fraction[pu]; 
    	double mass_internal_gain_fraction[pu]; 
    	double auxiliary_heat_capacity[Btu/h]; 
    	double aux_heat_deadband[degF]; 
    	double aux_heat_temperature_lockout[degF]; 
    	double aux_heat_time_delay[s]; 
    	double cooling_supply_air_temp[degF]; 
    	double heating_supply_air_temp[degF]; 
    	double duct_pressure_drop[inh2o]; 
    	double fan_design_power[W]; 
    	double fan_low_power_fraction[pu]; 
    	double fan_power[kW]; 
    	double fan_design_airflow[cfm]; 
    	double fan_impedance_fraction[pu]; 
    	double fan_power_fraction[pu]; 
    	double fan_current_fraction[pu]; 
    	double fan_power_factor[pu]; 
    	double heating_demand; 
    	double cooling_demand; 
    	double heating_COP[pu]; 
    	double cooling_COP[Btu/kWh]; 
    	double air_temperature[degF]; 
    	double outdoor_temperature[degF]; 
    	double outdoor_rh[%]; 
    	double mass_heat_capacity[Btu/degF]; 
    	double mass_heat_coeff[Btu/degF]; 
    	double mass_temperature[degF]; 
    	double air_volume[cf]; 
    	double air_mass[lb]; 
    	double air_heat_capacity[Btu/degF]; 
    	double latent_load_fraction[pu]; 
    	double total_thermal_mass_per_floor_area[Btu/degF];
    	double interior_surface_heat_transfer_coeff[Btu/h];
    	double number_of_stories; 
    	double is_AUX_on; 
    	double is_HEAT_on; 
    	double is_COOL_on; 
    	double thermal_storage_present; 
    	double thermal_storage_in_use; 
    	set {RESISTIVE=16, TWOSTAGE=8, FORCEDAIR=4, AIRCONDITIONING=2, GAS=1} system_type; 
    	set {LOCKOUT=4, TIMER=2, DEADBAND=1, NONE=0} auxiliary_strategy; 
    	enumeration {AUX=3, COOL=4, OFF=1, HEAT=2, UNKNOWN=0} system_mode; 
    	enumeration {AUX=3, COOL=4, OFF=1, HEAT=2, UNKNOWN=0} last_system_mode; 
    	enumeration {RESISTANCE=4, HEAT_PUMP=3, GAS=2, NONE=1} heating_system_type;
    	enumeration {HEAT_PUMP=2, ELECTRIC=2, NONE=1} cooling_system_type;
    	enumeration {ELECTRIC=2, NONE=1} auxiliary_system_type;
    	enumeration {TWO_SPEED=3, ONE_SPEED=2, NONE=1} fan_type;
    	enumeration {UNKNOWN=7, VERY_GOOD=6, GOOD=5, ABOVE_NORMAL=4, NORMAL=3, BELOW_NORMAL=2, LITTLE=1, VERY_LITTLE=0} thermal_integrity_level; 
    	enumeration {LOW_E_GLASS=2, GLASS=1, OTHER=0} glass_type; 
    	enumeration {INSULATED=4, WOOD=3, THERMAL_BREAK=2, ALUMINIUM=1, ALUMINUM=1, NONE=0} window_frame; 
    	enumeration {HIGH_S=5, LOW_S=4, REFL=3, ABS=2, CLEAR=1, OTHER=0} glazing_treatment; 
    	enumeration {OTHER=4, THREE=3, TWO=2, ONE=1} glazing_layers; 
    	enumeration {FULL=2, BASIC=1, NONE=0} motor_model; 
    	enumeration {VERY_GOOD=4, |GOOD=3, AVERAGE=2, POOR=1, VERY_POOR=0} motor_efficiency; 
    	int64 last_mode_timer;
    	double hvac_motor_efficiency[unit]; 
    	double hvac_motor_loss_power_factor[unit]; 
    	double Rroof[Btu/degF.h]; 
    	double Rwall[Btu/degF.h]; 
    	double Rfloor[Btu/degF.h]; 
    	double Rwindows[Btu/degF.h]; 
    	double Rdoors[Btu/degF.h]; 
    	double hvac_breaker_rating[A]; 
    	double hvac_power_factor[unit]; 
    	double hvac_load[kW]; 
    	double last_heating_load; 
    	double last_cooling_load; 
    	complex hvac_power; 
    	double total_load; 
    	enduse panel; 
    	double design_internal_gain_density[W/sf]; 
    	bool compressor_on;
    	int64 compressor_count;
    	timestamp hvac_last_on;
    	timestamp hvac_last_off;
    	double hvac_period_length;
    	double hvac_duty_cycle;
    	enumeration {NONE=2, BAND=1, FULL=0} thermostat_control; 
    }
    

## Properties

### Physical Design

Property name | Type | Unit | Description   
---|---|---|---  
floor_area | double | sf | Home conditioned floor area   
gross_wall_area | double | sf | Gross outdoor wall area   
ceiling_height | double | ft | Average ceiling height   
aspect_ratio | double | none | Aspect ratio of the home's footprint   
window_wall_ratio | double | none | Ratio of window area to wall area   
number_of_doors | double | none | Ratio of door area to wall area   
exterior_wall_fraction | double | none | Ratio of exterior wall ratio to wall area   
interior_exterior_wall_ratio | double | none | Ratio of interior to exterior walls   
exterior_ceiling_fraction | double | none | Ratio of external ceiling sf to floor area   
exterior_floor_fraction | double | none | Ratio of floor area used in UA calculation   
number_of_stories | double | none | Number of stories within the structure   
Rroof | double | degF.sf.h/Btu | Roof R-value   
Rwall | double | degF.sf.h/Btu | Wall R-value   
Rfloor | double | degF.sf.h/Btu | Floor R-value   
Rwindows | double | degF.sf.h/Btu | Window R-value   
Rdoors | double | degF.sf.h/Btu | Door R-value   
window_shading | double | none | Transmission coefficient through window due to glazing   
window_exterior_transmission_coefficient | double | none | Coefficient for the amount of energy that passes through window   
  
### HVAC Design

Property name | Type | Unit | Description   
---|---|---|---  
cooling_design_temperature | double | degF | System cooling design temperature   
heating_design_temperature | double | degF | System heating design temperature   
design_peak_solar | double | Btu/h | System design solar load   
design_internal_gains | double | Btu/h | System design internal gains   
cooling_supply_air_temp | double | degF | Temperature of air blown out of the cooling system   
heating_supply_air_temp | double | degF | Temperature of air blown out of the heating system   
duct_pressure_drop | double | in | End-to-end pressure drop for the ventilation ducts (inches of water)   
heating_COP | double | pu | System heating performance coefficient   
cooling_COP | double | Btu/kWh | System cooling performance coefficient   
design_heating_capacity | double | Btu/h | System heating capacity   
design_cooling_capacity | double | Btu/h | System cooling capacity   
design_heating_setpoint | double | degF | System design heating setpoint   
design_cooling_setpoint | double | degF | System design cooling setpoint   
auxiliary_heat_capacity | double | Btu/h | Installed auxiliary heating capacity   
over_sizing_factor | double | unit | Over sizes the heating and cooling system from standard specifications (0.2 = 120% sizing)   
  
### Heatflow

Property name | Type | Unit | Description   
---|---|---|---  
solar_heatgain_factor | double | none | Product of the window area, window transmitivity, and the window exterior transmission coefficient   
airchange_per_hour | double | none | Number of air-changes per hour   
internal_gain | double | Btu/h | Internal heat gains   
solar_gain | double | Btu/h | Solar heat gains   
incident_solar_radiation | double | Btu/h.sf | Average incident solar radiation hitting the house   
heat_cool_gain | double | Btu/h | System heat gains(losses)   
air_heat_fraction | double | pu | Fraction of the heat gain/loss that goes to air (as opposed to mass)   
mass_heat_capacity | double | Btu/degF | Interior mass heat capacity   
mass_heat_coeff | double | Btu/degF.h | Interior mass heat exchange coefficient   
air_heat_capacity | double | Btu/degF | Air thermal mass   
total_thermal_mass_per_floor_area | double | Btu/degF.sf | Total thermal mass per floor area   
interior_surface_heat_transfer_coeff | double | Btu/h.degF.sf | Interior surface heat transfer coefficient   
design_internal_gain_density | double | W/sf | Average density of heat generating devices in the house   
  
### Fan Design

Property name | Type | Unit | Description   
---|---|---|---  
fan_design_power | double | W | Designed maximum pwer draw of the ventilation fan   
fan_low_power_fraction | double | pu | Fraction of ventilation fan power draw during low-power mode (two-speed only)   
fan_power | double | kW | Current ventilation fan power draw   
fan_design_airflow | double | cfm | Designed airflow for the ventilation system   
fan_impedance_fraction | double | pu | Impedance component of fan ZIP load   
fan_power_fraction | double | pu | Power component of fan ZIP load   
fan_current_fraction | double | pu | Current component of fan ZIP load   
fan_power_factor | double | pu | Power factor of the fan load   
hvac_motor_efficiency | double | unit | Percent efficiency of HVAC motor when using motor model   
hvac_motor_loss_power_factor | double | unit | Power factor of motor loasses when using motor model   
  
### Thermostat

Property name | Type | Unit | Description   
---|---|---|---  
heating_setpoint | double | degF | Thermostat heating setpoint   
cooling_setpoint | double | degF | Thermostat cooling setpoint   
aux_heat_deadband | double | degF | Temperature offset from standard heat activation to auxiliary heat activation   
aux_heat_temperature_lockout | double | degF | Temperature at which auxiliary heat will not engage above   
aux_heat_time_delay | double | s | Time required for heater to run until auxiliary heating engages   
thermostat_deadband | double | degF | Deadband of thermostat control   
thermostat_cycle_time | int16 | none | Mimimum time in seconds between thermostat updates   
thermostat_last_cycle_time | timestamp | none | Last time the thermostat changed state   
last_mode_timer | int64 | none |   
  
### Derived

Property name | Type | Unit | Description   
---|---|---|---  
air_temperature | double | degF | Indoor air temperature   
outdoor_temperature | double | degF | Outdoor air temperature   
mass_temperature | double | degF | Interior mass temperature   
air_volume | double | cf | Air volume   
air_mass | double | lb | Air mass   
latent_load_fraction | double | pu | Fractional increase in cooling load due to latent heat   
heating_demand | double | none | The current power draw to run the heating system   
cooling_demand | double | none | The current power draw to run the cooling system   
envelope_UA | double | Btu/degF.h | Overall UA of the home's envelope   
airchange_UA | double | Btu/degF.h | Additional UA due to air infiltration   
  
### Load

Property name | Type | Unit | Description   
---|---|---|---  
panel | enduse | none | Total panel enduse load   
hvac_breaker_rating | double | A | Determines the amount of curren the HVAC circuit breaker can handle   
hvac_power_factor | double | unit | Power factor of HVAC   
hvac_load | double | none | Heating/cooling system load   
total_load | double | none | Total load   
  
### Enumerations

Property name | Type | Unit | Description   
---|---|---|---  
system_type | set | none | Describe HVAC system of house. (GAS, AIRCONDITIONING, FORCEDAIR, TWOSTAGE, RESISTIVE)   
heating_system_type | enumeration | none | Set heating mechanism for house (RESISTANCE, HEAT_PUMP, GAS, NONE)   
cooling_system_type | enumeration | none | Set cooling mechanism for hosue (HEAT_PUMP, ELECTRIC, NONE)   
auxiliary_system_type | enumeration | none | Can be specified for HEAT_PUMP heating systems (ELECTRIC, NONE)   
auxiliary_strategy | set | none | Control strategy for auxiliary heat (LOCKOUT, TIMER, DEADBAND, NONE)   
system_mode | enumeration | none | Heating/cooling system operation state (UNKNOWN, HEAT, OFF, COOL, AUX)   
fan_type | enumeration | none | Circulation fan (TWO_SPEED, ONE_SPEED, NONE)   
thermal_integrity_level | enumeration | none | Default envelope UA settings (VERY_GOOD, GOOD, ABOVE_NORMAL, NORMAL, BELOW_NORMAL, LITTLE, VERY_LITTLE, UNKNOWN)   
glass_type | enumeration | none | Type of window glass used (LOW_E_GLASS, GLASS, OTHER)   
window_frame | enumeration | none | Type of window frame (INSULATED, WOOD, THERMAL_BREAK, ALUMINUM, NONE)   
glazing_treatment | enumeration | none | Treatment that increases the reflectivity of exterior windows (HIGH_S, LOW_S, REFL, ABS, CLEAR, OTHER)   
glazing_layers | enumeration | none | Number of layers of glass in each window (THREE, TWO, ONE, OTHER)   
motor_model | enumeration | none | Indicates the level of detail used in modeling the HVAC motor parameters (FULL, BASIC, NONE)   
motor_efficiency | enumeration | none | Describes efficiency of the motor when using a motor model (VERY_GOOD, GOOD, AVERAGE, POOR, VERY_POOR)   
  
## Default House

The default house does not require any parameters be set. Thus, the minimum allowed specification for a single family house is 
    
    
    house {
    }
    

New houses are created with the following default values (meaning if the value is not set in the GLM file, it will be calculated automatically). 

Default parameter values  Parameter | Default value   
---|---  
load.power_fraction | 0.8   
load.impedance_fraction | 0.2   
load.current_fraction | 0.0   
design_internal_gain_density | 0.6 [W/sf]  
thermal_integrity_level | UNKNOWN  
hvac_breaker_rating | 0.0 [A]  
hvac_power_factor | 0.0   
Tmaterials | 0.0 [degF]  
cooling_supply_air_temp | 50.0 [degF]  
heating_supply_air_temp | 150.0 [degF]  
heating_system_type | HEAT_PUMP  
cooling_system_type | UNKNOWN  
auxiliary_system_type | UNKNOWN  
fan_type | UNKNOWN  
fan_power_factor | 0.96   
fan_current_fraction | 0.7332   
fan_impedance_fraction | 0.2534   
fan_power_fraction | 0.0135   
glazing_layers | TWO  
glass_type | LOW_E_GLASS  
glazing_treatment | CLEAR  
window_frame | THERMAL_BREAK  
motor_model | NONE  
motor_efficiency | AVERAGE  
hvac_motor_efficiency | 1.0   
hvac_motor_loss_power_factor | 0.125   
hvac_motor_real_loss | 0.0   
hvac_motor_reactive_loss | 0.0   
is_AUX_on | FALSE  
is_HEAT_on | FALSE  
is_COOL_on | FALSE  
thermal_storage_present | FALSE  
thermal_storage_inuse | FALSE  
  
Certain parameters are calculated by default as follows 

Thermal integrity levels  thermal_integrity_level | Rroof | Rwall | Rfloor | Rdoors | Rwindows | airchange_per_hour  
---|---|---|---|---|---|---  
VERY_LITTLE | 11.0 | 4.0 | 4.0 | 3.0 | 1/1.27 | 1.5   
LITTLE | 19.0 | 11.0 | 4.0 | 3.0 | 1/0.81 | 1.5   
BELOW_NORMAL | 19.0 | 11.0 | 11.0 | 3.0 | 1/0.81 | 1.0   
NORMAL | 30.0 | 11.0 | 19.0 | 3.0 | 1/0.6 | 1.0   
ABOVE_NORMAL | 30.0 | 19.0 | 11.0 | 3.0 | 1/0.6 | 1.0   
GOOD | 30.0 | 19.0 | 22.0 | 5.0 | 1/0.47 | 0.5   
VERY_GOOD | 48.0 | 19.0 | 22.0 | 5.0 | 1/0.47 | 0.5   
UNKNOWN | – | – | – | – | – | –   
  
  


Glazing solar heat gain coefficient (glazing_shgc) by window frame type (window_frame)  glazing_treatment | CLEAR | ABS | REFL | LOW_S | HIGH_S  
---|---|---|---|---|---  
glazing_layers | ONE | TWO | THREE | ONE | TWO | THREE | ONE | TWO | THREE | TWO | THREE | TWO | THREE   
window_frame | NONE | 0.86 | 0.76 | 0.68 | 0.73 | 0.62 | 0.34 | 0.31 | 0.29 | 0.34 | 0.41 | 0.27 | 0.70 | 0.62   
ALUMINUM | 0.75 | 0.67 | 0.60 | 0.64 | 0.55 | 0.31 | 0.28 | 0.27 | 0.31 | 0.37 | 0.25 | 0.62 | 0.55   
THERMAL_BREAK | 0.75 | 0.67 | 0.60 | 0.64 | 0.55 | 0.31 | 0.28 | 0.27 | 0.31 | 0.37 | 0.25 | 0.62 | 0.55   
WOOD | 0.64 | 0.57 | 0.51 | 0.54 | 0.46 | 0.26 | 0.24 | 0.22 | 0.26 | 0.31 | 0.21 | 0.52 | 0.46   
INSULATED | 0.64 | 0.57 | 0.51 | 0.54 | 0.46 | 0.26 | 0.24 | 0.22 | 0.26 | 0.31 | 0.21 | 0.52 | 0.46   
  
  


Values for Rwindows.  glass_type | LOW_E_GLASS | GLASS | OTHER  
---|---|---|---  
glazing_layers | ONE | TWO | THREE | ONE | TWO | THREE | ONE | TWO | THREE   
window_frame | NONE | undef | 1/0.30 | 1/0.27 | 1/1.04 | 1/0.48 | 1/0.31 | 2.0   
ALUMINUM | undef | 1/0.67 | 1/0.64 | 1/1.27 | 1/0.81 | 1/0.67   
THERMAL_BREAK | undef | 1/0.47 | 1/0.43 | 1/1.08 | 1/0.60 | 1/0.46   
WOOD | undef | 1/0.41 | 1/0.37 | 1/0.90 | 1/0.53 | 1/0.40   
INSULATED | undef | 1/0.33 | 1/0.31 | 1/0.81 | 1/0.44 | 1/0.34   
  
  


Automatically calculated defaults  Parameter | Default value   
---|---  
panel.max_amps | 200 [A]  
fan_type | heating_system_type==HEAT_PUMP ? ONE_SPEED : NONE  
**TODO**:  | Add rest of auto inits from house_e::init()   
  
## Example
    
    
    module residential;
    object house {
    }

# Implicit enduses

Enable implicit enduses in the house model. 
  
    
    module [residential] {
       implicit_enduses LIGHTS|PLUGS|OCCUPANCY|DISHWASHER|MICROWAVE|FREEZER|REFRIGERATOR|RANGE|EVCHARGER|WATERHEATER|CLOTHESWASHER|DRYER;
    }

## See Also

  * Residential module
    * User's Guide
    * Appliances
    * house class – Single-family home model.
    * residential_enduse class – Abstract residential end-use class.
    * occupantload – Residential occupants (sensible and latent heat).
    * ZIPload – Generic constant impedance/current/power end-use load.
  * Technical Documents 
    * Requirements
    * Specifications
    * Developer notes
    * Technical support document
    * Validation
