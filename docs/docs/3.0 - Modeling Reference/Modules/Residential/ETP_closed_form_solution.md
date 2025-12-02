
# Equivalent Thermal Parameter Model

The model approach that is used to estimate thermal loads is called an equivalent thermal parameter (ETP) modeling approach. Because the ETP approach has been proven to provide reasonably model the residential (and small commercial building) loads and energy consumption and also because it is based on first principles, this modeling approach has been chosen for the current work (Sonderegger 1978; Subbarao 1981; Wilson et al. 1985; Pratt and Taylor 1994).

## House

Thermal loads are implemented using the ETP model, which determines the heat flow between an inner and outer air mass and one solid mass embedded within the inner air mass. 

The basic ETP parameters are 

  * $U_a$: the conductance between the inner and outer air mass
  * $C_a$: the heat capacity of the internal air mass
  * $C_m$: the heat capacity of the internal solid mass
  * $U_m$: the conductance between the inner air mass and the inner solid mass
  * $T_o$: the outer air temperature
  * $T_i$: the inner air temperature
  * $T_m$: the inner mass temperature
  * $Q_a$: the heat flux to the interior air mass
  * $Q_m$: the heat flux to the interior solid mass (this is not supported yet)
  
The solution to the ETP model is based on the two ordinary differential equations (ODEs) 

$$\begin{align} \frac{d T_i}{d t} & = \frac{1}{C_a} \left [ T_m U_m - T_i \left ( U_a + U_m \right ) +Q_a +T_0 U_a \right ] \\\ \frac{d T_m}{d t} & = \frac{1}{C_m} \left [ U_m \left ( T_i - T_m \right ) + Q_m \right ] \end{align}$$

The general first-order ODEs are found by inspection: 

$$\begin{align} \dot T_i & = c_1 T_i + c_2 T_m + c_3 \\\ \dot T_m & = c_4 T_i + c_5 T_m + c_6 \end{align}$$

where the constants $c_1$ through $c_6$ are 

  * $c_1 = -\frac{U_a + U_m}{C_a}$
  * $c_2 = \frac{U_m}{C_a}$
  * $c_3 = \frac{Q_a + U_a T_0}{C_a}$
  * $c_4 = \frac{U_m}{C_m}$
  * $c_5 = -\frac{U_m}{C_m}$
  * $c_6 = \frac{Q_m}{C_m}$

The general form of the second-order ODE is given by 

$$p_1 \ddot T_i + p_2 \dot T_i + p_3 T_i + p_4 = 0$$

where 

  * $p_1 = \frac{1}{c_2}$
  * $p_2 = -\frac{c_1+c_5}{c_2}$
  * $p_3 = \frac{c_1 c_5}{c_2}-c_4$
  * $p_4 = \frac{c_3 c_5}{c_2}-c_6$

where 

  * $r_1$ and $r_2$ are the roots of $p_1 r^2 + p_2 r + p_3 = 0$,
  * $k_1 = \frac{r_2 T_{i,0} + r_2 p_4 / p_3 - \dot T_{i,0}}{r_2-r_1}$,
  * $ k_2 = \frac{T_{i,0} -r_1 k_1}{r_2}$
  * $t$ is the elapsed time,
  * $T_{i,0}$ is the initial value of $T_i$, and
  * $\dot T_{i,0} = c_1 T_{i,0} + c_2 T_{m,0} + (c_1 + c_2) T_0 + c_7$

with 

  * $c_7 = \frac{Q_a}{C_a}$, and
  * $T_{m,t} = k_1 \frac{r_1-c_1}{c_2} + k_2 \frac{r_2-c_1}{c_2} - \frac{p_4}{p_3} + \frac{c_6}{c_2}$

## Modeling Assumptions

The house model assumes that only envelope characteristics, solar gain through windows and internal gain contribute to the HVAC load. Only air conditioners and heat pumps are modeled in the current implementation. 

## Modeling Approach

The electric circuit analog of an ETP model used to simulate heating and cooling loads in a typical residence is shown in Figure 3. The heat transfer properties are represented by equivalent electrical components with associated parameters for modeling the thermostatically controlled heating, ventilation, and air-conditioning (HVAC) system. 

![ETP Representation of the Typical Residences](../../../../images/300px-Residential_Module_Guide_Figure_3.png)

##### Figure 1 – ETP Representation of the Typical Residences

where, 

  * $C_{air}$ – air heat capacity (Btu/°F or J/°C)
  * $C_{mass}$ – mass (of the building and its content) heat capacity (Btu/°F or J/°C)
  * $UA_{wall}$ – the gain/heat loss coefficient (Btu/°F.h or W/°C) to the ambient
  * $UA_{mass}$ – the gain/heat loss coefficient (Btu/°F.h or W/°C) between air and mass
  * $T_o$ – CLTDc \+ Tair (°F or °C)
  * $T_{ambient}$ – ambient temperature (°F or °C)
  * $T_{air}$ – air temperature inside the house (°F or °C)
  * $T_{mass}$ – mass temperature inside the house (°F or °C)
  * $Q_{HVAC}$ – heat rate for HVAC (Btu/hr or W)
  * $Q_{internal}$ – heat rate from other appliance, plug loads, lights and people in the residence (Btu/h or W)
  * $Q_{solar}$ – heat gain from solar (Btu/h or W)

A state space description of the ETP model is: 

$$\begin{align} \frac{dx}{dt} & = A x + B u \\\ y & = C x + D u \end{align}$$

where (**there's something important missing here: A, B, C, D are not described**) 

  * $R_1 = 1/UA_{insul}$
  * $R_2 = 1/UA_{mass}$
  * $Q = Q_{HVAC} + Q_{solar} + Q_{internal}$

Because there is wide diversity in the thermal parameters (UA, mass, efficiency of equipment, and over sizing factor) used to compute the load across homes, ranges can be provided for these parameters. Because of regional difference in construction practices and wide variation even within a region, a range of values can be assigned for critical parameters based on metering studies (Pratt et al. 1990). While estimating energy consumption of individual homes, the thermal parameters are randomly (from the established range using known distribution, for example, uniform) assigned to each home. 

The internal gains in the home are computed external to the house object and are assumed to be inputs to the ETP model. These gains represent heat from other appliances (such as range, microwave), plug loads, lights, and people. 

Solving the ETP model (simultaneously for Tair and Tmass), we can obtain the cooling/heating load of an individual home as a function of time. The energy consumption to meet the comfort needs in the home can then be computed by converting the thermal loads by using typical manufacturer-provided air conditioner part load performance data. Using the same simulation model, the energy consumption of a population of homes can also be computed. While simulating a population of homes, the energy consumption for each home is computed simultaneously at each time step. Therefore, we get an accurate representation of distribution feeder load when electric loads are aggregated. 

A single ETP model with different thermal parameters can be used to represent all homes in the population for simplicity, or if there is a need, multiple ETP models with different thermal parameters can be used. Therefore, in addition to changing input parameters while simulating a population of homes, the ETP model can also be changed to accurately represent a given building stock. This ensures that it accurately reproduces the effects of load diversity. 

## Limitation and Future Improvements

The approach described in the previous section only accounts for sensible loads. This does not lead to a significant error in heating load calculation (because most heating load is sensible); it does lead to some error in the estimation of the cooling load. 

## ETP closed form solution

The methodology described below is the same as used in the GridLAB-D™ code base (see [Caveats](#caveats)), although, over time additions have been made to allow for greater flexibility. Additionally modifications have been made to speed up the solution process as implemented in code. The solution methodology, as described here and implemented in GridLAB-D™ was validated against the referenced spreadsheet. 

[![Equivalent Thermal Parameters Circuit Modeled by House-e.](../../../../images/300px-Residential_module_users_guide_figure_1.png)](/wiki/File:Residential_module_users_guide_figure_1.png)

##### Figure 1. Equivalent Thermal Parameters Circuit Modeled by House-e.

For the thermal circuit in Figure 1, a heat balance (conservation of energy) can be written for the air temperature node ($T_A$) as: 

$Q_A - U_A (T_A-T_O) - H_M (T_A-T_M) - C_A \frac{dT_A}{dt} = 0 \qquad\qquad(1)$

The heat balance for the mass temperature node ($T_M$) can be written as: 

$Q_M - H_M (T_M-T_A) - C_M \frac{dT_M}{dt} = 0\qquad\qquad(2)$

As shown in the ETP closed form solution, Equation (1) can be solved for $T_M$, differentiated with respect to time to provide $dT_M/dt$, and both of these substituted into (2) to form a second order linear differential equation in $T_A$ of the form 

$a \frac{d^2T_A}{dt^2} + b \frac{dT_A}{dt} + c\ T_A = d\qquad\qquad(3)$

where: 

  * $a = \frac{C_M C_A}{H_M}$
  * $b = \frac{C_M (U_A + H_M)}{H_M} + C_A$
  * $c = U_A$
  * $d = Q_M + Q_A + U_A T_O$

which has the solution with known, constant boundary conditions $T_O$, $Q_A$, and $Q_M$ and initial conditions at time $t=0$ of $T_{A_o}$ and $dT_{A_o}/dt$

$T_A = A_1 e^{r_1 t} + A_2 e^{r_2 t} + \frac{d}{c}\qquad\qquad(4)$

where: 

  * $r_1 = \frac{-b + \sqrt{b^2-4ac}}{2a}$
  * $r_2 = \frac{-b - \sqrt{b^2-4ac}}{2a}$
  * $A_1 = \frac{r_2 T_{A_o} - \frac{dT_{A_o}}{dt} - r_2 \frac{d}{c}}{( r_2 - r_1 )}$
  * $A_2 = T_{A_o} - \frac{d}{c} - \frac{r_2 T_{A_o} - \frac{dT_{A_o}}{dt} - r_2 \frac{d}{c}}{( r_2 - r_1 )}$

The initial condition $T_{A_o}$ is known as the final condition of $T_A$ from previous time step. However, at any time step at which the boundary conditions $T_O$, $Q_A$, or $Q_M$ have changed (i.e. the weather, internal gains, heating/cooling output) from the previous time interval, then the new air temperature trajectory at the beginning of the time step can be derived from Equation (1) as 

$$\frac{dT_{A_o}}{dt} = \frac{H_M}{C_A T_{M_o}} - \frac{U_A + H_M}{C_A T_{A_o}} + \frac{U_A}{C_A T_O} + \frac{Q_A}{C_A}\qquad\qquad(5)$$

Then, differentiating Equation (4) and substituting it and Equation (4) into Equation (1) yields a solution for $T_M$ of the form: 

$$T_M = A_1 A_3 e^{r_1 t} + A_2 A_4 e^{r_2 t} + g + \frac{d}{c}\qquad\qquad(6)$$

where: 

  * $g = \frac{Q_M}{H_M}$
  * $A_3 = \frac{r_1 C_A}{H_M} + \frac{U_A + H_M}{H_M}$
  * $A_3 = \frac{r_2 C_A}{H_M} + \frac{U_A + H_M}{H_M}$
  
## Initial Room and Mass Air Temperature

When initializing a House_E simulation, the temperature and weather history prior to the first time step is unknown. First assume the house is at the equilibrium air temperature (at steady state with the heating and cooling system off, i.e. in balance). Equilibrium is defined by $dT_A/dt_o = 0$. Then, by differentiating (4), at the beginning of the initial time step, it can be shown that 

$$T_{A_{eq}} = \frac{d}{c} = T_O + \frac{Q_M + Q_A}{U_A}\qquad\qquad(7)$$

and the corresponding mass temperature at equilibrium is 

$$T_{M_{eq}} = T_{A_{eq}} + \frac{Q_M}{H_M}\qquad\qquad(8)$$

If the heating system would be “on” (based on the thermostat heating set point; see the section Heating/Cooling Thermostat Operations) at the condition $T_A = T_{A_{eq}}$, then the initial conditions are best approximated as 

$T_{A_o} = T_{M_o} = T_{set\ heat}\qquad\qquad(9)$ _# Heating system “on” at_ $T_A = T_{A_{eq}}$

If the cooling system would be “on” (based on the thermostat cooling set point) at the condition $T_A = T_{A_{eq}}$, then the initial conditions are best approximated as 

$T_{A_o} = T_{M_o} = T_{set\ cool}\qquad\qquad(10)$ _# Cooling system “on” at_ $T_A = T_{A_{eq}}$

The time of day when either of these approximate initial conditions is most correct is when the conditions in the house have been stable for a long period of time. So, starting the simulation at midnight may be a good choice. An often better choice would be the earlier of sunrise or just prior to a morning thermostat change. 

If neither the heating or cooling system would be “on” at the condition $T_A = T_{A_{eq}}$, then the initial conditions are best approximated as 

$T_{A_o} = T_{A_{eq}}\qquad\qquad(11)$ _# Heating/cooling system “off” at_ $T_A = T_{A_{eq}}$
$T_{M_o} = T_{M_{eq}}\qquad\qquad(12)$

### Derivation of ETP second-order ODE

Solving (2) for $T_A$ gives: 

$$0 = Q_M - H_M T_M + H_M T_A - C_M \frac{d}{dt}T_M\qquad (2.1)$$

$$T_A = \frac {C_M} {H_M} \frac{d}{dt}T_M + T_M - \frac {Q_M} {H_M}\qquad (2.2)$$

Differentiating (2.2): 

$$\frac{d}{dt}T_A = \frac {C_M} {H_M} \frac{d^2}{dt^2}T_M + \frac{d}{dt}T_M\qquad (2.3)$$

Solving (1.2) for $T_M$ gives: 

$$T_M = \frac {C_A} {H_M} \frac{d}{dt}T_A + \frac {U_A+H_M} {H_M} T_A - \frac {U_A} {H_M} T_O - \frac {Q_A} {H_M}\qquad (1.3)$$

Differentiating (1.3): 

$$\frac{d}{dt}T_M = \frac {C_A}{H_M} \frac{d^2}{dt^2}T_A + \frac {U_A+H_M} {H_M} \frac {d}{dt}T_A \qquad (1.4)$$

Rearranging (2.1): 

$$0 = C_M \frac{d}{dt}T_M + H_M T_M - Q_M - H_M T_A\qquad (2.4)$$

Substituting (1.3) and (1.4) into (2.4) gives: 

$$0 = \left [ \frac {C_M C_A} {H_M} \frac{d^2}{dt^2}T_A + \frac {C_M(U_A+H_M)} {H_M} \frac{d}{dt}T_A \right ]+ \left [ C_A \frac{d}{dt}T_A + (U_A + H_M) T_A - U_A T_O - Q_A \right ]- Q_M - H_M T_A\qquad (2.5)$$

Rearranging into the form of a differential equation gives: 

$$\frac {C_M C_A} {H_M} \frac{d^2}{dt^2}T_A + \left [ \frac {C_M(U_A+H_M)} {H_M} + C_A \right ] \frac{d}{dt}T_A + U_A T_A = Q_M + Q_A + U_A T_0\qquad (3)$$

or 

$$\frac {C_M C_A} {H_M} \frac{d^2}{dt^2}T_A + \left [ C_M \frac {U_A} {H_M} + C_M + C_A \right ] \frac{d}{dt}T_A + U_A T_A = Q_M + Q_A + U_A T_0\qquad (3.1)$$

or 

$$a \frac{d^2}{dt^2}T_A + b \frac{d}{dt}T_A + c T_A = d\qquad (3.2)$$

with 

$$a = \frac {C_M C_A} {H_M}\qquad (3.3a)$$

$$b = C_M \frac {U_A} {H_M} + C_M + C_A = \frac {C_M(U_A+H_M)} {H_M} + C_A\qquad (3.3b)$$

$$c = U_A\qquad (3.3c)$$

$$d = Q_M + Q_A + U_A T_0\qquad (3.3d)$$

## Indoor air temperature

The indoor air temperature is found by solving equation (3.2). A different solution method is used depending on whether $d$ is constant with time. 

### Fixed conditions

Let $T_A = e^{r t}$. Then: 

$$\frac{d}{dt}T_A = r e^{r t}$$

$$\frac{d^2}{dt^2}T_A = r^2 e^{r t}$$

and

$$a r^2 e^{r t} + b r e^{r t} + c e^{r t} = d\qquad (3.4)$$

The homogeneous (d=0) solution to (3.4) is 

$$a r^2 + b r + c = 0\qquad$$

so 

$$r = \frac {-b \pm \sqrt{b^2-4ac}} {2a}$$

$$r_1 = \frac {-b +\sqrt{b^2-4ac}} {2a}\qquad (3.4.1)$$

$$r_2 = \frac {-b - \sqrt{b^2-4ac}} {2a}\qquad (3.4.2)$$

From (3.3b): 

$$\begin{alignat}{2}b^2 & = \left [ \frac {C_M(U_A+H_M)} {H_M} + C_A \right ]^2 \\    & = C_M^2 \left ( 1+\frac{U_A}{H_M} \right )^2 + 2 C_M \left ( 1+\frac{U_A}{H_M} \right ) C_A + C_A^2 \\    & = C_M^2 + 2 C_M^2 \frac{U_A}{H_M} + C_M^2 \frac{U_A^2}{H_M^2} + 2 C_M C_A + 2 C_M \frac{U_A}{H_M} C_A + C_A^2\end{alignat}$$

From (3.3a) and (3.3c): 

$$4ac = 4 \left ( \frac {C_M C_A} {H_M}\right ) U_A$$

So 

$$\begin{alignat}{2}b^2-4ac & = C_M^2 + 2 C_M^2 \frac{U_A}{H_M} + C_M^2 \frac{U_A^2}{H_M^2} + 2 C_M C_A + 2 C_M \frac{U_A}{H_M} C_A + C_A^2 - 4 \left ( \frac {C_M C_A} {H_M}\right ) U_A \\& = C_M^2 \left [ 1 + 2\frac{U_A}{H_M}+ \frac{U_A^2}{H_M^2} + \frac{C_A}{C_M} + 2\frac{U_A}{H_M} \frac{C_A}{C_M} + \frac{C_A^2}{C_M^2} - 4 \frac{U_A}{H_M} \frac{C_A}{C_M} \right ] \\& = C_M^2 \left [ 1 + 2\frac{U_A}{H_M} + 2\frac{C_A}{C_M} \right ] + C_M^2 \left [ \frac{U_A^2}{H_M^2} - 2\frac{U_A}{H_M} \frac{C_A}{C_M} + \frac{C_A^2}{C_M^2}  \right ] \\& = C_M^2 \left [ 1 + 2\frac{U_A}{H_M} + 2\frac{C_A}{C_M} \right ] + C_M^2 \left [ \frac{U_A}{H_M} - \frac{C_A}{C_M}  \right ]^2 \\& > 0\end{alignat}$$

which guarantees that $r_1$ and $r_2$ are real and distinct numbers. 

Then we have 

$$(T_A)_C = A_1 e^{r_1 t} + A_2 e^{r_2 t}\qquad (3.5)$$

and 

$$T_A = (T_A)_C + (T_A)_P = A_1 e^{r_1 t} + A_2 e^{r_2 t} + (T_A)_P\qquad (3.6)$$

$$\frac{d}{dt}T_A = A_1 r_1 e^{r_1 t} + A_2 r_2 e^{r_2 t}\qquad (3.6.1)$$

$$\frac{d^2}{dt^2}T_A = A_1 r_1^2 e^{r_1 t} + A_2 r_2^2 e^{r_2 t} \qquad (3.6.2)$$

Substituting into (3.2) 

$$a A_1 r_1^2 e^{r_1 t} + a A_2 r_2^2 e^{r_2 t} +b A_1 r_1 e^{r_1 t} + b A_2 r_2 e^{r_2 t} +c A_1 e^{r_1 t} + c A_2 e^{r_2 t} = d\qquad (3.7)$$

$$A_1 e^{r_1 t} \underbrace{(a r_1^2+b r_1+c)}_{0} + A_2 e^{r_2 t} \underbrace{(a r_2^2+b r_2+c)}_{0}+ c(T_A)_P=d$$

$$(T_A)_P = \frac{d}{c}\qquad (3.7.1)$$

The general solution is: 

$$T_A = A_1 e^{r_1 t} + A_2 e^{r_2 t} + \frac{d}{c}\qquad (3.8)$$

$$T_A = A_1 e^{r_1 t} + A_2 e^{r_2 t} + \frac{Q_A+Q_M}{U_A} + T_O\qquad (3.8.1)$$

With the known boundary conditions at $ t=0$, $T_A(0)$ and $\frac{d}{dt}T_A(0)$

$$T_A(0) = A_1 + A_2 + \frac{d}{c}\qquad (4)$$

$$0 = A_1 + A_2 + \left ( \frac{d}{c} - T_A(0) \right )\qquad (4.1)$$

$$\frac{d}{dt}T_A(0) = A_1 r_1 + A_2 r_2\qquad (5)$$

$$0 = A_1 r_1 + A_2 r_2 - \frac{d}{dt}T_A(0)\qquad (5.1)$$

Then $A_1$ and $A_2$ can be found by subtracting (5) from (4) to eliminate $A_2$: 

$$\begin{alignat}{2}r_2 T_A(0) - \frac{d}{dt}T_A(0) & = r_2 A_1 + r_2 A_2 + r_2 \frac{d}{c} - r_1 A_1 - r_2 A_2 \\& = (r_2 - r_1) A_1 + r_2 \frac{d}{c}\end{alignat}\qquad (6)$$

$$A_1 = \frac {r_2 T_A(0) - \frac{d}{dt}T_A(0) - r_2 \frac{d}{c} } {r_2 - r_1}\qquad (6.1)$$

From (6.1) and (4) 

$$A_2 = T_A(0) - \frac{d}{c} - \frac { r_2 T_A(0) - \frac{d}{dt}T_A(0) - r_2 \frac{d}{c} } {r_2 - r_1}\qquad (7)$$

$$A_2  = T_A(0) \left ( 1 + \frac{r_2}{r_2-r_1} \right ) + \frac{d}{c} \left ( \frac{r_2}{r_2-r_1} - 1 \right ) - \left ( \frac{1}{r_2-r_1} \right) \frac{d}{dt}T_A(0)\qquad (7.1)$$

### Forced conditions

Given at the time $t$: 

$T_A(t)$ is the indoor air temperature, and
$T_M(t)$ is the building mass temperature
$T_O(t)$ is the outdoor air temperature,
$Q_A(t)$ is the heat added to the indoor air,
$Q_M(t)$ is the heat added to the building mass,

and the constant values 

$U_A$ is the building envelop conductivity to the indoor air,
$C_A$ is the heat capacity of the indoor air,
$H_M$ is the building mass conductivity to the indoor air, and
$C_M$ is the heat capacity of the building mass.

The heat balance on $T_A$ is 

$$0 = Q_A(t) - U_A ( T_A(t) - T_O(t) ) - H_M ( T_A(t) - T_M(t) ) - C_A \frac{d}{dt}T_A(t)\qquad (1a)$$

$$0 = Q_M(t) - H_M ( T_M(t) - T_A(t) ) - C_M \frac{d}{dt}T_M(t)\qquad (2a)$$

Rearranging (1a) and (2a) in the form for solving a differential equation: 

$$\frac{d}{dt}T_A(t) - \frac{H_M}{C_A}T_M(t) - \frac{U_A}{C_A}T_O(t) = \frac{1}{C_A}Q_A(t) - \frac{U_A+H_M}{C_A} T_A(t)\qquad (1.5)$$

$$\frac{d}{dt}T_M(t) + \frac{H_M}{C_M}T_M(t) - \frac{H_M}{C_M}T_A(t) = \frac{1}{C_M}Q_M(t)\qquad (2.5)$$

From above the second-order ODE is 

$$a \ddot x + b \dot x + c x = g(t) + h(t)\qquad (8)$$

where 

$$\begin{alignat}{2}
x &= T_A(t)-f_0/c \\\dot x &= \frac{d}{dt}T_A(t) \\\ddot x &= \frac{d^2}{dt^2}T_A(t) \\a &= \frac {C_M C_A} {H_M} & \qquad (3.3a) \\b &= C_M \frac {U_A} {H_M} + C_M + C_A = \frac {C_M(U_A+H_M)} {H_M} + C_A & \qquad (3.3b) \\c &= U_A & \qquad (3.3c) \\g(t) &= U_A T_O(t) + \frac{C_M U_A}{H_M} \frac{d}{dt} T_O(t) & \qquad (3.3e) \\h(t) &= Q_A(t) + Q_M(t) + \frac{C_M}{H_M} \frac{d}{dt} Q_A(t) & \qquad (3.3f)\end{alignat}$$

Separate (3.3e) into a constant and a linear function of time 

$$g(t) = g_0 + \Delta g \left ( t + \frac{C_M}{H_M} \right )\qquad (3.3g)$$

where 

$g_0 = U_A T_O(0) + \Delta g \frac{C_M}{H_M} \,$
$\Delta g = U_A \frac{T_O(\Delta t)-T_O(0)}{\Delta t}$

and $T_O(0)$ and $T_O(\Delta t)$ are the outdoor air temperatures at time $t=0$ and time $t=\Delta t$, respectively. 

Separate (3.3e) into a constant, a step $u(t)$, and an impulse $\delta(t)$ at time 0 

$$h(t) = h_0 + \Delta h \ u(t) + \frac{C_M}{H_M} \delta(t)\qquad (3.3h)$$

where 

$$\Delta h = Q_A(0^+) - Q_A(0^-) + Q_H(0^+) - Q_H(0^-) \,$$

Define the initial constant 

$$\begin{alignat}{2}f_0 = g_0 + h_0 = U_A T_O(0) + \frac{C_M}{H_M} \Delta g + Q_A(0^-) + Q_H(0^-)& \qquad (3.3i)\end{alignat}$$

We define the initial indoor air temperature conditions as 

$x_0 = T_A(0) - \frac{1}{U_A}f_0$ and $\dot x_0 = \frac{d}{dt}T_A(0)$

Use Laplace transforms convert equation (8) to $s$-domain 

$$a [ s^2 X(s) - s x_0 - \dot x_0 ] + b [ s X(s) - x_0 ] + c X(s) 
= \frac{1}{s} \Delta h + \frac{1}{s^2}\Delta g + \frac{C_M}{H_M}$$

Rearrange to solve for $X(s)$

$$\begin{alignat}{2}  X(s) = \mathcal{L}[x(t)]     &= \frac{ax_0 + bx_0 + a\dot x_0 + \frac{C_M}{H_M} + \frac{\Delta h}{s} + \frac{\Delta g}{s^2}}{as^2+bs+c} \\    &= \frac{ax_0s^3 + (bx_0+a\dot x_0+\frac{C_M}{H_M})s^2 + \Delta h\ s +\Delta g}{s^2(as^2+bs+c)} & \qquad (9)\end{alignat}$$

Expand the partial fractions to get 

$$\begin{alignat}{2}X(s) &= A\frac{s}{as^2+bs+c} + B\frac{1}{as^2+bs+c} + C\frac{1}{s} + D\frac{1}{s^2} \\        &= \frac{As^3 + Bs^2 + Cs(as^2+bs+c) + D(as^2+bs+c)}{s^2(as^2+bs+c)} \\        &= \frac{(A+Ca)s^3 + (B+Cb+Da)s^2 + (Cc+Db)s + Dc}{s^2(as^2+bs+c)} & \qquad (9.1) \end{alignat}$$

Match the terms in equations (9) and (9.1) 

$$ \begin{alignat}{2}a x_0 &= A + C a & \qquad (9.1a) \\b x_0 + a \dot x_0 + \frac{C_M}{H_M} &= B + C b + D a  & \qquad (9.1b) \\\Delta h &= C c + D b  & \qquad (9.1c) \\\Delta g &= D c & \qquad (9.1d) \end{alignat}$$

Solve for $A$, $B$, $C$, and $D$

$$\begin{alignat}{2}   
(9.1a) & \rightarrow A = a x_0 - C a \\
(9.1d) & \rightarrow D = \Delta g / c \\
(9.1b) & \rightarrow B = b x_0 + a \dot x_0 + \frac{C_M}{H_M} - C b - a \Delta g / c \\
(9.1c) & \rightarrow C = \Delta h / c - b \Delta g / c^2 \\
& \rightarrow A = a x_0 - a \Delta h / c + a b / c^2 \Delta g \\
& \rightarrow B = b x_0 + a \dot x_0 + \frac{C_M}{H_M} - b \Delta h / c + (b^2/c^2-a/c) \Delta g
\end{alignat}$$

So we have 

$A = a x_0 - \frac{a}{c}\Delta h + \frac{ab}{c^2}\Delta g$
$B = b x_0 + a \dot x_0 + \frac{C_M}{H_M} - \frac{b}{c}\Delta h + \left ( \frac{b^2}{c^2} - \frac{a}{c} \right ) \Delta g$
$C = \frac{1}{c}\Delta h - \frac{b}{c^2}\Delta g$
$D = \frac{1}{c}\Delta g$

From above we know the quantity $(b^2-4ac)>0$ so we factor 

$$as^2+bs+c = (s+p)(s+q)\qquad$$

with 

$$p,q = -\frac{-b \pm \sqrt{b^2-4ac}}{2a}$$

and use inverse Laplace transforms on (9.1) to get 

$$\begin{alignat}{2}    T_A(t) = \mathcal{L}^{-1}[X(s)] &= \mathcal{L}^{-1} \left[ A \frac{s}{(s+p)(s+q)} \right ] \\  &+ \mathcal{L}^{-1} \left[ B \frac{1}{(s+p)(s+q)} \right ] \\  &+ \mathcal{L}^{-1} \left[ C \frac{1}{s} \right ] \\  &+ \mathcal{L}^{-1} \left[ D \frac{1}{s^2} \right ] \\  &+ \frac{1}{U_A}f_0 \\  &= A \frac{1}{q-p} \left( q e^{-qt} - p e^{-pt} \right) +     B \frac{1}{q-p} \left( e^{-pt} - e^{-qt} \right) +     C +     Dt + \frac{1}{U_A}f_0\end{alignat}$$

which simplifies to 

$$T_A(t) = \frac{Ap-B}{p-q}e^{-pt} - \frac{Aq-B}{p-q}e^{-qt} + C + Dt + T_O(0) + \frac{C_M}{H_M U_A} \Delta g + \frac{Q_A(0^-) + Q_H(0^-)}{U_A}\qquad (10)$$

where 

$$\begin{alignat}{2}    A &= \frac {C_M C_A} {H_M} \left [ T_A(0) - \frac{1}{U_A} f_0 \right ] - \frac {C_M C_A} {H_M U_A}\Delta h + \frac {C_M C_A \left( C_M \frac {U_A} {H_M} + C_M + C_A \right)} {H_M U_A^2} \Delta g \\B &= \left( C_M \frac {U_A} {H_M} + C_M + C_A \right) \left [ T_A(0) - \frac{1}{U_A} f_0 \right ] + \frac {C_M C_A} {H_M} \frac{d}{dt} T_A(0) + \frac{C_M}{H_M} \\  & - \frac{\left( C_M \frac {U_A} {H_M} + C_M + C_A \right)}{U_A} \Delta h    + \left [ \frac{\left( C_M \frac {U_A} {H_M} + C_M + C_A \right)^2}{U_A^2} -\frac{C_M C_A}{H_M U_A} \right ] \Delta g \\C &= \frac{1}{U_A} \Delta h - \frac{\left( C_M \frac {U_A} {H_M} + C_M + C_A \right)}{U_A^2} \Delta g \\D &= \frac{1}{U_A} \Delta g \\p &= -r_2=\frac{C_M \frac {U_A} {H_M} + C_M + C_A+\sqrt{ C_M^2 \left[ 1 + 2\frac{U_A}{H_M} + \frac{C_A}{C_M} \right] + C_M^2 \left [ \frac{U_A}{H_M} - \frac{C_A}{C_M}  \right ]^2}}{2\frac {C_M C_A} {H_M}} \\q &= -r_1=\frac{C_M \frac {U_A} {H_M} + C_M + C_A-\sqrt{ C_M^2 \left[ 1 + 2\frac{U_A}{H_M} + \frac{C_A}{C_M} \right] + C_M^2 \left [ \frac{U_A}{H_M} - \frac{C_A}{C_M}  \right ]^2}}{2\frac {C_M C_A} {H_M}}\end{alignat}$$

## Mass temperature

The mass temperature is found by solving equation (1.3) after the air temperature is determine. A different solution method is used depending on whether $d$ is constant with time. 

### Fixed conditions

Solving (1.2) for $T_M$ at any time gives 

$$T_M = \frac{C_A}{H_M} \frac{d}{dt}T_A + \frac{U_A+H_M}{H_M} T_A - \frac{U_A}{H_M}T_O - \frac{Q_A}{H_M}\qquad (1.5)$$

Substituting (3.8) and (3.6.1) for $T_A$ and $dT_A/dt$ gives 

$$T_M = \frac{C_A}{H_M} \left ( A_1r_1e^{r_1t} + A_2r_2e^{r_2t} \right )    + \frac{U_A+H_M}{H_M} \left ( A_1r_1e^{r_1t} + A_2r_2e^{r_2t} + \frac{Q_M+Q_A+U_AT_O}{U_A} \right )    - \frac{U_A}{H_M}T_O - \frac{Q_A}{H_M}\qquad (1.5.1)$$

Rearranging and simplifying 

$$T_M = A_1 \left ( \frac{C_A}{H_M}r_1 + \frac{U_A+H_M}{H_M} \right ) e^{r_1t}    + A_2 \left ( \frac{C_A}{H_M}r_2 + \frac{U_A+H_M}{H_M} \right ) e^{r_2t}    + \frac{Q_M}{H_M} + \frac{Q_M}{U_A} + \frac{Q_A}{U_A} + T_O\qquad (1.5.2)$$

Let 

$$T_M = A_1 A_3 e^{r_1t} + A_2 A_4 e^{r_2t} + \frac{Q_M}{H_M} + \frac{Q_M}{U_A} + \frac{Q_A}{U_A} + T_O\qquad (1.5.3)$$

where 

$$A_3 \equiv \frac{C_A}{H_M}r_1 + \frac{U_A+H_M}{H_M}\qquad (1.5.3a)$$

$$A_4 \equiv \frac{C_A}{H_M}r_2 + \frac{U_A+H_M}{H_M}\qquad (1.5.3b)$$

(1.5.2) gives $T_M$ for the next time step, i.e., at $t = t_0 + \Delta t$. Note the solution is discontinuous at any time $t_s$ when $Q_M$, $Q_A$ or $T_O$ change. So at the time $t_s^+$ immediately after a change (1.3) must be solved for 

$$\frac{d}{dt}T_A|_{t_s^+}= \frac{H_M}{C_A}T_{M_{t_s}} - \frac{U_A+H_M}{H_M}T_{A_{t_s}}+ \frac{U_A}{C_A}T_{O_{t_s}}+ \frac{Q_{A_{t_s}}}{C_A}\qquad (1.5.4)$$

### Forced conditions

Substituting (3.3g) and (3.3h) into (1.5) with $ f_0=g_0+h_0$

$$\begin{alignat}{2}T_M(t) &= \frac{C_A}{C_M} \frac{d}{dt}T_A(t) + \frac{U_A+H_M}{H_M} T_A(t) - \frac{1}{H_M} \left ( f_0 + \Delta h + t \Delta g \right )& \qquad (11)\end{alignat}$$

From (10) we have 

$$\frac{d}{dt}T_A(t) = -p \frac{Ap-B}{p-q} e^{-pt} + q \frac{Aq-B}{p-q} e^{-qt} + D$$

Substitute this and (10) into (11) 

$$\begin{alignat}{2}T_M(t) &= \frac{C_A}{C_M} \left [ -p \frac{Ap-B}{p-q} e^{-pt} + q \frac{Aq-B}{p-q} e^{-qt} + D \right ] \\       &+ \frac{U_A+H_M}{H_M} \left [ \frac{Ap-B}{p-q} e^{-pt} - \frac{Aq-B}{p-q} e^{-qt} + C + D t \right ] \\       &- \frac{1}{H_M} \left ( f_0 + \Delta h + t \Delta g \right )\end{alignat}$$

Rearrange this to get 

$$ \begin{alignat}{2}T_M(t) &= \left ( \frac{U_A+H_M}{H_M} - p \frac{C_A}{C_M} \right ) \frac{Ap-B}{p-q} e^{-pt}        - \left ( \frac{U_A+H_M}{H_M} - q \frac{C_A}{C_M} \right ) \frac{Aq-B}{p-q} e^{-qt}       - \frac{1}{H_M} \left ( f_0 + \Delta h + t \Delta g \right )& \qquad (11.1)\end{alignat}$$

## Implementation Notes

### Initial conditions

The initial indoor air temperature condition $T_A(0)$ is available from the initial values given in the GLM model. However, the initial derivative of the indoor air temperature is not usually given. The value of $dT_A/dt(0)$ can be computed based on the initial state of the HVAC system by solving the equation (1.3) for $dT_A/dt$ at time $t=0$: 

$$\frac{d}{dt} T_A(0) = \frac{H_M}{C_A}T_M(0) + \frac{U_A}{C_A}T_O(0) + \frac{1}{C_A}Q_A(0) - \frac{U_A+H_M}{C_A} T_A(0)$$

where the conditions $T_M(0)$, $T_O(0)$, and $Q_A(0)$ are all known states of the ETP model at the end of the last time increment. 

### Solving for time

Solving equation (10) for time must be done using a numerical solver that implements Newton's method. GridLAB-D™ has an ETP solver function built-in for this purpose. See [Solvers API] for details. 

## Caveats

Only the indoor air and mass temperature solution for fixed outdoor air and heat gain condition is currently implemented in GridLAB-D™. This requires that outdoor condition and heat gain conditions be linearized for each solution and can lead to small errors when integrating over variable timesteps (which GridLAB-D™ does quite often). This can introduce discrepancies when comparing simulations where the timing of solution updates differ for otherwise identical temperature and heat gain conditions. This problem could be largely remedied by using the forced condition solution, but as noted above the derivation of this solution appears to be incorrect and cannot be implemented at this time. 

## Sources

  * [RG Pratt, **ETP Solution** , _PNNL Engineering Worksheet_ , 5/27/2008](https://github.com/gridlab-d/design/blob/master/ETP/ETP%20Closed-Form%20Solution%205-27-08.pdf)


  * [RG Pratt, **ETP Solution** , _Excel Workbook_ , 5/27/2008 (edited later)](https://github.com/gridlab-d/design/blob/master/ETP/)


# TODO:

TODO - Relevant - Should this go somewhere or is it no longer relevant?  

### **Heat Loss Coefficient ( $U_A$)**

Compute exterior surface areas: 

  Define the following, based on rectangular geometry. Let x = width, y = depth.
  Then the aspect ratio is $R = y / x$
  The floor area is $A = x y n$

  and the volume is $V = A h$

The gross exterior wall area ($A_{wt}$) can be derived by introducing the perimeter (p), as follows: 

$$y\ x = A / n$$

$$y = R\ x$$

$$R\ x^2 = A / n$$

$$x^2 = \frac{A}{n R}$$

$$x = \sqrt{\frac{A}{nR}}$$

$$p\ = 2x + 2y = 2x + 2Rx = 2x (1 + R)$$

$$p\ = 2 (1 + R)\sqrt{\frac{A}{nR}} $$

$$A_{wt} = n\ h\ p $$

Then   
Equation | Explanation  
---  | -- |
| $A_{wt} = 2 n h (1 + R) \sqrt{\frac{A}{nR}}$ | The gross exterior wall area ($A_{wt}$)   
| $A_g = WWR\ A_{wt}\ EWR$ | The gross window area ($A_g$)   
| $A_d = n_d\ A_{1d}$ | The total door area ($A_d$)   
| $A_w = (A_{wt}-(A_g + A_d))\ EWR$ | The net exterior wall area ($A_w$)   
| $A_c = \frac{A}{n} ECR$ | The net exterior ceiling area ($A_c$)   
| $A_f = \frac{A}{n} EFR$ | The net exterior floor area ($A_f$)   
  
The total heat loss coefficient (conductance), $U_A$, for the house (the last term is for air infiltration); the defaults produce $U_A$ = 522.1 Btu/°F.hr 

$$U_A = A_g U_g + \frac{A_d}{R_d} + \frac{A_w}{R_w} + \frac{A_c}{R_c} + \frac{A_f}{R_f} + 0.018 A h I $$

    
!!! note

    0.018 is the volumetric heat capacity of air at standard conditions (Btu/°F.ft³std-air-pressure)

### **Interior Mass Surface Conductance ( $H_m$)**

Surface area is estimates as total exterior walls (less doors and windows) + interior walls + ceilings 

$$H_m = h_s\ (\frac{A_w}{EWR}) + A_{wt} IWR + \frac{A_c n}{ECR}$$

### **Total “Air” Mass ( $C_a$)**

Based on tuning to typical home heating system cycling times, the “air mass” seems to be well approximated as 3 times the volumetric capacitance of the interior air volume. 

$C_a = 3\ (0.018 A\ h)$ _# Short-cycle thermal mass_

### **Total Thermal Mass ( $C_m$)**

$C_m = A\ m_f - 2\ (0.018 A\ h)$ # Thermal mass (daily cycle), less that added to the “air” mass



