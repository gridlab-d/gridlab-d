# Spec:NEVModules

!!! warning

	This page contains features that are unfinished, were never implemented, or have since been deprecated. We preserve these pages for archival purposes, and also as a foundational resource for prospective developers who may wish to implement the same or similar feature. Many of these pages provide robust explanations of the theory behind a particular module or feature that we hope readers will find useful. 
   
	**This page does not reflect the current state of GridLAB-D™**

This specification page outlines the interfacing with existing modules and devices external to the powerflow module. Examples include inverters in the generators module and houses in the residential module. Eventually, these devices will explicitly interface with the NEV capabilities. This specification details how both legacy compatibility/interfacing will occur, as well as interface points for new, "NEV-native" devices will work. 

# Legacy Implementation

Existing powerflow interfaces occur via pointers to the voltage properties of GridLAB-D™ (`voltage_A` ... `voltage_C` or `voltage_1` ... `voltage_12`) and some form of load interface. This load interface varies between contributions to the direct postings to the `current_x`, `shunt_x`, and `power_x` fields. Unless explicitly tied into the newer implementation method specified below, the existing external objects will be ignorant of the NEV implementation and continue to post load contributions in the traditional manner. 

`DELTAMODE`-enabled objects will also utilize the legacy notation, even though the values are posted through the separate `DynCurrent` variable. The notation outlined below will be used to define the traditional phase A, B, and C connection terminals for `DELTAMODE` objects. 

## GLM Inputs

On the individual objects, GLM inputs will not be any different. Objects will still parent or specify their connection to a powerflow object in the same manner they have under existing FBS/NR solver methodology. However, the terminal specification implied for the legacy 0, 1, and 2 indices of the current, shunt, and power arrays (corresponding to phase A, B, and C under three-phase connections) need to be specified on the connecting node. 

Connecting terminals for the traditional objects will be specified through the `legacy_load_phases` property. In the example below, terminals 1, 3 and 7 of the node are mapped to entries 0, 1, and 2 of the old implementation (so terminal 1 is implied as phase A, terminal 3 is implied phase B, and terminal 7 is implied phase C). 
    
    
     object node {
        name trad_load_connection;
        terminals 1,2,3,5,6,7;
        nominal_voltage 2401.77;
        legacy_load_phases "1,3,7";
     }
    

Triplex-connected devices will have a similar implementation, but only two entries are required (corresponding to L1 and L2). The example below would be ready for a residential module house connection into the NEV system onto terminals 1 (implied L1) and 7 (implied L2).: 
    
    
     object triplex_node {
        name house_goes_here;
        terminals "1,7,9";	//9 is the neutral
        nominal_voltage 120.0;
        legacy_load_phases "1,7";
     }
    

## Programming Considerations

In both cases described above, the corresponding contributions for current, power, and admittance/impedance will be posted to the appropriate terminal phase inside the node. `DELTAMODE`-enabled devices will also have their corresponding values translated to the appropriate terminals internally. All "traditionally posted" values will be incorporated into the multi-phase power, current, and impedance contributions as outlined in the [Node Specification], as part of the `Y_update_fxn` call from the NEV solver. 

# NEV-native Implementation

For newer implementations, or implementations requiring explicit NEV capabilities (e.g., modeling of neutral currents), the corresponding terminal connections will need to be implemented. 

## GLM Inputs

Individual objects tying into the new NEV-solver method will need to explicitly specify connecting terminals. This will be left to individual objects to determine the formatting, but values corresponding to the terminal value will need to be passed back to powerflow. Consider a simple three-phase inverter. The `connected_terminals` property will indicate which terminals on the node object the inverter should connect. In this case, terminals 1, 5, 9, and 12 are connected to the inverter in a Wye-connected fashion (delta is shown below). It is left to the internal programming and specifications of the inverter to interpret this sequence (this case may be A, B, C, and N). The only powerflow requirement is indicating the connection point of the phases. 
    
    
     object inverter {
        name NEV_connected_Wye;
        rated_VA 25000;
        connected_terminals "1,5,9,12";
     }
    

For a delta, or phase-to-phase, connected load, the connection terminals need to be specified in pairs. Suppose a three-phase motor is delta-connected and requires posting its contributions in the delta or phase-to-phase sense. An example implementation (not implemented in GridLAB-D™, only as an example here) could be in the form shown below. Much like the last example, this motor is connecting to terminals 1, 5, and 9 on the node. However, contributions will be posted in the phase-to-phase portions of the node's loading, not in the phase-to-earth portions. This will allow the values to be properly updated as the NEV solver iterates, rather than having "Wye-equivalent posted values" that will change on each GridLAB-D™ timestep. 
    
    
     object motor {
        name NEV_connected_delta;
        rated_HP 25.0;
        connected_terminals "1,5; 5,9; 9,1";
     }
    

## Programming Considerations

The key part of the program implementation is how the individual load quantities are posted by the powerflow-external object. Many of these objects assume the requested phases occur in sequence, so only the first block of the array is mapped. For example, the current implementation of `diesel_dg` only maps an address to `current_A` with an array `pCurrent`. It then assumes that `pCurrent[1]` refers to phase B, since phase B would immediately follow the mapped phase A value. NEV-explicit implementations will no longer be able to make this assumption and will have to have pointers to the appropriate quantity adjusted. 

For specific load pointers, proper posting locations will need to be determined based on the number of terminals and the connections. For example, if an inverter is connected to terminal 2's constant power load, but the node only has terminal 2, it needs to know to connect to the first entry of the constant power load. This connection consideration (as well as any delta-connected redirections) will be handled during the variable mapping phase. Read variables (e.g., voltage) will also need to be mapped in a similar fashion. 

The direct NEV-powerflow-related programming consideration is returning these addresses to the specific load contribution quantities. These phases will be mapped via individual pointers to the individual terminals. No "sequence assumptions" will be allowed. To enable multithreading capabilities, all posting to these pointers will be handled via the new transactional memory API. Aside from the explicit pointers and API calls, object behavior should remain identical. 

For DELTAMODE-connected objects, similar considerations will need to be made for `deltaI` and direct admittance posted values. Exposed variables in the node will still be mapped to as they are now, but explicit pointers will be needed for each terminal and will need to be adjusted for the connecting terminals' location within the overall terminal structure of that particular node. As with the normal load interface, individual terminal locations will be referred to with direct pointers and using the transactional memory API. 

# Related Concepts:

  * [Overview Page]
  * [Requirements]
  * [Specifications]
  * [Implementation]
  * [Keeler (Version 4.0)]
