## Restoration

As the **powerflow** module interacts with the **reliability** module, portions of the system may become isolated. The **restoration** object attempts to do feeder reconfiguration to close the isolated sections back into the system. The **restoration** object requires **reliability** or some reliability-like actions to function properly, as well as the **fault_check** object. The **restoration** object only works with the `NR` solver method at this time. 

A **restoration** object can be implemented as: 
    
    
    object restoration {
    	name RestorVal;
    	reconfig_attempts 3;
    	reconfig_iteration_limit 5;
    	populate_tree TRUE;
    	}
    

### Restoration Parameters

Property Name  | Type  | Unit  | Description   
---|---|---|---  
**reconfig_attempts**  | double  | Tries  | Number of reconfiguration attempts a system will perform before giving up and determining the system can not be fully restored at that condition.   
**reconfig_interation_limit**  | double  | count  | Number of **powerflow** iterations a particular reconfiguration can try before failing. Used to prevent infinite iterating by the solver.   
**populate_tree**  | boolean  | N/A  | Flag to populate the tree structure of the feeder. Used for the algorithm implmeented to increase reconfiguration iterations and attempts. Should be set to `TRUE` whenever `reconfiguration` is used.   
  
### Restoration State of Development

The **restoration** object is considered highly experimental at this time. 

