## Triplex Line Configuration

Triplex lines utilize their own configuration description method. Since the phases are no longer described as `A`, `B`, or `C`, the configuration is relabeled. A typical `triplex_line_configuration` is given as either a geometric configuration 
    
    
    object triplex_line_configuration {
    		conductor_1 trip_cond_H;
    		conductor_2 trip_cond_H;
    		conductor_N trip_cond_N;
    		insulation_thickness 0.08 in;
    		diameter 0.368 in;
    		}
    

or by using an explicit z-matrix 
    
    
    object triplex_line_configuration {
                   z11 1.52+0.61j;
                   z12 +0.55+0.44j;
                   z21 -0.55-0.44j;
                   z22 -1.52-0.61j;
                   }
    

Note: The explicit z-matrix version is an under-determined system. Ground and neutral currents will not be calculated, however, voltage and line currents will be correctly calculated. 

### Triplex Line Configuration Parameters

#### Properties

**triplex_line_configuration** does not declare inherited parent classes.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| conductor_1 | object | N/A | I | `triplex_conductor` object that represents the physical wire of phase 1. |
| conductor_2 | object | N/A | I | `triplex_conductor` object that represents the physical wire of phase 2. |
| conductor_N | object | N/A | I | `triplex_conductor` object that represents the physical wire of the neutral phase. |
| insulation_thickness | double | in | I | Thickness of the insulation around the phase 1 and phase 2 conductors |
| diameter | double | in | I | Diameter of the conductor |
| spacing | object | N/A | I | `line_spacing` object with information on the physical layout of the conductors. **This parameter is unused at this point. Future versions of GridLAB-D™ may implement this functionality** |
| z11 | complex | Ohm/mile | I | Phase 1 self-impedance, used for direct entry of impedance values |
| z12 | complex | Ohm/mile | I | Phase 1-2 induced impedance, used for direct entry of impedance values |
| z21 | complex | Ohm/mile | I | Phase 2-1 induced impedance, used for direct entry of impedance values |
| z22 | complex | Ohm/mile | I | Phase 2 self-impedance, used for direct entry of impedance values |
| rating.summer.continuous | double | A | I | Amp rating in summer, continuous |
| rating.summer.emergency | double | A | I | Amp rating in summer, short term |
| rating.winter.continuous | double | A | I | Amp rating in winter, continuous |
| rating.winter.emergency | double | A | I | Amp rating in winter, short term |

### Triplex Line Configuration State of Development

Triplex Line Configuration is considered a highly developed and validated model. 
