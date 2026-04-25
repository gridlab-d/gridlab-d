## Impedance Dump

!!! warning
    This page was automatically generated and requires review.

Impedance dump allow the impedance and line equation matrices to be output into an XML file for debugging or further use. 

The output format of the impedance dump fits the line equation format of William Kersting's "Distribution System Modeling and Analysis" book. I.e., the `impedance_dump` outputs the $\displaystyle{}a, b, c, d, A, B$ matrices of the equations:

$$\displaystyle{} \begin{bmatrix} VLG_{in} \\\ I_{in} \end{bmatrix} = \begin{bmatrix} a & b\\\ c & d\end{bmatrix}\cdot{}\begin{bmatrix}VLG_{out} \\\ I_{out}\end{bmatrix}$$

$$\displaystyle{}VLG_{out} = A\cdot{}VLG_{in}-B\cdot{}I_{in}$$

with the $\displaystyle{}b$ matrix typically representing the traditional impedance of a line. 

### Impedance Dump Parameters

#### Properties

**impedance_dump** does not declare inherited parent classes.

Input indicates properties you can set in models. Output marks properties produced or modified during simulation runtime.

| Property Name | Type | Unit | Input | Output | Description |
| --- | --- | --- | --- | --- | --- |
| group | char32 | N/A |  | ✓ | Using the `groupid` feature, specific objects to be included in the output file. If left blank, all link-based objects will be part of the output file. |
| filename | char256 | N/A | ✓ |  | Tells the object what file to print all information to. The file will be an XML-formatted text file. |
| runtime | timestamp | N/A |  | ✓ | Tells the object at what time to output the impedance dump. Can be in either seconds from epoch (Unix time) or with a timestamp (&#x27;2006-01-01 00:00:00&#x27;). If not specified, the default is immediately after the first time step solution. |
| runcount | int32 | N/A |  |  | Indicates the number of times the impedance dump has executed (See Remarks). |

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
