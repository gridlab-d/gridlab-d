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

Property Name  | Type  | Unit  | Description   
---|---|---|---  
**connect_type**  | enumeration  | N/A  | Selection method for the electrical connection type of the regulator implemented. Valid types may be referred to by number or keyword <br/> 0 - `UNKNOWN` \- Unknown regulator implementation that will throw an error if used <br/> 1 - `WYE_WYE` \- Wye connected regulator implementation <br/> 2 - `OPEN_DELTA_ABBC` \- Open delta connected regulator with CA open - **Note: Unimplemented at this time** <br/> 3 - `OPEN_DELTA_BCAC` \- Open delta connected regulator with AB open - **Note: Unimplemented at this time** <br/> 4 - `OPEN_DELTA_CABA` \- Open delta connected regulator with BC open - **Note: Unimplemented at this time** <br/> 5 - `CLOSED_DELTA` \- Closed delta connected regulator implementation - **Note: Unimplemented at this time**  
**band_center**  | double  | Volts  | Center point of the voltage level desired.   
**band_width**  | double  | Volts  | Allowed range for the voltage to vary before a change is implemented. Centered around `band_center`, so limits are at `band_center - band_width/2` and `band_center + band_width/2`.   
**time_delay**  | double  | seconds  | Amount of time from a change request to the physical changing of the tap position on the regulator. Represents mechanical delays in the regulator.   
**dwell_time**  | double  | seconds  | Amount of time a change must be consistently requested before enacted upon. Represents a transient filter or additional hysteresis implementation to prevent excessive tap changes due to transient spikes.   
**raise_taps**  | int16  | N/A  | Upper limit of tap positions allowed in the regulator.   
**lower_taps**  | int16  | N/A  | Lower limit of tap positions allowed in the regulator. Note: This value is represented as a magnitude value. The actual lower limit of the tap positions is assumed to be -`lower_taps`.   
**current_transducer_ratio**  | double  | per-unit  | Turns ratio for current transducer for the line-drop compensator control method.   
**power_transducer_ratio**  | double  | per-unit  | Turns ratio for the power transducer for the line-drop compensator control method.   
**compensator_r_setting_A**  | double  | Volts  | Compensator resistive value for phase `A`.   
**compensator_r_setting_B**  | double  | Volts  | Compensator resistive value for phase `B`.   
**compensator_r_setting_C**  | double  | Volts  | Compensator resistive value for phase `C`.   
**compensator_x_setting_A**  | double  | Volts  | Compensator reactive value for phase `A`.   
**compensator_x_setting_B**  | double  | Volts  | Compensator reactive value for phase `B`.   
**compensator_x_setting_C**  | double  | Volts  | Compensator reactive value for phase `C`.   
**CT_phase**  | set  | N/A  | Current transducer connection phase. Valid keywords are <br/> - `A` \- Phase `A` current transducer <br/> - `B` \- Phase `B` current transducer <br/> - `C` \- Phase `C` current transducer **Note: This function is not implemented at this time.**  
**PT_phase**  | set  | N/A  | Power transducer connection phase. Valid keywords are <br/> - `A` \- Phase `A` power transducer <br/> - `B` \- Phase `B` power transducer <br/> - `C` \- Phase `C` power transducer

!!! note
	
	This function is not implemented at this time.

Property Name  | Type  | Unit  | Description   
---|---|---|--- 
**regulation**  | double  | N/A  | Indicates range of voltage adjustment possible (i.e., per tap change ratio equals regulation / raise taps, or regulation of 0.1 indicates 10% rise in voltage at maximum tap position)   
**Control**  | enumeration  | N/A  | Defines the control scheme the regulator will use to operate. Valid keywords are: <br/> - `MANUAL` \- Manual control mode. User specifies all tap changes. <br/> - `OUTPUT_VOLTAGE` \- Output node of the regulator's voltage is examined. Tap changes are performed based on `band_center` and `band_width`. <br/> - `LINE_DROP_COMP` \- Line drop compensator control mode. Utilizes compensator information in addition to `band_center` and `band_width` to determine tap changes. <br/> - `REMOTE_NODE` \- Voltage of a remote node (specified by `sense_node` in the **regulator** object) in the system is examined. Tap changes are performed based on `band_center` and `band_width`.  
**control_level**  | enumeration  | N/A  | Defines how automatic controls influence the tap settings of the regulator. Valid keywords are: <br/> -  `INDIVIDUAL` \- Each phase is controlled individually. <br/> - `BANK` \- All phases are controlled identically. Using the `PT_phase` property, the regulator determines any control actions and applies it to all phases identically.   
**Type**  | enumeration  | N/A  | Type of step-voltage regulator implemented. Valid keywords are: <br/> - `A` \- Type A step-voltage regulator <br/> - `B` \- Type B step-voltage regulator  
**tap_pos_A**  | int16  | N/A  | Initial tap position for phase `A`. If left empty, the regulator will take a best guess at the initial tap position.   
**tap_pos_B**  | int16  | N/A  | Initial tap position for phase `B`. If left empty, the regulator will take a best guess at the initial tap position.   
**tap_pos_C**  | int16  | N/A  | Initial tap position for phase `C`. If left empty, the regulator will take a best guess at the initial tap position.   
  
### Regulator Configuration State of Development

Regulator Configuration is considered a well developed and validated model in terms of powerflow solutions, however, models may be developed to include more advanced features in the future. Additional configurations, controls, and/or losses may be included as needed. 

