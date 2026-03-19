## Transformer

Transformers provide a means to change the voltage from one node to another in the distribution system. Similar to the different **line** objects, a **transformer** object requires a configuration object to specify the details of the implementation. A typical transform implementation is 
    
    
    object transformer {
    	name xfrmr_709_775;
    	phases "ABC";
    	from node_709;
    	to node_775;
    	configuration xfrmr_config_400;
    	}
    

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
    

### Transformer Parameters

Transformers are derived from the **link** class and inherit all of its properties. The only unique property a **transformer** object contains is 

Property Name  | Type  | Unit  | Description   
---|---|---|---  
**aging_constant**  | double  | Kelvin  | Experimental value used in determining the transformer insulation breaking point. The default is 15000 K.   
**aging_granularity**  | double  | sec  | The maximum time step between transformer age and internal temperature updates. The default is 300 seconds.   
**ambient_temperature**  | double  | Celsius  | Output of the ambient temperature around the transformer. The default is 22.8 C.   
**climate**  | object  | N/A  | `climate` object that determines the outside ambient temperature around the transformer.   
**configuration**  | object  | N/A  | `transformer_configuration` object that describes the specific transformer implementation.   
**percent_loss_of_life**  | double  | %  | The percent amount of transformer's operational life used. If no initial value is given then the transformer is considered brand new.   
**top_oil_hot_spot_temperature**  | double  | Celsius  | The hot spot temperature of the top-oil in the transformer. Default initial value is the ambient temperature.   
**use_thermal_model**  | boolean  | N/A  | Flag used to enable use of the thermal/aging model. Default is FALSE.   
**winding_hot_spot_temperature**  | double  | Celsius  | The hot spot temperature of the transformer windings. Default initial value is the ambient temperature.   
  
### Transformer State of Development

Transformer is considered a highly developed and validated model in terms of powerflow solutions, however, models may be developed to include more advanced features in the future. Additionally, future work may include additional transformer configurations. 

