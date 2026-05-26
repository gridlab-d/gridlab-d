## Overhead Line Conductor

!!! warning
    This page was automatically generated and requires review.

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

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| geometric_mean_radius | double | ft | I | The GMR of the wire. |
| resistance | double | Ohm/mile | I | The resistance of the particular conductor, incorporating size and material effects. |
| diameter | double | in | I | Diameter of the conductor - used for capacitance calculations. |
| rating.summer.continuous | double | A | I | The continuous rating for the conductor during summer month usage. **This parameter is unused at this point. Future versions of GridLAB-D™ may implement this functionality** |
| rating.summer.emergency | double | A | I | The emergency (short time) rating for the conductor during summer month usage. **TODO - Status - This parameter is unused at this point. Future versions of GridLAB-D™ may implement this functionality** |
| rating.winter.continuous | double | A | I | The continuous rating for the conductor during winter month usage. **This parameter is unused at this point. Future versions of GridLAB-D™ may implement this functionality** |
| rating.winter.emergency | double | A | I | The emergency (short time) rating for the conductor during winter month usage. **This parameter is unused at this point. Future versions of GridLAB-D™ may implement this functionality** |

### Overhead Line Conductor State of Development

Overhead Line Conductor is considered a highly developed and validated model. 
