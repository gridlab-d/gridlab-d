# Spec:NEVSolver

!!! warning

	This page contains features that are unfinished, were never implemented, or have since been deprecated. We preserve these pages for archival purposes, and also as a foundational resource for prospective developers who may wish to implement the same or similar feature. Many of these pages provide robust explanations of the theory behind a particular module or feature that we hope readers will find useful. 
   
	**This page does not reflect the current state of GridLAB-D™**

The NEV solver will utilize the current injection method outlined in [1]. The underlying algorithm is utilized in the existing Newton-Raphson (NR) solver inside GridLAB-D™ as TCIM-NR. The Neutral Earth Voltage (NEV) solver will use the same TCIM-NR approach, but will expand the solution to support more than three phases. superLU will continue to be the default LU decomposition solver for the NEV solver, but support for external LU solvers (such as KLU) will be incorporated in a manner similar to the existing Newton-Raphson solver. 

# Process

While the details of the algorithm are found in [1], the basic process for implementation within GridLAB-D™ will occur in the following steps: 

  1. `link` objects will compute their static base admittance matrix contributions. This occurs as part of the normal exec loop process, likely during `presync` calls. 
     1. `link` objects will post their "off-block", "transfer admittance" diagonal matrix elements to the overall system admittance matrix, in `Y_NEV` format. This will be the portions associated with the current transfers between nodes.
     2. `link` objects will post their "on-diagonal" block matrix components to the appropriate nodes "self-admittance" matrix. This will be internal to the `node` objects.
  2. `node` objects will compute their admittance matrix contributions and `deltaI` vector contributions. This will occur as part of the NEV solver call, executed by the SWING node(s) of the system. This functionality will be initiated by the `Y_update_fxn` detailed in the [node] and [data format] specifications. 
     1. `node` objects will use any fixed components from the `link` objects update to form the basis of the self admittance matrix.
     2. `node` objects will contribute any fixed, `DELTAMODE`-oriented contributions to the "self-admittance" matrix portions.
     3. `node` objects will perform the power calculations for all specific load contributions associated with that bus.
     4. `node` objects will perform the calculations associated with computing the `deltaI` components. 
        1. Self admittance matrices will be utilized to compute base current values.
        2. Load currents calculated previously will be added to the base current values to compute `deltaI`.
        3. `DELTAMODE` contributions to `deltaI` will be accumulated.
     5. Formulate Jacobian `A`, `B`, `C`, and `D` matrices (from [1]). 
        1. Apply current contributions of defined loads (ZIP portions)
        2. Apply current contributions from other devices (e.g., `house_var`)
     6. Update self admittance matrices with Jacobian `A`, `B`, `C`, and `D` components
  3. Format the input data to the LU solver (see functions below for current superLU-implemented functions). 
     1. Form the larger admittance matrix to be passed to the LU solver method.
     2. Form the `deltaI` variable as the solution for the equations.
  4. Perform the LU decomposition to obtain the `deltaV` values
  5. `node` objects will perform "post-solution" updates via the `NEVBUSDATA`-mapped `V_update_fxn`. 
     1. Voltage updates will be performed.
     2. Voltage convergence will be checked.
     3. `DELTAMODE`-related checks on the SWING bus role (SWING or PQ bus emulation) will be performed.
  6. Reiterate from step 2, as necessary until convergence or a convergence limit is reached.
# Solution Timing

As with many items within GridLAB-D™, timing of the various steps in the NEV solver process will be a key concern. Pieces of information must fully pass between dependent components before certain steps, such as the LU decomposition, can occur. Current implementations of GridLAB-D™ impose the following restriction on the dependency of steps above: 

  * All admittance changes must be done and in the overall matrix format before the LU decomposition can occur.
  * All load contributions must be finalized before the nodal admittance contributions are computed.
  * All line contributions to nodal admittance portions must be finalized before the final nodal admittance contributions are computed.
  * Load contributions are typically included by other objects in sync or presync at this time.
  * Current and power calculations in individual objects will rely on the solved voltage values, so they must occur after the LU decomposition.
Based on these restrictions, the following execution order is proposed within the GridLAB-D™ framework. Note that the order of "intra-pass" operations will need to rely on object ranking or a similar mechanism to ensure operations occur in the proper order. 

  1. **Presync**
     1. Link objects post admittance contributions to nodes and overall admittance matrix
  2. **Sync**
     1. All loads and interfacing objects have post their contributions.
     2. Childed objects post their contributions to the main, "electrically connected" parent.
     3. Island "membership" should be determined (if any change was on the system)
     4. Nodal contributions to the overall admittance matrix are computed and posted.
     5. LU decomposition and voltage update process occurs on all islands within the system (possibly multiple SWING bus objects).
  3. **Post sync**
     1. All "voltage-reliant" calculations (e.g., power, current) are computed and any control-related actions are decided
# Islanding/reconfiguration

The NEV solver will include the capability to have multiple islanded systems within a single GLM, as well as support any "spontaneous islands" through reconfiguration and protective actions. Islands will be determined through the following criteria: 

  * Connection to a source 
    * SWING bus -- multiple SWINGs may exist
    * Distributed generation, especially in deltamode+. This will be determined with the `HAS_SOURCE` flag that already exists within `node` objects.
  * Pure support checks will be conducted via the `fault_check` object to see which objects have a connection to at least one source. This will require a slight modification to the connection determination inside ticket 767.
  * Unsupported objects will be placed into a state where they are excluded from current powerflow values. This will be indicated by a lack of information in the `terminals` field in NEVBUSDATA and `terminals_from` and `terminals_to` fields in the NEVBRANCHDATA structures.
+Note that while islands will solve individual powerflow sets, the current implementation of "failure to converge results in a failed simulation" will still apply to the whole system/set of system. i.e., if two islands are present and one fails to converge, the entire GridLAB-D™ instance will terminate, not just the divergent island. This condition is expected to be mitigated through object-level and user-level controls. 

# Functions

To implement the NEV solver in GridLAB-D™, some support functions are required. 

## Merge Sort Function

Utilized in the existing NR-solver implementation, superLU requires the input matrices to be in a specific sparse matrix format known as COLAMD. In order to effectively place the data in this structure, it must be sorted by matrix location. The `merge_sort` function already implemented in `solver_nr.cpp` will be used for this function. It will continue to serve as the intermediate step between the "raw admittance" matrix and the COLAMD-formatted superLU-ready matrix. 

## Node functions

To compartmentalize the powerflow solution task and to enable easier parallelization, two new functions will be implemented at the `node`-object level. These will perform intermediate-solution updates for the powerflow process. This functionality was previously included in the `solver_nr` implementation. 

### Y_update_fxn

This function will perform any pre-LU-decomposition tasks. This is primarily the formation of the diagonal block portions of the admittance matrix and the "solution" vector containing the current injection components. A Boolean return flag will indicate if the operation completed successfully, or if an error occurred. Individual `node` objects will perform the following on every call of `Y_update_fxn`

  1. Add in any `DELTAMODE`-related self-admittance components. These are typically associated with a shunt impedance of a Norton-equivalent circuit.
  2. Compute the "load power" as a current form for this bus. This will be the net contributions of any load components at the bus.
  3. Compute the `deltaI` components for this bus. This will include the load currents just computed, `DELTAMODE` generator contributions (`DynCurrent` values), and self-admittance matrix multiplications with voltage.
  4. Compute the Jacobian `A`, `B`, `C`, and `D` matrices (from [1]). These are associated with the specific loading at the bus. This is typically any ZIP components, as well as specific exogenous inputs (`house_var`, associated with "unrotated" current load from `house` objects).
  5. Combine the self-admittance portions and Jacobian `A`, `B`, `C`, and `D` matrices to form the final "diagonal block" portion of the admittance matrix.
  6. Convert the final "diagonal block" portions of the matrix into `Y_NEV` form for passing to the LU solver.
  7. Return `true` for a successful operation, `false` for a failure of the process.
### V_update_fxn

This function will perform updates to the voltage values at a bus following a successful LU decomposition of the admittance-Jacobian and current injection different equations. Each individual bus will perform the following on every call of `V_update_fxn`: 

  1. Apply `deltaV` updates associated with the LU decomposition solution.
  2. Check the "SWING" convergence condition if this is an initialization pass of `DELTAMODE`-enabled SWING bus with a generator attached.
  3. Check for convergence against the `maximum_voltage_error` field inside the `node` object.
  4. Return 0 for failed convergence, 1 for successful, and -1 for an error condition.
# References

  1. Garcia, P, J.L. Pereira, S. Carneiro Jr., V. da Costa, and N. Martins, "Three-Phase Power Flow Calculations Using the Current Injection Method," _IEEE Transactions on Power Systems_ , vol. 15, no. 2, May 2000, pp. 508-514.
# Related Concepts:

  * [Overview Page]
  * [Requirements]
  * [Specifications]
  * [Implementation]
  * [Keeler (Version 4.0)]

