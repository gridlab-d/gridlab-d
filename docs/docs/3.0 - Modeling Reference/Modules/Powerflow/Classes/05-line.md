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

#### Properties

**line** objects are derived from **[link](04-link.md)** objects, so any parameters of the **[link](04-link.md)** object are available as well.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

Table: line table 1 { #tbl:05-line-1 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| **configuration** | object | N/A | I | Name or reference to the particular configuration object **that** describes the properties of the **line** object. |
| **length** | double | ft | I | Length of the **line** object. |

### Line State of Development

Line is considered a highly developed and validated model in terms of powerflow solutions, however, models may be developed to include more advanced features in the future. 

## Line Configuration

Both `underground_line` and `overhead_line` objects take line configuration information to describe the particular line being implemented, or they can be described in their raw z-matrix values. A typical `line_configuration` object would be implemented as 
    
    
    object line_configuration {
    	name line_config_A;
    	conductor_A overhead_line_conductor_100;
    	conductor_B overhead_line_conductor_100;
    	conductor_C overhead_line_conductor_100;
    	conductor_N overhead_line_conductor_101;
    	spacing line_spacing_200;
    	}
    

or 
    
    
    object line_configuration {
           name line_config_B;
           z11 0.45+1.07j;
           z12 0.15+0.50j;
           z13 0.15+0.38j;
           z21 0.15+0.50j;
           z22 0.46+1.04j;
           z23 0.15+0.42j;
           z31 0.15+0.38j;
           z32 0.15+0.42j;
           z33 0.46+1.06j;
           }
    

If you want to factor in line capacitance effects, the `line_configuration` can be extended to: 
    
    
    object line_configuration {
           name line_config_B;
           z11 0.45+1.07j;
           z12 0.15+0.50j;
           z13 0.15+0.38j;
           z21 0.15+0.50j;
           z22 0.46+1.04j;
           z23 0.15+0.42j;
           z31 0.15+0.38j;
           z32 0.15+0.42j;
           z33 0.46+1.06j;
           c11 198.52;
           c22 198.52;
           c33 198.52;
           }
    

Note that for capacitance calculations to be included in the powerflow, the module-level directive must be included: 
    
    
    module powerflow {
       line_capacitance true;
    }

It is highly recommended to use the `line_spacing` and `overhead_line_conductor` or `underground_line_conductor` objects and let the internal equations calculate the capacitance (and impedance) for the user. 

### Line Configuration Parameters

#### Properties

**line_configuration** does not declare inherited parent classes.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

Table: line table 2 { #tbl:05-line-2 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| **conductor_A** | object | N/A | I | Object describing the conductor of phase A in the overhead or underground line object. (`overhead_line_conductor` or `underground_line_conductor`) |
| **conductor_B** | object | N/A | I | Object describing the conductor of phase B in the overhead or underground line object. (`overhead_line_conductor` or `underground_line_conductor`) |
| **conductor_C** | object | N/A | I | Object describing the conductor of phase C in the overhead or underground line object. (`overhead_line_conductor` or `underground_line_conductor`) |
| **conductor_N** | object | N/A | I | Object describing the conductor of phase N in the overhead or underground line object. (`overhead_line_conductor` or `underground_line_conductor`) |
| **spacing** | object | N/A | I | `line_spacing` object describing how the conductors are physically oriented on the pole or in the bundle. |


#### Z-matrix and Capacitance Matrix Properties

Describes the z-matrix and c-matrix directly for either underground or overhead lines instead of using the geometric configurations (This will over-write geometric configurations). For this notation, index 1 is phase A, 2 is phase B, and 3 is phase C. So element z12 represents the mutual/cross coupling impedance between phase A and phase B of this line configuration.

Table: line table 3 { #tbl:05-line-3 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| **z11** | complex | Ohm/mile | I |  |
| **z12** | complex | Ohm/mile | I |  |
| **z13** | complex | Ohm/mile | I |  |
| **z21** | complex | Ohm/mile | I |  |
| **z22** | complex | Ohm/mile | I |  |
| **z23** | complex | Ohm/mile | I |  |
| **z31** | complex | Ohm/mile | I |  |
| **z32** | complex | Ohm/mile | I |  |
| **z33** | complex | Ohm/mile | I |  |
| **c11** | double | nF/mile | I |  |
| **c12** | double | nF/mile | I |  |
| **c13** | double | nF/mile | I |  |
| **c21** | double | nF/mile | I |  |
| **c22** | double | nF/mile | I |  |
| **c23** | double | nF/mile | I |  |
| **c31** | double | nF/mile | I |  |
| **c32** | double | nF/mile | I |  |
| **c33** | double | nF/mile | I |  |

#### Ampere Rating Properties

Table: line table 4 { #tbl:05-line-4 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| **rating.summer.continuous** | double | A | I | Amp rating in summer, continuous |
| **rating.summer.emergency** | double | A | I | Amp rating in summer, short term |
| **rating.winter.continuous** | double | A | I | Amp rating in winter, continuous |
| **rating.winter.emergency** | double | A | I | Amp rating in winter, short term |




## Line Spacing

The line spacing object describe how the individual conductors of a distribution line are arranged underground or on the support pole. A typical implementation of a `line_spacing` object is 
    
    
    object line_spacing {
    	name line_spacing_200;
    	distance_AB 2.5;
    	distance_BC 4.5;
    	distance_AC 7.0;
    	distance_AN 5.656854;
    	distance_BN 4.272002;
    	distance_CN 5.0;
    	}

### Line Spacing Parameters

#### Properties

**line_spacing** does not declare inherited parent classes.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

Table: line table 5 { #tbl:05-line-5 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| **distance_AB** | double | ft | I | Distance between conductors of phase A and phase B. |
| **distance_BC** | double | ft | I | Distance between conductors of phase B and phase C. |
| **distance_AC** | double | ft | I | Distance between conductors of phase C and phase A. |
| **distance_AN** | double | ft | I | Distance between conductors of phase A and the neutral phase. |
| **distance_BN** | double | ft | I | Distance between conductors of phase B and the neutral phase. |
| **distance_CN** | double | ft | I | Distance between conductors of phase C and the neutral phase. |
| **distance_AE** | double | ft | I | Distance between conductor of phase A and the earth (ground). |
| **distance_BE** | double | ft | I | Distance between conductor of phase B and the earth (ground). |
| **distance_CE** | double | ft | I | Distance between conductor of phase C and the earth (ground). |
| **distance_NE** | double | ft | I | Distance between conductor of the neutral phase (phase N) and the earth (ground). |

### Line Spacing State of Development

Line Spacing is considered a highly developed and validated model in terms of powerflow solutions, however, models may be developed to include more advanced features in the future. 


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
    

`overhead_line` objects inherit from the **line** object and inherit all of its properties. `overhead_line` objects primarily translate the `configuration` options specified into a circuit equivalent, so not further properties than those provided by **line** are required. 


### Overhead Line State of Development

Overhead Line is considered a highly developed and validated model in terms of powerflow solutions, however, models may be developed to include more advanced features in the future. 


## Overhead Line Conductor

For overhead lines, the `line_configuration` object must specify the overhead line conductor types used in the particular setup. A typical `overhead_line_conductor` would be implemented as 
    
    
    object overhead_line_conductor {
    	name overhead_line_conductor_100;
    	geometric_mean_radius .00446;
    	resistance 1.12;
    	}

### Overhead Line Conductor Parameters

#### Properties

**overhead_line_conductor** does not declare inherited parent classes.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

Table: line table 6 { #tbl:05-line-6 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| **geometric_mean_radius** | double | ft | I | The GMR of the wire. |
| **resistance** | double | Ohm/mile | I | The resistance of the particular conductor, incorporating size and material effects. |
| **diameter** | double | in | I | Diameter of the conductor - used for capacitance calculations. |
| **rating.summer.continuous** | double | A | I | The continuous rating for the conductor during summer month usage. **This parameter is unused at this point. Future versions of GridLAB-Dâ„¢ may implement this functionality** |
| **rating.summer.emergency** | double | A | I | The emergency (short time) rating for the conductor during summer month usage. **TODO - Status - This parameter is unused at this point. Future versions of GridLAB-Dâ„¢ may implement this functionality** |
| **rating.winter.continuous** | double | A | I | The continuous rating for the conductor during winter month usage. **This parameter is unused at this point. Future versions of GridLAB-Dâ„¢ may implement this functionality** |
| **rating.winter.emergency** | double | A | I | The emergency (short time) rating for the conductor during winter month usage. **This parameter is unused at this point. Future versions of GridLAB-Dâ„¢ may implement this functionality** |

### Overhead Line Conductor State of Development

Overhead Line Conductor is considered a highly developed and validated model. 


## Underground Line

Underground lines represent burial distribution cables in a powerflow system. In terms of GridLAB-Dâ„¢ implementation, they are nearly identical to the `overhead_line` objects. A typical `underground_line` object would be written as 
    
    
    object underground_line {
    	phases "ABC";
    	name 703-727;
    	from node_703;
    	to load_827;
    	length 240;
    	configuration line_config_7241;
    	}
    

As with `overhead_line` objects, `underground_line` objects inherit all of their properties from the **line** object. The `underground_line` object again serves as a method to choose the appropriate translation algorithms to take the physical parameters of the system and create an equivalent model. As such, it has no new properties either. 

### Underground Line State of Development

Underground Line is considered a highly developed and validated model in terms of powerflow solutions, however, models may be developed to include more advanced features in the future. 


## Underground Line Conductor

Underground lines often contain concentric shielding layers around the central conductor. As a result, they require more parameters than the `overhead_line_conductor` objects to fully describe them. A typical `underground_line_object` is: 
    
    
    object underground_line_conductor { 
    	name ug_conduct_7210;
    	outer_diameter 1.980000;
    	conductor_gmr 0.036800;
    	conductor_diameter 1.150000;
    	conductor_resistance 0.105000;
    	neutral_gmr 0.003310;
    	neutral_resistance 5.903000;
    	neutral_diameter 0.102000;
    	neutral_strands 20.000000;
    	shield_gmr 0.000000;
    	shield_resistance 0.000000;
    	}

### Underground Line Conductor Parameters

#### Properties

**underground_line_conductor** does not declare inherited parent classes.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

Table: line table 7 { #tbl:05-line-7 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| **outer_diameter** | double | in | I | Diameter of the outside of the cable, including jacketing and shielding. |
| **conductor_gmr** | double | ft | I | Geometric mean radius of the conductor at the center of the concentric cable. |
| **conductor_diameter** | double | in | I | Diameter of the conductor at the center of the concentric cable. |
| **conductor_resistance** | double | Ohm/mile | I | Resistance of the conductor at the center of the concentric cable. |
| **neutral_gmr** | double | ft | I | Geometric mean radius of the concentric neutral of the cable. |
| **neutral_diameter** | double | in | I | Diameter of the concentric neutral of the cable. |
| **neutral_resistance** | double | Ohm/mile | I | Resistance of the concentric neutral of the cable. |
| **neutral_strands** | int16 | N/A | I | Number of strands composing the concentric neutral conductor. |
| **shield_thickness** | double | in | I | The thickness of Tape shield in inches |
| **shield_diameter** | double | in | I | The outside diameter of Tape shield in inches |
| **insulation_relative_permitivitty** | double | unit | I | Permitivitty of insulation, relative to air |
| **shield_gmr** | double | ft | I | Geometric mean radius of the shielding of the cable. |
| **shield_resistance** | double | Ohm/mile | I | Resistance of the cable shielding. |
| **rating.summer.continuous** | double | A | I | The continuous rating for the conductor during summer month usage. **This parameter is unused at this point. Future versions of GridLAB-Dâ„¢ may implement this functionality** |
| **rating.summer.emergency** | double | A | I | The emergency (short time) rating for the conductor during summer month usage. **This parameter is unused at this point. Future versions of GridLAB-Dâ„¢ may implement this functionality** |
| **rating.winter.continuous** | double | A | I | The continuous rating for the conductor during winter month usage. **This parameter is unused at this point. Future versions of GridLAB-Dâ„¢ may implement this functionality** |
| **rating.winter.emergency** | double | A | I | The emergency (short time) rating for the conductor during winter month usage. **This parameter is unused at this point. Future versions of GridLAB-Dâ„¢ may implement this functionality** |

### Underground Line Conductor State of Development

Underground Line Conductor is considered a highly developed and validated model. 
