## Triplex Node

!!! warning
    This page was automatically generated and requires review.

Triplex nodes represent special cases of the **node** object. The **triplex_node** object still serves as connection point between different links of the system and a point of measurable voltage. However, **triplex_node**s are casted to represent phases `1`, `2`, and `N` rather than `A`, `B`, and `C` like normal **node** objects. Simplified, they operate in the split-phase level of distribution rather than the three-phase level. 

Since **load** objects are directly derived from **node** objects, they are only valid for three-phase connections as well. Therefore, the **load** functionality has been built into the **triplex_load** object for split-phase level systems. 

It is important to note that triplex-based objects should include the phase `S` somewhere in their designation. 

A typical **triplex_node** implementation is 
    
    
    object triplex_node {
    	name TPL_tAS;
    	phases AS;
    	voltage_1 120 + 0j;		
    	voltage_2 120 + 0j;
    	voltage_N 0;
    	current_1  1.0;
    	power_1 1000+2000j;	
    	shunt_1 5.3333e-004 -2.6667e-004i;	
    	nominal_voltage 120;
    	};

### Triplex Node Parameters

#### Properties

**triplex_node** objects are derived from **[powerflow_object](02-powerflow_object.md)** objects, so any parameters of the **[powerflow_object](02-powerflow_object.md)** object are available as well.

Input indicates properties you can set in models. Output marks properties produced or modified during simulation runtime.

| Property Name | Type | Unit | Input | Output | Description |
| --- | --- | --- | --- | --- | --- |
| bustype | enumeration | N/A | ✓ |  | The type of bus the node represents. The different bus distinctions are only valid for the Gauss-Seidel and Newton-Raphson solver methods. The Forward-Back Sweep method (Kersting&#x27;s method) does not presently incorporate anything other than the `PQ` bus. Valid choices are &lt;br/&gt; - `PQ` for a constant power bus (default) &lt;br/&gt; - `PV` for a voltage-controlled (magnitude) bus &lt;br/&gt; - `SWING` for the infinite bus of a system. |
| busflags | set | N/A | ✓ |  | A flag to indicate if the current bus has a source or not. Mainly used for `PV` implementations. The only valid entries are `HASSOURCE` to indicate it is a supported bus, or an empty value indicating it is not. |
| reference_bus | object | N/A | ✓ |  | A reference node elsewhere in the system that the **triplex_node** will use to obtain frequency information if necessary (unimplemented in GridLAB-D™ at this point). |
| maximum_voltage_error | double | V | ✓ |  | The maximum voltage error for convergence checks in the different powerflow solvers. If left blank, it is derived from the `nominal_voltage` parameter. |
| voltage_1 | complex | V | ✓ |  | The voltage on phase 1 of a split-phase or triplex system. This may be specified in rectangular (7200.0+0.0j) or polar (7200.0+0.0d) formats. |
| voltage_2 | complex | V | ✓ |  | The voltage on phase 2 of a split-phase or triplex system. This may be specified in rectangular (7200.0+0.0j) or polar (7200.0+0.0d) formats. |
| voltage_N | complex | V | ✓ |  | The voltage on the neutral phase of a split-phase or triplex system. This may be specified in rectangular (7200.0+0.0j) or polar (7200.0+0.0d) formats. |
| voltage_12 | complex | V | ✓ |  | The voltage between phases `1` and `2` of the split-phase or triplex system. This is a derived quantity and can be read, but it is not recommended you set this value. |
| voltage_1N | complex | V | ✓ |  | The voltage between phases `1` and `N` of the split-phase or triplex system. This is a derived quantity and can be read, but it is not recommended you set this value. |
| voltage_2N | complex | V | ✓ |  | The voltage between phases `2` and `N` of the split-phase or triplex system. This is a derived quantity and can be read, but it is not recommended you set this value. |
| house_present | bool | N/A | ✓ |  | ⚠️ boolean for detecting whether a house is attached, not an input |
| GFA_enable | bool | N/A | ✓ |  | ⚠️ Disable/Enable Grid Friendly Applicance(TM)-type functionality |
| GFA_freq_low_trip | double | Hz | ✓ |  | ⚠️ Low frequency trip point for Grid Friendly Appliance(TM)-type functionality |
| GFA_freq_high_trip | double | Hz | ✓ |  | ⚠️ High frequency trip point for Grid Friendly Appliance(TM)-type functionality |
| GFA_volt_low_trip | double | pu | ✓ |  | ⚠️ Low voltage trip point for Grid Friendly Appliance(TM)-type functionality |
| GFA_volt_high_trip | double | pu | ✓ |  | ⚠️ High voltage trip point for Grid Friendly Appliance(TM)-type functionality |
| GFA_reconnect_time | double | s | ✓ |  | ⚠️ Reconnect time for Grid Friendly Appliance(TM)-type functionality |
| GFA_freq_disconnect_time | double | s | ✓ |  | ⚠️ Frequency violation disconnect time for Grid Friendly Appliance(TM)-type functionality |
| GFA_volt_disconnect_time | double | s | ✓ |  | ⚠️ Voltage violation disconnect time for Grid Friendly Appliance(TM)-type functionality |
| GFA_status | bool | N/A | ✓ |  | ⚠️ Low frequency trip point for Grid Friendly Appliance(TM)-type functionality |
| GFA_trip_method | enumeration | N/A | ✓ |  | ⚠️ Reason for GFA trip - what caused the GFA to activate Valid values: `NONE`, `UNDER_FREQUENCY`, `OVER_FREQUENCY`, `UNDER_VOLTAGE`, `OVER_VOLTAGE`. |
| service_status | enumeration | N/A | ✓ |  | ⚠️ In and out of service flag Valid values: `IN_SERVICE`, `OUT_OF_SERVICE`. |
| service_status_double | double | N/A | ✓ |  | ⚠️ In and out of service flag - type double - will indiscriminately override service_status - useful for schedules |
| previous_uptime | double | min | ✓ |  | ⚠️ Previous time between disconnects of node in minutes |
| current_uptime | double | min | ✓ |  | ⚠️ Current time since last disconnect of node in minutes |
| topological_parent | object | N/A | ✓ |  | ⚠️ topological parent as per GLM configuration |
| behaving_as_swing | bool | N/A | ✓ |  | ⚠️ Indicator flag for if a bus is behaving as a reference voltage source - valid for a SWING or SWING_PQ |
| frequency_measure_type | enumeration | N/A | ✓ |  | ⚠️ Frequency measurement dynamics-capable implementation Valid values: `NONE`, `SIMPLE`, `PLL`. |
| sfm_Tf | double | s | ✓ |  | ⚠️ Transducer time constant for simplified frequency measurement (seconds) |
| pll_Kp | double | pu | ✓ |  | ⚠️ Proportional gain of PLL frequency measurement |
| pll_Ki | double | pu | ✓ |  | ⚠️ Integration gain of PLL frequency measurement |
| measured_angle_1 | double | rad | ✓ |  | ⚠️ bus angle measurement, phase 1N |
| measured_frequency_1 | double | Hz | ✓ |  | ⚠️ frequency measurement, phase 1N |
| measured_angle_2 | double | rad | ✓ |  | ⚠️ bus angle measurement, phase 2N |
| measured_frequency_2 | double | Hz | ✓ |  | ⚠️ frequency measurement, phase 2N |
| measured_angle_12 | double | rad | ✓ |  | ⚠️ bus angle measurement, across the phases |
| measured_frequency_12 | double | Hz | ✓ |  | ⚠️ frequency measurement, across the phases |
| measured_frequency | double | Hz | ✓ |  | ⚠️ frequency measurement - average of present phases |

#### Developer Properties

These properties are published with `PA_HIDDEN` and are intended for internal or developer use.

| Property Name | Type | Unit | Input | Output | Description |
| --- | --- | --- | --- | --- | --- |
| prerotated_current_1 | complex | A |  |  | ⚠️ deltamode-functionality - bus current injection (in = positive), but will not be rotated by powerflow for off-nominal frequency, this an accumulator only, not a output or input variable |
| prerotated_current_2 | complex | A |  |  | ⚠️ deltamode-functionality - bus current injection (in = positive), but will not be rotated by powerflow for off-nominal frequency, this an accumulator only, not a output or input variable |
| prerotated_current_12 | complex | A |  |  | ⚠️ deltamode-functionality - bus current injection (in = positive), but will not be rotated by powerflow for off-nominal frequency, this an accumulator only, not a output or input variable |
| deltamode_generator_current_12 | complex | A |  |  | ⚠️ deltamode-functionality - bus current injection (in = positive), direct generator injection (so may be overwritten internally), this an accumulator only, not a output or input variable |
| deltamode_PGenTotal | complex | N/A |  |  | ⚠️ deltamode-functionality - power value for a diesel generator -- accumulator only, not an output or input |
| deltamode_full_Y_matrix | complex_array | N/A |  |  | ⚠️ deltamode-functionality full_Y matrix exposes so generator objects can interact for Norton equivalents |
| deltamode_full_Y_all_matrix | complex_array | N/A |  |  | ⚠️ deltamode-functionality full_Y_all matrix exposes so generator objects can interact for Norton equivalents |
| NR_powerflow_parent | object | N/A |  |  | ⚠️ NR powerflow - actual powerflow parent - used by generators accessing child objects |
| residential_nominal_current_1 | complex | A |  |  | ⚠️ posted current on phase 1 from a residential object, if attached |
| residential_nominal_current_2 | complex | A |  |  | ⚠️ posted current on phase 2 from a residential object, if attached |
| residential_nominal_current_12 | complex | A |  |  | ⚠️ posted current on phase 1 to 2 from a residential object, if attached |
| residential_nominal_current_1_real | double | A |  |  | ⚠️ posted current on phase 1, real, from a residential object, if attached |
| residential_nominal_current_1_imag | double | A |  |  | ⚠️ posted current on phase 1, imag, from a residential object, if attached |
| residential_nominal_current_2_real | double | A |  |  | ⚠️ posted current on phase 2, real, from a residential object, if attached |
| residential_nominal_current_2_imag | double | A |  |  | ⚠️ posted current on phase 2, imag, from a residential object, if attached |
| residential_nominal_current_12_real | double | A |  |  | ⚠️ posted current on phase 1 to 2, real, from a residential object, if attached |
| residential_nominal_current_12_imag | double | A |  |  | ⚠️ posted current on phase 1 to 2, imag, from a residential object, if attached |
| current_1 | complex | A |  |  | Constant current load on phase `1` of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here. |
| current_2 | complex | A |  |  | Constant current load on phase `2` of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here. |
| current_N | complex | A |  |  | Constant current load on the neutral phase of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here. |
| current_12 | complex | A |  |  | Constant current load on across phases `1` and `2` of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here. |
| power_1 | complex | VA |  |  | Constant power load on phase `1` of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here. |
| power_2 | complex | VA |  |  | Constant power load on phase `2` of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here. |
| power_12 | complex | VA |  |  | Constant power load across phases `1` and `2` of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here. |
| shunt_1 | complex | S |  |  | Constant admittance load on phase `1` of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here. |
| shunt_2 | complex | S |  |  | Constant admittance load on phase `2` of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here. |
| shunt_12 | complex | S |  |  | Constant admittance load across phases `1` and `2` of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here. |
| Norton_dynamic | bool | N/A |  |  | ⚠️ Flag to indicate a Norton-equivalent connection -- used for generators and deltamode |
| Norton_dynamic_child | bool | N/A |  |  | ⚠️ Flag to indicate a Norton-equivalent connection is made by a childed node object -- used for generators and deltamode |
| generator_dynamic | bool | N/A |  |  | ⚠️ Flag to indicate a voltage-sourcing or swing-type generator is present -- used for generators and deltamode |
| reset_disabled_island_state | bool | N/A |  |  | ⚠️ Deltamode/multi-island flag -- used to reset disabled status (and reform an island) |

### Triplex Node State of Development

Triplex Node is considered a highly developed and validated model in terms of powerflow solutions, however, models may be developed to include more advanced features in the future. 
