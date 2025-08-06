# Newton-Raphson Distribution Powerflow Solver

To integrate a external Newton-Raphson power flow solver, several important pieces of information must be exchanged. The basis for this exchange is currently stubbed out in `solver_nr.cpp` under the Powerflow Module source code. Using the algorithm developed in [1], two basic information structures will be required. These are a structure for bus quantities of interest and a structure for link properties of interest. Some generic properties of the system will also be made available. All complex quantities will be defined in the rectangular coordinate plane. To interpret these structures, a count of both the number of total busses and total branches in the system is provided. 

# Data model

The first structure points to information about bus quantities. This structure will contain a pointer array to the three-phase voltage, power, impedance loads, and current loads. The named fields of the structure are `V`, `S`, `Y`, and `I` respectively. The voltage, power, current loads, and impedance loads (given as admittance values) are all arrays of size $n \times 3\,\\!$ where $n\,\\!$ is the bus count. The three columns will contain pointers to the A, B, and C phase quantities of a three phase system. All values passed will represent Wye-configured values. Any Delta-connected devices will have the quantities converted appropriately before passing to the Newton-Raphson solver. Power and current quantities will follow the convention of positive for loading and negative for generation. The structure also contains information about the bus type, voltage basis, and power basis of the system. These items are in named fields `type`, `kv_base`, and `mva_base` respectively. Bus type information will be passed numerically with 0 representing a PQ bus, 1 representing a PV bus, and 2 representing a swing or slack bus. The `kv_base` and `mva_base` represent the quantities needed for per-unit calculations. 

The second structure points to information about branch quantities. This structure will contain a pointer array to the three-phase admittance matrix for a connected branch, a "from" index, a "to" index, and a voltage ratio. The three-phase admittance matrix will be of $3 \times 3\,\\!$ size pointed to by field `Y`. The "from" and "to" values will represent the busses to which each branch is connected and are labeled `from` and `to` respectively. The values provided will be associated with the rows of the bus information. For example, if a branch has a "from" value of 3 and a "to" value of 5, this would represent a connection between the busses represented by rows 3 and 5 of the bus information vectors. The voltage ratio information, contained in the `v_ratio` field, will contain any voltage or turns ratio information for a transformer or similar branch device. 

In order to properly index the full arrays of information, the total number of busses and branches in the system will be provided. These will be integer values representing the total number of items in each pointer array. This will enable the Newton-Raphson method to interpret the length of each array and ensure no invalid quantities are read. 

Due to the pointer nature of the information provided, once the Newton-Raphson solver has reached convergence, updated values for the voltages will simply be written back into their appropriate memory location. This will update the objects inside GridLAB-D in preparation for any further analysis required. Once the update has been finished or the solver is terminated, the Newton-Raphson method will return one of three flag values. If a value of 0 is returned, it indicates the Newton-Raphson method failed to complete even a single iteration. If a positive integer is returned, this is the number of iterations a successful convergence required. If a negative integer is returned, this is the number of iterations attempted before the solver failed. For example, if the solver performed 6 iterations, it would return a value of 6 indicating a successful convergence or -6 indicating a failure to converge. 

# Implementation

The **powerflow** module uses a module global variable called `solver_method` to identify which method should be used. To select the Newton-Raphson method, the following module directive should be used 
    
    
    module powerflow {
      solver_method NR;
    }
    

The Newton-Raphson solver is implemented in the file `solver_nr.cpp` with declarations in `solver_nr.h` in the powerflow module. Two structures are used to pass data to the solver. The `BUSDATA` structure contains an array of pointers to bus information: 
    
    
    typedef struct  {
    	int type;        ///< bus type (0=PQ, 1=PV, 2=SWING)
    	complex *V[3];   ///< bus voltage
    	complex *S[3];   ///< constant power
    	complex *Y[3];   ///< constant admittance (impedance loads)
    	complex *I[3];   ///< constant current
    	double kv_base;  ///< kV basis
    	double mva_base; /// MVA basis
    } BUSDATA;
    

and the `BRANCHDATA` structure contains an array of pointers to branch information, as well as indexes into the `BUSDATA` array: 
    
    
    typedef struct {
    	complex *Y[3][3]; ///< branch admittance
    	int from;         ///< index into bus data
    	int to;           ///< index into bus data
    	double v_ratio;   ///< voltage ratio(v_from/v_to)
    } BRANCHDATA;
    

The solver function prototype is `int solve_nr(int bus_count, BUSDATA *bus, int branch_count, BRANCHDATA *branch)`, where `bus_count` and `branch_count` are the counts of entries in the bus and branch arrays. 

The solver returns 0 if the powerflow cannot be solved at all, a positive number to indicate the number of iterations complete if the powerflow was solved, and a negative number to indicate the number of iterations at which it was determined that the solver could not converge. 

# References

[1] Garcia, P., Pereira, J., Carneiro, Hr. S., da Costa, V., and Martins, N., "Three-Phase Power Flow Calculations Using the Current Injection Method," IEEE Transactions on Power Systems, vol. 15, no. 2, pp. 508-514, May 2000. 


  
