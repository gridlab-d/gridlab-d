## Recloser

**recloser** objects are a special type of **[switch](26-switch.md)** that open at the detection of a fault condition and will close if the fault condition is removed or isolated within a certain period of time. The time is typically determined by the number of closing tries and the time between tries. **recloser** objects work with both the `FBS` and `NR` solver methods, but their reliability functionality only works with the `NR` method. A typical recloser implementation is 
    
    
    object recloser {
        name recloser_2;
        phases "ABCN";
        from node_2a;
        to node_2b;
        retry_time 1s;
        max_number_of_tries 3;
    }
    

Recloser objects inherit for switch and therefore share all parameters belonging to both **[switch](26-switch.md)** and **[link](04-link.md)**. **recloser** objects are exercised only by the **reliability** module at this time. 

### Recloser Parameters

#### Properties

**recloser** objects are derived from **[switch](26-switch.md)** objects, so any parameters of the **[switch](26-switch.md)** object are available as well.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

Table: recloser table 1 { #tbl:27-recloser-1 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| **retry_time** | double | s | I | The time to wait in seconds before trying to close after a fault condition is detected. This parameter is unused at this time and is put in place for future functionality. |
| **max_number_of_tries** | double | N/A | I | The number of times the recloser will try to close before permanently opening |
| **number_of_tries** | double | N/A | IO | Number of tries a **recloser** has been actuated in the current fault condition. |

### Recloser State of Development

Recloser is considered a validated model in terms of powerflow solutions and **reliability** integration. However, testing has been limited and feature additions may still be necessary. 
