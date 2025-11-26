# Sync Check

The synchronization check capability in GridLAB-D™ will be implemented to perform paralleling for two independent power grids. This could be used to parallel two separate power systems, or to reconnect a microgrid to the bulk power system. In the simulation, the frequency and voltage metrics are checked. When conditions are satisfied, the [sync_check] object sends a closure command to its parent [switch_object]. 

# Published properties (i.e., GLM inputs)

The mapping between the properties and variables is listed in Table 1. The [sync_check] object will inherit all standard [Object_(directive)] values as well. The variable definitions are presented in Table 2 of the next section ["Variable definitions"]. Note that the properties listed here represent two different modes for calculating the from/to voltage conditions - `MAG_DIFF` and `SEP_DIFF`. Notes are included as to which property is used for which mode. 

##### Table 1 - Mapping between Properties and Variables  

Property  | Mapped Variable  | Data Type  | Descriptions   
---|---|---|---  
armed  | sc_flag  | Boolean  | Turns on/off the action functionality <br/> * `True` \- This object is functional <br/> * `False` \- This object is disabled 
volt_compare_mode  | volt_compare_mode  | Enumeration  | Determines which voltage difference calculation approach is used. This determines how the difference between `from` and `to` node voltages are compared. Two entries are supported: <br/> * `MAG_DIFF` \- consider only the magnitude of the overall complex difference between the from and to voltage. e.g., abs(F-T). This is the default mode. <br/> * `SEP_DIFF` \- consider the difference of voltage magnitude and difference of voltage angle separately. e.g., abs(|F|-|T|) and abs(angle(F)-angle(T)). 
frequency_tolerance  | eps_freq  | Double  | The user-specified tolerance in Hz for checking the frequency metric. Used by both `volt_compare_mode` types.   
voltage_tolerance  | eps_volt  | Double  | The user-specified tolerance in per unit for checking the voltage metric. Used only by the `MAG_DIFF` mode of `volt_compare_mode`.   
voltage_magnitude_tolerance  | eps_mag_volt  | Double  | The user-specified tolerance in per unit for the difference in voltage magnitudes for checking the voltage metric. Used only by the `SEP_DIFF` mode of `volt_compare_mode`.   
voltage_angle_tolerance  | eps_ang_volt  | Double  | The user-specified tolerance in degrees for the difference in voltage angles for checking the voltage metric. Used only by the `SEP_DIFF` mode of `volt_compare_mode`.   
metrics_period  | t_ud  | Double  | The user-defined period when both metrics are satisfied. Used by both `volt_compare_mode` types.   
delta_trigger_mult  | delta_trigger_mult  | Double  | Multiplier of the appropriate voltage/frequency tolerances to trigger deltamode. e.g., in `MAG_DIFF` mode, `voltage_tolerance` and `frequency_tolerance` will be multiplied by this value, and if current conditions are within that band (and `sync_check` is armed), deltamode will be triggered/maintained until either a close action occurs, or the band is exited.   
  
Two sample [sync_check] objects defined in the glm file are shown as follows. 

Using only the difference of the overall magnitude: 
    
    
    object sync_check
    {
       name sc_m12;
       parent swt_01;
       armed true;
       volt_compare_mode MAG_DIFF;
       frequency_tolerance 0.6 Hz;
       voltage_tolerance 0.01; //Unit: pu
       metrics_period 5 s;
       delta_trigger_mult 2.0;
    }
    

Using the difference of the voltage magnitudes and angles separately: 
    
    
    object sync_check
    {
       name sc_m12;
       parent swt_01;
       armed true;
       volt_compare_mode SEP_DIFF;
       frequency_tolerance 0.6 Hz;
       voltage_magnitude_tolerance 0.01; //Unit: pu
       voltage_angle_tolerance 5 deg;
       metrics_period 5 s;
       delta_trigger_mult 2.0;
    }
    

# Variable definitions

Variables of the [sync_check] functionality are defined as follows: 

##### Table 2 - Variable Definitions  

Variable  | Data Type  | Unit  | Definition   
---|---|---|---  
Flag   
sc_flag  | Boolean  | N/A  | Turns on/off the action functionality <br/> * `True` \- This object is functional <br/> * `False` \- This object is disabled
volt_compare_mode  | Enumeration  | N/A  | Determines the algorithm to use for comparing the voltages of the `from` and `to` nodes of the `switch_object` <br/> * `MAG_DIFF` \- Use only the magnitude of the difference - e.g., abs(F-T) <br/> * `SEP_DIFF` \- Use the difference of the magnitude and angle separately - e.g., abs(|F|-|T|) and abs(angle(F)-angle(T)) 
swt_status  | Enumeration  | N/A  | Status of the parent switch object. Valid states are: <br/> * `OPEN` \- Its parent `switch object` is open and no current can flow <br/> * `CLOSED` \- Its parent `switch object` is closed and conducting  
Voltage   
volt_m1  | Complex  | V  | The voltage phasor measured at the 'from' node of its parent switch   
volt_m2  | Complex  | V  | The voltage phasor measured at the 'to' node of its parent switch   
volt_norm  | Double  | V  | Nominal voltage, which is set as the value pulled from the variable/attribute 'nominal_voltage' of either the 'from' or 'to' node object of its parent switch object   
volt_diff  | Double  | V  | The magnitude of the difference between volt_m1 and volt_m2 - used by `MAG_DIFF`  
volt_diff_pu  | Double  | pu  | volt_diff normalized against volt_norm - used by `MAG_DIFF`  
volt_diff_mag  | Double  | V  | The magnitude of the difference between the magnitude of volt_m1 and the magnitude of volt_m2 - used by `SEP_DIFF`  
volt_diff_mag_pu  | Double  | pu  | volt_diff_mag normalized against volt_norm - used by `SEP_DIFF`  
volt_diff_ang  | Double  | deg  | The magnitude of the difference between the angle of volt_m1 and the angle of volt_m2 - used by `SEP_DIFF`  
Frequency   
freq_m1  | Double  | Hz  | The frequency measured at the 'from' node of its parent switch   
freq_m2  | Double  | Hz  | The frequency measured at the 'to' node of its parent switch   
freq_diff  | Double  | Hz  | The absolute value of the difference between freq_m1 and freq_m2   
freq_diff_pu  | Double  | pu  | freq_diff normalized against nominal_frequency (powerflow global)   
Tolerance   
eps_freq  | Double  | Hz  | The user-specified tolerance for checking the frequency metric   
eps_volt  | Double  | pu  | The user-specified tolerance for checking the magnitude of the overall voltage difference. Used for the `MAG_DIFF` mode of operation   
eps_mag_volt  | Double  | pu  | The user-specified tolerance for checking the magnitude of the difference of voltage magnitudes. Used for the `SEP_DIFF` mode of operation   
eps_angle_volt  | Double  | deg  | The user-specified tolerance for checking the magnitude of the difference of voltage angles. Used for the `SEP_DIFF` mode of operation   
t_ud  | Double  | sec  | The user-defined period when both metrics are satisfied   
delta_trigger_mult  | Double  | N/A  | User-defined multiplier for the appropriate tolerances to trigger/maintain deltamode   
Timer   
t_sat  | Double  | sec  | The total period (initialized as 0) during which both metrics have been satisfied continuously   
dt_dm  | Double  | sec  | Current deltamode timestep   
  
# Methodology

### Quasi-Steady State Time Series (QSTS)

In QSTS mode, the sync_check object will check current voltage and frequency metrics against the appropriate tolerance values, but multiplied by the `deltamode_trigger_mult` variable. If they are within this expanded range and the sync_check is armed, deltamode will be triggered. 

### Deltamode

Paralleling will only occur when the grid-alignment conditions are met. While the voltage and frequency metrics are within the multiple of `deltamode_trigger_mult` and the appropriate tolerances, deltamode will continue to be requested (`SM_DELTA`). e.g., if in `MAG_DIFF` mode, the sync_check is armed, and the magnitude of the voltage difference is less than `deltamode_trigger_mult` * `eps_volt`, a request to stay in deltamode will be sent. 

# Validation

This subsection provides an outline on how the [sync_check] object will be tested to ensure its functionality. The current plan is to use two 4-node test systems interconnected through a [sync_check] object, which is open initially. The frequency and voltage values measured at the 'from' and 'to' nodes of the [sync_check] object are initialized in different values. The deviations must be larger than the user defined tolerances. The frequency and voltage of the 'from' node of its parent [switch] object are manipulated by a player towards the measurements of the 'to' node. Once the deviations are both within the tolerance longer than the user defined period, a 'closure' command should be sent to close its parent [switch] object. This sample use case will be included in the autotest for the [sync_check] object. 

# See also

  * [Requirements]
  * [Implementation]
  * [sync_ctrl]
  * [IEEE Std C50.13TM-2014](https://standards.ieee.org/project/C50_13.html)
