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
    

### Transformer Configuration Parameters

Property Name  | Type  | Unit  | Description   
---|---|---|---  
**connect_type**  | enumeration  | N/A  | Describes the electrical connection between the high and low side of the transformer. These may be referenced by keyword or number <br/> 0 - `UNKNOWN` \- An unknown transformer that will throw an error when used. <br/> 1 - `WYE_WYE` \- A wye to wye connected transformer. <br/> 2 - `DELTA_DELTA` \- A delta to delta connected transformer. <br/> 3 - `DELTA_GWYE` \- A delta to grounded-wye connected transformer. <br/> 4 - `SINGLE_PHASE` \- A single leg of a wye to wye connected transformer. <br/> 5 - `SINGLE_PHASE_CENTER_TAPPED` \- A single-phase, center-tapped transformer or split-phase transformer. Used to connect three-phase distribution to triplex-distribution.  
install_type  | enumeration  | N/A  | Describes the type of transformer the object represents. Used for informational purposes only. Valid types may be referenced by keyword or number <br/> 0 - `UNKNOWN` \- No information on the transformer physical type. <br/> 1 - `POLETOP` \- A pole-mounted transformer. <br/> 2 - `PADMOUNT` \- A pad, or ground level transformer. <br/> 3 - `VAULT` \- An enclosed transformer "building," either underground or above ground.  
**primary_voltage**  | double  | Volts  | Nominal voltage of the primary winding side of the transformer.   
**secondary_voltage**  | double  | Volts  | Nominal voltage of the secondary winding side of the transformer.   
**power_rating**  | double  | kilo-Volt Amperes  | Nominal power rating of the entire transformer.   
**powerA_rating**  | double  | kilo-Volt Amperes  | Nominal power rating of windings associated with phase `A` if wye-connected or `AB` if delta-connected.   
**powerB_rating**  | double  | kilo-Volt Amperes  | Nominal power rating of windings associated with phase `B` if wye-connected or `BC` if delta-connected.   
**powerC_rating**  | double  | kilo-Volt Amperes  | Nominal power rating of windings associated with phase `C` if wye-connected or `CA` if delta-connected.   
**resistance**  | double  | per-unit Ohm  | De-referenced characteristic resistance of the transformer   
**reactance**  | double  | per-unit Ohm  | De-referenced characteristic reactance of the transformer   
**impedance**  | complex  | per-unit Ohm  | De-referenced characteristic impedance of the transformer. Note that `resistance` and `reactance` above directly write the real and complex portions of this parameter, so only `resistance` and `reactance` or just `impedance` need to be specified.   
**shunt_impedance**  | complex  | per-unit Ohm  | Some transformer models support a shunt impedance value to represent no load losses (only wye-wye and center-tap transformers use this value at this time).   
**impedance1**  | complex  | per-unit Ohm  | De-referenced characteristic impedance of the transformer. Currently only used with center-tap transformers. Defaults to zero; not required for operation. Allows user to reflect impedance values on both the primary and secondary side of transformer (primary is specified by `impedance`, secondary by `impedance1` and `impedance2`). Phase 1 equals phase 1 of split-phasing.   
**impedance2**  | complex  | per-unit Ohm  | De-referenced characteristic impedance of the transformer. Currently only used with center-tap transformers. Defaults to zero; not required for operation. Allows user to reflect impedance values on both the primary and secondary side of transformer (primary is specified by `impedance`, secondary by `impedance1` and `impedance2`). Phase 2 equals phase 2 of split-phasing.   
**full_load_loss**  | double  | per-unit Ohm  | This is the losses of the transformer when at rated load.   
**no_load_loss**  | double  | per-unit Ohm  | The losses through the transformer when there is no load.   
**reactance_resistance_ratio**  | double  | N/A  | The ratio the reactance to the resistance for both shunt and series impedances of the transformer. default is 10.   
**tank_fittings_weight**  | double  | Pounds  | The weight of the transformer's tank and fittings assembly.   
**oil_volume**  | double  | Gallons  | The amount of oil contained within the transformer.   
**core_coil_weight**  | double  | Pounds  | The weight of the transformer's core and coil assembly.   
**rated_winding_hot_spot_rise**  | double  | Celsius  | The winding hot spot temperature rise over ambient at rated transformer load. Default is 80 degrees C.   
**rated_top_oil_rise**  | double  | Celsius  | The top oil temperature rise over ambient at rated transformer load.   
**rated_winding_time_constant**  | double  | Hours  | The winding's time constant.   
**installed_insulation_life**  | double  | Hours  | The transformer's operational time span.   
**coolant_type**  | enumeration  | N/A  | The type of coolant used in the transformer. Valid types may be referenced by keyword or number <br/> 0 - `UNKNOWN` \- An unknown and unknown coolant type that will throw an error when used. <br/> 1 - `MINERAL_OIL` \- A transformer immersed in mineral oil. <br/> 2 - `DRY` \- A transformer with air as its coolant. This type is not handled yet. 
**cooling_type**  | enumeration  | N/A  | The type of cooling used in the transformer. Valid types may be referenced by keyword or number <br/> 0 - `UNKNOWN` \- An unknown and unknown cooling type that will throw an error when used. <br/> 1 - `OA` \- A liquid immersed self cooled transformer. <br/> 2 - `FA` \- A forced air cooled liquid immersed transformer. <br/> 3 - `NDFOA` \- A transformer with non-direction forced oil and air flow. <br/> 4 - `NDFOw` \- A transformer with non-direction forced oil and water flow. <br/> 5 - `DFOA` \- A transformer with direction forced oil and air flow. <br/> 6 - `DFOw` \- A transformer with direction forced oil and water flow.  
  
### Transformer Configuration State of Development

Transformer Configuration is considered a highly developed and validated model in terms of powerflow solutions, however, models may be developed to include more advanced features in the future, and additional models may be included as needed. 

