## Triplex Line Conductor

As with the `underground_line` and `overhead_line` objects, `triplex_line` objects have their own conductor objects. This object describes the physical characteristics of the actual wire used in the triple line bundle. A typical implementation would be: 
    
    
    object triplex_line_conductor {
    	name trip_cond_1;
    	resistance 0.97;
    	geometric_mean_radius 0.0111;		
    	}

### Triplex Line Conductor Parameters

#### Properties

**triplex_line_conductor** does not declare inherited parent classes.

Input indicates properties you can set in models. Output marks properties produced or modified during simulation runtime.

| Property Name | Type | Unit | Input | Output | Description |
| --- | --- | --- | --- | --- | --- |
| resistance | double | Ohm/mile | ✓ |  | Resistance of the conductor. |
| geometric_mean_radius | double | ft | ✓ |  | GMR of conductor. |
| rating.summer.continuous | double | A | ✓ |  | ⚠️ amp ratings for the cable during continuous operation in summer |
| rating.summer.emergency | double | A | ✓ |  | ⚠️ amp ratings for the cable during short term operation in summer |
| rating.winter.continuous | double | A | ✓ |  | ⚠️ amp ratings for the cable during continuous operation in winter |
| rating.winter.emergency | double | A | ✓ |  | ⚠️ amp ratings for the cable during short term operation in winter |

### Triplex Conductor State of Development

Triplex Conductor is considered a highly developed and validated model. 
