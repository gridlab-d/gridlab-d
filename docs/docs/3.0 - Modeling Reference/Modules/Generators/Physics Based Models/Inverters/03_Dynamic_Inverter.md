# Dynamic Inverter Model

## Background and Motivation

While the QSTS inverter model is practical for models intended for system planning and long-term energy analysis, GridLAB-D™ also offers the ability to simulate sub-second dynamics when transient behavior matter for stability, protection, or control performance, especially critical with high penetrations of inverter-based resources (IBRs).

To address electromechanical transients, frequency dynamics, and control interactions, particularly in microgrids and weakly interconnected feeders where IBRs may be the primary source of voltage and frequency regulation [1], GridLAB-D™ developed the `inverter_dyn` class. It implements three-phase electromechanical dynamic models of inverters, enabling transient stability simulations of large-scale, three-phase unbalanced distribution systems [1]. The models have been validated against electromagnetic simulation results [1] and field test data from the CERTS/AEP microgrid testbed [1, 2], and they have been applied to islanded feeder studies with over 5,000 nodes [1].

### Design Philosophy

At its core, the `inverter_dyn` object — implemented in `inverter_dyn.cpp` — models the inverter with different circuit representations depending on the control mode.

For the **_grid-forming_** paradigm, the model represents the inverter as a voltage source (the internal synthesized AC voltage) behind a coupling impedance (a filter reactance and optional resistance). Thus, the grid-forming inverter autonomously synthesizes its own voltage magnitude and frequency reference.

In the **_grid-following_** paradigm, although the same voltage-source converter hardware is used, the control strategy makes the model behave as controlled current sources from the network's perspective. Hence, the grid-following inverter tracks the grid voltage and phase using a phase-locked loop (PLL) and injects a commanded current.

These two fundamental paradigms are captured in the `control_mode` enumeration:

- `GRID_FORMING`: Inverter acts as a voltage source setting its own frequency and voltage. It is suitable for islanded microgrids or as a reference machine.
- `GRID_FOLLOWING`: Inverter acts as a controlled current source locked to the grid. It is used for grid-tied PV/BESS operating under a stiff grid reference.
- `GFL_CURRENT_SOURCE`: Simplified grid-following representation that injects current directly without modeling the inner voltage-source converter dynamics.

!!! note

    The distinction between `GRID_FORMING` and `GRID_FOLLOWING` is not merely a parameter choice — it changes the fundamental circuit equivalent, the control equations solved at each timestep, and the Norton admittance contributed to the system Jacobian. Mixing both types on the same feeder is the normal operating scenario for high-IBR distribution studies. [1]

### Network Interface

Similarly to its QSTS counterpart, the dynamic inverter model connects to the distribution network by parenting to a powerflow node, meter, load, or their triplex (split-phase) equivalents. The object publishes its *Norton equivalent* (current injection and admittance) to the powerflow solver. Once updated, the powerflow solver works out the nodal voltage equations for the whole network. Subsequently, the inverter model reads back the solved terminal voltages and updates its internal state variables (internal angle, frequency, d-q currents, PLL outputs, etc.) so that the system dynamics can move forward in time, through the connection shown in [](#fig:inverter_dyn_connection).

![Dynamic Inverter equivalent circuit network interface [1]](../../../../../../images/inverter_dyn_connection.png){ #fig:inverter_dyn_connection }

To be linked with a three-phase distribution network powerflow solver, the voltage source model is converted to its Norton equivalence, as shown in [](#fig:inverter_dyn_Thevenin2Norton) (a) and (b).

![Dynamic Inverter equivalent circuits: (a) Inverter internal voltage source, (b)
Norton equivalent current source [1]](../../../../../../images/inverter_dyn_Thevenin2Norton.png){ #fig:inverter_dyn_Thevenin2Norton }

## Parameters and Functionality

Details on the variable types and units expected by GridLAB-D™ are given along with the code insider documentation in [](#tbl:inverter-dyn-parameters). The following sections detail some of the parameters and their functionality.

### Nameplate and circuit parameters

For its dynamical inverter model, GridLAB-D™ provides foundational nameplate and circuit parameters — the per-unit base and the filter impedance — that are used to internally normalize the control variables. These parameters are:

-  `rated_power`: *Rated power* of the inverter is the per-unit base for all power quantities, i.e., `Pset`, `Pref`, `Qref`, `Pmax`, `Qmax`, etc.
- `rated_DC_Voltage`: *Rated DC bus voltage* is used as the base for the DC bus voltage. [](#eq:eq1).
- `Pref`: *Active power reference* for the current operating interval that feeds into the droop as the power measurement comparison point, and can be updated externally by a supervisory controller or energy management system.
- `Xfilter`: *Per-unit reactance of the AC output filter* (coupling inductance). Forms the imaginary part of the Thévenin impedance $Z_{f}$ in equation (1) behind which the inverter's internal voltage source sits. A smaller `Xfilter` means tighter coupling to the grid, that is faster current response but potentially lower stability margins.
- `Rfilter`: *Per-unit resistance of the AC output filter*. Forms the real part of $Z_{f}$ in equation (1).
- `pdispatch`: *The desired active power dispatch setpoint* for the inverter, in per unit of `rated_power`. If left unset, the inverter does not use the dipatch logic and falls back to `Pset`/`Pref`. However, it become the top-level active power target driving the control chain when set to a valid pu value either through GridLAB-D™ model or an external controller.
- `pdispatch_offset`: *Additive offset* applied on top of `pdispatch`. It defaults to 0 and is intended for situations where a supervisory energy management system sets a base dispatch level via `pdispatch` and a secondary controller (e.g., a local frequency response or a demand response signal) applies a small correction via the offset, without overwriting the base setpoint. The effective dispatch target become `pdispatch` + `pdispatch_offset`.

### Grid-Forming (GFM) Inverter — Physical Model { #sec:grid-forming-inverter-physical-model }

#### Equivalent Circuit

A grid-forming inverter is represented as an ideal three-phase sinusoidal voltage source $E$ behind a coupling reactance $X_f$ (and optional resistance $R_f$). For power flow, this Thévenin source is converted to a Norton equivalent:

$$\displaystyle{}I_{Norton} = \frac{E}{Z_{f}} \tag{1}$$

$$\displaystyle{}Y = \frac{1}{Z_{f}} \tag{2}$$

to calculate the current and admitance to be injected in the distribution network at the node or load connection point, as shown in [](#fig:inverter_dyn_grid_forming_concept).

![Grid-Forming Inverter model concept [1]](../../../../../../images/inverter_dyn_grid_forming_concept.png){ #fig:inverter_dyn_grid_forming_concept }

Two sub-modes govern how $E$ is computed:

- `CONSTANT_DC_BUS`: The DC bus voltage $V_{dc}$ is held fixed. $E$ is computed from the droop controller output and the fixed $V_{dc}$. This sub-mode offers a simpler functionality ignoring DC dynamics.
- `DYNAMIC_DC_BUS`: The DC bus capacitor voltage $V_{dc}$ evolves dynamically based on power balance between the DC source (PV, BESS) and AC output. Required for accurate BESS or PV transient studies.

#### CERTS Droop Control

![Grid-Forming Inverter CERTS Droop Control: (a) Q-V droop control and (b) P-f droop control
and overload mitigation control. [1]](../../../../../../images/inverter_dyn_certs_control.png){ #fig:inverter_dyn_certs_control }

The default grid-forming controller implements the CERTS (Consortium for Electric Reliability Technology Solutions) droop control [1, 2] — a decentralized control law where:

- **Q-V droop:** Reactive power ($Q$) is regulated by adjusting the internal voltage magnitude $E$. As $Q$ increases above the setpoint, $E$ droops below the reference. Consequently, circulating reactive power between paralleled grid-forming inverter gets mitigated.
- **P-f droop:** Active power ($P$) is regulated by adjusting the inverter output frequency. As $P$ increases above the setpoint, frequency droops below nominal. This ensures the inverters phase angles get synchronized power is shared adequately.

Bearing in mind that for practical control implementation reasons, more parameters might be requested, to set the **Q-V droop** control in [](#fig:inverter_dyn_certs_control) (a) in GridLAB-D™, the `inverter_dyn` object should include the following parameters:

- `Vset`: *Voltage setpoint*, that is the terminal voltage at which $Q$ output is zero — the droop curve's operating point.
- `mq` or `Q_V_droop`: *Q-V droop gain* defines the slope of the $Q$ vs. $V$ curve, that is how much $Q$ the inverter injects per unit of voltage deviation from `Vset`. A larger `mq`/`Q_V_droop` means a steeper response — more reactive power for a given voltage error.
- `Qref`: *Reactive power reference setpoint*, that is the $Q$ dispatch target around which the droop operates.
- `Qmax`: *Upper reactive power limit*, that is the highest threshold above which the inverter will not inject more Q regardless of how far voltage sags.
- `Qmin`: *Lower reactive power limit*, which caps leading reactive power (absorption).
- `kpqmax`: *Proportional gain of the Qmax anti-windup controller*. When $Q$ hits the `Qmax` limit, this controller modulates the voltage reference to prevent integrator windup in the outer voltage loop.
- `kiqmax`: *Integral gain of the Qmax anti-windup controller*. Works alongside `kpqmax` for the same limiting purpose.
- `Tq`: *Time constant* of the reactive power measurement low-pass filter. The raw $Q$ measurement is smoothed through a first-order lag with this time constant before entering the droop law, preventing the controller from reacting to noise or fast switching transients.
- `Tv`: *Time constant* of the voltage measurement low-pass filter. Similarly to `Tq`, it smooths the terminal voltage measurement `v_measure` before it is compared against `Vset`.
- `kpv`: *Proportional gain of the inner voltage control loop*. In implementations where the Q-V droop feeds an inner proportional-integral (PI) voltage regulator (rather than directly commanding $E$), this is the proportional term of that PI.
- `kiv`: *Integral gain of the inner voltage control loop*. This is the integral term of the same PI, designed to minimize/eliminate steady-state voltage error at the operating point.
- `E_max` or `Emax`: *Upper saturation limit on the voltage controller output* , i.e., on the magnitude of the internal synthesized voltage $E$, which prevents the droop from commanding an unrealistically high internal electromotive force (EMF).
- `E_min` or `Emin`: *Lower saturation limit on the voltage controller output*. It prevents collapse of the internal voltage reference during severe undervoltage events.
- `v_measure`: A GridLAB-D™ hidden diagnostic variable, this is the actual low-pass filtered terminal voltage value currently seen by the droop controller, after the `Tv` filter.
- `q_measure`: A GridLAB-D™ hidden diagnostic variable, this is the actual low-pass filtered $Q$ measurement currently entering the droop law, after the `Tq` filter.
- `E_mag`: A GridLAB-D™ hidden diagnostic variable, this is the resulting magnitude of the internal voltage $E$ being synthesized by the inverter, that is the output of the entire Q-V droop plus voltage control chain.

In summary, the Q-V droop control operation is conducted by

- The two measurement filters (`Tq`, `Tv`) to decouple the controller from fast noise,
- The droop slope `mq` to set the steady-state Q-V characteristic,
- The PI (`kpv`, `kiv`) to eliminate any residual voltage error, and
- The output limiters (`Emin`/`Emax`) and the Qmax anti-windup (`kpqmax`/`kiqmax`) to enforce the inverter's hardware and thermal boundaries.

The **P-f droop** can be defined in two ways via the `P_f_droop_setting_mode` parameter:

- `FSET_MODE`: The frequency setpoint `fset` is the primary parameter; the $P$ setpoint is derived from it. This is the natural choice for isochronous or equal-share droop configurations. The isochronous (swing) unit holds frequency at exactly the nominal value, hence `fset = 60 Hz`. However, when multiple inverters are to share load proportionally without any of them having a pre-assigend dispatch, they all receive the same `fset` and the same droop slope `mp`.
- `PSET_MODE`: The active power setpoint `Pset` is the primary parameter; the frequency setpoint is derived. This option is the logical one when units have assigned dispatch levels, e.g., one BESS is dispatched to carry 50 kW and another 100 kW.

Similarly to the Q-V droop control, the P-f droop control in [](#fig:inverter_dyn_certs_control) (b) is defined by certain parameters in GridLAB-D™, which can be specified in the `inverter_dyn` object:

- `P_f_droop_setting_mode`: Parameter that selects whether the droop curve is anchored by a frequency setpoint (`FSET_MODE`) or a power setpoint (`PSET_MODE`). It determines which of `fset` or `Pset` is the primary input.
- `fset`: *Frequency setpoint*. This is the frequency at which the inverter's power output $P$ equals `Pset`. It is the primary parameter in `FSET_MODE` and the derived parameter from the droop curve in `PSET_MODE`.
- `Pset`: *Active power setpoint*. This is the MW dispatch target around which the droop operates. It represents the primary parameter in `PSET_MODE` and the derived parameter from the droop curve in `FSET_MODE`.
- `mp`: *P-f droop gain*, expressed in rad/s per pu power, defines the slope of frequency $f$ vs. active power $P$ curve in angular frequency units. It is typically set to $3.77 \; rad/s/pu$ ($≈ 2π × 0.6 \; Hz/pu$), corresponding to a 1% frequency droop at full load.
- `P_f_droop`: *P-f droop gain* is expressed in per-unit (i.e., as a fraction of rated frequency per pu power) and it is typically set to 0.01 (1%). It is an alternative representation of the same droop slope as `mp`, the two being related by `mp = P_f_droop × w_ref`.
- `w_ref`: *Rated angular frequency reference*, nominally at $2π \; × \; 60 \; ≈ \; 376.99 \; rad/s$ for a 60 Hz system, it serves as the base for frequency error calculation and links `mp` to `P_f_droop`.
- `freq`: *Output frequency* computed by the P-f droop controller at the current operating point representing what the inverter actually synthesizes as its frequency.

- `Pmax`: *Upper active power limit* that ensures the inverter will not produce more active power than prescribed, regardless of how far frequency sags.
- `Pmin`: *Lower active power limit* is the symmetric lower clamp and limits motoring or reverse power flow for BESS applications, for example.
- `kppmax`: *Proportional gain* of the `Pmax` anti-windup controller. When $P$ hits the `Pmax` limit, this controller modulates the frequency/angle reference to prevent integrator windup in the outer angle loop.
- `kipmax`: *Integral gain* of the `Pmax` anti-windup controller. It works alongside `kppmax` for the same limiting purpose.
- `w_lim`: *Saturation limit* on the output of the `Pmax` anti-windup controller. It caps how aggressively the anti-windup correction can perturb the frequency reference, preventing the limiter itself from destabilizing the angle integrator.
- `Tp`: *Time constant* of the active power measurement low-pass filter. The raw $P$ measurement is smoothed through a first-order lag with this time constant before entering the droop law, filtering out switching noise and fast power transients.

**_Example:_**

```bash
object inverter_dyn {
    name simple_DC_model_grid_forming_inverter;
    parent grid_form_simple_DC;
    rated_power 100 kW;	 // full rated power (not per-phase)
    flags DELTAMODE;
    control_mode GRID_FORMING;
    
    //Criterion to exit transient mode
    frequency_convergence_criterion 1e-9;  //Convergence criterion (rad/s)
    voltage_convergence_criterion 1e-3;	   //Convergence criterion (V)

    //Pref and Qref only initialize non-SWING-connected
    //(if SWING, powerflow initializes)
    Pref 1.0 kW;    //Real power reference for initialization
    Qref 200 W;	    //Reactive power reference for initialization

    E_max 1.2;		//Maximum internal voltage output
    Rfilter 0.005;  //Real portion of inverter filter (pu)
    Xfilter 0.05;	//Reactive portion of inverter filter (pu)

    // P-f droop
    mp 3.77;  		//P-f droop in rad/s/pu - 3.77 represents 1% droop
    kppmax 3;		//Proportional gain for P_Max controller
    kipmax 60;		//Integral gain fo P_Max controller
    Pmax 1.5;		//Maximum power controller can deliver (pu)
    Pmin 0;		    //Minimum power controller can deliver (pu)

    // Q-V droop
    mq 0.05; 		//Q-V droop -  0.05 represents 5% droop
}
```

#### Isochronous Control

As referenced in [\[2\]](#ref2), in `FSET_MODE`, the inverter has a frequency-reference-first thinking — `f_set` is the primary quantity and $P$ will follow. Hence, in an islanded microgrid, an inverter can be designated as the **isochronous ("swing") unit** maintaining frequency at exactly the nominal value (e.g., 60 Hz) and balancing whatever real power is required. This is the grid-forming analogue of a synchronous swing bus. The CERTS droop architecture was designed to support this operational mode. 

#### Current Limiting

Unlike a synchronous machine, an inverter has hard current limits imposed by the power electronics (IGBT/SiC ratings). `inverter_dyn` implements a **current-limiting strategy** for grid-forming inverters: when the magnitude of the computed output current exceeds `I_max`, the internal voltage reference is modified (via a virtual impedance or direct clamping) to bring the current within limits. The intermediate, unclamped currents are exposed as `I[A/B/C]_Out_PU_temp[]` for diagnostic purposes. Setting the flags `phase_angle_correction` and `virtual_resistance_correction` to true enables two layers of correction to mitigate the impact of current limiting on the inverter's voltage angle and magnitude, respectively, which can help maintain stability during overload conditions.

### Grid-Following (GFL) Inverter — Physical Model { #sec:grid-following-inverter-physical-model }

#### Equivalent Circuit

Although a grid-following inverter uses the same voltage-source converter hardware as a grid-forming inverter (see [](#fig:inverter_dyn_connection)), its control strategy makes it behave as a **controlled current source** from the network's perspective. The terminal voltage is determined by the grid; the inverter modulates its internal voltage $E$ to inject a desired current, as detailed in [1] and shown in [](#fig:inverter_dyn_grid_following_concept).

![Grid-Following Inverter model concept [1]](../../../../../../images/inverter_dyn_grid_following_concept.png){ #fig:inverter_dyn_grid_following_concept }

Following the representation in [1], GridLAB-D™ implements both components of the grid-following control strategy, that is:

- the phase-locked loop (PLL) in [](#fig:inverter_dyn_glf_pll), used to estimate the phase angle $\angle{\delta_g}$ of the grid side voltage. With $\angle{\delta_g}$ obtained, the controller can inject the specified $P$ and $Q$ into the grid.
- the current control loop in [](#fig:inverter_dyn_glf_current_control) to quickly regulate the current $I_g\angle{\phi_g}$ injected into the grid, so the grid-following inverter can behave as a current source.

![Grid-Following Inverter model — PLL control block per phase ($i \in \{A,B,C\}$) [1]](../../../../../../images/inverter_dyn_glf_pll.png){ #fig:inverter_dyn_glf_pll }

![Grid-Following Inverter model — Current control block per phase ($i \in \{A,B,C\}$) [1]](../../../../../../images/inverter_dyn_glf_current_control.png){ #fig:inverter_dyn_glf_current_control }

The following assumptions are made when implementing grid-following inverter in GridLAB-D™: 

- For simplicity, the grid-following controller is modeled per phase. This means each phase has its own PLL and current control loop.
- We assume each phase injects the same amount of P and Q into the grid.
- For a split phase connection, we assume the inverter is connected between two phases, Phase 1 and Phase 2.

For the `GRID_FOLLOWING` control mode, two sub-modes are available via parameter `grid_following_mode`:

- `BALANCED_POWER`, which assumes balanced three-phase operation. The inverter injects equal power on all three phases based on total $P$ and $Q$ commands. This is a computationally efficient control algorithm, appropriate when phase imbalance is small.
- `POSITIVE_SEQUENCE`, which models only the positive-sequence component. Equivalent to a phasor-domain single-phase representation scaled to three phases, this control is used for positive-sequence transmission-style studies run in a three-phase environment.

#### Phase-Locked Loop (PLL)

The PLL is the sensing subsystem that tracks the grid's voltage angle so the inverter can synchronize its current injection. In `inverter_dyn`, the PLL output (angle and frequency) drives the dq-frame current control to achieve synchronization with the grid, as detailed in [1]. The key PLL parameters — proportional `kpPLL` and integral `kiPLL` gains — control how quickly the inverter responds to grid angle changes to bring the grid voltage on q-axis to zero.

#### Inner Current Control

In full `GRID_FOLLOWING` mode (not `GFL_CURRENT_SOURCE`), the model includes an **inner current control loop** that outputs the internal voltages $e_{di}$ and $e_{qi}$ for each phase. `kpc` and `kic` are the proportional and integral gains of the current control loop, respectively, while `F_current` represents the feed forward term gain ($k$ in the diagram in [](#fig:inverter_dyn_glf_current_control)). In the rotating dq reference frame:

- The **d-axis** current reference is derived from the active power command: $i_{gdi\_ref} = \frac{Pref}{u_{gdi}}$, and
- The **q-axis** current reference is derived from the reactive power command: $i_{gqi\_ref} = \frac{-Qref}{u_{gqi}}$.

GridLAB-D™ offers two external controllers to synthesize $Pref$ and $Qref$, that is the *frequency-watt* and *volt-var* controllers, respectively. Both controllers are given the option to be enabled or disabled by setting the model properties `frequency_watt` and `volt_var`.

[](#fig:inverter_dyn_gfl_freq_watt) shows the control block of the Frequency-Watt control. It measures the variation of frequency and changes the reference of output power $P$. The frequency is measured by a PLL.

![Grid-Following Inverter model — Frequency-watt control](../../../../../../images/300px-Inv_dyn_fig9.png){ #fig:inverter_dyn_gfl_freq_watt }

The first order lag filters of the external frequency-watt controller are defined by the following parameters:

- `Tpf`: the time constant of the power measurement low pass filter,
- `Tff`: the time constant of frequency measurement low pass filter,
- `frequency_watt_droop` or `Rp`: the p-f droop gain,
- `db_UF`: upper limit of dead band for under-frequency (UF) control,
- `db_OF`: lower limit of dead band for over-frequency (OF) control,
- `rampUpRate_real` and `rampDownRate_real`: active power control ramping up/down rate, considered only if flag `checkRampRate_real` is set to true.

Similarly, [](#fig:inverter_dyn_glf_volt_var) shows the control block of the Volt-Var control. It measures the variation of voltage and changes the reference of reactive power $Q$. 

![Grid-Following Inverter model — Volt-var control](../../../../../../images/300px-Inv_dyn_fig10.png){ #fig:inverter_dyn_glf_volt_var }

The first order lag filters of the external volt-var controller are defined by the following parameters:

- `Tqf`: the time constant of the reactive power measurement low pass filter,
- `Tvf`: the time constant of the voltage measurement low pass filter,
- `volt_var_droop` or `Rq`: the Q-V droop gain,
- `db_UV`: upper limit of dead band for under-voltage (UV) control,
- `db_OV`: lower limit of dead band for over-voltage (OV) control,
- `rampUpRate_reactive` and `rampDownRate_reactive`: reactive power control ramping up/down rate, considered only if flag `checkRampRate_reactive` is set to true.

These lag filters are designed to smooth the current references before they drive the inverter's modulated voltage.

**_Example:_**

```bash
object inverter_dyn {
    name Grid_Following_Inverter;
    rated_power 100 kW;  // full rated power (not per-phase)
    flags DELTAMODE;
    control_mode GRID_FOLLOWING;
    //grid_following_mode BALANCED_POWER;  // Inject balanced power
    grid_following_mode POSITIVE_SEQUENCE; // Inject balanced currents
    frequency_watt true;
    volt_var true;
    Pref_max  1;     // Active power limit
    Pref 100 kW;     // Active power reference
    Qref 0.0 kvar;   // Reactive power reference
    Rfilter 0.005;   // Real portion of inverter filter (pu)
    Xfilter 0.05;    // Reactive portion of inverter filter (pu)
    kpPLL  50;       // Proportional gain of PLL
    kiPLL  900;      // Integral gain of PLL
    kpc  0.05;       // proportional gain of current loop
    kic  5;          // Integral gain of current loop
    F_current 0.5;   // Feedforward term

    Rp 0.05;         // Frequency-watt droop 5% 
    Tpf 0.25;        // Time constant for frequency-watt
    Tff 0.02;        // Delay for frequency measurement in Frequency-watt

    Rq 0.05;         // Volt-var droop 5%
    Tqf 0.2;         // Time constant for volt-var
    Tvf 0.05;        // Delay for voltage measurement in volt-var
}
```

### Grid-Following Inverter - Current Source Representation

In the [Grid-Forming Inverter — Physical Model](#sec:grid-forming-inverter-physical-model) section, the grid-following inverter is represented as a voltage source behind impedance ([](#fig:inverter_dyn_Thevenin2Norton)), and the detailed inner current control loop is modeled. 


In the detailed [Grid-Following Inverter — Physical Model](#sec:grid-following-inverter-physical-model), the inverter internally models the voltage-source converter hardware and inner current control loop ([](#fig:inverter_dyn_Thevenin2Norton) (a)), which behaves as a controlled current source from the network's perspective ([](#fig:inverter_dyn_Thevenin2Norton) (b)). However, one drawback of this method from GridLAB-D™ simulation run is the low efficiency as the simulation step has to be set less than 2 ms. There could be also issues with the numerical stability of the solution. Therefore, this section introduces a simplified current source representation in which the shunt admittance $Y_{L}$ is ignored. Although the dynamic response of the current loop is ignored, the simulation efficiency can be improved.

To set a grid-following inverter as a current source, the `control_mode` parameter should be set to `GFL_CURRENT_SOURCE`. In this mode, the PLL is still kept as in [](#fig:inverter_dyn_gfl_pll), but the current loop is represented by simple low-pass filters defined by time constant `Tif`.

**_Example:_**

```bash
object inverter_dyn {
    name Grid_Following_Inverter;
    rated_power 100 kW;	 // full rated power (not per-phase)
    flags DELTAMODE;
    control_mode GFL_CURRENT_SOURCE;
    //grid_following_mode BALANCED_POWER;  // Inject balanced power
    grid_following_mode POSITIVE_SEQUENCE; // Inject balanced currents
    frequency_watt true;
    volt_var true;
    Pref_max  1;     // Active power limit
    Pref 100 kW;     // Active power reference
    Qref 0.0 kvar;   // Reactive power reference
    Rfilter 0.005;   // Real portion of inverter filter (pu)
    Xfilter 0.05;	 // Reactive portion of inverter filter (pu)
    kpPLL  50;       // Proportional gain of PLL
    kiPLL  900;      // Integral gain of PLL
    kpc  0.05;       // proportional gain of current loop
    kic  5;          // Integral gain of current loop
    F_current 0.5;   // Feedforward term

    Rp 0.05;         // Frequency-watt droop 5%	
    Tpf 0.25;        // Time constant for frequency-watt
    Tff 0.02;        // Delay for frequency measurement in Frequency-watt
    
    Rq 0.05; 		 // Volt-var droop 5%
    Tqf 0.2;         // Time constant for volt-var
    Tvf 0.05;        // Delay for voltage measurement in volt-var

    Tif 0.002;        // Time constant for current loop low-pass filter
}
```

### IEEE 1547 Inverter Compliance Testing

Enabling and setting the IEEE 1547 standard options are intended to facilitate compliance testing of inverters against the IEEE 1547, the Institute of Electrical and Electronics Engineers (IEEE) standard that defines interconnection and interoperability requirements for distributed energy resources with the electric power system. 

The IEEE 1547 block in `inverter_dyn` implements autonomous protection and disconnect logic — it makes the inverter self-monitor its operating point and trip offline (stop injecting current) if the grid conditions fall outside bounds prescribed by the IEEE 1547 standard for interconnection of distributed energy resources. Currently, the IEEE 1547 cessation/tripping functionality is only enabled for grid following inverters by setting the flag `enable_1547_checks` to true. If set for a grid forming inverter, this flag is deactivated, and hence all parameters are ignored.

In a quasi-steady state (QSTS)-only simulation, inverter protection is typically modeled externally (if at all) by checking power flow results and toggling objects on or off at event boundaries. Because `inverter_dyn` operates in transient mode, it can enforce the _time-delayed_ nature of protection correctly. For example, a momentary voltage sag that recovers in 0.1 s will not trip an inverter whose clearing time is 2.0 s, which is physically accurate and matters for studies like fault ride-through, islanding, and cold load pickup.

The following parameters are used to define the thresholds and timing for the IEEE 1547 cessation/tripping logic:

- **The version selector** — `IEEE_1547_version`: an enum that auto-populates default paramters to match a specific standard version:

    - `NONE`: no auto-population; user must set all thresholds manually,
    - `IEEE1547_2003`: original IEEE 1547-2003, having simpler, fewer bands,
    - `IEEE1547A_2014`: amendment 1547a-2014 (default in the code); adds the over/under-frequency bands,
    - `IEEE1547_2018`: fully revised IEEE 1547-2018; the most stringent; ride-through requirements.

    Setting `IEEE_1547_version` in the model file is the easiest way to get a standards-compliant protection configuration without specifying every threshold individually. Individual parameters can be overridden afterward if needed.

 - **Supporting variables**

    - `enable_1547_checks`: boolean master on/off switch. It defaults to _false_, so protection is opt-in. The inverter runs without any trip logic unless this flag is explicitly set to _true_.
    - `inverter_1547_status`: read-only output boolean getting set to _false_ if the inverter is tripped.
    - `reconnect_time`: the amount of time in seconds for how long the conditions must remain within limits before the inverter resumes generation. It defaults to 300 s (5 minutes), matching the IEEE 1547-2003 recommendation.
    - `IEEE_1547_trip_method`: output recording which threshold caused the most recent trip — one of the following:
      
      - `NONE`: no trip reason,
      - `OVER_FREQUENCY_LOW`: Low over-frequency level trip - OF1,
      - `OVER_FREQUENCY_HIGH`: High over-frequency level trip - OF2,
	  - `UNDER_FREQUENCY_LOW`: Low under-frequency level trip - UF1,
      - `UNDER_FREQUENCY_HIGH`: High under-frequency level trip - UF2,
      - `UNDER_VOLTAGE_LOW`: Lowest under-voltage level trip,
      - `UNDER_VOLTAGE_MID`: Middle under-voltage level trip,
      - `UNDER_VOLTAGE_HIGH`: High under-voltage level trip,
      - `OVER_VOLTAGE_LOW`: Low over-voltage level trip,
      - `OVER_VOLTAGE_HIGH`: High over-voltage level trip.

      This output is useful for post-analysis of what protection element operated during a transient event.

  - **The two quantities monitored**

      - Frequency — four bands, mirroring the OF1/OF2/UF1/UF2 structure of IEEE 1547a-2014, defined by a setpoint parameter in Hz and a clearing time parameter in seconds (and their default values):

           - `over_freq_low_cutout` [60.5 Hz] and `over_freq_low_disconnect_time` [2.0 s] for OF1 (low over-freq),
           - `over_freq_high_cutout` [62.0 Hz] and `over_freq_high_disconnect_time` [0.16 s] for OF2 (high over-freq),
           - `under_freq_low_cutout` [57.0 Hz] and `under_freq_low_disconnect_time` [0.16 s] for UF1 (low under-freq).
           - `under_freq_high_cutout` [59.5 Hz] and `under_freq_high_disconnect_time` [2.0 s] for UF2 (high under-freq).
           
       - Voltage — five bands across under- and over-voltage:
           
           - `under_voltage_low_cutout` [0.45 pu] and `under_voltage_low_disconnect_time` [0.16 s] for UV lowest,
           - `under_voltage_middle_cutout` [0.60 pu] and `under_voltage_middle_disconnect_time` [1.0 s] for UV middle,
           - `under_voltage_high_cutout` [0.88 pu] and `under_voltage_high_disconnect_time` [2.0 s] for UV high,
           - `over_voltage_low_cutout` [1.10 pu] and `over_voltage_low_disconnect_time` [1.0 s] for OV low,
           - `over_voltage_high_cutout` [1.20 pu] and `over_voltage_high_disconnect_time` [0.16 s] for OV high.

### Inverter Output Variables

Among the output variables, with per-unit (PU) quantities being normalized to the inverter's rated MVA and rated voltage, the following are particularly important for monitoring and analysis:

- `phaseA/B/C_I_Out`: actual AC terminal current in Amperes injected by the inverter on each phase (complex phasor),
- `phaseA/B/C_I_Out_PU`: actual AC terminal current normalized to rated current,
- `IA/B/C_Out_PU_temp`: pre-limiting current used during the current-limiting calculation for GFM inverters,
- `power_A/B/C`: complex AC power output per phase,
- `VA_Out`: total three-phase complex power output.

## Typical Use Cases

### Islanded Microgrid

One or more inverters are set to **GRID_FORMING** with CERTS droop control. At least one acts as isochronous (swing reference). The remainder participate in proportional power sharing via P-f and Q-V droop. This is the canonical use case validated against CERTS/AEP microgrid field data in [1, 2], where the test bed near Columbus, Ohio demonstrated seamless islanding and resynchronization meeting all IEEE 1547 and power quality requirements.

### Grid-Connected Feeder with High PV/BESS Penetration

Grid-following inverters (**GRID_FOLLOWING**, **BALANCED_POWER**) model rooftop PV systems injecting at a given **P_set** and **Q_set**. A few grid-forming inverters may represent community storage or a substation-side smart inverter providing local voltage regulation. The transient-mode engine resolves the fast transients during cloud events or load steps. [1]

### Islanding Detection and Transition Studies

A feeder initially connected to the utility (with the substation as the voltage/frequency reference) is disconnected mid-simulation. Grid-forming inverters must detect islanding and take over the reference role. Because **inverter_dyn** integrates dynamic state variables continuously, it can model the frequency excursion and recovery during this transition. The CERTS test bed demonstrated this capability in hardware. [2]

### Frequency Response Studies

Because the droop coefficients directly map frequency deviation to active power response, **inverter_dyn** can be used to study under-frequency load shedding (UFLS) interactions, synthetic inertia provision (via appropriate droop time constants), and co-existence of IBRs with synchronous generators. Results comparing grid-forming and grid-following performance in large distribution systems are presented in [1].

## References

[1] W. Du, F. K. Tuffner, K. P. Schneider, R. H. Lasseter, J. Xie, Z. Chen, and B. P. Bhattarai, "Modeling of Grid-Forming and Grid-Following Inverters for Dynamic Simulation of Large-Scale Distribution Systems," *IEEE Transactions on Power Delivery*, vol. 36, no. 4, pp. 2035–2045, Aug. 2021. DOI: [10.1109/TPWRD.2020.3018647](https://doi.org/10.1109/TPWRD.2020.3018647). PNNL report no. PNNL-SA-149830. Available via OSTI: <https://www.osti.gov/biblio/1909842>. Available via PNNL: <https://www.pnnl.gov/publications/modeling-grid-forming-and-grid-following-inverters-dynamic-simulation-large-scale>.

[2] R. H. Lasseter, J. H. Eto, B. Schenkman, J. Stevens, H. Volkommer, D. Klapp, E. Linton, H. Hurtado, and J. Roy, "CERTS Microgrid Laboratory Test Bed," *IEEE Transactions on Power Delivery*, vol. 26, no. 1, pp. 325–332, Jan. 2011. LBNL report no. LBNL-3553E. Available via IEEE Xplore: <https://ieeexplore.ieee.org/document/5673682>. Available via OSTI: <https://www.osti.gov/biblio/983805>.

## Summary of Key Parameters

Table: Key Parameters and Variables of the GridLAB-D™ Dynamic Inverter (`inverter_dyn`) Object { #tbl:inverter-dyn-parameters }

|Published Name|Unit|Type|Description|
|---|---|---|---|
|**control_mode**||enumeration|Inverter control mode: grid-forming or grid-following [GRID_FORMING, GRID_FOLLOWING, GFL_CURRENT_SOURCE]|
|**grid_following_mode**||enumeration|grid-following mode, positive sequency or balanced three phase power [BALANCED_POWER, POSITIVE_SEQUENCE]|
|**grid_forming_mode**||enumeration|grid-forming mode, CONSTANT_DC_BUS or DYNAMIC_DC_BUS [CONSTANT_DC_BUS, DYNAMIC_DC_BUS]|
|**P_f_droop_setting_mode**||enumeration|Definition of P-f droop curve [FSET_MODE, PSET_MODE]|
|**phaseA_I_Out**|A|complex|AC current on A phase in three-phase system|
|**phaseB_I_Out**|A|complex|AC current on B phase in three-phase system|
|**phaseC_I_Out**|A|complex|AC current on C phase in three-phase system|
|**phaseA_I_Out_PU**|pu|complex|AC current on A phase in three-phase system, pu|
|**phaseB_I_Out_PU**|pu|complex|AC current on B phase in three-phase system, pu|
|**phaseC_I_Out_PU**|pu|complex|AC current on C phase in three-phase system, pu|
|**IA_Out_PU_temp**|pu|complex| Phase A current for current limiting calculation of a grid-forming inverter, pu|
|**IB_Out_PU_temp**|pu|complex| Phase B current for current limiting calculation of a grid-forming inverter, pu|
|**IC_Out_PU_temp**|pu|complex| Phase C current for current limiting calculation of a grid-forming inverter, pu|
|**power_A**|VA|complex|AC power on A phase in three-phase system|
|**power_B**|VA|complex|AC power on B phase in three-phase system|
|**power_C**|VA|complex|AC power on C phase in three-phase system|
|**VA_Out**|VA|complex|AC power|
|**rated_power**|VA|double| The rated power of the inverter|
|**rated_DC_Voltage**|V|double| The rated dc bus of the inverter|
|**Xfilter**|pu|double|DELTAMODE:  per-unit values of inverter filter.|
|**Rfilter**|pu|double|DELTAMODE:  per-unit values of inverter filter.|
|**pdispatch**|pu|double|Desired generator dispatch set point in p.u.|
|**pdispatch_offset**|pu|double|Desired offset to generator dispatch in p.u.|
|**Pref**|W|double|DELTAMODE: The real power reference.|
|**Qref**|VAr|double|DELTAMODE: The reactive power reference.|
|**kpc**||double|DELTAMODE: Proportional gain of the current loop.|
|**kic**||double|DELTAMODE: Integral gain of the current loop.|
|**F_current**||double|DELTAMODE: feed forward term gain in current loop.|
|**Tif**||double|DELTAMODE: time constant of first-order low-pass filter of current loop when using current source representation.|
|**frequency_watt**||bool|DELTAMODE: Boolean used to indicate whether inverter f/p droop is included or not|
|**checkRampRate_real**||bool|DELTAMODE: Boolean used to indicate whether check the ramp rate|
|**volt_var**||bool|DELTAMODE: Boolean used to indicate whether inverter volt-var droop is included or not|
|**checkRampRate_reactive**||bool|DELTAMODE: Boolean used to indicate whether check the ramp rate|
|**Tpf**|s|double|DELTAMODE: the time constant of power measurement low pass filter in frequency-watt.|
|**Tff**|s|double|DELTAMODE: the time constant of frequency measurement low pass filter in frequency-watt.|
|**Tqf**|s|double|DELTAMODE: the time constant of low pass filter in volt-var.|
|**Tvf**|s|double|DELTAMODE: the time constant of low pass filter in volt-var.|
|**Pref_max**|pu|double|DELTAMODE: the upper and lower limits of power references in grid-following mode.|
|**Pref_min**|pu|double|DELTAMODE: the upper and lower limits of power references in grid-following mode.|
|**Qref_max**|pu|double|DELTAMODE: the upper and lower limits of reactive power references in grid-following mode.|
|**Qref_min**|pu|double|DELTAMODE: the upper and lower limits of reactive power references in grid-following mode.|
|**Rp**|pu|double|DELTAMODE: p-f droop gain in frequency-watt.|
|**frequency_watt_droop**|pu|double|DELTAMODE: p-f droop gain in frequency-watt.|
|**db_UF**|Hz|double|DELTAMODE: upper dead band for frequency-watt control, UF for under-frequency|
|**db_OF**|Hz|double|DELTAMODE: lower dead band for frequency-watt control, OF for over-frequency|
|**Rq**|pu|double|DELTAMODE: Q-V droop gain in volt-var.|
|**volt_var_droop**|pu|double|DELTAMODE: Q-V droop gain in volt-var.|
|**db_UV**|pu|double|DELTAMODE: dead band for volt-var control, UV for under-voltage|
|**db_OV**|pu|double|DELTAMODE: dead band for volt-var control, OV for over-voltage|
|**rampUpRate_real**||double|DELTAMODE: ramp rate for grid-following frequency-watt|
|**rampDownRate_real**||double|DELTAMODE: ramp rate for grid-following frequency-watt|
|**rampUpRate_reactive**||double|DELTAMODE: ramp rate for grid-following volt-var|
|**rampDownRate_reactive**||double|DELTAMODE: ramp rate for grid-following volt-var|
|**frequency_convergence_criterion**|rad/s|double|Max frequency update for grid-forming inverters to return to QSTS|
|**voltage_convergence_criterion**|V|double|Max voltage update for grid-forming inverters to return to QSTS|
|**current_convergence_criterion**|A|double|Max current magnitude update for grid-following inverters to return to QSTS, or initialize|
|**kpPLL**||double|DELTAMODE: Proportional gain of the PLL.|
|**kiPLL**||double|DELTAMODE: Integral gain of the PLL.|
|**Tp**||double|DELTAMODE: time constant of low pass filter, P calculation.|
|**Tq**||double|DELTAMODE: time constant of low pass filter, Q calculation.|
|**Tv**||double|DELTAMODE: time constant of low pass filter, V calculation.|
|**Vset**|pu|double|DELTAMODE: voltage set point in grid-forming inverter, usually 1 pu.|
|**kpv**||double|DELTAMODE: proportional gain of voltage loop.|
|**kiv**||double|DELTAMODE: integral gain of voltage loop.|
|**mq**|pu|double|DELTAMODE: Q-V droop gain, usually 0.05 pu.|
|**Q_V_droop**|pu|double|DELTAMODE: Q-V droop gain, usually 0.05 pu.|
|**E_max**||double|DELTAMODE: E_max and E_min are the maximum and minimum of the output of voltage controller.|
|**E_min**||double|DELTAMODE: E_max and E_min are the maximum and minimum of the output of voltage controller.|
|**Emax**||double|DELTAMODE: E_max and E_min are the maximum and minimum of the output of voltage controller.|
|**Emin**||double|DELTAMODE: E_max and E_min are the maximum and minimum of the output of voltage controller.|
|**Pset**|pu|double|DELTAMODE: power set point in P-f droop.|
|**fset**|Hz|double|DELTAMODE: frequency set point in P-f droop.|
|**mp**|rad/s/pu|double|DELTAMODE: P-f droop gain, usually 3.77 rad/s/pu.|
|**P_f_droop**|pu|double|DELTAMODE: P-f droop gain in per unit value, usually 0.01.|
|**kppmax**||double|DELTAMODE: proportional gain for Pmax controller.|
|**kipmax**||double|DELTAMODE: integral gain for Pmax controller.|
|**w_lim**||double|DELTAMODE: saturation limit of Pmax controller.|
|**Pmax**|pu|double|DELTAMODE: maximum limit and minimum limit of Pmax controller and Pmin controller.|
|**Pmin**|pu|double|DELTAMODE: maximum limit and minimum limit of Pmax controller and Pmin controller.|
|**w_ref**|rad/s|double|DELTAMODE: the rated frequency, usually 376.99 rad/s.|
|**freq**|Hz|double|DELTAMODE: the frequency obtained from the P-f droop controller.|
|**Imax**|pu|double|DELTAMODE: the maximum current of a grid-forming inverter.|
|**kpqmax**||double|DELTAMODE: proportional gain for Qmax controller.|
|**kiqmax**||double|DELTAMODE: integral gain for Qmax controller.|
|**Qmax**|pu|double|DELTAMODE: maximum limit and minimum limit of Qmax controller and Qmin controller.|
|**Qmin**|pu|double|DELTAMODE: maximum limit and minimum limit of Qmax controller and Qmin controller.|
|**VFlag**||bool|DELTAMODE: Voltage flag to choose between PI control or direct control.|
|**V_In**|V|double|DC input voltage|
|**I_In**|A|double|DC input current|
|**P_In**|W|double|DC input power|
|**enable_1547_checks**||bool|DELTAMODE: Enable IEEE 1547-2003 disconnect checking|
|**reconnect_time**|s|double|DELTAMODE: Time delay after IEEE 1547-2003 violation clears before resuming generation|
|**inverter_1547_status**||bool|DELTAMODE: Indicator if the inverter is curtailed due to a 1547 violation or not|
|**IEEE_1547_version**||enumeration|DELTAMODE: Version of IEEE 1547 to use to populate defaults [NONE, IEEE1547_2003, IEEE1547A_2014, IEEE1547_2018]|
|**over_freq_high_cutout**|Hz|double|DELTAMODE: OF2 set point for IEEE 1547a|
|**over_freq_high_disconnect_time**|s|double|DELTAMODE: OF2 clearing time for IEEE1547a|
|**over_freq_low_cutout**|Hz|double|DELTAMODE: OF1 set point for IEEE 1547a|
|**over_freq_low_disconnect_time**|s|double|DELTAMODE: OF1 clearing time for IEEE 1547a|
|**under_freq_high_cutout**|Hz|double|DELTAMODE: UF2 set point for IEEE 1547a|
|**under_freq_high_disconnect_time**|s|double|DELTAMODE: UF2 clearing time for IEEE1547a|
|**under_freq_low_cutout**|Hz|double|DELTAMODE: UF1 set point for IEEE 1547a|
|**under_freq_low_disconnect_time**|s|double|DELTAMODE: UF1 clearing time for IEEE 1547a|
|**under_voltage_low_cutout**|pu|double|Lowest voltage threshold for undervoltage|
|**under_voltage_middle_cutout**|pu|double|Middle-lowest voltage threshold for undervoltage|
|**under_voltage_high_cutout**|pu|double|High value of low voltage threshold for undervoltage|
|**over_voltage_low_cutout**|pu|double|Lowest voltage value for overvoltage|
|**over_voltage_high_cutout**|pu|double|High voltage value for overvoltage|
|**under_voltage_low_disconnect_time**|s|double|Lowest voltage clearing time for undervoltage|
|**under_voltage_middle_disconnect_time**|s|double|Middle-lowest voltage clearing time for undervoltage|
|**under_voltage_high_disconnect_time**|s|double|Highest voltage clearing time for undervoltage|
|**over_voltage_low_disconnect_time**|s|double|Lowest voltage clearing time for overvoltage|
|**over_voltage_high_disconnect_time**|s|double|Highest voltage clearing time for overvoltage|
|**IEEE_1547_trip_method**||enumeration|DELTAMODE: Reason for IEEE 1547 disconnect - which threshold was hit [NONE, OVER_FREQUENCY_HIGH, OVER_FREQUENCY_LOW, UNDER_FREQUENCY_HIGH, UNDER_FREQUENCY_LOW, UNDER_VOLTAGE_LOW, UNDER_VOLTAGE_MID, UNDER_VOLTAGE_HIGH, OVER_VOLTAGE_LOW, OVER_VOLTAGE_HIGH]|
|**phase_angle_correction**||bool|DELTAMODE: Boolean used to indicate whether inverter applies phase angle correction during current limiting|
|**virtual_resistance_correction**||bool|DELTAMODE: Boolean used to indicate whether inverter applies virtual resistance correction during current limiting|