## Link

The link object is a connection between nodes in a distribution system. The **link** object is not directly useful, but is the basis for objects associated with overhead lines, underground lines, triplex lines, transformers, regulators, switches, and fuses.

### Example Link

A **link** only requires three parameters to be specified by default. Most of the actual functionality comes through derived objects such as lines, transformers, and switches.

    object link {
    	name Node1toNode2;
    	phases ABC;
    	from Node1;
    	to Node2;
    }

### Link Object Parameters

**link** objects are derived from **[powerflow_object](powerflow_object.md)** objects, so any parameters of the **[powerflow_object](powerflow_object.md)** object are available as well.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

#### General Configuration Properties

These properties define the basic topology and operating state of the link.

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| status | enumeration | N/A | IO | Operating status of the link. Primarily used for switches and fuses, but may be used to remove any link from service. In the `FBS` solver, this directly controls forward/back sweep participation. Valid values: `OPEN`, `CLOSED`. |
| from | object | N/A | I | Source-side node of the link. Must reference a node-based powerflow object. |
| to | object | N/A | I | Load-side node of the link. Must reference a node-based powerflow object. |
| mean_repair_time | double | s | I | Time after a fault has cleared before the object will be restored to service. Used by the **reliability** module. |

#### Power Flow Measurements

These properties are computed by the simulation during postsync and are output only. They represent power flowing through the link with respect to the `from` and `to` node designations.

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| power_in | complex | VA | IO | Total three-phase power flowing into the link from the `from` node. |
| power_out | complex | VA | IO | Total three-phase power flowing out of the link toward the `to` node. |
| power_out_real | double | W | IO | Real component of `power_out`. |
| power_losses | complex | VA | IO | Total three-phase power loss across the link. |
| power_in_A | complex | VA | IO | Power flowing into the link on phase A. |
| power_in_B | complex | VA | IO | Power flowing into the link on phase B. |
| power_in_C | complex | VA | IO | Power flowing into the link on phase C. |
| power_out_A | complex | VA | IO | Power flowing out of the link on phase A. |
| power_out_B | complex | VA | IO | Power flowing out of the link on phase B. |
| power_out_C | complex | VA | IO | Power flowing out of the link on phase C. |
| power_losses_A | complex | VA | IO | Power loss on phase A. |
| power_losses_B | complex | VA | IO | Power loss on phase B. |
| power_losses_C | complex | VA | IO | Power loss on phase C. |

#### Current Measurements

These properties are computed by the simulation during postsync and are output only. They represent the current flowing through the link. Note: these have not been fully validated for every derived link type.

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| current_in_A | complex | A | O | Current flowing into the link on phase A, with respect to the `from` node. |
| current_in_B | complex | A | O | Current flowing into the link on phase B, with respect to the `from` node. |
| current_in_C | complex | A | O | Current flowing into the link on phase C, with respect to the `from` node. |
| current_out_A | complex | A | O | Current flowing out of the link on phase A, with respect to the `to` node. |
| current_out_B | complex | A | O | Current flowing out of the link on phase B, with respect to the `to` node. |
| current_out_C | complex | A | O | Current flowing out of the link on phase C, with respect to the `to` node. |

#### Fault Current and Voltage Properties

These properties are populated by the fault analysis subsystem (via the **reliability** module or `eventgen` object) and are output only. 

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| fault_current_in_A | complex | A | IO | Fault current flowing into the link on phase A. |
| fault_current_in_B | complex | A | IO | Fault current flowing into the link on phase B. |
| fault_current_in_C | complex | A | IO | Fault current flowing into the link on phase C. |
| fault_current_out_A | complex | A | IO | Fault current flowing out of the link on phase A. |
| fault_current_out_B | complex | A | IO | Fault current flowing out of the link on phase B. |
| fault_current_out_C | complex | A | IO | Fault current flowing out of the link on phase C. |
| fault_voltage_A | complex | V | O | Fault voltage on phase A. |
| fault_voltage_B | complex | V | O | Fault voltage on phase B. |
| fault_voltage_C | complex | V | O | Fault voltage on phase C. |

#### Dispatch Properties

These properties support scheduled power dispatch through the link. When `set_pdispatch` is set to `true`, the simulation computes `pdispatch` as the average of `power_in` and `power_out` real components and resets `pdispatch_offset` to zero. The `set_pdispatch` flag is automatically cleared after triggering.

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| pdispatch | double | W | IO | Scheduled real power flow from `from` to `to`. Overwritten when `set_pdispatch` is triggered. |
| pdispatch_offset | double | W | IO | Offset applied to the scheduled power flow. Reset to zero when `set_pdispatch` is triggered. |
| set_pdispatch | bool | N/A | IO | When set to `true`, triggers computation of `pdispatch` as the average of real power in and out. Automatically reset to `false` after triggering. |

#### Flow Direction and Overload Status

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| flow_direction | set | N/A | IO | Per-phase flag indicating the direction of power flow relative to the `from` and `to` designations. Computed during postsync from power flow results. Multiple flags are combined as a set. <br/> 0x000 - `UNKNOWN` - Flow direction is indeterminate. <br/> 0x001 - `AF` - Phase A power flows from `from` to `to`. <br/> 0x002 - `AR` - Phase A power flows from `to` to `from` (reverse). <br/> 0x003 - `AN` - No power flow on phase A. <br/> 0x010 - `BF` - Phase B power flows from `from` to `to`. <br/> 0x020 - `BR` - Phase B power flows from `to` to `from` (reverse). <br/> 0x030 - `BN` - No power flow on phase B. <br/> 0x100 - `CF` - Phase C power flows from `from` to `to`. <br/> 0x200 - `CR` - Phase C power flows from `to` to `from` (reverse). <br/> 0x300 - `CN` - No power flow on phase C. |
| overloaded_status | bool | N/A | O | Indicates whether the link has exceeded its thermal rating. Set to `true` if any phase current exceeds its continuous rating, or if transformer power exceeds its kVA rating. |

#### Thermal Rating Properties

These properties define the current (for lines) or power (for transformers) limits of the link. They are input only. Limit checking is only active when the global `use_link_limits` flag is enabled and the link is a line or transformer (switches, fuses, regulators, and sectionalizers are excluded). Default values are 1000 A continuous and 2000 A emergency.

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| continuous_rating_A | double | A | I | Continuous current rating for phase A. |
| continuous_rating_B | double | A | I | Continuous current rating for phase B. |
| continuous_rating_C | double | A | I | Continuous current rating for phase C. |
| emergency_rating_A | double | A | I | Emergency current rating for phase A. |
| emergency_rating_B | double | A | I | Emergency current rating for phase B. |
| emergency_rating_C | double | A | I | Emergency current rating for phase C. |

#### Transient Simulations / In-Rush Properties

These properties control the numerical integration method and convergence tolerance used for in-rush current calculations during transient (deltamode) simulation. If the integration method is left as `UNDEFINED`, the object inherits the global integration method setting. These properties are only used when both deltamode and in-rush calculations are enabled.

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| inrush_convergence_value | double | V | I | Convergence tolerance for deltamode in-rush completion, measured as the change in line voltage drop magnitude between iterations. Default is 0.0001 V. |
| inrush_integration_method_capacitance | enumeration | N/A | I | Integration method for capacitive elements of the link. Valid values: `NONE`, `UNDEFINED`, `TRAPEZOIDAL`, `BACKWARD_EULER`. |
| inrush_integration_method_inductance | enumeration | N/A | I | Integration method for inductive elements of the link. Valid values: `NONE`, `UNDEFINED`, `TRAPEZOIDAL`, `BACKWARD_EULER`. |

??? note "Internal Properties"

	#### Internal Properties
	These properties are published with `PA_HIDDEN` and are intended for internal or developer use. They support triplex line neutral current calculations.

	| Property Name | Type | Unit | I/O | Description |
	| --- | --- | --- | --- | --- |
	| triplex_neutral_1_value | complex | N/A | O | Internal triplex neutral coupling coefficient for phase 1. |
	| triplex_neutral_2_value | complex | N/A | O | Internal triplex neutral coupling coefficient for phase 2. |



### Link State of Development

Link is considered a highly developed and validated model.

---

### Review Summary

**Factual Errors Fixed:**
- **Fault current/voltage descriptions**: Rewritten from bare source code comments to complete sentences.
- **Rating descriptions**: Removed parenthetical "(set individual line segments)" which was confusing; added context about default values and when limit checking is active.

**Behavioral Claims Added to Section Intros with Source Evidence:**

- "Power flow properties computed during postsync" — `BOTH_link_postsync_fxn()` calls `calculate_power()` which writes all power properties; `BOTH_link_postsync_fxn()` is called from `postsync()` (line: `BOTH_link_postsync_fxn();` in `postsync`) and `inter_deltaupdate_link()`.
- "Fault current values are zeroed at the start of each new timestep in presync" — `presync()` contains `if (prev_LTime != t0) { if (If_in[0] != 0 ...) { If_in[0] = 0; If_in[1] = 0; If_in[2] = 0; If_out[0] = 0; ...} }`.
- "When `set_pdispatch` is triggered, the simulation computes pdispatch as average of real power in and out" — `BOTH_link_postsync_fxn()` contains `pdispatch.pdispatch = (power_in.Re() + power_out.Re()) / 2.0;`.
- "Limit checking excludes switches, fuses, regulators, and sectionalizers" — `init()` contains `if (gl_object_isa(obj, "transformer", ...) || gl_object_isa(obj, "underground_line", ...) || ...)` with an `else { check_link_limits = false; }` for other types.
- "In-rush integration methods default to UNDEFINED and inherit the global setting" — `init()` contains `if (inrush_int_method_inductance == IRM_UNDEFINED) { inrush_int_method_inductance = inrush_integration_method; }`.
