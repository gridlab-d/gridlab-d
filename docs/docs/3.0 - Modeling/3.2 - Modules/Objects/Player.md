# Player

Play data into the model 


## Synopsis
    
    
    module tape;
    object player {
      name _player-name_ ;
      parent _target-object-name_ ;
      property _target-property-name_ ;
      file _output-file-name_ ;
      flags DELTAMODE; // 
    }
    

## Remarks

A player provides the ability to update a single object variable at specified times. The values are read from a file formatted like comma-separated value (CSV) files or other sources (for example, open database connectivity ODBC or Matlab). The source data must have timestamps (or time changes) in the first column, and the values to be posted in the second column. 

In specifying a player in a model input **file** , the **property** to which the value is written must be specified. The variable to be updated must exist in the player’s **parent** , which must also be specified in the input model. A **loop** count can also be specified that will allow the source to be played more than once. For example, the following lines in a model input file will use the player in a file named **lightingDemand.txt** to update the demand variable in the **lights** object. 


       object player {
           name player;
           parent lights;
           property demand;
           file lighting.player;
           loop 1000;
       }

To define a player input file in the CSV format, you must always place the time specification in the first column and the value in the second column. 

The time specification can either be absolute or relative. The first time must be absolute, but subsequently time can be either. However, after a loop is executed, only values with relative times are used. Absolute time must conform to the format for timestamp properties. Relative times are always in the form: 

     +###smhdw
    

where `smhdw` specifies seconds, minutes, hours, days, or weeks. 

The value in the second column must be compatible with the read formatting for the target property. 

There are three recognized line formats for players: 

an absolute timestamp in seconds, then the value  | `123456789,42.0`  
---|---  
an absolute timestamp in YYYY-MM-DD HH:mm:ss, then the value  | `2000-01-01 3:00:00,123.4`  
a relative timestamp, in +Ns, +Nm, +Nh +Nd, then the value  | `+30s,5678.9`  
  
It is allowable to construct players without a parent and without a target property, so long as the player has the property 'value' published manually. The player's value can then be used as a schedule transform, allowing specific data sets to be played to a number of different objects without the memory overhead of the schedule. The value property can be published with the following snippet: 
    
    
    module tape;
    
    class player{
      double value; //Note that this property must be named as 'value', i.e., this statement must be copied exactly.
    }
    

### DELTAMODE Processing

When simulation_mode is set to DELTA the player processes records using subsecond timing. In this situation, timestamps must include fractional seconds to a precision of 6 decimal places (i.e., with a resolution of 1 ms). The timezone specification must be omitted when playing subsecond intervals. 

**TODO**:  The behavior of DST is not specified in subsecond mode, i.e., are timestamp in the localtime or standard time? (see ticket:563). 

## Properties

### parent

* **object** -
    Built-in property that specifies the object that the player will attempt to write values to.

### property

* **string** -
    The object properties that will be updated based on value in the player's input file. Property names are case sensitive, and whitespace between properties is verboten. Properties with units may be converted to compatible internal units, if necessary. Complex properties may be written as their individual real and imaginary parts by appending ".real" or ".imag" to the property name, such as "power.real", but those parts cannot convert their units, if the associated complex property has an internal unit specified.

### file

* **string** -
    By default, the name of the file from which the reads input. If left empty, the player will use a file name based on the target object's name. Input streams may be specified using the "stream:path", where stream may be "file", "odbc", or "memory". The path for file refer to a file name, to a global variable name for memory, and to a server login string for odbc. See the Tape Database Input and Tape Memory Input sections for more details.

### loop

* **double** -
    By default, this value is zero. When using relative time in the player file, this determines the number of times the values with should "loop". The first timestamp in the file should designate the initial start time of the player, then all subsequent relative times will be "looped" the number of times defined by the loop variable. Note, the initial timestamp will be ignored on all subsequent loops.

### flags

* **enumeration** -
    Use DELTAMODE to enable subsecond operation when processing data. Some features of player do not work the same when operation in DELTAMODE is enabled. This includes timezone processing (see <http://sourceforge.net/p/gridlab-d/tickets/563>).

## See also

  * recorder
  * Tape (module)
