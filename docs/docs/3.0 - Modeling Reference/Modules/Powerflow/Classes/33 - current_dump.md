## Current Dump

This object allows the user to collect all of the currents in the system into one *.csv file at a given run time. In all cases, this is the current flowing INTO the link object (as defined by the to/from convention). Currents are placed in the *.csv output file with format: 

link_name,  | currA_real,  | currA_imag,  | currB_real,  | currB_imag,  | currC_real,  | currC_imag,   
---|---|---|---|---|---|---  
In rectangular   
**link_1**  | 10  | 0  | -5  | -8.66  | -5  | 8.66   
Or in polar (radians)   
**link_2** | 20  | 0 | 20 | -2.0944  | 20  | 2.0944   
  
### Default Current Dump

The minimal amount of code to specify a **currdump** object is 
    
    
    object currdump {
           filename output_current.csv;
           }
    

which will produce an output file of the given name in the format shown above, and will display the current of every link object in the glm file. 

### Current Dump Parameters

Property Name  | Type  | Unit  | Description   
---|---|---|---  
**filename**  | char32  | N/A  | Tells the object what file to print all information to. While a *.csv is not necessary, it is recommended as the formatted output is in *.csv format.   
**group**  | char32  | N/A  | Using the `group_id` feature, this allows only nodes with the matching `group_id` to be dumped into the output file.   
**runtime**  | timestamp  | N/A  | Tells the object at what time to output the currents of the system. Can be in either seconds from epoch (Unix time) or with a timestamp ('2006-01-01 00:00:00'). If not specified, the default is immediately after the first time step solution.   
**mode**  | enumeration  | N/A  | Allows the user to choose between polar and rectangular coordinates when printing output. Valid choices are <br/> - `rect` rectangular coordinates (default) <br/> - `polar` polar coordinates (in radians)
  
### Current Dump State of Development

Current Dump is considered a well developed, but unvalidated model. Additional features may be included as needed. 

