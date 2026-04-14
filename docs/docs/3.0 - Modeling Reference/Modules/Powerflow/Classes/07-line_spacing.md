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

Input indicates properties you can set in models. Output marks properties produced or modified during simulation runtime.

| Property Name | Type | Unit | Input | Output | Description |
| --- | --- | --- | --- | --- | --- |
| distance_AB | double | ft | ✓ |  | Distance between conductors of phase A and phase B. |
| distance_BC | double | ft | ✓ |  | Distance between conductors of phase B and phase C. |
| distance_AC | double | ft | ✓ |  | Distance between conductors of phase C and phase A. |
| distance_AN | double | ft | ✓ |  | Distance between conductors of phase A and the neutral phase. |
| distance_BN | double | ft | ✓ |  | Distance between conductors of phase B and the neutral phase. |
| distance_CN | double | ft | ✓ |  | Distance between conductors of phase C and the neutral phase. |
| distance_AE | double | ft | ✓ |  | Distance between conductor of phase A and the earth (ground). |
| distance_BE | double | ft | ✓ |  | Distance between conductor of phase B and the earth (ground). |
| distance_CE | double | ft | ✓ |  | Distance between conductor of phase C and the earth (ground). |
| distance_NE | double | ft | ✓ |  | Distance between conductor of the neutral phase (phase N) and the earth (ground). |

### Line Spacing State of Development

Line Spacing is considered a highly developed and validated model in terms of powerflow solutions, however, models may be developed to include more advanced features in the future. 
