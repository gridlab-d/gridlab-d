# Neutral Earth Voltage (NEV)

Implement neutral earth voltage and generic capabilities into the powerflow module. 

This work will incorporate the ability to model neutral-earth voltages and other more advanced powerflow capabilities into GridLAB-D. Expected release is in Keeler (Version 4.0). 

# Classes

* Overhead Line Equations
* Underground Line Equations
* Load Equations
* Transformer Equations

## Protection Devices

Protection devices, i.e. circuit breakers, reclosers, sectionalizers, fuses, and switches, are used to protect the system from dangerously high currents flowing through the system. They are used to break the connection typically on the 3 phases of a distribution line. So while these devices look like a link for load carrying phases of the system they don't necessarily break a neutral line. Some devices do require a ground connection for the electronics contained within however this is a single contact not a link type connect where there is a ground at one end of the device and an ground at the other end of the device. When these devices are closed the impedance across them is very negligible so any cross coupling between phases can be ignored. When these devices are open they are open circuits for the phases they are connected to. 

# NEV Loads


All loads will be described in terms of ZIP fractions and represented by voltage and current injections into the powerflow solution. The loads will be connected in a nodal manner so that any combination of wye and delta connected loads can be attached to a single bus. 

## Parameters

The parameters required to determine the current injections due to a load connected to nodes $n$ and $m$ of a bus are listed below. 

  * $|S_{b,nm}|$ The magnitude of the base absolute power of the load connected to nodes $n$ and $m$ of a particular bus (VA) at $V_{b,nm}$.
  * $V_{nm}$ The variable voltage difference between the node to ground voltages of nodes $n$ and $m$ of a particular bus (V).
  * $|V_{b,nm}|$ The magnitude of the base voltage difference between the node to ground voltages of nodes $n$ and $m$ of a particular bus (V).
  * $pfr_{p,nm}$ The constant power fraction of the load connected to nodes $n$ and $m$ of a particular bus (unitless).
  * $pf_{p,nm}$ The power factor of the constant power fraction of the load connected to nodes $n$ and $m$ of a particular bus (-1.0 - 1.0).
  * $pfr_{i,nm}$ The constant current fraction of the load connected to nodes $n$ and $m$ of a particular bus (unitless) at $V_{b,nm}$.
  * $pf_{i,nm}$ The power factor of the constant current fraction of the load connected to nodes $n$ and $m$ of a particular bus (-1.0 - 1.0) at $V_{b,nm}$.
  * $pfr_{z,nm}$ The constant impedance fraction of the load connected to nodes $n$ and $m$ of a particular bus (unitless) at $V_{b,nm}$.
  * $pf_{z,nm}$ The power factor of the constant impedance fraction of the load connected to nodes $n$ and $m$ of a particular bus (-1.0 - 1.0) at $V_{b,nm}$.

## Equations

For the parameters described above the current injections due to each type of load fraction can be found below. 

### Constant Impedance Loads

The constant impedance of the load can be found by the following equations 

$$ \displaystyle{}S_{z,nm}=S_{b,nm}*pfr_{z,nm}*[pf_{z,nm}+j*sign(pf_{z,nm})*(1-pf_{z,nm}^{2})^{\frac{1}{2}}]$$
$$ \displaystyle{}Z_{nm}=\frac{|V_{b,nm}|^{2}}{S_{z,nm}^{*}}$$

The current injection due to the constant impedance can then be found from the equation below. 

$$ \displaystyle{}I_{z,nm}=\frac{V_{nm}}{Z_{nm}}$$

### Constant Current Loads

The constant current of the load can be found by the following equations 

$$ \displaystyle{}S_{i,nm}=S_{b,nm}*pfr_{i,nm}*[pf_{i,nm}+j*sign(pf_{i,nm})*(1-pf_{i,nm}^{2})^{\frac{1}{2}}]$$
$$ \displaystyle{}I_{nm}=(\frac{S_{i,nm}}{|V_{b,nm}|})^{*}$$

The current injection due to the constant impedance can then be found from the equation below. 

$$ \displaystyle{}I_{i,nm}=I_{nm}$$

### Constant Power Loads

The constant Power of the load can be found by the following equation 

$$ \displaystyle{}S_{p,nm}=S_{b,nm}*pfr_{p,nm}*[pf_{p,nm}+j*sign(pf_{p,nm})*(1-pf_{p,nm}^{2})^{\frac{1}{2}}]$$

The current injection due to the constant impedance can then be found from the equation below. 

$$ \displaystyle{}I_{p,nm}=(\frac{S_{p,nm}}{V_{nm}})^{*}$$

### Total Current Injection

The total current injection into a node $n$ due to loads connecting node $n$ to any number of other nodes on the same bus can be found by the following equation 

$$ \displaystyle{}I_{n}=\Sigma (I_{z,nj}+I_{i,nj}+I_{p,nj})$$

where $j$ are the nodes the loads connect node $n$ to on a bus. 


# NEV Overhead
**Work in progress/unfinished**

## Assumptions
Assumes ground is infinite plane, zero potential plane 

## Variables

Table 1 - Equation Notation  Variable | Definition   
---|---  
$\displaystyle{}V_{i_{mg}}$ | Voltage at node i, phase m relative to true ground (V)   
$\displaystyle{}I_{i_{mg}}$ | Voltage at node i, phase m relative to true ground (A)   
$\displaystyle{}\hat z_{i-j_{nn}}$ | Element of series impedance matrix relating voltage/current relationship for line connecting nodes i and j, corresponding to self impedance of phase n. Return path (ground) impedance folded in ($\Omega /mile$)   
$\displaystyle{}\hat z_{i-j_{nm}}$ | Element of series impedance matrix relating voltage/current relationship for line connecting nodes i and j, corresponding to phases n and m. Return path (ground) impedance folded in ($\Omega /mile$)   
$\displaystyle{}r_i$ | Resistance of conductor i ($\Omega /mile$)   
$\displaystyle{}\omega$ | System angular frequency ($rad/s$)   
$\displaystyle{}f$ | System frequency ($Hz$)   
$\displaystyle{}G=0.1609347\times 10^{-3}$ | Constant for converting from CGS units ($\Omega /mile$)   
$\displaystyle{}RD_i$ | Radius of conductor i ($ft$)   
$\displaystyle{}GMR_i$ | Geometric mean radius of conductor i ($ft$)   
$\displaystyle{}D_{ij}$ | Distance between conductors i and j ($ft$)   
$\displaystyle{}S_{ij}$ | Distance between conductor i and conductor j's image ($ft$)   
$\displaystyle{}\theta_{ij}$ | Angle between a pair of lines drawn from conductor i to its own image and to the image of conductor j ($rad$)   
$\displaystyle{}\rho$ | Resistivity of earth ($\Omega -meters$)   
$\displaystyle{}\epsilon _{air}$ | Relative permittivity of air = $1.4240\times 10^{-2} (\mu F/mile)$  
  
## Overhead Distribution Lines

Overhead lines will be modeled in a flexible way such that not only neutral conductors can be included but also any number of phases and/or circuits. 

### Series Impedance

Initially neglecting shunt admittances, the voltage/current relationship between two 'nodes' 1 and 2 (corresponding to physical terminal locations in the network) with $m$ phases can be expressed in matrix form: 

$$
\begin{bmatrix}
V_{1_{ag}} \\
\downarrow \\
V_{1_{mg}}
\end{bmatrix}
-
\begin{bmatrix}
V_{2_{ag}} \\
\downarrow \\
V_{2_{mg}}
\end{bmatrix}
=
\begin{bmatrix}
\hat{z}_{1-2_{aa}} & \hat{z}_{1-2_{am}} \\
\downarrow & \downarrow  \\
\hat{z}_{1-2_{ma}} & \hat{z}_{1-2_{mm}}
\end{bmatrix}
\begin{bmatrix}
I_{1-2_{ag}} \\
\downarrow \\
I_{1-2_{mg}}
\end{bmatrix}
$$

For example, consider a distribution line with two electrically isolated feeders sharing one neutral phase. One of the feeders has all three phases $(a, b, c)$ present, while the other has only two phases $(a, c)$ present. For clarity, the two phases on the second feeder are renamed $(d, e)$. Then, the voltage drop on the line can be expressed by: 

$$
\begin{bmatrix}
V_{1_{ag}} \\
V_{1_{bg}} \\
V_{1_{cg}} \\
V_{1_{dg}} \\
V_{1_{eg}} \\
V_{1_{ng}}
\end{bmatrix}
- 
\begin{bmatrix}
V_{2_{ag}} \\
V_{2_{bg}} \\
V_{2_{cg}} \\
V_{2_{dg}} \\
V_{2_{eg}} \\
V_{2_{ng}}
\end{bmatrix}
=
\begin{bmatrix}
\hat{z}_{1-2_{aa}} & \hat{z}_{1-2_{ab}} & \hat{z}_{1-2_{ac}} & \hat{z}_{1-2_{ad}} & \hat{z}_{1-2_{ae}} & \hat{z}_{1-2_{an}} \\
\hat{z}_{1-2_{ba}} & \hat{z}_{1-2_{bb}} & \hat{z}_{1-2_{bc}} & \hat{z}_{1-2_{bd}} & \hat{z}_{1-2_{be}} & \hat{z}_{1-2_{bn}} \\
\hat{z}_{1-2_{ca}} & \hat{z}_{1-2_{cb}} & \hat{z}_{1-2_{cc}} & \hat{z}_{1-2_{cd}} & \hat{z}_{1-2_{ce}} & \hat{z}_{1-2_{cn}} \\
\hat{z}_{1-2_{da}} & \hat{z}_{1-2_{db}} & \hat{z}_{1-2_{dc}} & \hat{z}_{1-2_{dd}} & \hat{z}_{1-2_{de}} & \hat{z}_{1-2_{dn}} \\
\hat{z}_{1-2_{ea}} & \hat{z}_{1-2_{eb}} & \hat{z}_{1-2_{ec}} & \hat{z}_{1-2_{ed}} & \hat{z}_{1-2_{ee}} & \hat{z}_{1-2_{en}} \\
\hat{z}_{1-2_{na}} & \hat{z}_{1-2_{nb}} & \hat{z}_{1-2_{nc}} & \hat{z}_{1-2_{nd}} & \hat{z}_{1-2_{ne}} & \hat{z}_{1-2_{nn}}
\end{bmatrix}
\begin{bmatrix}
I_{1-2_{ag}} \\
I_{1-2_{bg}} \\
I_{1-2_{cg}} \\
I_{1-2_{dg}} \\
I_{1-2_{eg}} \\
I_{1-2_{ng}}
\end{bmatrix}
$$

The hat notation, taken from Kersting, indicates that the return path, i.e. the ground impedance, has been folded into the other impedances. According to Carson's equations, the elements of the primitive impedance matrix can be calculated by: 

$$ \displaystyle{}\hat z_{1-2_{ii}}=r_i+4\omega P_{ii}G + j\left(X_i + 2\omega G\ln{\frac{S_{ii}}{RD_i}} + 4\omega Q_{ii}G\right)\Omega /mi$$

$$ \displaystyle{}\hat z_{1-2_{ij}}=4\omega P_{ii}G + j\left(2\omega G\ln{\frac{S_{ij}}{D_{ij}}} + 4\omega Q_{ij}G\right)\Omega /mi$$

Wherein: 

$$ X_i = 2\omega{}G\ln{\frac{RD_i}{GMR_{i}}}\Omega /mi$$

$$ P_{ij} = \frac{\pi}{8}-\frac{1}{3\sqrt{2}}k_{ij}\cos{\theta_{ij}}+\frac{k_{ij}^2}{16}\cos{2\theta_{ij}}\left(0.6728+\ln{2}{k_{ij}}\right)+\frac{k_{ij}^2}{16}\theta_{ij}\sin{2\theta_{ij}}$$

$$ Q_{ij} = -0.0386 + \frac{1}{2}\ln{\frac{2}{k_{ij}}}+\frac{1}{3\sqrt{2}}k_{ij}\cos{\theta_{ij}}$$

$$ k_{ij} = 8.565\times 10^{-4}S_{ij}\sqrt{\frac{f}{\rho}}$$

The primitive series impedance matrix can be inverted to yield the primitive series admittance matrix for the line. For a more complete model of the line, the primitive shunt admittance matrix can then be added. 

### Shunt Admittance

The method in Kersting uses Carson's equations to construct shunt admittance matrices for overhead lines. Carson's equations make use of conductor 'images' which are modeled as being parallel underground conductors parallel to each conductor, equidistant from the ground surface. The primitive potential coefficient matrix $P$ is assembled such that for each element: 

$$ \displaystyle{}P_{ii}= \frac{1}{2\pi\epsilon_{air}}\ln{\frac{S_{ii}}{RD_{i}}}mile/\mu F$$

$$ \displaystyle{}P_{ij}= \frac{1}{2\pi\epsilon_{air}}\ln{\frac{S_{ij}}{D_{ij}}}mile/\mu F$$

Having constructed matrix $P$, the primitive shunt admittance matrix can then be constructed simply by: 

$$ \displaystyle{}Y_{shunt}=j\omega 10^{-6}P^{-1} Siemens/mi$$

# NEV Transformer

The approach described by Roger Dugan and used in Open DSS has been adopted to model transformers and regulators. 

## Assumptions

Assumes ground is infinite plane, zero potential plane 

## Variables

Table 1 - Equation Notation  Variable | Definition   
---|---  
$\displaystyle{}z_{ij_{(p)}}$ | Short circuit impedance between windings i and j, phase p (%)   
$\displaystyle{}z_{base}$ | Impedance base to convert to a 1V base, equal to $\frac{1}{S_{base}}$  
$\displaystyle{}z_{ij_{(p)}}^{1V}$ | Short circuit impedance between windings i and j, phase p, on a 1V base ($\Omega$)   
$\displaystyle{}S_{base_{i}}$ | VA base for winding i (VA)   
$\displaystyle{}V_{rated,i}$ | Voltage rating for winding i (V)   
$\displaystyle{}\mathbf{Z_B}$ | Short circuit impedance matrix   
$\displaystyle{}\mathbf{B}$ | Incidence matrix relating short circuit currents   
$\displaystyle{}\mathbf{N}$ | Incidence matrix relating short circuit currents to terminal currents and expressing voltage ratios   
$\displaystyle{}\mathbf{A}$ | Incidence matrix relating terminal connections   
$\displaystyle{}a$ | Tap setting coefficient for regulator/tap-changing transformer   
$\displaystyle{}n_{taps}$ | Tap setting for regulator/tap-changing transformer   
$\displaystyle{}w_{taps}$ | Percent change in voltage per tap change for regulator/tap-changing transformer   
  
## Series Impedance

### Generic Approach

In [1], a method is presented which claims to be generic/applicable for modeling of all (n-winding) transformer configurations, including split-phase (4-wire) types. The result of this method is a primitive nodal admittance matrix which can be used/inserted in the system bus admittance matrix. The process is outlined as follows. 

First, a short-circuit impedance matrix, $\mathbf{Z_B}$, is constructed for the transformer. $\mathbf{Z_B}$ is of order $p(n-1)$ where $p$ is the number of phases and n is the number of windings. The values in $\mathbf{Z_B}$ are all relative to one of the windings, which is why for a common two-winding transformer, there is only one submatrix. The elements for the diagonal submatrices are calculated by: 

$\displaystyle{}z_{B_{ii,kk}}=z_{SC_{i,i+1}} \cdot z_{base}$ for $i=1$ to $m-1$

where '$ii$' indicates the diagonal submatrix corresponding to winding $i$, '$kk$' indicates the element corresponding to phase $k$, $z_{base}$ is a multiplier to convert the impedance to a 1V base, and $m$ is the number of phases. The off diagonal submatrices (present when there are more than two windings) are calculated by: 

$\displaystyle{}z_{B_{ij}}=\frac{1}{2}\left [z_{B_{ii}}+z_{B_{jj}}-z_{i+1,j+1}\right ]$

The primitive nodal admittance matrix is then constructed by: 

$$\displaystyle{}\mathbf{Y_{prim}}=\mathbf{ANBZ_B}^{-1}\mathbf{B}^T\mathbf{N}^T\mathbf{A}^T$$

In the above equation, $\mathbf{B}$ is an incidence matrix relating currents in the short circuit reference frame, $\mathbf{N}$ is an incidence matrix relating the short circuit currents to the winding currents and whose non-zero elements are the corresponding voltage rating, and lastly $\mathbf{A}$ is an incidence matrix relating winding currents to actual terminal currents. 

Note that for voltage regulators with load tap changers, the only difference is in the elements of the $\mathbf{N}$ matrix. For source side windings, the elements of the N matrix are simply the reciprocal of the rated voltage: 

$$\displaystyle{}n_{ii}=\frac{1}{V_{rated,i}}$$

For load side windings, the elements of the N matrix can be calculated as: 

$$\displaystyle{}n_{jj}=\frac{1}{aV_{rated,i}}$$

The coefficient $a$ represents the tap setting: 

$$\displaystyle{}a=1+n_{taps}w_{taps}$$

where $n_{taps}$ is the tap number and $w_{taps}$ is the percent voltage change of one tap (usually equal to 0.625%). 

### Shunt Admittance

If they are to be included, the shunt admittances can be added directly to the primitive nodal admittance matrix. 

## Examples

The first example will be covered in detail, where others will merely give the relevant matrices. 

### Three-Phase Two Winding Grounded Wye to Delta Transformer

This example will walk through the process of constructing a primitive admittance matrix for a three-phase two winding grounded wye to delta transformer with the following specs: Winding Line-to-Line Voltages: 12.47 kV (primary); 4.16 kV (secondary) Three-phase power rating: 6000 kVA Per-Phase Percent Short Circuit Impedance: $1 + j6$ The first step is to calculate/assemble the short circuit impedance matrix, $\mathbf{Z_B}$. Since it is a two-winding transformer, there will be $(2-1) = 1$ submatrices. This means there will be no 'off-diagonal' submatrices. Since there are three phases, each submatrix will be of order 3. The neutral terminal is not yet included here since it does not have a transformer winding. Given short circuit impedances for each phase/winding pair, $z_{HL_a},z_{HL_b},z_{HL_c}$, the short circuit impedance matrix can be stated: 

$$\mathbf{Z_B} =
\begin{bmatrix}
z_{HL_a}^{1V} & 0 & 0 \\
0 & z_{HL_b}^{1V} & 0 \\
0 & 0 & z_{HL_c}^{1V}
\end{bmatrix}
=
\begin{bmatrix}
\frac{z_{HL_a}}{z_{base}} & 0 & 0 \\
0 & \frac{z_{HL_b}}{z_{base}} & 0 \\
0 & 0 & \frac{z_{HL_c}}{z_{base}}
\end{bmatrix}
=
\begin{bmatrix}
\frac{0.01 + j0.06}{2.0 \times 10^6} & 0 & 0 \\
0 & \frac{0.01 + j0.06}{2.0 \times 10^6} & 0 \\
0 & 0 & \frac{0.01 + j0.06}{2.0 \times 10^6}
\end{bmatrix}$$

The next step is constructing the $B$ matrix, which relates the short circuit currents to the terminal currents which are defined as entering the transformer: 

$$\begin{bmatrix}
I_{H_a}^{1V} \\
I_{L_a}^{1V} \\
I_{H_b}^{1V} \\
I_{L_b}^{1V} \\
I_{H_c}^{1V} \\
I_{L_c}^{1V}
\end{bmatrix}
=
\mathbf{B}
\begin{bmatrix}
I_{SC_a} \\
I_{SC_b} \\
I_{SC_c}
\end{bmatrix}$$

$$\mathbf{B} =
\begin{bmatrix}
1 & 0 & 0 \\
-1 & 0 & 0 \\
0 & 1 & 0 \\
0 & -1 & 0 \\
0 & 0 & 1 \\
0 & 0 & -1
\end{bmatrix}$$

Next, the $N$ matrix is constructed which will apply the turns ratio multipliers to the ground-referenced 1V base quantities. Each phase-submatrix of $N$ will be a 4x2 matrix, since the (2) high and low ground-referenced currents must be translated into (4) currents for both ends of both the high and low windings: 

$$\begin{bmatrix}
I_{H_{a1}} \\
I_{H_{a2}} \\
I_{L_{a1}} \\
I_{L_{a2}} \\
I_{H_{b1}} \\
I_{H_{b2}} \\
I_{L_{b1}} \\
I_{L_{b2}} \\
I_{H_{c1}} \\
I_{H_{c2}} \\
I_{L_{c1}} \\
I_{L_{c2}}
\end{bmatrix}
=
\mathbf{N}
\begin{bmatrix}
I_{H_a}^{1V} \\
I_{L_a}^{1V} \\
I_{H_b}^{1V} \\
I_{L_b}^{1V} \\
I_{H_c}^{1V} \\
I_{L_c}^{1V}
\end{bmatrix}$$

$$\mathbf{N} =
\begin{bmatrix}
\frac{1}{7200} & 0 & 0 & 0 & 0 & 0 \\
-\frac{1}{7200} & 0 & 0 & 0 & 0 & 0 \\
0 & \frac{1}{4160} & 0 & 0 & 0 & 0 \\
0 & -\frac{1}{4160} & 0 & 0 & 0 & 0 \\
\cdots & \cdots & \cdots & \cdots & \cdots & \cdots \\
0 & 0 & 0 & \frac{1}{7200} & 0 & 0 \\
0 & 0 & 0 & -\frac{1}{7200} & 0 & 0 \\
0 & 0 & 0 & 0 & \frac{1}{4160} & 0 \\
0 & 0 & 0 & 0 & -\frac{1}{4160} & 0 \\
\cdots & \cdots & \cdots & \cdots & \cdots & \cdots \\
0 & 0 & 0 & 0 & 0 & \frac{1}{7200} \\
0 & 0 & 0 & 0 & 0 & -\frac{1}{7200} \\
0 & 0 & 0 & 0 & 0 & \frac{1}{4160} \\
0 & 0 & 0 & 0 & 0 & -\frac{1}{4160}
\end{bmatrix}$$

Note that line-to-line voltage ratings have been converted to line-to-ground values. Finally, the $\mathbf{A}$ matrix is constructed to represent the transformer terminal connections (e.g. wye/delta configuration). At this point the neutral conductor/terminal can be included. Here, again the currents are defined as entering the transformer: 

$$\begin{bmatrix}
I_{H_a} \\
I_{H_b} \\
I_{H_c} \\
I_{H_n} \\
I_{L_a} \\
I_{L_b} \\
I_{L_c}
\end{bmatrix}
=
\mathbf{A}
\begin{bmatrix}
I_{H_{a1}} \\
I_{H_{a2}} \\
I_{L_{a1}} \\
I_{L_{a2}} \\
I_{H_{b1}} \\
I_{H_{b2}} \\
I_{L_{b1}} \\
I_{L_{b2}} \\
I_{H_{c1}} \\
I_{H_{c2}} \\
I_{L_{c1}} \\
I_{L_{c2}}
\end{bmatrix}
=
\left[
\begin{array}{cccccccccccc}
1 & 0 & 0 & 0 & 0 & 0 & 0 & 0 & 0 & 0 & 0 & 0 \\
0 & 0 & 0 & 0 & 1 & 0 & 0 & 0 & 0 & 0 & 0 & 0 \\
0 & 0 & 0 & 0 & 0 & 0 & 0 & 0 & 1 & 0 & 0 & 0 \\
0 & 1 & 0 & 0 & 0 & 1 & 0 & 0 & 0 & 0 & 0 & 0 \\
0 & 0 & 1 & 0 & 0 & 0 & 1 & 0 & 0 & 0 & 0 & 0 \\
0 & 0 & 0 & 1 & 0 & 0 & 0 & 1 & 0 & 0 & 0 & 0 \\
0 & 0 & 0 & 0 & 0 & 0 & 0 & 0 & 0 & 1 & 0 & 1
\end{array}
\right]
\begin{bmatrix}
I_{H_{a1}} \\
I_{H_{a2}} \\
I_{L_{a1}} \\
I_{L_{a2}} \\
I_{H_{b1}} \\
I_{H_{b2}} \\
I_{L_{b1}} \\
I_{L_{b2}} \\
I_{H_{c1}} \\
I_{H_{c2}} \\
I_{L_{c1}} \\
I_{L_{c2}}
\end{bmatrix}$$

Multiplying the assembled matrices yields the primitive nodal admittance matrix for the transformer: 

$$\displaystyle{}\mathbf{Y_{prim}}=\mathbf{ANBZ_B}^{-1}\mathbf{B}^T\mathbf{N}^T\mathbf{A}^T=$$

$$\displaystyle{} \begin{bmatrix}     
    0.1043-j0.6256& 0             & 0             &-0.1043+j0.6256&-0.1805+j1.0828& 0.1805-j1.0828& 0\\
    0             & 0.1043-j0.6256& 0             &-0.1043+j0.6256& 0             &-0.1805+j1.0828& 0.1805-j1.0828\\
    0             & 0             & 0.1043-j0.6256&-0.1043+j0.6256& 0.1805-j1.0828& 0             &-0.1805+j1.0828\\
    
-0.1043+j0.6256&-0.1043+j0.6256&-0.1043+j0.6256& 0.3128-j1.8769& 0 & 0 & 0\\\ -0.1805+j1.0828& 0 & 0.1805-j1.0828& 0 & 0.6247-j3.7482&-0.3124+j1.8741&-0.3124+j1.8741\\\ 
    
    0.1805-j1.0828&-0.1805+j1.0828& 0             & 0             &-0.3124+j1.8741& 0.6247-j3.7482&-0.3124+j1.8741\\
    0             & 0.1805-j1.0828&-0.1805+j1.0828& 0             &-0.3124+j1.8741&-0.3124+j1.8741& 0.6247-j3.7482
    \end{bmatrix}$$

### Three-Phase Two Winding Grounded Wye to Grounded Wye Transformer

This example is for the same transformer as above, but with both sides connected in grounded wye. Therefore, only the A matrix is altered: 

$$\begin{bmatrix}
I_{H_a} \\
I_{H_b} \\
I_{H_c} \\
I_{H_n} \\
I_{L_a} \\
I_{L_b} \\
I_{L_c} \\
I_{L_n}
\end{bmatrix}
=
\mathbf{A}
\begin{bmatrix}
I_{H_{a1}} \\
I_{H_{a2}} \\
I_{L_{a1}} \\
I_{L_{a2}} \\
I_{H_{b1}} \\
I_{H_{b2}} \\
I_{L_{b1}} \\
I_{L_{b2}} \\
I_{H_{c1}} \\
I_{H_{c2}} \\
I_{L_{c1}} \\
I_{L_{c2}}
\end{bmatrix}
=
\left[
\begin{array}{cccccccccccc}
1 & 0 & 0 & 0 & 0 & 0 & 0 & 0 & 0 & 0 & 0 & 0 \\
0 & 0 & 0 & 0 & 1 & 0 & 0 & 0 & 0 & 0 & 0 & 0 \\
0 & 0 & 0 & 0 & 0 & 0 & 0 & 0 & 1 & 0 & 0 & 0 \\
0 & 0 & 0 & 0 & 0 & 1 & 0 & 0 & 0 & 1 & 0 & 0 \\
0 & 0 & 1 & 0 & 0 & 0 & 1 & 0 & 0 & 0 & 0 & 0 \\
0 & 0 & 0 & 1 & 0 & 0 & 0 & 1 & 0 & 0 & 0 & 0 \\
0 & 0 & 0 & 0 & 0 & 0 & 0 & 0 & 0 & 1 & 0 & 0 \\
0 & 0 & 0 & 0 & 0 & 0 & 0 & 0 & 0 & 0 & 1 & 1
\end{array}
\right]
\begin{bmatrix}
I_{H_{a1}} \\
I_{H_{a2}} \\
I_{L_{a1}} \\
I_{L_{a2}} \\
I_{H_{b1}} \\
I_{H_{b2}} \\
I_{L_{b1}} \\
I_{L_{b2}} \\
I_{H_{c1}} \\
I_{H_{c2}} \\
I_{L_{c1}} \\
I_{L_{c2}}
\end{bmatrix}$$

$\displaystyle{}\mathbf{Y_{prim}}=\mathbf{ANBZ_B}^{-1}\mathbf{B}^T\mathbf{N}^T\mathbf{A}^T=$

$$\displaystyle{} \begin{bmatrix} 
    
    0.1043-j0.6256& 0             & 0             &-0.1043+j0.6256&-0.1805+j1.0828& 0             & 0             & 0.1805-j1.0828\\
    0             & 0.1043-j0.6256& 0             &-0.1043+j0.6256& 0             &-0.1805+j1.0828& 0             & 0.1805-j1.0828\\
    0             & 0             & 0.1043-j0.6256&-0.1043+j0.6256& 0             & 0             &-0.1805+j1.0828& 0.1805-j1.0828\\
    

-0.1043+j0.6256&-0.1043+j0.6256&-0.1043+j0.6256& 0.3128-j1.8769& 0.1805-j1.0828& 0.1805-j1.0828& 0.1805-j1.0828&-0.5414+j3.2484\\\ -0.1805+j1.0828& 0 & 0 & 0.1805-j1.0828& 0.3124-j1.8741& 0 & 0 &-0.3124+j1.8741\\\ 
    
    
    0             &-0.1805+j1.0828& 0             & 0.1805-j1.0828& 0             & 0.3124-j1.8741& 0             &-0.3124+j1.8741\\
    0             & 0             & 0.1805-j1.0828& 0.1805-j1.0828& 0             & 0             & 0.3124-j1.8741&-0.3124+j1.8741\\
    0.1805-j1.0828& 0.1805-j1.0828& 0.1805-j1.0828&-0.5414+j3.2484&-0.3124+j1.8741&-0.3124+j1.8741&-0.3124+j1.8741& 0.9371-j5.6223
    \end{bmatrix}$$

### Single-Phase Line-to-Ground to Split Phase (Triplex) Transformer

The important modeling consideration with this transformer is that there are effectively three windings. The high side winding will be denoted winding H, and the two split phases 'L' and 'T'. The transformer data is given here: Winding Line-to-Ground Voltages: 7.2 kV (primary); 0.12 kV (each secondary) Power rating: 25 kVA Percent Short Circuit Impedance: $0 + 2.04j$ (winding H to winding L); $0 + 2.04j$ (winding H to winding T); $0 + 1.36$ (winding L to winding T) 

First, constructing the short circuit impedance matrix, the matrix will be of order $(3-1)=2$ since there are three windings. Using the impedance data: 

$$\mathbf{Z_B} =
\begin{bmatrix}
\frac{z_{HL}^{1V}}{z_{base}} & \frac{1}{2} \left( \frac{z_{HL}^{1V}}{z_{base}} + \frac{z_{HT}^{1V}}{z_{base}} - \frac{z_{LT}^{1V}}{z_{base}} \right) \\
\frac{1}{2} \left( \frac{z_{HL}^{1V}}{z_{base}} + \frac{z_{HT}^{1V}}{z_{base}} - \frac{z_{LT}^{1V}}{z_{base}} \right) & \frac{z_{HT}^{1V}}{z_{base}}
\end{bmatrix}
=
\begin{bmatrix}
\frac{j0.0204}{25 \times 10^3} & \frac{1}{2} \left( \frac{j0.0204}{25 \times 10^3} + \frac{j0.0204}{25 \times 10^3} - \frac{j0.0136}{25 \times 10^3} \right) \\
\frac{1}{2} \left( \frac{j0.0204}{25 \times 10^3} + \frac{j0.0204}{25 \times 10^3} - \frac{j0.0136}{25 \times 10^3} \right) & \frac{j0.0204}{25 \times 10^3}
\end{bmatrix}
=
\begin{bmatrix}
j0.816 \times 10^{-6} & j0.544 \times 10^{-6} \\
j0.544 \times 10^{-6} & j0.816 \times 10^{-6}
\end{bmatrix}$$

Then, constructing the $\mathbf{B}$ matrix to relate short circuit currents is assembled such that the short circuit current matrix (where winding 'H' is short circuited) equals the product $\mathbf{B}I_{sc}$: $ \displaystyle{}I_H=\mathbf{B}I_{sc}$

$$\begin{bmatrix}
I_H \\
I_L \\
I_T
\end{bmatrix}
=
\mathbf{B}
\begin{bmatrix}
I_L \\
I_T
\end{bmatrix}_{SC}$$

$$\begin{bmatrix}
I_H \\
I_L \\
I_T
\end{bmatrix}
=
\begin{bmatrix}
1 & 1 \\
-1 & 0 \\
0 & -1
\end{bmatrix}
\begin{bmatrix}
I_L \\
I_T
\end{bmatrix}_{SC}$$

The $\mathbf{N}$ matrix relates both sides of each winding, so here the turns ratios are reflected. N will be $2m\times m-1=6\times 3$: 

$$\begin{bmatrix}
I_{H1} \\
I_{H2} \\
I_{L1} \\
I_{L2} \\
I_{T1} \\
I_{T2}
\end{bmatrix}
=
\mathbf{N}
\begin{bmatrix}
I_H \\
I_L \\
I_T
\end{bmatrix}$$

$$\begin{bmatrix}
I_{H1} \\
I_{H2} \\
I_{L1} \\
I_{L2} \\
I_{T1} \\
I_{T2}
\end{bmatrix}
=
\begin{bmatrix}
\frac{1}{7200} & 0 & 0 \\
-\frac{1}{7200} & 0 & 0 \\
0 & \frac{1}{120} & 0 \\
0 & -\frac{1}{120} & 0 \\
0 & 0 & \frac{1}{120} \\
0 & 0 & -\frac{1}{120}
\end{bmatrix}
\begin{bmatrix}
I_H \\
I_L \\
I_T
\end{bmatrix}$$

Finally, the $\mathbf{A}$ matrix relates the connections of the transformer. This is where the reverse polarity of the tertiary winding is reflected. Here the primary neutral terminal, $N_P$, and secondary neutral terminal, $N_S$, (common to both the 'low' and 'tertiary' windings) are reflected. 

$$\begin{bmatrix}
I_H \\
I_{N_P} \\
I_L \\
I_T \\
I_{N_S}
\end{bmatrix}
=
\mathbf{A}
\begin{bmatrix}
I_{H1} \\
I_{H2} \\
I_{L1} \\
I_{L2} \\
I_{T1} \\
I_{T2}
\end{bmatrix}$$

$$\begin{bmatrix}
I_H \\
I_{N_P} \\
I_L \\
I_T \\
I_{N_S}
\end{bmatrix}
=
\begin{bmatrix}
1 & 0 & 0 & 0 & 0 & 0 \\
0 & 1 & 0 & 0 & 0 & 0 \\
0 & 0 & 1 & 0 & 0 & 0 \\
0 & 0 & 0 & 0 & 0 & 1 \\
0 & 0 & 0 & 1 & 1 & 0
\end{bmatrix}
\begin{bmatrix}
I_{H1} \\
I_{H2} \\
I_{L1} \\
I_{L2} \\
I_{T1} \\
I_{T2}
\end{bmatrix}$$

The primitive admittance matrix can then be calculated using the assembled matrices: 

$$\begin{bmatrix}
0 - j0.0284 & 0 + j0.0284 & 0 + j0.8510 & 0 - j0.8510 & 0 \\
0 + j0.0284 & 0 - j0.0284 & 0 - j0.8510 & 0 + j0.8510 & 0 \\
0 + j0.8510 & 0 - j0.8510 & 0 - j153.19 & 0 - j102.12 & 0 + j255.31 \\
0 - j0.8510 & 0 + j0.8510 & 0 - j102.12 & 0 - j153.19 & 0 + j255.31 \\
0 & 0 & 0 + j255.31 & 0 + j255.31 & 0 - j510.62
\end{bmatrix}$$

### Single-Phase Step Voltage Regulator

Rated (nominal) line-to-neutral voltage: 2400V Rated power: 1666.66 kVA Tap width: 0.625% Tap setting: 10 Short circuit percent impedance: 0.01 + j0.01 Note that even if the regulator impedance/losses are to be neglected, a small value must be used. As described above, the tap changes are included in the N matrix. The set of matrices can be constructed: 

$$\begin{bmatrix}
0 - j0.0284 & 0 + j0.0284 & 0 + j0.8510 & 0 - j0.8510 & 0 \\
0 + j0.0284 & 0 - j0.0284 & 0 - j0.8510 & 0 + j0.8510 & 0 \\
0 + j0.8510 & 0 - j0.8510 & 0 - j153.19 & 0 - j102.12 & 0 + j255.31 \\
0 - j0.8510 & 0 + j0.8510 & 0 - j102.12 & 0 - j153.19 & 0 + j255.31 \\
0 & 0 & 0 + j255.31 & 0 + j255.31 & 0 - j510.62
\end{bmatrix}$$

The primitive admittance matrix can then be calculated using the assembled matrices: 

$$\begin{bmatrix}
1446.2 - j1446.2 & -1446.2 + j1446.2 & -1361.1 + j1361.1 & 1361.1 - j1361.1 \\
-1446.2 + j1446.2 & 1446.2 - j1446.2 & 1361.1 - j1361.1 & -1361.1 + j1361.1 \\
-1361.1 + j1361.1 & 1361.1 - j1361.1 & 1281.0 - j1281.0 & -1281.0 + j1281.0 \\
1361.1 - j1361.1 & -1361.1 + j1361.1 & -1281.0 + j1281.0 & 1281.0 - j1281.0
\end{bmatrix}$$

## References

  1. Dugan, Roger C., "An Example of 3-phase Transformer Modeling for Distribution System Analysis," IEEE T&D Conference, Vol. 3, 2003.


# NEV Underground

Underground distribution lines will be modeled in a flexible way such that not only neutral conductors can be included but also any number of phases and/or circuits. 

## Variables

Table 1 - Equation Notation  Variable | Definition   
---|---  
$\displaystyle{}V_{i_{mg}}$ | Voltage at node i, phase m relative to true ground (V)   
$\displaystyle{}I_{i_{mg}}$ | Voltage at node i, phase m relative to true ground (A)   
$\displaystyle{}\hat z_{i-j_{nn}}$ | Element of series impedance matrix relating voltage/current relationship for line connecting nodes i and j, corresponding to self impedance of phase n. Return path (ground) impedance folded in ($\Omega /mile$)   
$\displaystyle{}\hat z_{i-j_{nm}}$ | Element of series impedance matrix relating voltage/current relationship for line connecting nodes i and j, corresponding to phases n and m. Return path (ground) impedance folded in ($\Omega /mile$)   
$\displaystyle{}r_{i,c}$ | Resistance of the phase conductor for cable i ($\Omega /mi$)   
$\displaystyle{}r_{i,cn}$ | The effective resistance of the concentric neutral ring for cable i ($\Omega /mi$)   
$\displaystyle{}r_{i,sh}$ | The effective resistance of the tape-shield for cable i ($\Omega /mi$)   
$\displaystyle{}\omega$ | System angular frequency ($rad/s$)   
$\displaystyle{}f$ | System frequency ($Hz$)   
$\displaystyle{}G=0.1609347\times 10^{-3}$ | Constant for converting from CGS units ($\Omega /mi$)   
$\displaystyle{}d_{i,c}$ | Diameter of the phase conductor for cable i ($in$)   
$\displaystyle{}d_{i,s}$ | Diameter of a neutral strand for cable i ($in$)   
$\displaystyle{}d_{i,sh}$ | Diameter of the tape-shield for cable i ($in$)   
$\displaystyle{}d_{i,od}$ | Outer diameter of cable i ($in$)   
$\displaystyle{}R_{i}$ | Radius of the circle passing through the concentric neutral strands for cable i ($in$)   
$\displaystyle{}T_{i}$ | Thickness of the tape-shield for cable i ($mil$)   
$\displaystyle{}k_{i}$ | The number of neutral strands for cable i ($unitless$)   
$\displaystyle{}GMR_{i,c}$ | Geometric mean radius of the phase conductor for cable i ($ft$)   
$\displaystyle{}GMR_{i,s}$ | Geometric mean radius of a neutral strand for cable i ($ft$)   
$\displaystyle{}GMR_{i,sh}$ | Geometric mean radius of the tape-shield for cable i ($ft$)   
$\displaystyle{}GMR_{i,cn}$ | The effective Geometric mean radius of the concentric neutral ring for cable i ($ft$)   
$\displaystyle{}D_{ij}$ | Distance between conductors i and j ($ft$)   
$\displaystyle{}S_{ij}$ | Distance between conductor i and conductor j's image ($ft$)   
$\displaystyle{}\theta_{ij}$ | Angle between a pair of lines drawn from conductor i to its own image and to the image of conductor j ($rad$)   
$\displaystyle{}\rho$ | Resistivity of earth ($\Omega -meters$)   
$\displaystyle{}\rho_{i,sh}$ | Resistivity of the tape-shield for cable i at a temperature of 50\Celsius ($\Omega -meters$)   
$\displaystyle{}\epsilon_{0}$ | The permittivity of free space = $1.4240\times 10^{-2} (\mu F/mi)$  
$\displaystyle{}\epsilon_{i,r}$ | The relative permittivity of the insulation medium for cable i ($unitless$)   
$\displaystyle{}C_{1}=\frac{1}{63360}$ | conversion factor for converting inches to miles for cable i ($mi/in$)   
$\displaystyle{}C_{2}=\frac{1}{63360000}$ | conversion factor for converting mils to miles for cable i ($mi/mil$)   
$\displaystyle{}C_{3}=\frac{1}{1609.344}$ | conversion factor for converting meters to miles for cable i ($mi/m$)   
  
## Series Impedance

Initially neglecting shunt admittances, the voltage/current relationship between two 'nodes' 1 and 2 (corresponding to physical terminal locations in the network) with $m$ phases can be expressed in matrix form: 

$$\begin{bmatrix}
V_{1_{ag}} \\
\downarrow \\
V_{1_{mg}}
\end{bmatrix}
-
\begin{bmatrix}
V_{2_{ag}} \\
\downarrow \\
V_{2_{mg}}
\end{bmatrix}
=
\begin{bmatrix}
\hat{z}_{1-2_{aa}} & \hat{z}_{1-2_{am}} \\
\downarrow & \downarrow \\
\hat{z}_{1-2_{ma}} & \hat{z}_{1-2_{mm}}
\end{bmatrix}
\begin{bmatrix}
I_{1-2_{ag}} \\
\downarrow \\
I_{1-2_{mg}}
\end{bmatrix}$$

For example, consider a distribution line with two electrically isolated feeders sharing one neutral phase. One of the feeders has all three phases $(a, b, c)$ present, while the other has only two phases $(a, c)$ present. For clarity, the two phases on the second feeder are renamed $(d, e)$. Then, the voltage drop on the line can be expressed by: 

$$\begin{bmatrix}
V_{1_{ag}} \\
V_{1_{bg}} \\
V_{1_{cg}} \\
V_{1_{dg}} \\
V_{1_{eg}} \\
V_{1_{ng}}
\end{bmatrix}
-
\begin{bmatrix}
V_{2_{ag}} \\
V_{2_{bg}} \\
V_{2_{cg}} \\
V_{2_{dg}} \\
V_{2_{eg}} \\
V_{2_{ng}}
\end{bmatrix}
=
\begin{bmatrix}
\hat{z}_{1-2_{aa}} & \hat{z}_{1-2_{ab}} & \hat{z}_{1-2_{ac}} & \hat{z}_{1-2_{ad}} & \hat{z}_{1-2_{ae}} & \hat{z}_{1-2_{an}} \\
\hat{z}_{1-2_{ba}} & \hat{z}_{1-2_{bb}} & \hat{z}_{1-2_{bc}} & \hat{z}_{1-2_{bd}} & \hat{z}_{1-2_{be}} & \hat{z}_{1-2_{bn}} \\
\hat{z}_{1-2_{ca}} & \hat{z}_{1-2_{cb}} & \hat{z}_{1-2_{cc}} & \hat{z}_{1-2_{cd}} & \hat{z}_{1-2_{ce}} & \hat{z}_{1-2_{cn}} \\
\hat{z}_{1-2_{da}} & \hat{z}_{1-2_{db}} & \hat{z}_{1-2_{dc}} & \hat{z}_{1-2_{dd}} & \hat{z}_{1-2_{de}} & \hat{z}_{1-2_{dn}} \\
\hat{z}_{1-2_{ea}} & \hat{z}_{1-2_{eb}} & \hat{z}_{1-2_{ec}} & \hat{z}_{1-2_{ed}} & \hat{z}_{1-2_{ee}} & \hat{z}_{1-2_{en}} \\
\hat{z}_{1-2_{na}} & \hat{z}_{1-2_{nb}} & \hat{z}_{1-2_{nc}} & \hat{z}_{1-2_{nd}} & \hat{z}_{1-2_{ne}} & \hat{z}_{1-2_{nn}}
\end{bmatrix}
\begin{bmatrix}
I_{1-2_{ag}} \\
I_{1-2_{bg}} \\
I_{1-2_{cg}} \\
I_{1-2_{dg}} \\
I_{1-2_{eg}} \\
I_{1-2_{ng}}
\end{bmatrix}$$

The hat notation, taken from [Kersting], indicates that the return path, i.e. the ground impedance, has been folded into the other impedances. According to Carson's equations, the elements of the primitive impedance matrix can be calculated by: 

$$\hat{z}_{1-2_{ii}} = r_i + 4\omega P_{ii}G + j\left(X_i + 2\omega G \ln{\frac{S_{ii}}{RD_i}} + 4\omega Q_{ii}G\right) \, \Omega / \text{mi}$$

$$\hat{z}_{1-2_{ij}} = 4\omega P_{ii}G + j\left(2\omega G \ln{\frac{S_{ij}}{D_{ij}}} + 4\omega Q_{ij}G\right) \, \Omega / \text{mi}$$

Wherein: 

$X_i = 2\omega{}G\ln{\frac{RD_i}{GMR_{i}}}\Omega /mi$

$P_{ij} = \frac{\pi}{8}-\frac{1}{3\sqrt{2}}k_{ij}\cos{\theta_{ij}}+\frac{k_{ij}^2}{16}\cos{2\theta_{ij}}\left(0.6728+\ln{2}{k_{ij}}\right)+\frac{k_{ij}^2}{16}\theta_{ij}\sin{2\theta_{ij}}$

$Q_{ij} = -0.0386 + \frac{1}{2}\ln{\frac{2}{k_{ij}}}+\frac{1}{3\sqrt{2}}k_{ij}\cos{\theta_{ij}}$

$k_{ij} = 8.565\times 10^{-4}S_{ij}\sqrt{\frac{f}{\rho}}$

The primitive series admittance matrix can be inverted to yield the primitive series admittance matrix for the line. For a more complete model of the line, the primitive shunt admittance matrix can then be added. 

The effective resistance and GMR of the tape-shield and concentric neutral ring need to be calculated in order to use the previously defined equations. 

### Concentric Neutral

The effective geomentric mean of the concentric neutral ring can be found using the following equation. 

$$\displaystyle{}GMR_{i,cn}=(GMR_{i,s}k_{i}R_{i}^{k_{i}-1})^{\frac{1}{k_{i}}} ft$$

$R$ is the radius of the circle passing through the center of the concentric neutral strands in ft and can be found using the equation below. 

$$\displaystyle{}R_{i}=\frac{d_{i,od}-d_{i,s}}{24} ft$$

The effective resistance of the concentric neutral ring is calculated using the following equation. 

$$\displaystyle{}r_{i,cn}=\frac{r_{i,s}}{k_{i}} \Omega/mi$$

Because the distance between cables is much greater than $R_{i}$ it is a good approximation to treat the concentric neutral strands as a single conductor located a distance $R$ above the center of the cable when determining distances between adjacent conductor cables. 

### Tape-Shielded

The GMR of the tape shield is given the equation below. 

$$\displaystyle{}GMR_{i,sh}=\frac{\frac{d_{i,sh}}{2}-\frac{T_{i}}{2000}}{12} ft$$

The resistance of the tape sheild is the given in the equation below. 

$$\displaystyle{}r_{i,sh}=\frac{C_{3}\rho_{i,sh}}{(C_{1}C_{2}d_{i,sh}T{i}+(C_{2}T_{i})^2)} \Omega/mi$$

The distance between the tape shield and it's own phase conductor is $GMR_{sh}$. 

## Shunt Admittance

The equations presented below assume that the electric field created by the charge on the phase conductor is is confined to the boundary of the insulation. Because of this assumption there is no cross coupling of the admittances between conductors. 

### Concentric Neutral

The shunt admittance between a conductor and the concentric neutral ring for a single cable is defined by the equation below. 

$$\displaystyle{}y_{in}=0+j\frac{2\pi\omega\epsilon_{0}\epsilon_{i,r}}{10^{6}(ln(\frac{2R_{i}}{d_{i,c}})-\frac{1}{k_{i}}ln(k_{i}\frac{d_{i,s}}{2R_{i}}))} S/mi$$

### Tape-Shield

The shunt admittance between a conductor and the tape-shield for a single cable is defined by the equation below. 

$$ \displaystyle{}y_{in}=0+j\frac{2\pi\omega\epsilon_{0}\epsilon_{i,r}}{10^{6}ln(\frac{2R_{i}}{d_{i,c}})} S/mi$$


