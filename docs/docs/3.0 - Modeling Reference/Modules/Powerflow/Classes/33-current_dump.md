## Currdump

!!! warning
    This page was automatically generated and requires review.

This object allows the user to collect all of the currents in the system into one *.csv file at a given run time. In all cases, this is the current flowing INTO the link object (as defined by the to/from convention). Currents are placed in the *.csv output file with format: 

link_name,  | currA_real,  | currA_imag,  | currB_real,  | currB_imag,  | currC_real,  | currC_imag,   
---|---|---|---|---|---|---  
In rectangular   
**link_1**  | 10  | 0  | -5  | -8.66  | -5  | 8.66   
Or in polar (radians)   
**link_2** | 20  | 0 | 20 | -2.0944  | 20  | 2.0944   

### Currdump Parameters

#### Properties

**currdump** does not declare inherited parent classes.

Input indicates properties you can set in models. Output marks properties produced or modified during simulation runtime.

| Property Name | Type | Unit | Input | Output | Description |
| --- | --- | --- | --- | --- | --- |
| group | char32 | N/A |  | ✓ | Using the `group_id` feature, this allows only nodes with the matching `group_id` to be dumped into the output file. |
| runtime | timestamp | N/A | ✓ | ✓ | Tells the object at what time to output the currents of the system. Can be in either seconds from epoch (Unix time) or with a timestamp (&#x27;2006-01-01 00:00:00&#x27;). If not specified, the default is immediately after the first time step solution. |
| filename | char256 | N/A | ✓ |  | Tells the object what file to print all information to. While a *.csv is not necessary, it is recommended as the formatted output is in *.csv format. |
| runcount | int32 | N/A |  |  | ⚠️ the number of times the file has been written to |
| mode | enumeration | N/A | ✓ | ✓ | Allows the user to choose between polar and rectangular coordinates when printing output. Valid choices are &lt;br/&gt; - `rect` rectangular coordinates (default) &lt;br/&gt; - `polar` polar coordinates (in radians) Valid values: `RECT`, `POLAR`. |

### Default Current Dump

The minimal amount of code to specify a **currdump** object is 
    
    
    object currdump {
           filename output_current.csv;
           }
    

which will produce an output file of the given name in the format shown above, and will display the current of every link object in the glm file. 

### Current Dump State of Development

Current Dump is considered a well developed, but unvalidated model. Additional features may be included as needed. 
