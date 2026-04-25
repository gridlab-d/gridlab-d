## Underground Line

Underground lines represent burial distribution cables in a powerflow system. In terms of GridLAB-D™ implementation, they are nearly identical to the `overhead_line` objects. A typical `underground_line` object would be written as 
    
    
    object underground_line {
    	phases "ABC";
    	name 703-727;
    	from node_703;
    	to load_827;
    	length 240;
    	configuration line_config_7241;
    	}
    

As with `overhead_line` objects, `underground_line` objects inherit all of their properties from the **link** object. The `underground_line` object again serves as a method to choose the appropriate translation algorithms to take the physical parameters of the system and create an equivalent model. As such, it has no new properties either. 

### Underground Line Parameters

### Underground Line State of Development

Underground Line is considered a highly developed and validated model in terms of powerflow solutions, however, models may be developed to include more advanced features in the future. 
