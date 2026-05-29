## Transformer

Transformers provide a means to change the voltage from one node to another in the distribution system. Similar to the different **line** objects, a **transformer** object requires a configuration object to specify the details of the implementation. A typical transform implementation is 
    
    
    object transformer {
    	name xfrmr_709_775;
    	phases "ABC";
    	from node_709;
    	to node_775;
    	configuration xfrmr_config_400;
    	}

### Transformer Parameters

#### Properties

**transformer** objects are derived from **[link](04-link.md)** objects, so any parameters of the **[link](04-link.md)** object are available as well.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| configuration | object | N/A | I | `transformer_configuration` object that describes the specific transformer implementation. |
| climate | object | N/A | I | `climate` object that determines the outside ambient temperature around the transformer. |
| ambient_temperature | double | degC | IO | Output of the ambient temperature around the transformer. The default is 22.8 C. |
| top_oil_hot_spot_temperature | double | degC | IO | The hot spot temperature of the top-oil in the transformer. Default initial value is the ambient temperature. |
| winding_hot_spot_temperature | double | degC | IO | The hot spot temperature of the transformer windings. Default initial value is the ambient temperature. |
| percent_loss_of_life | double | N/A | O | The percent amount of transformer's operational life used. If no initial value is given then the transformer is considered brand new. |
| aging_constant | double | N/A | I | Experimental value used in determining the transformer insulation breaking point. The default is 15000 K. |
| use_thermal_model | bool | N/A | I | Flag used to enable use of the thermal/aging model. Default is FALSE. |
| transformer_replacement_count | double | N/A | IO | Counter of the number times the transformer has been replaced due to lifetime failure |
| aging_granularity | double | s | I | The maximum time step between transformer age and internal temperature updates. The default is 300 seconds. |
| phase_A_primary_flux_value | double | Wb | IO | Instantaneous magnetic flux in phase A on the primary side of the transformer during saturation calculations |
| phase_B_primary_flux_value | double | Wb | IO | Instantaneous magnetic flux in phase B on the primary side of the transformer during saturation calculations |
| phase_C_primary_flux_value | double | Wb | IO | Instantaneous magnetic flux in phase C on the primary side of the transformer during saturation calculations |
| phase_A_secondary_flux_value | double | Wb | IO | Instantaneous magnetic flux in phase A on the secondary side of the transformer during saturation calculations |
| phase_B_secondary_flux_value | double | Wb | IO | Instantaneous magnetic flux in phase B on the secondary side of the transformer during saturation calculations |
| phase_C_secondary_flux_value | double | Wb | IO | Instantaneous magnetic flux in phase C on the secondary side of the transformer during saturation calculations |

### Transformer Thermal/Aging Model

A newly added feature to transformers in 3.0 is a thermal/aging model. New parameters are placed within the transformer and and transformer_configuration in order to use this new feature. An implementation of the thermal model within transformer is 
    
    
    object transformer {
    	name xfrmr_709_775;
    	phases "ABC";
    	from node_709;
    	to node_775;
    	configuration xfrmr_config_400;
            use_thermal_model TRUE;
            climate Seattle;
            aging_granularity 300;
            percent_loss_of_life 20;
    	}

### Transformer State of Development

Transformer is considered a highly developed and validated model in terms of powerflow solutions, however, models may be developed to include more advanced features in the future. Additionally, future work may include additional transformer configurations. 


## Transformer Configuration

The `transformer_configuration` object describes the details of a particular transformer implementation. It includes information like the power rating, connection type, and nominal voltage on each side. A typical delta-delta transformer configuration would be implemented as 
    
    
    object transformer_configuration {
    	name xfrm_config_400;
    	connect_type DELTA_DELTA;
    	install_type PADMOUNT;
    	power_rating 500;
    	primary_voltage 4800;
    	secondary_voltage 480;
    	resistance 0.09;
    	reactance 1.81;
    	}

### Transformer Configuration Parameters

#### Properties

**transformer_configuration** does not declare inherited parent classes.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| connect_type | enumeration | N/A | I | Describes the electrical connection between the high and low side of the transformer. These may be referenced by keyword or number <br/> 0 - `UNKNOWN` \- An unknown transformer that will throw an error when used. <br/> 1 - `WYE_WYE` \- A wye to wye connected transformer. <br/> 2 - `DELTA_DELTA` \- A delta to delta connected transformer. <br/> 3 - `DELTA_GWYE` \- A delta to grounded-wye connected transformer. <br/> 4 - `SINGLE_PHASE` \- A single leg of a wye to wye connected transformer. <br/> 5 - `SINGLE_PHASE_CENTER_TAPPED` \- A single-phase, center-tapped transformer or split-phase transformer. Used to connect three-phase distribution to triplex-distribution. |
| install_type | enumeration | N/A | I | Describes the type of transformer the object represents. Used for informational purposes only. Valid types may be referenced by keyword or number <br/> 0 - `UNKNOWN` \- No information on the transformer physical type. <br/> 1 - `POLETOP` \- A pole-mounted transformer. <br/> 2 - `PADMOUNT` \- A pad, or ground level transformer. <br/> 3 - `VAULT` \- An enclosed transformer "building," either underground or above ground. |
| coolant_type | enumeration | N/A | I | The type of coolant used in the transformer. Valid types may be referenced by keyword or number <br/> 0 - `UNKNOWN` \- An unknown and unknown coolant type that will throw an error when used. <br/> 1 - `MINERAL_OIL` \- A transformer immersed in mineral oil. <br/> 2 - `DRY` \- A transformer with air as its coolant. This type is not handled yet. |
| cooling_type | enumeration | N/A | I | The type of cooling used in the transformer. Valid types may be referenced by keyword or number <br/> 0 - `UNKNOWN` \- An unknown and unknown cooling type that will throw an error when used. <br/> 1 - `OA` \- A liquid immersed self cooled transformer. <br/> 2 - `FA` \- A forced air cooled liquid immersed transformer. <br/> 3 - `NDFOA` \- A transformer with non-direction forced oil and air flow. <br/> 4 - `NDFOw` \- A transformer with non-direction forced oil and water flow. <br/> 5 - `DFOA` \- A transformer with direction forced oil and air flow. <br/> 6 - `DFOw` \- A transformer with direction forced oil and water flow. |
| primary_voltage | double | V | I | Nominal voltage of the primary winding side of the transformer. |
| secondary_voltage | double | V | I | Nominal voltage of the secondary winding side of the transformer. |
| power_rating | double | kVA | I | Nominal power rating of the entire transformer. |
| powerA_rating | double | kVA | I | Nominal power rating of windings associated with phase `A` if wye-connected or `AB` if delta-connected. |
| powerB_rating | double | kVA | I | Nominal power rating of windings associated with phase `B` if wye-connected or `BC` if delta-connected. |
| powerC_rating | double | kVA | I | Nominal power rating of windings associated with phase `C` if wye-connected or `CA` if delta-connected. |
| resistance | double | pu*Ohm | I | De-referenced characteristic resistance of the transformer |
| reactance | double | pu*Ohm | I | De-referenced characteristic reactance of the transformer |
| impedance | complex | pu*Ohm | I | De-referenced characteristic impedance of the transformer. Note that `resistance` and `reactance` above directly write the real and complex portions of this parameter, so only `resistance` and `reactance` or just `impedance` need to be specified. |
| resistance1 | double | pu*Ohm | I | Secondary series impedance (only used when you want to define each individual winding seperately), pu, real |
| reactance1 | double | pu*Ohm | I | Secondary series impedance (only used when you want to define each individual winding seperately), pu, imag |
| impedance1 | complex | pu*Ohm | I | De-referenced characteristic impedance of the transformer. Currently only used with center-tap transformers. Defaults to zero; not required for operation. Allows user to reflect impedance values on both the primary and secondary side of transformer (primary is specified by `impedance`, secondary by `impedance1` and `impedance2`). Phase 1 equals phase 1 of split-phasing. |
| resistance2 | double | pu*Ohm | I | Secondary series impedance (only used when you want to define each individual winding seperately), pu, real |
| reactance2 | double | pu*Ohm | I | Secondary series impedance (only used when you want to define each individual winding seperately), pu, imag |
| impedance2 | complex | pu*Ohm | I | De-referenced characteristic impedance of the transformer. Currently only used with center-tap transformers. Defaults to zero; not required for operation. Allows user to reflect impedance values on both the primary and secondary side of transformer (primary is specified by `impedance`, secondary by `impedance1` and `impedance2`). Phase 2 equals phase 2 of split-phasing. |
| shunt_resistance | double | pu*Ohm | I | Shunt impedance on primary side, pu, real |
| shunt_reactance | double | pu*Ohm | I | Shunt impedance on primary side, pu, imag |
| shunt_impedance | complex | pu*Ohm | I | Some transformer models support a shunt impedance value to represent no load losses (only wye-wye and center-tap transformers use this value at this time). |
| core_coil_weight | double | lb | I | The weight of the transformer's core and coil assembly. |
| tank_fittings_weight | double | lb | I | The weight of the transformer's tank and fittings assembly. |
| oil_volume | double | gal | I | The amount of oil contained within the transformer. |
| rated_winding_time_constant | double | h | I | The winding's time constant. |
| rated_winding_hot_spot_rise | double | degC | I | The winding hot spot temperature rise over ambient at rated transformer load. Default is 80 degrees C. |
| rated_top_oil_rise | double | degC | I | The top oil temperature rise over ambient at rated transformer load. |
| no_load_loss | double | pu | I | The losses through the transformer when there is no load. |
| full_load_loss | double | pu | I | This is the losses of the transformer when at rated load. |
| reactance_resistance_ratio | double | N/A | I | The ratio the reactance to the resistance for both shunt and series impedances of the transformer. default is 10. |
| installed_insulation_life | double | h | I | The transformer's operational time span. |
| magnetization_location | enumeration | N/A | I | winding to place magnetization influence for in-rush calculations Valid values: `NONE`, `PRIMARY`, `SECONDARY`, `BOTH`. |
| inrush_saturation_enabled | bool | N/A | I | flag to include saturation effects during inrush calculations |
| L_A | double | pu | I | Air core inductance of transformer |
| phi_K | double | pu | I | Knee flux value where the air core inductance interstes the flux axis of the saturation curve |
| phi_M | double | pu | I | Peak magnetization flux at rated voltage of the saturation curve |
| I_M | double | pu | I | Peak magnetization current at rated voltage of the saturation curve |
| T_D | double | N/A | I | Inrush decay time constant for inrush current |

### Transformer Thermal/Aging Model

A new feature added to transformers in 3.0 is the thermal/aging model. New parameters are added to `transformer_configuration` for this new feature. This model only works with a SINGLE_PHASE_CENTER_TAPPED **transformer**. A typical implementation is 
    
    
    object transformer_configuration {
    	name xfrm_config_400;
    	connect_type SINGLE_PHASE_CENTER_TAPPED;
    	install_type PADMOUNT;
    	power_rating 500;
    	primary_voltage 4800;
    	secondary_voltage 480;
            full_load_loss 0.006;
            no_load_loss 0.003;
            reactance_resistance_ratio 10;
            core_coil_weight 50;
            tank_fittings_weight 60;
            oil_volume 5;
            rated_winding_hot_spot_rise 80;
            rated_top_oil_rise 30;
            rated_winding_time_constant 0.5;
            installed_insulation_life 175200;
            coolant_type MINERAL_OIL;
            cooling_type OA;
    	}

### Transformer Configuration State of Development

Transformer Configuration is considered a highly developed and validated model in terms of powerflow solutions, however, models may be developed to include more advanced features in the future, and additional models may be included as needed. 
