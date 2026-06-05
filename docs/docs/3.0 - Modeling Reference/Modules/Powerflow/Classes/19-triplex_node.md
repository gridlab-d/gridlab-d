## Triplex Node

Triplex nodes represent special cases of the **node** object. The **triplex_node** object still serves as connection point between different links of the system and a point of measurable voltage. However, **triplex_node**s are casted to represent phases `1`, `2`, and `N` rather than `A`, `B`, and `C` like normal **node** objects. Simplified, they operate in the split-phase level of distribution rather than the three-phase level. 

Since **load** objects are directly derived from **node** objects, they are only valid for three-phase connections as well. Therefore, the **load** functionality has been built into the **triplex_load** object for split-phase level systems. 

It is important to note that triplex-based objects should include the phase `S` somewhere in their designation. 

### Example Triplex Node

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

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

#### General Properties

These properties control the fundamental bus configuration and solver behavior of the triplex node.

Table: triplex_node table 1 { #tbl:19-triplex-node-1 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| **bustype** | enumeration | N/A | I | The type of bus the node represents. The different bus distinctions are only valid for the Gauss-Seidel and Newton-Raphson solver methods. The Forward-Back Sweep method (Kersting's method) does not presently incorporate anything other than the `PQ` bus. Valid choices are <br/> - `PQ` for a constant power bus (default) <br/> - `PV` for a voltage-controlled (magnitude) bus <br/> - `SWING` for the infinite bus of a system. |
| **busflags** | set | N/A | I | A flag to indicate if the current bus has a source or not. Mainly used for `PV` implementations. The only valid entries are `HASSOURCE` to indicate it is a supported bus, or an empty value indicating it is not. |
| **reference_bus** | object | N/A | I | A reference node elsewhere in the system that the **triplex_node** will use to obtain frequency information if necessary (unimplemented in GridLAB-D™ at this point). |
| **maximum_voltage_error** | double | V | I | The maximum voltage error for convergence checks in the different powerflow solvers. If left blank, it is derived from the `nominal_voltage` parameter. |

#### Voltage Properties

These properties hold the bus voltage phasors for split-phase systems. Voltages may be specified in rectangular (`120.0+0.0j`) or polar (`120.0+0.0d`) format. The `_1`, `_2`, `_N` variants are phase-to-neutral voltages and serve as both user-settable initial conditions and simulation outputs updated each powerflow iteration. The `_12`, `_1N`, `_2N` variants are line-to-line or derived voltages; setting them directly is not recommended.

Table: triplex_node table 2 { #tbl:19-triplex-node-2 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| **voltage_1** | complex | V | IO | The voltage on phase 1 of a split-phase or triplex system. This may be specified in rectangular (7200.0+0.0j) or polar (7200.0+0.0d) formats. |
| **voltage_2** | complex | V | IO | The voltage on phase 2 of a split-phase or triplex system. This may be specified in rectangular (7200.0+0.0j) or polar (7200.0+0.0d) formats. |
| **voltage_N** | complex | V | IO | The voltage on the neutral phase of a split-phase or triplex system. This may be specified in rectangular (7200.0+0.0j) or polar (7200.0+0.0d) formats. |
| **voltage_12** | complex | V | IO | The voltage between phases `1` and `2` of the split-phase or triplex system. This is a derived quantity and can be read, but it is not recommended you set this value. |
| **voltage_1N** | complex | V | IO | The voltage between phases `1` and `N` of the split-phase or triplex system. This is a derived quantity and can be read, but it is not recommended you set this value. |
| **voltage_2N** | complex | V | IO | The voltage between phases `2` and `N` of the split-phase or triplex system. This is a derived quantity and can be read, but it is not recommended you set this value. |
| **house_present** | bool | N/A | O | Flag indicating whether a house object is attached to this node. |

#### Service Status Properties

These properties track whether the node is in service and how long it has been connected or disconnected. The `service_status_double` property provides a schedule-friendly numeric override for the enumeration-based `service_status`.

Table: triplex_node table 3 { #tbl:19-triplex-node-3 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| **service_status** | enumeration | N/A | IO | Indicates whether the node is in service or disconnected. Valid values: `IN_SERVICE`, `OUT_OF_SERVICE`. |
| **service_status_double** | double | N/A | I | Double-valued override for `service_status`, intended for use with schedules. Set to `1.0` for `IN_SERVICE`, `0.0` for `OUT_OF_SERVICE`. The default value of `-1.0` disables the override. Other values cause an error. |
| **previous_uptime** | double | min | IO | Previous uptime duration between the last two disconnects of this node. |
| **current_uptime** | double | min | IO | Elapsed time since the most recent disconnect of this node. Set to `-1.0` when the node is out of service. |

#### Frequency Measurement Properties

These properties configure and report frequency and angle measurements during transient simulation. The measurement method is selected by `frequency_measure_type`; if set to `NONE` (the default unless overridden by a module-level global setting), no measurements are performed. The `SIMPLE` method uses a first-order transducer model controlled by `sfm_Tf`. The `PLL` method uses a phase-locked loop controlled by `pll_Kp` and `pll_Ki`.

The four configuration properties are input only. The seven `measured_*` properties are output.

Table: triplex_node table 4 { #tbl:19-triplex-node-4 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| **frequency_measure_type** | enumeration | N/A | I | Selects the frequency measurement method. Valid values: `NONE`, `SIMPLE`, `PLL`. |
| **sfm_Tf** | double | s | I | Transducer time constant for the `SIMPLE` method. |
| **pll_Kp** | double | pu | I | Proportional gain for the `PLL` method. |
| **pll_Ki** | double | pu | I | Integration gain for the `PLL` method. |
| **measured_angle_1** | double | rad | O | Measured bus voltage angle on phase 1. |
| **measured_frequency_1** | double | Hz | O | Measured frequency on phase 1. |
| **measured_angle_2** | double | rad | O | Measured bus voltage angle on phase 2. |
| **measured_frequency_2** | double | Hz | O | Measured frequency on phase 2. |
| **measured_angle_12** | double | rad | O | Measured bus voltage angle across phases 1 and 2. |
| **measured_frequency_12** | double | Hz | O | Measured frequency across phases 1 and 2. |
| **measured_frequency** | double | Hz | O | Measured frequency averaged across all energized phases. |

#### Grid Friendly Appliance (GFA) Properties

These properties configure Grid Friendly Appliance-type voltage and frequency trip/reconnect logic. When `GFA_enable` is `true`, the node monitors its voltage and frequency against configurable trip thresholds. If a violation persists longer than the corresponding disconnect time, the node is tripped out of service. After the violation clears, the node remains disconnected for `GFA_reconnect_time` before being restored.

The first eight properties are input-only configuration parameters. `GFA_status` and `GFA_trip_method` are both input and output — they can be set initially but are updated by the simulation at runtime.

Table: triplex_node table 5 { #tbl:19-triplex-node-5 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| **GFA_enable** | bool | N/A | I | Enables or disables GFA-type functionality on this node. |
| **GFA_freq_low_trip** | double | Hz | I | Low frequency trip point. |
| **GFA_freq_high_trip** | double | Hz | I | High frequency trip point. |
| **GFA_volt_low_trip** | double | pu | I | Low voltage trip point. |
| **GFA_volt_high_trip** | double | pu | I | High voltage trip point. |
| **GFA_freq_disconnect_time** | double | s | I | Duration a frequency violation must persist before disconnection. |
| **GFA_volt_disconnect_time** | double | s | I | Duration a voltage violation must persist before disconnection. |
| **GFA_reconnect_time** | double | s | I | Delay after a trip event before the node is restored to service. |
| **GFA_status** | bool | N/A | IO | Whether GFA considers the node in service (`true`) or tripped (`false`). |
| **GFA_trip_method** | enumeration | N/A | IO | Reason for the most recent GFA trip. Valid values: `NONE`, `UNDER_FREQUENCY`, `OVER_FREQUENCY`, `UNDER_VOLTAGE`, `OVER_VOLTAGE`. |

#### Topology and Swing Status Properties

These properties expose the node's topological parent relationship and its runtime swing-bus behavior. Neither is meaningfully user-configurable — `topological_parent` is determined during initialization and `behaving_as_swing` is recomputed every postsync. Both are effectively output-only or informational.

Table: triplex_node table 6 { #tbl:19-triplex-node-6 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| **topological_parent** | object | N/A | O | Topological parent of this node as determined during initialization. Reflects the object's `parent` field. |
| **behaving_as_swing** | bool | N/A | O | Whether this bus is currently acting as a reference voltage source. Only meaningful for `SWING` or `SWING_PQ` bus types. |

??? note "Internal Properties"

	#### Internal Properties
	These properties are published with `PA_HIDDEN` and are intended for internal or developer use.

	Table: triplex_node table 7 { #tbl:19-triplex-node-7 }

	| Property Name | Type | Unit | I/O | Description |
	| --- | --- | --- | --- | --- |
	| prerotated_current_1 | complex | A | — | deltamode-functionality - bus current injection (in = positive), but will not be rotated by powerflow for off-nominal frequency, this an accumulator only, not a output or input variable |
	| prerotated_current_2 | complex | A | — | deltamode-functionality - bus current injection (in = positive), but will not be rotated by powerflow for off-nominal frequency, this an accumulator only, not a output or input variable |
	| prerotated_current_12 | complex | A | — | deltamode-functionality - bus current injection (in = positive), but will not be rotated by powerflow for off-nominal frequency, this an accumulator only, not a output or input variable |
	| deltamode_generator_current_12 | complex | A | — | deltamode-functionality - bus current injection (in = positive), direct generator injection (so may be overwritten internally), this an accumulator only, not a output or input variable |
	| deltamode_PGenTotal | complex | N/A | — | deltamode-functionality - power value for a diesel generator -- accumulator only, not an output or input |
	| deltamode_full_Y_matrix | complex_array | N/A | — | deltamode-functionality full_Y matrix exposes so generator objects can interact for Norton equivalents |
	| deltamode_full_Y_all_matrix | complex_array | N/A | — | deltamode-functionality full_Y_all matrix exposes so generator objects can interact for Norton equivalents |
	| NR_powerflow_parent | object | N/A | — | NR powerflow - actual powerflow parent - used by generators accessing child objects |
	| residential_nominal_current_1 | complex | A | — | posted current on phase 1 from a residential object, if attached |
	| residential_nominal_current_2 | complex | A | — | posted current on phase 2 from a residential object, if attached |
	| residential_nominal_current_12 | complex | A | — | posted current on phase 1 to 2 from a residential object, if attached |
	| residential_nominal_current_1_real | double | A | — | posted current on phase 1, real, from a residential object, if attached |
	| residential_nominal_current_1_imag | double | A | — | posted current on phase 1, imag, from a residential object, if attached |
	| residential_nominal_current_2_real | double | A | — | posted current on phase 2, real, from a residential object, if attached |
	| residential_nominal_current_2_imag | double | A | — | posted current on phase 2, imag, from a residential object, if attached |
	| residential_nominal_current_12_real | double | A | — | posted current on phase 1 to 2, real, from a residential object, if attached |
	| residential_nominal_current_12_imag | double | A | — | posted current on phase 1 to 2, imag, from a residential object, if attached |
	| current_1 | complex | A | — | Constant current load on phase `1` of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here. |
	| current_2 | complex | A | — | Constant current load on phase `2` of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here. |
	| current_N | complex | A | — | Constant current load on the neutral phase of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here. |
	| current_12 | complex | A | — | Constant current load on across phases `1` and `2` of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here. |
	| power_1 | complex | VA | — | Constant power load on phase `1` of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here. |
	| power_2 | complex | VA | — | Constant power load on phase `2` of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here. |
	| power_12 | complex | VA | — | Constant power load across phases `1` and `2` of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here. |
	| shunt_1 | complex | S | — | Constant admittance load on phase `1` of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here. |
	| shunt_2 | complex | S | — | Constant admittance load on phase `2` of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here. |
	| shunt_12 | complex | S | — | Constant admittance load across phases `1` and `2` of the split-phase or triplex system. This value is typically handled through the **triplex_load** object, so modification is not recommended here. |
	| Norton_dynamic | bool | N/A | — | Flag to indicate a Norton-equivalent connection -- used for generators and deltamode |
	| Norton_dynamic_child | bool | N/A | — | Flag to indicate a Norton-equivalent connection is made by a childed node object -- used for generators and deltamode |
	| generator_dynamic | bool | N/A | — | Flag to indicate a voltage-sourcing or swing-type generator is present -- used for generators and deltamode |
	| reset_disabled_island_state | bool | N/A | — | Deltamode/multi-island flag -- used to reset disabled status (and reform an island) |

### Triplex Node State of Development

Triplex Node is considered a highly developed and validated model in terms of powerflow solutions, however, models may be developed to include more advanced features in the future. 


