# Fault Check

The **fault_check** object performs "support/islanding checks" on objects inside the **powerflow** module. Its primary purpose is to determine if a particular node or link is still in service after a reconfiguration or fault event. **fault_check** is set up to operate as an independent topology checking object, but does have ties to the **reliability** module and the **restoration** object's functionality. The **fault_check** object only works with the `NR` `solver_method` at this time. Other solvers may be incorporated at a later date.

For detailed information about reliability analysis and fault scenario setup, see the [Reliability User Guide](../../Reliability/Reliability_User_Guide.md). A typical **fault_check** object would be implemented as 
    
    
    object fault_check {
    	name fault_check_obj;
    	check_mode ONCHANGE;
    	output_filename outage_check.txt;
    	}
    

As with other objects, not all of the parameters need to be specified. A minimal implementation would be similar to 
    
    
    object fault_check {
    	name fault_check_obj;
    	}

### Fault Check Parameters

#### Properties

**fault_check** does not declare inherited parent classes.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

Table: fault_check table 1 { #tbl:36-fault-check-1 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| **check_mode** | enumeration | N/A | I | Defines the fault checking scheme utilized. Valid entries are <br/> - `SINGLE` \- Fault checks and voltage support checks are only performed once, before the first solver pass of the `NR` solver. <br/> - `ONCHANGE` \- Fault checks and voltage support checks are performed any time an admittance change is flagged on the `NR` solver. <br/> - `ALL` \- Fault checks and voltage support checks are performed on every iteration <br/> - `SINGLE_DEBUG` \- Fault checks and voltage support checks are only done once, at the very beginning of the simulation. This mode will terminate the simulation after doing the initial fault check. This mode also bypasses several "critical errors" that would ordinarily terminate the simulation immediately, to allow for debugging the topology better (e.g., an islanded single node). <br/> - `SWITCHING` \- Special variant of `ONCHANGE` used with manipulation of **switch** objects and external interfaces. May be deprecated in the future. |
| **output_filename** | char1024 | N/A | I | File name for the text file of support check status values. Will output the bus number, phase information, and island association (if relevant) at each timestamp the **fault_check** object runs. If `full_output_file` is set, this will include both supported and unsupported node lists. |
| **reliability_mode** | bool | N/A | I | Boolean flag to indicate if the **fault_check** object is running in a **reliability**-module-based mode. **reliability** will toggle this mode and it is only provided for user information, it is not a specifiable property. |
| **strictly_radial** | bool | N/A | IO | Boolean flag to indicate which topology checking algorithm to use. Defaults to `true`, indicating a radial system is represented. If set `true`, line from/to fields are explicitly utilized. If set to `false`, a algorithm supporting meshed topologies and arbitrary from/to designations is supported. Note: the meshed topology check is more exhaustive and slower, hence the option for when feeders are known to 100% be radial. |
| **full_output_file** | bool | N/A | I | Boolean flag to toggle what gets written to `output_filename`. Defaults to `false`, which will only output unsupported nodes in the output. If set to `true`, it will provide a list of unsupported and supported nodes. Useful with `grid_association` to see what island a node is associated. |
| **grid_association** | bool | N/A | I | Boolean flag to indicate if the island association checking is performed. This option must be enabled in multiple islands or multiple independent SWING node scenarios. If `false`, only the "primary SWING" node will be checked for topology continuity -- all other SWING-type nodes will be ignored and removed from service (if not connected to the primary SWING topology). When enabled, overrides `strictly_radial` to `false`. |
| **eventgen_object** | object | N/A | I | Object link to an `eventgen` object in the **reliability** module. This object will be used to implement any "unscheduled" faults, such as switch openings or fuses blowing. Without this object specified, such objects will still open or trip, but may result in an unsolvable system matrix and prematurely terminate the simulation. |

### Fault Check State of Development

The **fault_check** object has been rigorously tested for topology checking and islanding operations and is considered well-developed and validated on that functionality. The original implementation was developed in conjunction with the **reliability** module and the **restoration** object. That functionality has been tested and is considered validated, but rigorous testing has not been conducted and additional features may be added at a future date. 
