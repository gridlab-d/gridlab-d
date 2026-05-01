---

# GridLAB-D Inverter Model — Parameters and Functionality

---

## 1. Core Identification and Status Parameters

These parameters govern what the inverter is and whether it is active in the simulation.

**`generator_status`** (`OFFLINE` / `ONLINE`)
Controls whether the inverter participates in the power flow solution at all. When set to `OFFLINE`, the object exists in the model but contributes no injection. Defaults to `ONLINE` at runtime.

**`phases`** (`A`, `B`, `C`, `N`, `S`)
Declares which AC phases the inverter is connected to. This drives the phase-specific variable mapping performed during `init()` — a single-phase rooftop PV on phase B will only inject into that phase's current and power accumulators. The `S` flag denotes split-phase (triplex) connection.

**`islanded_state`** (boolean)
Signals to all control modes that the inverter is operating in an islanded network. When `true`, several control modes alter their behavior — for example, load-following modes account for the `soc_reserve` floor to ensure the battery retains capacity for sustained island operation.

---

## 2. Model Selection: Legacy vs. Four-Quadrant

The inverter's behavior is organized around two distinct modeling tiers, selected through a pair of properties.

**`inverter_type`**
The top-level model selector. The valid options are:

| Value | Description |
|---|---|
| `TWO_PULSE` | Legacy topology-based model |
| `SIX_PULSE` | Legacy topology-based model |
| `TWELVE_PULSE` | Legacy topology-based model |
| `PWM` | Legacy pulse-width modulation model |
| `FOUR_QUADRANT` | Current recommended model; enables full control mode selection |

The legacy types (`TWO_PULSE` through `PWM`) parameterize efficiency implicitly from the converter topology and are retained for backward compatibility. For any new work involving smart inverter functions, battery storage, or deltamode, `FOUR_QUADRANT` must be selected.

**`generator_mode`** (Legacy model only)
When using a legacy inverter type, this selects the operating mode: `CONSTANT_V`, `CONSTANT_PQ`, `CONSTANT_PF`, or `SUPPLY_DRIVEN`. In the four-quadrant model, this should always be set to `SUPPLY_DRIVEN` and the `four_quadrant_control_mode` property takes over.

---

## 3. Four-Quadrant Control Modes

**`four_quadrant_control_mode`** is the central dispatch property of the modern inverter model. It selects from the following modes:

**`NONE`**
The inverter object is present but performs no active control. Useful for testing or when the inverter is managed entirely by an external controller.

**`CONSTANT_PQ`**
The inverter outputs fixed real and reactive power as specified by `P_Out` (W) and `Q_Out` (VAr). This is the simplest grid-connected dispatch mode. The output is not voltage-sensitive and does not adjust unless `P_Out` or `Q_Out` are externally modified (e.g., via a player or controller object).

**`CONSTANT_PF`**
The inverter operates at a fixed power factor specified by `power_factor`. Real power is determined by the attached resource (solar irradiance, battery dispatch), and reactive power is computed as the corresponding lagging or leading component at the declared power factor. This is the default mode when `four_quadrant_control_mode` is not explicitly set.

**`VOLT_VAR`**
The inverter implements a piecewise-linear Volt-VAR curve defined by four voltage-reactive power breakpoints: (`V1`, `Q1`), (`V2`, `Q2`), (`V3`, `Q3`), (`V4`, `Q4`). Voltages are specified in per-unit relative to `V_base`, and reactive power values are in per-unit of rated power. The inverter reads its terminal voltage each timestep and interpolates the appropriate Q injection from the curve. A lockout timer (`volt_var_control_lockout`, in seconds) prevents rapid successive changes. This mode directly implements the smart inverter Volt-VAR function described in IEEE 1547.

**`VOLT_WATT`**
The inverter limits its real power output as a function of terminal voltage, using a two-point linear ramp defined by (`VW_V1`, `VW_P1`) and (`VW_V2`, `VW_P2`). Below `VW_V1` the inverter operates at full available power; above `VW_V2` it curtails to `VW_P2`. This implements the Volt-Watt curtailment function for overvoltage mitigation.

**`VOLT_VAR_FREQ_PWR`**
A combined mode that simultaneously applies a Volt-VAR schedule and a frequency-power schedule. The VAR response is driven by terminal voltage via `volt_var_sched`, and real power is modified as a function of measured frequency via `freq_pwr_sched`. Slew rate limits (`max_var_slew_rate` in VAr/s, `max_pwr_slew_rate` in W/s) prevent instantaneous jumps. A `delay_time` parameter introduces a response delay between observing a voltage or frequency deviation and applying the corresponding output change. The `disable_volt_var_if_no_input_power` flag allows the Volt-VAR function to be suppressed when the DC source (e.g., PV) is not producing power.

**`LOAD_FOLLOWING`**
The inverter monitors the power flow on a designated `sense_object` (a node or link) and dispatches the attached battery to charge or discharge in order to keep that power within defined thresholds. Charging begins when the sensed power falls below `charge_on_threshold` and stops at `charge_off_threshold`. Discharging begins above `discharge_on_threshold` and stops at `discharge_off_threshold`. Rates are bounded by `max_charge_rate` and `max_discharge_rate`. Lockout timers (`charge_lockout_time`, `discharge_lockout_time`) prevent rapid mode cycling.

**`GROUP_LOAD_FOLLOWING`**
An extension of load-following where multiple inverters coordinate their battery dispatch collectively. The group shares a common set of thresholds (`charge_threshold`, `discharge_threshold`) and aggregate rate limits (`group_max_charge_rate`, `group_max_discharge_rate`, `group_rated_power`). Individual inverters in the group scale their dispatch proportionally.

**`VOLTAGE_SOURCE` (VSI)**
Activates the voltage source inverter model for grid-forming operation. In this mode the inverter synthesizes its own internal voltage and acts as a controlled voltage source rather than a current source. This mode is only active in deltamode and is described further in Section 7.

---

## 4. Power Rating and Efficiency Parameters

**`rated_power`** (VA)
The rated apparent power capacity of the inverter. This is the primary sizing parameter and sets the ceiling on total VA output. It also serves as the base for per-unit calculations in Volt-VAR and Volt-Watt curves, and for computing the filter impedance base (`Zbase`) in VSI mode.

**`rated_battery_power`** (W)
When a battery is attached, this separately declares the battery's rated real power, which may differ from the inverter's total VA rating.

**`inverter_efficiency`**
A scalar (0–1) applied to the DC input power to compute available AC output in the four-quadrant model. A value of 0.95 means 5% of input power is lost in conversion. This is a flat efficiency — for more accurate representation across operating points, the multipoint efficiency model should be used instead.

**`battery_soc`** (pu)
The current state of charge of an attached battery, ranging from 0 to 1. The inverter reads this to determine whether charging or discharging is permissible. It does not write this value directly — the battery object owns the SOC state.

**`soc_reserve`** (pu)
The minimum SOC fraction that the battery must retain in islanded operation. The inverter will not discharge below this level when `islanded_state` is `true`.

**`P_In`**, **`V_In`**, **`I_In`**
DC-side input quantities. `P_In` is the primary input in most configurations, representing the DC power delivered by the attached resource. `V_In` and `I_In` are available for configurations where the DC voltage or current are independently meaningful.

**`VA_Out`**
The total AC apparent power output of the inverter as a complex quantity. This is the aggregate across all active phases.

---

## 5. Multipoint Efficiency Model

When `use_multipoint_efficiency` is set to `true` and a solar object is the child resource, the inverter replaces the flat `inverter_efficiency` scalar with a California Energy Commission (CEC) Sandia model efficiency curve. This model is parameterized by:

| Parameter | Description |
|---|---|
| `maximum_dc_power` (`p_dco`) | DC power at which the inverter reaches rated output |
| `maximum_dc_voltage` (`v_dco`) | DC voltage at the maximum power point |
| `minimum_dc_power` (`p_so`) | Minimum DC power required for the inverter to start |
| `c_0`, `c_1`, `c_2`, `c_3` | Polynomial coefficients of the efficiency curve |

Pre-configured coefficient sets are available for three manufacturers via the `inverter_manufacturer` property: `FRONIUS`, `SMA`, and `XANTREX`. Setting one of these populates the coefficients automatically.

---

## 6. Power Factor Regulation

Power factor regulation is an auxiliary function that can be layered on top of load-following or group load-following modes. It is enabled by setting `pf_reg` to `INCLUDED` or `INCLUDED_ALT`.

**`pf_reg_activate`**
The power factor magnitude below which regulation activates. When the power factor at the `sense_object` drops below this threshold, the inverter begins injecting reactive power to correct it.

**`pf_reg_deactivate`**
The power factor magnitude above which regulation is no longer needed and the inverter returns to its baseline dispatch.

**`pf_target`** (signed)
The desired power factor to maintain. Sign convention: positive is inductive (lagging), negative is capacitive (leading).

**`pf_reg_high`** / **`pf_reg_low`**
Outer bounds for the power factor regulation band. If the measured power factor exceeds `pf_reg_high`, the inverter goes to full reverse reactive injection. If it falls below `pf_reg_low`, regulation ceases entirely.

**`pf_reg_activate_lockout_time`** (s)
A mandatory pause between the deactivation of power factor regulation and its reactivation, preventing rapid oscillation.

---

## 7. Deltamode Dynamic Model

When the inverter is flagged for deltamode inclusion (via `OF_DELTAMODE` or the `all_generator_delta` global), the object registers three deltamode interface functions and transitions to a differential-equation-based representation during dynamic substepping. Two underlying controller implementations are available, selected by `dynamic_model_mode`.

### 7.1 Controller Architecture

**`dynamic_model_mode`** (`PI` / `PID`)
Selects between a proportional-integral controller (the default) and a proportional-integral-derivative controller for the current regulation loops.

The dynamic controller operates in the dq-reference frame, decomposing each phase's current injection into a real-axis (d) component and a reactive-axis (q) component. The controller computes current errors and drives modulation signals that determine the inverter's AC current injection.

**PI/PID Gain Parameters (per phase, for phases A/B/C):**

| Parameter | Description |
|---|---|
| `kpd`, `kpq` | d- and q-axis integration gains |
| `kid`, `kiq` | d- and q-axis proportional gains |
| `kdd`, `kdq` | d- and q-axis derivative gains (PID only) |

**Reference setpoints:**

| Parameter | Description |
|---|---|
| `Pref` | Real power reference (W) |
| `Qref` | Reactive power reference (VAr) |

**State variables (per phase):**

The controller exposes its internal state for observation and diagnostics: current errors (`epA/B/C`, `eqA/B/C`), error derivatives (`delta_epA/B/C`, `delta_eqA/B/C`), modulation signals (`mdA/B/C`, `mqA/B/C`), and their changes (`delta_mdA/B/C`, `delta_mqA/B/C`). The dq-axis current for each phase is exposed as `IdqA/B/C`.

**`inverter_convergence_criterion`**
The maximum allowable change in error before the deltamode substepper considers the inverter to have converged and allows the simulation to exit deltamode. Default is 1×10⁻³.

**`current_convergence`** (A)
A separate convergence criterion applied specifically to current changes on the first deltamode timestep, handling initialization transients.

### 7.2 Droop Control Parameters

**`inverter_droop_fp`** (boolean)
Enables frequency-power (f/P) droop control. When active, the inverter adjusts its real power output in proportion to the deviation of the measured frequency from the nominal reference (`f_nominal`, default 60 Hz).

**`R_fp`**
The droop coefficient for f/P control. A larger value produces a steeper power response per unit frequency deviation.

**`Tfreq_delay`** (s)
A first-order time constant for the frequency signal seen by the droop controller. This filters out measurement noise and prevents the inverter from reacting instantaneously to high-frequency transients.

**`inverter_droop_vq`** (boolean)
Enables voltage-reactive power (V/Q) droop control. When active, the inverter adjusts Q in proportion to the deviation of terminal voltage from the reference.

**`R_vq`**
The droop coefficient for V/Q control. Default is 0.05 pu.

**`Tvol_delay`** (s)
First-order time constant for the voltage signal seen by the V/Q droop controller.

**`Tp_delay`**, **`Tq_delay`** (s)
Time constants for the filtered real and reactive power signals seen by the VSI droop controller specifically.

### 7.3 Ramp Rate Limiting

The inverter can enforce ramp rate limits on both real and reactive power output during deltamode operation.

| Parameter | Description |
|---|---|
| `enable_ramp_rates_real` | Boolean to activate real power ramp limiting |
| `max_ramp_up_real` (W/s) | Maximum rate of increase in real power |
| `max_ramp_down_real` (W/s) | Maximum rate of decrease in real power |
| `enable_ramp_rates_reactive` | Boolean to activate reactive power ramp limiting |
| `max_ramp_up_reactive` (VAr/s) | Maximum rate of increase in reactive power |
| `max_ramp_down_reactive` (VAr/s) | Maximum rate of decrease in reactive power |

Default ramp rates are initialized to 1 GW/s and 1 GVAr/s (effectively unlimited), so enabling the flags without setting rates has no practical effect.

---

## 8. Voltage Source Inverter (VSI) Mode

VSI mode is activated by setting `four_quadrant_control_mode` to `VOLTAGE_SOURCE` in combination with deltamode. The inverter transitions from a controlled current source to a controlled voltage source, synthesizing its own internal voltage reference and acting as the grid-forming element in an islanded or weakly-connected network.

**`VSI_mode`** (`VSI_ISOCHRONOUS` / `VSI_DROOP`)
Selects between two grid-forming strategies. Isochronous mode holds voltage and frequency rigidly at setpoint (suitable when only one grid-forming source is present). Droop mode allows voltage and frequency to sag slightly under load, enabling multiple VSI sources to share load proportionally.

**`VSI_Rfilter`**, **`VSI_Xfilter`** (pu)
The resistance and inductance of the inverter's output filter, expressed in per-unit on the inverter's own base. These are used to compute the filter admittance injected into the parent node's full admittance matrix during deltamode, implementing the Norton-equivalent network interface. Default values are 0.02 pu (R) and 0.10 pu (X).

**`VSI_freq`** (Hz)
The internal frequency reference for the VSI. Default is 60 Hz.

**`ki_Vterminal`**, **`kp_Vterminal`**
Integral and proportional gains for the terminal voltage magnitude controller in isochronous VSI mode. Default integral gain is 5.86; proportional gain defaults to 0 (integral-only control).

**`V_set_droop`**
The voltage magnitude setpoint for the V/Q droop controller in `VSI_DROOP` mode.

**`Pmax`**, **`Pmin`** (pu)
Real power limits for the grid-forming inverter.

**`kppmax`**, **`kipmax`**
Proportional and integral gains of the Pmax limiting controller in droop mode. Defaults are 3 and 30 respectively.

**`Pmax_Low_Limit`**
Lower output limit of the Pmax controller. Defaults to −100, effectively unconstrained downward.

**Internal VSI state variables (hidden):**

`e_source_A/B/C` — Internal voltage magnitude for each phase of the grid-forming source.
`V_angle_A/B/C` — Internal voltage angle for each phase.
`pCircuit_V_Avg` — Three-phase average terminal voltage magnitude, used in droop calculations.

---

## 9. IEEE 1547 Protective Relay Logic

The inverter implements IEEE 1547-2003 and IEEE 1547a-2014 protective disconnect logic, activated by setting `enable_1547_checks` to `true`. The version is selected via `IEEE_1547_version` (`IEEE1547` or `IEEE1547A`), which auto-populates the default trip setpoints. Individual setpoints can then be overridden.

**`reconnect_time`** (s)
After a trip condition clears, the inverter waits this long before resuming generation. The 1547-2003 default is 300 seconds (5 minutes).

**`inverter_1547_status`** (boolean)
Read-only indicator of whether the inverter is currently active (`true`) or tripped due to a violation (`false`).

### Frequency Trip Bands (IEEE 1547a)

Two over-frequency and two under-frequency bands are defined, each with a setpoint and a clearing time:

| Parameter | Default (1547a) | Description |
|---|---|---|
| `over_freq_high_cutout` | 62.0 Hz | OF2 trip setpoint |
| `over_freq_high_disconnect_time` | 0.16 s | OF2 clearing time |
| `over_freq_low_cutout` | 60.5 Hz | OF1 trip setpoint |
| `over_freq_low_disconnect_time` | 2.0 s | OF1 clearing time |
| `under_freq_high_cutout` | 59.5 Hz | UF2 trip setpoint |
| `under_freq_high_disconnect_time` | 2.0 s | UF2 clearing time |
| `under_freq_low_cutout` | 57.0 Hz | UF1 trip setpoint |
| `under_freq_low_disconnect_time` | 0.16 s | UF1 clearing time |

### Voltage Trip Bands

Three under-voltage and two over-voltage bands are defined:

| Parameter | Default (1547a) | Description |
|---|---|---|
| `under_voltage_low_cutout` | 0.45 pu | Lowest UV threshold |
| `under_voltage_middle_cutout` | 0.60 pu | Mid UV threshold |
| `under_voltage_high_cutout` | 0.88 pu | High UV threshold |
| `over_voltage_low_cutout` | 1.10 pu | Low OV threshold |
| `over_voltage_high_cutout` | 1.20 pu | High OV threshold |

Each band has a corresponding `_disconnect_time` parameter. The inverter accumulates violation time internally and trips once the accumulated violation time for any band exceeds its clearing time.

**`IEEE_1547_trip_method`**
A read-only enumeration that reports which threshold caused the most recent trip: `OVER_FREQUENCY_HIGH`, `OVER_FREQUENCY_LOW`, `UNDER_FREQUENCY_HIGH`, `UNDER_FREQUENCY_LOW`, `UNDER_VOLTAGE_LOW/MID/HIGH`, or `OVER_VOLTAGE_LOW/HIGH`.

---

## 10. DC Interface Parameters

**`V_In`** (V), **`I_In`** (A), **`P_In`** (W)
The DC-side voltage, current, and power. For solar-connected inverters, `P_In` is driven by the child `solar` object's computed output. For battery-connected inverters, the battery object populates this based on its dispatch state.

**`Vdc`** (V)
A legacy DC voltage parameter retained for backward compatibility with older models that used the TWO/SIX/TWELVE_PULSE inverter types.

---

## 11. AC Output Observable Variables

These are the primary quantities to record in output players or recorders:

| Property | Description |
|---|---|
| `VA_Out` | Total AC apparent power output (complex, VA) |
| `power_A/B/C` | Per-phase apparent power (complex, VA) |
| `phaseA/B/C_V_Out` | Per-phase AC terminal voltage (complex, V) |
| `phaseA/B/C_I_Out` | Per-phase AC current (grid-following) |
| `phaseA/B/C_I_Forming` | Per-phase AC current (grid-forming / VSI) |
| `curr_VA_out_A/B/C` | Current-timestep per-phase power (used for convergence) |
| `prev_VA_out_A/B/C` | Previous-timestep per-phase power (used for convergence) |

The distinction between `I_Out` (grid-following) and `I_Forming` (grid-forming) reflects that the two operating modes produce fundamentally different current quantities — one is the controlled injection from a current-source model, the other is the measured terminal current flowing through the VSI's filter impedance.