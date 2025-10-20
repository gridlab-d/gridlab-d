# Spec:NEVNode

!!! warning

	This page contains features that are unfinished, were never implemented, or have since been deprecated. We preserve these pages for archival purposes, and also as a foundational resource for prospective developers who may wish to implement the same or similar feature. Many of these pages provide robust explanations of the theory behind a particular module or feature that we hope readers will find useful. 
    
	**This page does not reflect the current state of GridLAB-D™**

The NEV implementation of node-based objects will describe what additional or new information is needed to model the NEV behavior on powerflow. All previous functionality of nodes, meters, loads, triplex_nodes, triplex_meters, triplex_meters will stay the same. 

## Node/Meter

In order to support NEV, all node-based objects will need to define terminals in order to provide connection points for transformers or lines. There will need to be a way for voltages to be specified for each terminal as well. Below is an example of how such information could be represented in a glm. This example represents a node that would traditionally have an A, B, C, and neutral phase, as well as another A phase (of a different circuit) on the same point. Note that the individual phase voltages are defined against the neutral (e.g., `2400.0+0j,1,4`) and the neutral potential definition `(0+0j,4,0)` sets the neutral earth voltage to 0 (possibly implying a bonded connection). Terminal 0 is the implied ground that always exists on every node of the system. Terminal 0 is reserved to represent the infinite plane, zero-potential ground point in the system. 
    
    
     object node {
     	terminals "1,2,3,4,64";	
     	voltage "2400.0+0j,1,4; -1200-2078.4610j,2,4; -1200+2078.4609j,3,4; 0+0j,4,0; 7200+0j,64,4";
     	nominal_voltage 2400.0;	//nominal voltage at node
     }
    

This input would then be read to create some internal variables: `NEV_voltage`[n] array, `NEV_current`[n][n] array, `NEV_power`[n][n] array, `NEV_shunt`[n][n] array, and `NEV_Y_FULL`[2][n][n] where n is corresponds to the number of terminals of the top most parent node. `NEV_Y_FULL` is where the admittance matrices of the node, and all links connected to that node are aggregated together and split into real and imaginary. The table below provides some definitions for the new properties shown in the above example. 

Table 1 - NEV Node properties  Property | Definition   
---|---  
`terminals` | Terminal definition for the node. It defines how many distinct voltage potentials a node supports. Values between 1 and 64 are supported.   
`voltage` | Potential between terminals of `terminals`. Used to define starting voltages (SWING node and initial powerflow solutions) and used to read voltages. Format for the specification is "complex value,terminal 1, terminal 2" to define a potential between terminals 1 and 2. Terminal 0 is absolute ground (0.0 Volt potential).   
  
### Infinite Children

The NEV implementation of node-base objects will allow for infinite children. Because of this Child nodes must defer their initialization until their parent has initialized. To ensure the correct updating of voltage, current, shunt admittance, and power arrays between parents and children the sizes of the arrays must be the same. So Child terminals numbers must match the parent terminal numbers. For example say we have a parent node defined below. 
    
    
     object node {
     	name node1;
     	terminals "1,2,3,4";	
     	voltage "2400.0+0j,1,0; -1200-2078.4610j,2,0; -1200+2078.4609j,3,0; 0.0,4,0";
     	nominal_voltage 2400.0;	//nominal voltage at node
     }
    

In order to correctly child a single-phase constant power load onto phase A the load object would have to look like this. 
    
    
     object load {
     	parent node1;
     	terminals "1,4";	
     	voltage "2400.0+0j,1,0; 0+0j,4,0;";
     	nominal_voltage 2400.0;	//nominal voltage at node
     	constant_load "24+15j,1,4";
     }
    

In this way the child load object would check the size of it's parent `NEV_voltage`, `NEV_power`, `NEV_current`, `NEV_shunt` arrays to determine the size of it's arrays and put the load information in the same spot as it's parent would when the child updates it's parent load arrays. In other words, all terminals specified in a child node must exist in the terminals of the parent node. If a parent node only has terminal 1, 2, and 3 then the child node cannot have terminals 4, 5, and 6 because terminals 4, 5, and 6 do not exist on the parent node and voltage information cannot be passed down to the child terminals 4, 5, and 6 and current injections can't be passed back up. 

#### Connector Object

Parent-child connections are the preferred method for breaking an identical electrical potential into separate node objects. The NEV solver will also support a `connector` object, which will effectively do the same operations as the parent-child relationship in a `link`-based object. See the [Spec:NEVLink] page for more details on the `connector` object. 

#### Ranking

Because every child node aggregates its `NEV_power`, `NEV_current`, `NEV_shunt` up to it's parent's corresponding arrays in Sync() every child must be one rank lower than its parent. Also All links connected to a node must be one rank lower than the node. 

### NEV Voltages

If the `terminals` property is specified but no `voltage` field, then the `NEV_voltage` array will be populated with voltages using the `nominal_voltage` and the angle for each index will be arbitrarily determined by the normal 120-degree phase rotation. For example if we have a node: 
    
    
     object node {
     	terminals "1,2,3,4,5,6";	
     	nominal_voltage 7200.0;	//nominal voltage at node
     }
    

Then `NEV_voltage` = [7200+j0, -3600-j6235.3829, -3600+j6235.3829, 7200.0+j0, -3600-j6235.3829, -3600+j6235.3829] 

### NEV Current, Shunt, and Power

The `NEV_current`, `NEV_shunt`, and `NEV_power` arrays get instantiated in init(). Each element contained in these arrays represents the current injection, power injection, or shunt admittance between the terminals of the node where the terminals are represented by the row and column of the element. Positive directionality for power and current injections is defined as the power or current flowing into the terminal. the structure of the `NEV_current`, `NEV_shunt`, and `NEV_power` arrays are shown below. 

`NEV_power`[N][N] 

$$\displaystyle{}\begin{bmatrix}
S_{terminal_1-Ground}&S_{terminal_1-terminal_2}&\rightarrow &S_{terminal_1-termianl_N}\\ S_{terminal_2-terminal_1}&S_{terminal_2-Ground}&\rightarrow &S_{terminal_2-termianl_N}\\ \downarrow &\downarrow &\downarrow &\downarrow \\ S_{terminal_N-terminal_1}&S_{terminal_N-terminal_2}&\rightarrow &S_{terminal_N-Ground} \end{bmatrix}$$

`NEV_current`[N][N] 

$$\displaystyle{}\begin{bmatrix}
I_{terminal_1-Ground}&I_{terminal_1-terminal_2}&\rightarrow &I_{terminal_1-termianl_N}\\ I_{terminal_2-terminal_1}&I_{terminal_2-Ground}&\rightarrow &I_{terminal_2-termianl_N}\\ \downarrow &\downarrow &\downarrow &\downarrow \\ I_{terminal_N-terminal_1}&I_{terminal_N-terminal_2}&\rightarrow &I_{terminal_N-Ground} \end{bmatrix}$$

`NEV_shunt`[N][N] 

$$\displaystyle{}\begin{bmatrix}
Y_{terminal_1-Ground}&Y_{terminal_1-terminal_2}&\rightarrow &Y_{terminal_1-termianl_N}\\ Y_{terminal_2-terminal_1}&Y_{terminal_2-Ground}&\rightarrow &Y_{terminal_2-termianl_N}\\ \downarrow &\downarrow &\downarrow &\downarrow \\ Y_{terminal_N-terminal_1}&Y_{terminal_N-terminal_2}&\rightarrow &Y_{terminal_N-Ground} \end{bmatrix}$$

### Order of operations

#### Init()
    
    
    if(parent != NULL){
        if(parent has not initialized){
            defer initialization;
        } else {
        create NEV_voltage, NEV_current, NEV_power, and NEV_shunt arrays the same size of as my parent's;
        populate NEV_voltage with initial voltages from parent;
    } else {
        create NEV_voltage, NEV_current, NEV_power, and NEV_shunt arrays with the size corresponding to the largest terminal specified in terminals;
        create NEV_full_Y;
        populate NEV_voltage with initial voltages from voltages or terminals and nominal_voltage;
        if(bustype == SWING){
        Instantiate NEVBUSDATA belonging strictly to me; //Not really sure how to handle multiple swings 
        } else {
            link to the NEVBUSDATA structure belonging to the nearest upstream swing node;
        }
    }
    

#### Presync()
    
    
    if(I'm a meter){
        update energy usage using the previous NEV_voltage, NEV_current, NEV_power, and NEV_shunt;
    }
    reset NEV_current, NEV_power, and NEV_shunt arrays to zero;
    

#### Sync()

SPECIAL NOTICE!!! Any islanding events need to be triggered before this function in order to determine if the node has become a new swing node. 
    
    
    if(I'm a load){
        aggregate NEV_current, NEV_power, and NEV_shunt arrays with input from constant_current, constant_power, and constant_impedance;
    }
    if(parent != NULL){
        aggregate into parent's NEV_current, NEV_power, and NEV_shunt with my NEV_current, NEV_power, and NEV_shunt;
    }
    

#### Postsync()
    
    
    if(parent != NULL)
        update NEV_voltages with parent's NEV_voltages;
    

## Loads

Loads will follow their the same behavior as the nodes but will have three additional properties in describing NEV loads as shown below 
    
    
     object load {
     	terminals "1, 2, 3, 4, 64";	
     	voltage "2400.0+0j,1,0; -1200-2078.4610j,2,0; -1200+2078.4609j,3,0; 0+0j,4,0; 7200+0j,64,0";
     	nominal_voltage 2400.0;	//nominal voltage at node
     	constant_power "12+9j,1,4; 26.43+14j,3,64";
     	constant_current "-3-12j,2,3";
     	constant_impedance "12-23j,1,3; 0.5-4.5j,2,4";
     }
    

The table below provides some definitions for the new properties shown in the above example. 

Table 1 - NEV Load properties  Property | Definition   
---|---  
`terminals` | Terminal definition for the node. Synonymous with phases, but not defined in an `ABC` sense.   
`voltage` | Potential between corresponding terminals. Used to define starting voltages (SWING node and initial powerflow solutions) and used to read voltages.   
`constant_power` | An [M][3] complex array of the constant power loads where M is the number of entries separated by the semi-colons.   
`constant_current` | An [M][3] complex array of the constant current loads where M is the number of entries separated by the semi-colons.   
`constant_impedance` | An [M][3] complex array of the constant impedance loads where M is the number of entries separated by the semi-colons.   
  
### Order of operations

#### Init()
    
    
    if(parent != NULL){
        if(parent has not initialized){
            defer initialization;
        } else {
        create NEV_voltage, NEV_current, NEV_power, and NEV_shunt arrays the same size of as my parent's;
        populate NEV_voltage with initial voltages from parent;
    } else {
        create NEV_voltage, NEV_current, NEV_power, and NEV_shunt arrays with the size corresponding to the largest terminal specified in terminals;
        create NEV_full_Y;
        populate NEV_voltage with initial voltages from voltages or terminals and nominal_voltage;
        if(bustype == SWING){
        Instantiate NEVBUSDATA belonging strictly to me; //Not really sure how to handle multiple swings 
        } else {
            link to the NEVBUSDATA structure belonging to the nearest upstream swing node;
        }
    }
    

#### Presync()
    
    
    if(I'm a meter){
        update energy usage using the previous NEV_voltage, NEV_current, NEV_power, and NEV_shunt;
    }
    reset NEV_current, NEV_power, and NEV_shunt arrays to zero;
    
    
    
    change bustype to SWING if an islanding event is triggered by reliability;
    

#### Sync()

SPECIAL NOTICE!!! Any islanding events need to be triggered before this function in order to determine if the node has become a new swing node. 
    
    
    if(I'm a load){
        aggregate NEV_current, NEV_power, and NEV_shunt arrays with input from constant_current, constant_power, and constant_impedance;
    }
    if(parent != NULL){
        aggregate into parent's NEV_current, NEV_power, and NEV_shunt with my NEV_current, NEV_power, and NEV_shunt;
    } else {
        if(bustype != SWING){
            search for nearest upstream swing bus;
            link to that NEVBUSDATA structure belonging to that swing bus;
        }
        update NEV_Y_FULL with NEV_shunt;
        formulate the Jacobian A, B, C, and D matrices for the NEV NR solver with NEV_current, NEV_power, and NEV_Y_FULL in sparse matrix format;
    if(bustype == SWING){
        call NEV NR solver;
        see
    

#### Postsync()
    
    
    if(parent != NULL)
        update NEV_voltages with parent's NEV_voltages;
    

### Load Convention

As was seen from the load example specific terminal connections are given when creating constant impedance, current, and power loads. However, the NEV NR solver needs all current and power injections described in a wye configuration. So any loads connected in a delta configuration need to be converted to a wye equivalent when formulating the Jacobian matrices. lets take an example. Say I have a load defined below. 
    
    
     object load {
         parent node1;
         terminals "1,2,3,4";	
         voltage "2400.0+0j,1,0; 0+0j,4,0;";
         nominal_voltage 2400.0;	//nominal voltage at node
         constant_current "$I_{12}$,1,2; $I_{13}$,1,3; $I_{23}$,2,3";
     }
    

This load's `NEV_current` array would look like this. 

$$\displaystyle{}\begin{bmatrix}
0+j0&I_{12}&I_{13}&0+j0\\ -I_{12}&0+j0&I_{23}&0+j0\\ -I_{13}&-I_{23}&0+j0&0+j0\\ 0+j0&0+j0&0+j0&0+j0 \end{bmatrix}$$

Converting all the loads to a Wye configuration would result in a `NEV_current` that looks like 

$$\displaystyle{}\begin{bmatrix}
0+j0&0+j0&0+j0&I_{12}+I_{13}\\ 0+j0&0+j0&0+j0&I_{23}-I_{12}\\ 0+j0&0+j0&0+j0&-I_{13}-I_{23}\\ -I_{12}-I_{13}&-I_{23}+I_{12}&I_{13}+I_{23}&0+j0 \end{bmatrix}$$

The Equations for determining Wye equivalent currents from delta configured constant current loads and constant power loads can be found below 

$$ \displaystyle{}I_{terminal_i-terminal_{neutral}}=\Sigma I_{terminal_i-terminal_j} + \Sigma [\frac{S_{terminal_i-termianl_j}}{V_{terminal_i}-V_{terminal_j}}]^*$$

## Multiple Swing Nodes

The NEV Solver will support the ability for multiple buses to represent infinite reference buses. More importantly, it will also support the ability for there to be multiple, independent powerflows within the same GLM (e.g., islands). These two scenarios will be handled through two independent approaches. 

Multiple swing nodes in the same powerflow will not require any specific changes or implementations. The TCIM-NR solver being deployed for the NEV implementation looks for swing-type nodes, and skips over their voltage update portion. This leaves them at the designated reference voltage and lets the powerflow iterate until a feasible solution is met. The current NR solver has an explicit check to make sure only one swing-designated node exists on the system, but that requirement will be relaxed to allow more than one swing node within the same topology. 

Multiple islands will be handled via a different mechanism. The `fault_check` object within powerflow will need to be altered to check for multiple continuous topologies. Utilizing the swing-designators and the `HAS_SOURCE` powerflow object flags, each distinct topology will be checked for connection to an electrical source. Powerflow feasibility will not be checked, so passing this step only indicates electrical topology feasibility, not necessarily powerflow validity. 

Source checking will start by traversing a particular topology until a source-capable bus is detected (either a swing-node or a `HAS_SOURCE` flagged node). This traversion, as well as the topological continuity check will utilize the information in the `NEVBUSDATA` and `NEVBRANCHDATA` structures. In particular, the `from` and `to` fields of the `NEVBRANCHDATA` and `matrix_association` field of `NEVBUSDATA` will be utilized. `fault_check` will parse the node/link structure of the GLM until all nodes designated as sources have been associated with a particular matrix. 

Due to the use of the `NEVBRANCHDATA` and `NEVBUSDATA` structures, islanding/separation operations are restricted to occurring only on `link` objects. Parent/child separations into islands will not be supported or allowed. All distinct islands in the system will utilize the same `NEVBUSDATA` and `NEVBRANCHDATA` structures. However, the sparse-formatted matrix values and other solver-specific variables (e.g., `deltaI` term) will be posted to distinct locations for each island. This location will be attributed to unique values in the `NEVBUSDATA` `matrix_association` field. Nodes will be responsible for posting to their correct matrix location. 

Calls to the NEV solver will be handled by the highest ranked object within powerflow during the `sync` pass of GridLAB-D™. To insure a single, highest-ranked object, all powerflow nodes will have an implicit parent with a new `nev_solver` class. This will not be a GLM-specifiable class, but will be automatically added any-time the NEV solver is specified. This `nev_solver` object will construct and call the NEV-based TCIM-NR for the number of distinct islands, determined by unique numbers in the `NEVBUSDATA` `matrix_association` field. NEV solver calls will no longer be conducted via the swing node of the powerflow (as was done in the current NR implementation). This will prevent any reconfiguration ranking issues, meshed topology issues, or overall user input issues from preventing proper execution of the NEV solver routine. 

As indicated elsewhere in the specifications, iteration handling and convergence checks will still be handled with global implications. That is, if a powerflow object of one island requests a timestamp reiteration, all powerflow objects will reiterate (based on the overall, GridLAB-D™ time iteration loop). Furthermore, if an individual island fails to converge or meets a singularity condition, this will fail the entire simulation. Methods to mitigate this complete failure are expected to come through other user functionality or objects. 

# See also

  * [Overview Page]
  * [Requirements]
  * [Specifications]
  * [Implementation]
  * [Keeler (Version 4.0)]

