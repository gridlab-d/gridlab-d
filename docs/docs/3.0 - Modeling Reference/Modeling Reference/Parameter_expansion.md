# Parameter expansion

Global and environment parameter expansion [Template:NEW30]

## Synopsis

String operations
    
    
    ${name=string}
    ${name:-string}
    ${name:=string}
    ${name:+string}
    ${name:offset}
    ${name:offset:length}
    ${parameter/pattern/string}
    
    

Integer operations
    
    
    ${name=value}
    ${name+=value}
    ${name-=value}
    ${name*=value}
    ${name/=value} 
    ${name%=value}
    ${name++}
    ${++name}
    ${name--}
    ${--name}
    ${name<value[?yes:no]}
    ${name>value[?yes:no]}
    ${name<=value[?yes:no]}
    ${name>=value[?yes:no]}
    ${name==value[?yes:no]}
    ${name!=value[?yes:no]}
    

Bitwise operations
    
    
    ${name&=value}
    ${name|=~value} 
    ${name^=value} 
    ${name^=~value} 
    ${name&value[?yes:no]} 
    ${name|~value[?yes:no]} 
    

## Description

Parameter expansion is used to alter the lexical behavior the variable. 

`${parameter:-word}`
    Use default value. If parameter is unset or null, the expansion of word is substituted. Otherwise, the value of parameter is substituted.

`${parameter:=word}`
    Assign default value. If parameter is unset or null, the expansion of word is assigned to parameter. The value of parameter is then substituted. Positional parameters and special parameters may not be assigned to in this way.

`${parameter:+word}`
    Use alternate value. If parameter is null or unset, nothing is substituted, otherwise the expansion of word is substituted.

`${parameter:offset}`

`${parameter:offset:length}`
    Perform substring expansion. Expands to up to length characters of parameter starting at the character specified by offset. If length is omitted, expands to the substring of parameter starting at the character specified by offset. length and offset are arithmetic expressions (see ARITHMETIC EVALUATION below). length must evaluate to a number greater than or equal to zero. If offset evaluates to a number less than zero, the value is used as an offset from the end of the value of parameter. A negative offset is taken relative to one greater than the maximum index of the specified array. Note that a negative offset must be separated from the colon by at least one space to avoid being confused with the :- expansion. Substring indexing is zero-based unless the positional parameters are used, in which case the indexing starts at 1.

`${parameter/pattern/string}`
    Perform pattern substitution. The pattern is expanded to produce a pattern just as in pathname expansion. Parameter is expanded and the longest match of pattern against its value is replaced with string. If Ipattern begins with /, all matches of pattern are replaced with string. Normally only the first match is replaced. If pattern begins with #, it must match at the beginning of the expanded value of parameter. If pattern begins with %, it must match at the end of the expanded value of parameter. If string is null, matches of pattern are deleted and the / following pattern may be omitted.

`${name=value}`
    Assigned the value to the global variable (only for int32 values).

`${name++}`

`${++name}`

`${name--}`

`${--name}`
    Increments or decrements the global variable (only for int32 values). When the increment/decrement is before the variable name, the increment/decrement operation is performed before the variable is expanded. When the increment/decrement is after the variable name, the increment/decrement operation is performed after the variable is expanded.

`${name+=value}`

`${name-=value}`

`${name*=value}`

`${name/=value}`

`${name%=value}`
    Operates on the global variable (only for int32 values). The addition (+=), substract (-=), multiplication (*=), division (/=), and modulo (%=) operators are performed before the value is expanded.

`${name<value[?yes:no]}`

`${name>value[?yes:no]}`

`${name<=value[?yes:no]}`

`${name>=value[?yes:no]}`

`${name==value[?yes:no]}`

`${name!=value[?yes:no]}`
    Performs a comparison test of the global variable to a value (only for int32 values). If the test is true, the value 1 is expanded, otherwise the value 0 is expanded. If the string _yes_ is provided or empty, it is used in place of the value 1. If the string for _no_ is provided or empty, it is used in place of the value 0.

`${name&=value}`

`${name|=~value}`

`${name^=value}`

`${name^=~value}`
    Perform bitwise operations on the int32 variable. The bitwise and (&=), or (|=), xor (^=), and equiv (^+~) operations are performed on the value before it is expanded.

`${name&value[?yes:no]}`

`${name|~value[?yes:no]}`
    Perform bitwise tests on the int32 variable. The bitwise is-set (&) and is-clear (|~) return 1 (or _yes_) if the result is non-zero and 0 (or _no_) if the result is zero.

## Caveat

During implicitly looped loader operations such as with [object:.._n_], the parameter is expanded only once prior to the loop being executed. Sometimes, it is desirable to execute the parameter expansion operation each time the parameter is referenced. In such cases use of the [expansion variables] syntax is required, e.g., 
    
    
    object test:..10 {
      name `Test_{count++}`;
    }
    

instead of 
    
    
    object test:..10 {
      name "Test_${count++}";
    }
    

## Version

This functionality is proposed for [Hassayampa (Version 3.0)] but is not yet implemented. 

## See also

  * [Expansion variables]
  * [Global variables]

