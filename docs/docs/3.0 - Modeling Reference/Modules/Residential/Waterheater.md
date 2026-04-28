## waterheater

Typical residential water heaters range in size from about 30 gallons up to 120 gallons or so, with most tanks falling in the range of about 40 to 80 gallons. Hot water is drawn from the top of the tank to service the home's water loads, with cold make-up water injected near the bottom of the tank. When a hot water draw is in progress, the tank will tend to have a layer of cold water at the bottom.

GridLAB-D™ supports three heating modes (`heat_mode`): electric resistance (`ELECTRIC`), natural gas (`GASHEAT`), and heat pump (`HEAT_PUMP`). The choice of heating mode determines both which physical model is used and which published properties are relevant during a simulation.

Water is typically heated by two elements, one located near the bottom of the tank and one located higher in the tank, usually halfway to two-thirds of the way to the top. The lower element shoulders most of the heating load, with the upper element engaging only when the cold water layer has reached it. The two elements are controlled by independent thermostats, but only one is permitted to be on at a time, the upper element having priority. The design is intended to allow rapid (re)heating of the smaller volume of water above the top element so that full-temperature water is available as soon as possible after a depletion. Water heater elements range in capacity from about 1500 Watts to 6000 Watts, with 4500 Watts being common.

Thermostatic controls have a "dead band" associated with the setpoint, which prevents rapid cycling of power to the elements which would result if the turn-on temperature equaled the turn-off temperature. The dead band is typically a few degrees above and below the nominal setpoint.

Most heaters are shipped with both the upper and lower thermostats set to the same temperature (often 120 degrees-F), but they may be modified at installation or a later time. Some manufacturers recommend that if hotter water is desired, only the lower element be set to a higher temperature. This will maintain the entire tank at the higher temperature but allow for rapid recovery of the upper volume to the lower setting following a depletion. The net effect on energy use is negligible if the upper thermostat is set to a lower setting. However, some homeowners wrongly set the upper element to a higher temperature than the lower one, which results in somewhat different behavior. Because of thermal stratification, the upper volume will be maintained at a higher temperature than the lower volume, resulting in somewhat lower standby losses than when the entire tank is heated to the higher setting.

The waterheater uses one of four simulation models (`waterheater_model`): a one-node lumped-parameter model (ONENODE), a two-node stratified model (TWONODE), a physics-based Fortran model (FORTRAN), or a multi-layer discretized model (MULTILAYER). For the standard electric and gas modes the model is selected automatically at runtime based on the current tank state; FORTRAN and MULTILAYER must be selected explicitly by the user.

### Default Waterheater

An empty waterheater object, along the lines of


    object waterheater { }


will be constructed into a semi-consistent state. The "default waterheater" ends up being similar to


    object waterheater {
       tank_volume 50.0 gal;
       tank_diameter 1.5 ft;
       tank_height 3.782 ft;
       inlet_water_temperature 60.0 degF;
       location GARAGE;                  // 80% probability; 20% INSIDE
       heat_mode ELECTRIC;
       waterheater_model NONE;           // auto-selects ONENODE or TWONODE at runtime
       tank_setpoint random.normal(125,5);                  // bound [90, 160]
       thermostat_deadband (1 + fabs(random.normal(2,1)));  // bound [1, 10]
       tank_UA random.normal(2.0, 0.2) * tank_volume/50;    // bound [0.1, 10]
       heating_element_capacity 4.5 kW;  // tanks >= 50 gal; smaller tanks may be 3.2 or 3.5 kW
       re_override OV_NORMAL;
    }


Any properties that are not set explicitly will carry these default values.

### Waterheater Properties

#### Core Tank Properties

Property Name | Type | Unit | Description | Default or Valid Range
---|---|---|---|---
**tank_volume** | double | gallons | The water volume of the water tank. | Valid range: 20–100 gallons. Defaults to 50 if unspecified.
**tank_UA** | double | BTU·h/degF | The product of the tank insulation U-value and surface area. | Scaled proportionally to tank volume (reference: 50 gal).
**tank_diameter** | double | ft | The diameter of the water heater tank. Used with `tank_height` to infer missing geometry.
**tank_height** | double | ft | The physical height of the water heater tank. | Defaults to 3.782 ft if unspecified.
**water_demand** | double | gpm | Hot water consumption rate. Constant unless driven by a Player or loadshape object.
**heating_element_capacity** | double | kW | The rate at which the heating element (or compressor, for HEAT_PUMP) adds thermal energy to the water.
**inlet_water_temperature** | double | degF | The temperature of cold make-up water entering the bottom of the tank. | 60 degF.
**tank_setpoint** | double | degF | The target temperature around which the thermostat cycles. | Valid range: 90–160 degF.
**thermostat_deadband** | double | degF | Total deadband width around the setpoint (half above, half below). | Valid range: 0–10 degF.

#### Operating Mode and Model Selection

Property Name | Type | Unit | Description
---|---|---|---
**heat_mode** | enumeration | — | Energy source for heating. One of `ELECTRIC`, `GASHEAT`, or `HEAT_PUMP`.
**location** | enumeration | — | `INSIDE` or `GARAGE`. Determines whether heat losses warm the conditioned space and whether outdoor temperature influences the effective ambient temperature.
**waterheater_model** | enumeration | — | Simulation model to use. One of `NONE` (auto-select ONENODE/TWONODE), `ONENODE`, `TWONODE`, `FORTRAN`, or `MULTILAYER`.
**re_override** | enumeration | — | Demand response override. `OV_NORMAL` (thermostat-controlled), `OV_ON` (forced on), or `OV_OFF` (forced off).

#### Status and Output Properties

Property Name | Type | Unit | Description
---|---|---|---
**temperature** | double | degF | Current temperature of the hot water in the tank (whole-tank for ONENODE; upper zone for TWONODE).
**height** | double | ft | Height of the hot/cold boundary within the tank (TWONODE model). Equals tank height when the tank is FULL.
**current_tank_status** | enumeration | — | Read-only tank fill state: `FULL`, `PARTIAL`, or `EMPTY`.
**load_state** | enumeration | — | Read-only heat-flow state: `DEPLETING`, `RECOVERING`, or `STABLE`.
**is_waterheater_on** | double | — | Simple on/off indicator: 1 = heating element active, 0 = off.
**demand** | complex | kVA | Total apparent power consumption of the water heater.
**actual_load** | double | kW | Real power draw calculated from the current voltage across the coils.
**previous_load** | double | kW | Real power draw at the previous sync operation.
**actual_power** | complex | kVA | Apparent power calculated from the current voltage across the coils.

#### Gas Waterheater Properties

Property Name | Type | Unit | Description
---|---|---|---
**gas_fan_power** | double | kW | Power draw of the combustion air/vent fan while the burner is running. Defaults to 1% of `heating_element_capacity`.
**gas_standby_power** | double | kW | Standby power draw (e.g., electronic igniter logic) when the burner is off. Defaults to 0 W.

#### Heat Pump Waterheater Properties

Property Name | Type | Unit | Description
---|---|---|---
**heat_pump_coefficient_of_performance** | double | Btu/kWh | Current COP of the heat pump. Calculated internally; not a user input.
**Tcontrol** | double | degF | Blended control temperature used by the HEAT_PUMP thermostat logic. Calculated internally; not a user input.
**heatpump_predicted_energy** | double | Wh | (Hidden) Internal energy-deficit tracker (`energytake`). Increases as the tank cools; decreases as the compressor or element heats.

#### FORTRAN Model Properties

These properties are only relevant when `waterheater_model FORTRAN`. Valid tank sizes are 40 and 80 gallons only.

Property Name | Type | Unit | Description
---|---|---|---
**dr_signal** | double | — | On/off demand-response signal sent to the Fortran model. Default: 1 (on).
**operating_mode** | double | — | Operating mode for the Fortran model. Mode 3 selects electric-resistance-only for the 80-gallon tank; other modes select heat-pump operation.
**fortran_sim_time** | double | s | Time interval (in seconds) the Fortran model should simulate per synchronization step. Must be > 0.
**waterheater_power** | double | kW | Current power draw reported by the Fortran model.
**COP** | double | — | Current COP reported by the Fortran heat-pump model.

#### MULTILAYER Model Properties

These properties are only relevant when `waterheater_model MULTILAYER`.

Property Name | Type | Unit | Description | Default
---|---|---|---|---
**lower_tank_setpoint** | double | degF | Setpoint for the lower heating element thermostat. | `tank_setpoint`.
**upper_tank_setpoint** | double | degF | Setpoint for the upper heating element thermostat. | `tank_setpoint`.
**lower_tank_deadband** | double | degF | Deadband for the lower heating element thermostat. | `thermostat_deadband`.
**upper_tank_deadband** | double | degF | Deadband for the upper heating element thermostat. | `thermostat_deadband`.
**lower_tank_temperature** | double | degF | Read-only temperature at the lower element sensor location (layer 1).
**upper_tank_temperature** | double | degF | Read-only temperature at the upper element sensor location (layer 10).
**lower_heating_element_state** | enumeration | — | Read-only state of the lower heating element: `ON` or `OFF`.
**upper_heating_element_state** | enumeration | — | Read-only state of the upper heating element: `ON` or `OFF`.
**discrete_step_size** | double | s | Time step for the internal discrete dynamics loop. Valid range: 1–60 s. | 1 s.
**circular_flow_rate** | double | gpm | Heuristic recirculation flow activated when a heating element turns on. Valid range: 1–3 gpm. | 2 gpm.
**T_mixing_valve** | double | degF | Reference temperature at which the mixing valve operates. Should be set at or below `upper_tank_setpoint`. | `Tmin_upper`

### Operating Modes

#### ELECTRIC:  Electric Resistance


    heat_mode ELECTRIC;


The default heating mode. One or two resistive heating elements immersed in the tank heat the water directly. The thermostat compares `temperature` to `tank_setpoint` ± `thermostat_deadband`/2 and turns the element on or off accordingly. `heating_element_capacity` defaults to 4.5 kW for tanks ≥ 50 gallons and is randomly chosen from 3.2, 3.5, or 4.5 kW for smaller tanks if not specified.

Heat losses from the element dissipate into the location of the tank (`INSIDE` or `GARAGE`). `gas_fan_power` and `gas_standby_power` are not used in this mode; they are automatically set to non-negative values but have no physical effect on the electric model.

#### GASHEAT: Natural Gas

    heat_mode GASHEAT;


A natural gas burner heats the water. The thermodynamic tank model (ONENODE/TWONODE) is the same as for ELECTRIC; only the power source differs. Two additional properties capture the auxiliary electrical loads:

- **`gas_fan_power`**: Power consumed by the combustion air fan or vent fan while the burner is firing. Defaults to 1% of `heating_element_capacity` if not set.
- **`gas_standby_power`**: Standby electrical load (e.g., electronic pilot or thermostat logic) when the burner is off. Defaults to 0 W if not set.

The `heating_element_capacity` property represents the gas burner's thermal output in kW, and it obeys the same defaulting rules as for ELECTRIC mode.

#### HEAT_PUMP: Heat Pump Water Heater


    heat_mode HEAT_PUMP;
    waterheater_model NONE;   // uses built-in empirical HPWH model (not FORTRAN)


A refrigerant-cycle heat pump extracts heat from the surrounding air and deposits it into the tank water, achieving a COP greater than 1.0. A backup electric resistance element activates when the heat pump cannot keep up with demand.

**Empirical energy-tracking model (default HPWH):**

GridLAB-D™'s native HEAT_PUMP implementation uses an empirical model based on an internal energy-deficit tracker called `energytake` (published as `heatpump_predicted_energy`). `energytake` represents the amount of energy the heat pump would need to consume to bring the tank back to setpoint temperature.

- **Idle mode** (`Ton ≤ Tw ≤ Toff`, element capacity = 0): The heat pump is off. `energytake` increases logarithmically with time to reflect standby heat losses. Tank temperature is back-calculated from `energytake`.
- **Active mode** (`Tw < Ton` or element capacity > 0): The compressor or electric element is running. `energytake` decreases as heat is added.
  - If `energytake > compressor_max_threshold` (default 2000 Wh), the electric resistance backup element activates.
  - If `energytake < heating_element_min_threshold` (default 1100 Wh), the backup element deactivates.
  - The compressor activates when `energytake > compressor_min_threshold` (default ≈ `thermostat_deadband`/2 × 2.44 × `tank_volume`).

The thermostat in HEAT_PUMP mode uses a blended `Tcontrol` temperature rather than `Tw` directly when the tank is in a `PARTIAL` state.

COP is calculated internally and published via `heat_pump_coefficient_of_performance`. It is not a user input.

**FORTRAN-based HPWH model:**

For a higher-fidelity heat pump simulation, set `waterheater_model FORTRAN`. See the [FORTRAN Model](#fortran-model) section below.

### Waterheater Models

The `waterheater_model` property selects the simulation model. For `ELECTRIC` and `GASHEAT` modes the default (`NONE`) automatically transitions between ONENODE and TWONODE based on the current tank state. FORTRAN and MULTILAYER must be selected explicitly.

#### ONENODE and TWONODE — Standard Model

The standard model (`waterheater_model NONE`, `ONENODE`, or `TWONODE`) is the default for ELECTRIC and GASHEAT modes and is also used by the native HEAT_PUMP mode.

### Modeling Assumptions

The GridLAB-D™ approach to modeling electric water heaters is designed to be computationally fast yet reasonably accurate. It accommodates the common two-element design and the possibility for "inverted" thermostat settings, wherein the upper element maintains a higher temperature than the lower element.

To achieve the necessary computational speed, the standard model makes the following assumptions:

  1. Thermal stratification in the tank is not directly modeled. Depending on the situation, the water will be considered to be either of uniform temperature throughout the tank or "lumped" into two temperature regions (hot and cold layers).
  2. The injection of cold inlet water at the bottom of the tank results in either complete mixing with the hot water in the tank or no mixing at all, depending on the volumetric flow rate.

The water heater simulation uses two different models depending on the state of the tank at any given moment:

  1. **One-Node Model** – A simple, lumped-parameter electric analogue model that considers the entire tank to be a single "slug" of water at a uniform temperature. This model concerns the temperature of the water at any given time and/or the time required for the temperature to move between two specified points.
  2. **Two-Node Model** – Applies when the heater is in a state of partial depletion. Considers the heater to consist of two slugs of water, each at a uniform temperature. The upper "hot" node is near the heater's setpoint temperature, while the lower "cold" node is near the inlet water temperature. This model concerns the location of the boundary between the hot and cold nodes, calculating the movement of that boundary as hot water is drawn from the tank and/or heat is added to the tank.

The water heater simulation keys on two primary "states" of the water heater:

  * **Tank State** – The tank can be in one of three states (read via `current_tank_status`):
    * **FULL** – All the water in the tank is at a uniform temperature near the heater's setpoint. The One-Node model applies.
    * **PARTIAL** – The tank is in a state of partial depletion, where some of the hot water has been (or is being) drawn out, leaving hot and cold layers of water in the tank. The Two-Node model applies.
    * **EMPTY** – The tank has been completely depleted; all the water is at a uniform temperature near the water inlet temperature. The One-Node model applies.
  * **Load State** – This refers to the current water load on the heater; that is, whether and how fast hot water is being drawn from the top of the tank (read via `load_state`). Formally, this load state applies only when the tank state is **PARTIAL**, but is useful for the **FULL** and **EMPTY** tank states because it tells whether the tank will begin to move toward a **PARTIAL** state or stay in the current state. There are three possible load states:
    * **DEPLETING** – Hot water is being drawn at a rate sufficient to move the boundary between the hot and cold zones upward. That is, hot water is being drawn out faster than the heating element can warm the incoming cold water, so the upper layer of hot water is getting smaller and the lower layer of cold water is getting larger.
    * **RECOVERING** – Hot water is either not being drawn from the tank or is being drawn at a low enough level that the boundary between the hot and cold layers is moving downward. That is, the hot water is being drawn out at a rate low enough that the heating element can warm the replacement water faster than it is being introduced, causing the upper layer of hot water to get larger and the lower layer of cold water to get smaller.
    * **STABLE** – Simplistically, this state implies that hot water is being drawn at a rate that matches the heating element's ability to warm the incoming cold water. Actually, the hot water draw does not have to exactly match the warming rate for this state to apply. Because there are other heat flows acting on the water (e.g., jacket losses to surrounding air), in any given situation where the tank state is **PARTIAL** there is a range of hot water flow rates for which the tank will never reach either the **FULL** or the **EMPTY** state.

For example, if the tank begins near the **FULL** state and hot water is drawn from it at a rate slightly higher than the heating element can match, the hot/cold boundary will begin to move upward. However, as the boundary moves up, the size of the hot layer gets smaller, reducing the area of warm tank that is exposed to the surrounding air. The lowered area means lowered heat losses, which may tip the balance so that the heating element eventually can keep up with the hot water draw. Similarly, starting with an almost empty tank, the heating element's capacity may exceed a small hot water draw and move the hot/cold boundary downward until jacket losses tip the balance. When the load state is **STABLE**, the calculated time to transition is infinite.

This **STABLE** state is illustrated in Figure 1. For an example water heater, the figure shows the time required to deplete an initially full tank ("time to transition") at various hot water flow rates. Note that flow rates above roughly 0.45 gpm result in positive and finite times to transition. Flow rates below about 0.44 gpm are finite and negative, meaning the tank is not depleting at all, but is actually recovering (i.e., the heating element can more than keep up with the heat removed by the water draw). In between those two points—between the positive and negative spikes to infinity—the tank is in the **STABLE** state that will never reach either the **FULL** or the **EMPTY** state.

These two critical flow rates depend on many factors, including the tank size and shape, tank $UA$, heating element capacity, and the hot and cold layer temperatures. They are identified in the GridLAB-D™ water heater model by calculating the rate of change of the hot/cold boundary position $h$ with respect to time $(dh/dt)$. Note in the lower graphic of Figure 1 that the time to transition is positive when $dh/dt$, calculated at the **FULL** starting point $h0$, is negative (meaning the tank is depleting). Also note that the time to transition is negative (meaning the tank is really recovering and will remain **FULL**) when $dh/dt$, calculated at the target **EMPTY** state, is positive. The area between the two critical flow rates is identified by differing signs between $dh/dt$ calculated at **FULL** and $dh/dt$ calculated at **EMPTY**.

![Illustration of using dh/dt to identify the **STABLE** state](../../../../images/300px-Residential_Module_Guide_Figure_1.png)

##### Figure 1. Illustration of using dh/dt to identify the **STABLE** state

### Modeling Approach

Figure 2 shows a schematic representation of the water heater model in which Tavg is the average water temperature throughout the tank and Tamb is the ambient temperature. The thermal capacitance of the water Cw is a function of the tank volume:

$$C_w = V(gal) \frac{1(ft^3)}{7.48(gal)} \frac{62.4(lb_m)}{1(ft^3)} \frac{1(Btu)}{1(lb_m \dot F)}$$

The thermal conductance of the tank shell (or "jacket") $UA$ is calculated from the known R-values of the sides and top of the tank divided into their corresponding areas.

![Water heater model schematic representation](../../../../images/300px-Residential_Module_Guide_Figure_2.png)

##### Figure 2. Water heater model schematic representation

#### One-Node Model

Considering Figure 2 and treating the water heater as a single node with thermal capacitance $C_w$, a conductance $UA$ to ambient conditions, with mass flow rate and heat input rate of $Q_{elec}$, a heat balance on the water node is as follows:

$$Q_{elec} - \dot m C_p \left ( T_w - T_{inlet} \right ) + UA \left ( T_{amb} - T_w \right ) = C_w \frac{dT_w}{dt}$$

or,

$$
dt = \frac{
  C_w
}{
  \dot{m} C_p T_{\text{inlet}}
  + U A T_{\text{amb}}
  - \left( U A + \dot{m} C_p \right) T_w
  + Q_{\text{elec}}
} \, dT_w
$$

The time required to change the tank's temperature from an initial temperature $T_0$ to a new temperature $T_1$ is given by integrating that equation.

$$
t_1 - t_0
= \int_{T_0}^{T_1}
\frac{1}{
  \dfrac{
    \dot{m} C_p T_{\text{inlet}}
    + U A T_{\text{amb}}
    + Q_{\text{elec}}
  }{C_w}
  - \dfrac{
    U A + \dot{m} C_p
  }{C_w} T_w
}\, dT_w
$$

That is an integral of the form $dx/(a+bx)$, which has solution $\log(a+bx)/b$. Therefore, the final model of the time required to raise (or lower) the tank's temperature is

$$t_1 - t_0 = \frac{1}{b} \log \left ( a + b T_w \right ) \bigg|_{T_0}^{T_1}$$

Where

  * $a = \frac{\dot m C_p T_{inlet} + U A T_{amb} + Q_{elec}}{C_w}$
  * $b = \frac{ U A + \dot m C_p }{C_w}$

The reverse problem of calculating the new temperature of the tank from a known initial temperature and time difference, $t1-t0$, follows directly:

$$T_1 = -\frac{a}{b} + \left ( \frac{a}{b} + T_0 \right ) e^{b \left( t_1 - t_0 \right ) }$$

#### Two-Node Model

This model, which applies when the heater is in a state of partial depletion, considers the heater to consist of two slugs of water, each at a uniform temperature. The upper "hot" node is near the heater's setpoint temperature, while the lower "cold" node is near the inlet water temperature. The time required to change the tank's hot water column from an initial height of $h_0$ to a final height of $h_1$ is given by the following equation:

$$t_1 - t_0 = \frac{1}{b} \log \left ( \frac{dh_w}{dt}\right ) \bigg |_{h_0}^{h_1}$$

Where $dh/dt$ is the location of temperature boundary along the height of the water column. This is calculated as a function of mass flow rate and the temperature difference between the upper and lower interface layers of the water column, as given by the following:

$$\frac{dh}{dt} = a + b h$$

where

  * $a = \frac{Q_{elec}+U A T_{amb}}{C_w T_{lower}} - \frac{\dot m C_p}{C_w}$
  * $b = \frac{U A}{C_w}$

In the two-node model, during each synchronization cycle, the height of the hot water column is calculated based on the mass flow rate, using the following equation:

$$h_1 = \frac{e^{cb \cdot \Delta t} \left ( cA + cb \cdot h_0 \right ) - cA}{cb}$$

where $cA$ and $cb$ are the numerically computed coefficients from the discretized `dhdt` expression evaluated at the upper-node temperature difference.

#### FORTRAN Model


    waterheater_model FORTRAN;


The FORTRAN model uses a high-fidelity physics-based simulation originally written in Fortran. It is restricted to **40-gallon** and **80-gallon** tanks only; other volumes will cause a simulation error.

For the 80-gallon tank, `operating_mode` selects the sub-type:

| Operating Mode | Description |
|---|---|
| 3 | Electric resistance only (dual 3900 W elements) |
| other | Heat pump with backup electric element (compressor + 4200 W upper + 2000 W lower) |

The Fortran model requires `fortran_sim_time` to be set to a positive value (in seconds) representing the simulation interval per GridLAB-D™ synchronization step. Physical parameters such as tank geometry, element positions, sensor positions, heat loss rate, and COP are determined automatically from lookup tables based on `tank_volume` and `operating_mode`.

Key FORTRAN model parameters set automatically by `init()`:

| Parameter | 40 gal | 80 gal (mode 3) | 80 gal (HP) |
|---|---|---|---|
| Tank height | 1.1 m | 1.524 m | 1.4732 m |
| Tank diameter | 0.3568 m | 0.508 m | 0.5 m |
| Upper element power | 4200 W | 3900 W | 4200 W |
| Lower element power | 2000 W | 3900 W | 2000 W |
| Compressor power | 750 W | 750 W | 750 W |
| Ambient temp. lower limit | 45°F | 45°F | 45°F |
| Ambient temp. upper limit | 109°F | 109°F | 109°F |

Use `dr_signal` to send a demand-response on/off command to the Fortran model. The `COP` and `waterheater_power` properties provide read-back of the Fortran model's current operating point.

#### MULTILAYER Model


    waterheater_model MULTILAYER;


The MULTILAYER model discretizes the tank vertically into 10 internal layers (12 total states including boundary nodes), providing a higher-resolution temperature profile than the TWONODE approach. It is computationally more expensive and numerical stability is sensitive to the choice of `discrete_step_size`.

* **Thermostat control:** Independent setpoints and deadbands are maintained for the upper (layer 10) and lower (layer 1) heating elements:

  - Upper element: `upper_tank_setpoint` ± `upper_tank_deadband`/2 — has priority; lower element cannot fire while upper element is on.
  - Lower element: `lower_tank_setpoint` ± `lower_tank_deadband`/2 — fires only when the upper element is off.

  Both default to `tank_setpoint` and `thermostat_deadband` if not explicitly set.

* **Circulation flow:** When a heating element is on, a heuristic recirculation flow (`circular_flow_rate`, default 2 gpm, range 1–3 gpm) is applied to the zone surrounding that element to model mixing in the vicinity of the heater.

* **Mixing valve:** The `T_mixing_valve` property represents a mixing valve reference temperature. Water demand is scaled by a mixing fraction such that the delivered water temperature at the tap is approximately `T_mixing_valve`. If `T_mixing_valve` is not set, it defaults to `Tmin_upper`. Setting it above `Tmax_upper` is allowed but reduces mixing efficiency.

* **State read-back:** `lower_tank_temperature` (layer 1) and `upper_tank_temperature` (layer 10) expose the temperatures at each element's sensor location. `lower_heating_element_state` and `upper_heating_element_state` report the current on/off state of each element.

* **Initialization constraints:**
  - `discrete_step_size` must be in [1, 60] seconds; values outside this range are silently clamped.
  - `circular_flow_rate` must be in [1, 3] gpm; values outside this range are silently clamped.
  - `T_mixing_valve` should be ≤ `upper_tank_setpoint` for effective mixing.

### Demand Response Override

The `re_override` property provides direct external control over the heating element, bypassing the internal thermostat:

| Value | Effect |
|---|---|
| `OV_NORMAL` | Normal thermostat-controlled operation (default). |
| `OV_ON` | Heating element forced on regardless of water temperature. In MULTILAYER mode, the upper element is forced on and the lower element is forced off. |
| `OV_OFF` | Heating element forced off regardless of water temperature. In MULTILAYER mode, both elements are forced off. |

`OV_ON` does not override the 212°F boiling protection; the element will still be shut off if the tank approaches boiling.

The FORTRAN model uses the separate `dr_signal` property instead of `re_override`.

### Simulation Sequence

Each time the water heater is sync'd, the simulation follows four steps:

  1. Calculate the energy consumed since the last iteration. The heater remembers whether it was heating and simply computes the consumption based on the time interval since the last sync.
  2. Update the tank temperature or the location of the hot/cold boundary, depending on whether the tank was previously **FULL**, **PARTIAL**, or **EMPTY**.
  3. Discern whether the tank needs heat. If the tank is in (or has reached) a **FULL** state at the thermostat setting, the power will be turned off. Otherwise the element will be turned on. Note that, for the Single-Node model, the heater state remains unchanged from its previous state when the water temperature is between the heating cut-off and cut-on temperatures (the deadband around the thermostat setpoint).
  4. Calculate and post the time to next transition. For example, if the heater is on, this is either the time for the water to reach the cut-off temperature or for the hot/cold boundary to reach the bottom of the tank, depending on the tank state.

For the **MULTILAYER** model, steps 2–4 are replaced by a discrete-time matrix simulation over a horizon of `discrete_step_size` seconds per internal step. The model recalculates the full transition horizon whenever operating conditions change (water demand, inlet temperature, setpoints, or override state).

For the **FORTRAN** model, the external Fortran subroutine is called each synchronization step with the current operating conditions, and the results (`waterheater_power`, `COP`) are read back directly.

### Complicating Factors

Several factors complicate the simulation. First, switching models between **FULL**, **PARTIAL**, and **EMPTY** states requires careful attention to not only what's happening now, but what state(s) things were in previously. Second, identifying when the load state is **STABLE** can be difficult. Finally, each of the models (One-Node and Two-Node) has two incarnations—one to calculate time to transition, and a second "inverted" one to calculate temperature/boundary location after a given amount of time. Because of limitations on floating point precision, the inverted model may not show that the water heater has actually reached the state it was anticipating. For example, the time model may show that the tank will reach the cut-off temperature of, say, 135 degrees in 12 minutes. After 12 minutes have elapsed, the core updates the water heater as expected. The temperature model may calculate that the final temperature is in fact 134.95 degrees. In a worst case the simulation could go into an infinite loop and never actually change states.


### Waterheater State of Development

Waterheater is considered a stable model, with a fair amount of functionality.