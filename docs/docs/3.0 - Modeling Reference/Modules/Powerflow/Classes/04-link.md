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
    

### Link Object Parameters

**link_object** objects are derived from **[powerflow_object](02-powerflow_object.md)** objects, so any parameters of the **[powerflow_object](02-powerflow_object.md)** object are available as well.

Input indicates properties you can set in models. Output marks properties produced or modified during simulation runtime.

| Property Name | Type | Unit | Input | Output | Description |
| --- | --- | --- | --- | --- | --- |
| status | enumeration | N/A | ✓ | ✓ | Status of the line in terms of being `OPEN` or `CLOSED`. This property is mainly used for switches and fuses, but may be used to remove lines from service. This functionality is primarily used in the `FBS` solver mode. |
| from | object | N/A | ✓ | ✓ | One connecting end of the **link** object. This will be the name or reference to a **node**-based object elsewhere in the powerflow model. |
| to | object | N/A | ✓ | ✓ | The other connecting end of the **link** object. This will be the name or reference to a **node**-based object elsewhere in the powerflow model. |
| power_in | complex | VA | ✓ | ✓ | The calculated power flowing into the particular **link** object as a sum of all three phases. |
| power_out | complex | VA | ✓ | ✓ | The calculated power flowing out of the particular **link** object as a sum of all three phases. |
| power_out_real | double | W | ✓ | ✓ | ⚠️ power flow out (w.r.t to node), real |
| power_losses | complex | VA | ✓ | ✓ | The calculated power loss for all three phases between the input and output of the **link**.. |
| power_in_A | complex | VA | ✓ | ✓ | The calculated power on phase A flowing into the **link**. |
| power_in_B | complex | VA | ✓ | ✓ | The calculated power on phase B flowing into the **link**. |
| power_in_C | complex | VA | ✓ | ✓ | The calculated power on phase C flowing into the **link**. |
| power_out_A | complex | VA | ✓ | ✓ | The calculated power on phase A flowing out of the **link**. |
| power_out_B | complex | VA | ✓ | ✓ | The calculated power on phase B flowing out of the **link**. |
| power_out_C | complex | VA | ✓ | ✓ | The calculated power on phase C flowing out of the **link**. |
| power_losses_A | complex | VA | ✓ | ✓ | The calculated power loss between the input and output of the **link** on phase A. |
| power_losses_B | complex | VA | ✓ | ✓ | The calculated power loss between the input and output of the **link** on phase B. |
| power_losses_C | complex | VA | ✓ | ✓ | The calculated power loss between the input and output of the **link** on phase C. |
| current_out_A | complex | A |  | ✓ | The calculated current flowing out of the link object on phase `A`. Note: This has not been fully tested for every object. |
| current_out_B | complex | A |  | ✓ | The calculated current flowing out of the link object on phase `B`. Note: This has not been fully tested for every object. |
| current_out_C | complex | A |  | ✓ | The calculated current flowing out of the link object on phase `C`. Note: This has not been fully tested for every object. |
| current_in_A | complex | A |  | ✓ | The calculated current flowing into the link object on phase `A`. Note: This has not been fully tested for every object. |
| current_in_B | complex | A |  | ✓ | The calculated current flowing into the link object on phase `B`. Note: This has not been fully tested for every object. |
| current_in_C | complex | A |  | ✓ | The calculated current flowing into the link object on phase `C`. Note: This has not been fully tested for every object. |
| fault_current_in_A | complex | A | ✓ | ✓ | ⚠️ fault current flowing in, phase A |
| fault_current_in_B | complex | A | ✓ | ✓ | ⚠️ fault current flowing in, phase B |
| fault_current_in_C | complex | A | ✓ | ✓ | ⚠️ fault current flowing in, phase C |
| fault_current_out_A | complex | A | ✓ | ✓ | ⚠️ fault current flowing out, phase A |
| fault_current_out_B | complex | A | ✓ | ✓ | ⚠️ fault current flowing out, phase B |
| fault_current_out_C | complex | A | ✓ | ✓ | ⚠️ fault current flowing out, phase C |
| pdispatch | double | W | ✓ | ✓ | ⚠️ Scheduled flow from-&gt;to in W |
| pdispatch_offset | double | W | ✓ | ✓ | ⚠️ Offset to scheduled flow from-&gt;to in W |
| set_pdispatch | bool | N/A | ✓ | ✓ | ⚠️ trigger to set pdispatch equal to (power_in + power_out)/2 |
| fault_voltage_A | complex | A |  | ✓ | ⚠️ fault voltage, phase A |
| fault_voltage_B | complex | A |  | ✓ | ⚠️ fault voltage, phase B |
| fault_voltage_C | complex | A |  | ✓ | ⚠️ fault voltage, phase C |
| overloaded_status | bool | N/A |  | ✓ | ⚠️ overloaded status (true/false) |
| flow_direction | set | N/A | ✓ | ✓ | This is a flag telling which direction current is flowing, relative to the `to` and `from` designations, on a each phase of a link object. &lt;br/&gt;    0x000 - `UNKNOWN` \- The flow direction is indeterminate. &lt;br/&gt;  0x001 - `AF` \- Current is flowing from the `from` node to the `to` node on phase `A`. &lt;br/&gt;  0x002 - `AR` \- Current is flowing from the `to` node to the `from` node on phase `A` (reverse flow). &lt;br/&gt; 0x003 - `AN` \- No current is flowing on phase `A`. &lt;br/&gt; 0x010 - `BF` \- Current is flowing from the `from` node to the `to` node on phase `B`. &lt;br/&gt; 0x020 - `BR` \- Current is flowing from the `to` node to the `from` node on phase `B` (reverse flow). &lt;br/&gt; 0x030 - `BN` \- No current is flowing on phase `B`. &lt;br/&gt; 0x100 - `CF` \- Current is flowing from the `from` node to the `to` node on phase `C`. &lt;br/&gt; 0x200 - `CR` \- Current is flowing from the `to` node to the `from` node on phase `C` (reverse flow). &lt;br/&gt; 0x300 - `CN` \- No current is flowing on phase `C`. |
| mean_repair_time | double | s | ✓ |  | Time after a fault has cleared before the object will be restored to service. Utilized by the **reliability** module. |
| continuous_rating_A | double | A | ✓ |  | ⚠️ Continuous rating for phase A of this link object (set individual line segments) |
| continuous_rating_B | double | A | ✓ |  | ⚠️ Continuous rating for phase B of this link object (set individual line segments) |
| continuous_rating_C | double | A | ✓ |  | ⚠️ Continuous rating for phase C of this link object (set individual line segments) |
| emergency_rating_A | double | A | ✓ |  | ⚠️ Emergency rating for phase A of this link object (set individual line segments) |
| emergency_rating_B | double | A | ✓ |  | ⚠️ Emergency rating for phase B of this link object (set individual line segments) |
| emergency_rating_C | double | A | ✓ |  | ⚠️ Emergency rating for phase C of this link object (set individual line segments) |
| inrush_convergence_value | double | V | ✓ |  | ⚠️ Tolerance, as change in line voltage drop between iterations, for deltamode in-rush completion |
| inrush_integration_method_capacitance | enumeration | N/A | ✓ | ✓ | ⚠️ Selected integration method to use for capacitive elements of the link Valid values: `NONE`, `UNDEFINED`, `TRAPEZOIDAL`, `BACKWARD_EULER`. |
| inrush_integration_method_inductance | enumeration | N/A | ✓ | ✓ | ⚠️ Selected integration method to use for inductive elements of the link Valid values: `NONE`, `UNDEFINED`, `TRAPEZOIDAL`, `BACKWARD_EULER`. |

#### Developer Properties

These properties are published with `PA_HIDDEN` and are intended for internal or developer use.

| Property Name | Type | Unit | Input | Output | Description |
| --- | --- | --- | --- | --- | --- |
| triplex_neutral_1_value | complex | N/A |  | ✓ |  |
| triplex_neutral_2_value | complex | N/A |  | ✓ |  |

### Link State of Development

Link is considered a highly developed and validated model. 

