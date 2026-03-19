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

**capacitor** objects are derived from **node** objects, so any parameters of the **node** object are available as well. 

Property Name  | Type  | Unit  | Description   
---|---|---|---  
**pt_phase**  | set  | N/A  | Determines the participating phases. These are the phases the various control schemes will monitor to determine their actions. Follows the same conventions as the overall `phases` property in powerflow.   
**phases_connected**  | set  | N/A  | Connection of the capacitors. This allows for delta-connected capacitors on a wye-connected system and vice versa. Follows the same conventions as the overall `phases` property. If left empty or undefined, defaults to the `phases` property of the capacitor.   
**switchA**  | enumeration  | N/A  | Status of the switch that enables or disables the capacitor attached to phase `A` if wye-connected or `AB` if delta-connected.   
**switchB**  | enumeration  | N/A  | Status of the switch that enables or disables the capacitor attached to phase `B` if wye-connected or `BC` if delta-connected.   
**switchC**  | enumeration  | N/A  | Status of the switch that enables or disables the capacitor attached to phase `C` if wye-connected or `CA` if delta-connected.   
**cap_A_switch_count**  | int16  | N/A  | Hold of the number of times the switch has changed (OPEN to CLOSED, CLOSED to OPEN) on phase `A` if wye-connected or `AB` if delta-connected.   
**cap_B_switch_count**  | int16  | N/A  | Hold of the number of times the switch has changed (OPEN to CLOSED, CLOSED to OPEN) on phase `B` if wye-connected or `BC` if delta-connected.   
**cap_C_switch_count**  | int16  | N/A  | Hold of the number of times the switch has changed (OPEN to CLOSED, CLOSED to OPEN) on phase `C` if wye-connected or `CA` if delta-connected.   
**control**  | enumeration  | N/A  | Defines the control scheme the capacitor will utilize to perform switching operations. Valid control mode keywords are <br/> - `MANUAL` \- Capacitor switching is controlled manually through `switchA`, `switchB`, and `switchC`. <br/> - `VAR` \- VAR controlled mode. A remote line needs to be specified in `remote_sense` or `remote_sense_B` that will have its reactive power checked against `VAr_set_high` and `VAr_set_low`. <br/> - `VOLT` \- Voltage controlled mode. The capacitor node itself or a node specified by `remote_sense` or `remote_sense_B` has its voltage checked against `voltage_set_high` and `voltage_set_low`. <br/> - `VARVOLT` \- Combination control scheme. Has two modes. If `voltage_set_low` is specified, performs control similar to `VOLT` first, and then `VAR` second. If `voltage_set_low` is unspecified or set to zero, operates in `VAR` mode primarily. However, `voltage_set_high` is monitored and will switch the capacitors off and lock them out if exceeded (voltage safety).
**voltage_set_high**  | double  | Volts  | High setpoint for voltage-based capacitor switching operations. This setpoint will turn the capacitors off.   
**voltage_set_low**  | double  | Volts  | Low setpoint for voltage-based capacitor switching operations. This setpoint will turn the capacitors on.   
**VAr_set_high**  | double  | Volt-Amperes reactive  | High setpoint for VAr-based capacitor switching operations. This setpoint will turn the capacitors on.   
**VAr_set_low**  | double  | Volt-Amperes reactive  | Low setpoint for VAr-based capacitor switching operations. This setpoint will turn the capacitors off.   
**capacitor_A**  | double  | Volt-Amperes reactive  | Capacitor size information for capacitor connected to phase `A` in a wye connection or on phase `AB` in a delta connection.   
**capacitor_B**  | double  | Volt-Amperes reactive  | Capacitor size information for capacitor connected to phase `B` in a wye connection or on phase `BC` in a delta connection.   
**capacitor_C**  | double  | Volt-Amperes reactive  | Capacitor size information for capacitor connected to phase `C` in a wye connection or on phase `CA` in a delta connection.   
**cap_nominal_voltage**  | double  | Volts  | Capacitor rated nominal voltage. Used for situations when `nominal_voltage` doesn't match the rated voltage or if a line-to-line voltage is specified when the capacitors are on a wye-connected system. If left blank, defaults to the `nominal_voltage` specified.   
**time_delay**  | double  | seconds  | Time delay before any capacitor switching operation takes place. Represents mechanical switching delays.   
**dwell_time**  | double  | seconds  | Time period a switching operation must be consistently requested before any switching operation is attempted. Serves as a transient filter or additional hysteresis to prevent transient events from causing excessive capacitor switching.   
**lockout_time**  | double  | seconds  | Time period a capacitor will lock out switching operations after `voltage_set_high` is exceeded in the `VARVOLT` control method.   
**remote_sense**  | object  | N/A  | Remote **node** or **link** object for `VOLT`, `VAR`, or `VARVOLT` control schemes. If a **node** object is specified, the remote voltage is read. If **link** object is specified, the reactive power is read.   
**remote_sense_B**  | object  | N/A  | Remote **node** or **link** object for `VOLT`, `VAR`, or `VARVOLT` control schemes. If a **node** object is specified, the remote voltage is read. If **link** object is specified, the reactive power is read. Under the `VARVOLT` control scheme, this must be the opposite of the type specified in `remote_sense`.   
**control_level**  | enumeration  | N/A  | Specifies how the switching action occurs for all phases of the capacitor. Valid keywords are <br/> - `BANK` \- All capacitors are switched based on the control scheme and `pt_phase` property. <br/> - `INDIVIDUAL` \- Capacitors are switched individually based on the control scheme and `pt_phase` property.
  
### Capacitor State of Development

Capacitor is considered a well developed and validated model in terms of powerflow solutions, however, models may be developed to include more advanced features in the future. Additional configurations and controls may be included as needed. 

