# Microgrids

**TODO**: 
This was a "Dev_" page. Review and rework to update

TODO - Introduction - Microgrids are useful 

# Implementation

The microgrids capability in GridLAB-D™ will be implemented to allow islanded, smaller power system simulations. These simulations will examine sub-second influences on parameters like frequency and voltage. The final capability will allow the examination of transients in the voltage and frequency associated with microgrid operations. 

## Equations

The primary basis for the implementation of the microgrids capability will involve modeling the relevant components with accurate equation models. At this time, these models are expected to operate down to a 1-millisecond time step. 

Equations are taken from Reference 1 and 2 below. Equation notation will follow: 

##### Table 1 - Equation Notation  

Variable | Definition   
---|---  
$\displaystyle{}\Delta{}P$ | Imbalance between generation and load of the system   
$\displaystyle{}\omega{}$ | Synchronous frequency of the system   
$\displaystyle{}H$ | Inertial constant of the system   
$\displaystyle{}K_D$ | Damping factor of system   
$\displaystyle{}e_x$ | Instantaneous stator phase to neutral voltage for phase or axis $\displaystyle{}x$  
$\displaystyle{}i_x$ | Instantaneous stator current for phase or axis $\displaystyle{}x$  
$\displaystyle{}e_{fd}$ | Field voltage   
$\displaystyle{}i_{fd}$ | Field current   
$\displaystyle{}i_{kd}, i_{kq}$ | Amortisseur currents   
$\displaystyle{}\theta$ | Angle between direct axis of rotor and phase $a$ of the stator (mechanical angle of the rotor)   
$\displaystyle{}\delta$ | Rotor angle - angle between voltage and current   
$\displaystyle{}S_d$ | $\displaystyle{}d$-axis component (direct axis) of $dq0$ transformation, or its inverse   
$\displaystyle{}S_q$ | $\displaystyle{}q$-axis component (quadrature axis) of $dq0$ transformation, or its inverse   
$\displaystyle{}S_0$ | $\displaystyle{}0$-axis (imbalance) component of $dq0$ transformation, or its inverse   
$\displaystyle{}S_a$ | $\displaystyle{}a$-axis component of $dq0$ transformation, or its inverse   
$\displaystyle{}S_b$ | $\displaystyle{}b$-axis component of $dq0$ transformation, or its inverse   
$\displaystyle{}S_c$ | $\displaystyle{}c$-axis component of $dq0$ transformation, or its inverse   
$\displaystyle{}R_x$ | Armature resistance for an individual phase   
$\displaystyle{}R_{fd}$ | Field circuit resistance   
$\displaystyle{}R_{kd}, R_{kq}$ | Amortisseur resistances   
$\displaystyle{}L_{xy0}$ | Mean value of varying inductance between phase or axis $x$ and $y$ \- see page 65 of 1  
$\displaystyle{}L_{xy2}$ | Amplitude of sinusoidal variation in inductance between phase or axis $x$ and $y$ \- see page 65 of 1  
$\displaystyle{}L_{xfd}$ | Mutual inductance between stator of phase $x$ and field winding   
$\displaystyle{}L_{fkd}$ | Mutual inductance between amortisseur and field winding   
$\displaystyle{}L_{ffd}$ | Self inductance of field winding   
$\displaystyle{}L_{xkd},L_{xkq}$ | Mutual inductance between stator of phase $x$ and amortisseur circuit   
$\displaystyle{}L_{kkd},L_{kkq}$ | Self inductance of amortisseur circuit on $dq0$-axis   
$\displaystyle{}\omega_{elec}$ | Current electrical speed (radians per second) of synchronous mechanical devices on system (current grid frequency)   
$\displaystyle{}\omega_{ref}$ | Reference rotation speed (radians per second) of synchronous machines on system (nominal frequency, e.g., $\displaystyle{}2\pi60$)   
$\displaystyle{}\omega_{mech}$ | Mechanical rotor speed (radians per second) of machine   
$\displaystyle{}\omega_{mech0}$ | Rated mechanical speed (radians per second) of the machine   
$\displaystyle{}J$ | Moment of inertia of the machine   
$\displaystyle{}P_t$ | Power at the terminals (stator) of the generator   
$\displaystyle{}T_m$ | Mechanical torque of the generator   
$\displaystyle{}T_e$ | Electrical torque of the generator   
$\displaystyle{}VA_{m}$ | Machine rated power output   
$\displaystyle{}p$ | Differential with time ($\displaystyle{}\frac{d}{dt}$)   
$\displaystyle{}P_{mech}$ | Current mechanical power output of generator   
$\displaystyle{}P_{mech\_prev}$ | Previous mechanical power output of generator   
$\displaystyle{}K$ | Governor scalar relating frequency change and power output change   
$\displaystyle{}R$ | Governor speed regulation (droop) parameter   
$\displaystyle{}V_{FB}$ | Voltage associated with stabilizing loop on exciter control   
$\displaystyle{}V_R$ | Voltage associated with regulator output on exciter control   
$\displaystyle{}V_A$ | Voltage associated with amplifier and lead-lag compensator output on exciter control   
$\displaystyle{}V_{Comp}$ | Voltage associated with load compensator model output (if it exists)   
$\displaystyle{}R_{Comp}$ | Exciter load compensator resistance value   
$\displaystyle{}X_{Comp}$ | Exciter load compensator inductance value   
$\displaystyle{}V_{PSS}$ | Voltage output from PSS device   
$\displaystyle{}K_E$ | Exciter gain   
$\displaystyle{}T_E$ | Exciter time constant   
$\displaystyle{}K_F$ | Exciter stabilizer gain   
$\displaystyle{}T_F$ | Exciter stabilizer time constant   
$\displaystyle{}T_B$ | Exciter lead lag numerator time constant   
$\displaystyle{}T_C$ | Exciter lead lag denominator time constant   
$\displaystyle{}T_A$ | Exciter regulator time constant   
$\displaystyle{}K_A$ | Exciter regulator gain   
$\displaystyle{}K_C$ | Exciter rectifier regulation limitations on output   
$\displaystyle{}A_{EX}$ | Exciter saturation function gain   
$\displaystyle{}B_{EX}$ | Exciter saturation function exponential term 

## Sub-second Implementation (Dynamic)

The GridLAB-D™ dynamic simulations represent electro-mechanic transients of unbalanced micro grid operation. The synchronous machines models are in fundamental frequency phasor representation considering unbalanced operation. The network and loads are represented with a full abc model. Additionally, diesel governor control and automatic voltage regulators are modeled. Figure 1 briefly presents the overall algorithm. Each model in the algorithm is explained in detail below. 

![Figure 1](../../../../../images/Sub-second_algorithm.png)

##### Figure 1. Overall algorithm of sub-second implementation

### Notation

The following variables and parameters are used in the dynamic model equations below. 

##### Table 1 - Equation Notation  

| Variable                          | Definition                           
|-|-|
| $i$ | Generator connected to bus i
| $E^\prime_{d,qi}$                 | Transient voltages for direct and quadrature axis
| $E^{\prime\prime}_{d,qi}$         | Subtransient voltages for direct and quadrature axis              
| $\psi_{1d,2qi}$                   | Flux linkages of direct and quadrature axis dampers               
| $T^\prime_{do,qoi}$               | Open circuit transient time constants for direct and quadrature axis                           
| $T^{\prime\prime}_{do,qoi}$       | Open circuit subtransient time constants for direct and quadrature axis                        
| $x^\prime_{d,qi}$                 | Transient reactances for direct and quadrature axis               
| $x^{\prime\prime}_{d,qi}$         | Subtransient reactances for direct and quadrature axis            
| $x_{li}$                          | Leakage reactance                   
| $R_{0,1,2i}$                      | Zero, positive, and negative sequence resistances                
| $I_{d,qi}$                        | Currents for direct and quadrature axis                          
| $I_{0,1,2i}$                      | Zero, positive, and negative sequence stator currents            
| $E_{fd}$                          | Field voltage                      
| $\omega_{i}$                      | Rotor mechanical speed             
| $\omega_{s}$                      | Rated rotor mechanical speed       
| $\delta_i$                        | Rotor angular position             
| $T_{mechi}$                       | Mechanical torque                  
| $H_{i}$                           | Inertia constant                   
| $D_{i}$                           | Machine damping                    
| $\left[ E^{\prime\prime}_{a,b,c} \right]$ | Vector of internal machine subtransient voltages in abc coordinates                           
| $V_{a,b,c}$                       | Network bus voltages in abc coordinates                         
| $I_{a,b,c}$                       | Network bus current injections in abc coordinates               
| $\left[ Jac \right]$              | Jacobian matrix of three-phase power flow solution              
| $\left[ Y_{Ga,b,ci} \right]$      | Matrix of machine subtransient admittances in abc coordinates   
| $Y_{G0,1,2i}$                     | Machine subtransient admittances in symmetrical components coordinates                        
| $\left[ I_{GSa,b,ci} \right]$     | Vector of machine Norton current sources in abc coordinates     
| $P_{T}, Q_{T}$                    | Total terminal active and reactive generated power              
| $I_{GS 0,1,2}$                    | Machine Norton current sources in symmetrical component coordinates                           
| $\left[ V_{Ga,b,c} \right]$       | Vector of generator phase terminal voltages                     
| $V_{R}$                           | Regulator voltage                  
| $V_{set}$                         | Voltage setting of automatic voltage regulator (AVR)            
| $V_{err}$                         | Control error for AVR             
| $x_{b}$                           | State variable of AVR transient gain reduction                  
| $T_{B}$                           | Time constant of AVR transient gain reduction                   
| $T_{C}$                           | Time constant of AVR transient gain reduction                   
| $T_{A}$                           | Exciter time constant              
| $K_{A}$                           | Exciter gain                      
| $E_{MAX,MIN}$                     | Exciter limits                    
| $\omega_{set}$                    | Governor speed setting            
| $R$                               | Governor droop                    
| $x_{1,2}$                         | Governor electric control box state variables                  
| $T_{1,2,3}$                       | Governor electric control box time constants                   
| $y_{gov}$                         | Governor electric control box output                           
| $K$                               | Governor actuator gain           
| $x_{4,5,6}$                       | Governor actuator state variables
| $T_{4,5,6}$                       | Governor actuator time constants 
| $y_{throttle}$                    | Governor actuator output (throttle)      

## Classic synchronous machine

The classic, synchronous machine is governed by several equations. For rotating machines, it is often useful to perform a space transformation from the $abc$ plane to a $dq0$ plane. This aligns the system with the rotor of the machine and makes inductance values fixed, rather than angle dependent. The transformation is given by: 

$$\begin{bmatrix}
    S_d \\
    S_q \\
    S_0 
\end{bmatrix} = \frac{2}{3} \begin{bmatrix} \cos(\theta) & \cos(\theta - 120^\circ{}) & \cos(\theta + 120^\circ)\\ -\sin(\theta) & -\sin(\theta - 120^\circ{}) & -\sin(\theta + 120^\circ)\\ \frac{1}{2} & \frac{1}{2} & \frac{1}{2} \end{bmatrix} \begin{bmatrix} S_a\\ S_b\\ S_c \end{bmatrix}$$

with an inverse transform of 

$$\begin{bmatrix}    
    S_a \\
    S_b \\
    S_c 
\end{bmatrix} = \begin{bmatrix} \cos(\theta) & -\sin(\theta) & 1\\ \cos(\theta - 120^\circ) & -\sin(\theta - 120^\circ{}) & 1\\ \cos(\theta + 120^\circ) & -\sin(\theta + 120^\circ{}) & 1 \end{bmatrix} \begin{bmatrix} S_d\\ S_q\\ S_0 \end{bmatrix}$$

This transformation is also useful for voltages and currents on the machine. For traditional, transmission-level synchronous machine modeling, the $S_0$ terms are assumed zero (balanced condition). For the unbalanced microgrid scenarios, all three components of the transformation will need to be kept. 

Despite the inclusion of the $0$ terms, many assumptions go into the typical $dq0$ representation that may not be valid for a microgrid scenario. To ensure these assumptions do not bias the distribution-level results, the $dq0$ transformation is not used to explicitly solve for the stator voltages or flux values. 

The equations for the three stator voltages are given by: [R1.2.1]

$$
\begin{split}
e_a &= p\psi_a - R_a i_a \\
e_b &= p\psi_b - R_b i_b \\
e_c &= p\psi_c - R_c i_c
\end{split}
$$

where the individual fluxes are defined as 

$$
\begin{split}
\psi_a =& -i_a L_{aa0} + L_{aa2} \cos(2\theta) 
         + i_b L_{ab0} + L_{ab2} \cos\left(2\theta + \frac{\pi}{3}\right) \\
       & + i_c L_{ac0} + L_{ac2} \cos\left(2\theta - \frac{\pi}{3}\right) 
         + i_{fd} L_{afd} \cos(\theta) \\
       & + i_{kd} L_{akd} \cos(\theta) 
         - i_{kq} L_{akq} \sin(\theta) \\
\psi_b =& i_a L_{ba0} + L_{ba2} \cos\left(2\theta + \frac{\pi}{3}\right) 
         - i_b L_{bb0} + L_{bb2} \cos\left(2\theta + \frac{2\pi}{3}\right) \\
       & + i_c L_{cb0} + L_{cb2} \cos\left(2\theta - \pi\right) 
         + i_{fd} L_{bfd} \cos\left(\theta - \frac{2\pi}{3}\right) \\
       & + i_{kd} L_{bkd} \cos\left(\theta - \frac{2\pi}{3}\right) 
         - i_{kq} L_{bkq} \sin\left(\theta - \frac{2\pi}{3}\right) \\
\psi_c =& i_a L_{ca0} + L_{ca2} \cos\left(2\theta - \frac{\pi}{3}\right) 
         + i_b L_{cb0} + L_{cb2} \cos\left(2\theta - \pi\right) \\
       & - i_c L_{cc0} + L_{cc2} \cos\left(2\theta + \frac{2\pi}{3}\right) 
         + i_{fd} L_{cfd} \cos\left(\theta + \frac{2\pi}{3}\right) \\
       & + i_{kd} L_{ckd} \cos\left(\theta + \frac{2\pi}{3}\right) 
         - i_{kq} L_{ckq} \sin\left(\theta + \frac{2\pi}{3}\right)
\end{split}
$$

The rotor voltage equations are governed by: 

$$
\begin{split}
e_{fd}&=p\psi_{fd}+R_{fd}i_{fd} \\
0&=p\psi_{kd}+R_{kd}i_{kd} \\
0&=p\psi_{kq}+R_{kq}i_{kq}
\end{split}
$$

where the individual fluxes are defined as 

$$\begin{split}\psi_{fd}&=L_{ffd}i_{fd}+L_{fkd}i_{kd}-L_{afd}i_a\cos(\theta{})-L_{bfd}i_b\cos(\theta{}-\frac{2\pi{}}{3})-L_{cfd}i_c\cos(\theta{}+\frac{2\pi{}}{3})\\
\psi_{kd}&=L_{fkd}i_{fd}+L_{kkd}i_{kd}-L_{akd}i_a\cos(\theta{})-L_{bkd}i_b\cos(\theta{}-\frac{2\pi{}}{3})-L_{ckd}i_c\cos(\theta{}+\frac{2\pi{}}{3})\\ \psi_{kq}&=L_{kkq}i_{kq}+L_{akq}i_a\cos(\theta{})+L_{bkq}i_b\cos(\theta{}-\frac{2\pi{}}{3})+L_{ckq}i_c\cos(\theta{}+\frac{2\pi{}}{3})\end{split}$$

Various components of these terms may be zeroed out, or omitted, for different models of the synchronous generator. These terms will need to be handled appropriately to ensure the solver is stable. 

The output power of the stator is defined as: 

$$P_t=e_ai_a + e_bi_b + e_ci_c$$

The mechanical components of the synchronous generator are governed by a series of equations relating the electrical and mechanical aspects of the generator. These are computed as: 

$$J=\frac{2H}{\omega_{mech0}^2}VA_{m}$$

$$T_m-T_e=J\frac{d\omega_{mech}}{dt}$$

$$P_m=T_m\omega_{mech}$$

and 

$$P_e=T_e\omega_{elec}$$

$$\frac{d\delta}{dt}=\omega_{elec}-\omega_{mech}$$

### Three-phase synchronous machine model

Unbalanced operation of three phase synchronous machines is modeled using a simplified fundamental frequency model in phasor representation according to [1, 2, 3, 4]. This simplification allows representing the machine in symmetrical components where the positive sequence represents the main electrical torque, and the negative sequence current produces a torque in opposition. The total electrical torque is constant, facilitating the solution and determination of equilibrium. However, the variation of electrical torque due to unbalanced operation reported in [5, 6] is ignored. In addition, typical assumptions for transient stability models are also made: ignoring sub-transient saliency, and neglecting the stator dynamics [7]. 

The machine electrical dynamic equations are: 

$$T^{\prime}_{doi} \frac{d E^{\prime}_{qi}}{dt}=E_{fd}- E^{\prime}_{qi}- (x_{di}-x^{\prime}_{di}) \left[I_{di}- \frac{(x^{\prime}_{di}-x^{\prime\prime}_{di})}{( x^{\prime}_{di}-x_{li})^2} \left[\psi_{1di}+ (x^{\prime}_{di}-x_{li}) I_{di}- E^{\prime}_{qi}\right] \right]$$

$$T^{\prime\prime}_{doi} \frac{d \psi_{1di}}{dt}=- \psi_{1di}+ E^{\prime}_{qi}- (x^{\prime}_{di}-x_{li}) I_{di}$$

$$T^{\prime}_{qoi} \frac{d E^{\prime}_{di}}{dt}=- E^{\prime}_{di}+ (x_{qi}-x^{\prime}_{qi}) \left[I_{qi}- \frac{(x^{\prime}_{qi}-x^{\prime\prime}_{qi})}{( x^{\prime}_{qi}-x_{li})^2} \left[\psi_{2qi}+ (x^{\prime}_{qi}-x_{li}) I_{qi}+ E^{\prime}_{di}\right] \right]$$

$$T^{\prime\prime}_{qoi} \frac{d \psi_{2qi}}{dt}=- \psi_{2qi}- E^{\prime}_{di}- (x^{\prime}_{qi}-x_{li}) I_{qi}$$

$$E^{\prime\prime}_{di}=-\frac{(x^{\prime\prime}_{qi}-x_{li})}{(x^{\prime}_{qi}-x_{li})} E^{\prime}_{di}+ \frac{(x^{\prime}_{qi}-x^{\prime\prime}_{qi})}{(x^{\prime}_{qi}-x_{li})} \psi_{2qi}$$

$$E^{\prime\prime}_{qi}= \frac{(x^{\prime\prime}_{di}-x_{li})}{(x^{\prime}_{di}-x_{li})} E^{\prime}_{qi}+ \frac{(x^{\prime}_{di}-x^{\prime\prime}_{di})}{(x^{\prime}_{di}-x_{li})} \psi_{1di}$$

The machine mechanical dynamic equations are: 

$$\frac{d\delta_{i}}{dt}=\omega_{i}-\omega_{s}$$

$$\frac{2 H_{i}}{\omega_{s}} \frac{d\omega_{i}}{dt}=T_{mechi}- \frac{(x^{\prime\prime}_{qi}-x_{li})}{( x^{\prime}_{qi}-x_{li})} E^{\prime}_{di} I_{di}- \frac{(x^{\prime\prime}_{di}-x_{li})}{( x^{\prime}_{di}-x_{li})} E^{\prime}_{qi} I_{qi}- \frac{(x^{\prime}_{di}-x^{\prime\prime}_{di})}{( x^{\prime}_{di}-x_{li})} \psi_{1di} I_{qi}+ \frac{(x^{\prime}_{qi}-x^{\prime\prime}_{qi})}{( x^{\prime}_{qi}-x_{li})} \psi_{2qi} I_{di}- (x^{\prime\prime}_{qi}-x^{\prime\prime}_{di}) I_{di} I_{qi}- (R_{2i}-R_{si}) I_{2i}^2- D_{i}(\omega_{i}- \omega_{s}) $$

  
### Network solution: initial static power flow and network solution during dynamic simulations

The network and loads are represented in full abc coordinates. The abc representation allows a complete representation of line and load unbalances. The network solution is anextension the method proposed in [8]. 
  
To adequately initialize the dynamic simulation, the static power flow solution of three-phase synchronous generator buses should consider the symmetry built features of generators. In other words the phase voltage and power unbalance of the generator buses cannot be freely determined. Even though the phase powers and voltages can be still unbalanced, they are subject to a generator symmetry built constraint. The symmetry built constraint was first introduced in [9] for PQ generator buses in a distribution network power flow solution. In [10], the symmetry built constraint is also applied to slack and PV buses, allowing for an adequate initialization of dynamic simulation from the static power flow solution. The symmetry constraints are applied to the initial static power flow as follows. 

The following equations describe the network solution model in compact form, for more details see [8]. 

$$\left[ \Delta I_{a,b,c} \right] = \left[ Jac \right] \left[ \Delta V_{a,b,c} \right]$$

Where the matrix $\left[ Jac \right]$ is equal to the bus admittance matrix, except for the diagonal elements that have additional terms to represent ZIP loads [8]. The full generator admittances are incorporated to the bus admittance matrices to represent the machine characteristics. The generator admittance matrices are, according to [9]: 

$$[Y_{Ga,b,ci}] = \frac{1}{3}\begin{bmatrix} 1 & 1 & 1 \\ 1 & e^{j 4\pi /3} & e^{j 2\pi /3} \\ 1 & e^{j 2\pi /3} & e^{j 4\pi /3} \\ \end{bmatrix} \begin{bmatrix} Y_{G0i} & 0 & 0 \\ 0 & Y_{G1i} & 0 \\ 0 & 0 & Y_{G2i} \\ \end{bmatrix} \begin{bmatrix} 1 & 1 & 1 \\ 1 & e^{j 2\pi /3} & e^{j 4\pi /3} \\ 1 & e^{j 4\pi /3} & e^{j 2\pi /3} \\ \end{bmatrix}$$
 
 The symmetry constraint applied to generator buses in the initial static power flow solution is given by [9]: 

$$I_{GS1}= \frac{P_{T}+jQ_{T} + \left[ V_{Ga,b,c} \right]^{*T} \left[ Y_{Ga,b,ci} \right] \left[ V_{Ga,b,ci} \right]} {V_{Ga}^{*}+ e^{j 4\pi /3} V_{Gb}^{*}+ e^{j 2\pi /3} V_{Gc}^{*}}$$

$$\left[ I_{GSa,b,ci} \right] = \frac{1}{3}\begin{bmatrix} 1 \\ e^{j 4\pi /3} \\ e^{j 2\pi /3} \\ \end{bmatrix} I_{GS1}$$

Due to generator symmetry built, there are no negative and zero sequence current sources [9]: 

$$\displaystyle{} I_{GS0}= I_{GS2}=0$$

The symmetry built constraint was applied to PQ generator buses in [9]. The application to Slack and PV generator buses in a micro grid setting is reported in [10]. The symmetry constraint is no longer applied during the dynamic simulations, where the symmetric generator’s current source injection depend on the dynamic states as explained in the following point. 

### Interface between machine model and network solution

The synchronous machine dynamic models are linked to the network solution by using Norton current source equivalents with shunt generator impedances, as suggested in [9, 11]. The current sources are symmetric balanced current injections to respect the generator symmetry built (windings symmetrically distributed in stator and rotor), and the generator impedances account for the effect of unbalanced terminal voltages. It is important to notice that the total generator current (current source minus current derived through generator shunt impedance) can also be unbalanced. The interface is formulated as: 

$$\left[ I_{GSa,b,ci} \right]= \left[ E^{\prime\prime}_{a,b,c} \right] \left[ Y_{Ga,b,ci} \right] $$

Where $\left[ E^{\prime\prime}_{a,b,c} \right]$ is a balanced voltage calculated by transforming $E^{\prime\prime}_{di}$ and $E^{\prime\prime}_{qi}$ to network reference frame and then to abc reference frame. 

### Synchronous machine controllers: governor and automatic voltage regulator

Generator control models are also defined in the GridLAB-D™ dynamic simulations. The generator control models are: 

  * Woodward diesel governor (DEGOV1) that modifies the mechanical power of the diesel generator proportionally to the generator speed deviation
  * Simplified exciter system (SEXS) that modifies the generator field voltage (and hence its reactive power) to control the generator^\prime s terminal voltage (average voltage magnitude of all phases)

DEGOV1 and SEXS models are commonly used in power system industry-grade transient stability programs such as GE PSLF and PSS/E.Block diagrams for DEGOV1 and SEXS can be found at: <http://www.powerworld.com/files/Block-Diagrams-16.pdf>. DEGOV1 is on page 123 and SEXS is on page 88. 

The simplified exciter system (SEXS) equations are: 

$$V_{err} = V_{set}- average \left( |[ V_{Ga,b,c} \right] | )$$

$$T_{B} \frac{d x_{b}}{dt} = V_{err}- x_{b}$$

$$V_{R}= x_{b}+ T_{C} \frac{d x_{b}}{dt}$$

$$T_{A} \frac{d E_{fd}}{dt} = K_{A} V_{R}- E_{fd}$$

$$E_{MIN} \leq E_{fd} \leq E_{MAX}$$

The diesel governor DGOV1 equations are: 

  * Electric control box:

$$T_{1} T_{2} \frac{d x_{2}}{dt} = \omega_{set}- \omega_{i}- R \cdot y_{throttle}- x_{1}- x_{2}$$

$$\frac{d x_{1}}{dt} = x_{2}$$

$$\displaystyle{} y_{gov} = T_{3} x_{2}+ x_{1}$$

  * Actuator:

$$T_{5} \frac{d x_{5}}{dt} = K \cdot y_{gov}- x_{5}$$

$$T_{6} \frac{d x_{6}}{dt} = x_{5}- x_{6}$$

$$\frac{d x_{4}}{dt} = x_{6}$$

$$T_{MIN} \leq x_{4} \leq T_{MAX}$$

$$\displaystyle{} y_{throttle}= T_{4} x_{6}+ x_{4}$$

  * Diesel engine:
  
$$T_{mechi} = delay \left( y_{t}, T_{D} \right)$$

### Traditional Transmission-level Implementation Comparison

Under traditional, positive-sequence, balanced implementation, the positive-sequence portion was often transferred into the $dq0$ space for the solutions. As part of this transformation and derivation, the equations were based on the assumption that the system is balanced three-phase. With this assumption, many of the terms are considered identical. Machine parameters are based on phase $A$ of the system, or a multiple of 3.0 of the phase $A$ component. Dependent on the parameter, this balanced assumption may pose problems with voltages or currents of a microgrid system. The nature of this impact is unknown at this time. 

With the ability to collapse the equations to balanced forms, and as a result ignore the $i_0$ term of the $dq0$ transformation, the equations for implementation become significantly reduced. Since everything is transferred to the $dq0$ plane, much of the formulation occurs in that notation. Table 2 will show the positive-sequence version compared to the unbalanced three-phase version above. Equations may be simplified for space using notation from 1. 

##### Table 2 - Traditional vs. unbalanced implementation  

Quantity | Traditional | Unbalanced   
---|---|---  
Voltages at terminal | $\begin{align}e_d &= p\psi_d - \psi_q p\theta - R_a i_d \\e_q &= p\psi_q + \psi_d p\theta - R_a i_q \\e_0 &= 0\end{align}$ | $\begin{align}e_a&=p\psi_a-R_ai_a\\ e_b&=p\psi_b-R_bi_b\\ e_c&=p\psi_c-R_ci_c\end{align}$  
Field and rotor voltages | $\begin{align}e_{fd}&=p\psi_{fd}+R_{fd}i_{fd}\\ 0&=p\psi_{kd}+R_{kd}i_{kd}\\ 0&=p\psi_{kq}+R_{kq}i_{kq} \end{align}$ | $\begin{align}e_{fd}&=p\psi_{fd}+R_{fd}i_{fd}\\ 0&=p\psi_{kd}+R_{kd}i_{kd}\\ 0&=p\psi_{kq}+R_{kq}i_{kq}\end{align}$  
Flux calculations - terminal | $\begin{align}\psi_d&=-L_di_d+L_{afd}i_{fd}+L_{akd}i_{kd}\\ \psi_q&=-L_qi_q+L_{akq}i_{kq} \end{align}$ | $\begin{align}\psi_a=&-i_al_{aa}+i_bl_{ab}+i_cl_{ac}+i_{fd}L_{afd}\cos(\theta{}) \\ &+i_{kd}L_{akd}\cos(\theta{})-i_{kq}L_{akq}\sin(\theta{})\\ \psi_b=&i_al_{ba}-i_bl_{bb}+i_cl_{bc}+i_{fd}L_{bfd}\cos(\theta{}-\frac{2\pi{}}{3})\\ &+i_{kd}L_{bkd}\cos(\theta{}-\frac{2\pi{}}{3})-i_{kq}L_{bkq}\sin(\theta{}-\frac{2\pi{}}{3})\\ \psi_c=&i_al_{ca}+i_bl_{cb}-i_cl_{cc}+i_{fd}L_{cfd}\cos(\theta{}+\frac{2\pi{}}{3})\\ &+i_{kd}L_{ckd}\cos(\theta{}+\frac{2\pi{}}{3})-i_{kq}L_{ckq}\sin(\theta{}+\frac{2\pi{}}{3})\end{align}$  
Flux calculations - rotor | $\begin{align}\psi_{fd}&=L_{ffd}i_{fd}+L_{fkd}i_{kd}-\frac{3}{2}L_{afd}i_d\\ \psi_{kd}&=L_{fkd}i_{fd}+L_{kkd}i_{kd}-\frac{3}{2}L_{akd}i_d\\ \psi_{kq}&=L_{kkq}i_{kq}-\frac{3}{2}L_{akq}i_q\end{align}$ | $\begin{align}\psi_{fd}=&L_{ffd}i_{fd}+L_{fkd}i_{kd}-L_{afd}i_a\cos(\theta{})\\ &-L_{bfd}i_b\cos(\theta{}-\frac{2\pi{}}{3})-L_{cfd}i_c\cos(\theta{}+\frac{2\pi{}}{3})\\ \psi_{kd}=&L_{fkd}i_{fd}+L_{kkd}i_{kd}-L_{akd}i_a\cos(\theta{})\\ &-L_{bkd}i_b\cos(\theta{}-\frac{2\pi{}}{3})-L_{ckd}i_c\cos(\theta{}+\frac{2\pi{}}{3})\\ \psi_{kq}=&L_{kkq}i_{kq}+L_{akq}i_a\cos(\theta{})+L_{bkq}i_b\cos(\theta{}-\frac{2\pi{}}{3})\\ &+L_{ckq}i_c\cos(\theta{}+\frac{2\pi{}}{3})\end{align}$  
Torque equations | $\begin{align}T_e&=\frac{3}{2}(\psi_di_q-\psi_qi_d)\frac{\omega_{elec}}{\omega_{mech}}\\ &=\frac{3}{2}(\psi_di_q-\psi_qi_d)\frac{pf}{2}\end{align}$ | $T_e=\frac{P_{mech}}{\omega_{mech}}-J\frac{d\omega_{mech}}{dt}$  
Power equations | $\begin{align}P_t=\frac{3}{2}&(i_dp\psi_d+i_qp\psi_q+2i_0p\psi_0)\\ &+(\psi_di_q-\psi_qi_d)\omega_{elec}\\ &-(i_d^2+i_q^2+2i_0^2)R_a\end{align}$ | $\displaystyle{}P_t=e_ai_a + e_bi_b + e_ci_c$  
  
From the table, it is clear the unbalanced equations end up being a little more complicated. Simplifications and reductions associated with the $dq0$ space and balanced voltage and current values allow the traditional model to collapse significantly. It is unclear at this time what the additional benefit the full three-phase unbalanced model brings over the assumed balanced model, so implementations may be altered to fit the "assumed balanced" model to reduce computational burdens. 

## Speed control

For the first implementation, simple speed control for the synchronous generator will be provided by two simple governor schemes. An isochronous and speed-droop governor will be implemented for frequency and speed control on the initial microgrids test scenarios. 

### Isochronous governor

An isochronous governor is used to adjust a generator's power output to maintain a specific frequency. This type of governor is often used on single-generator systems, or when only a single generator responds to changing loads (e.g., the other generators are ungoverned). A basic isochronous governor equation would be 

$$\displaystyle{}P_{mech}=P_{prev\_mech}+\int(-K(\omega_{elec}-\omega_{ref}))$$

where $K$ is an appropriate scalar between frequency deviation and power output. The integration operator is meant to reduce the isochronous governor's response to noise and momentary load changes. 

### Speed-droop governor

Speed-droop controls are necessary when more than one generator needs to correct for load changes. Each generator responds in a proportional manner to the frequency shift and adjusts its output. As a result, all generators (with the droop control) pick up a portion of the power output change. A simple droop governor equation is given by 

$$\displaystyle{}P_{mech}=P_{prev\_mech}+\Delta{}y$$

$$\Delta{}y=-\frac{1}{R}\left(\omega_{elec}-\omega_{ref}+\frac{1}{K}\frac{d\Delta{}y}{dt}\right) \tag{R1.2.2}$$

As with the isochronous governor control, $K$ is the scalar between frequency deviation and power output adjustments, while $R$ represents the droop factor.[R1.2.2]

## Exciters

The requirements call for the implementation of a simple DC exciter [R1.2.3]. A simple thyristor-rectifier exciter will also be implemented to enable more feasible modeling of lower cost, modern diesel generators. Both exciter types support load compensation inputs through the voltage $V_{Comp}$
1. In both cases, this is defined as: 

$$V_{Comp}=\left|\tilde{E}_{t}+(R_{Comp}+jX_{Comp})\tilde{I}_{t}\right|$$

where $\tilde{E}_{t}$ and $\tilde{I}_{t}$ are the positive-sequence terminal voltage and current, respectively. The positive-sequence version is used as a rough indication for the excitation field adjustment since only one field exists for all three phases (only one rotor per generator). 

### DC Exciter

To properly model the synchronous machine, the field windings inside the rotor need to be excited. A simple self-excited DC exciter will be modeled for the initial implementation. The modeled exciter is an IEEE type DC1A, diagrammed on page 363 of 1. The exciter is governed by the equations 

$$\frac{de_{fd}}{dt}=\frac{V_R-K_Ee_{fd}+e_{fd}SE(e_{fd})}{T_E}$$

$$V_{FB}=K_F\frac{de_{fd}}{dt}-T_F\frac{dV_{FB}}{dt}$$

$$\frac{dV_R}{dt}=\frac{K_AV_A-V_R}{T_A}$$

$$\frac{dV_A}{dt}=\frac{V_{in}+T_C\frac{dV_{in}}{dt}-V_A}{T_B}$$

where 

$$\displaystyle{}V_{in}=V_{PSS}+V_{Comp}+V_{ref}-V_{FB}$$

$$\displaystyle{}SE(e_{fd})=A_{EX}\exp{(B_{EX}e_{fd})}$$

The regulator output ($V_R$) can be further constrained by limiting values. These values, $V_{RMAX}$ and $V_{RMIN}$, are applied to the output of the regulator, if present. 

Terminal voltage for the exciter control is taken as the magnitude of the positive sequence component of all connected voltage terminals. 

This equation set represents an implementation where all possible components of the DC exciter are implemented (compensators, stabilizers, and amplifiers). These devices may not all always be present, so appropriate catches and checks will need to be in place to "remove" unneeded portions of the equations.[R1.2.3][R1.2.4]

### Thyristor Rectifier Exciter

A simple thyristor rectifier exciter does not have a feedback loop, so it is a more simplistic implementation that a DC exciter. This implementation is also known as an IEEE type AC4A exciter. The exciter is represented by the equations derived from Figure 8.42 of 1 as: 

$$\frac{dV_A}{dt}=\frac{V_{in}+T_C\frac{dV_{in}}{dt}-V_A}{T_B}$$

$$\frac{de_{fd}}{dt}=\frac{K_AV_R-e_{fd}}{T_A}$$

where 

$$\displaystyle{}V_{in}=V_{PSS}+V_{Comp}+V_{ref}$$

The value of $V_{in}$ can be constrained by limiting values of $V_{IMIN}$ and $V_{IMAX}$. The output can also be limited by values $V_{RMIN}$ and $V_{RMAX}-K_Ci_{fd}$. As with the DC exciter, the implementation will include all possible inputs (including PSS units), which may not always be present. Appropriate catches and processes will need to be in place to ensure this does not cause any issues with the solver. 

# Solver

The key development for the microgrids capability in GridLAB-D™ is the inclusion of a power system dynamic solver that interfaces with the appropriate objects within the GridLAB-D™ environment. Initially, this is expected to be predominately the powerflow module. 

Updates to voltage and frequency values should be made apparent for the next "quasi-steady-state" powerflow iteration. These values should be implemented in such a way that powerflow will not significantly override the control actions from the generator or load, with regards to voltage or frequency. An incremental powerflow solution is required at all dynamic timesteps to ensure the system is properly responding, so this incremental powerflow solution should coincide with the static solution at "quasi-steady-state" points.[R3.4][R3.5]

The implemented solver shall be done as a modified Euler, predictor-corrector solution method1. This implementation provides the basic structure laid out in the Power System Toolbox (PST) for MATLAB 3. Utilizing the modified Euler method will allow the flexibility of the different timestep sizes and progression outlined in the Solution Timesteps section below. 

## Interfacing Overview

To implement the dynamic equations to produce the frequency information, the microgrids implementation will need to be able to exchange several different pieces of information. These can be broken into simple inputs and outputs related to the solver. These inputs and outputs may be direct module-level interfaces, object-level inputs, as well as function-level interfaces for other objects.[R2] Interfacing with the static powerflow will require the implementation of PV-bus functionality into the Newton-Raphson solver. 

### Object inclusion

Inclusion in the microgrids-enabled dynamic solver capability will be handled during GridLAB-D's normal object initialization routines. objects contributing to the dynamics of the system will flag appropriately for inclusion by the solver during its initialization.[R1.1]

### Solver published inputs

Most of the inputs to the microgrids capability will be handled by the solver directly, with links to appropriate objects. One "extraneous" situation will require an external input into the dynamic solver. This inputs is: 

##### Table 3 - Solver inputs  

Variable | Type | Units | Definition   
---|---|---|---  
`external_frequency` | double | Hz | Externally driven frequency of the system   
  
If the system assumed to be connected to a much larger, higher inertia system with minimal impact on the frequency, the `external_frequency` variable will be used to set the system synchronous frequency. 

In cases of just being attached to a larger system, it is recommended than an over-sized generator be attached at the `SWING` node (high inertia and power values). This will allow the dynamic influences of a larger-connected (but not explicitly modeled) system to be accounted for without having to use the `external_frequency` variable. 

### Solver published outputs

Most of the added capabilities will be handled at the specific objects. However, a few global outputs will be available from the dynamic solver. These will include: 

##### Table 4 - Solver outputs  

Variable | Type | Units | Definition   
---|---|---|---  
**frequency** | double | Hz | Current estimated synchronous frequency of the system   
**frequency_change** | double | Hz/sec | Change in frequency per second   
**observed_generation** | double | Watts | Total power into the system observed   
**observed_load** | double | Watts | Total load power observed on the system   
  
These values are published mainly for recorder objects to capture them for overall GridLAB-D™ data output. The nature of the dynamic solver will require the creation of a new, frequency-capable recorder-like capability to capture the transient effects on the system. These properties will be among those recorded. 

### Solver data structure

To facilitate data operations between the individual objects and the dynamic solver capability, a common data structure will be used to pass information back and forth. This data structure should contain information and pointers to the following elements: 

##### Table 5 - Solver interface elements  

Variable | Definition   
---|---  
**central_frequency** | Pointer to the central synchronous frequency solution of the system   
**central_frequency_change** | Pointer to central frequency change between previous and current timestep   
**timestamp** | Pointer to the current timestamp of the solution   
**timestamp_change** | Pointer to the difference between the last and current timestamp   
**inertia** | Pointer to accumulated inertia of the system   
**phases** | Phase information - encoded like NR solver (0x04 = A, 0x02 = B, 0x01 = C)   
**voltage** | Pointer to complex voltage values of the object  
**current** | Pointer to complex current values of the object  
**current_contrib** | Pointer to complex current contributions of the object  
**power** | Pointer to complex power contributions of the object  
**impedance** | Pointer to complex impedance contributions of the object  
**frequency** | Pointer to local frequency solution associated with the object  
**frequency_change** | Pointer to frequency change in the local frequency solution between previous and current timestep   
**machine_parameters** | Pointer to machine parameters - includes exciter, pss, and governor properties   
  
`NULL` fields will be ignored as "no equipment present" to the solver. The pointer for this data structure will be passed to the dynamic solver capability during the solver's initialization routine.[R2.1][R2.2]

### Solver external functions

The nature of the microgrid capability and the dynamic solver will not initially require any external functions for devices. All relevant data passes and solver calls should be handled internal to the implementation. 

## Solver timing

The actual execution of the solver and how often it is called are key aspects of the proper integration of the dynamic solution capabilities within GridLAB-D™. Solver calls will need to be properly timed with the powerflow solution, as well as the requirements of the individual dynamic components. To ensure proper dynamic transitions, a form of the steady state powerflow will be resolved at each timestep of the dynamic simulation for the "non-contributing" objects. Loads and devices not directly participating in the dynamic solver are assumed to maintain fixed load values over the sub-GridLAB-D-standard timesteps. These devices may influence the magnitude and duration of the dynamic response, so they must be included in some form. Functionality for faster, dynamic loads will be in place for future implementations of microgrid devices. Through the continuous updates of the quasi-static powerflow, transitions at normal GridLAB-D™ timesteps should be minimized to be predominately non-dynamic state changes. 

### Solver Passes

The actual solver will follow the modified Euler, predictor-corrector implementation of the MATLAB Power System Toolbox (PST)3. For the predictor steps, the solution proceeds as: 

  1. Solve base powerflow
  2. Solve the network interface components (solve the loadflow - this may be a reduced loadflow) 
     1. Solve for the generators and motors
     2. Solve for the exciters
     3. Solve for the governors
  3. Solve the system dynamics components 
     1. Solve for the generators and motors
     2. Solve for any PSS units present
     3. Solve for the exciters
     4. Solve for the governors
  4. Apply the solutions as a predictor update
This is followed by a similar process for the corrector step. The corrector step follows: 

  1. Solve the network interface components (solve the loadflow - this may be a reduced loadflow) 
     1. Solve for the generators and motors
     2. Solve for any PSS units present
     3. Solve for the exciters
     4. Solve for the governors
  2. Solve the system dynamics components 
     1. Solve for the generators and motors
     2. Solve for any PSS units present
     3. Solve for the exciters
     4. Solve for the governors
  3. Apply the solutions as a corrector update

After all "machine values" have been updated, a global frequency is estimated 

  1. Update frequency calculation

After these two steps complete, the simulation advances to the next timestep. This sequence will repeat until the next GridLAB-D™ overall timestep is encountered. At that point, the changes will be reflected into the quasi-steady state powerflow solution, and the process will repeat. 

### Solution Timesteps

Timestep progression will be handled in a manner similar to GridLAB-D's core functionality. All objects requesting a dynamic solution update will request a time for recalculation. The minimum value will drive the simulation forward. The solver shall be implemented as a predictor-corrector solver, so larger timestep progression should be possible. A "maximum dynamic" timestep will also be specified to ensure any unexpected "passive" element (not requesting a timestep update) are handled.[R3.2]

The initial solver time resolution will be 1 ms. Timestep updates will occur in multiples of 1 ms, but will not be allowed to be any less than 1 ms.[R3][R3.1]

### Solver Call Timing

Actual execution of the solver will rely heavily on the interactions with the powerflow module. Dynamic quantity updates will need to wait for a final powerflow solution before progressing forward. To ensure proper powerflow convergence, the dynamics capabilities must not interfere with the static powerflow. The use of the new Subsecond capabilities and `postupdate` calls will be implemented to ensure this interaction does not occur. This will ensure all "dynamic contribution" objects are ready to transition back to the static powerflow. 

# Testing and Validation

Testing and validation will be conducted at various points along the programming to ensure the implementation is producing the correct results. Testing and validation will also be performed against known IEEE test systems and against simple systems in other software packages. Due to the nature of the three-phase unbalanced powerflow, fully testing the system will be difficult. The DigSilent Powerfactory software 4 will be used as the primary validation software suite for the GridLAB-D™ microgrids capability. 

A variety of test systems are needed to test the functionality of the microgrids capabilities. The expected models will be: 

  * Single machine, infinite bus system
  * Two machine, single line system
  * Two machine, three line system
  * Adapted IEEE 34-bus test feeder

Further validation could be accomplished with a model of real microgrids in operation on the United States grid. These will not be necessary for final implementation success, but will be useful validation systems: 

  * Oak Ridge National Laboratory microgrid
  * San Diego Gas & Electric Beach Cities microgrid
  * CERTS AEP microgrid

Validation using these real systems will occur against measured field data for those systems. 

## Single machine, infinite bus system

The classic single machine, infinite bus (SMIB) system provides an initial testing platform for the microgrids solver capabilities. The SMIB will be utilized to test a balanced, three-phase system. With infinite inertia (via the infinite bus), the system frequency will be fixed at 60 Hz. The system will run unfaulted. The system should run at a steady value and not produce any control actions. 

The completion of this test will gauge the numerical stability of the microgrids capability. The end results will also be validated against the base SMIB equations, or a simulation of the SMIB in the DigSilent or PST software. 

## Two machine, single line system

![Simple Two-machine Test System](../../../../../images/300px-Two-machine_single-line_System.png)

##### Figure 1. Simple Two-machine Test System

The two machine, single line (TWSL) system (Figure 1) will be used to test the simulation of actual dynamics on the system. All devices are full three-phase. Overhead lines connecting the two generators and the load follow the format of the IEEE 4-node test feeders 5. All test results will be validated against a DigSilent PowerFactory simulation to ensure the three-phase, unbalanced power is being handled correctly. 

The two machine, single line system will be used in a variety of ways to test and validate the results. For all of the testing scenarios, Gen 1 and Gen 2 are assumed to be 100 MVA diesel generators. Unless otherwise specified, both generators are identical. Both incorporate DC exciters and droop-control governors with the following parameters: 

##### Table 6 - Two machine, single line system parameters  

Variable | Value   
---|---  
$H$ | 10.0   
$\omega_{ref}$ | $120\pi{}$ radians/second   
$K$ | 100   
$R$ | 400 MW/Hz   
$K_E$ | Computed so $V_R$ is 0   
$T_E$ | 1.15   
$K_F$ | 0.058   
$T_F$ | 0.62   
$T_B$ | 0.06   
$T_C$ | 0.173   
$T_A$ | 0.89   
$K_A$ | 187   
$A_{EX}$ | 0.014   
$B_{EX}$ | 1.55   
  
The actual tests are defined as: 

##### Table 7 - Two machine, single line scenarios  

Scenario | Testing | Description   
---|---|---  
**Sinusoidal Balanced Power** | Frequency updates, solver interactions, governor response, exciter response | A time-varying, balanced "power" value will be played into the load portion of the system. The power will be slow moving with a time period of 10 seconds. The amplitude of this variation should be large enough to encourage governor action and to examine frequency updates. Throughout the variation, exciter actions are expected to vary the voltage on the generator terminals.   
**Sinusoidal Unbalanced Power** | Frequency updates, solver interactions, governor response, exciter response | A time-varying, unbalanced "power" value will be played into the load portion of the system. The power will be slow moving with a time period of 10 seconds. The amplitude of this variation should be large enough to encourage governor action and to examine frequency updates. Throughout the variation, exciter actions are expected to vary the voltage on the generator terminals.   
**Step-up Balanced Power** | Frequency updates, solver interactions, governor response, exciter response | A balanced step increase in the "power" value will be implemented after running the system for several seconds of "steady state" conditions. The amplitude of the step should be sufficient to encourage governor response. The variation is expected to cause exciter action and vary the voltage at the generator terminals.   
**Step-up Unbalanced Power** | Frequency updates, solver interactions, governor response, exciter response | An unbalanced step increase (with different step values at every phase) in the "power" value will be implemented after running the system for several seconds of "steady state" conditions. The amplitude of the step should be sufficient to encourage governor response. The variation is expected to cause exciter action and vary the voltage at the generator terminals.   
**Step-down Balanced Power**| Frequency updates, solver interactions, governor response, exciter response | A balanced step decrease in the "power" value will be implemented after running the system for several seconds of "steady state" conditions. The amplitude of the step should be sufficient to encourage governor response. The variation is expected to cause exciter action and vary the voltage at the generator terminals.   
**Step-down Unbalanced Power** | Frequency updates, solver interactions, governor response, exciter response | An unbalanced step decrease (with different step values at every phase) in the "power" value will be implemented after running the system for several seconds of "steady state" conditions. The amplitude of the step should be sufficient to encourage governor response. The variation is expected to cause exciter action and vary the voltage at the generator terminals.   
**Isochronous Governor Test** | Frequency updates, solver interactions, governor response, exciter response | Replicate all of the above tests with one generator set as an isochronous generator running at 60 Hz   
  
Successful completion of these tests, as well as the successful validation against DigSilent results, are required before attempting the Adapted 34-bus test feeder. 

## Two machine, three line system

![Basic Two-machine Test System](../../../../../images/300px-Two-machine_three-line_System.png)

##### Figure 2. Basic Two-machine Test System

The two machine, three line (TWTL) system (Figure 2) will be used to test the simulation of actual dynamics on the system. Many parameters will follow those of the TWSL system. All devices are full three-phase. Overhead lines connecting the two generators and the load follow the format of the IEEE 4-node test feeders 5. All test results will be validated against a DigSilent PowerFactory simulation to ensure the three-phase, unbalanced power is being handled correctly. 

The two machine, three line system will be used in a variety of ways to test and validate the results. For all of the testing scenarios, Gen 1 and Gen 2 are assumed to be 100 MVA diesel generators. Both generators follow the exact same setup as the two machine, single line system tested previously. 

The actual tests for the two machine, three line system will be identical to those of the two machine, single line system. However, the extra interaction path is expected to change the numerical values and will require resimulation by the DigSilent PowerFactory software. 

## Adapted IEEE 34-bus test feeder

![Adapted IEEE 34-bus Test System](../../../../../images/300px-IEEE34Modified.png)

##### Figure 3. Adapted IEEE 34-bus Test System 6

The adapted 34-bus test feeder (Figure 3) will serve as the overall test and validation system for the initial microgrids implementation. Since wind turbine generators and energy storage are not part of the initial microgrids implementations, these devices will be substituted with equivalent diesel generators. All simulation results will be validated against values obtained for an equivalent model run in the DigSilent software. 

### References

  1. Kundur, P. “Power system stability and control” New York: McGraw-hill, 1994.
  2. Harley, R. G., E. B. Makram, and E. G. Duran. "The effects of unbalanced networks on synchronous and asynchronous machine transient stability." Electric power systems research 13, no. 2 (1987): 119-127.
  3. Makram, E. B., V. O. Zambrano, and R. G. Harley. "Synchronous generator stability due to multiple faults on unbalanced power systems." Electric power systems research 15, no. 1 (1988): 31-39.
  4. Makram, E. B., V. O. Zambrano, R. G. Harley, and Juan C. Balda. "Three-phase modeling for transient stability of large scale unbalanced distribution systems." Power Systems, IEEE Transactions on 4, no. 2 (1989): 487-493.
  5. Salim, R. H., and R. A. Ramos. "A Model-Based Approach for Small-Signal Stability Assessment of Unbalanced Power Systems." IEEE Transactions on Power Systems, November 2012.
  6. Krause, P., O. Wasynczuk, and S. Scott. "Analysis of electric machinery." IEEE Power Eng. Soc 15, no. 3 (1995).
  7. Kundur, P., and P. L. Dandeno. "Implementation of advanced generator models into power system stability programs." Power Apparatus and Systems, IEEE Transactions on 7 (1983): 2047-2054.
  8. Garcia, Paulo AN, Jose Luiz R. Pereira, Sandoval Carneiro Jr, Vander M. da Costa, and Nelson Martins. "Three-phase power flow calculations using the current injection method." Power Systems, IEEE Transactions on 15, no. 2 (2000): 508-514.
  9. Chen, T-H., M-S. Chen, Toshio Inoue, Paul Kotas, and Elie A. Chebli. "Three-phase cogenerator and transformer models for distribution system analysis." Power Delivery, IEEE Transactions on 6, no. 4 (1991): 1671-1681.
  10. Elizondo M, F Tuffner, K Schneider, “Network solution for initialization and simulation of transient stability model of unbalanced microgrid,” to be submitted
  11. Stott, B.. "Power system dynamic response calculations." Proceedings of the IEEE 67, no. 2 (1979): 219-241.
  12. Fitzgerald, A.E., C. Kingsley, Jr., and S. D. Umans, _Electric Machinery_ , McGraw-Hill, Inc., New York, NY, 1990.
  13. Cheung, Chow, Rogers, and Vanfretti, Power System Toolbox, ONLINE Available: <http://www.eps.ee.kth.se/personal/vanfretti/pst/Power_System_Toolbox_Webpage/PST.html>
  14. DigSilent GmbH, DigSilent PowerFactory Software, ONLINE Available: <http://www.digsilent.de/index.php/products-powerfactory.html>
  15. IEEE Distribution System Analysis Subcommittee, IEEE 4 Node Test Feeder, ONLINE Available: <http://ewh.ieee.org/soc/pes/dsacom/testfeeders/>
  16. Lu, S., M. Elizondo, N. Samaan, K. Kalsi, E. Mayhorn, R. Diao, C. Jin, and Y. Zhang, "Control Strategies for Distributed Energy Resources to Maximize the Use of Wind Power in Rural Microgrids," in _Proceedings of the 2011 IEEE PES General Meeting_ , Detroit, MI, USA, July 24-28, 2011.


## Super-second Implementation

TODO - Incomplete - Super-second implementation details will go here - AVR and Drooping 


# Related Concepts:

  * User's manual
  * Requirements
  * Specifications
  * Grizzly (Version 2.3)
