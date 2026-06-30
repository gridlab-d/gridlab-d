# Network Flow Guide

!!! warning

	This page contains features that are unfinished, were never implemented, or have since been deprecated. We preserve these pages for archival purposes, and also as a foundational resource for prospective developers who may wish to implement the same or similar feature. Many of these pages provide robust explanations of the theory behind a particular module or feature that we hope readers will find useful. 
	
	**This page does not reflect the current state of GridLAB-D™**

The Network Module implements a balanced three-phase positive sequence power flow solver using the Gauss-Seidel method (Gauss 1809), as described by Kundur (1993). 

# Balanced Three-Phase Using Gauss-Seidel

The Gauss-Seidel (GS) method is a linear iterative method (as opposed to the Newton-Raphson (NR) method, which uses a quadratic iterative method). The only differences between the two methods are the rate of convergence and the robustness of convergence when the starting state is bad. Because GridLAB-D™ is a quasi-steady time-series simulation, most state changes are small and result in few iterations in either method. However, the GS method can be more easily implemented using parallel processing systems (such as multicore and shared-memory systems). In addition, the GS method is more likely to solve for new conditions that are far from the current solution. However, the GS method can have difficulty computing flows under high-transfer conditions. 

The GS method uses an iterative approach proposed by von Seidel (1874). The GS method iterates using the first-order approximation of the Taylor expansion, and the NR method uses a second-order approximation. In the GS method the fundamental equation for the $k$th node can written as 

$$\frac{P_k - j Q_k}{V_k^*} = Y_{kk} \bar V_k + \sum_{i=1,i\ne k}^n{Y_{ki} \bar V_i} \qquad(1) 
$$

The GS method is often criticized as inferior. This is categorically not true. The GS method has strengths and weaknesses when compared to the NR method. Depending on the circumstances, one method may be preferred over the other. But both, indeed all, valid methods produce the same answers. In fact, many commercial power flow solvers implement both methods concurrently, recognizing the necessity to exploit the appropriate method under any given circumstance. Hence, the implementation of the GS method does not make GridLAB-D™'s solution method inferior. It is simply a recognition that the circumstances of the power flow solution needed in GridLAB-D™ led to the choice of GS as the default power flow solver. Other power flow solvers can and should be implemented to address different circumstances. We encourage users and developers to consider doing so. 

# Node Solution

The node is one of the major components of the method used for solving a power flow network. In essence, the distribution network can be seen as a series of nodes and links. A node’s primary responsibility is to act as an aggregation point for the links that are attached to it, and to update the current and voltage values that will be used in the calculations done in the links. 

Three types of nodes are defined. Nodes are simply a basic object that exports the voltages for each phase. Triplex nodes export voltages for three lines off a single split phase. All other node objects can have one or more phases defined. The types of nodes that are supported are the following: 

  * PQ buses are nodes that have both constant real and reactive power injections.
  * PV buses are for nodes that have constant real power injection but can control reactive power injection.
  * SWING buses are nodes that are designated to absorb the residual error and are also used for generators that can control both real and reactive power injections.
## PQ Bus

The PQ bus is the most commonly found bus type in electric network models. PQ buses are nodes where both the real power ($P$) and reactive power ($Q$) are given. In these cases, the updated voltage at a node is found from an existing (non-zero) voltage using 

$$\bar V \gets \frac{P-\jmath Q}{\bar Y_{kk}\bar V_k^*} - \frac{1}{\bar Y_{kk}} \sum_{i=1,i\ne k}^n{\bar Y_{ki}\bar V_i} \qquad (2)$$

where 

  * $ \bar Y_{kk} \gets \sum_{i=1,i\ne k}^n{\bar Y_{ki}} + G + \jmath B$
## PV Bus

The PV bus is the next most common bus type in electric network model. PV buses are nodes where the real power ($P$) is given, but the reactive power ($Q$) must be determined at each iteration. In these cases, the updated voltage for a node is found by calculating 

$$Q_k = \Im \left [ \bar V _k^* \sum_{i=1}^n{Y_{ik} \bar V_i} \right ] \qquad(3)$$

If $Q$ exceeds the reactive power limits $[Qmin, Qmax]$, then the angle is fixed to the corresponding limit and the bus is solved as a PQ bus. 

## SWING Bus

The SWING bus occurs at least once in any given island of a network models. Large models may have more than one SWING bus, particularly if areas of the network are only lightly coupled by relatively high-impedance links. SWING bus nodes are nodes where both the real power ($P$) and reactive power ($Q$) must be determined each iteration. In these cases, the updated voltage for a node is found by 

$$\bar S \gets \bar V^* \left ( \bar Y_{kk} \bar V + \sum_{i=1,i\ne k}^n{\bar Y_{kk} \bar V_k}\right ) \qquad (4)$$

where 

  * $ P + \jmath Q \gets \bar S$
# Branch Solution

The link object implements the general solution elements for branches using the GS method. The following sequence of operations is performed on each branch during a bottom-up sync event. The effective admittance Y is calculated by including half of the line charging capacitance B (Kundur 1993): 

$$\bar Y_{eff} \gets \bar Y + \jmath \frac{B}{2} \qquad (5)$$

The admittance coefficient is the inverse of the transformer turns ratio n, if any (Kundur 1993): 

$$c \gets \frac 1 n \qquad (6)$$

The effective line self-admittance is the product of the admittance coefficient and component admittance: 

$$\bar Y_c \gets c \bar Y_{eff} \qquad (7)$$

Add the self-admittance and the shunt admittances to the busses (Kundur 1993): 

$$\Sigma \bar Y_{from} \gets \Sigma \bar Y_{from} + \bar Y_c + \bar Y_c(c-1) \qquad (8)$$

$$\Sigma \bar Y_{to} \gets \Sigma \bar Y_{to} + \bar Y_c + \bar Y_{eff}(1-c) \qquad (9)$$

  
Compute the line current injections on the busses: 

$$I_{from} \gets \bar V_{to} Y_c \qquad (10)$$

$$I_{to} \gets \bar V_{from} Y_c \qquad (11)$$

Add the current injections to the busses: 

$$\Sigma \bar {YV}_{from} \gets \Sigma \bar {YV} - \bar I_{from} \qquad (12)$$

$$\Sigma \bar {YV}_{to} \gets \Sigma \bar {YV} - \bar I_{to} \qquad (13)$$

Compute the line current (flow from toward to bus) 

$$\bar I \gets \bar I_{from} - \bar I_{to} \qquad (14)$$

# References

Gauss CF. 1809. Theoria motus corporum coelestium in sectionibus conicis solem ambientium. Perthes et Besser, Hamburg, Germany. 

Kundur, P. 1993. Power System Stability and Control. McGraw Hill, New York. von Seidel, P.L. 1874. Über ein Verfahren, die Gleichungen, auf welche die Methode der kleinsten Quadrate führt, sowie lineare Gleichungen überhaupt, durch successive Annäherung aufzulösen. Abh. bayer Akad. Wiss, Germany. 

D. P. Chassin., "GridLAB-D™ Technical Support Document: Network Module Version 1.0", PNNL-17616, Pacific Northwest National Laboratory, Richland WA, USA May 2008.


