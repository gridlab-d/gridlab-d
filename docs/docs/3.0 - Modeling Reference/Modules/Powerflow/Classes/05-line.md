## Line

!!! warning
    This page was automatically generated and requires review.

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

#### Properties

**line** objects are derived from **[link](04-link.md)** objects, so any parameters of the **[link](04-link.md)** object are available as well.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| configuration | object | N/A | I | Name or reference to the particular configuration object **that** describes the properties of the **line** object. |
| length | double | ft | I | Length of the **line** object. |
