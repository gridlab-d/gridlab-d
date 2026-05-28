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

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| outer_diameter | double | in | I | Diameter of the outside of the cable, including jacketing and shielding. |
| conductor_gmr | double | ft | I | Geometric mean radius of the conductor at the center of the concentric cable. |
| conductor_diameter | double | in | I | Diameter of the conductor at the center of the concentric cable. |
| conductor_resistance | double | Ohm/mile | I | Resistance of the conductor at the center of the concentric cable. |
| neutral_gmr | double | ft | I | Geometric mean radius of the concentric neutral of the cable. |
| neutral_diameter | double | in | I | Diameter of the concentric neutral of the cable. |
| neutral_resistance | double | Ohm/mile | I | Resistance of the concentric neutral of the cable. |
| neutral_strands | int16 | N/A | I | Number of strands composing the concentric neutral conductor. |
| shield_thickness | double | in | I | The thickness of Tape shield in inches |
| shield_diameter | double | in | I | The outside diameter of Tape shield in inches |
| insulation_relative_permitivitty | double | unit | I | Permitivitty of insulation, relative to air |
| shield_gmr | double | ft | I | Geometric mean radius of the shielding of the cable. |
| shield_resistance | double | Ohm/mile | I | Resistance of the cable shielding. |
| rating.summer.continuous | double | A | I | The continuous rating for the conductor during summer month usage. **This parameter is unused at this point. Future versions of GridLAB-D™ may implement this functionality** |
| rating.summer.emergency | double | A | I | The emergency (short time) rating for the conductor during summer month usage. **This parameter is unused at this point. Future versions of GridLAB-D™ may implement this functionality** |
| rating.winter.continuous | double | A | I | The continuous rating for the conductor during winter month usage. **This parameter is unused at this point. Future versions of GridLAB-D™ may implement this functionality** |
| rating.winter.emergency | double | A | I | The emergency (short time) rating for the conductor during winter month usage. **This parameter is unused at this point. Future versions of GridLAB-D™ may implement this functionality** |

### Underground Line Conductor State of Development

Underground Line Conductor is considered a highly developed and validated model. 
