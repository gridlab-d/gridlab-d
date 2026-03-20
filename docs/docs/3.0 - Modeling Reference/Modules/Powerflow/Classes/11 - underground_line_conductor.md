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

Property Name  | Type  | Unit  | Description   
---|---|---|---  
**outer_diameter**  | double  | inches  | Diameter of the outside of the cable, including jacketing and shielding.   
**conductor_gmr**  | double  | feet  | Geometric mean radius of the conductor at the center of the concentric cable.   
**conductor_diameter**  | double  | inches  | Diameter of the conductor at the center of the concentric cable.   
**conductor_resistance**  | double  | Ohm/mile  | Resistance of the conductor at the center of the concentric cable.   
**neutral_gmr**  | double  | feet  | Geometric mean radius of the concentric neutral of the cable.   
**neutral_diameter**  | double  | inches  | Diameter of the concentric neutral of the cable.   
**neutral_resistance**  | double  | Ohm/mile  | Resistance of the concentric neutral of the cable.   
**neutral_strands**  | integer  | N/A  | Number of strands composing the concentric neutral conductor.   
**insultation_relative_permitivitty**  | double  | N/A (scalar)  | Relative permitivitty of the insulation in a concentric neutral cable - relative to air - used for capacitance calculations.   
**shield_gmr**  | double  | feet  | Geometric mean radius of the shielding of the cable.   
shield_resistance  | double  | Ohm/mile  | Resistance of the cable shielding.   
**rating.summer.continuous**  | double  | Amperes  | The continuous rating for the conductor during summer month usage. **This parameter is unused at this point. Future versions of GridLAB-D™ may implement this functionality**  
**rating.summer.emergency**  | double  | Amperes  | The emergency (short time) rating for the conductor during summer month usage. **This parameter is unused at this point. Future versions of GridLAB-D™ may implement this functionality**  
**rating.winter.continuous**  | double  | Amperes  | The continuous rating for the conductor during winter month usage. **This parameter is unused at this point. Future versions of GridLAB-D™ may implement this functionality**  
**rating.winter.emergency**  | double  | Amperes  | The emergency (short time) rating for the conductor during winter month usage. **This parameter is unused at this point. Future versions of GridLAB-D™ may implement this functionality**  
  
### Underground Line Conductor State of Development

Underground Line Conductor is considered a highly developed and validated model. 

