# Automatic Generation Control (AGC)/Secondary Control

## Background and Motivation

To accommodate scenarios in which multiple distributed energy resources, such as inverters, need to coordinate to provide frequency regulation and maintain scheduled power flows in microgrids or distribution systems, GridLAB-D™ implements a secondary control system — Automatic Generation Control (AGC) — in the `sec_control` object (with declarations in `sec_control.hpp` and definitions in `sec_control.cpp`).

Specifically, the controller:

1. Regulates frequency and power flow, that is it monitors frequency deviations and power errors to maintain system balance.

2. Controls multiple types of generation resources including:
   - Diesel generators (as defined by the `diesel_dg` object),
   - Dynamic inverters (as defined by the `inverter_dyn` object),
   - Tie-lines power flow between areas.

3. Implements a PID (Proportional-Integral-Derivative) controller [2] with anti-windup features to calculate power adjustments based on frequency and power errors.

4. Implements frequency dead-band logic and tie-line tolerance features to avoid unnecessary control actions during small deviations.

5. Adjusts generator setpoints, specifically for inverters specifically, by modifying their power dispatch setpoints within their operational limits to help restore system frequency to its nominal value.

## Secondary Control Functionality

The secondary controller object — `sec_control` — has the structure illustrated in [](#fig:secondary-controller-structure), which follows the textbook theoretical implementation in [](#fig:agc-control-structure-source-1). 

![GridLAB-D™ Secondary controller structure](../../../../../images/300px-SecondaryControlChart.png){ #fig:secondary-controller-structure }


![AGC Control Structure [1]](../../../../../images/300px-AGC_WW.png){ #fig:agc-control-structure-source-1 }

In a power system, the secondary control is engaged when supplementary control action is required to restore the system frequency to nominal levels. The AGC is concerned with matching generation and load, while restoring frequency to the pre-contingency value while balancing the power transfer between system areas to reach a predefined schedule. To that goal, the AGC calculates the Area Control Error (ACE) according to 

$$\displaystyle ACE = B * (f0 - f) + \sum_{i \in N_{s}} c_{i} (P_{i}^{*} - P_{i}) \tag{1}$$

in which:

- Frequency deviation $\Delta f = f_{0} - f$ is converted to power via a frequency bias $B$ [MW/Hz],
- Adjustment to the current power deviation from schedule is applied to each unit $i$ in the set $N_{s}$ of participating units (and potentially the inter-tie flows).

The AGC signal is represented by the required change in power obtained from the ACE signal through a PID controller

$$\displaystyle \Delta P = (K_{P} + K_{D}s + \frac{K_{I}}{s}) ACE \tag{2}$$

which is then distributed to each participating unit weighted by a participation factor.

Additionally, sampling blocks are added to allow for different information input and output rates, and a low pass filter could be enabled for each unit channel to smooth out the signal to individual units. 

## Inter-Tie Modeling

Inter-tie modeling is incorporated into the unit error input in equation (1) (also shown in [](#fig:inter-tie-line-ace)). The scheduled flow, $P_{i}^{*}$, and actual flow $P_{i}$, on this inter-tie are _positive_ if they correspond with the definition and _negative_ otherwise. A multiplier $c_{i}$ is introduced to account for this directionality, and is equal to 1 for all generators. For example, consider Microgrid A in [](#fig:inter-tie-line-ace):

- When $P_{i}^{*} - P_{i}$ is _positive_, either not enough power is exported ($P_{i}^{*} > 0$) or too much power is imported ($P_{i}^{*} < 0$). Either way, _more_ generation from the units in Microgrid A is needed, which is achieved by _increasing_ its error signal, $P_{\text{err}}^A$. Therefore, $P_{i}^{*} - P_{i}$, should be multiplied by $c_{i} = -1$, to negate the minus sign in the summation point.
- When $P_{i}^{*} - P_{i}$ is _negative_, either too much power is exported ($P_{i}^{*} > 0$), or too little power is imported ($P_{i}^{*} < 0$). Either way, _less_ generation from the units in Microgrid A is needed, which is achieved by _decreasing_ its error signal, $P_{\text{err}}^A$. This is similarly achieved by setting multiplier $c_{i} = -1$.

The logic for Microgrid B is equal and opposite to that of Microgrid A, and therefore $c_{i}=1$.
  
![Illustration of inter-tie incorporation into the secondary controller via the unit error, $\epsilon_{\text{unit}}$, input](../../../../../images/300px-SecondaryControlIntertie.png){ #fig:inter-tie-line-ace }

## Anti-Windup

Since the secondary controller predominantly works as an integrator and various sampling intervals are also involved, some **anti-windup** functionality is important. A few different options are considered. 

## Secondary Control Parameters and Implementation Details (`sec_control`)

Details on the variable types and units expected by GridLAB-D™ are given along with the code insider documentation in [](#tbl:sec-ctrl-parameters). The following subsections detail the key parameters that GridLAB-D™ implementation offers for modeling the `sec_control` object. This specific object has been designed and implemented to be integrated and used for transient mode analysis, but it can step out of it back into quasi-steady state (QSTS) regime upon request.

### Frequency Measurement Inputs
- `f0`: Nominal frequency (default: 60 Hz). Frequency deviation is calculated using measured frequency from parent node (mapped via `measured_frequency` property).
- `deadband`: Frequency deadband for PID input (default: 0.2 Hz = 200 mHz). As long as $\Delta f$ is within the deadband, the propagated error is 0.
- `underfrequency_limit`/`overfrequency_limit`: The _upper_/_lower_ limit $\Delta f_{\text{max}}$ on $\Delta f$ is ` f0 - underfrequency_limit`/`f0 - overfrequency_limit` (default: 57.0 Hz, and 62.0 Hz, respectively).

### Tie-line Control
- `tieline_tol`: Tie-line error tolerance in per unit (default: 0.05 = 5%) w.r.t controller set schedule in link parameter `pdispatch + pdispatch_offset`.

### PID Controller Parameters
- `kpPID`: Proportional gain (default: 0).
- `kiPID`: Integral gain.
- `kdPID`: Derivative gain (default: 0).

!!!note

    The default PID configuration is integral-only control (kp=0, kd=0).

- `anti_windup`:  Enumeration to select the anti-windup to handle the integration process:

    - `ZERO_IN_DEADBAND`: This option simply zeros the integrator state when the the frequency is within its deadband and all tie-line flows are within tolerance.

    - `FEEDBACK_PIDOUT`: This is a common feedback mechanism used on sampled outputs, and is shown in [](#fig:secondary-controller-structure). The difference of the PID output, $\text{PID}_{\text{output}}$ and the sampled output $\Delta P$ is fed back to the weighted error signal prior to the integration stage. If we consider $K_{D} = K_{P} = 0$ then there are two cases: 

        - **Sampling** : In this case $\text{PID}_{\text{output}} = \Delta P$, the feedback signal is 0 and the integration step is  

        $$x_{i}[t+1] = \text{PID}_{\text{output}}[t+1] = \text{PID}_{\text{output}}[t] + \Delta t \cdot (K_{I}\epsilon[t]) \tag{3}$$

        where $\epsilon = \sum_{i \in N_{s}} c_{i} (P_{i}^{*} - P_{i})$

        - **No-sampling** : In this case $\Delta P = 0$ and the feedback signal is $-\text{PID}_{\text{output}}$, leading the integration step to be:  

        $$x_i[t+1] = \text{PID}_{\text{output}}[t+1] = \text{PID}_{\text{output}}[t] + \Delta t \cdot (K_i\epsilon[t] - \text{PID}_{\text{output}}[t]) \tag{4}$$

!!!note

    The **no-sampling** case looks very similar to a low pass filter, which helps keep the integrator output from increasing too rapidly between sample instances. In a more general case where $K_{D} \neq K_{P} \neq 0$, the integrator output and PID output don't line up as neatly, but the concept behind the method remains the same. 

  - `FEEDBACK_INTEGRATOR`: This is the same as the `FEEDBACK_PIDOUT` method, except that the integrator output is fed back rather than the whole PID output. The relationship to a low pass filter will therefore be preserved even when $K_{D} \neq K_{P} \neq 0$, however, the feedback might not be as effective if the integrator gain is relatively small compared to the other two PID gains. 

### Sampling and Timing
- `Ts`: Main sampling period in seconds for controller output.
- `Ts_P`: Sampling period in seconds for unit/tie-line error inputs.
- `Ts_f`: Sampling period in seconds for frequency.
- `Tlp`: AGC low-pass filter time constant for participants in seconds. Default is 0, meaning no low-pas filter.

### Participant Management
- `participant_input`: Command string for creating/modifying the set of participating objects in secondary control, as detailed in [](#tbl:unit-spec-table).
- `participant_count`: Number of objects participating in secondary control.

### Unit Specific Parameters

The unit specific parameters are provided via the `participant_input` parameter, which specifies the parameters via a player file. Properties are provided in a specific order, which is listed below in column _Pos_ (1 indexed). 

Table: Unit Specific Parameters { #tbl:unit-spec-table }

Pos | Parameter | Units | Range | Default | Description   
---|---|---|---|---|---  
1 | name |  |  |  | Object name (used to get a pointer to the object internally)   
2 | **Generator Object:** $\alpha_i$ | | **Generator Object:** $(0,1]$ | | **Generator Object:** unit participation factor.
  | **Link Object:** $c_i$ | | **Link Object:** $\{1,-1\}$ | | **Link Object:** directionality indicator 1 = Link _to_ zone, -1= Link _from_ zone.
3 | $\Delta P_{\text{dn}}$ | MW | $[0, (P_{\text{max}} - P_{\text{min}})P_{r}]$ | $(P_{\text{max}} - P_{\text{min}})P_{r}$ | Maximum change from original setpoint in the _downward_ direction. If output is $P^{*} = P_{\text{set}} + \Delta P^{*}$ then $\Delta P^{*} \geq -\Delta P_{\text{dn}}$. _Note:_ Not used for link objects.
4 | $\Delta P_{\text{up}}$ | MW | $[0, (P_{\text{max}} - P_{\text{min}})P_{r}]$ | $(P_{\text{max}} - P_{\text{min}})P_{r}$ | Maximum change from original setpoint in the _upward_ direction. If output is $P^{*} = P_{\text{set}} + \Delta P^{*}$ then $\Delta P^{*} \leq \Delta P_{\text{up}}$. _Note:_ Not used for link objects, since $\Delta P^{*} = 0$ always.
5 | $P_{\text{max}}$ | p.u. | $[-1,1]$ |  diesel generator: 1 | **Generator Object** : Maximum allowable output w.r.t rating $P_r$
  | | | | Grid Forming: **Pmax** |
  | | | | Grid Following: **Pref_max** |
  | | | | link: $\epsilon_{\text{tie}}$ | **Link Object** : positive tie-line tolerance $(P_i^{*} - P_{i})/P^{*} \leq P_{\text{max}}$
6 | $P_{\text{min}}$ | p.u. | $[-1,1]$ |  diesel generator: 0 | **Generator Object** : Minimum allowable output w.r.t rating $P_{r}$
  | | | | Grid Forming: **Pmin** |
  | | | | Grid Following: **Pref_min** |
  | | | | link: $-\epsilon_{\text{tie}}$ | **Link Object** : negative tie-line tolerance $(P_i^{*} - P_{i})/P^{*} \geq P_{\text{min}}$
7 | $T_i$ | s | $[\Delta t, \infty)$ | $T_{\text{lp}}$ | Low pass filter time constant. _Note_ : Note used for link objects.
  
There are three different key words that can be specified when passing unit specific parameters: 

- `ADD`: used to add new objects to the secondary controller,
- `MODIFY`: used to modify parameters of objects already participating in secondary control,
- `REMOVE`: used to to remove an object from the secondary controller.

The input file specified under the `participant_input` parameter can be given as a **glm** file (text file with certain structure) or a classic **csv** file. The **csv** method is recommended in general as it is more robust, easy to read, and does not risk exceeding the allotted array buffer of 1024 characters. To pass information to the secondary controller, this file needs to follow the following structure per row:

  1. The appropriate key word suggesting the action to be applied to the listed participants,
  2. The unit name and relevant properties (to skip a property simply leave it blank),
  3. Additional units, separated by a line in csv or `;` in glm.
  4. Repeat steps 1-3 for more key words as necessary
  
**Example**

The syntax is demonstrated using the following example. Initially (at time $t_{0}$) the secondary controller contains: 

Table: CSV Syntax { #tbl:table-syntax }

Name | $\alpha_i$ | $\Delta P_{\text{dn}}$ MW | $\Delta P_{\text{up}}$ MW | $P_{\text{max}}$ p.u. | $P_{\text{min}}$ p.u. | $T_i$ s   
---|---|---|---|---|---|---  
**gen1** | 0.7 | default | default | 0.9 | 0.3 | default   
**gen2** | 0.3 | 0.5 | 0.5 | default | default | 1   
**tie1** | -1 | N/A | N/A | 0.1 | 0.1 | N/A   
  
At time $t_{1}$ **gen2** is removed from secondary control. In response **gen1** participation factor $\alpha_{i} = 1$ and $P_{\text{max}} = 1$, and the tie line tolerance is changed to $\pm 15\%$

These specifications are translated in **glm** syntax in a simple text-formatted file as

```text
  t0 ADD gen1, 0.7, , , 0.9, 0.3; gen2, 0.3, 0.5, 0.5, , , 1; tie1, -1, , , 0.1, 0.1;
  t1 REMOVE gen2; MODIFY gen1, 1, , , 1; tie1, , , , 0.15, 0.15;
```    

Translated to **csv** syntax, these specifications are presented in 2 csv files accordingly:

- **t0.csv** contains lines
    
```text
  ADD
  gen1, 0.7, , , 0.9, 0.3
  gen2, 0.3, 0.5, 0.5, , , 1
  tie1, -1, , , 0.1, 0.1
```

- **t1.csv**
    
```text
  REMOVE
  gen2
  MODIFY
  gen1, 1, , , 1
  tie1, , , , 0.15, 0.15
```

These specifications in **csv** format are then loaded at specified times through a **player** file containing:

```text   
  t0 t0.csv
  t1 t1.csv
```

## Requirements/Limitations

The current GridLAB-D™ implementation of the secondary control imposes such requirements and limitations, specifically:

1. The secondary control object operates in transient mode only, but will transition back to QSTS.
2. Currently only `inverter_dyn` (the dynamic inverter) and `diesel_dg` (diesel generator) objects are supported as generators. Any power flow link object can be used as an inter-tie.
3. The frequency is currently only measured at the parent node of the secondary controller object.

### Generator - AGC Interaction Properties

The following properties are implemented in the `inverter_dyn` and `diesel_dg` objects to allow interaction with the secondary controller.  

Table: Generator Properties { #tbl:table-generator }

Object | Parameter | GLM file name| units | Default | Description   
---|---|---|---|---|---  
**diesel_dg** | $P_{\text{set}}$ | **pdispatch** | p.u. | GGOV with Rselect=1: $P_{\text{ref}}/R$ | Power set point   
 | | | | GGOV with Rselect=-1 or -2: $K_{\text{turb}}(P_{\text{ref}}/R - w_{\text{fnl}})$ |
 | | | | DEGOV1: $(\omega_{\text{ref}} - 1)/R$ |
 | | | | P_CONSTANT: $P_{\text{ref}}$ |
 | $\Delta P^{\star}$ | **pdispatch_offset** | p.u. | 0 | Offset to $P_{\text{set}}$. This is the quantity that the secondary controller manipulates   
**inverter_dyn** | $P_{\text{set}}$ | **pdispatch** | p.u. | GRID_FORMING, PSET_MODE: $P_{\text{set}}$ | Power set point   
 | | | | GRID_FORMING, FSET_MODE: $(2\pi f_{\text{set}} - \omega_{\text{ref}})/m_{p}$ |
 | | | | GRID_FOLLOWING or GFL_CURRENT_SOURCE: $P_{\text{ref}}/S_{\text{base}}$ |
 | $\Delta P^{*}$ | **pdispatch_offset** | p.u. | 0 | Offset to $P_{\text{set}}$. This is the quantity that the secondary controller manipulates   
  
The current set point, $P^{*}$, is calculated as  

$$P^{*} = P_{\text{set}} + \Delta P^{*} \tag{5}$$

### Link - AGC Interaction Properties

The following properties are implemented in the `link` object in the `powerflow` module to allow interaction with the secondary controller as a tie-line. 

Table: Link Properties { #tbl:table-link }

Parameter | GLM file name | units | Description   
---|---|---|---  
$P_{\text{set}}$ | **pdispatch** | W | Scheduled flow. Positive flow matches the links from-to definition.   
$\Delta P^{*}$ | **pdispatch_offset** | W | Offset to the scheduled flow. _Note_ : currently unused.   
set dispatch trigger | **set_dispatch** | true/false | Trigger to set schedule to current power flow value. When True will set: $P_{\text{set}} = (P_{\text{in}} + P_{\text{out}})/2$, $\Delta P^{*} = 0$  
  
### Secondary Control Interaction with QSTS Mode

**Initialization.** Since QSTS mode is a steady state formulation there is no frequency error. Therefore, all states are initialized to zero. 

**Return to QSTS Operation.** The controller keeps track of two conditions: 

1. Frequency within deadband $2\epsilon$: $-\epsilon \leq \Delta f \leq \epsilon$
2. Tie-lines within tolerance: $P_{\text{min}} \leq (P^{*}_{i} - P_{i})/P^{*}_{i} \leq P_{\text{max}} \forall i$

When both of these conditions are met for the length of two output sampling periods ($2T_{s}$) then the secondary controller requests to return to QSTS mode. 

## Examples

### Multiple Secondary Controllers With Inter-Ties

The following glm snippet shows how to set up a controller including inter-ties as shown in [](#fig:secondary-controller-ieee-123-bus-case). For brevety, only the controller for Microgrid 2 is detailed.

- Main **glm** model file:
    
```text
  // Controller Parametrization
  object sec_control {
    name secondary_controller_MG2;
    flags DELTAMODE;
    parent meter_50;
    deadband 0.001; // 1 mHz deadband
    B 1.7476; // MW/Hz
    kiPID 0.04; // pu/s
    kpPID 0; //pu
    Ts 0.2; // 200ms output sample time
    Ts_f 0.1; // 100ms frequency sample time
    Ts_P 0.2; // 200ms unit error and inter-tie sample time
    anti_windup FEEDBACK_PIDOUT;
    participant_input "sec_cntrl_MG2_part_init.csv";
  };
  
  //Triggers to set the tie-line schedules 
  object player{
    name MG1_MG2_tie_set;
    flags DELTAMODE;
    parent microgrid_switch1;
    file "tielineset.player";
    property set_pdispatch;
  };
  object player{
    name MG2_MG3_tie_set;
    flags DELTAMODE;
    parent microgrid_switch4;
    file "tielineset.player";
    property set_pdispatch;
  };
```   

- Participant parameters as **csv** in `sec_cntrl_MG2_part_init.csv`: 
    
```text
    ADD
    Inv2, 1
    microgrid_switch1, 1
    microgrid_switch4, -1
```

- Triggering player `tielineset.player`: 

```text
  2001-08-01 12:00:00, true
```

### Low Pass Filter

The following example uses the low pass filter option via a default value. The `participant_input` also shows a custom value for $P_{\text{min}}$ for Gen4. 

- Main **glm** model file:

```text
  object sec_control {
    name secondary_controller;
    flags DELTAMODE;
    parent node_150;
    deadband 0.001; //1 mHz deadband
    B 12.1187; //MW/Hz
    kiPID 0.2; // pu/s;
    kpPID 0; //pu
    Ts 1;
    Tlp 1;
    anti_windup FEEDBACK_PIDOUT;
    participant_input "sec_cntrl_part_init.csv";
  };
```    

- Participant parameters as **csv** in `sec_cntrl_part_init.csv`:

```text
  ADD
  Gen1, 0.3
  Gen4, 0.2,,,,0.3
  Inv1, 0.2
  Inv3, 0.3
```

![Secondary controller setup on the IEEE 123 Bus case for illustration purposes.](../../../../../images/300px-Sec_cntrl_IEEE_123.png){ #fig:secondary-controller-ieee-123-bus-case }


## References

[1] Wood, Allen J., Bruce F. Wollenberg, and Gerald B. Sheblé. "Power generation, operation, and control". John Wiley & Sons, 2013.

[2] Aström, Karl. "Control System Design" [Online PDF](https://www.cds.caltech.edu/~murray/courses/cds101/fa02/caltech/astrom-ch6.pdf), 2002, pp. 228–231.

## Summary of Key Parameters

Table: Key Parameters and Variables of the GridLAB-D™ AGC/secondary control (`sec_control`) Object { #tbl:sec-ctrl-parameters }

|Published Name|Unit|Type|Description|
|---|---|---|---|
|**participant_input**||char|command string for creating/modifying secondary controller participants|
|**participant_count**||int|Number of objects currently participating in secondary control.|
|**f0**|Hz|double|Nominal frequency in Hz|
|**underfrequency_limit**|Hz|double|Maximum positive input limit to PID controller is f0 - underfrequency_limit|
|**overfrequency_limit**|Hz|double|Maximum negative input limit to PID controller is f0 - overfrequency_limit|
|**deadband**|Hz|double|Deadband for PID controller input in Hz|
|**tieline_tol**|pu|double|Generic tie-line error tolerance in p.u. (w.r.t set point)|
|**B**|MW/Hz|double|frequency bias in MW/Hz|
|**kpPID**|pu|double|PID proportional gain in pu|
|**kiPID**|pu|double|PID integral gain in pu/s|
|**kdPID**|pu|double|PID derivative gain in pu*s|
|**anti_windup**||enumeration|Integrator windup handling [NONE, ZERO_IN_DEADBAND, FEEDBACK_PIDOUT, FEEDBACK_INTEGRATOR]|
|**Ts**|s|double|Secondary controller sampling period in sec.|
|**Ts_P**|s|double|Secondary controller input sampling period in sec for unit/tie-line errors.|
|**Ts_f**|s|double|Secondary controller input sampling period in sec for frequency.|
|**Tlp**|s|double|Default low pass time constant for participants in sec. Default is 0, meaning no low pass filter.|