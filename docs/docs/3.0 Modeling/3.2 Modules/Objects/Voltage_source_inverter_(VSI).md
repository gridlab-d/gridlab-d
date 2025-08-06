# Voltage source inverter (VSI)

**Source URL:** https://gridlab-d.shoutwiki.com/wiki/Voltage_source_inverter_(VSI)

## Overview

This document describes GridLAB-D implementation of voltage source inverter (VSI). VSI is implemented in a similar way as the [diesel_dg]. A Norton current source behind an admittance is used to represent the VSI. A Thevenin voltage source is then converted from the Norton source for the calculation of voltage source _e_source_ magnitude and angle. 

![VSI Norton to Thevenin](../../../images/500px-VSI_Norton_to_Thevenin.png)

Two VSI modes are implemented: isochronous mode and droop mode. 

  * **The isochronous mode VSI**
    The isochronous mode VSI is usually placed at the SWING bus, with constant voltage source _e_source_ angle. In this way, frequency of the VSI is assumed to be constant. In order to keep the bus voltage at VSI at a reasonable range, the measured VSI terminal voltage is compared to the nominal voltage, and a PI voltage controller is used to obtain the magnitude of _e_source_.

![Voltage control of e_source](../../../images/400px-Voltage_control_of_e_source.png)

The slew rate limits for both real and reactive power have been implemented in the isochronous mode VSI. An export function in the inverter object is defined to update the VSI current injection _IGenerated_ to the grid in each power flow iteration.
In each power flow iteration in solver_nr.cpp, if the VSI bus has the export function (_current_injection_update_) mapped, the export function will be executed. In this function, when the ramp rate check for real power is enabled, the real power output change from the last time step (either delta-mode time step, or steady-state mode time step) is calculated, and compared with the ramp rate limit. The ramp rate up and down limits have been defined in glm file, or given as default values. If the change exceeds the limit, the real power value is updated based on the value in the last time step, and the ramp rate limit. The updated real power output will also change the VSI current injection _IGenerated_ , as well as the _e_source_ value correspondingly.  


In addition, in the delta-mode _inter_deltaupdate_ function, real and reactive power outputs changes are compared to predefined/default real and reactive slew rate respectively, when real and reactive power slew rate check is enabled.   


To be noticed, both real and reactive power slew rate check cannot be enabled in the same time with more than one VSI in the feeder, or power flow converge problem will appear.

* **The droop mode VSI**
The droop mode VSI includes two droops: _f_ /_p_ and _v_ /_q_. Based on measured (delayed) power outputs from VSI, the corresponding frequency (and therefore _e_source_ angle) and _e_source_ magnitude are updated.

![VSI Droop](../../../images/500px-VSI_droop.png)

The slew rate limits for both real and reactive power have been implemented in the droop mode VSI. Similar to the implementation for the isochronous mode VSI, the real power slew rate check is executed in each power flow iteration. In addition, the real and reactive power changes are examined in each delta mode time step in _inter_deltaupdate_ function.  


To be noticed, both real and reactive power slew rate check cannot be enabled in the same time with more than one VSI in the feeder, or power flow converge problem will appear.

Battery is required to be attached to VSI with enough energy stored. Simulations of VSI can still be run without the attachment of battery, but battery is added if taking into account the reality. The battery is working through the inverter object, with the state-of-charge updated in the battery object, and power outputs calculated from the inverter object. Currently if there is no Battery attached to VSI, a warning will be given, and the VSI is assumed to be attached to an infinite power input. 

## GridLAB-D implementation

### Isochronous mode VSI object example

This inverter object is a VSI in isochronous mode. The VSI is implemented under the inverter type FOUR_QUADRANT. The droop mode VSI is chosen by selecting four_quadrant_control_mode as VOLTAGE_SOURCE, VSI_mode as VSI_ISOCHRONOUS (by default VSI_mode is VSI_ISOCHRONOUS for VSI objects), use_multipoint_efficiency as FALSE, generator_status as ONLINE. The dynamic_model_mode is always given as PI for VSI objects.  
  
The isochronous mode VSI is placed on a SWINGbus. The power rating per phase given for this VSI is 1 MVA. At the first time step of the simulation, the real and reactive power outputs from the VSI are calculated based on power flow solutions, rather than based on the P_Out and Q_Out given. After entering into the delta mode, the _e_source_ angle is kept constant, and magnitude is adjusted using a PI voltage controller, with the integrator gain _ki_Vterminal_ given as 0.01, and the proportional gain _kp_Vterminal_ given as 0.1. The default filter impedance of the VSI is given as 0.0025 + j0.06 p.u..   
  
The real power slew rate is enabled in this example, with maximum ramp down rate defined as 25 MW/s, and maximum ramp up rate given as 30 MW/s. 

### Example of Isochronous mode VSI
    
    
    module generators;
    object inverter {
         parent 150;
         name VSI;
         inverter_type FOUR_QUADRANT;
         use_multipoint_efficiency FALSE;
         four_quadrant_control_mode VOLTAGE_SOURCE;
         generator_status ONLINE;
         inverter_efficiency 1;
         enable_ramp_rates_real true;
         max_ramp_down_real 25.0 MW/s;
         max_ramp_up_real 30.0 MW/s;
         enable_ramp_rates_reactive false;
         max_ramp_down_reactive 100 MVAr/s;
         max_ramp_up_reactive 100 MVAr/s;
         rated_power 10000 kVA; // Per phase rating
         VSI_Rfilter 0.0025;
         VSI_Xfilter 0.06;
         P_Out 9000000;
         Q_Out 3000000;
         V_In 3000+0j;
         I_In 2000+0j;
         flags DELTAMODE;
         dynamic_model_mode PI;
         inverter_convergence_criterion 0.001;
         ki_Vterminal 0.01;
         kp_Vterminal 0.1;
    }
    

### Droop mode VSI object example

This inverter object is a VSI in droop mode. The VSI is implemented under the inverter type FOUR_QUADRANT. The droop mode VSI is chosen by selecting four_quadrant_control_mode as VOLTAGE_SOURCE, VSI_mode as VSI_DROOP, use_multipoint_efficiency as FALSE, generator_status as ONLINE. The dynamic_model_mode is always given as PI for VSI objects.   
  
The power rating per phase given for this VSI is 1000 kVA. Before entering the delta mode, the real and reactive power outputs from the VSI are equal to the given P_Out and Q_Out values, which are 1 MW and 1MVar respectively. After entering into the delta mode, the power outputs will be based on the droop curve parameters defined. The filter impedance of the VSI is given as 0.0025 + j0.06 p.u..   
  
The reactive power slew rate is enabled in this example, with both maximum ramp down rate and maximum ramp up rate given as 20 MW/s or MVAr/s. 

### Example of Droop mode VSI
    
    
    module generators;
    object inverter {
         parent 150;
         name VSI;
         inverter_type FOUR_QUADRANT;
         use_multipoint_efficiency FALSE;
         four_quadrant_control_mode VOLTAGE_SOURCE;
         generator_status ONLINE;
         inverter_efficiency 1;
         enable_ramp_rates_real false;
         max_ramp_down_real 15.0 MW/s;
         max_ramp_up_real 15.0 MW/s;
         enable_ramp_rates_reactive true;
         max_ramp_down_reactive 20 MVAr/s;
         max_ramp_up_reactive 20 MVAr/s;
         rated_power 10000 kVA; // Per phase rating
         P_Out 1000000;
         Q_Out 1000000;
         V_In 1000+0j;
         I_In 1000+0j;
         flags DELTAMODE;
         dynamic_model_mode PI;
         inverter_convergence_criterion 0.001;
         VSI_Rfilter 0.0025;
         VSI_Xfilter 0.06;
         // Droop curve parameters
         Tp_delay 0.01;
         R_fp 0.00000001;
         Tq_delay 0.01;
         R_vq 0.0000005;
    }
    

## Properties

This table lists the properties related to VSI implementation. Some parameters used by other types of inverters can be found in the inverter wiki page. 

Property name | Type | Unit | Description   
---|---|---|---  
inverter_type | enumeration | none | Defines type of inverter technology and efficiency of the unit (FOUR_QUADRANT , PWM, TWELVE_PULSE, SIX_PULSE, TWO_PULSE).   
Should choose FOUR_QUADRANT for VSI object.   
four_quadrant_control_mode | enumeration | none | Control mode of the inverter when FOUR_QUADRANT (NONE, CONSTANT_PQ, CONSTANT_PF, CONSTANT_V, VOLT_VAR, VOLTAGE_SOURCE).  
Should choose VOLTAGE_SOURCE for VSI object.   
VSI_mode | enumeration | none | Mode of VSI (VSI_ISOCHRONOUS, VSI_DROOP)   
use_multipoint_efficiency | bool | none | A boolean flag to toggle using Sandia National Laboratory's multipoint efficiency model   
generator_status | enumeration | none | Defines if generator is in operation or not (ONLINE, OFFLINE)   
inverter_efficiency | double | none | four_quadrant_control_mode:The efficiency of the inverter  
rated_power | double | [VA] | four_quadrant_control_mode:The per phase rated power of the inverter  
P_Out | double | [VA] | Value to output in four quadrant control mode CONSTANT_PQ and VOLTAGE_SOURCE  
Q_Out | double | [VAr] | Value to output in four quadrant control mode CONSTANT_PQ and VOLTAGE_SOURCE  
V_In | complex | [V] | DC voltage passed in by the DC object (e.g. solar panel or battery)   
I_In | complex | [A] | DC current passed in by the DC object (e.g. solar panel or battery)   
flags | unit32 | none | Object flag to be used for indication of delta mode inclusion   
dynamic_model_mode | enumeration | none | DELTAMODE: Underlying model to use for deltamode control   
inverter_convergence_criterion | double | none | The maximum change in error threshold for exiting deltamode   
VSI_Rfilter | double | none | VSI filter resistance (p.u.). Default value is 0.03 p.u.   
VSI_Xfilter | double | none | VSI filter inductance (p.u.). Default value is 0.3 p.u.   
enable_ramp_rates_real | bool | none | Flag indicating whether the ramp rate check for real power is enabled   
max_ramp_down_real | double | [W/s] | Maximum real power rap down change   
max_ramp_up_real | double | [W/s] | Maximum real power rap up change   
enable_ramp_rates_reactive | bool | none | Flag indicating whether the ramp rate check for reactive power is enabled   
max_ramp_down_reactive | double | [VAr/s] | Maximum reactive power rap down change   
max_ramp_up_reactive | double | [VAr/s] | Maximum reactive power rap up change 

Parameters related to isochronous mode   

Property name | Type | Unit | Description   
---|---|---|---  
ki_Vterminal | double | none | DELTAMODE: The integrator gain for the VSI terminal voltage modulation. Default value is 0.01.   
kp_Vterminal | double | none | DELTAMODE: The proportional gain for the VSI terminal voltage modulation. Default value is 0.1.  

Parameters related to droop mode 
Property name | Type | Unit | Description   
---|---|---|---    
Tp_delay | double | [s] | DELTAMODE: The time constant for delayed real power seen by the VSI droop controller.   
Tq_delay | double | [s] | DELTAMODE: The time constant for delayed reactive power seen by the VSI droop controller.   
R_fp | double | none | DELTAMODE: The droop parameter of the f/p droop.   
R_vq | double | none | DELTAMODE: The droop parameter of the v/q droop.   
  
## Test cases

### Case 1

In IEEE 123-bus feeder, one VSI in isochronous mode is placed at swing bus, and one VSI in droop mode is placed at another bus.

Part of the feeder is disconnected at 12:00:05.0001 PST. As seem from the real power outputs result in the figure below, since the frequency is kept to 60 HZ by the isochronous VSI, the real power output of the droop VSI is kept the same. The real power outputs from isochronous VSI reduces after disconnection of the feeder.   
At 12:00:8.001 PST, the part of the feeder is reconnected. Outputs from the two VSIs return to initial values.   


![IsochronousVSI](../../../images/700px-IsochronousVSI_droopVSI.png)

### Case 2

In IEEE 123-bus feeder, one VSI in droop mode is placed at swing bus, and one VSI in droop mode is placed at another bus.   

Part of the feeder is disconnected at 12:00:05.0001 PST. As seem from the real power outputs result in the figure below, real power outputs decrease at two VSIs. With the reduction of real power outputs, frequency of the VSI increases, as seen from the second figure below.   


![TwoDroopVSI](../../../images/700px-TwoDroopVSIs.png)

![Frequency](../../../images/700px-Frequency.png)

  
To run these cases, please find in the autotest in GridLAB-D generator module. 

## See Also

  * [Modules]
    * [ Generators]
