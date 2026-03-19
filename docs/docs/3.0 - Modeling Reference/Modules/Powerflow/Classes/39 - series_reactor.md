## Series Reactor

The series reactor is a link object designed to model a series reactance on each of the three phases. 
    
    
    object series_reactor {
           from node1;
           to node2;
           phases ABC;
           phase_A_impedance 1+1j;
           phase_B_resistance 2;
           phase_C_reactance 3;
    }
    

### Series Reactor Parameters

Property Name  | Type  | Unit  | Description   
---|---|---|---  
**phase_A_impedance**  | double  | Ohm  | Series impedance on phase `A`.   
**phase_A_resistance**  | double  | Ohm  | Series resistance on phase `A`. Maps directly into phase_A_impedance, but allows user to specify real portion separately.   
**phase_A_impedance**  | double  | Ohm  | Series reactance on phase `A`. Maps directly into phase_A_impedance, but allows user to specify reactive portion separately.   
**phase_B_impedance**  | double  | Ohm  | Series impedance on phase `B`.   
**phase_B_resistance**  | double  | Ohm  | Series resistance on phase `B`. Maps directly into phase_B_impedance, but allows user to specify real portion separately.   
**phase_B_impedance**  | double  | Ohm  | Series reactance on phase `B`. Maps directly into phase_B_impedance, but allows user to specify reactive portion separately.   
**phase_C_impedance**  | double  | Ohm  | Series impedance on phase `C`.   
**phase_C_resistance**  | double  | Ohm  | Series resistance on phase `C`. Maps directly into phase_C_impedance, but allows user to specify real portion separately.   
**phase_C_impedance**  | double  | Ohm  | Series reactance on phase `C`. Maps directly into phase_C_impedance, but allows user to specify reactive portion separately.   
**rated_current_limit**  | double  | Amps  | Rated current limit for the reactor. Not used at this time.   
  
### Series Reactor State of Development

Series reactor has been tested, but not fully validated. 

