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

Property Name  | Type  | Unit  | Description   
---|---|---|---  
**conductor_1**  | object  | N/A  | `triplex_conductor` object that represents the physical wire of phase 1.   
**conductor_2**  | object  | N/A  | `triplex_conductor` object that represents the physical wire of phase 2.   
**conductor_N**  | object  | N/A  | `triplex_conductor` object that represents the physical wire of the neutral phase.   
**insulation_thickness**  | double  | inches  | Thickness of the insulation around the phase 1 and phase 2 conductors   
**diameter**  | double  | inches  | Diameter of the conductor   
**spacing**  | object  | N/A  | `line_spacing` object with information on the physical layout of the conductors. **This parameter is unused at this point. Future versions of GridLAB-D™ may implement this functionality**  
**z11-z22**  | complex  | Ohm/mile  | Describes the z-matrix explicitly as opposed to using geometric configurations. Using this will over-write the geometric configurations.   
  
### Triplex Line Configuration State of Development

Triplex Line Configuration is considered a highly developed and validated model. 

