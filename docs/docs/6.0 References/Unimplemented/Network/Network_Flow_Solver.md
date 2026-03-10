# Network

!!! warning

	This page contains features that are unfinished, were never implemented, or have since been deprecated. We preserve these pages for archival purposes, and also as a foundational resource for prospective developers who may wish to implement the same or similar feature. Many of these pages provide robust explanations of the theory behind a particular module or feature that we hope readers will find useful. 
	
	**This page does not reflect the current state of GridLAB-D™**

The **network** module implements a balanced three-phase positive sequence power flow solver using the Gauss-Seidel method (Gauss 1809), as described by Kundur (1993). 

# Balanced Three-Phase Steady-state Transmission Network Flow Solution Using Gauss-Seidel Method

The Gauss-Seidel (GS) method is a linear iterative method (as opposed to the Newton-Raphson (NR) method, which uses a quadratic iterative method). The only differences between the two methods are the rate of convergence and the robustness of convergence when the starting state is bad. Because GridLAB-D™ is a quasi-steady time-series simulation, most state changes are small and result in only one or two iterations in either method. However, the GS method can be more easily implemented using parallel processing systems (such as multicore systems). In addition, the GS method is more likely to solve for new conditions that are far from the current solution. However, the GS method can have difficulty computing flows under high-transfer conditions. 

The GS method uses an iterative approach proposed by von Seidel (1874). The GS method iterates using the first-order approximation of the Taylor expansion, and the NR method uses a second-order approximation. In the GS method the fundamental equation for the kth node can written as 

$$\frac{P_k + \jmath Q_k}{\overline V_K^*} = Y_{kk} \overline V_k + \sum\limits_{i=1,i\neq k}^n{\overline Y_{ki} \overline V_i} 
$$

The GS method is often criticized as inferior. This is categorically not true. The GS method has strengths and weaknesses when compared to the NR method. Depending on the circumstances, one method may be preferred over the other. But both, indeed all, valid methods produce the same answers. In fact, many commercial power flow solvers implement both methods concurrently, recognizing the necessity to exploit the appropriate method under any given circumstance. Hence, the implementation of the GS method does not make GridLAB-D's solution method inferior. It is simply a recognition that the circumstances of the power flow solution needed in GridLAB-D™ led to the choice of GS as the default power flow solver. Other power flow solvers can and should be implemented to address different circumstances. We encourage users and developers to consider doing so. 

**Important note** : The solution method requires that the best known voltage be used at all times. This means that each time a voltage is updated, all the branch _YV_ contributions to adjacent busses must be immediately updated. 

## Bus Solutions

The node is one of the major components of the method used for solving a power flow network. In essence, the distribution network can be seen as a series of nodes and links. A node’s primary responsibility is to act as an aggregation point for the links that are attached to it, and to update the current and voltage values that will be used in the calculations done in the links. 

Three types of nodes are defined. Nodes are simply a basic object that exports the voltages for each phase. Triplex nodes export voltages for three lines off a single split phase. All other node objects can have one or more phases defined. 

The types of nodes that are supported are the following: 

  * PQ buses are nodes that have both constant real and reactive power injections.
  * PV buses are for nodes that have constant real power injection but can control reactive power injection.
  * SWING buses are nodes that are designated to absorb the residual error and are also used for generators that can control both real and reactive power injections.
### PQ Bus

The PQ bus is the most commonly found bus type in electric network models. PQ buses are nodes where both the real power (P) and reactive power (Q) are given. In these cases, the updated voltage at a node is found from an existing (non-zero) voltage using 

$$\overline V_k \gets \frac{P_k - \jmath Q_k}{ \overline {Y}_k (\overline V_k^* - \sum\limits_{i = 1,i \neq k}^n{\overline Y_{ki} \overline V_i})}$$

where $\overline {Y}_k \gets G_k + \jmath B_k + \sum\limits_{i=1,i\neq k}^n{\overline Y_{ki}}$

### PV Bus

The PV bus is the next most common bus type in electric network model. PV buses are nodes where the real power (P) is given, but the reactive power (Q) must be determined at each iteration. In these cases, the updated voltage for a node is found by calculating (3.2) If Q exceeds the Q limits [Qmin, Qmax], then the angle is fixed to the corresponding limit and the bus is solved as a PQ bus. 

$$Q_k \gets \Im \left[ \overline V_k^* ( \overline Y_k \overline V_k + \sum\limits_{i = 1,i \neq k}^n{\overline Y_{ki} \overline V_i} ) \right]$$

### SWING Bus

The SWING bus occurs at least once in any given island of a network models. Large models may have more than one SWING bus, particularly if areas of the network are only lightly coupled by relatively high-impedance links. SWING bus nodes are nodes where both the real power (P) and reactive power (Q) must be determined each iteration. In these cases, the updated voltage for a node is found by 

$$\overline S \gets \overline V_k^* ( \overline Y_k \overline V_k + \sum\limits_{i = 1,i \neq k}^n{\overline Y_{ki} \overline V_i} )$$

where $P + \jmath Q = \overline S$

## Branch Solutions

The link object implements the general solution elements for branches using the GS method. The following sequence of operations is performed on each branch during a bottom-up sync event. 

The effective admittance Y is calculated by including half of the line charging capacitance B (Kundur 1993): 

$$\overline Y_{k_{eff}} \gets \overline Y_k + \jmath \frac{B}{2}$$

The admittance coefficient is the inverse of the transformer turns ratio $n$, if any (Kundur 1993): 

$$c \gets \frac{1}{n}$$

The effective line self-admittance is the product of the admittance coefficient and component admittance: 

$$\overline Y_c \gets c \overline Y_{k_{eff}}$$

Add the self-admittance and the shunt admittances to the busses (Kundur 1993): 

$$\begin{alignat}{2} \sum \overline Y_{from} & \gets \sum \overline Y_{from} + \overline Y_c + (c-1) \overline Y_c \\\ \sum \overline Y_{to} & \gets \sum \overline Y_{to} + \overline Y_c + (1-c) \overline Y_{eff} \\\ \end{alignat}$$

Compute the line current injections on the busses: 

$$\begin{alignat}{2} \begin{alignat}{2} \sum \overline I_{from} & \gets \sum \overline V_{to} \overline Y_c \\\ \sum \overline I_{to} & \gets \sum \overline V_{from} \overline Y_c \\\ \end{alignat} \end{alignat}$$

Add the current injections to the busses: 

$$\begin{alignat}{2} \sum \overline {YV}_{from} & \gets \sum \overline {YV}_{from} + \overline I_{from} \\\ \sum \overline {YV}_{to} & \gets \sum \overline {YV}_{to} + \overline I_{to} \\\ \end{alignat}$$

# Network module implementation

The **network** module implements the Gauss-Seidel solution method for balanced positive-sequence flow power models of transmission systems. 

The module global variables are shown in Table 1. 

##### Table 1. Network module properties  

_Property_ | _Type_  | _Default_  | _Unit_  | _Description_    
---|---|---|---|---  
acceleration_factor  | double | 1.4 | pu | The voltage update gain factor (usually between 1.4 and 1.7)   
convergence_limit  | double | 0.001 | V | The maximum allowable voltage change for iteration to halt   
mvabase  | double | 1.0 | MVA | The megaVolt-Amp basis to use in calculating powers   
kvbase  | double | 12.5 | kV | The kiloVolt basis to use calculating voltages   
model_year  | int16 | 2000 | CE | The basis year of the model   
model_case  | char8 | "S" | WSF | The basis of the case (e.g., winter, summer, fall)   
model_name  | char32 | (unnamed) | * | The name of the model   
  
## Node class

Network **node** objects represent busses in the transmission network. Three types of nodes are possible, and selecting using the **type** property: 

  1. **PQ** busses are general busses that have constant power (both real and reactive power are invariant);
  2. **PV** busses are busses that have constant real power, but reactive power must be computed; and
  3. **SWING** busses are busses for which both real and reactive power must be computed and there must be at least one **SWING** bus per network _island_.

The **node** class must clear the admittance and current injection accumulators on the pre-topdown pass and compute the new voltage on the bottom-up pass of synchronization. The voltage update pseudo code is as follows:     
    
    self-admittance <- admittance-accumulator + complex (conductance + j susceptance)
       if type is SWING 
           power <= conjugate ( conjugate voltage x ( self-admittance x voltage + current-injection-accumulator ) )
           new-voltage <= voltage
           voltage-change <= complex zero
       else if self-admittance non-zero
           if type is PV
               imaginary power <= imaginary ( voltage * ( self-admittance x voltage + current-injection-accumulator ) )
               if imaginary power less-than minimum reactive-limit
                    imaginary power <= minimum-reactive-limit
                    new-voltage <= ( conjugate power / conjugate voltage - current-injection-accumulator ) / self-admittance
               else if imaginary power greater-than maximum reactive-limit
                    imaginary power <= minimum-reactive-limit
                    new-voltage <= ( conjugate power / conjugate voltage - current-injection-accumulator ) / self-admittance
               else
                    new-voltage <= ( conjugate power / conjugate voltage - current-injection-accumulator ) / self-admittance
                    magnitude new-voltage <= magnitude voltage
               end
           end
           voltage-change <= new-voltage - voltage
           voltage <= new-voltage
           end
       else
           new-voltage <= voltage
           voltage-change <= complex zero
       end
       
       for each branch in incident-branches-list 
           current-change <= voltage-change x branch.admittance / branch.turns-ratio
           if node is branch.from-bus
               other-bus <= branch.to-bus
           else
               other-bus <= branch.from-bus
           end
           other-bus.current-injection-accumulator <+ current-change
       end
       
       if magnitude voltage-change greater-than convergence-limit
           time-step <= zero
       else
           time-step <= infinity
       end
    


## Link class

The network **link** object represents branches in the transmission network. The **link** class must compute the initial voltage estimates for the solution during the first bottom-up synchronization pass. The **link** need not update the voltage in subsequent iterations for the same time-step unless the admittance or turns-ratio has changed. Changes to the current-injection-accumulators resulting from bus voltage changes are handled by the **node'** s bottom-up pass. The code for the **link** bottom-up synchronization is as follows: 

      effective-admittance <= admittance + j line-susceptance / 2 
      admittance-contribution <= effective-admittance / turns-ratio
      from-bus.self-admittance-accumulator <+ admittance-contribution + admittance-contribution x ( 1/turns-ratio - 1 )
      from-bus.current-injection-accumulator <+ to-bus.voltage x admittance / turns-ratio
      to-bus.self-admittance-accumulator <+ admittance-contribution + effective-admittance x ( 1 - 1/turns-ratio )
      to-bus.current-injection-accumulator <+ from-bus.voltage x admittance / turns-ratio
      net-current-flow <= ( from-bus.voltage - to-bus.voltage ) x admittance / turns-ratio
    

# Derived Classes

All derived classes must update the appropriate properties of the **link** and **node** classes. The following derivations are recommended/anticipated: 

## Transformers

The **transformer** class is derived from the **link** class, with the value of **turns_ratio** being non-unitary. 

## Switches

The **switch** class is derived from the **link** class, with the value of the admittance being zero when the switch is open. Attention should be given the possibility that operating a switch can lead to islands, which would require additional **SWING** busses. 

## Generators

The **generator** class is derived from the **node** class where the real **power** is positive. If the reactive power is fixed, the **node** **type** should be **PQ** and if the reactive power is variable, it should be **PV**

## Loads

The **load** class is derived from the **node** class where the real **power** is negative. The **node** **type** should be **PQ**. 

## Others

Consideration should be given to implementing the following 

  * capacitor banks
  * phase shifters
  * high-voltage DC lines
  * relays
  * metering devices and phasor measurement units

# Model check procedure

The **network** model check procedure requires the following properties be verified. 

  1. Each link must be connected to a bus on both ends (**from** and **to**);
  2. Each link must have a non-zero admittance (i.e., is must not be normally open);
  3. Each node must have at least one incident link;
  4. Each flow area must have a **SWING** bus;
  5. Each no must be connected to a **SWING** bus or to a node that recursively connects to a **SWING** bus

If each of these can be confirmed for the entire model, then the check is successful. A warning may be emitted if a flow area has more than one **SWING** bus. 

# Model import/export

## CDF Files

The CDF import routine reads an IEEE CDF file and loads the model into GridLAB-D™. 

The IEEE CDF file format is documented at <http://www.ee.washington.edu/research/pstca/formats/cdf.txt>. Additional information and sample data files are also available at <http://www.ee.washington.edu/research/pstca/>. For more information on IEEE CDF files, see [http://www.google.com/search?hl=en&q=IEEE%20power%20flow%20file%20format%20CDF](http://www.google.com/search?hl=en&q=IEEE%20power%20flow%20file%20format%20CDF). 

# References

Gauss CF. 1809. Theoria motus corporum coelestium in sectionibus conicis solem ambientium. Perthes et Besser, Hamburg, Germany. 

Kundur, P. 1993. Power System Stability and Control. McGraw Hill, New York. 

Von Seidel, P.L. 1874. Über ein Verfahren, die Gleichungen, auf welche die Methode der kleinsten Quadrate führt, sowie lineare Gleichungen überhaupt, durch successive Annäherung aufzulösen. Abh. bayer Akad. Wiss, Germany. 


