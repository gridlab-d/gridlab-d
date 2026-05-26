## Switch Object

!!! warning
    This page was automatically generated and requires review.

Switch objects are used to change topology and add or remove elements from a powerflow system. When a switch is opened, no current flow is permitted and the downstream objects will be effectively removed from the system. A typical switch implementation is 
    
    
    object switch {
    	name switch1;
    	phases ABCN;
    	from node_250;
    	to node_243;
    	status CLOSED;
    	}

### Switch Object Parameters

#### Properties

**switch_object** objects are derived from **[link](04-link.md)** objects, so any parameters of the **[link](04-link.md)** object are available as well.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| phase_A_state | enumeration | N/A | IO | Status of the phase A portion of the switch. Valid states are: <br/> - `OPEN` \- **switch** is open and no current can flow <br/> - `CLOSED` \- **switch** is closed and conducting |
| phase_B_state | enumeration | N/A | IO | Status of the phase B portion of the switch. Valid states are: <br/> - `OPEN` \- **switch** is open and no current can flow <br/> - `CLOSED` \- **switch** is closed and conducting |
| phase_C_state | enumeration | N/A | IO | Status of the phase C portion of the switch. Valid states are: <br/> - `OPEN` \- **switch** is open and no current can flow <br/> - `CLOSED` \- **switch** is closed and conducting |
| operating_mode | enumeration | N/A | IO | Switching operations governing criterion. Two settings are available: <br/> - `INDIVIDUAL` \- each phase of the **switch** object is controlled individually. <br/> - `BANKED` \- all valid phases of the **switch** object are controlled together. A voting scheme of valid phases is used to determine this. For example, if a **switch** has phases A, B, and C with only phase C closed, phase A and B will override it and open phase A. |
| switch_impedance | complex | Ohm | I | ⚠️ Impedance value of the swtich when closed |
| switch_resistance | double | Ohm | I | ⚠️ Resistance portion of impedance value of the switch when it is closed. |
| switch_reactance | double | Ohm | I | ⚠️ Reactance portion of impedance value of the switch when it is closed. |

### Switch State of Development

Switch is considered a well developed and validated model in terms of powerflow solutions, however, models incorporating fault analysis, reliability, and other advanced features have not been validated. 
