# Switch 

The **switch** objects provide a means to electrically disconnect two portions of the power system model. The **switch** is a subclass of **link** objects, so it will connect two **node**-based objects on the system. Operations on the **switch** will either allow current flow between these **node** objects, or prevent it. 

## GLM Inputs

The **switch** is a very basic device. A minimal switch, accepting the default of being closed, would look like: 
    
    
     object switch {
     	phases ABC;
     	name connecting_switch;
     	from node1;
     	to node2;
     }
    

The switch-specific, full GLM-accessible-properties object would be: 
    
    
     object switch {
     	phases ABC;
     	name connecting_switch;
     	from node1;
     	to node2;
     	status CLOSED;
     	switch_resistance 0.0001 Ohm;
     }
    

It is useful to note that the `status` field will manipulate all of the phases on the switch. i.e., this device only operates in banked mode - if individual phase switching is needed, individual **switch** devices would need to be instantiated for each phase. 

All standard **link** properties are inherited as well, and are not listed here (e.g., current and power flow through the switch). 

Details on the properties are outlined in Table 1. Note that although some of these are defined in the base **link** class, they're also listed here for completeness. 

##### Table 1 - Switch properties  

Property | Type | Definition   
---|---|---  
`phases` | set | Phases the switch will conduct across. All switch operations will affect all of these phases (only operates in a banked configuration)   
`name` | char1024 | Base GLD property - name of the object for easier debugging and reference   
`from` | object | Base link property - name of the node-based object on one end of the switch   
`to` | object | Base link property - name of the node-based object on the opposite side of the switch   
`status` | enumeration | Base link property - can be `OPEN` (`from` and `to` are disconnected) or `CLOSED` (`from` and `to` are connected). By default, the device starts `CLOSED`. All `phases` present are influenced by this (only banked operation).   
`switch_impedance` | complex | Defines the impedance (per-phase) of the switch, when in the `CLOSED` state. Defaults to the `default_resistance` property of the `powerflow` module for both the real and reactive components.   
`switch_resistance` | double | Defines just the real portion (per-phase) of `switch_impedance`. If unspecified, defaults to `default_resistance`, as dictated by `switch_impedance` above.   
`switch_reactance` | double | Defines just the reactive portion (per-phase) of `switch_impedance`. If unspecified, defaults to `default_resistance`, as dictated by `switch_impedance` above.   
  
## Model Implementation

TODO - Relevance - Review whether this level of detail of model implementation is necessary

Implementation for the two different solver method (FBS and NR) are similar, but different enough that the details are outlined here. Switches are assumed to just connect direct phases. e.g., phase A of `from` to phase A of `to`; no phase-switching or other configurations are supported. 

### FBS

The Forward-Backward Sweep method implementation follows the standard line equations from Distribution System Modeling and Analysis, by William Kersting: 

$\mathbf{V}_{to}=\mathbf{A}\mathbf{V}_{from}-\mathbf{B}\mathbf{I}_{to}$

$\mathbf{V}_{to}=\mathbf{d}\mathbf{V}_{from}-\mathbf{b}\mathbf{I}_{from}$

$\mathbf{V}_{from}=\mathbf{a}\mathbf{V}_{to}+\mathbf{b}\mathbf{I}_{to}$

$\mathbf{I}_{to}=\mathbf{-c}\mathbf{V}_{from}+\mathbf{d}\mathbf{I}_{from}$

$\mathbf{I}_{to}=-\mathbf{c}\mathbf{V}_{from}+\mathbf{a}\mathbf{I}_{from}$

$\mathbf{I}_{from}=\mathbf{c}\mathbf{V}_{to}+\mathbf{d}\mathbf{I}_{to}$


The matrices $\mathbf{A},\mathbf{B},\mathbf{a},\mathbf{b},\mathbf{c},$ and $\mathbf{d}$ must be defined. By Kersting's equations, $\mathbf{A}=\mathbf{a}^{-1}$ and $\mathbf{B}=\mathbf{a}^{-1}\mathbf{b}$. 

For `OPEN` states, the relevant phases of the diagonal elements of the matrices are zero: 

$\mathbf{A}=0.0$
$\mathbf{B}=0.0$
$\mathbf{a}=0.0$
$\mathbf{b}=0.0$
$\mathbf{c}=0.0$
$\mathbf{d}=0.0$

For `CLOSED` states, the relevant phases of the diagonal elements of the matrices are: 

$\mathbf{A}=1.0$,
$\mathbf{B}=$`switch_impedance`,
$\mathbf{a}=1.0$,
$\mathbf{b}=$ `switch_impedance`,
$\mathbf{c}=0.0$,
$\mathbf{d}=1.0$

### NR

The Newton-Raphson method implementation is simplified into four matrices. The specifics of their use are detailed, but equations will be omitted, for brevity. 

$\mathbf{Y}$ \- admittance matrix - used in the powerflow solution

$\mathbf{b}$ \- impedance matrix - used for current and power calculations

$\mathbf{a}$ \- voltage ratio matrix - used in some fault and standard current calculations

$\mathbf{d}$ \- voltage ratio matrix - used for current and power calculations

For `OPEN` states, the relevant phases of the diagonal elements of the matrices are zero: 

$\mathbf{Y}=0.0$,
$\mathbf{b}=0.0$,
$\mathbf{a}=0.0$,
$\mathbf{d}=0.0$

For `CLOSED` states, the relevant phases of the diagonal elements of the matrices are: 

$\mathbf{Y}=\frac{1}{Z_s}$,
$\mathbf{b}=Z_{s}$,
$\mathbf{a}=1.0$,
$\mathbf{d}=1.0$

where $Z_{s}$ is the value of `switch_impedance`. 

Whenever a switch state changes (`OPEN` to `CLOSED` and vice-versa), the switch will also set the powerflow global `NR_admit_change` to a value of `true`. 

## Programming Considerations

The switch has several function calls from the `reliability` module that will need to be either deprecated/removed, or updated. 

The **recloser** and **sectionalizer** objects are still expected to be sub-classes of the **switch**. No functional changes are expected to occur with this implementation change, but code updates may be needed to ensure proper functionality. 

## Testing

Primary testing will be to ensure the **switch** still passes all existing autotests, particularly those in `reliability` and those that utilize a **switch**. 

No further autotests should be needed for the base functionality, since the existing autotests already encompass commonly-used scenarios. 

## Related Concepts:

  * Overview Page
  * Requirements
  * Implementation
