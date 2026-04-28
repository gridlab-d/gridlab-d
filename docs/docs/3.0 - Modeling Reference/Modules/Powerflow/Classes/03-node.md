## Node

The node object is equivalent to a bus of the distribution system. It provides a connection point for **link**-based objects and a point of known voltages on the system. Three-phase voltage is typically available in either wye-connected or delta-connected form. Wye-connected voltages are contained in `voltage_A`, `voltage_B`, and `voltage_C`. Delta-connected voltages are available in `voltage_AB`, `voltage_BC`, and `voltage_CA`.

### Node Parameters

**node** objects are derived from **[powerflow_object](02-powerflow_object.md)** objects, so any parameters of the **[powerflow_object](02-powerflow_object.md)** object are available as well.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

#### General Properties

These properties control the fundamental bus configuration and solver behavior of the node.

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| bustype | enumeration | N/A | I | Defines the bus type for powerflow analysis. The bus type distinction is primarily used by the Newton-Raphson solver. In Forward-Back Sweep mode, the first node initialized is automatically promoted to `SWING`. Valid values: <br/> - `PQ` — Constant power bus (default). <br/> - `PV` — Voltage-controlled (magnitude) bus. <br/> - `SWING` — Infinite bus / voltage reference for the system. <br/> - `SWING_PQ` — Swing bus that can revert to PQ behavior when used with a `fault_check` object in `grid_association` mode. |
| busflags | set | N/A | I | Internal flag set for topology analysis. `HASSOURCE` is set by default on all nodes and is unused at this time (the propagation logic that would clear it requires the SUPPORT_OUTAGES compile flag, which is not enabled in standard builds). ISSOURCE marks a bus as an independent source entry point for fault_check grid association mode. Valid values: `HASSOURCE`, `ISSOURCE`. |
| reference_bus | object | N/A | I | Reference node from which this node inherits its frequency value. Used in the Forward-Back Sweep solver during presync. |
| maximum_voltage_error | double | V | I | Maximum voltage error threshold for convergence checks. If left at zero, it is automatically derived from `nominal_voltage` multiplied by the global `default_maximum_voltage_error`. |
| mean_repair_time | double | s | I | Time after a fault clears before the object is considered back in service. Primarily used for reliability module interactions. |

#### Voltage Properties

These properties hold the bus voltage phasors. Voltages may be specified in rectangular (`7200.0+0.0j`) or polar (`7200.0+0.0d`) format. The `_A`, `_B`, `_C` variants are phase-to-ground (wye) voltages and serve as both user-settable initial conditions and simulation outputs updated each powerflow iteration. The `_AB`, `_BC`, `_CA` variants are line-to-line (delta) voltages derived from the corresponding phase-to-ground differences; setting them directly is not recommended.

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| voltage_A | complex | V | IO | Bus voltage on phase A to ground. |
| voltage_B | complex | V | IO | Bus voltage on phase B to ground. |
| voltage_C | complex | V | IO | Bus voltage on phase C to ground. |
| voltage_AB | complex | V | IO | Line-to-line voltage across phases AB. Computed as `voltage_A - voltage_B`. |
| voltage_BC | complex | V | IO | Line-to-line voltage across phases BC. Computed as `voltage_B - voltage_C`. |
| voltage_CA | complex | V | IO | Line-to-line voltage across phases CA. Computed as `voltage_C - voltage_A`. |

#### Service Status Properties

These properties track whether the node is in service and how long it has been connected or disconnected. The `service_status_double` property provides a schedule-friendly numeric override for the enumeration-based `service_status`.

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| service_status | enumeration | N/A | IO | Indicates whether the node is in service or disconnected. Valid values: `IN_SERVICE`, `OUT_OF_SERVICE`. |
| service_status_double | double | N/A | I | Double-valued override for `service_status`, intended for use with schedules. Set to `1.0` for `IN_SERVICE`, `0.0` for `OUT_OF_SERVICE`. The default value of `-1.0` disables the override. Other values cause an error. |
| previous_uptime | double | min | IO | Previous uptime duration between the last two disconnects of this node. |
| current_uptime | double | min | IO | Elapsed time since the most recent disconnect of this node. Set to `-1.0` when the node is out of service. |

#### Frequency Measurement Properties

These properties configure and report frequency and angle measurements during deltamode simulation. The measurement method is selected by `frequency_measure_type`; if set to `NONE` (the default unless overridden by a module-level global setting), no measurements are performed. The `SIMPLE` method uses a first-order transducer model controlled by `sfm_Tf`. The `PLL` method uses a phase-locked loop controlled by `pll_Kp` and `pll_Ki`.

The four configuration properties are input only. The seven `measured_*` properties are both input and output — initial values may be set by the user, but the simulation overwrites them each deltamode timestep.

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| frequency_measure_type | enumeration | N/A | I | Selects the frequency measurement method. Valid values: `NONE`, `SIMPLE`, `PLL`. |
| sfm_Tf | double | s | I | Transducer time constant for the `SIMPLE` method. |
| pll_Kp | double | pu | I | Proportional gain for the `PLL` method. |
| pll_Ki | double | pu | I | Integration gain for the `PLL` method. |
| measured_angle_A | double | rad | IO | Measured bus voltage angle on phase A. |
| measured_angle_B | double | rad | IO | Measured bus voltage angle on phase B. |
| measured_angle_C | double | rad | IO | Measured bus voltage angle on phase C. |
| measured_frequency_A | double | Hz | IO | Measured frequency on phase A. |
| measured_frequency_B | double | Hz | IO | Measured frequency on phase B. |
| measured_frequency_C | double | Hz | IO | Measured frequency on phase C. |
| measured_frequency | double | Hz | IO | Measured frequency averaged across all energized phases. |

#### Grid Friendly Appliance (GFA) Properties

These properties configure Grid Friendly Appliance-type voltage and frequency trip/reconnect logic. When `GFA_enable` is `true`, the node monitors its voltage and frequency against configurable trip thresholds. If a violation persists longer than the corresponding disconnect time, the node is tripped out of service. After the violation clears, the node remains disconnected for `GFA_reconnect_time` before being restored.

The first eight properties are input-only configuration parameters. `GFA_status` and `GFA_trip_method` are both input and output — they can be set initially but are updated by the simulation at runtime.

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| GFA_enable | bool | N/A | I | Enables or disables GFA-type functionality on this node. |
| GFA_freq_low_trip | double | Hz | I | Low frequency trip point. |
| GFA_freq_high_trip | double | Hz | I | High frequency trip point. |
| GFA_volt_low_trip | double | pu | I | Low voltage trip point. |
| GFA_volt_high_trip | double | pu | I | High voltage trip point. |
| GFA_freq_disconnect_time | double | s | I | Duration a frequency violation must persist before disconnection. |
| GFA_volt_disconnect_time | double | s | I | Duration a voltage violation must persist before disconnection. |
| GFA_reconnect_time | double | s | I | Delay after a trip event before the node is restored to service. |
| GFA_status | bool | N/A | IO | Whether GFA considers the node in service (`true`) or tripped (`false`). |
| GFA_trip_method | enumeration | N/A | IO | Reason for the most recent GFA trip. Valid values: `NONE`, `UNDER_FREQUENCY`, `OVER_FREQUENCY`, `UNDER_VOLTAGE`, `OVER_VOLTAGE`. |

#### Topology and Swing Status Properties

These properties expose the node's topological parent relationship and its runtime swing-bus behavior. Neither is meaningfully user-configurable — `topological_parent` is determined during initialization and `behaving_as_swing` is recomputed every postsync. Both are effectively output-only or informational.

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| topological_parent | object | N/A | O | Topological parent of this node as determined during initialization. Reflects the object's `parent` field. |
| behaving_as_swing | bool | N/A | O | Whether this bus is currently acting as a reference voltage source. Only meaningful for `SWING` or `SWING_PQ` bus types. |

<!-- <details>
<summary>Internal Properties</summary> -->

??? note "Internal Properties"

	#### Developer Properties
	These properties are published with `PA_HIDDEN` and are intended for internal or developer use.

	| Property Name | Type | Unit | I/O | Description |
	| --- | --- | --- | --- | --- |
	| current_A | complex | A | O | The current load on phase A (wye) or phase AB (delta) of the node. Typically handled through the **load** object; modification is not recommended. |
	| current_B | complex | A | O | The current load on phase B (wye) or phase BC (delta) of the node. Typically handled through the **load** object; modification is not recommended. |
	| current_C | complex | A | O | The current load on phase C (wye) or phase CA (delta) of the node. Typically handled through the **load** object; modification is not recommended. |
	| power_A | complex | VA | O | The power load on phase A (wye) or phase AB (delta) of the node. Typically handled through the **load** object; modification is not recommended. |
	| power_B | complex | VA | O | The power load on phase B (wye) or phase BC (delta) of the node. Typically handled through the **load** object; modification is not recommended. |
	| power_C | complex | VA | O | The power load on phase C (wye) or phase CA (delta) of the node. Typically handled through the **load** object; modification is not recommended. |
	| shunt_A | complex | S | O | The shunt admittance load on phase A (wye) or phase AB (delta) of the node. Typically handled through the **load** object; modification is not recommended. |
	| shunt_B | complex | S | O | The shunt admittance load on phase B (wye) or phase BC (delta) of the node. Typically handled through the **load** object; modification is not recommended. |
	| shunt_C | complex | S | O | The shunt admittance load on phase C (wye) or phase CA (delta) of the node. Typically handled through the **load** object; modification is not recommended. |
	| prerotated_current_A | complex | A | O | Deltamode bus current injection on phase A (in = positive). Not rotated by powerflow for off-nominal frequency. Accumulator only. |
	| prerotated_current_B | complex | A | O | Deltamode bus current injection on phase B (in = positive). Not rotated by powerflow for off-nominal frequency. Accumulator only. |
	| prerotated_current_C | complex | A | O | Deltamode bus current injection on phase C (in = positive). Not rotated by powerflow for off-nominal frequency. Accumulator only. |
	| deltamode_generator_current_A | complex | A | O | Deltamode direct generator current injection on phase A (in = positive). May be overwritten internally. Accumulator only. |
	| deltamode_generator_current_B | complex | A | O | Deltamode direct generator current injection on phase B (in = positive). May be overwritten internally. Accumulator only. |
	| deltamode_generator_current_C | complex | A | O | Deltamode direct generator current injection on phase C (in = positive). May be overwritten internally. Accumulator only. |
	| deltamode_PGenTotal | complex | N/A | — | Deltamode power value for a diesel generator. Accumulator only. |
	| deltamode_full_Y_matrix | complex_array | N/A | — | Deltamode full admittance matrix exposed for generator Norton equivalent interactions. |
	| deltamode_full_Y_all_matrix | complex_array | N/A | O | Deltamode full admittance matrix (including all contributions) exposed for generator Norton equivalent interactions. |
	| NR_powerflow_parent | object | N/A | — | Actual powerflow parent in Newton-Raphson. Used by generators accessing child objects. |
	| current_inj_A | complex | A | O | Bus current injection on phase A (in = positive). Not rotated for off-nominal frequency. Accumulator only. |
	| current_inj_B | complex | A | O | Bus current injection on phase B (in = positive). Not rotated for off-nominal frequency. Accumulator only. |
	| current_inj_C | complex | A | O | Bus current injection on phase C (in = positive). Not rotated for off-nominal frequency. Accumulator only. |
	| current_AB | complex | A | O | Bus current delta-connected injection on phases AB (in = positive). Accumulator only. |
	| current_BC | complex | A | O | Bus current delta-connected injection on phases BC (in = positive). Accumulator only. |
	| current_CA | complex | A | O | Bus current delta-connected injection on phases CA (in = positive). Accumulator only. |
	| current_AN | complex | A | O | Bus current wye-connected injection on phase A (in = positive). Accumulator only. |
	| current_BN | complex | A | O | Bus current wye-connected injection on phase B (in = positive). Accumulator only. |
	| current_CN | complex | A | O | Bus current wye-connected injection on phase C (in = positive). Accumulator only. |
	| power_AB | complex | VA | O | Bus power delta-connected injection on phases AB (in = positive). Accumulator only. |
	| power_BC | complex | VA | O | Bus power delta-connected injection on phases BC (in = positive). Accumulator only. |
	| power_CA | complex | VA | O | Bus power delta-connected injection on phases CA (in = positive). Accumulator only. |
	| power_AN | complex | VA | O | Bus power wye-connected injection on phase A (in = positive). Accumulator only. |
	| power_BN | complex | VA | O | Bus power wye-connected injection on phase B (in = positive). Accumulator only. |
	| power_CN | complex | VA | O | Bus power wye-connected injection on phase C (in = positive). Accumulator only. |
	| shunt_AB | complex | S | O | Bus shunt delta-connected admittance on phases AB. Accumulator only. |
	| shunt_BC | complex | S | O | Bus shunt delta-connected admittance on phases BC. Accumulator only. |
	| shunt_CA | complex | S | O | Bus shunt delta-connected admittance on phases CA. Accumulator only. |
	| shunt_AN | complex | S | O | Bus shunt wye-connected admittance on phase A. Accumulator only. |
	| shunt_BN | complex | S | O | Bus shunt wye-connected admittance on phase B. Accumulator only. |
	| shunt_CN | complex | S | O | Bus shunt wye-connected admittance on phase C. Accumulator only. |
	| residential_nominal_current_A | complex | A | O | Posted current on phase A from a residential object, if attached. |
	| residential_nominal_current_B | complex | A | O | Posted current on phase B from a residential object, if attached. |
	| residential_nominal_current_C | complex | A | O | Posted current on phase C from a residential object, if attached. |
	| residential_nominal_current_A_real | double | A | O | Posted current on phase A from a residential object, real component. |
	| residential_nominal_current_A_imag | double | A | O | Posted current on phase A from a residential object, imaginary component. |
	| residential_nominal_current_B_real | double | A | O | Posted current on phase B from a residential object, real component. |
	| residential_nominal_current_B_imag | double | A | O | Posted current on phase B from a residential object, imaginary component. |
	| residential_nominal_current_C_real | double | A | O | Posted current on phase C from a residential object, real component. |
	| residential_nominal_current_C_imag | double | A | O | Posted current on phase C from a residential object, imaginary component. |
	| house_present | bool | N/A | O | Flag indicating whether a house object is attached to this node. |
	| Norton_dynamic | bool | N/A | — | Flag indicating a Norton-equivalent connection is present. Used for generators and deltamode. |
	| Norton_dynamic_child | bool | N/A | — | Flag indicating a Norton-equivalent connection is made by a childed node object. Used for generators and deltamode. |
	| generator_dynamic | bool | N/A | — | Flag indicating a voltage-sourcing or swing-type generator is present. Used for generators and deltamode. |
	| reset_disabled_island_state | bool | N/A | O | Deltamode/multi-island flag used to reset disabled status and reform an island. |

<!-- </details> -->

### Default Node

A minimalist node could be created with

    object node {
    	name NodeOne;
    	phases ABC;
    	nominal_voltage 7200.0;
    }

which is the same as specifying

    object node {
    	name NodeOne;
    	phases ABC;
    	nominal_voltage 7200.0;
    	voltage_A 7200.0+0d;
    	voltage_B 7200.0-120.0d;
    	voltage_C 7200.0+120.0d;
    	bustype PQ;
    }

### Node State of Development

Node is considered a highly developed and validated model.
