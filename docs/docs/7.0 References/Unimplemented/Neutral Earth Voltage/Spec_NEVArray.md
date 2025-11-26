# Spec:NEVArray

!!! warning

	This page contains features that are unfinished, were never implemented, or have since been deprecated. We preserve these pages for archival purposes, and also as a foundational resource for prospective developers who may wish to implement the same or similar feature. Many of these pages provide robust explanations of the theory behind a particular module or feature that we hope readers will find useful. 
	
	**This page does not reflect the current state of GridLAB-D™**

This specification page outlines the formatting for array input data (GLM-based) for NEV objects in explicit detail. This will include the syntax flags and the approach for implementation within GridLAB-D™. 

# GLM Array Input Example

To accommodate the flexibility required for the NEV solver, many GLM-level inputs need to be specified in a form of array notation. An example with a load below, with the `constant_impedance` field, represents a constant impedance load specification across several terminals of the `node` object. The overall idea is passing in an array-like structure of data into the required powerflow object. 
    
    
     object load {
     	terminals "1,2,3";	//Three-phases
     	voltage "2400.0+0.0d,1,0; 2400.0-120.0d,2,0; 2400.0+120.0d,3,0";    //Wye, phase-ground specification of voltage
     	constant_impedance "2500+1200j,1,2; 2500+1200j,2,3; 2500+1200j,3,1"; //Implied delta-connected, balanced, constant impedance load
     	nominal_voltage 2400.0;						     //nominal voltage at node
     }
    

# Array Input Syntax

All array-like inputs into the powerflow module will be parsed in the GLM as character strings. Specific characters and sequences as defined in Table 1. 

##### Table 1 - Powerflow Array Parsing Reserved Characters  Character | Function   
---|---  
<comma> "," | column or sub-element separator in the array.   
<semicolon> ";" | row or element separator in the array.   
  
Spaces before or after the individual delimiters will be ignored. Spaces in text fields (i.e., object names) will be supported. Spaces in numeric values will be expected to precede unit specifications on that value, so the appropriate [GLD Unit] conversions can be performed. 

# Array Input Parsing

Array inputs will be parsed by a routine within the powerflow module directly. As part of this separation, core functionality ties like schedules, schedule transforms, and player transforms will need to be handled. These specific items will be caught during the initialization and population of the fields within the specific objects. All of these array types will be of varying size, dependent on the terminal count of objects and the actual fields that need population. For simplicity, all arrays are expected to be allocated as single-dimensional, with appropriate index-offsets calculated by the object reading the array (e.g., a $3\times{}3$ matrix is allocated as a $9\times{}1$ array, with indexing similar to $3\times{}$`row_index` \+ `column_index`). Implementations should be row-major, with single-dimensional arrays effectively having a `row_index` of zero. 

## Integer array

Integer arrays are utilized in the terminal specifications for `node` objects and for connecting terminal information in `link` objects. This input is expected to be two-dimensional only in `link` objects where an implicit neutral specification is required (e.g., concentric neutral or tape-shielded underground cables). 
    
    
     typedef struct {
     	int *integer_data;	//Dynamic array of actual integer values
     	int num_rows;		//Number of rows of the array
     	int num_cols;		//Number of columns of the array
     } NEV_INT_ARRAY;
    

## Double array

Double arrays are utilized in the distance specifications for `link` objects. This input is expected to always be two-dimensional. The two dimensional structure will either be a full $(n+1)\times{}(n+1)$ matrix of distances (full or strictly upper triangular), or an $n\times{}2$ matrix of Cartesian coordinate pairs, where $n$ is the number of conductors. Both input types will fit into the following structure, with a distinction between "pure distance" and "Cartesian coordinates" handled in the individual objects (e.g., `overhead_line`). 
    
    
     typedef struct {
     	double *double_data;		//Dynamic array of actual double values
     	int num_rows;			//Number of rows of the array
     	int num_cols;			//Number of columns of the array
     } NEV_DOUBLE_ARRAY;
    

Full matrix input parsing will need to distinguish between full-matrix specification and upper-triangular specification. Examples of both types of inputs and how they will be stored in memory are in the next subsection. 

### Full matrix specification

A full matrix specification for a $3\times{}3$ matrix could look like: 
    
    
     link_impedance "1,2,3; 4,5,6; 7,8,9";
    

which would be read in to represent a matrix of $\begin{bmatrix} 1 & 2 & 3\\ 4 & 5 & 6\\ 7 & 8 & 9 \end{bmatrix}$

### Upper triangular matrix specification

All upper triangular specifications for line spacing are expected to be strictly upper triangular matrix. Despite the different between upper triangular and strictly upper triangular, both will be parsed in a similar fashion. An example of an upper triangular $3\times{}3$ matrix could look like: 
    
    
     link_distance "1,2,3; 4,5; 6";
    

which would be read in to represent a matrix of $\begin{bmatrix} 1 & 2 & 3\\ 0 &4 & 5 \\ 0 & 0 & 6 \end{bmatrix}$

It is important to note that the interpretation of this as upper triangular or strictly upper triangular will be left to the object utilizing the array. 

## Complex array

Complex arrays are utilized in the direct impedance specifications for `link` objects. This input is expected to always be two-dimensional and square. 
    
    
     typedef struct {
     	complex *complex_data;		//Dynamic array of actual complex values
     	int num_rows;			//Number of rows of the array
     	int num_cols;			//Number of columns of the array
     } NEV_COMPLEX_ARRAY;
    

## Object array

Object arrays are utilized in the list of spacing or conductor objects for `link` objects. This input is expected to always be one-dimensional. 
    
    
     typedef struct {
     	OBJECT *object_data;		//Dynamic array of object pointers
     	int num_rows;			//Number of rows of the array
     } NEV_OBJECT_ARRAY;
    

## Terminal bridging array

Terminal bridging arrays are utilized whenever a value is needed to be defined between two terminal points. Voltage and load specifications inside `node` objects utilize this format. This input is expected to always produce one-dimensional arrays. 
    
    
     typedef struct {
     	complex *complex_data;		//Dynamic array of complex values
     	int *terminal_A;		//Dynamic array of first terminal of connection
     	int *terminal_B;		//Dynamic array of second terminal of connection
     	int num_rows;			//Number of rows of the arrays
     } NEV_COMPLEXTERMINAL_ARRAY;
    

# Time-varying data I/O

To maintain maximum usability and flexibility for different distribution analysis scenarios, the NEV array specifications need to be able to utilize data sources like schedules and players, as well as data sinks like recorders. Implementation will be handled via the additional class properties added in the GLM file, similar to how the market module determines statistical intervals to calculate. 

Players, recorders, and schedules will all interact with the array properties of the NEV objects through these class properties. For players, recorders, and schedules functionality is expected to already exist. For player and schedule transforms, verification of this functionality with class properties needs to be verified and possibly extended. 

A simple example of the class properties approach is shown below. The syntax for the declarations and available variables will be shown in the next subsections. 
    
    
     clock {
     	timezone "PST+8PDT";
     	starttime '2001-01-01 00:00:00 PST';
     	stoptime '2001-01-08 00:00:00 PST';
     }
     
     module powerflow {
     	solver_method NEV;
     };
     
     class node {
     	complex voltage_1_2;	//Voltage between terminals 1 and 2
     	complex voltage_2_3;	//Voltage between terminals 2 and 3
     	complex voltage_3_1;	//Voltage between terminals 3 and 1
     	complex voltage_1_0;	//Voltage between terminals 1 and ground
     }
     
     object node {
     	terminals "1,2,3";
     	name swing_bus;
     	bustype SWING;
     	voltage "2400.0+0.0d,1,0;
     			 2400.0-120.0d,2,0;
     			 2400.0+120.0d,3,0";
     	nominal_voltage 2400.0;
     }
     
     object player {
     	parent swing_bus;
     	property voltage_1_0;
     	file voltage_player_input.csv;
     }
     
     object recorder {
     	parent swing_bus;
     	property "voltage_1_2,voltage_2_3,voltage_3_1,voltage_1_0";
     	interval 30;
     	file voltage_recorder_output.csv;
     }
    

## Class property syntax

All class-level definitions of properties are expected to follow the format of: 
    
    
     type property_terminal;
     type property_terminal_terminal;
    

where the typing is the overall C++/GridLAB-D™ data type, and the terminals represent the connecting point of the properties. All class-property declarations are expected to propogate into derived classes. For example, `voltage_2_1` declared in the `node` class should also be available to `load` objects. 

Complex-valued properties will also support `_real`, `_reac`, and `_imag` designations to capture the real or imaginary portions of the fields. These values will be appended after the terminal designation, with an example mapping to the reactive portion of the voltage between terminals 1 and 2 looking like: 
    
    
     class node {
     	complex voltage_1_2_imag;
     }
    

Note that the syntax provided is using underscores to separate the terminal fields. It should be noted that this does not reserve the underscore from other uses, especially in names or other properties. Exact matches will be required on property names as well. For example, a property definition of `voltage_1_2` will not interfere with `nominal_voltage_1_2` (note the latter property doesn't exist, this was just an example). 

## Valid property prefixes

The class-defined published properties approach for interfacing NEV objects with recorders, players, and schedules will only support a specific subset of parameters in both `node` and `link` objects. Other definitions will be treated identically to the existing class-property framework in that they will not be updated by any internal mechanisms of the `node` or `link` objects. 

### Node property prefixes

`node` objects will automatically update the base property types listed in Table 2. Note that all `node` class-property definitions will require two terminals. If a property value between a terminal and ground is desired, the number "0" must be entered for the second terminal. Single terminal definitions will not be parsed. All class-defined properties are complex valued. 

##### Table 2 - Node object properties available for class definition  Property | Definition   
---|---  
`voltage` | Voltage values between two terminals   
  
### Load property prefixes

`load` objects will be able to map the properties of Table 3. These are also all complex valued properties. `load` objects will also have access to the property defined in Table 2. 

##### Table 3 - Complex-valued load object properties available for class definition  Property | Definition   
---|---  
`constant_power` | Constant power load specification between two terminals   
`constant_current` | Constant current load specification between two terminals   
`constant_impedance` | Constant impedance load specification between two terminals   
`base_power` | Base power value for ZIP-percentage-specified load between two terminals   
  
`load` objects will also have access to the properties of Table 4. These properties are only double-valued, not complex. 

##### Table 4 - Double-valued load object properties available for class definition  Property | Definition   
---|---  
`power_pf` | Power factor for constant power fraction of ZIP-percentage-specified load on two terminals   
`current_pf` | Power factor for constant current fraction of ZIP-percentage-specified load on two terminals   
`impedance_pf` | Power factor for constant impedance fraction of ZIP-percentage-specified load on two terminals   
`power_fraction` | Fraction of the full `base_power` portion of ZIP-percentage-specified load for constant power on two terminals   
`current_fraction` | Fraction of the full `base_power` portion of ZIP-percentage-specified load for constant current on two terminals   
`impedance_fraction` | Fraction of the full `base_power` portion of ZIP-percentage-specified load for constant impedance on two terminals   
  
### Meter property prefixes

`meter` objects will be able to map the properties of Table 5. These are all complex-valued properties. `meter` objects will also have access to the property defined in Table 2. 

##### Table 5 - Meter object properties available for class definition  Property | Definition   
---|---  
`measured_power` | Measured power dissipation between two terminals   
`measured_voltage` | Measured voltage between two terminals (identical to `voltage` values)   
`measured_current` | Measured current between two terminals   
  
### Link property prefixes

`link` objects will automatically update the base property types listed in Table 6. Note that all `link` class-property definitions will require two terminals to determine which conductor/cable the property is measuring. Single terminal definitions will not be parsed. Note that for all definitions, "in" is defined as the from-node connected side and "out" is the to-node connected side, regardless of actual flow direction. For example, 
    
    
     class link {
     	complex power_in_2_3;
     }
    

would publish the power values associated with current flowing on a conductor between terminal 2 of the from node and terminal 3 of the to node. The power would be signed such that it is associated with the current flowing into the conductor from the terminal 2 connection point (from side). 

##### Table 6 - Link object properties available for class definition  Property | Definition   
---|---  
`power_in` | Measured power flowing into a cable/conductor, using the from/to convention described   
`power_out` | Measured power flowing out of a cable/conductor, using the from/to convention described   
`power_losses` | Estimated power losses on a particular cable/conductor   
`current_in` | Measured current flowing into a cable/conductor, using the from/to convention described   
`current_out` | Measured current flowing out of a cable/conductor, using the from/to convention described   
  
## Class property timing

In order to maintain proper functionality with recorder and player objects, reading and writing the values of the class-defined properties needs to be sequenced properly. Player objects currently execute during the presync pass, so all reads of such properties should occur first thing in the sync routine. Recorder objects currently execute during the postsync pass, so all updates to published properties should be completed by the end of an object's postsync call. 

# See also

  * [Overview Page]
  * [Requirements]
  * [Specifications]
  * [Implementation]
  * [Keeler (Version 4.0)]
