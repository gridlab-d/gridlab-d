## Line

The line object represents power lines in a distribution system. The line object has two implementations: `overhead_line`, and `underground_line`. Each line must be called appropriately. Information about the particular line type will be contained in other objects called `line_configuration`. 

**line**-based objects inherit properties from the **link** object just covered. Two new properties are also added: `configuration` and `length`. 

Typical usage of an overhead line would be 
    
    
    object overhead_line {
    	name Node1toNode2;
    	phases ABC;
    	from Node1;
    	to Node2;
    	length 5280;
    	configuration Best_overhead_line_cfg;
    	}
    

and the typical usage of the underground line would be 
    
    
    object underground_line {
    	name Node1toNode2;
    	phases ABC;
    	from Node1;
    	to Node2;
    	length 5280;
    	configuration An_underground_line_cfg;
    	}
    

### Line Parameters

Along with the inherited **link** properties, **line** objects have: 

Property Name  | Type  | Unit  | Description   
---|---|---|---  
**length**  | double  | feet  | Length of the **line** object.   
**configuration**  | object  | N/A  | Name or reference to the particular configuration object **that** describes the properties of the **line** object.   
  
