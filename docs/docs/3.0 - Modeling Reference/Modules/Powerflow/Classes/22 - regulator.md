## Regulator

Regulators are essentially tap-changing transformers that attempt to maintain a voltage level at a specified point in the system. Regulators are one of two objects in the **powerflow** module that incorporate a form of automatic control. To take full advantage of this functionality, simulations of greater than one time step (time-varying simulations) are recommended. Similar to **transformer** and **line** objects, regulators require a **regulator_configuration** to determine many of their operating parameters. 

A typical implementation would be 
    
    object regulator {
    	name Reg799781;
    	phases "ABC";
    	from node_799;
    	to node_781;
    	configuration reg_conf_79978101;
    	}
    

### Regulator Parameters

A **regulator** object is a derived class from **link** objects. Therefore, all of the parameters available to the **link** object apply here as well. 

Property Name  | Type  | Unit  | Description   
---|---|---|---  
**configuration**  | object  | N/A  | **regulator_configuration** object that describes the specific regulator implementation.   
**tap_A**  | int16  | N/A  | Position of the tap on phase `A` of a wye-connected or phase `AB` of a delta-connected system. This parameter is most useful to be read in automatic regulator modes, but serves as the input for tap position of the phase under the manual control scheme.   
**tap_B**  | int16  | N/A  | Position of the tap on phase `B` of a wye-connected or phase `BC` of a delta-connected system. This parameter is most useful to be read in automatic regulator modes, but serves as the input for tap position of the phase under the manual control scheme.   
**tap_C**  | int16  | N/A  | Position of the tap on phase `C` of a wye-connected or phase `CA` of a delta-connected system. This parameter is most useful to be read in automatic regulator modes, but serves as the input for tap position of the phase under the manual control scheme.   
**sense_node**  | object  | N/A  | Remote node for the automatic control method to monitor. Only utilized in `REMOTE_NODE` control scheme. This must be a **node**-based object to work properly.   
**tap_A_change_count**  | int16  | N/A  | Holds the number of times the tap position on phase `A` of a wye-connected or phase `AB` of a delta-connected system has changed.   
**tap_B_change_count**  | int16  | N/A  | Holds the number of times the tap position on phase `B` of a wye-connected or phase `BC` of a delta-connected system has changed.   
**tap_C_change_count**  | int16  | N/A  | Holds the number of times the tap position on phase `C` of a wye-connected or phase `CA` of a delta-connected system has changed.   
  
### Regulator State of Development

Regulator is considered a well developed and validated model in terms of powerflow solutions, however, models may be developed to include more advanced features in the future. Additional configurations, controls, and/or losses may be included as needed. 

