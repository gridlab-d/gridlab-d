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

Input indicates properties you can set in models. Output marks properties produced or modified during simulation runtime.

| Property Name | Type | Unit | Input | Output | Description |
| --- | --- | --- | --- | --- | --- |
| connect_type | enumeration | N/A | ✓ |  | Describes the electrical connection between the high and low side of the transformer. These may be referenced by keyword or number &lt;br/&gt; 0 - `UNKNOWN` \- An unknown transformer that will throw an error when used. &lt;br/&gt; 1 - `WYE_WYE` \- A wye to wye connected transformer. &lt;br/&gt; 2 - `DELTA_DELTA` \- A delta to delta connected transformer. &lt;br/&gt; 3 - `DELTA_GWYE` \- A delta to grounded-wye connected transformer. &lt;br/&gt; 4 - `SINGLE_PHASE` \- A single leg of a wye to wye connected transformer. &lt;br/&gt; 5 - `SINGLE_PHASE_CENTER_TAPPED` \- A single-phase, center-tapped transformer or split-phase transformer. Used to connect three-phase distribution to triplex-distribution. |
| install_type | enumeration | N/A | ✓ |  | Describes the type of transformer the object represents. Used for informational purposes only. Valid types may be referenced by keyword or number &lt;br/&gt; 0 - `UNKNOWN` \- No information on the transformer physical type. &lt;br/&gt; 1 - `POLETOP` \- A pole-mounted transformer. &lt;br/&gt; 2 - `PADMOUNT` \- A pad, or ground level transformer. &lt;br/&gt; 3 - `VAULT` \- An enclosed transformer &quot;building,&quot; either underground or above ground. |
| coolant_type | enumeration | N/A | ✓ |  | The type of coolant used in the transformer. Valid types may be referenced by keyword or number &lt;br/&gt; 0 - `UNKNOWN` \- An unknown and unknown coolant type that will throw an error when used. &lt;br/&gt; 1 - `MINERAL_OIL` \- A transformer immersed in mineral oil. &lt;br/&gt; 2 - `DRY` \- A transformer with air as its coolant. This type is not handled yet. |
| cooling_type | enumeration | N/A | ✓ |  | The type of cooling used in the transformer. Valid types may be referenced by keyword or number &lt;br/&gt; 0 - `UNKNOWN` \- An unknown and unknown cooling type that will throw an error when used. &lt;br/&gt; 1 - `OA` \- A liquid immersed self cooled transformer. &lt;br/&gt; 2 - `FA` \- A forced air cooled liquid immersed transformer. &lt;br/&gt; 3 - `NDFOA` \- A transformer with non-direction forced oil and air flow. &lt;br/&gt; 4 - `NDFOw` \- A transformer with non-direction forced oil and water flow. &lt;br/&gt; 5 - `DFOA` \- A transformer with direction forced oil and air flow. &lt;br/&gt; 6 - `DFOw` \- A transformer with direction forced oil and water flow. |
| primary_voltage | double | V | ✓ |  | Nominal voltage of the primary winding side of the transformer. |
| secondary_voltage | double | V | ✓ |  | Nominal voltage of the secondary winding side of the transformer. |
| power_rating | double | kVA | ✓ |  | Nominal power rating of the entire transformer. |
| powerA_rating | double | kVA | ✓ |  | Nominal power rating of windings associated with phase `A` if wye-connected or `AB` if delta-connected. |
| powerB_rating | double | kVA | ✓ |  | Nominal power rating of windings associated with phase `B` if wye-connected or `BC` if delta-connected. |
| powerC_rating | double | kVA | ✓ |  | Nominal power rating of windings associated with phase `C` if wye-connected or `CA` if delta-connected. |
| resistance | double | pu*Ohm | ✓ |  | De-referenced characteristic resistance of the transformer |
| reactance | double | pu*Ohm | ✓ |  | De-referenced characteristic reactance of the transformer |
| impedance | complex | pu*Ohm | ✓ |  | De-referenced characteristic impedance of the transformer. Note that `resistance` and `reactance` above directly write the real and complex portions of this parameter, so only `resistance` and `reactance` or just `impedance` need to be specified. |
| resistance1 | double | pu*Ohm | ✓ |  | ⚠️ Secondary series impedance (only used when you want to define each individual winding seperately, pu, real |
| reactance1 | double | pu*Ohm | ✓ |  | ⚠️ Secondary series impedance (only used when you want to define each individual winding seperately, pu, imag |
| impedance1 | complex | pu*Ohm | ✓ |  | De-referenced characteristic impedance of the transformer. Currently only used with center-tap transformers. Defaults to zero; not required for operation. Allows user to reflect impedance values on both the primary and secondary side of transformer (primary is specified by `impedance`, secondary by `impedance1` and `impedance2`). Phase 1 equals phase 1 of split-phasing. |
| resistance2 | double | pu*Ohm | ✓ |  | ⚠️ Secondary series impedance (only used when you want to define each individual winding seperately, pu, real |
| reactance2 | double | pu*Ohm | ✓ |  | ⚠️ Secondary series impedance (only used when you want to define each individual winding seperately, pu, imag |
| impedance2 | complex | pu*Ohm | ✓ |  | De-referenced characteristic impedance of the transformer. Currently only used with center-tap transformers. Defaults to zero; not required for operation. Allows user to reflect impedance values on both the primary and secondary side of transformer (primary is specified by `impedance`, secondary by `impedance1` and `impedance2`). Phase 2 equals phase 2 of split-phasing. |
| shunt_resistance | double | pu*Ohm | ✓ |  | ⚠️ Shunt impedance on primary side, pu, real |
| shunt_reactance | double | pu*Ohm | ✓ |  | ⚠️ Shunt impedance on primary side, pu, imag |
| shunt_impedance | complex | pu*Ohm | ✓ |  | Some transformer models support a shunt impedance value to represent no load losses (only wye-wye and center-tap transformers use this value at this time). |
| core_coil_weight | double | lb | ✓ |  | The weight of the transformer&#x27;s core and coil assembly. |
| tank_fittings_weight | double | lb | ✓ |  | The weight of the transformer&#x27;s tank and fittings assembly. |
| oil_volume | double | gal | ✓ |  | The amount of oil contained within the transformer. |
| rated_winding_time_constant | double | h | ✓ |  | The winding&#x27;s time constant. |
| rated_winding_hot_spot_rise | double | degC | ✓ |  | The winding hot spot temperature rise over ambient at rated transformer load. Default is 80 degrees C. |
| rated_top_oil_rise | double | degC | ✓ |  | The top oil temperature rise over ambient at rated transformer load. |
| no_load_loss | double | pu | ✓ |  | The losses through the transformer when there is no load. |
| full_load_loss | double | pu | ✓ |  | This is the losses of the transformer when at rated load. |
| reactance_resistance_ratio | double | N/A | ✓ |  | The ratio the reactance to the resistance for both shunt and series impedances of the transformer. default is 10. |
| installed_insulation_life | double | h | ✓ |  | The transformer&#x27;s operational time span. |
| magnetization_location | enumeration | N/A | ✓ |  | ⚠️ winding to place magnetization influence for in-rush calculations Valid values: `NONE`, `PRIMARY`, `SECONDARY`, `BOTH`. |
| inrush_saturation_enabled | bool | N/A | ✓ |  | ⚠️ flag to include saturation effects during inrush calculations |
| L_A | double | pu | ✓ |  | ⚠️ Air core inductance of transformer |
| phi_K | double | pu | ✓ |  | ⚠️ Knee flux value where the air core inductance interstes the flux axis of the saturation curve |
| phi_M | double | pu | ✓ |  | ⚠️ Peak magnetization flux at rated voltage of the saturation curve |
| I_M | double | pu | ✓ |  | ⚠️ Peak magnetization current at rated voltage of the saturation curve |
| T_D | double | N/A | ✓ |  | ⚠️ Inrush decay time constant for inrush current |

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
