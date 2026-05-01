# Notes and questions

!!! note "Before this goes completely out...."

    This file and its contents are for keeping ideas saved.
    
    It should be deleted before fully releasing teh documentation.

- Describe the generic parameters of the inverter object
- Describe some control options
- New inverter_dyn object and why introduced. does this work as both grid forming and following?????


Why GridLAB-D Specifically Needs an Inverter Model
1. Time-varying resource integration
GridLAB-D is designed to simulate PV and storage over time with weather-driven inputs. The inverter model is what translates a DC power source (solar irradiance → DC watts) into an AC grid injection with realistic constraints — efficiency curves, power factor settings, and clipping at rated kVA.
2. AC-DC interface representation
GridLAB-D explicitly models the DC side (panels, battery state of charge) and the AC side (grid connection) as separate objects. The inverter sits between them and handles:

DC-to-AC conversion efficiency
Maximum power point tracking (MPPT) behavior
AC output limits

Without this, there's no physically meaningful way to connect a DC resource to the AC network in the simulation.
3. Four-quadrant operation and reactive power dispatch
GridLAB-D's inverter model supports operation in all four quadrants — importing/exporting both P and Q. This is necessary for modeling:

Smart inverter functions per IEEE 1547-2018
Volt-VAR optimization schemes
Demand response involving storage discharge

4. Unbalanced three-phase network
Like OpenDSS, GridLAB-D solves unbalanced distribution networks. A single-phase rooftop PV inverter connected to phase B needs to inject current correctly into that phase only — a balanced bus approximation would corrupt the three-phase solution.
5. Control mode switching over time
Because GridLAB-D runs time-series, the inverter model needs to handle mode transitions — e.g., switching from constant power factor to Volt-VAR mode at a scheduled time, or curtailing output when SOC limits are hit. A static PQ injection can't represent this.
6. Islanding and microgrid scenarios
GridLAB-D is often used for microgrid and resilience studies. In islanded mode, at least one inverter must act as a grid-forming source (voltage and frequency reference). The inverter model captures this mode switch, which completely changes its role in the power flow solution.

These represent the specifications and some examples for the **inverter_dyn** object, which is to represent grid-forming and grid-following inverters. The **inverter_dyn** is a reworked inverter object. The **inverter_dyn** object was created to focus on dynamic responses of the inverter, especially in a microgrid or islanded system scenario. The existing inverter object has significant extra modes and some incompatible code. In order to maintain backwards compatibility with that functionality, the inverter was left, as-is, and the **inverter_dyn** was created to handle the new dynamics-oriented development.

=============================

No — none of the four_quadrant_control_mode options require transient calculations when operating in QSS. But the answer has some important nuance worth unpacking.

All QSS Modes Are Algebraic at Each Timestep
In QSS, every control mode computes its P and Q injection through a purely algebraic calculation at each timestep — no differential equations, no time integration, no memory of how the state evolved between the current and previous timestep in a dynamic sense. The modes are:
ModeWhat it computes each stepCONSTANT_PQFixed P and Q — trivially algebraicCONSTANT_PFQ = f(P, power_factor) — algebraicVOLT_VARQ = f(V_terminal) via piecewise linear curve — algebraicVOLT_WATTP = f(V_terminal) via linear ramp — algebraicVOLT_VAR_FREQ_PWRP and Q = f(V, freq) — algebraic with slew limitingLOAD_FOLLOWINGP_dispatch = f(sensed_power, SOC, thresholds) — algebraicGROUP_LOAD_FOLLOWINGSame as above, aggregated across group — algebraicVOLTAGE_SOURCEOnly meaningful in deltamode — see below

The Nuance: Quasi-Transient Behavior in Some Modes
A few modes carry state from one timestep to the next, which gives them a pseudo-transient character — but this is fundamentally different from true transient simulation.
VOLT_VAR — lockout timer
The volt_var_control_lockout parameter prevents the inverter from changing its Q injection more frequently than a specified interval. The inverter checks elapsed simulation time against the last dispatch time. This is a discrete timer, not a differential equation — there is no time integration happening.
VOLT_VAR_FREQ_PWR — slew rate limiting and delay
The max_var_slew_rate, max_pwr_slew_rate, and delay_time parameters cause the output to ramp between timesteps rather than jump. The inverter computes the maximum allowable change per timestep (slew_rate × Δt) and clamps the step accordingly. This looks like a transient but it is still just arithmetic on the previous timestep's output value — there is no ODE being integrated.
LOAD_FOLLOWING — lockout timers
charge_lockout_time and discharge_lockout_time carry a timer state across timesteps, but again this is a discrete gate, not a dynamic equation.

VOLTAGE_SOURCE Is the Exception — But Only in Deltamode
VOLTAGE_SOURCE / VSI mode is the one mode that genuinely requires transient calculations — PI/PID current controllers, filter dynamics, droop response — but these only activate when the object enters deltamode. In a pure QSS simulation with no deltamode enabled, VOLTAGE_SOURCE either does nothing meaningful or falls back to a simplified injection. It was designed from the ground up as a deltamode construct.

Why This Matters Practically
The distinction is important for two reasons:
Timestep sensitivity — Because QSS modes are algebraic, their results do not depend on how small your timestep is (within reason). A VOLT_VAR inverter will reach the same steady-state Q for a given terminal voltage whether your timestep is 1 second or 15 minutes. The slew-rate-limited modes in VOLT_VAR_FREQ_PWR are the exception — their ramp behavior is timestep-dependent, so very coarse timesteps will skip over intermediate ramp states.
What QSS cannot capture — None of the QSS modes can represent the sub-cycle dynamics of the inverter's actual switching behavior, PLL response, current controller settling, or fault current contribution. If those phenomena matter for your study, you need deltamode with VOLTAGE_SOURCE or coupling to an EMT tool. For hosting capacity, Volt-VAR optimization, or long time-series DER integration studies, the QSS algebraic modes are both sufficient and appropriate.