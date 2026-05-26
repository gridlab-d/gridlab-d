## Transformer

!!! warning
    This page was automatically generated and requires review.

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
| transformer_replacement_count | double | N/A | IO | ⚠️ counter of the number times the transformer has been replaced due to lifetime failure |
| aging_granularity | double | s | I | The maximum time step between transformer age and internal temperature updates. The default is 300 seconds. |
| phase_A_primary_flux_value | double | Wb | IO | ⚠️ instantaneous magnetic flux in phase A on the primary side of the transformer during saturation calculations |
| phase_B_primary_flux_value | double | Wb | IO | ⚠️ instantaneous magnetic flux in phase B on the primary side of the transformer during saturation calculations |
| phase_C_primary_flux_value | double | Wb | IO | ⚠️ instantaneous magnetic flux in phase C on the primary side of the transformer during saturation calculations |
| phase_A_secondary_flux_value | double | Wb | IO | ⚠️ instantaneous magnetic flux in phase A on the secondary side of the transformer during saturation calculations |
| phase_B_secondary_flux_value | double | Wb | IO | ⚠️ instantaneous magnetic flux in phase B on the secondary side of the transformer during saturation calculations |
| phase_C_secondary_flux_value | double | Wb | IO | ⚠️ instantaneous magnetic flux in phase C on the secondary side of the transformer during saturation calculations |

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
