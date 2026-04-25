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

**line_configuration** does not declare inherited parent classes.

Input indicates properties you can set in models. Output marks properties produced or modified during simulation runtime.

| Property Name | Type | Unit | Input | Output | Description |
| --- | --- | --- | --- | --- | --- |
| conductor_A | object | N/A | ✓ |  | Object describing the conductor of phase A in the overhead or underground line object. (`overhead_line_conductor` or `underground_line_conductor`) |
| conductor_B | object | N/A | ✓ |  | Object describing the conductor of phase B in the overhead or underground line object. (`overhead_line_conductor` or `underground_line_conductor`) |
| conductor_C | object | N/A | ✓ |  | Object describing the conductor of phase C in the overhead or underground line object. (`overhead_line_conductor` or `underground_line_conductor`) |
| conductor_N | object | N/A | ✓ |  | Object describing the conductor of phase N in the overhead or underground line object. (`overhead_line_conductor` or `underground_line_conductor`) |
| spacing | object | N/A | ✓ |  | `line_spacing` object describing how the conductors are physically oriented on the pole or in the bundle. |
| z11 | complex | Ohm/mile | ✓ |  |  |
| z12 | complex | Ohm/mile | ✓ |  |  |
| z13 | complex | Ohm/mile | ✓ |  |  |
| z21 | complex | Ohm/mile | ✓ |  |  |
| z22 | complex | Ohm/mile | ✓ |  |  |
| z23 | complex | Ohm/mile | ✓ |  |  |
| z31 | complex | Ohm/mile | ✓ |  |  |
| z32 | complex | Ohm/mile | ✓ |  |  |
| z33 | complex | Ohm/mile | ✓ |  |  |
| c11 | double | nF/mile | ✓ |  |  |
| c12 | double | nF/mile | ✓ |  |  |
| c13 | double | nF/mile | ✓ |  |  |
| c21 | double | nF/mile | ✓ |  |  |
| c22 | double | nF/mile | ✓ |  |  |
| c23 | double | nF/mile | ✓ |  |  |
| c31 | double | nF/mile | ✓ |  |  |
| c32 | double | nF/mile | ✓ |  |  |
| c33 | double | nF/mile | ✓ |  |  |
| rating.summer.continuous | double | A | ✓ |  | ⚠️ amp rating in summer, continuous |
| rating.summer.emergency | double | A | ✓ |  | ⚠️ amp rating in summer, short term |
| rating.winter.continuous | double | A | ✓ |  | ⚠️ amp rating in winter, continuous |
| rating.winter.emergency | double | A | ✓ |  | ⚠️ amp rating in winter, short term |

### Line configuration properties

Property Name  | Type  | Unit  | Description   
---|---|---|---  
**conductor_A**  | object  | N/A  | Object describing the conductor of phase A in the overhead or underground line object. (`overhead_line_conductor` or `underground_line_conductor`)   
**conductor_B**  | object  | N/A  | Object describing the conductor of phase B in the overhead or underground line object. (`overhead_line_conductor` or `underground_line_conductor`)   
**conductor_C**  | object  | N/A  | Object describing the conductor of phase C in the overhead or underground line object. (`overhead_line_conductor` or `underground_line_conductor`)   
**conductor_N**  | object  | N/A  | Object describing the conductor of phase N in the overhead or underground line object. (`overhead_line_conductor` or `underground_line_conductor`)   
**spacing**  | object  | N/A  | `line_spacing` object describing how the conductors are physically oriented on the pole or in the bundle.   
**z11-z33**  | complex  | Ohm/mile  | describes the z-matrix directly for either underground or overhead lines instead of using the geometric configurations (This will over-write geometric configurations). For this notation, index 1 is phase A, 2 is phase B, and 3 is phase C. So element z12 represents the mutual/cross coupling impedance between phase A and phase B of this line configuration.   
**c11-c33**  | double  | nF/mile  | describes the z-matrix directly for either underground or overhead lines instead of using the geometric configurations (This will over-write geometric configurations). For this notation, index 1 is phase A, 2 is phase B, and 3 is phase C. So element c12 represents the mutual/cross coupling capacitance between phase A and phase B of this line configuration. Unlike the zXX terms, this is raw capacitance and does not have the frequency factored into the calculation yet.   

### Line State of Development

Line is considered a highly developed and validated model in terms of powerflow solutions, however, models may be developed to include more advanced features in the future. 
