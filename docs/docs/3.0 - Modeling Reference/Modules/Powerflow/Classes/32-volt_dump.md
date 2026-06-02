## Voltdump

This object allows the user to collect all of the voltages in the system into one CSV file at a given run time. This can be used to determine the cause of convergence problems. Voltages are placed in the CSV output file with format:

node_name,  | voltA_real,  | voltA_imag,  | voltB_real,  | voltB_imag,  | voltC_real,  | voltC_imag,   
---|---|---|---|---|---|---  
**node_1**,  | 7200,  | 0,  | -3600,  | -6235.4,  | -3600,  | 6235.4   
**node_2**,  | 2400,  | 0,  | -1200,  | -2078.5,  | -1200,  | 2078.5   

### Voltdump Parameters

#### Properties

**voltdump** does not declare inherited parent classes.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

Table: volt_dump table 1 { #tbl:32-volt-dump-1 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| **group** | char32 | N/A | I | Using the `group_id` feature, this allows only nodes with the matching `group_id` to be dumped into the output file. |
| **runtime** | timestamp | N/A | IO | Tells the object at what time to output the voltages of the system. Can be in either seconds from epoch (Unix time) or with a timestamp ('2006-01-01 00:00:00'). If not specified, the default is immediately after the first time step solution. |
| **filename** | char256 | N/A | I | Tells the object what file to print all information to. While a CSV extension is not necessary, it is recommended as the formatted output is CSV. |
| **file** | char256 | N/A | I | The file to dump the voltage data into |
| **runcount** | int32 | N/A | — | The number of times the voltages have been dumped. |
| **mode** | enumeration | N/A | I | Allows the user to choose between polar and rectangular coordinates when printing output. Valid choices are <br/> - `rect` rectangular coordinates <br/> - `polar` polar coordinates (default - in radians) Valid values: `RECT`, `POLAR`. |

### Default Volt Dump

The minimal amount of code to specify a **voltdump** object is 
    
    
    object voltdump {
           filename output_voltage.csv;
           }
    

which will produce an output file of the given name in the format shown above, and will display the voltage of every node in the glm file. 

### Volt Dump State of Development

Volt Dump is considered a well developed, but unvalidated model. Additional features may be included as needed. 


