## Restoration

!!! warning
    This page was automatically generated and requires review.

As the **powerflow** module interacts with the **reliability** module, portions of the system may become isolated. The **restoration** object attempts to do feeder reconfiguration to close the isolated sections back into the system. The **restoration** object requires **reliability** or some reliability-like actions to function properly, as well as the **fault_check** object. The **restoration** object only works with the `NR` solver method at this time. 

A **restoration** object can be implemented as: 
    
    
    object restoration {
    	name RestorVal;
    	reconfig_attempts 3;
    	reconfig_iteration_limit 5;
    	populate_tree TRUE;
    	}

### Restoration Parameters

#### Properties

**restoration** does not declare inherited parent classes.

Input indicates properties you can set in models. Output marks properties produced or modified during simulation runtime.

| Property Name | Type | Unit | Input | Output | Description |
| --- | --- | --- | --- | --- | --- |
| reconfig_attempts | int32 | N/A | ✓ |  | Number of reconfiguration attempts a system will perform before giving up and determining the system can not be fully restored at that condition. |
| reconfig_iteration_limit | int32 | N/A | ✓ |  | ⚠️ Number of iterations to let PF go before flagging this as a bad reconfiguration |
| source_vertex | object | N/A | ✓ | ✓ | ⚠️ Source vertex object for reconfiguration |
| faulted_section | object | N/A | ✓ | ✓ | ⚠️ Faulted section for reconfiguration |
| feeder_power_limit | char1024 | N/A | ✓ |  | ⚠️ Comma-separated power limit (VA) for feeders during reconfiguration |
| feeder_power_links | char1024 | N/A | ✓ |  | ⚠️ Comma-separated list of link-based objects for monitoring power through |
| feeder_vertex_list | char1024 | N/A | ✓ |  | ⚠️ Comma-separated object list that defines the feeder vertices |
| microgrid_power_limit | char1024 | N/A | ✓ |  | ⚠️ Comma-separated power limit (complex VA) for microgrids during reconfiguration |
| microgrid_power_links | char1024 | N/A | ✓ |  | ⚠️ Comma-separated list of link-based objects for monitoring power through |
| microgrid_vertex_list | char1024 | N/A | ✓ |  | ⚠️ Comma-separated object list that defines the microgrid vertices |
| lower_voltage_limit | double | pu | ✓ |  | ⚠️ Lower voltage limit for the reconfiguration validity checks - per unit |
| upper_voltage_limit | double | pu | ✓ |  | ⚠️ Upper voltage limit for the reconfiguration validity checks - per unit |
| output_filename | char1024 | N/A | ✓ |  | ⚠️ Output text file name to describe final or attempted switching operations |
| generate_all_scenarios | bool | N/A | ✓ |  | ⚠️ Flag to determine if restoration reconfiguration and continues, or explores the full space |

### Restoration State of Development

The **restoration** object is considered highly experimental at this time. 
