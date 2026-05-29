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

#### Properties

**regulator** objects are derived from **[link](04-link.md)** objects, so any parameters of the **[link](04-link.md)** object are available as well.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

Table: 22-regulator table 1 { #tbl:22-regulator-1 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| configuration | object | N/A | I | **regulator_configuration** object that describes the specific regulator implementation. |
| tap_A | int16 | N/A | IO | Position of the tap on phase `A` of a wye-connected or phase `AB` of a delta-connected system. This parameter is most useful to be read in automatic regulator modes, but serves as the input for tap position of the phase under the manual control scheme. |
| tap_B | int16 | N/A | IO | Position of the tap on phase `B` of a wye-connected or phase `BC` of a delta-connected system. This parameter is most useful to be read in automatic regulator modes, but serves as the input for tap position of the phase under the manual control scheme. |
| tap_C | int16 | N/A | IO | Position of the tap on phase `C` of a wye-connected or phase `CA` of a delta-connected system. This parameter is most useful to be read in automatic regulator modes, but serves as the input for tap position of the phase under the manual control scheme. |
| msg_mode | enumeration | N/A | I | Messages regarding remote node voltage to come internally from gridlabd or externally through co-simulation. Set to EXTERNAL only if you have co-simulation enabled Valid values: `INTERNAL`, `EXTERNAL`. |
| remote_voltage_A | complex | V | IO | Remote node voltage, Phase A to ground |
| remote_voltage_B | complex | V | IO | Remote node voltage, Phase B to ground |
| remote_voltage_C | complex | V | IO | Remote node voltage, Phase C to ground |
| tap_A_change_count | double | N/A | IO | Holds the number of times the tap position on phase `A` of a wye-connected or phase `AB` of a delta-connected system has changed. |
| tap_B_change_count | double | N/A | IO | Holds the number of times the tap position on phase `B` of a wye-connected or phase `BC` of a delta-connected system has changed. |
| tap_C_change_count | double | N/A | IO | Holds the number of times the tap position on phase `C` of a wye-connected or phase `CA` of a delta-connected system has changed. |
| sense_node | object | N/A | I | Remote node for the automatic control method to monitor. Only utilized in `REMOTE_NODE` control scheme. This must be a **node**-based object to work properly. |
| regulator_resistance | double | Ohm | I | The resistance value of the regulator when it is not blown. |

### Regulator State of Development

Regulator is considered a well developed and validated model in terms of powerflow solutions, however, models may be developed to include more advanced features in the future. Additional configurations, controls, and/or losses may be included as needed. 


## Regulator Configuration

The **regulator_configuration** object describes the details of a particular **regulator** object implementation. This includes details such as the control scheme, regulator type, sensing information, and time delays. A typical regulator configuration would look similar to 
    
    
    object regulator_configuration {
    	name reg_conf_79978101;
    	connect_type 2;
    	band_center 122.000;
    	band_width 2.0;
    	time_delay 30.0;
    	raise_taps 16;
    	lower_taps 16;
    	current_transducer_ratio 350;
    	power_transducer_ratio 40;
    	compensator_r_setting_A 1.5;
    	compensator_x_setting_A 3.0;
    	compensator_r_setting_B 1.5;
    	compensator_x_setting_B 3.0;
    	CT_phase "ABC";
    	PT_phase "ABC";
    	regulation 0.10;
    	Control MANUAL;
    	control_level INDIVIDUAL;
    	Type A;
    	tap_pos_A 7;
    	tap_pos_B 4;
    	}

### Regulator Configuration Parameters

#### Properties

**regulator_configuration** does not declare inherited parent classes.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

Table: 22-regulator table 2 { #tbl:22-regulator-2 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| connect_type | enumeration | N/A | I | Selection method for the electrical connection type of the regulator implemented. Valid types may be referred to by number or keyword <br/> 0 - `UNKNOWN` \- Unknown regulator implementation that will throw an error if used <br/> 1 - `WYE_WYE` \- Wye connected regulator implementation <br/> 2 - `OPEN_DELTA_ABBC` \- Open delta connected regulator with CA open - **Note: Unimplemented at this time** <br/> 3 - `OPEN_DELTA_BCAC` \- Open delta connected regulator with AB open - **Note: Unimplemented at this time** <br/> 4 - `OPEN_DELTA_CABA` \- Open delta connected regulator with BC open - **Note: Unimplemented at this time** <br/> 5 - `CLOSED_DELTA` \- Closed delta connected regulator implementation - **Note: Unimplemented at this time** |
| band_center | double | V | I | Center point of the voltage level desired. |
| band_width | double | V | I | Allowed range for the voltage to vary before a change is implemented. Centered around `band_center`, so limits are at `band_center - band_width/2` and `band_center + band_width/2`. |
| time_delay | double | s | I | Amount of time from a change request to the physical changing of the tap position on the regulator. Represents mechanical delays in the regulator. |
| dwell_time | double | s | I | Amount of time a change must be consistently requested before enacted upon. Represents a transient filter or additional hysteresis implementation to prevent excessive tap changes due to transient spikes. |
| raise_taps | int16 | N/A | I | Upper limit of tap positions allowed in the regulator. |
| lower_taps | int16 | N/A | I | Lower limit of tap positions allowed in the regulator. Note: This value is represented as a magnitude value. The actual lower limit of the tap positions is assumed to be -`lower_taps`. |
| current_transducer_ratio | double | pu | I | Turns ratio for current transducer for the line-drop compensator control method. |
| power_transducer_ratio | double | pu | I | Turns ratio for the power transducer for the line-drop compensator control method. |
| compensator_r_setting_A | double | V | I | Compensator resistive value for phase `A`. |
| compensator_r_setting_B | double | V | I | Compensator resistive value for phase `B`. |
| compensator_r_setting_C | double | V | I | Compensator resistive value for phase `C`. |
| compensator_x_setting_A | double | V | I | Compensator reactive value for phase `A`. |
| compensator_x_setting_B | double | V | I | Compensator reactive value for phase `B`. |
| compensator_x_setting_C | double | V | I | Compensator reactive value for phase `C`. |
| CT_phase | set | N/A | I | Current transducer connection phase. Valid keywords are <br/> - `A` \- Phase `A` current transducer <br/> - `B` \- Phase `B` current transducer <br/> - `C` \- Phase `C` current transducer **Note: This function is not implemented at this time.** |
| PT_phase | set | N/A | I | Power transducer connection phase. Valid keywords are <br/> - `A` \- Phase `A` power transducer <br/> - `B` \- Phase `B` power transducer <br/> - `C` \- Phase `C` power transducer |
| regulation | double | N/A | I | Indicates range of voltage adjustment possible (i.e., per tap change ratio equals regulation / raise taps, or regulation of 0.1 indicates 10% rise in voltage at maximum tap position) |
| control_level | enumeration | N/A | I | Defines how automatic controls influence the tap settings of the regulator. Valid keywords are: <br/> -  `INDIVIDUAL` \- Each phase is controlled individually. <br/> - `BANK` \- All phases are controlled identically. Using the `PT_phase` property, the regulator determines any control actions and applies it to all phases identically. |
| Control | enumeration | N/A | I | Defines the control scheme the regulator will use to operate. Valid keywords are: <br/> - `MANUAL` \- Manual control mode. User specifies all tap changes. <br/> - `OUTPUT_VOLTAGE` \- Output node of the regulator's voltage is examined. Tap changes are performed based on `band_center` and `band_width`. <br/> - `LINE_DROP_COMP` \- Line drop compensator control mode. Utilizes compensator information in addition to `band_center` and `band_width` to determine tap changes. <br/> - `REMOTE_NODE` \- Voltage of a remote node (specified by `sense_node` in the **regulator** object) in the system is examined. Tap changes are performed based on `band_center` and `band_width`. |
| reverse_flow_control | enumeration | N/A | I | Type of control used when power is flowing in reverse through the regulator Valid values: `LOCK_NONE`, `LOCK_NEUTRAL`, `LOCK_CURRENT_POSITION`. |
| Type | enumeration | N/A | I | Type of step-voltage regulator implemented. Valid keywords are: <br/> - `A` \- Type A step-voltage regulator <br/> - `B` \- Type B step-voltage regulator |
| tap_pos_A | int16 | N/A | I | Initial tap position for phase `A`. If left empty, the regulator will take a best guess at the initial tap position. |
| tap_pos_B | int16 | N/A | I | Initial tap position for phase `B`. If left empty, the regulator will take a best guess at the initial tap position. |
| tap_pos_C | int16 | N/A | I | Initial tap position for phase `C`. If left empty, the regulator will take a best guess at the initial tap position. |

### Regulator Configuration State of Development

Regulator Configuration is considered a well developed and validated model in terms of powerflow solutions, however, models may be developed to include more advanced features in the future. Additional configurations, controls, and/or losses may be included as needed. 
