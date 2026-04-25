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

Input indicates properties you can set in models. Output marks properties produced or modified during simulation runtime.

| Property Name | Type | Unit | Input | Output | Description |
| --- | --- | --- | --- | --- | --- |
| geometric_mean_radius | double | ft | ✓ |  | The GMR of the wire. |
| resistance | double | Ohm/mile | ✓ |  | The resistance of the particular conductor, incorporating size and material effects. |
| diameter | double | in | ✓ |  | Diameter of the conductor - used for capacitance calculations. |
| rating.summer.continuous | double | A | ✓ |  | The continuous rating for the conductor during summer month usage. **This parameter is unused at this point. Future versions of GridLAB-D™ may implement this functionality** |
| rating.summer.emergency | double | A | ✓ |  | The emergency (short time) rating for the conductor during summer month usage. **TODO - Status - This parameter is unused at this point. Future versions of GridLAB-D™ may implement this functionality** |
| rating.winter.continuous | double | A | ✓ |  | The continuous rating for the conductor during winter month usage. **This parameter is unused at this point. Future versions of GridLAB-D™ may implement this functionality** |
| rating.winter.emergency | double | A | ✓ |  | The emergency (short time) rating for the conductor during winter month usage. **This parameter is unused at this point. Future versions of GridLAB-D™ may implement this functionality** |

### Overhead Line Conductor State of Development

Overhead Line Conductor is considered a highly developed and validated model. 
