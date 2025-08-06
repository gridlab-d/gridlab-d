# Commercial User Guide 
## Commercial Overview

TODO

## Using the Office

TODO

### Default Office

The "default office" is an incomplete construct. The minimum definition for an office object includes: 
    
    
    object office{
       floor_height 6 ft;
       floor_area 4000 sf;
       interior_mass 2000;
       interior_UA 2.0;
       exterior_UA 2.0;
       hvac.cooling.capacity -4500; // must be negative
       hvac.heating.capacity 4500;
    }
    

### Office Schedule

The office has a built-in occupancy schedule subsystem that is used to determine the minimum air change coefficient. This subsystem parses a string and constructs a bitfield for the hours of each day that the building should be occupied. By default, office buildings are occupied from 8am to 5pm local time, Monday through Friday. 

The format for the office schedule consists of two parts, the day and the hours, in a semicolon delimited list. The default string is "1-5 8-17". The first part is the range for the days of the week, with 0 being Sunday, 1 for Monday, etc. The hours are numbered 0-23 and reflect a 24 hour clock. Multiple schedule elements can be aggregated with an OR operation by seperating them with semicolons. For example, "1-4 8-17; 5 8-20; 6 8-23; 0 8-20" would define an 8-5 schedule Mon-Thur, 8-8 on Friday, 8am-11pm on Saturday, and 8-8 on Sunday. 

### Properties

Property Name | Type | Unit | Description   
---|---|---|---  
floor_area | double | ft^2 | Floor area of the office (presuming one floor)   
floor_height | double | ft | Ceiling height within the office interior   
exterior_ua | double | BTU/degF/hr | Exterior thermal resistance   
interior_ua | double | BTU/degF/hr | Interior thermal resistance   
interior_mass | double | BTU/degF | ...   
glazing | double | ft^2 | ...   
glazing.north | double | ft^2 | ...   
glazing.northeast | double | ft^2 | ...   
glazing.east | double | ft^2 | ...   
glazing.southeast | double | ft^2 | ...   
glazing.south | double | ft^2 | ...   
glazing.southwest | double | ft^2 | ...   
glazing.west | double | ft^2 | ...   
glazing.northwest | double | ft^2 | ...   
glazing.horizontal | double | ft^2 | ...   
glazing.coefficient | double | per unit | ...   
occupancy | double | - | Current occupancy ratio   
occupants | double | people | Total occupants for the office   
schedule | char256 | - | Schedule string. Semicolon delimited cron-style schedule definition.   
air_temperature | double | degF | Office interior air temperature   
mass_temperature | double | degF | Office interior mass temperature   
temperature_change | double | degF/hr | ...   
Qh | double | BTU/hr | ...   
Qs | double | BTU/hr | ...   
Qi | double | BTU/hr | ...   
Qz | double | BTU/hr | ...   
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
lights.fraction | double | - | The fraction of the installed lights that are turned on   
plugs.capacity | double | kW | Total power of the devices plugged into wall sockets   
plugs.fraction | double | - | The current fraction of the total power draw from the various devices   
demand | complex | kW | Aggregate peak power draw from the office   
total_load | complex | kW | Current aggregate power draw from the office   
energy | complex | kWh | Accumulated energy consumed by the office   
power_factor | complex | - | ...   
power | complex | kW | Constant power component of the office load   
current | complex | A | Constant current component of the office load   
admittance | complex | 1/Ohm | Constant resistance component of the office load   
hvac.demandkW | complex | kW | ...   
hvac.loadkW | complex | kW | ...   
hvac.energy | double | kWh | ...   
hvac.power_factor | complex | - | ...   
lights.demandkW | complex | kW | ...   
lights.loadkW | complex | kW | ...   
lights.energy | complex | kWh | ...   
lights.power_factor | double | - | ...   
plugs.demandkW | complex | - | ...   
plugs.loadkW | complex | - | ...   
plugs.energy | complex | - | ...   
plugs.power_factor | double | - | ...   
cooling_setpoint | double | - | ...   
heating_setpoint | double | - | ...   
thermostat_deadband" | double | - | ...   
control.ventilation_fraction | double | - | ...   
control.lighting_fraction | double | - | ...   
  

## Commercial \- Commercial building developer's guide 

Note
    The small office building is part of the original implementation of the commercial module and is expected to be deprecated when the full commercial building implementation is completed. This will include deprecation of the multizone class. Small office buildings will be derived from the building implementation when that is validated and released.

### Building

The building class implements that abstract class used to solve all linearized multizone building models. All multizone commercial building classes are derived from this class. 

#### Class members

_General Properties_ 
| Property| Unit | Constraints | Default | Description | Remarks  
--|--|--|--|--|--|
T | degF | N×1 ∈ **R** | Ø | Node temperatures |   
N | (int16) | ∈ **N** + | 1 | Number of nodes in model   
U | Btu/degF/h | N×N symmetric ∈ **R** *2 | Ø | Node conductances   
C | Btu/degF | N×1 ∈ **R** * | Ø | Node capacitance | NaN indicates outdoor node   
Q | Btu/h | N×1 ∈ **R** | Ø | Node heat flows |   
Qs | Btu/h | N×1 ∈ **R *** | Ø | Node solar heat gain |   
Qi | Btu/h | N×1 ∈ **R *** | Ø | Node internal heat gain |   
  
_Default HVAC properties_ 
| Property| Unit | Constraints | Default | Description | Remarks  
--|--|--|--|--|--|
Qfl | Btu/h | N×1 ∈ **R *** | NaN | Node fan heat gain at low power |   
Qfh | Btu/h | N×1 ∈ **R *** | NaN | Node fan heat gain at high power |   
Qf | Btu/h | N×1 ∈ **R *** | NaN | Node heat gain from fans |   
Qhc | Btu/h | N×1 ∈ **R *** | NaN | Node heating capacity |   
Qh | Btu/h | N×1 ∈ **R *** | NaN | Node heat gain from heating |   
Qcc | Btu/h | N×1 ∈ **R *** | NaN | Node cooling capacity |   
Qc | Btu/h | N×1 ∈ **R *** | NaN | Node heat loss from cooling | 

**Note** : NaN is used to indicate that no default HVAC equipment is associated with the node   
  
_Default controller properties_ 

| Property| Unit | Constraints | Default | Description | Remarks  
--|--|--|--|--|--|
Ts | (double) | N×1 ∈ {0,1,2,3,4,5} | NaN | HVAC state | 0=OFF, 1=VENT, 2=HEAT, 3=COOL, 4=AUX, 5=ECON   
Vm | pu/h | N×1 ∈ **R *** | NaN | Minimum ventilation required |   
Th | degF | N×1 ∈ **R +** | NaN | Heating set-point | Must be less than $Tc-2Td$  
Tc | degF | N×1 ∈ **R +** | NaN | Cooling set-point | Must be greater than $Th+2Td$  
Td | degF | N×1 ∈ **R +** | NaN | Set-point deadband |   
tl | s | N×1 ∈ **N** + | 300 | Control lockout time | Must be less than or equal to maximum_timestep   


**Note** : NaN is used to indicate that no default control equipment is associated with the node   

  
_Other members_

   |  |  |  |  |  |  
--|--|--|--|--|
autosize | (bool) | ∈ {TRUE,FALSE} | FALSE | Enables automatic sizing of arrays   
load | (enduse) |  |  | Electric end use load composition   
  
### Default HVAC Controller

The default controller implements a simple single zone vent/heat/cool/aux control. To override the default controller you must implement the _plc_() function is the derived class. 

The default control strategy for node _n_ is as follows: 
    
    
    if mode == OFF || mode == VENT
      if T < Th-2*Td
        mode = AUX
      else if T < Th - Td/2
        mode = HEAT
      else if T > Tc + Td/2
        mode = COOL
      else if Vm > 0 
        mode = VENT
      else
        mode = OFF
    
    else if _mode_ == HEAT
      if T < Th-2*Td
        mode = AUX
      else if T > Th+Td/2
        if Vm > 0
          mode = VENT
        else
          mode = OFF
    
    else if _mode_ == COOL
      if T < Tc - Td/2
        if Vm > 0
          mode = VENT
        else
          mode = OFF
    
    else if _mode_ == AUX
      if T > Th - Td/2
        if Vm > 0
          mode = VENT
        else
          mode = OFF
    

Note - 
    Implementing the _plc_() function for a building means that the default controller is disabled for all nodes in the building. This means that if you want to continue using the default controller for some nodes you must call the building::plc() function directly for that node.  
