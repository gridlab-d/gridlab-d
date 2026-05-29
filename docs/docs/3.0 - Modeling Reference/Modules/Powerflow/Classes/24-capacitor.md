## Capacitor

Capacitors are used for reactive power compensation and voltage support scenarios. The **capacitor** implements a switchable set of capacitors. **capacitor** objects are one of two objects in the **powerflow** module that incorporate a form of automatic control. To take full advantage of this functionality, simulations of greater than one time step (time-varying simulations) are recommended. Single-phase powerflow connections (phase `S`) are not supported by capacitors at this time. A typical capacitor implementation is 
    
    
    object capacitor {
    	phases ABCN;
    	name CapNode;
    	phases_connected ABCD;
    	control MANUAL;
    	capacitor_A 0.5 MVAr;
    	capacitor_B 0.5 MVAr;
    	capacitor_C 0.5 MVAr;
    	control_level INDIVIDUAL;
    	switchA OPEN;
    	switchB OPEN;
    	switchC CLOSED;
    	nominal_voltage 7200;
    	}

### Capacitor Parameters

#### Properties

**capacitor** objects are derived from **[node](03-node.md)** objects, so any parameters of the **[node](03-node.md)** object are available as well.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

Table: 24-capacitor table 1 { #tbl:24-capacitor-1 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| pt_phase | set | N/A | I | Determines the participating phases. These are the phases the various control schemes will monitor to determine their actions. Follows the same conventions as the overall `phases` property in powerflow. Valid values: `A`, `B`, `C`, `D`, `N`. |
| phases_connected | set | N/A | I | Connection of the capacitors. This allows for delta-connected capacitors on a wye-connected system and vice versa. Follows the same conventions as the overall `phases` property. If left empty or undefined, defaults to the `phases` property of the capacitor. Valid values: `A`, `B`, `C`, `D`, `N`. |
| switchA | enumeration | N/A | IO | Status of the switch that enables or disables the capacitor attached to phase `A` if wye-connected or `AB` if delta-connected. Valid values: `OPEN`, `CLOSED`. |
| switchB | enumeration | N/A | IO | Status of the switch that enables or disables the capacitor attached to phase `B` if wye-connected or `BC` if delta-connected. Valid values: `OPEN`, `CLOSED`. |
| switchC | enumeration | N/A | IO | Status of the switch that enables or disables the capacitor attached to phase `C` if wye-connected or `CA` if delta-connected. Valid values: `OPEN`, `CLOSED`. |
| control | enumeration | N/A | I | Defines the control scheme the capacitor will utilize to perform switching operations. Valid control mode keywords are <br/> - `MANUAL` \- Capacitor switching is controlled manually through `switchA`, `switchB`, and `switchC`. <br/> - `VAR` \- VAR controlled mode. A remote line needs to be specified in `remote_sense` or `remote_sense_B` that will have its reactive power checked against `VAr_set_high` and `VAr_set_low`. <br/> - `VOLT` \- Voltage controlled mode. The capacitor node itself or a node specified by `remote_sense` or `remote_sense_B` has its voltage checked against `voltage_set_high` and `voltage_set_low`. <br/> - `VARVOLT` \- Combination control scheme. Has two modes. If `voltage_set_low` is specified, performs control similar to `VOLT` first, and then `VAR` second. If `voltage_set_low` is unspecified or set to zero, operates in `VAR` mode primarily. However, `voltage_set_high` is monitored and will switch the capacitors off and lock them out if exceeded (voltage safety). |
| cap_A_switch_count | double | N/A | IO | Hold of the number of times the switch has changed (OPEN to CLOSED, CLOSED to OPEN) on phase `A` if wye-connected or `AB` if delta-connected. |
| cap_B_switch_count | double | N/A | IO | Hold of the number of times the switch has changed (OPEN to CLOSED, CLOSED to OPEN) on phase `B` if wye-connected or `BC` if delta-connected. |
| cap_C_switch_count | double | N/A | IO | Hold of the number of times the switch has changed (OPEN to CLOSED, CLOSED to OPEN) on phase `C` if wye-connected or `CA` if delta-connected. |
| voltage_set_high | double | V | IO | High setpoint for voltage-based capacitor switching operations. This setpoint will turn the capacitors off. |
| voltage_set_low | double | V | IO | Low setpoint for voltage-based capacitor switching operations. This setpoint will turn the capacitors on. |
| voltage_deadband_center | double | V | I | The voltage deadband center |
| voltage_deadband | double | V | I | The deadband between voltage_set_high and voltage_set_low |
| VAr_set_high | double | VAr | IO | High setpoint for VAr-based capacitor switching operations. This setpoint will turn the capacitors on. |
| VAr_set_low | double | VAr | IO | Low setpoint for VAr-based capacitor switching operations. This setpoint will turn the capacitors off. |
| VAr_deadband_center | double | VAr | I | The VAr deadband center |
| VAr_deadband | double | VAr | I | The deadband between VAr_set_high and VAr_set_low |
| current_set_low | double | A | IO | high current set point for current control mode (turn on) |
| current_set_high | double | A | IO | low current set point for current control mode (turn off) |
| current_deadband_center | double | A | I | The current deadband center |
| current_deadband | double | A | I | The deadband between current_set_high and current_set_low |
| capacitor_A | double | VAr | I | Capacitor size information for capacitor connected to phase `A` in a wye connection or on phase `AB` in a delta connection. |
| capacitor_B | double | VAr | I | Capacitor size information for capacitor connected to phase `B` in a wye connection or on phase `BC` in a delta connection. |
| capacitor_C | double | VAr | I | Capacitor size information for capacitor connected to phase `C` in a wye connection or on phase `CA` in a delta connection. |
| cap_nominal_voltage | double | V | I | Capacitor rated nominal voltage. Used for situations when `nominal_voltage` doesn't match the rated voltage or if a line-to-line voltage is specified when the capacitors are on a wye-connected system. If left blank, defaults to the `nominal_voltage` specified. |
| time_delay | double | s | I | Time delay before any capacitor switching operation takes place. Represents mechanical switching delays. |
| dwell_time | double | s | I | Time period a switching operation must be consistently requested before any switching operation is attempted. Serves as a transient filter or additional hysteresis to prevent transient events from causing excessive capacitor switching. |
| lockout_time | double | s | I | Time period a capacitor will lock out switching operations after `voltage_set_high` is exceeded in the `VARVOLT` control method. |
| remote_sense | object | N/A | I | Remote **node** or **link** object for `VOLT`, `VAR`, or `VARVOLT` control schemes. If a **node** object is specified, the remote voltage is read. If **link** object is specified, the reactive power is read. |
| remote_sense_B | object | N/A | I | Remote **node** or **link** object for `VOLT`, `VAR`, or `VARVOLT` control schemes. If a **node** object is specified, the remote voltage is read. If **link** object is specified, the reactive power is read. Under the `VARVOLT` control scheme, this must be the opposite of the type specified in `remote_sense`. |
| control_level | enumeration | N/A | I | Specifies how the switching action occurs for all phases of the capacitor. Valid keywords are <br/> - `BANK` \- All capacitors are switched based on the control scheme and `pt_phase` property. <br/> - `INDIVIDUAL` \- Capacitors are switched individually based on the control scheme and `pt_phase` property. |

### Capacitor State of Development

Capacitor is considered a well developed and validated model in terms of powerflow solutions, however, models may be developed to include more advanced features in the future. Additional configurations and controls may be included as needed. 
