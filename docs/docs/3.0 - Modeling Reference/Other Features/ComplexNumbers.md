# Complex Numbers

Complex values are represented using the complex property type.

To declare a complex number in a class use the syntax

        class my_class {
        complex my_complex;
        }

Accepted complex number notations are i, j, and r. Complex notation d is used to designate polar coordinates.

You may include a unit with the declaration using the syntax

        class my_class {
        complex my_complex[unit];
        }

where unit is one of the supported units.

To define a complex number in an object use the syntax

        object my_class {
        my_complex value;
        }

You may include units in the definition using the syntax

        object my_class {
        my_complex value unit;
        }

provided the unit defined is compatible with the unit declared.

## ASCII formatting

Complex number are formatted in one of four ways depending on the internal settings and last update to the variable (- indicates sign is optional, + indicates sign is mandatory, and none indicates number cannot have a sign).

Math rectangular format

        #.######e-###+#.######e-###i
Engineering rectangular format

        -#.######e-###-#.######e-###j

Polar radians format

        #.######e-###+#.######e-###r

Polar degrees format

        #.######e-###+#.######e-###d

Units, if specified by the class will be appended following a space, i.e.,

        e-###-#.######e-###j units

where units is one of the units in unitfile.txt (or one derived from them).

The format of complex numbers is controlled by the complex_format global variable.

## Complex Format

The **complex_format** global variable control the formatting used to convert complex numbers between their in-memory and their string representations.

        host% gridlabd -D complex_format='%+lg%+lg%c'
        host% gridlabd --define complex_format='%+lg%+lg%c'
        #set complex_format=%+lg%+lg%c

!!! caveat

        Current the formatting assumes that the conversion is always in the order REAL + IMAGINARY + ANGLE_UNIT. There may be other orders of representation that are desired but they cannot be supported using the current assumptions.

!!! bugs

        If the formatting of complex numbers is not presented in the order %lg %lg %c with appropriate non-conversion parsing characters interspersed, GridLAB-D™ may crash without warning or remedy.
        
### Complex Output Format

Control the output representation of complex numbers. The complex_output_format global variable controls the representation formatting used for the output of complex numbers (as opposed to **complex_format** which controls the digits/precision).

        host% gridlabd -D complex_output_format=DEFAULT
        host% gridlabd --define complex_format=RECT
        #set complex_output_format=POLAR_DEG


Valid options for the argument are: 

Table: Complex Number Argument Options { #tbl:table-complex }

Value | Description | Math Representation | Output Text   
---|---|---|---  
`DEFAULT` | Output is dictated by the complex variable itself (and relevant flag) - pre-4.3 behavior | Varies | Varies   
`RECT` | Output is in rectangular format | $>x+yj$ | `0.8660+0.5j`  
`POLAR_DEG` | Output is in polar format, with angle expressed in degrees | $x+yj$ | $\angle(\textrm{atand}\left(\frac{y}{x}\right))$ | `1.0+30.0d`  
`POLAR_RAD` | Output is in polar format, with angle expressed in radians  | $x+yj$ | $\angle(\textrm{atan}\left(\frac{y}{x}\right))$ | `1.0+0.5236r`  