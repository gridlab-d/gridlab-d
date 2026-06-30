# Component Modeling

The method used for modeling components in the power flow module is consistent with Kersting (2007). Some minor manipulations were required for Gauss-Seidel implementation. The following sections are included to address any differences between our method and those described in Kersting (2007). 

## Node

Nodes represent busses or junctions in the distribution topology. The unbalanced three-phase voltage is calculated for each node and is available via the various output methods of GridLAB-D™. The different solvers utilize nodes differently and support different node types. 

The Forward-Back Sweep method (Kersting's method) treats all nodes as the same. Every node is treated as a connection point for lines and provides voltage levels at various points in the system. Loads on the system are handled explicitly in the load object. 

The Gauss-Seidel and Newton-Raphson methods have more explicitly defined bus types. Three bus types are available in both solvers. They are: 

  * `SWING` \- the infinite bus, voltage reference of a particular system
  * `PQ` \- a standard bus providing voltage levels (loads are handled explicitly in load objects)
  * `PV` \- a fixed voltage magnitude bus

and are specified with the input parameter `bustype`. All busses are `PQ` by default, so if a swing bus needed to be designated, use the parameter 
    
    bustype SWING;

in the input file. 

Despite the solver method selected, nodes provide a voltage point at a specific point in the system. The values for each of the three phases of the distribution system are contained in the `voltage_A`, `voltage_B`, and `voltage_C` parameters. Furthermore, for bus types like `SWING` and `PV`, the voltage parameters are used to specify the fixed specifications of that node. 

Along with line-to-neutral voltages, the line-to-line voltages are also available from nodes. These values are available from the `voltage_AB`, `voltage_BC`, and `voltage_CA` parameters. 

## Load

Loads are very similar to nodes and are handled the same way within the solver. Loads can inherit any properties of a node, so for the Gauss-Seidel and Newton-Raphson solvers a load could be specified as a `PV` bus with negative load and represent a generator. 

Loads can explicitly model three different types of loads connected to the distribution system. These loads are: 

  * constant power
  * constant current
  * constant impedance

The different loads are specified through the `constant_power_X`, `constant_current_X` and `constant_impedance_X` input parameters respectively. In each of the loading types, the `X` value is substituted by the appropriate phase. For example, if a constant current load of $1.044 - \jmath0.98 \text{ Amps}$ were needed on phase A, the load object would have a parameter of 
    
    constant_current_A 1.044-j0.98;
    
  
Loads are primarily connected in one of two forms: wye or delta. The connection of the different aspects of the load are determined by the `phases` parameter of the load. If a `D` appears anywhere in the `phases` parameter, a delta connection is assumed. For example, 
    
    phases ABD;
    

would indicate that a delta connected load exists on the line-to-line connection AB. 

Many IEEE test systems and feeders have all of the loads defined in terms of power, but subclassed as either a power, current, or impedance load. Due to GridLAB-D™'s ability to explicitly model these load types, the constant current and impedance load values must be translated from their power rating into an Amperage or impedance value. 

To calculate the constant current load from a system, use the equation 

$$\displaystyle I_{load} = \left(\frac{P}{V}\right)^{*}$$

where 

  * $I_{load}$ is the constant current load,
  * $P$ is the specified power value of the load,
  * $V$ is the nominal voltage at the load bus, and
  * $*$ is the complex conjugate operator.

It is important to point out that $V$ is _not_ a magnitude value. It is the nominal voltage on the particular phase of interest. For example, phase C in a balanced system with a nominal voltage magnitude of 2400 Volts would be represented as $V = 2400 \angle{} 120^{\circ}$. If the proper angle is not used, the current load value will be incorrect and produce incorrect answers in your system. 

  
Constant impedance loads are calculated using the equation 

$$\displaystyle Z_{load} = \left( \frac{V \cdot V^{*}}{P}\right)^{*}$$

where $Z_{load}$ represents the constant impedance load. 

Unlike the current calculations, the impedance calculation does not require as much care with the nominal voltage. In the numerator of the equation is $V \cdot V^{*}$ which is the same as $|V|^2$. However, the outside conjugation must still be considered to obtain the correct impedance value. 

## Meter

Meters are also very similar to node objects. Meters provide a method for measuring the instantaneous power or energy over time that is flowing through a node. This is useful for time-varying simulations with `recorder` objects attached. GridLAB-D™'s normal output methods (.XML, .TXT) will only record the final timestep of a system, so for time varying systems a `recorder` and meter are needed to track power and energy, as well as voltage and current. 

Due to the nature of GridLAB-D™'s solvers, current passing through a node or load is not directly measurable. Rather, a meter must be used to examine this current flow. The complex current flowing through each of the three phases in a system is available via the `measured_current_A`, `measured_current_B`, and `measured_current_C` variables. The A, B, and C letters represent the value obtained from each of the three phases of the system. 

All power measurements in the meter are based on the fundamental power equation given by 

$$S = P + \jmath{}Q = V \cdot{} I^{*}$$

where $V$ and $I$ represent the complex voltage and current respectively. Three different power measurements are available from a meter and are defined as: 

  * `measured_power` or power magnitude as $|S| = |S_A| + |S_B| + |S_C|$,
  * `measured_real_power` as $P = P_A + P_B + P_C$, and
  * `measured_reactive_power` as $Q = Q_A + Q_B + Q_C$

where $A$, $B$, and $C$ represent the three different phases of the distribution system. 

Energy passing through a metered point in the system is also available. The energy is obtained using the simple equation 

$$E = |S| \cdot t$$

with $|S|$ representing the sum of the magnitudes of power of the three phases and $t$ represents the time in hours. The result is a measure of energy passing through the meter in Watt-hours available on the variable `measured_energy`. 
