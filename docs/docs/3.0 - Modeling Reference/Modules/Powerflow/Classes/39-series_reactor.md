## Series Reactor

!!! warning
    This page was automatically generated and requires review.

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

#### Properties

**series_reactor** objects are derived from **[link](04-link.md)** objects, so any parameters of the **[link](04-link.md)** object are available as well.

Input indicates properties you can set in models. Output marks properties produced or modified during simulation runtime.

| Property Name | Type | Unit | Input | Output | Description |
| --- | --- | --- | --- | --- | --- |
| phase_A_impedance | complex | Ohm | ✓ |  | Series impedance on phase `A`. |
| phase_A_resistance | double | Ohm | ✓ |  | Series resistance on phase `A`. Maps directly into phase_A_impedance, but allows user to specify real portion separately. |
| phase_A_reactance | double | Ohm | ✓ |  | ⚠️ Reactive portion of phase A&#x27;s impedance |
| phase_B_impedance | complex | Ohm | ✓ |  | Series impedance on phase `B`. |
| phase_B_resistance | double | Ohm | ✓ |  | Series resistance on phase `B`. Maps directly into phase_B_impedance, but allows user to specify real portion separately. |
| phase_B_reactance | double | Ohm | ✓ |  | ⚠️ Reactive portion of phase B&#x27;s impedance |
| phase_C_impedance | complex | Ohm | ✓ |  | Series impedance on phase `C`. |
| phase_C_resistance | double | Ohm | ✓ |  | Series resistance on phase `C`. Maps directly into phase_C_impedance, but allows user to specify real portion separately. |
| phase_C_reactance | double | Ohm | ✓ |  | ⚠️ Reactive portion of phase C&#x27;s impedance |
| rated_current_limit | double | A | ✓ |  | Rated current limit for the reactor. Not used at this time. |

### Series Reactor State of Development

Series reactor has been tested, but not fully validated. 
