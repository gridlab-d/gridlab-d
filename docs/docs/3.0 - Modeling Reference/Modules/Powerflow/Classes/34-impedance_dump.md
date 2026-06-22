# Impedance Dump

Impedance dump allow the impedance and line equation matrices to be output into an XML file for debugging or further use. 

The output format of the impedance dump fits the line equation format of William Kersting's "Distribution System Modeling and Analysis" book. I.e., the `impedance_dump` outputs the $\displaystyle{}a, b, c, d, A, B$ matrices of the equations:

$$\displaystyle{} \begin{bmatrix} VLG_{in} \\\ I_{in} \end{bmatrix} = \begin{bmatrix} a & b\\\ c & d\end{bmatrix}\cdot{}\begin{bmatrix}VLG_{out} \\\ I_{out}\end{bmatrix}$$

$$\displaystyle{}VLG_{out} = A\cdot{}VLG_{in}-B\cdot{}I_{in}$$

with the $\displaystyle{}b$ matrix typically representing the traditional impedance of a line. 

### Impedance Dump Parameters

#### Properties

**impedance_dump** does not declare inherited parent classes.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

Table: impedance_dump table 1 { #tbl:34-impedance-dump-1 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| **group** | char32 | N/A | I | Using the `groupid` feature, specific objects to be included in the output file. If left blank, all link-based objects will be part of the output file. |
| **filename** | char256 | N/A | I | Tells the object what file to print all information to. The file will be an XML-formatted text file. |
| **runtime** | timestamp | N/A | O | Tells the object at what time to output the impedance dump. Can be in either seconds from epoch (Unix time) or with a timestamp ('2006-01-01 00:00:00'). If not specified, the default is immediately after the first time step solution. |
| **runcount** | int32 | N/A | — | Indicates the number of times the impedance dump has executed (See Remarks). |

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

### Impedance Dump State of Development

Impedance Dump is considered a well developed, but unvalidated model. Additional features may be included as needed. 


