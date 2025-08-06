# PQ CONSTANT mode diesel dg

This document describes GridLAB-D implementation of diesel generator in PQ constant mode.   
  
A constant_P mode is implemented in the governor type P_CONSTANT: 

![caption](../../images/700px-Diesel_dg_P_constant_with_actuator_and_time_delay.png)

In the constant_P mode, a time delay is applied to the electric power output from the diesel generator. The delayed electric power output is compared with the constant real power reference, then applied to a PI controller, to get the actuator input. The actuator part and time delay part of the GGOV01 governor is used in constant_P mode. Output of the constant_P mode is the mechanical power of the diesel generator.   
  
The constant_Q mode is implemented based on the existing exciter SEX_PTI: 

![caption](../../images//500px-Diesel_dg_Q_constant.png)

## GridLAB-D Implementation

### Diesel Generator in PQ Constant mode example

This diesel generator object is implemented in both constant P and constant Q mode.   
  
By selecting _Governor_type_ as P_CONSTANT, the constant P mode is selected. This example sets ''Pref'' value as 0.5 p.u. The PI controller settings for the constant Pref mode are 0 for proportional control, and 0.05 for integral control. In addition, parameters of the actuator and time delay part of the P_CONSTANT are also defined in the example.   
  
By selecting _Exciter_Q_constant_mode_ as true, the constant Q mode is selected. This example sets ''Qref'' value as 0.6 p.u.. The PI controller settings for the constant Q mode are 0.01 for proportional control, and 0.05 for integral control. 

### Example of Diesel Generator in PQ Constant mode
    
    
    module generators;
    object diesel_dg {
         flags DELTAMODE;
         parent 8;
         name Gen2;
         Rated_V 4156; //Line-to-Line value
         Rated_VA 1000000; // Defaults to 10 MVA
         Gen_type DYN_SYNCHRONOUS;
         rotor_speed_convergence ${rotor_convergence};
         Exciter_type SEXS;
         Governor_type P_CONSTANT;
         // Actuator and time delay parameters for P_CONSTANT mode
         P_CONSTANT_Tpelec 1.0; // Electrical power transducer time constant, sec. (>0.)
         P_CONSTANT_Tact 0.05; //0.5; // Actuator time constant
         P_CONSTANT_Kturb 1.5; // Turbine gain (>0.)
         P_CONSTANT_wfnl 0.2; //0.2; // No load fuel flow, p.u
         P_CONSTANT_Tb 0.01;//0.1; // Turbine lag time constant, sec. (>0.)
         P_CONSTANT_Tc 0.2; // Turbine lead time constant, sec.
         P_CONSTANT_Teng 0.0; // Transport lag time constant for diesel engine
         P_CONSTANT_ropen 050; // Maximum valve opening rate, p.u./sec.
         P_CONSTANT_rclose -050; // Minimum valve closing rate, p.u./sec.
         P_CONSTANT_Kimw 0.0;//0.002; // Power controller (reset) gain 
         inertia 2.5;
         // PI controller parameters of P_CONSTANT mode
         P_CONSTANT_Pref 0.5; // Set P reference, p.u. 
         P_CONSTANT_kp 0;  // ki for the PI controller implemented in P constant delta mode
         P_CONSTANT_ki 0.05;  // kp for the PI controller implemented in P constant delta mode
         Exciter_Q_constant_Qref 0.6; // Set Q reference, p.u. 
         Exciter_Q_constant_mode true; // Flag indicating whether the diesel generator exciter is operating based on Qref given
         Exciter_Q_constant_kp 0.01;  // ki for the PI controller implemented in Q constant delta mode
    

} 

## Properties

This table lists the properties related to diesel generator in PQ constant mode. Some parameters used by diesel_dg can be found in the diesel_dg wiki page. 

Property name | Type | Unit | Description   
---|---|---|---  
Gen_type | enumeration | none | Defines type of diesel generator (INDUCTION  , SYNCHRONOUS, DYN_SYNCHRONOUS).   
Should choose DYN_SYNCHRONOUS for diesel generator in PQ constant mode.   
rotor_speed_convergence | double | rad | Convergence criterion on rotor speed used to determine when to exit deltamode   
Exciter_type | enumeration | none | Exciter model for dynamics-capable implementation (NO_EXC , SEXS).   
Should choose diesel_dg_type|SEXS for this diesel generator in PQ constant mode.   
Governor_type | enumeration | none | Governor model for dynamics-capable implementation (NO_GOV , DEGOV1, GAST, GGOV1_OLD, GGOV1, P_CONSTANT).   
Should choose P_CONSTANTfor this diesel generator in PQ constant mode.   
Parameters related to P constant mode   
P_CONSTANT_Pref | double | none | Pref value for P constant mode   
P_CONSTANT_kp | double | none | Parameter of the proportional control for constant P mode   
P_CONSTANT_ki | double | none | Parameter of the integration control for constant P mode   
P_CONSTANT_Tpelec | double | s | Electrical power transducer time constant   
P_CONSTANT_Tact | double | s | Actuator time constant   
P_CONSTANT_Kturb | double | none | Turbine gain   
P_CONSTANT_wfnl | double | none | No load fuel flow   
P_CONSTANT_Tb | double | s | Turbine lag time constant   
P_CONSTANT_Tc | double | s | Turbine lead time constant   
P_CONSTANT_Teng | double | s | Transport lag time constant for diesel engine   
P_CONSTANT_ropen | double | /s | Maximum valve opening rate   
P_CONSTANT_rclose | double | /s | Minimum valve closing rate   
P_CONSTANT_Kimw | double | /s | Power controller (reset) gain   
Parameters related to Q constant mode   
Exciter_Q_constant_Qref | double | none | Qref value for Q constant mode   
Exciter_Q_constant_mode | double | none | True if the generator is operating under constant Q mode   
Exciter_Q_constant_kp | double | none | Parameter of the proportional control for constant Q mode   
Exciter_Q_constant_ki | double | none | Parameter of the integration control for constant Q mode   
  
## Test cases

In order to verify the implementation of PQ_CONSTANT mode diesel generator, a test case in 123-bus feeder with one isochronous mode diesel_dg Gen 1, and one PQ_CONSTANT mode diesel_dg Gen 2is applied. At 5.001 second, part of the feeder is disconnected. Gen 1 will reduce its generation, and Gen 2 will maintain its generation after the transient. Below diagram shows the generation from the two generators before and after the transient. 

![PQ_CONSTANT mode diesel generator result](../../images/700px-Diesel_dg_PQ_constant_simulation_result.png)

To run this case, please find in the autotest in GridLAB-D generator module. 

## See Also

  * Modules
    *  Generators
