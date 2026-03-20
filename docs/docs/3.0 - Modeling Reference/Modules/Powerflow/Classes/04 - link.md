## Link

The link object is a connection between nodes in a distribution system. The **link** object is not directly useful, but is the basis for objects associated with overhead lines, underground lines, triplex lines, transformers, regulators, switches, and fuses. 

### Default Link

A **link** only requires three parameters to be specified by default. Most of the actual functionality comes through other objects. 
    
    
    object link {
    	name Node1toNode2;
    	phases ABC;
    	from Node1;
    	to Node2;
    	}
    

### Link Parameters

Again, as with all powerflow objects, `phases` and `nominal_voltage` are inherently part of **link**. `nominal_voltage` does not need to be specified for **link** objects. 

Property Name  | Type  | Unit  | Description   
---|---|---|---  
**from**  | object  | N/A  | One connecting end of the **link** object. This will be the name or reference to a **node**-based object elsewhere in the powerflow model.   
**to**  | object  | N/A  | The other connecting end of the **link** object. This will be the name or reference to a **node**-based object elsewhere in the powerflow model.   
**power_in**  | complex  | Volt-Amperes  | The calculated power flowing into the particular **link** object as a sum of all three phases.   
**power_out**  | complex  | Volt-Amperes  | The calculated power flowing out of the particular **link** object as a sum of all three phases.   
**power_losses**  | complex  | Volt-Amperes  | The calculated power loss for all three phases between the input and output of the **link**..   
**power_in_A**  | complex  | Volt-Amperes  | The calculated power on phase A flowing into the **link**.   
**power_in_B**  | complex  | Volt-Amperes  | The calculated power on phase B flowing into the **link**.   
**power_in_C**  | complex  | Volt-Amperes  | The calculated power on phase C flowing into the **link**.   
**power_out_A**  | complex  | Volt-Amperes  | The calculated power on phase A flowing out of the **link**.   
**power_out_B**  | complex  | Volt-Amperes  | The calculated power on phase B flowing out of the **link**.   
**power_out_C**  | complex  | Volt-Amperes  | The calculated power on phase C flowing out of the **link**.   
**power_losses_A**  | complex  | Volt-Amperes  | The calculated power loss between the input and output of the **link** on phase A.   
**power_losses_B**  | complex  | Volt-Amperes  | The calculated power loss between the input and output of the **link** on phase B.   
**power_losses_C**  | complex  | Volt-Amperes  | The calculated power loss between the input and output of the **link** on phase C.   
**status**  | enumeration  | N/A  | Status of the line in terms of being `OPEN` or `CLOSED`. This property is mainly used for switches and fuses, but may be used to remove lines from service. This functionality is primarily used in the `FBS` solver mode.   
**current_out_A**  | complex  | Amperes  | The calculated current flowing out of the link object on phase `A`. Note: This has not been fully tested for every object.   
**current_out_B**  | complex  | Amperes  | The calculated current flowing out of the link object on phase `B`. Note: This has not been fully tested for every object.   
**current_out_C**  | complex  | Amperes  | The calculated current flowing out of the link object on phase `C`. Note: This has not been fully tested for every object.   
**current_in_A**  | complex  | Amperes  | The calculated current flowing into the link object on phase `A`. Note: This has not been fully tested for every object.   
**current_in_B**  | complex  | Amperes  | The calculated current flowing into the link object on phase `B`. Note: This has not been fully tested for every object.   
**current_in_C**  | complex  | Amperes  | The calculated current flowing into the link object on phase `C`. Note: This has not been fully tested for every object.   
**flow_direction**  | set  | N/A  | This is a flag telling which direction current is flowing, relative to the `to` and `from` designations, on a each phase of a link object. <br/>    0x000 - `UNKNOWN` \- The flow direction is indeterminate. <br/>  0x001 - `AF` \- Current is flowing from the `from` node to the `to` node on phase `A`. <br/>  0x002 - `AR` \- Current is flowing from the `to` node to the `from` node on phase `A` (reverse flow). <br/> 0x003 - `AN` \- No current is flowing on phase `A`. <br/> 0x010 - `BF` \- Current is flowing from the `from` node to the `to` node on phase `B`. <br/> 0x020 - `BR` \- Current is flowing from the `to` node to the `from` node on phase `B` (reverse flow). <br/> 0x030 - `BN` \- No current is flowing on phase `B`. <br/> 0x100 - `CF` \- Current is flowing from the `from` node to the `to` node on phase `C`. <br/> 0x200 - `CR` \- Current is flowing from the `to` node to the `from` node on phase `C` (reverse flow). <br/> 0x300 - `CN` \- No current is flowing on phase `C`.
**mean_repair_time**  | double  | seconds  | Time after a fault has cleared before the object will be restored to service. Utilized by the **reliability** module.   
  
### Link State of Development

Link is considered a highly developed and validated model. 

