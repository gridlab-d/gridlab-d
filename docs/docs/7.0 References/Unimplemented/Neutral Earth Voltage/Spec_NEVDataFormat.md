# Spec:NEVDataFormat

!!! warning

	This page contains features that are unfinished, were never implemented, or have since been deprecated. We preserve these pages for archival purposes, and also as a foundational resource for prospective developers who may wish to implement the same or similar feature. Many of these pages provide robust explanations of the theory behind a particular module or feature that we hope readers will find useful. 
  
	**This page does not reflect the current state of GridLAB-D™**

This specification page outlines the formatting for array input data (GLM-based) for NEV objects, as well as the internal common programming structure. Input changes will require implementation of new functionality for interpreting GLM inputs. Note that the NEV solver methods and inputs will be exclusive to a Newton-Raphson solver implementation. While the Forward-Backward Sweep method could also be used for NEV solutions, it is not planned to be implemented at this time. 

# GLM Array Inputs

GLM-file inputs will need to support specifying which phase various connections will support. The multiple input arguments needed for the different phases and "generic" nature of the NEV implementation will be handled using a format similar to the [Double_array] specification, but outlined in further detail on the [NEV Array Page]. Below is the proposed implementation of what a `load` object with Wye-specified voltages and a delta-connected, constant impedance load could look like: 
    
    
     object load {
     	terminals "1,2,3";
     	voltage "2400.0+0.0d,1,0; 2400.0-120.0d,2,0; 2400.0+120.0d,3,0";
     	constant_impedance "2500+1200j,1,2; 2500+1200j,2,3; 2500+1200j,3,1";
     	nominal_voltage 2400.0;
     }
    

Note that `terminals`, `voltage`, and `constant_impedance` utilize the syntax of array and input specification of [NEV Array Page], which is expected to be the method for incorporating NEV-related data for all [powerflow] objects. The details of these individual fields are covered on the [Node specification] page. This merely serves as an example of the array-like input structure. Specific field formats are discussed on their appropriate parent class (`node` and `link`) pages. 

# Powerflow Data Structure

Inside the source code, all interfacing to the NEV solver will be done through a common data structure. The structure will follow a form similar to the version 3.1 and earlier Newton-Raphson structure and will be divided into two main structures: a node/bus structure and a link/branch structure. Sparse matrix operations will store the data in the `Y_NEV` structure defined in Table 1. 

Table 1 - Sparse Matrix Storage Structure - `Y_NEV` Property | C/C++ Type | Definition   
---|---|---  
`matrix_association` | `int` | Matrix designator for this entry (islands)   
`row_ind` | `unsigned int` | Row location of sparse matrix element   
`col_ind` | `unsigned int` | Column location of sparse matrix element   
`Y_value` | `double` | Value of sparse matrix element   
  
## Node/bus Structure

The elements of the node/bus structures are defined in Table 2. Node data will be formed in a structure typed `NEVbusdata`. Bolded items are internal working variables for the TCIM-Newton-Raphson method [1], but will be allocated within the [node] and [link] objects themselves. 

Table 2 - NEV Node/bus Program Data Structure - `NEVbusdata` Property | C/C++ Type | Definition   
---|---|---  
`name` | `char *` | Pointer to name field of [node] object.   
`obj` | `OBJECT *` | Pointer to object header of [node] object.   
`bustype` | `char` | Type of bus for powerflow handling. 0=`PQ`, 1=`PV`, 2=`SWING`  
`status_flags` | `unsigned int` | Bit-masked flag variable for miscellaneous information that needs to be passed to powerflow (TBD).   
`terminals` | `unsigned int64` | Bit-masked variable for terminals (phases) included on this node. Each bit position represents a unique phase.   
`original_terminals` | `unsigned int64` | Bit-masked variable for original terminals (phases) included on this node. Used as a comparison point for reliability/reconfiguration-related items. Each bit position represents a unique phase.   
`voltage` | `complex *` | Pointer to complex array within [node] object where voltages are stored.   
`Link_Table` | `int*` | Pointer to integer array that tracks [link] objects electrically connected to this [node]. This will include child-connected [link] objects. Entries refer to the corresponding index of the `NEVbranchdata` structure.   
`Link_Table_Size` | `unsigned int` | Integer count of number of connected [link] objects in the `Link_Table` entry.   
`max_volt_error` | `double` | Value of maximum voltage error allowed (convergence criterion)   
`dynamics_enabled` | `bool *` | Pointer to deltamode indicator for [node] object to determine if special dynamics code needs to be enacted.   
`Y_matrix_self` | `complex *` | Pointer to complex self-admittance matrix of this particular node. Expected size is the number of terminals squared (terminal count by terminal count square matrix).   
**`matrix_association`** | `int` | Index of associated matrix in the solver. Utilized by the solver for implementing multiple islands in the same GLM.   
**`matrix_index`** | `unsigned int` | Starting index of this object's place in all matrices   
**`matrix_size`** | `unsigned char` | Base size of the "self" portion of this [node] object's matrix entries. Should coincide with a count of active terminals on the system.   
`Y_update_fxn` | `void *` | Pointer to admittance update function for this particular `node` object. See details below. Defaults to `NULL` if no solver-level updates are required.   
`V_update_fxn` | `void *` | Pointer to voltage update function for this particular `node` object. See details below. Defaults to `NULL` if no solver-level updates are required, which implies it is independent of the interconnected powerflow.   
  
### Update Functions

In an effort to compartmentalize the NEV code, two update functions will be performed to move key updating aspects from the central solver code to the individual objects. This will help with data compartmentalization and parallelization of the code. Two specific function sets will be utilized for these updates. `Y_update_fxn` will perform any admittance-related updates and `V_update_fxn` will perform the voltage updates associated with the powerflow iteration process. Further details will be in the [node] specification and the [NEV solver] specification. 

`Y_update_fxn` will perform "admittance-level, intermediate solution" updates, including those that typically happen on a per-iteration basis. e.g., `A`, `B`, `C`, and `D` Jacobian matrix updates of [1] will be performed via a function call from the central solver. The function will be a simple Boolean return (pass/fail) that will update the "changing admittance" (`Y_matrix_update`) portion at every solution pass. 

`V_update_fxn` will perform the voltage updates for each intermediate solution of the powerflow. As with the previous admittance-level changes, this will perform the voltage updates to node/bus voltages at each iteration. This will return a simple character value indicating a pass, fail, or error state. This will be utilized to indicate if convergence was reached or not, or if some other issue occurred. This function will also call any `DELTAMODE`-related checks of the SWING initialization routine. 

## Link/branch structure

The elements of the link/branch structures are defined in Table 3. Link data will be formed in a structure typed `NEVbranchdata`. Bolded items are internal working variables for the TCIM-Newton-Raphson method [1], but will be allocated within the [node] and [link] objects themselves. 

Table 3 - NEV Link/bus Program Data Structure - `NEVbranchdata` Property | C/C++ Type | Definition   
---|---|---  
`name` | `char *` | Pointer to name field of [link] object.   
`obj` | `OBJECT *` | Pointer to object header of [link] object.   
`terminals_from` | `unsigned int64` | Bit-masked variable for terminals (phases) that are connected on the "from" side of the link. Each bit position represents a unique phase.   
`original_terminals_from` | `unsigned int64` | Bit-masked variable for original terminals (phases) that are connected on the "from" side of the link. Used as a comparison point for reliability/reconfiguration-related items. Each bit position represents a unique phase.   
`terminals_to` | `unsigned int64` | Bit-masked variable for terminals (phases) that are connected on the "to" side of the link. Each bit position represents a unique phase.   
`original_terminals_to` | `unsigned int64` | Bit-masked variable for original terminals (phases) that are connected on the "to" side of the link. Used as a comparison point for reliability/reconfiguration-related items. Each bit position represents a unique phase.   
`status` | `enumeration *` | Pointer to enumeration status variable of [link] object   
`link_type` | `enumeration *` | Pointer to enumeration of type-variable of [link] object. e.g., OH/UG line, transformer, etc. -- Used for [reliability].   
`from` | `int` | Index to entry of `NEVbusdata` that corresponds to the "from" [node] object of this [link].   
`to` | `int` | Index to entry of `NEVbusdata` that corresponds to the "to" [node] object of this [link].   
`fault_current_from` | `complex *` | Pointer to complex array of calculated fault current at the "from" side of the [link] object. Used by reliability.   
`fault_current_to` | `complex *` | Pointer to complex array of calculated fault current at the "to" side of the [link] object. Used by reliability.   
`fault_link_below` | `int` | Index to entry of `NEVbranchdata` that was responsible for a fault this [link] object currently "sees". Used by reliability.   
  
It is useful to note that many of the above variable references are pointers to the original [node] and [link] data variables. If multiple [powerflow] solutions are required, such as in forecasting or scenario-based problems, values for these components will need to be saved as well. 

# Data population

Many of the elements of the data structure are utilized by the underlying NEV solver, but will be populated by the individual base object types. Details will be included in the individual object specifications. 

# References

  1. Garcia, P, J.L. Pereira, S. Carneiro Jr., V. da Costa, and N. Martins, "Three-Phase Power Flow Calculations Using the Current Injection Method," _IEEE Transactions on Power Systems_ , vol. 15, no. 2, May 2000, pp. 508-514.
# See also

  * [Overview Page]
  * [Requirements]
  * [Specifications]
  * [Implementation]
  * [Keeler (Version 4.0)]
