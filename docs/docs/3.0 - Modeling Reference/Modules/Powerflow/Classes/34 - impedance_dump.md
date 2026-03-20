## Impedance Dump

Impedance dump allow the impedance and line equation matrices to be output into an XML file for debugging or further use. 

The output format of the impedance dump fits the line equation format of William Kersting's "Distribution System Modeling and Analysis" book. I.e., the `impedance_dump` outputs the $\displaystyle{}a, b, c, d, A, B$ matrices of the equations:

$$\displaystyle{} \begin{bmatrix} VLG_{in} \\\ I_{in} \end{bmatrix} = \begin{bmatrix} a & b\\\ c & d\end{bmatrix}\cdot{}\begin{bmatrix}VLG_{out} \\\ I_{out}\end{bmatrix}$$

$$\displaystyle{}VLG_{out} = A\cdot{}VLG_{in}-B\cdot{}I_{in}$$

with the $\displaystyle{}b$ matrix typically representing the traditional impedance of a line. 

### Default Impedance Dump

The minimal specifications for **impedence_dump** are 
    
    
    object impedance_dump {
    	filename impedance_output.xml;
    }
    

Like other "dump" objects in powerflow, additional parameters can be added to describe when to run (runtime) and for only link objects with a specific groupid: 
    
    
    object impedance_dump {
        group "class=overhead_line";
        runtime '2020-12-11 01:00:00';
        filename impedance_output.xml;
    }
    

### Impedance Dump Parameters

Property Name  | Type  | Description   
---|---|---  
**filename**  | char256  | Tells the object what file to print all information to. The file will be an XML-formatted text file.   
**group**  | char32  | Using the `groupid` feature, specific objects to be included in the output file. If left blank, all link-based objects will be part of the output file.   
**runtime**  | timestamp  | Tells the object at what time to output the impedance dump. Can be in either seconds from epoch (Unix time) or with a timestamp ('2006-01-01 00:00:00'). If not specified, the default is immediately after the first time step solution.   
**runcount**  | int32  | Indicates the number of times the impedance dump has executed (See Remarks).

!!! remarks

	Currently, the `impedance_dump` object will only execute once, at the selected runtime. If multiple dumps are desired, a player or some other interface would need to update the `runtime` and reset the `runcount` values. Also note that the file in `filename` will be overwritten, so if this route is used, something must externally rename or copy the XML prior to running the next impedance dump.
  
### Impedance Dump State of Development

Impedance Dump is considered a well developed, but unvalidated model. Additional features may be included as needed. 

