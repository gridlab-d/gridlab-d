## Overhead and Underground Lines

Overhead and underground (concentric neutral and tape-shield) lines are both supported. Single-phase and three-phase lines with a neutral conductor are supported. Systems with more than 3 phases, i.e. n-phase configurations, are not currently supported. Equations used are consistent with Kersting (2007). 

### Line Shunt Admittance

By default, the `powerflow` module does not model the shunt admittance of underground or overhead lines. This feature can be enabled by setting `line_capacitance` to `true`, similar to: 
    
    
    module powerflow {
    	line_capacitance true;
    	};
    

If `line_capacitance` is enabled and relevant parameters are missing from the individual lines, shunt admittance portions of those devices will be excluded. 

## Secondary Lines

Single-phase triplex secondary cable is supported. Equations used are consistent with Kersting (2007). 

## Transformers

For three-phase Delta-connected secondary transformers, the impedance within the Delta windings must be calculated. With standard transformer per unit calculations, the secondary line impedance is calculated. Equivalent impedance within the Delta is calculated by multiplying the calculated transformer secondary impedance by 3. The equations for Delta secondary in the following subsections use the impedance within the Delta. 

For single-phase transformers connected in a three-phase Delta configuration, the impedance of each individual transformer is equivalent to the impedance within the Delta. Therefore, calculation of the secondary impedance is straightforward. 

These equations assume that the transformers modeled are consistent with C57.12.00 (2006): _"The angular displacement between high-voltage and low-voltage phase voltages of three-phase transformers with Y-Δ or Δ-Y connections shall be 30°, with the low voltage lagging the high voltage..."_

Equations for Y-Y or Δ-Δ are the same for step-up or step-down transformers. Equations for step-up and step-down cases are different for the Wye-Delta and Delta-Wye connections because of the "American Standard Thirty Degree" connection, as described in Kersting (2007) and C57.12.00 (2006). 

Supported transformers and are discussed in the following subsections. 

### Delta-Grounded Wye (Step-down)

Equations for the Delta-Grounded Wye step-down transformer are consistent with Kersting (2007). 

### Delta-Grounded Wye (Step-up)

The equations for the step-up and step-down transformer are not identical because of the use of the “American Standard Thirty Degree” connection, as described in Kersting (2007) and IEEE C57.12.00 (2006). In order to obtain the equations for the step-up transformer from the step-down transformer equations the $[c]$ and $[d]$ matrices must be multiplied by: 

$$S= \begin{bmatrix}     
    0 & 0 & -1 \\
    -1 & 0 & 0 \\
    0 & -1 & 0
\end{bmatrix}$$

  
Matrix $[A]$ must be multiplied by: 

$$S^{-1}= \begin{bmatrix} 
    0 & -1 & 0 \\
    0 & 0 & -1 \\
    -1 & 0 & 0
\end{bmatrix}$$

### Ungrounded Y-Δ (Step-down)

Equations for the Ungrounded Y-Δ step-down transformer are consistent with Kersting (2007). 

### Ungrounded Y-Δ (Step-up)

The equations for the step-up and step-down transformer are not identical because of the use of the "American Standard Thirty Degree" connection, as described in Kersting (2007) and C57.12.00 (2006). In order to obtain the equations for the step-up transformer from the step-down transformer equations the _c_ and _d_ matrices must be multiplied by: 

$$S= \begin{bmatrix} 
    0 & 0 & -1 \\
    -1 & 0 & 0 \\
    0 & -1 & 0
\end{bmatrix}$$
    

  
Matrix _A_ must be multiplied by: 

$$S^{-1}= \begin{bmatrix} 
    0 & -1 & 0 \\
    0 & 0 & -1 \\
    -1 & 0 & 0
\end{bmatrix}$$
    

### Grounded Y-Grounded Y (Step-up and Step-down)

Equations for the Grounded Y-Grounded Y step-up and step-down transformers are consistent with Kersting (2007). 

### Δ-Δ (Step-up and Step-down)

Equations for the Δ-Δ step-up and step-down transformers are consistent with Kersting (2007). 

### Open Y-Open Δ (Step-down)

Equations for the Open Wye-Open Δ step-down transformers are consistent with Kersting (2007). 

### Single-Phase (Step-down)

Not yet implemented. 

### Single-Phase Center-Tapped (Step-down)

Single-phase center-tapped transformers mark the transition point between the primary and secondary distribution system. These are the transformers that step the voltage down from the primary distribution system voltage (for example, 12.47 kV) to the residential voltage (120 V and 240 V). 

Transformers are modeled using an interlaced design. The representative equations are created using the method described in Kersting (2007), with the exception that 3 x 3 matrices are in used in lieu of 2 x 2 matrices and shunt impedances have been incorporated into the equations. The specific formats are shown below. 

#### A-Phase Connected Primary

$$[a] = \left [ \begin{matrix} Z_{eq} n_t & 0 & 0 \\\ Z_{eq} n_t & 0 & 0 \\\ 0 & 0 & 0 \end{matrix} \right ]$$


$$[b] = \left [ \begin{matrix} Z_{eq} n_t Z_1 + \frac{Z_0}{n_t} & -\frac{Z_0}{n_t} & 0 \\\ \frac{Z_0}{n_t} & -\left ( Z_{eq} n_t Z_2 + \frac{Z_0}{n_t} \right ) & 0 \\\ 0 & 0 & 0 \end{matrix} \right ]$$

$$[c] = \left [ \begin{matrix} \frac{n_t}{Z_c} & 0 & 0 \\\ 0 & 0 & 0 \\\ 0 & 0 & 0 \end{matrix} \right ]$$


$$[d] = \left [ \begin{matrix} \frac{Z_1 n_t}{Z_c} + \frac{1}{n_t} & \frac{-1}{n_t} & 0 \\\ 0 & 0 & 0 \\\ 0 & 0 & 0 \end{matrix} \right ]$$


$$[A] = \left [ \begin{matrix} \frac{1}{n_t} & 0 & 0 \\\ \frac{1}{n_t} & 0 & 0 \\\ 0 & 0 & 0 \end{matrix} \right ]$$

$$[B] = \left [ \begin{matrix} Z_1 + \frac{Z_0}{Z_{eq} n_t^2} & -\frac{Z_0}{Z_{eq} n_t^2} & 0 \\\ \frac{Z_0}{Z_{eq} n_t^2} & -\left ( Z_2 + \frac{Z_0}{Z_{eq} n_t^2} \right ) & 0 \\\ 0 & 0 & 0 \end{matrix} \right ]$$

#### B-Phase Connected Primary

$$[a] = \left [ \begin{matrix} 0 & Z_{eq} n_t & 0 \\\ 0 & Z_{eq} n_t & 0 \\\ 0 & 0 & 0 \end{matrix} \right ]$$


$$[b] = \left [ \begin{matrix} Z_{eq} n_t Z_1 + \frac{Z_0}{n_t} & -\frac{Z_0}{n_t} & 0 \\\ \frac{Z_0}{n_t} & -\left ( Z_{eq} n_t Z_2 + \frac{Z_0}{n_t} \right ) & 0 \\\ 0 & 0 & 0 \end{matrix} \right ]$$

$$[c] = \left [ \begin{matrix} 0 & 0 & 0 \\\ \frac{n_t}{Z_c} & 0 & 0 \\\ 0 & 0 & 0 \end{matrix} \right ]$$


$$[d] = \left [ \begin{matrix} 0 & 0 & 0 \\\ \frac{Z_1 n_t}{Z_c} + \frac{1}{n_t} & \frac{-1}{n_t} & 0 \\\ 0 & 0 & 0 \end{matrix} \right ]$$


$$[A] = \left [ \begin{matrix} 0 & \frac{1}{n_t} & 0 \\\ 0 & \frac{1}{n_t} & 0 \\\ 0 & 0 & 0 \end{matrix} \right ]$$


$$[B] = \left [ \begin{matrix} Z_1 + \frac{Z_0}{Z_{eq} n_t^2} & -\frac{Z_0}{Z_{eq} n_t^2} & 0 \\\ \frac{Z_0}{Z_{eq} n_t^2} & -\left ( Z_2 + \frac{Z_0}{Z_{eq} n_t^2} \right ) & 0 \\\ 0 & 0 & 0 \end{matrix} \right ]$$

#### C-Phase Connected Primary

$$[a] = \left [ \begin{matrix} 0 & 0 & Z_{eq} n_t \\\ 0 & 0 & Z_{eq} n_t \\\ 0 & 0 & 0 \end{matrix} \right ]$$


$$[b] = \left [ \begin{matrix} Z_{eq} n_t Z_1 + \frac{Z_0}{n_t} & -\frac{Z_0}{n_t} & 0 \\\ \frac{Z_0}{n_t} & -\left ( Z_{eq} n_t Z_2 + \frac{Z_0}{n_t} \right ) & 0 \\\ 0 & 0 & 0 \end{matrix} \right ]$$


$$[c] = \left [ \begin{matrix} 0 & 0 & 0 \\\ 0 & 0 & 0 \\\ \frac{n_t}{Z_c} & 0 & 0 \end{matrix} \right ]$$

  
$$[d] = \left [ \begin{matrix} 0 & 0 & 0 \\\ 0 & 0 & 0 \\\ \frac{Z_1 n_t}{Z_c} + \frac{1}{n_t} & \frac{-1}{n_t} & 0 \end{matrix} \right ]$$

  
$$[A] = \left [ \begin{matrix} 0 & 0 & \frac{1}{n_t} \\\ 0 & 0 & \frac{1}{n_t} \\\ 0 & 0 & 0 \end{matrix} \right ]$$


$$[B] = \left [ \begin{matrix} Z_1 + \frac{Z_0}{Z_{eq} n_t^2} & -\frac{Z_0}{Z_{eq} n_t^2} & 0 \\\ \frac{Z_0}{Z_{eq} n_t^2} & -\left ( Z_2 + \frac{Z_0}{Z_{eq} n_t^2} \right ) & 0 \\\ 0 & 0 & 0 \end{matrix} \right ]$$

where $Z_{eq} = \frac{Z_0+Z_c}{Z_c}$

and 

$Z_c = \frac{R_c jX_c}{R_c + jX_c}$ is the core shunt impedance. 

Voltage and current values used for the secondary system sweeps are shown below. 

Backward sweep: $[ I_{abc} ] = [c] \times [V_{12n}] + [d] \times [I_{12n}]$

Forward sweep: $V_{12n} = [A] \times [V_{abc}] - [B] \times [I_{12n}]$

where 

  * $I_{abc}$ is the primary current on lines _a_ , _b_ , and _c_
  * $I_{12n}$ is secondary current on lines 1, 2, and _n_
  * $V_{abc}$ is primary voltage on lines _a_ , _b_ , and _c_
  * $V_{12n}$ is secondary voltage on lines 1, 2, and _n_
