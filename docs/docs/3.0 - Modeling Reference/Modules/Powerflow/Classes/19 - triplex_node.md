## Triplex Node

Triplex nodes represent special cases of the **node** object. The **triplex_node** object still serves as connection point between different links of the system and a point of measurable voltage. However, **triplex_node**s are casted to represent phases `1`, `2`, and `N` rather than `A`, `B`, and `C` like normal **node** objects. Simplified, they operate in the split-phase level of distribution rather than the three-phase level. 

Since **load** objects are directly derived from **node** objects, they are only valid for three-phase connections as well. Therefore, the **load** functionality has been built into the **triplex_load** object for split-phase level systems. 

It is important to note that triplex-based objects should include the phase `S` somewhere in their designation. 

A typical **triplex_node** implementation is 
    
    
    object triplex_node {
    	name TPL_tAS;
    	phases AS;
    	voltage_1 120 + 0j;		
    	voltage_2 120 + 0j;
    	voltage_N 0;
    	current_1  1.0;
    	power_1 1000+2000j;	
    	shunt_1 5.3333e-004 -2.6667e-004i;	
    	nominal_voltage 120;
    	};
    

### Triplex Node Parameters

**triplex_node** objects are technically derived from **node** objects as well. However due to the triplex nature of their use and the particular implementation, the normal **node** parameters are not available for use. 

Property Name  | Type  | Unit  | Description   
---|---|---|---  
**bustype**  | enumeration  | N/A  | The type of bus the node represents. The different bus distinctions are only valid for the Gauss-Seidel and Newton-Raphson solver methods. The Forward-Back Sweep method (Kersting's method) does not presently incorporate anything other than the `PQ` bus. Valid choices are <br/> - `PQ` for a constant power bus (default) <br/> - `PV` for a voltage-controlled (magnitude) bus <br/> - `SWING` for the infinite bus of a system.  
**busflags**  | enumeration  | N/A  | A flag to indicate if the current bus has a source or not. Mainly used for `PV` implementations. The only valid entries are `HASSOURCE` to indicate it is a supported bus, or an empty value indicating it is not.   
**reference_bus**  | object  | N/A  | A reference node elsewhere in the system that the **triplex_node** will use to obtain frequency information if necessary (unimplemented in GridLAB-D™ at this point).   
maximum_voltage_error  | double  | Volts  | The maximum voltage error for convergence checks in the different powerflow solvers. If left blank, it is derived from the `nominal_voltage` parameter.   
**voltage_1**  | complex  | Volts  | The voltage on phase 1 of a split-phase or triplex system. This may be specified in rectangular (7200.0+0.0j) or polar (7200.0+0.0d) formats.   
**voltage_2**  | complex  | Volts  | The voltage on phase 2 of a split-phase or triplex system. This may be specified in rectangular (7200.0+0.0j) or polar (7200.0+0.0d) formats.   
**voltage_N**  | complex  | Volts  | The voltage on the neutral phase of a split-phase or triplex system. This may be specified in rectangular (7200.0+0.0j) or polar (7200.0+0.0d) formats.   
**voltage_12**  | complex  | Volts  | The voltage between phases `1` and `2` of the split-phase or triplex system. This is a derived quantity and can be read, but it is not recommended you set this value.   
**voltage_1N**  | complex  | Volts  | The voltage between phases `1` and `N` of the split-phase or triplex system. This is a derived quantity and can be read, but it is not recommended you set this value.   
**voltage_2N**  | complex  | Volts  | The voltage between phases `2` and `N` of the split-phase or triplex system. This is a derived quantity and can be read, but it is not recommended you set this value.   
**current_1**  | complex  | Amperes  | Constant current load on phase `1` of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here.   
**current_2**  | complex  | Amperes  | Constant current load on phase `2` of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here.   
**current_N**  | complex  | Amperes  | Constant current load on the neutral phase of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here.   
**current_12**  | complex  | Amperes  | Constant current load on across phases `1` and `2` of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here.   
**power_1**  | complex  | Volt-Amperes  | Constant power load on phase `1` of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here.   
**power_2**  | complex  | Volt-Amperes  | Constant power load on phase `2` of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here.   
**power_12**  | complex  | Volt-Amperes  | Constant power load across phases `1` and `2` of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here.   
**shunt_1**  | complex  | Siemens (mhos)  | Constant admittance load on phase `1` of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here.   
**shunt_2**  | complex  | Siemens (mhos)  | Constant admittance load on phase `2` of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here.   
**shunt_12**  | complex  | Siemens (mhos)  | Constant admittance load across phases `1` and `2` of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here.   
**impedance_1**  | complex  | Ohms  | Constant impedance load on phase `1` of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here.   
**impedance_2**  | complex  | Ohms  | Constant impedance load on phase `2` of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here.   
**impedance_12**  | complex  | Ohms  | Constant impedance load across phases `1` and `2` of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here.   
  
### Triplex Node State of Development

Triplex Node is considered a highly developed and validated model in terms of powerflow solutions, however, models may be developed to include more advanced features in the future. 

