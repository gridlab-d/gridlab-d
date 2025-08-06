# Office

Single-zone [commercial] office building with rooftop package unit 

## Synopsis
    
    
    class office {
    	double [floor_area][[sf]];
    	double [floor_height][[ft]];
    	double [exterior_ua][[Btu/degF/h]];
    	double [interior_ua][[Btu/degF/h]];
    	double [interior_mass][[Btu/degF]];
    	double [glazing][[sf]];
    	double [glazing.north][[sf]];
    	double [glazing.northeast][[sf]];
    	double [glazing.east][[sf]];
    	double [glazing.southeast][[sf]];
    	double [glazing.south][[sf]];
    	double [glazing.southwest][[sf]];
    	double [glazing.west][[sf]];
    	double [glazing.northwest][[sf]];
    	double [glazing.horizontal][[sf]];
    	double [glazing.coefficient][[pu]];
    	double [occupancy];
    	double [occupants];
    	[char256] [schedule];
    	double [air_temperature][[degF]];
    	double [mass_temperature][[degF]];
    	double [temperature_change][[degF/h]];
    	double [outdoor_temperature][[degF]];
    	double [Qh][[Btu/h]];
    	double [Qs][[Btu/h]];
    	double [Qi][[Btu/h]];
    	double [Qz][[Btu/h]];
    	enumeration {[OFF]=0, [VENT]=5, [ECON]=4, [COOL]=3, [AUX]=2, [HEAT]=1} [hvac_mode];
    	double [hvac.cooling.balance_temperature][[degF]];
    	double [hvac.cooling.capacity][[Btu/h]];
    	double [hvac.cooling.capacity_perF][[Btu/degF/h]];
    	double [hvac.cooling.design_temperature][[degF]];
    	double [hvac.cooling.efficiency][[pu]];
    	double [hvac.cooling.cop][[pu]];
    	double [hvac.heating.balance_temperature][[degF]];
    	double [hvac.heating.capacity][[Btu/h]];
    	double [hvac.heating.capacity_perF][[Btu/degF/h]];
    	double [hvac.heating.design_temperature][[degF]];
    	double [hvac.heating.efficiency][[pu]];
    	double [hvac.heating.cop][[pu]];
    	double [lights.capacity][[kW]];
    	double [lights.fraction][[pu]];
    	double [plugs.capacity][[kW]];
    	double [plugs.fraction][[pu]];
    	complex [demand][[kW]];
    	complex [total_load][[kW]];
    	complex [energy][kWh];
    	double [power_factor];
    	complex [power][[kW]];
    	complex [current][[A]];
    	complex [admittance][[1/Ohm]];
    	complex [hvac.demand][[kW]];
    	complex [hvac.load][[kW]];
    	complex [hvac.energy][kWh];
    	double [hvac.power_factor];
    	complex [lights.demand][[kW]];
    	complex [lights.load][[kW]];
    	complex [lights.energy][kWh];
    	double [lights.power_factor];
    	double [lights.heatgain_fraction];
    	double [lights.heatgain][[[Units|kW];
    	complex [plugs.demand][[[Units|kW];
    	complex [plugs.load][[[Units|kW];
    	complex [plugs.energy][[[Units|kWh];
    	double [plugs.power_factor];
    	double [plugs.heatgain_fraction];
    	double [plugs.heatgain][[kW]];
    	double [cooling_setpoint][[degF]];
    	double [heating_setpoint][[degF]];
    	double [thermostat_deadband][[degF]];
    	double [control.ventilation_fraction];
    	double [control.lighting_fraction];
    	double [ACH];
    }
    

## Properties

Property name | Type | Unit | Description   
---|---|---|---  
[floor_area] | double | [ft^2] | Floor area of the office (presuming one floor).   
floor_height | double | ft | Ceiling height within the office interior   
exterior_ua | double | BTU/degF/hr | Exterior thermal resistance   
interior_ua | double | BTU/degF/hr | Interior thermal resistance   
interior_mass | double | BTU/degF | The thermal mass of the interior finishing and building materials   
glazing | double | ft^2 | The external glazing area (total area)   
glazing.north | double | ft^2 | The external glazing area facing north   
glazing.northeast | double | ft^2 | The external glazing area facing north-east   
glazing.east | double | ft^2 | The external glazing area facing east   
glazing.southeast | double | ft^2 | The external glazing area facing south-east   
glazing.south | double | ft^2 | The external glazing area facing south   
glazing.southwest | double | ft^2 | The external glazing area facing south-west   
glazing.west | double | ft^2 | The external glazing area facing west   
glazing.northwest | double | ft^2 | The external glazing area facing north-west   
glazing.horizontal | double | ft^2 | The external glazing area facing up (skyward)   
glazing.coefficient | double | per unit | The fraction of solar radiation that is transmitted by the glazing.   
occupancy | double | - | Current occupancy ratio   
occupants | double | people | Total occupants for the office   
schedule | char256 | - | Schedule string. Semicolon delimited cron-style schedule definition.   
air_temperature | double | degF | Office interior air temperature   
mass_temperature | double | degF | Office interior mass temperature   
temperature_change | double | degF/hr | The rate of change of temperature since the last update   
Qh | double | BTU/hr | The HVAC gain/loss since the last update   
Qs | double | BTU/hr | The solar heat gain since the last update   
Qi | double | BTU/hr | The internal heat gains since the last update   
Qz | double | BTU/hr | The inter-zonal heat gain/loss since the last update   
hvac_mode | enumeration | - | Current mode of the HVAC system. HEAT, AUX, COOL, ECON, VENT, or OFF.   
hvac.cooling.balance_temperature | double | degF | The balance temperature of the HVAC cooler   
hvac.cooling.capacity | double | BTU/hr | The constant heat output capacity of the HVAC cooler (should be negative)   
hvac.cooling.capacity_perF | double | BTU/degF/hr | The temperature-dependent heat output capacity of the HVAC cooler (should be negative)   
hvac.cooling.design_temperature | double | degF | The design temperature of the HVAC cooler   
hvac.cooling.efficiency | double | - | The cooling efficiency of the HVAC cooler   
hvac.cooling.cop | double | - | Coefficient of performance of the HVAC cooler   
hvac.heating.balance_temperature | double | degF | The balance temperature of the HVAC heater   
hvac.heating.capacity | double | BTU/hr | The constant heat output capacity of the HVAC heater   
hvac.heating.capacity_perF | double | BTU/degF/hr | The temperature-dependent heat output capacity of the HVAC heater   
hvac.heating.design_temperature | double | degF | The design temperature of the HVAC heater   
hvac.heating.efficiency | double | BTU/W | The heating efficiency of the HVAC heater   
hvac.heating.cop | double | - | Coefficient of performance of the HVAC heater   
lights.capacity | double | kW | Total power of the lights installed in the office   
lights.fraction | double | pu | The fraction of the installed lights that are turned on   
plugs.capacity | double | kW | Total power of the devices plugged into wall sockets   
plugs.fraction | double | pu | The current fraction of the total power draw from the various devices   
demand | complex | kW | Aggregate peak power draw from the office   
total_load | complex | kW | Current aggregate power draw from the office   
energy | complex | kWh | Accumulated energy consumed by the office   
power_factor | complex | - | The power factor of the house load   
power | complex | kW | Constant power component of the office load   
current | complex | A | Constant current component of the office load   
admittance | complex | 1/Ohm | Constant resistance component of the office load   
hvac.demand[kW] | complex | kW | The HVAC load   
hvac.load[kW] | complex | kW | The HVAC load   
hvac.energy | double | kWh | The HVAC energy use   
hvac.power_factor | complex | - | The HVAC power factor   
lights.demand[kW] | complex | kW | The lighting load   
lights.load | complex | kW | The lighting load   
lights.energy | complex | kWh | The lighting energy use   
lights.power_factor | double | - | The lighting power factor   
plugs.demand | complex | kW | The plug load   
plugs.load | complex | kW | The plug load   
plugs.energy | complex | kWh | The plug energy use   
plugs.power_factor | double | - | The plug power factor   
cooling_setpoint | double | degF | The cooling thermostat set-point   
heating_setpoint | double | degF | The heating thermostat set-point   
thermostat_deadband" | double | degF | The thermostat deadband (hysteresis)   
control.ventilation_fraction | double | pu | The current outside air fraction for ventilation   
control.lighting_fraction | double | pu | The current lighting fraction in effect   
  
## Default Office

The "default office" is an undefined construct. The minimum definition for an office object must include 
    
    
    object office{
       floor_height 6 ft;
       floor_area 4000 sf;
       interior_mass 2000;
       interior_UA 2.0;
       exterior_UA 2.0;
       hvac.cooling.capacity -4500; // must be negative
       hvac.heating.capacity 4500;
    }
    

## Office Schedules

The office has a built-in occupancy schedule subsystem that is used to determine the minimum air change coefficient. This subsystem parses a string and constructs a bitfield for the hours of each day that the building should be occupied. By default, office buildings are occupied from 8am to 5pm local time, Monday through Friday. 

The format for the office schedule consists of two parts, the day and the hours, in a semicolon delimited list. The default string is "1-5 8-17". The first part is the range for the days of the week, with 0 being Sunday, 1 for Monday, etc. The hours are numbered 0-23 and reflect a 24 hour clock. Multiple schedule elements can be aggregated with an OR operation by separating them with semicolons. For example, "1-4 8-17; 5 8-20; 6 8-23; 0 8-20" would define an 8-5 schedule Mon-Thur, 8-8 on Friday, 8am-11pm on Saturday, and 8-8 on Sunday. 

## Example

**TODO**: 

## Bugs

As of [Navajo (Version 4.3)] the office building class has not been validated. Use of the [residential] [house] class is recommended with appropriate adjustments to parameters until validation is completed. 

## See also

  * [User's manuals]
    * [Commercial module]
    * Building types 
      * Office
      * [Large office] **TODO**: 
      * [Small office] **TODO**: 
      * [Retail] **TODO**: 
      * [Grocery] **TODO**: 
      * [Food_service] **TODO**: 
      * [Lodging] **TODO**: 
      * [School] **TODO**: 
      * [Health] **TODO**: 
  * Technical documents 
    * [Requirements]
    * [Specifications]
    * [Technical support document]
    * [Developer's guide]
    * [Validation]
  * [Residential]
  * [Modules]

