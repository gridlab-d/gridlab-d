## Algorithm Selection

By default, the forward-back sweep algorithm is used to solve powerflow models in GridLAB-D™. To change the solver method, a `solver_method` switch must be passed in your .GLM file. When calling the powerflow module, specify `FBS` for the forward-back sweep method , `GS` for the Gauss-Seidel method, or `NR` for the Newton-Raphson method. For example, 
    
    
    module powerflow{
        solver_method GS;
    }
    

would make the powerflow module objects available to your model file and solve the resultant powerflow using the Gauss-Seidel method. 

### Forward-back sweep

The basic equations are in the following forms: 

Backward sweep: $[ I_{abc} ]_n = [ c ] \times [ V_{abc} ]_m + [ d ] \times [ I_{abc} ]_n$

Forward sweep: $[ V_{abc} ]_m = [ A ] \times [ V_{abc} ]_m + [ B ] \times [ I_{abc} ]_n$

where the $[c]$, $[d]$, $[A]$, and $[B]$ matrices represent individual characteristics of each link section as described in Kersting (2007). 

### Gauss-Seidel

The basic Gauss-Seidel equation is: 

$$V_i^{(k)} = \frac{1}{Y_{ii}} \left [ \frac{P_{i,sch} - \jmath Q_{i,sch}}{V_i^{(k-1)*}} - \sum_{j=1}^{i-1} Y_{ij} V_j^{(k)} - \sum_{j=i+1}^{N} Y_{ij} V_j^{(k-1)} \right ]$$

where 

  * $V$ is the node voltage;
  * $i$ is the current node;
  * $k$ is the current iteration number;
  * $Y_{ii}$ is the self admittance matrix;
  * $P_{i,sch}$ is the scheduled real power injection;
  * $Q_{i,sch}$ is the schedule reactive power injection; and
  * $Y_{ij}$ is the admittance of the line connecting node $i$ with the $j$node.

As a requirement of the Gauss-Seidel method, one node in the system must be designated as a swing bus. This node represents an infinite bus and provides a fixed voltage reference for Gauss-Seidel iterations. 

The Gauss-Seidel solver is an ongoing implementation. Most objects in the powerflow library are available for use. Objects that still need to be implemented include switches, relays, fuses, split-phase transformers, and regulators. 

### Newton-Raphson

Newton-Raphson is currently under development. Some features in the powerflow solver may work at this time, but the implementation is not complete. 

The Newton-Raphson solver is comprised of two principle sets of equations. The first set of equations describes the current injection from loads into the system. They are split into a real and imaginary component and are given as: 

$$\begin{matrix} \displaystyle \Delta{}I^s_{rk}=\frac{(P_k^{sp})^sV_{rk}^s+(Q_k^{sp})^sV_{mk}^s}{(V_{rk}^s)^2+(V_{mk}^s)^2}-\sum_{i=1}^n \sum_{t} \left(G_{ki}^{st}V_{r_i}^t-B_{ki}^{st}V_{m_i}^t\right)\\\ \\\ \displaystyle \Delta{}I^s_{mk}=\frac{(P_k^{sp})^sV_{mk}^s+(Q_k^{sp})^sV_{rk}^s}{(V_{rk}^s)^2+(V_{mk}^s)^2}-\sum_{i=1}^n \sum_{t} \left(G_{ki}^{st}V_{m_i}^t-B_{ki}^{st}V_{r_i}^t\right) \end{matrix}$$

where 

  * $\Delta{}I$ is the current injection at that bus
  * $k$ is the bus number
  * $s$ is the current phase of interest
  * $t$ represents all of the phases connected to the bus
  * $P$ is the real power component of a load on the bus
  * $Q$ is the reactive power component of a load on the bus
  * $E_k$ represents the voltage of the bus such that 
    * $V_{rk}$ is the real portion of the voltage
    * $V_{mk}$ is the imaginary portion of the voltage, so
    * $E_k = V_{rk} + \jmath{}V_{mk}$
  
With the current injections calculated, the voltage updates are computed via 

  
$$\begin{bmatrix} \Delta{}I_{mk} \\\ \Delta{}I_{rk} \end{bmatrix} = -\mathbf{J^{-1}} \begin{bmatrix} \Delta{}V_{mk} \\\ \Delta{}V_{rk} \end{bmatrix}$$

  
where 

$\mathbf{J^{-1}}$ represents the inverse Jacobian given by 

$$\mathbf{J} = \begin{bmatrix} \displaystyle \frac{\delta{}\Delta{}I_{mk}}{\delta{}V_{rk}} & \displaystyle \frac{\delta{}I_{mk}}{\delta{}V_{mk}}\\   
    & \\
\displaystyle \frac{\delta{}\Delta{}I_{rk}}{\delta{}V_{rk}} & \displaystyle \frac{\delta{}I_{rk}}{\delta{}V_{mk}} \end{bmatrix}$$ 

As with the Gauss-Seidel method, one node must be designated a swing or slack bus. This node will represent an infinite bus and provides the fixed voltage reference for the solver iterations. 

The Newton-Raphson solver utilizes the superLU sparse matrix solver package by default. An external matrix solving package can be utilized by following the specifications outlined on the Powerflow External LU Solver Interface page. 

The Newton-Raphson solver is currently complete, except for some specific `regulator` models. All other objects are functional at this time. 
