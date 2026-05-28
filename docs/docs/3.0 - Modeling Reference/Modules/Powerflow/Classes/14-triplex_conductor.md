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

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| resistance | double | Ohm/mile | I | Resistance of the conductor. |
| geometric_mean_radius | double | ft | I | GMR of conductor. |
| rating.summer.continuous | double | A | I | Amp ratings for the cable during continuous operation in summer |
| rating.summer.emergency | double | A | I | Amp ratings for the cable during short term operation in summer |
| rating.winter.continuous | double | A | I | Amp ratings for the cable during continuous operation in winter |
| rating.winter.emergency | double | A | I | Amp ratings for the cable during short term operation in winter |

### Triplex Conductor State of Development

Triplex Conductor is considered a highly developed and validated model. 
