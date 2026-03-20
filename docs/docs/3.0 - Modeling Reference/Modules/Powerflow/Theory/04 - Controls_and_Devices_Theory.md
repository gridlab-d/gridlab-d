## Regulators

Single-Phase Y-connected, Single-Phase Δ connected, and Single-Phase Open Δ regulators all are supported in MANUAL control modes. 

Single- and Three-phase Y-connected regulators are supported in multiple automatic control modes, including REMOTE_NODE, OUTPUT_VOLTAGE, and LINE_DROP_COMP. 

Regulators use a supporting regulator configuration object to define the properties. The regulator object itself defines standard link parameters, such as to, from, phases, and configuration. 

REMOTE_NODE uses a remote sensing nodes, named sense_node to control voltage at a given node in the system. Voltage is not corrected (PT & CT ratio are not used), meaning band_center and band_width are given by the actual voltage desired at that node. 

OUTPUT_VOLTAGE looks at the actual output voltage of the regulator at the node it is connected to. Similar to REMOTE_NODE control mode, it looks at the actual voltages at the node. 

Finally, LINE_DROP_COMP is modeled according to Kersting (2007), using a model for a line drop compensator, where current out of the regulator is used to estimate the voltage at a specific point on the system determined by the compensator settings (current_transducer_ratio, power_transducer_ratio, and r & x settings for each individual phase). 

Mechanical and dwelling time changes have also been implemented. Mechanical delays, called by time_delay, indicates the physical amount of time it takes a tap to actually change. Dwelling time, called by dwell_time, is the amount of time the regulator waits between state changes to determine whether the voltage change is a fluctuation or long-term change. 

Matrix equations used are consistent with Kersting (2007), for both Type A & Type B regulators. 

**Example of regulator properties:**
    
    
    object regulator_configuration:1 {
      connect_type              WYE_WYE 
                                (or OPEN_DELTA_ABBC);
      band_center               120.0 V;
      band_width                2.0 V;
      time_delay                15 s;                                    //mechanical time delay
      dwell_time                45 s;                                    //wait time
      raise_taps                16;                                      //upper and lower tap limits
      lower_taps                16;
      current_transducer_ratio  600;                                     //only used for LINE_DROP_COMP
      power_transducer_ratio    60;                                      // | --each phase can be set individually
      compensator_r_setting_A   6.0;                                     // |
      compensator_r_setting_B   6.0;                                     // |
      compensator_r_setting_C   6.0;                                     // |
      compensator_x_setting_A   16.0;                                    // |
      compensator_x_setting_B   16.0;                                    // |
      compensator_x_setting_C   16.0;                                    // -
      regulation                0.1;
      Control                   LINE_DROP_COMP;                          //defines control mode
                                (or REMOTE_NODE, MANUAL, OUTPUT_VOLTAGE)
      Type                      A;
                                (or B)
      tap_pos_A                 0;
      tap_pos_B                 0;                                       //initial tap positions
      tap_pos_C                 0;
    }
    
    
    
    object regulator {
      name          test_regulator;
      from          node_1;
      to            node_2;
      phases        ABCN;
      configuration regulator_configuration:1;
      (sense_node   node_5000;)                                          //used when in REMOTE_NODE control mode
    }
    

## Switch

System line switches are modeled as shown below. 

If in service, _Z_ = 0.0001 

If out of service, _Z_ = ∞ 

$$[a] = \left [ \begin{matrix} 1 & 0 & 0 \\ 0 & 1 & 0 \\ 0 & 0 & 1 \end{matrix} \right ] 

[b] = \left [ Z_{abc} \right ] 

[c] = \left [ \begin{matrix} 0 & 0 & 0 \\ 0 & 0 & 0 \\ 0 & 0 & 0 \end{matrix} \right ] 

[d] = \left [ \begin{matrix} 1 & 0 & 0 \\ 0 & 1 & 0 \\ 0 & 0 & 1 \end{matrix} \right ]$$


$$[A] = \left [ \begin{matrix} 1 & 0 & 0 \\ 0 & 1 & 0 \\ 0 & 0 & 1 \end{matrix} \right ] 

[B] = \left [ Z_{abc} \right ]$$

## Fuse

System line fuses have been modified as a simple over-current device. The equations are shown below. 

If in service, _Z_ = 0.0001 

If out of service, _Z_ = ∞ 

$$[a] = \left [ \begin{matrix} 1 & 0 & 0 \\ 0 & 1 & 0 \\ 0 & 0 & 1 \end{matrix} \right ] 

[b] = \left [ Z_{abc} \right ] 

[c] = \left [ \begin{matrix} 0 & 0 & 0 \\ 0 & 0 & 0 \\ 0 & 0 & 0 \end{matrix} \right ] 

[d] = \left [ \begin{matrix} 1 & 0 & 0 \\ 0 & 1 & 0 \\ 0 & 0 & 1 \end{matrix} \right ]$$


$$[A] = \left [ \begin{matrix} 1 & 0 & 0 \\ 0 & 1 & 0 \\ 0 & 0 & 1 \end{matrix} \right ] 

[B] = \left [ Z_{abc} \right ]$$

## Substation

The substation object in the GridLAB-D™ powerflow module performs two objectives. The substation reads the load_voltage property from the pw_load parent, if present, and converts this positive sequence value to the equivalent balanced three-phase voltages to act as the swing bus voltages for the powerflow solution. The substation takes the three phase unbalanced power solution seen at the substation node, calculates the average power on the phases, and writes this average to the load_power property in the pw_load parent, if present. The substation node also passes positive sequence ZIP components, explicitly set at the substation, to the pw_load parent. In addition, there is a property that allows the user to specify which phase at the substation is the reference phase for the GridLAB-D™ powerflow solution. The substation object is updated to keep track of the three phase power solution. 

Substation is a child class of the node object inside the powerflow module. 

### Equations

Substation uses the following equation to convert the positive sequence value from its pw_load connection (load_voltage) to the three phase balanced voltages used as the swing bus voltage solution. 

$$\begin{bmatrix} \displaystyle V_{A}\\\ & \\\ \displaystyle V_{B}\\\ & \\\ \displaystyle V_{C} \end{bmatrix} = \begin{bmatrix} \displaystyle 1 & \displaystyle 1 & \displaystyle 1 \\\ & \\\ \displaystyle 1 & \displaystyle a^2 & \displaystyle a \\\ & \\\ \displaystyle 1 & \displaystyle a & \displaystyle a^2 \end{bmatrix}*\begin{bmatrix} \displaystyle 0\\\ & \\\ \displaystyle V_{positive sequence}\\\ & \\\ \displaystyle 0 \end{bmatrix}*b$$

Where $b$ is conditional upon which phase is chosen as the reference phase, $a$ is the complex number $1\angle120^\circ$, and all other variables are complex. The values of $b$ for each of the possible reference phases are shown below. 

Reference Phase  | b   
---|---  
Phase A  | 1   
Phase B  | $a$  
Phase C  | $a^2$  
  
The average phase load is determined by the below equation. All variables are complex values not the real power loads. 

$$P_{load} = \frac{P_{A}+P_{B}+P_{C}}{3}$$

A typical substation implementation is 
    
    
    object substation {
    	name SubS;
    	bustype SWING;
            parent network_node;
            reference_PHASE_A;
            phase ABCN;
    	nominal_voltage 7199.558;
    }
    

## Capacitor

The capacitor object provides a method to attach reactive compensation into the distribution system. Two principle modes are available to the capacitor operation: manual and automatic. Under the manual mode, capacitors are switched on by the system modeler explicitly. In one of the automatic methods, the capacitor object will switch capacitors on and off based on criteria specified. 

The physical configuration of the capacitors is dictated by the input `phases_connected`. Using a syntax identical to the `phases` keyword in all powerflow objects, the configuration of the attached capacitors can be defined. For example, if 
    
    
    phases_connected A;
    

were passed as the argument in the input, a single wye-connected capacitor would be connected to phase A of the system. To obtain a delta connection, a `D` value must be specified in the `phases_connected` property. If a delta-connected capacitor bank on phases AB and BC were desired, the input file would specify 
    
    
    phases_connected ABCD;
    

It is important to note that in this case, a phase connection between CA is also implied. To obtain the open-CA configuration, the `capacitor_C` value would need to be set to 0 VAr. 

Capacitance values are defined as the nominal reactive power the capacitor can provide. This will be expressed in terms of Volt-Amps reactive or VArs. As alluded to, these values are specified on inputs `capacitor_A`, `capacitor_B`, and `capacitor_C` on the system. To convert this value into an equivalent reactance, this nominal power must be converted into a nominal impedance value. This is accomplished in one of two ways. The first is to specify the nominal voltage rating of the capacitor via the `cap_nominal_voltage` variable. This variable is useful for connections other than the default powerflow connection (e.g. a delta-connected capacitor bank on a wye-connected system) or where the nominal voltage rating of the capacitor is different than the surrounding powerflow objects. In the absence of this variable, the standard nominal voltage of the system at that point, `nominal_voltage`, is used. 

Regardless of the nominal voltage used, the impedance value is calculated in the same manner. Using the constant impedance calculation for the load object, the capacitor impedance is determined by 

$$\displaystyle{} Z_{cap} = \left(\frac{V \cdot{} V^{*}}{P}\right)^{*}$$

where $V$ is the nominal voltage, $P$ is the nominal power rating, and $*$ again represents the complex conjugate operator. Once obtained, this impedance value is applied as a load to the system any time the appropriate switch of the capacitor is closed. During open periods, an impedance of $Z_{cap} = \infty{}$ is used. 

As mentioned, the capacitor has two primary modes of operation. When the `control_method` variable is set to `MANUAL`, the capacitor switches are controlled explicitly using the `switchA`, `switchB`, and `switchC` inputs to the object. Simple `OPEN` or `CLOSED` values indicate whether the switch should be open or closed during the analysis. 

The capacitor has three automatic methods of determining the switching point. The first of these is a voltage control method set using `control_method VOLT`. Two set-points are defined on the input parameters of the capacitor. When the voltage measured falls below the value specified in `voltage_set_low`, the capacitor for that phase is switched in. When the voltage exceeds the value of `voltage_set_high`, the capacitor switches off if it was already on. Proper spacing of the two set-points provides a deadband of operation and prevents the capacitor bank from constantly switching off and on. 

A second automatic method is determined using the reactive power value at a measurement point. With `control_method` specified as `VAR`, the reactive power is monitored against specific threshold points. Under the VAr control method, if the VAr value goes below the `VAr_set_low` value, the capacitor will switch off. If the VAr value exceeds the `VAr_set_high` parameter, the corresponding capacitor will be switched on. Once again, set point spacing is important to prevent excessive capacitor switching. 

A third automatic method is a hybrid method of the first two. When `control_method` is set to `VARVOLT`, both set point pairs will be evaluated. The capacitor behaves basically like the `VAR` control scheme. Normally, the capacitor is switched in and out based off the measured reactive power and its relation to the `VAr_set_low` and `VAr_set_high` parameters. However once a switching action occurs, the system checks an additional safety threshold. The capacitor examines the voltage magnitude to see if it exceeds `voltage_set_high`. If this voltage limit is exceeded, that particular phase of the capacitor is switched off (or prevented from switching on) and locked out for a duration specified in `lockout_time`. After `lockout_time` has cleared, the capacitor tries to resume switching operations on that phase. 

For all of the automatic control schemes, control is provided to only monitor certain phases of the system. The input parameter `pt_phase` is used to specify which phase to monitor for switching operations. This parameter takes the same format as `phases_connected` with values of A, B, C, and D. For example, if voltage control was being used as an input of 
    
    
    pt_phase BCD;
    

was provided, the BC line-line voltage would be monitored to determine the switching point of the capacitors. 

When the `VAR` or `VARVOLT` control scheme is selected, a remote line must be specified for VAr monitoring. This is accomplished by specifying the `remote_sense` property with the name of the line. For example, if we wanted to monitor a line called Bus401to402 we would include 
    
    
    remote_sense Bus401to402;
    

in the input parameter list to the capacitor. Once this line is specified, all VAr-related switching operations will be performed based on the quantity measured from this line. 

Under default operation, the `VOLT` and `VARVOLT` control schemes only monitor the local voltage of the node to which the capacitor is connected. To offer further flexibility and reliability, it is often desirable to switch the capacitor based on voltage conditions elsewhere in the system. Often times, this will be a node downstream in the feeder. To utilize a remote node's values instead of the attachment point of the capacitor, two different parameters are utilized. 

Under the `VOLT` control scheme, the `remote_sense` parameter can be used to specify the remote node by its name. Similar to the VAr monitoring method, if a bus with the name Bus754 was the desired reference point for capacitor switching decisions, 
    
    
    remote_sense Bus754;
    

would be specified on the input parameter list to the capacitor. 

Given the nature of the `VARVOLT` control scheme, a remote line to monitor must always be specified. Therefore, if it is desired to monitor the voltage on a remote node under this scheme, the `remote_sense_B` parameter must be specified. This merely provides a second remote measurement point for the capacitor to monitor. For the `VARVOLT` control method, the remote line and node specifications could be interchanged without issue. If the examples above are combined, a `VARVOLT` controlled capacitor that monitors the reactive power on line Bus401to402 and the voltage on node Bus754 would have 
    
    
    remote_sense Bus401to402;
    remote_sense_B Bus754;
    

in its parameter list. It is important to note that these two parameters are only BOTH active for the `VARVOLT` case. Furthermore, if both are used, one must specify a line and the other must specify a node. The `remote_sense_B` parameter can not be used to monitor a second line of interest. 

The capacitor object handles two variations on the capacitor operation. The input parameter `control_level` lets the modeler specify if all capacitors will be operated individually or as a bank. With 
    
    
    control_level INDIVIDUAL;
    

each capacitor switch is operated independently. However, if 
    
    
    control_level BANK;
    

is specified, the capacitors are either all switched in or all switched off based on one phase of interest. Coordinating with the `pt_phase` variable, if the monitored phase of interest on phase C indicated a switching condition, the switches to capacitors A, B, and C would all close. When C requested an open condition, all three phases would then open. 

To provide further functionality in the capacitor, two time delays are also available. The first of these time delays represents a mechanical switching delay. The value of `time_delay` will specify how many seconds after a change is requested on the capacitors that the switch actually responds. 

The second delay, `dwell_time`, represents a "required period of consistency" before switching. For example, if 
    
    
    dwell_time 2.0;
    

were specified, the system would need to request the same capacitor action over two seconds before the switch would be actuated (which would then be subject to the mechanical delay specified in `time_delay`). This is useful to prevent large, single second long "transient" spikes from erroneously switching the capacitors. 

The `dwell_time` and `time_delay` values are both utilized by the automatic control schemes. However, only `time_delay` is factored into manual operation of the capacitors. Any system "consistency" intervals are left to the modeler to determine and monitor. 

## Coordinated Volt-Var Control

The Coordinated Volt-VAr Control (CVVC) object provides a means to coordinate regulators and capacitors on a distribution feeder, or group of feeders. Using the logic from Borozan et al. (2001), a desired voltage level and reactive power compensation for a group of feeders is controlled by a central entity. 

The CVVC object works by examining two collections of measurements. The first set of these are voltage measurement points throughout the system. These points are typically `node` objects specified by name. Utilizing a combination of these points, and which regulator they are attached to, the CVVC object adjusts regulator tap positions to maintain a desired voltage. 

The secondary set of measurements comes from a `link`-based object somewhere in the system. This link is typically at the top of all the feeders, and may link the transmission network to the distribution (via a subtransmission network). Using the logic outlined in Borozan et al. (2001), the capacitors in the system are switched in an out based on size and distance from the feeder. The capacitor changes only occur during times when no regulator change is present. If a regulator is still changing, the reactive power logic is deferred as small reactive power changes can occur from the voltage adjustments. 

The CVVC object supports activation and deactivation during a simulation. By changing the `control_method` variable, the system can transition between an active and standby states. This attempts to provide insight into how the system may behave if only operated during certain periods of the day. When in standby mode, capacitors and regulators are returned to their original control modes (whether that be automatic or not). 
