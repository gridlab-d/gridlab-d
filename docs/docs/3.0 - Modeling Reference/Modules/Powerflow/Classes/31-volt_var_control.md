## Volt Var Control

With multiple feeders attached to a common point, it is often useful to coordinate the voltage regulators and capacitors on the system. The **volt_var_control** object coordinates selected **regulator** and **capacitor** objects on the system. Using voltage measurements at **node** object points, the **volt_var_control** tries to maintain a desired voltage. In addition to voltage measurements, the **volt_var_control** utilizes a power measurement at a **link** object to determine how to switch various **capacitor** objects on the system in and out of service. Due to differences in the timing of power calculations in the Forward-Back Sweep and Newton-Raphson powerflow solvers, capacitors may switch at slightly different intervals for the same system. The overall control behaves the same in both solver methods, but this difference in capacitor timing may result in different final operating points. A typical Volt-VAr Controller implementation is 
    
    
    object volt_var_control {
    	name IVVC37;
    	control_method ACTIVE;
    	capacitor_delay 10.0;
    	regulator_delay 5.0;
    	desired_pf 0.98;
    	d_max 0.8;
    	d_min 0.1;
    	substation_link "SubTransNode-799";
    	regulator_list "reg799-781,regnode799-U0081";
    	capacitor_list "CapNode_A,CapNode_B";
    	voltage_measurements "load829,1,load841,1,load825,1,U0029,2,U0041,2,U0025,2";
    	minimum_voltages 2500.0;
    	maximum_voltages 3000.0;
    	desired_voltages 2600.0;
    }
    

Many of the parameters on the Volt-VAr Controller can be left unspecified. When unspecified, default values or criteria are enacted. A minimalist implemenation would look similar to 
    
    
    object volt_var_control {
    	name IVVC37;
    	regulator_list "reg799-781,regnode799-U0081";
    	capacitor_list "CapNode_A,CapNode_B";
    }

### Volt Var Control Parameters

#### Properties

**volt_var_control** does not declare inherited parent classes.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

Table: 31-volt_var_control table 1 { #tbl:31-volt-var-control-1 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| control_method | enumeration | N/A | I | Defines the control scheme the **volt_var_control** object is currently operating in. There are two modes currently supported: <br/> - `STANDBY` \- The **volt_var_control** is inactive and all **regulator** and **capacitor** control is handled by their local definitions. <br/> - `ACTIVE` \- The **volt_var_control** is actively adjusting the regulators of interest, as well as coordinating capacitor insertions and removals. This is the default state. |
| capacitor_delay | double | s | I | Default delay for any **capacitor**s under the **volt_var_control** object's control. If a delay is not explicitly defined in the **capacitor**'s local properties, this value will be used as the time delay for all switching operations. Defaults to 5.0 seconds. |
| regulator_delay | double | s | I | Default delay for any **regulator**s under the **volt_var_control** object's control. If a delay is not explicityly defined in the **regulator_configuration** object association with the **regulator**, this value will be used as the time delay in all tap-changing operations. Defaults to 5.0 seconds. |
| desired_pf | double | N/A | I | The value for **desired_pf** is used for setting threshold value for switching **capacitors** in and out of the system. It is determined at the `substation_link` object. If `pf_signed` is set to true, a negative value represents an inductive power factor (lagging) desired and a positive value represents a capacitive power factor (leading). Defaults to 0.98. |
| d_max | double | N/A | I | Scaling constant for switching the **capacitor**s on as a ratio of their size to the required reactive power correction. Typically between 0.3 and 0.6. It must not overlap with `d_min`, and a larger spread between the two values helps prevent **capacitor** switching oscillations. Defaults to 0.6. |
| d_min | double | N/A | I | Scaling constant for switching the **capacitor**s off as a ratio of their size and the required reactive power correction. Typically between 0.1 and 0.4. It must not overlap with `d_max`. As with `d_max`, a larger spread between `d_min` and `d_max` will help prevent **capacitor** switching oscillations. Defaults to 0.3. |
| substation_link | object | N/A | I | Defines the **link** to extract power information from for power factor correction. This object must be of the main-type **link**. Typically, this will be attached to a subtransmission-level **transformer** object to monitor reactive power for the entire network. If unpopulated, this defaults to monitoring the power through the first regulator specified in `regulator_list`. |
| pf_phase | set | N/A | I | Defines the phases of `substation_link` to monitor and accumulate for the power factor calculations. `pf_phase` can be any combination of `PHASE_A`, `PHASE_B`, or `PHASE_C`. If left blank, `pf_phase` will default to the phases of the `substation_link` object. Valid values: `A`, `B`, `C`. |
| regulator_list | char1024 | N/A | I | List of regulators for the **volt_var_control** object to control. The list is a comma-separated object name for each regulator for which control is desired. No trailing comma is required. One regulator would be specified as `regulator_list reg799781;`, while three would be `regulator_list reg799781,reg981,reg01;`. |
| capacitor_list | char1024 | N/A | I | List of capacitors for the **volt_var_control** object to control. This list is also comma-separated, like the `regulator_list`. If no capacitors are specified, no reactive adjustments are performed. All capacitors are operated in banked mode by the **volt_var_control** object. The listed capacitors are sorted by size and distance for switching operations. An example capacitor list is `capacitor_list CapA,CapB,CapC`. |
| voltage_measurements | char1024 | N/A | I | List of end-of-line measurements for the **volt_var_control** to use in **regulator** control. This list is comma-separated, like the `capacitor_list` and `regulator_list`. If left blank, the measurement point defaults to load side of each **regulator** in `regulator_list`. The list can take two formats, depending on the number of regulators: <br/> - One regulator - The list is a comma-separated list of just the object names. No regulator designation is necessary, since only one regulator is present. An example is br/> - `measurement_list NodeA,NodeB,NodeC,NodeD;`. <br/> - One or more regulators - The list is a comma-separated list of object names, and their associated regulator. All measurements must be represented as a pair in the list. The first item is the object name, and the second item is the **regulator** position in the `regulator_list` variable. For example, `measurement_list NodeA,1,NodeC,2,NodeB,1;` will assign `NodeA` and `NodeB` to regulator 1, and `NodeC` to regulator 2. Notice these can occur in any regulator order, as long as the second portion of the data pair is the regulator number. |
| minimum_voltages | char1024 | N/A | I | List of minimum voltages for the **volt_var_control** object to maintain. If only 1 value is specified, this value will be used for all **regulator**s in `regulator_list`. A value can be specified for each **regulator** as a comma-separated list. For example, to specify two minimum voltage levels for two different regulators, use `minimum_voltages 2600,2500;`. If left blank, the minimimum voltage is set at 0.95 p.u. of the **regulator**'s TO node `nominal_voltage`. |
| maximum_voltages | char1024 | N/A | I | List of maximum voltages for the **volt_var_control** object to maintain. If only 1 value is specified, this value will be used for all **regulator**s in `regulator_list`. A value can be specified for each **regulator** as a comma-separated list. For example, to specify two maximum voltage levels for two different regulators, use `maximum_voltages 4500,4400;`. If left blank, the maximimum voltage is set at 1.05 p.u. of the **regulator**'s TO node `nominal_voltage`. |
| desired_voltages | char1024 | N/A | I | List of desired, or target voltages for the **volt_var_control** object to maintain. If only 1 value is specified, this value will be used for all **regulator**s in `regulator_list`. A value can be specified for each **regulator** as a comma-separated list. For example, to specify two desired voltage levels for two different regulators, use `desired_voltages 4500,4400;`. If left blank, the desired voltage is set at the **regulator**'s TO node `nominal_voltage`. |
| max_vdrop | char1024 | N/A | I | List of voltage drop thresholds for high or low loading operation (selection of `high_load_deadband` or `low_load_deadband` for a regulator). If the voltage drop between the regulator and the lowest end-of-line measurement is greater than `max_vdrop`, the corresponding `high_load_deadband` is used. Otherwise, the corresponding `low_load_deadband` is used. `max_vdrop` can be a comma-separated list of values for each regulator of `regulator_list`. if only one value is specified, that value will be used for all regulators. If left blank, `max_vdrop` defaults to 1.5x the corresponding **regulator** object's step up tap voltage value. |
| high_load_deadband | char1024 | N/A | I | List of tap-changing bandwidth thresholds for high loading operation (as determined by `max_vdrop`). `high_load_deadband` represents a +/- `high_load_deadband` deadband around the desired voltage before a tap change is requested on the regulator. This can be specified for each regulator as a comma-separated list. If a single value is specified, that value will be used on all **regulator**s in the `regulator_list`. If unspecified, `high_load_deadband` defaults to the voltage value associated with a single tap change on the **regulator**. |
| low_load_deadband | char1024 | N/A | I | List of tap-changing bandwidth thresholds for low loading operation (as determined by `max_vdrop`). `low_load_deadband` represents a +/- `low_load_deadband` deadband around the desired voltage before a tap change is requested on the regulator. This can be specified for each regulator as a comma-separated list. If a single value is specified, that value will be used on all **regulator**s in the `regulator_list`. If unspecified, `low_load_deadband` defaults to the voltage value associated with a two tap change on the **regulator** (2x the default of `high_load_deadband`). |
| pf_signed | bool | N/A | I | Set to true to consider the sign on the power factor.  Otherwise, it just maintains the deadband of +/-desired_pf |

### VoltVar Control State of Development

VoltVar Control is considered a well developed model, but has not been fully validated at this time. Advanced features and additional controls may be added as needed. 
