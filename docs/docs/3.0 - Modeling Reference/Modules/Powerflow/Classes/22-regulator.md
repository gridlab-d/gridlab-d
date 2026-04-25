## Regulator

!!! warning
    This page was automatically generated and requires review.

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

#### Properties

**regulator** objects are derived from **[link](04-link.md)** objects, so any parameters of the **[link](04-link.md)** object are available as well.

Input indicates properties you can set in models. Output marks properties produced or modified during simulation runtime.

| Property Name | Type | Unit | Input | Output | Description |
| --- | --- | --- | --- | --- | --- |
| configuration | object | N/A | ✓ |  | **regulator_configuration** object that describes the specific regulator implementation. |
| tap_A | int16 | N/A | ✓ | ✓ | Position of the tap on phase `A` of a wye-connected or phase `AB` of a delta-connected system. This parameter is most useful to be read in automatic regulator modes, but serves as the input for tap position of the phase under the manual control scheme. |
| tap_B | int16 | N/A | ✓ | ✓ | Position of the tap on phase `B` of a wye-connected or phase `BC` of a delta-connected system. This parameter is most useful to be read in automatic regulator modes, but serves as the input for tap position of the phase under the manual control scheme. |
| tap_C | int16 | N/A | ✓ | ✓ | Position of the tap on phase `C` of a wye-connected or phase `CA` of a delta-connected system. This parameter is most useful to be read in automatic regulator modes, but serves as the input for tap position of the phase under the manual control scheme. |
| msg_mode | enumeration | N/A | ✓ | ✓ | ⚠️ messages regarding remote node voltage to come internally from gridlabd or externally through co-simulation. Set to EXTERNAL only if you have co-simulation enabled Valid values: `INTERNAL`, `EXTERNAL`. |
| remote_voltage_A | complex | V | ✓ | ✓ | ⚠️ remote node voltage, Phase A to ground |
| remote_voltage_B | complex | V | ✓ | ✓ | ⚠️ remote node voltage, Phase B to ground |
| remote_voltage_C | complex | V | ✓ | ✓ | ⚠️ remote node voltage, Phase C to ground |
| tap_A_change_count | double | N/A | ✓ | ✓ | Holds the number of times the tap position on phase `A` of a wye-connected or phase `AB` of a delta-connected system has changed. |
| tap_B_change_count | double | N/A | ✓ | ✓ | Holds the number of times the tap position on phase `B` of a wye-connected or phase `BC` of a delta-connected system has changed. |
| tap_C_change_count | double | N/A | ✓ | ✓ | Holds the number of times the tap position on phase `C` of a wye-connected or phase `CA` of a delta-connected system has changed. |
| sense_node | object | N/A | ✓ |  | Remote node for the automatic control method to monitor. Only utilized in `REMOTE_NODE` control scheme. This must be a **node**-based object to work properly. |
| regulator_resistance | double | Ohm | ✓ |  | ⚠️ The resistance value of the regulator when it is not blown. |

### Regulator State of Development

Regulator is considered a well developed and validated model in terms of powerflow solutions, however, models may be developed to include more advanced features in the future. Additional configurations, controls, and/or losses may be included as needed. 
