## Substation

!!! warning
    This page was automatically generated and requires review.

Substations were used to connect distribution powerflow in the **powerflow** module with PowerWorld through the **network** module. The **substation** object converts the sequence voltage provided by the **network** module to three-phase swing bus voltage for the unbalanced three-phase powerflow solution. The **substation** object also passes the unbalanced three-phase powerflow solution to back to the **network** module as an single power value representing the average load on all three phases of the swing bus. Furthermore, the **substation** object sets which phase is the reference phase for the distribution powerflow. A typical substation implementation is 
    
    
    object substation {
    	name SubS;
    	bustype SWING;
            parent network_node;
            reference_PHASE_A;
            phase ABCN;
    	nominal_voltage 7199.558;
    }
    

In order to properly connect the substation object to the **network** module, the substation object's parent must be a pw_load object. 

### Substation Parameters

#### Properties

**substation** objects are derived from **[node](03-node.md)** objects, so any parameters of the **[node](03-node.md)** object are available as well.

Input indicates properties you can set in models. Output marks properties produced or modified during simulation runtime.

| Property Name | Type | Unit | Input | Output | Description |
| --- | --- | --- | --- | --- | --- |
| zero_sequence_voltage | complex | V | ✓ | ✓ | ⚠️ The zero sequence representation of the voltage for the substation object. |
| positive_sequence_voltage | complex | V | ✓ | ✓ | The positive sequence voltage given from a PowerWorld bus model or a user specified value. |
| negative_sequence_voltage | complex | V | ✓ | ✓ | ⚠️ The negative sequence representation of the voltage for the substation object. |
| base_power | double | VA | ✓ |  | ⚠️ The 3 phase VA power rating of the substation. |
| power_convergence_value | double | VA | ✓ |  | ⚠️ Default convergence criterion before power is posted to pw_load objects if connected, otherwise ignored |
| reference_phase | enumeration | N/A | ✓ |  | The phase that will be used as the reference angle for the powerflow solution. &lt;br/&gt; - PHASE_A(Default) &lt;br/&gt; - PHASE_B &lt;br/&gt; - PHASE_C Valid values: `PHASE_A`, `PHASE_B`, `PHASE_C`. |
| transmission_level_constant_power_load | complex | VA | ✓ |  | The positive-sequence constant power load to be posted directly to the pw_load object ([powerflow] solver does not handle this, it is explicitly converted and posted to PowerWorld&#x27;s solver). |
| transmission_level_constant_current_load | complex | A | ✓ |  | the positive-sequence constant current load to be posted directly to the pw_load object ([powerflow] solver does not handle this, it is explicitly converted and posted to PowerWorld&#x27;s solver). |
| transmission_level_constant_impedance_load | complex | Ohm | ✓ |  | The positive-sequence constant impedance load to be posted directly to the pw_load object ([powerflow] solver does not handle this, it is explicitly converted and posted to PowerWorld&#x27;s solver). |
| distribution_load | complex | VA |  | ✓ | ⚠️ The total load of all three phases at the substation object. |
| distribution_power_A | complex | VA |  | ✓ | The measured power of the attached powerflow on phase A. |
| distribution_power_B | complex | VA |  | ✓ | The measured power of the attached powerflow on phase B. |
| distribution_power_C | complex | VA |  | ✓ | The measured power of the attached powerflow on phase C. |
| distribution_voltage_A | complex | V |  | ✓ |  |
| distribution_voltage_B | complex | V |  | ✓ |  |
| distribution_voltage_C | complex | V |  | ✓ |  |
| distribution_voltage_AB | complex | V | ✓ |  |  |
| distribution_voltage_BC | complex | V | ✓ |  |  |
| distribution_voltage_CA | complex | V | ✓ |  |  |
| distribution_current_A | complex | A | ✓ |  |  |
| distribution_current_B | complex | A | ✓ |  |  |
| distribution_current_C | complex | A | ✓ |  |  |
| distribution_real_energy | double | Wh |  | ✓ |  |
| measured_reactive | double | kVar | ✓ |  |  |

### Substation State of Development

The only transmission powerflow software that substation currently works with is PowerWorld. Please note that the specific the transmission current and impedance loads are converted to complex power values first and then posted to the proper properties(load_current and load_impedance) in the pw_load object. The average_transmission_power_load value must be added to the average_distribution_load before posting to pw_load(load_power). 

Substation's three phase voltages are determined differently dependent upon three scenarios. If there is a pw_load object attached to the substation object, then the three phase voltages are determined by the sequence voltage value read from the pw_load object. The three phase voltages are determined by the positive_sequence_voltage property if there is a player object populating that property in the absence of a pw_load object. In the absence of a player object and a pw_load object, the three phase voltages are determined by user input or the substation object's powerflow parent just like any node object. If there is no pw_load connected to the substation then the substation doesn't post the average_distribution_load, average_transmission_current_load, average_transmission_impedance_load, and average_transmission_power_load properties. 
