## Overhead Line

Overhead lines are one of three specific line types incorporated into the **powerflow** distribution-level module. The `overhead_line` object will take spacing and conductor parameters and translate those values to appropriate impedance matrices based for the specific overhead transmission line configuration. A typical overhead line would be written as 
    
    
    object overhead_line{
    	phases "ABCN";
    	name 701-802;
    	from node_701;
    	to load_802;
    	length 125960;
    	configuration line_config_A;
    	}
    

`overhead_line` objects are based around the **link** object and inherit all of its properties. `overhead_line` objects primarily translate the `configuration` options specified into a circuit equivalent, so not further properties than those provided by **link** are required. 

### Overhead Line State of Development

Overhead Line is considered a highly developed and validated model in terms of powerflow solutions, however, models may be developed to include more advanced features in the future. 

