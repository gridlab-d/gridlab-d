## Node

The node object is equivalent to a bus of the distribution system. It provides a connection point for **link**-based objects and a point of known voltages on the system. Three phase voltage is typically available in either wye-connected or delta-connected three phase. Wye-connected voltages are contained in `voltage_A`, `voltage_B`, and `voltage_C`. Delta-connected voltages are available in `voltage_AB`, `voltage_BC`, and `voltage_CA`. 

### Default Node

A minimalist node could be created with 
    
    
    object node {
    	name NodeOne;
    	phases ABC;
    	nominal_voltage 7200.0;
    	}
    

which is the same as specifying 
    
    
    object node {
    	name NodeOne;
    	phases ABC;
    	nominal_voltage 7200.0;
    	voltage_A 7200.0+0d;
    	voltage_B 7200.0-120.0d;
    	voltage_C 7200.0+120.0d;
    	bustype PQ;
    	}
    

### Node Parameters

As with all powerflow objects, `phases` and `nominal_voltage` are inherently part of **node**. 

Property Name  | Type  | Unit  | Description   
---|---|---|---  
**voltage_A**  | complex  | Volts  | The voltage on phase `A` of a three-phase system. This may be specified in rectangular (7200.0+0.0j) or polar (7200.0+0.0d) formats.   
**voltage_B**  | complex  | Volts  | The voltage on phase `B` of a three-phase system. This may be specified in rectangular (7200.0+0.0j) or polar (7200.0+0.0d) formats.   
**voltage_C**  | complex  | Volts  | The voltage on phase `C` of a three-phase system. This may be specified in rectangular (7200.0+0.0j) or polar (7200.0+0.0d) formats.   
**voltage_AB**  | complex  | Volts  | The voltage on phase `AB` of a delta-connected three-phase system. This is a derived quantity and can be read, but it is not recommended you set this value.   
**voltage_BC**  | complex  | Volts  | The voltage on phase `BC` of a delta-connected three-phase system. This is a derived quantity and can be read, but it is not recommended you set this value.   
**voltage_CA**  | complex  | Volts  | The voltage on phase `CA` of a delta-connected three-phase system. This is a derived quantity and can be read, but it is not recommended you set this value.   
**current_A**  | complex  | Amperes  | The current load on phase `A` (wye) or phase `AB` (delta) of the node. This value is typically handled through the **load** object, so modification is not recommended here.   
**current_B**  | complex  | Amperes  | The current load on phase `B` (wye) or phase `BC` (delta) of the node. This value is typically handled through the **load** object, so modification is not recommended here.   
**current_C**  | complex  | Amperes  | The current load on phase `C` (wye) or phase `CA` (delta) of the node. This value is typically handled through the **load** object, so modification is not recommended here.   
**power_A**  | complex  | Volt-Amperes  | The power load on phase `A` (wye) or phase `AB` (delta) of the node. This value is typically handled through the **load** object, so modification is not recommended here.   
**power_B**  | complex  | Volt-Amperes  | The power load on phase `B` (wye) or phase `BC` (delta) of the node. This value is typically handled through the **load** object, so modification is not recommended here.   
**power_C**  | complex  | Volt-Amperes  | The power load on phase `C` (wye) or phase `CA` (delta) of the node. This value is typically handled through the **load** object, so modification is not recommended here.   
**shunt_A**  | complex  | Siemens (mhos)  | The shunt admittance load on phase `A` (wye) or phase `AB` (delta) of the node. This value is typically handled through the **load** object, so modification is not recommended here.   
**shunt_B**  | complex  | Siemens (mhos)  | The shunt admittance load on phase `B` (wye) or phase `BC` (delta) of the node. This value is typically handled through the **load** object, so modification is not recommended here.   
**shunt_C**  | complex  | Siemens (mhos)  | The shunt admittance load on phase `C` (wye) or phase `CA` (delta) of the node. This value is typically handled through the **load** object, so modification is not recommended here.   
**bustype**  | enumeration  | N/A  | The type of bus the node represents. The different bus distinctions are only valid for the Gauss-Seidel and Newton-Raphson solver methods. The Forward-Back Sweep method (Kersting's method) does not presently incorporate anything other than the `PQ` bus. Valid choices are <br/> - `PQ` for a constant power bus (default) <br/> - `PV` for a voltage-controlled (magnitude) bus <br/> - `SWING` for the infinite bus of a system. 
**maximum_voltage_error**  | double  | Volts  | The maximum voltage error for convergence checks in the different powerflow solvers. If left blank, it is derived from the `nominal_voltage` parameter.   
**busflags**  | enumeration  | N/A  | A flag to indicate if the current bus has a source or not. Mainly used for `PV` implementations. The only valid entries are `HASSOURCE` to indicate it is a supported bus, or an empty value indicating it is not. Unused at this time.   
**reference_bus**  | object  | N/A  | A reference node elsewhere in the system that the **node** will use to obtain frequency information if necessary (unimplemented in GridLAB-D™ at this point).   
**mean_repair_time**  | double  | seconds  | Time after a fault clears for the object to be considered back in service. Mainly used for **reliability** module interactions at this time.   
  
### Node State of Development

Node is considered a highly developed and validated model. 

