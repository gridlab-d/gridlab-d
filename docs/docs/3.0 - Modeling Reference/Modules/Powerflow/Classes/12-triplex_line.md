## Triplex Line

!!! warning
    This page was automatically generated and requires review.

The third type of line available in the **powerflow** module is the triplex lines. Triplex lines represent the distribution wires coming from the transformer into a typical residential home. That is, they are typically composed of one neutral wire and two "hot" wires. Triplex lines require the phase `S` to be specified as part of the `phases` parameter for proper implementation. A typical triplex line would be implemented in a similar fashion to 
    
    
    object triplex_line {
    	phases AS;
    	length 100 ft;	
    	from node_4a;
    	to node_4;
    	configuration triplex_config_AB;
    	}
    

As with the `underground_line` and `overhead_line` objects, `triplex_line` objects inherit all of their properties from the **link** object. However, `triplex_line`s use a different configuration structure than the `overhead_line` and `underground_line` objects. 

### Triplex Line Parameters

### Triplex Line State of Development

Triplex Line is considered a highly developed and validated model in terms of powerflow solutions, however, models may be developed to include more advanced features in the future. 
