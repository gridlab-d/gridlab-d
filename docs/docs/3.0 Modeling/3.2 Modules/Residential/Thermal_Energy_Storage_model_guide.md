# Thermal Energy Storage model guide

Thermal energy storage is based on technology that uses ice to cool air in place of a standard air conditioning unit. By making the ice at night with a compressor and utilizing the ice during the day to cool, the peak load of the cooling can be shifted to off peak hours. The model is based on the specifications and general functionality of the Ice Bear® unit developed by Ice Energy®. 

The thermal energy storage unit uses a standard compressor and R410a refrigerant to cool down and freeze stored water during off peak hours when demand on the power grid is less. During peak hours when the demand for power is higher, the thermal storage unit runs a pump to flow the refrigerant through the ice block and a heat exchanger to cool air for the building. The heat exchanger is integrated into the existing cooling system of the building such that it is in the air conditioning unit itself or in the duct work of the building. The pump uses relatively little power, thereby reducing the additional peak demand load that would normally by placed by a traditional compressor in a standard air conditioning unit. 

The Ice Bear® system is designed to be used in conjunction with existing building cooling systems. These could be commercial rooftop and split systems ranging from 4-20 Tons or ductless systems ranging from 3-5 Tons. It can be applied to a 3-5 Ton system or it can be used as a 5 Ton stage on a 7.5-20 Ton system. The Ice Bear® unit is designed to be a 5 Ton unit with 30 Ton-hours of storage and has restrictions of running above 15°F and below 115°F outdoor temperature. 

# Model

The Ice Bear® system is a 5 Ton unit with 30 Ton-hours of storage and is designed to be used on commercial systems. For the purposes of GridLab-d, thermal storage is scaled based on the size of each house. Several houses can be put together, simulating a commercial building, so forcing a 5 Ton sizing on each house is not practical for simulating a commercial configuration. This model is not designed to model the Ice Bear® unit, but to model the effects of the technology based on the Ice Bear® unit's specifications. 

## Sizing

The thermal energy storage unit is sized based on the sizing of the air conditioning unit as specified in house or can alternatively be user defined. A simple linear scaling is applied based on the Ice Bear® system specifications. 

## Power Consumption

The power consumption for the thermal energy storage unit is a simple ratio of the Ice Bear® power consumption ($P_{IceBear}$) and cooling load ($Q_{IceBear}$) compared to the cooling load as defined in house or as defined by the user ($Q_{house}$). The 5 Ton Ice Bear® unit uses 3,360 Watts to charge at off peak hours and 300 Watts to cool air during on peak hours. 

    $P_{actual} = P_{IceBear}\frac{Q_{house}}{Q_{IceBear}}$

It should be noted that house will run the fan to circulate air when the thermal energy storage is used to cool the air. This is a requirement of the system, but the calculations for the fan power are handled by house and not thermal energy storage. 

## Ice Storage

The cooling load rating for the thermal energy storage unit is, by default, defined by house, but can be user defined. The Ice Bear® is a 5 Ton unit ($Q_{IceBear}$) with 30 Ton-hours or 360,000 Btu of energy storage ($S_{IceBear}$). This information is used to calculate the energy storage of the thermal energy storage unit to be used with the house. 

    $S_{actual} = S_{IceBear}\frac{Q_{house}}{Q_{IceBear}}$

Additionally, the level of the stored energy at start up can be defined as the actual amount in Btu or as a state of charge defined as a percentage. By default, the unit is set to be fully charged. 

## Thermal Losses

The ice block being used to store thermal energy will lose stored capacity when the outside temperature is greater than the freezing temperature of water. The rate at which the stored thermal energy is lost is dependent on the insulating material and the outside temperature. By default, there is no loss of thermal energy storage, but the coefficient of thermal conductivity can be user specified in W/m/°C. The thickness of the insulation is fixed at 50mm (0.05m), so the user can adjust this by scaling the coefficient of thermal conductivity accordingly. 

    $ Rate = \frac{kA\Delta{T}}{d}$

where k is the coefficient of thermal conductivity (W/M/°C) of the material insulating the ice block, A is the surface area (m2), ΔT is t1 \- t2 (t1 being the ice temperature in °F and t2 being the outside temperature in °F), d is the thickness of the material insulating the ice block and the Rate is in Joules per second or Watts. k is converted from the more common found form of W/m/°C to Btu/m/sec/°F to match the common variables in the GridLab-D, where 1 W/M/°C = 0.00052667 Btu/m/sec/°F. 

The Ice Bear® unit stores 460 gallons of water for 360,000 Btu thermal energy storage. The volume of water is scaled linearly with the amount of required thermal energy storage. Converting gallons to m3 (460 U.S. Gallons = 1.7413 m3) and then converting volume to surface area with the assumption that the water is stored in a cube. 

Table 1 - Coefficient of Thermal Conductivity (W/m/°C)  Material | k (W/m/°C)   
---|---  
Air | 0.024   
Argon | 0.016   
Cellulose | 0.039   
Cotton Wool | 0.029   
Expanded Polystyrene | 0.03   
Foamed Plastics | 0.03   
Glass Wool | 0.04   
Kapok Insulation | 0.034   
Magnesia Insulation | 0.07   
Nitrogen | 0.024   
Polyurethane Foam | 0.02   
Rock Wool Insulation | 0.045   
Sawdust | 0.08   
Silica Aerogel | 0.02   
Styrofoam | 0.033   
Urethane Foam | 0.021   
Vermiculite | 0.058   
  
# Inputs

There are several inputs for thermal energy storage, however, none of these variables need to be user defined as everything either has a default value or is set by the designed cooling capacity in house. The default values are set for more ideal conditions (i.e. no thermal loss). If many thermal storage units are to be used, it is recommended to use skewed schedules to offset the recharge and discharge loading on the system. 

## User Defined Inputs

Table 2 - User Defined Variables for Thermal Storage  Property Name | Type | Unit | Description   
---|---|---|---  
total_capacity | double | Btu | The total capacity of energy storage of the unit. When left to default, it is scaled based on the HVAC sizing in house.   
stored_capacity | double | Btu | The amount of energy stored in the unit at the start of the simulation. If this exceeds the total_capacity, it will be set equal to the total_capacity. If SOC (state of charge) is also set, SOC is the dominant value and will be used instead.   
recharge_power | double | kW | The rated power required to run the compressor and charge the unit (make ice).   
discharge_power | double | kW | The rated power required to run the pump and discharge the unit (melt the ice).   
recharge_pf | double | NA | The rated power factor of the compressor to charge the unit (make ice).   
discharge_pf | double | NA | The rated power factor of the pump to discharge the unit (melt the ice).   
discharge_schedule_type | enum | NA | Specifies the use of either the "INTERNAL" or "EXTERNAL" schedule for the discharge (INTERNAL = default, EXTERNAL = user defined)   
recharge_schedule_type | enum | NA | Specifies the use of either the "INTERNAL" or "EXTERNAL" schedule for the recharge (INTERNAL = default, EXTERNAL = user defined)   
recharge_time | double | NA | The time schedule indicating the hours of operation for the recharge cycle (0 = OFF, 1 = ON). The recharge and discharge cycles can not overlap. The model will default to a recharge cycle in the event that both are set to be on.   
discharge_time | double | NA | The time schedule indicating the hours of operation for the discharge cycle (0 = OFF, 1 = ON). The recharge and discharge cycles can not overlap. The model will default to a recharge cycle in the event that both are set to be on.   
discharge_rate | double | Btu/hr | The rated capacity of the unit as it would relate to a normal HVAC unit (i.e. a 5 Ton unit to cool a house). When left to default, it is scaled based on the HVAC sizing in house.   
SOC | double | % | The state of charge of the system in percent of ice energy available. If store capacity is also set, SOC is the dominant value and will be used.   
k | double | W/m/°C | The coefficient of thermal conductivity in Watts per meter per °C.   
  
## Inputs Taken From house

Table 3 - Internal Variables for Thermal Storage  Property Name | Type | Unit | Description   
---|---|---|---  
design_cooling_capacity | double | Btu/hr | The designed cooling capacity of the unit.   
outside_temperature | double | °F | The outside temperature.   
thermal_storage_present | double | NA | Used as a bit to let house know that thermal storage is present and has capacity to be used (0 = not installed or not available, 1 = available and usable).   
thermal_storage_inuse | double | NA | Used as a bit from house to let thermal storage know that it needs to turn on and provide cooling.   
  

