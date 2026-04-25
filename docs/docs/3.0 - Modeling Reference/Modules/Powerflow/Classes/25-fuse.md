## Fuse

!!! warning
    This page was automatically generated and requires review.

Fuse objects are used to place a current limitation between two nodes. If the current is exceeded, the fuse will open and prevent further current flow. Due to limitations in the Forward-Back Sweep algorithm, fuses only affect the first downstream node. If other loads exist downstream, they will cause an oscillatory voltage swing that has no real representation. **reliability** module functionality only exists in the Newton-Raphson solver at this time. A minimalist **fuse** could be implemented as 
    
    
    object fuse {
    	 phases "ABC";
    	 name node1-node2;
    	 from node1;
    	 to node2;
    	 }
    

A typical **fuse** object would be implemented, with the same parameters as above, as 
    
    
    object fuse {
     	phases "ABC";
     	name node1-node2;
     	from node1;
     	to node2;
     	current_limit 9999.0 A;
     	mean_replacement_time 3600.0;
    	repair_dist_type NONE;
     	}

### Fuse Parameters

#### Properties

**fuse** objects are derived from **[link](04-link.md)** objects, so any parameters of the **[link](04-link.md)** object are available as well.

Input indicates properties you can set in models. Output marks properties produced or modified during simulation runtime.

| Property Name | Type | Unit | Input | Output | Description |
| --- | --- | --- | --- | --- | --- |
| phase_A_status | enumeration | N/A | ✓ | ✓ | Status of the fuse on phase `A` (only valid if phase `A` is in the `phases` property). Two keywords are valid: &lt;br/&gt; - GOOD - The fuse on phase `A` is still conducting and has not exceeded its current limit. &lt;br/&gt; - BLOWN - The fuse on phase `A` has exceeded its current limit and is no longer conducting. |
| phase_B_status | enumeration | N/A | ✓ | ✓ | Status of the fuse on phase `B` (only valid if phase `B` is in the `phases` property). Two keywords are valid: &lt;br/&gt; - GOOD - The fuse on phase `B` is still conducting and has not exceeded its current limit. &lt;br/&gt; - BLOWN - The fuse on phase `B` has exceeded its current limit and is no longer conducting. |
| phase_C_status | enumeration | N/A | ✓ | ✓ | Status of the fuse on phase `C` (only valid if phase `C` is in the `phases` property). Two keywords are valid: &lt;br/&gt; - GOOD - The fuse on phase `C` is still conducting and has not exceeded its current limit. &lt;br/&gt; - BLOWN - The fuse on phase `C` has exceeded its current limit and is no longer conducting. |
| repair_dist_type | enumeration | N/A | ✓ | ✓ | Distribution to be used after a fuse has blow to restore it. Current valid settings are: &lt;br/&gt; - `NONE` \- No distribution is used and the value in `mean_replacement_time` is taken directly. &lt;br/&gt; - `EXPONENTIAL` \- An exponential distribution is used with `mean_replacement_time` taken as one over the lambda value. |
| current_limit | double | A | ✓ |  | Current rating for the fuse. If exceeded, the particular phase will go to an open circuit condition. |
| mean_replacement_time | double | s | ✓ |  | Mean time to replace the fuse if blown. This could represent a travel requirement (remote location) or scarcity requirement (shipping time for parts). This value overrides any value specified in `mean_repair_time` for the **link** object itself. |
| fuse_resistance | double | Ohm | ✓ |  | ⚠️ The resistance value of the fuse when it is not blown. |

### Fuse State of Development

Fuse is considered a well developed and validated model in terms of powerflow solutions. Reliability functionality has been tested and validated, but is not fully vetted and may change as advanced functionality is included. 
