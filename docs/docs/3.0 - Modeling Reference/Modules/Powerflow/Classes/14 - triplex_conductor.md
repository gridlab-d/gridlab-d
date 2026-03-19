## Triplex Conductor

As with the `underground_line` and `overhead_line` objects, `triplex_line` objects have their own conductor objects. This object describes the physical characteristics of the actual wire used in the triple line bundle. A typical implementation would be: 
    
    
    object triplex_line_conductor {
    	name trip_cond_1;
    	resistance 0.97;
    	geometric_mean_radius 0.0111;		
    	}
    

### Triplex Conductor Parameters

Property Name  | Type  | Unit  | Description   
---|---|---|---  
**resistance**  | double  | Ohm/mile  | Resistance of the conductor.   
**geometric_mean_radius**  | double  | feet  | GMR of conductor.   
  
### Triplex Conductor State of Development

Triplex Conductor is considered a highly developed and validated model. 

