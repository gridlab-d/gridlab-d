# Recorder

Record data to a stream 

## Synopsis
    
    
    module [tape];
    object recorder {
      name "_recorder-name_ ";
      parent "_target-object-name_ ";
      property "_target-property-name_ ";
      file "_output-file-name_ ";
      interval _sampling-interval_ ;
      limit _sampling-limit_ ;
      trigger "_relation value_ ";
      flags DELTAMODE; // 
      flush -1; // 
    }
    

## Remarks

A recorder provides the ability to collect a recording of one of more properties of an object. It can specify the sampling interval, triggers, and other properties affecting the recording. For example, the following lines in a model input file will record the values of the energy and power variables in the meter object to a file called meter.csv at 3600-second intervals. 
    
    
    object recorder{
      name MeterCorder;
      parent meter;
      property energy,power;
      file meter.csv;
      interval 3600;
      limit 1000;
    }
    

The recorder places a [timestamp] in the first column of every row it emits. By default the [timestamp] is formatted using the [ISO] format (i.e., **yyyy-mm-dd HH:MM:SS TZ**). However, if the value of the [dateformat] [global variable] is not [ISO], then the alternative date/time formatting rules will apply as follows: 

#set [dateformat]=[US]
    The [timestamp] will be formatted as **mm-dd-yyyy HH:MM:SS[.SSSSSS]**.

#set [dateformat]=[EURO]
    The [timestamp] will be formatted as **dd-mm-yyyy HH:MM:SS[.SSSSSS]**.

### DELTAMODE Processing

When [simulation_mode] is set to [DELTA] the recorder processes records using [subsecond] timing. In this situation, timestamps will be recorded using fractional seconds to a precision of 6 decimal places (i.e., with a resolution of 1 [ms]). The timezone specification in always omitted when recording [subsecond] intervals. 

## Properties

### parent

object
    Built-in property that specifies the object that the recorder will attempt to read values from.

### property

string
    Comma-separated list of published properties that will be read, in order, and written to the recorder's output file. Property names are case sensitive, and whitespace between properties is verboten. Properties with units may be converted to other relevant units. Complex properties may be read as their individual real and imaginary parts by appending ".real" or ".imag" to the property name, such as "power.real", but those parts cannot convert their units, if the associated complex property has one.

### trigger

string
    A short string that contains an equality or inequality and provides a simple trigger that will delay the opening of a recorder until the condition is met for the first property in the list. The first character must be the operator, <, =, or >, and it must be immediately followed by the signed value to compare the property to. An example would be ">+90.0" for the house air temperature. Once the trigger is met, the recorder will read to its limit as normal.

### file

string
    By default, the name of the file to write the recorder output to. If left empty, the recorder will generate a file name based on the target object class and internal ID number. The exact mode is dependent on the format of this string. A simple file name will write text output to the specified file. Other output modes are available with "mode:path", where mode may be "file", "odbc", "memory", or "plot". The path for file and plot refer to a file name, to a global variable name for memory, and to a server login string for odbc. See the [Tape Database Output], [Tape Memory Output], and [Plotting Output] sections for more details.

### flush

number
    By default the output buffer is flushed to disk when it is full (the size of the buffer is system specific). This default corresponds to the flush value -1. If flush is set to 0, the buffer is flushed every time a record is written. If flush is set to a value greater than 0, the buffer is flushed whenever the condition `clock mod flush == 0` is satisfied.

### interval

integer
    The frequency at which the recorder samples the specified properties, in seconds. A frequency of 0 indicates that they should be read & written every iteration (note, that each timestep often requires multiple iterations, so a frequency of zero may lead to multiple measurements in a timestep). A frequency of -1 indicates that they should be read every timestep, but only written if one or more values change. By default, this is TS_NEVER.

### limit

integer
    The number of rows to write to the output stream. A non-positive value puts no limit on the file size (use at your own risk). By default, this is 0. The limit is only checked when output non-subsecond value.

### multifile

char1024
    The name of the file to use for multi-run recorder output. Multiple runs will append the number of the run to the column headers. Multi-run results can only be recorded given a fixed-length interval and a finite limit that will end before the simulation does, in order to properly close and move the temporary multi-run output file to its proper location.

### plotcommands

string
    Used with gnuplot. See [Plotting Output] for details.

### xdata

string
    Used with gnuplot. See [Plotting Output] for details.

### columns

string
    Used with gnuplot. See [Plotting Output] for details.

### output

enumeration
    Used with gnuplot. See [Plotting Output] for details.

### header_units

enumeration
    Used to control the appearance of units for the column headers. "ALL" will force the units to be printed for every column, if present. "NONE" will suppress the printing of units on the column header, even if conversions are being made. "DEFAULT" will only print the units if they are defined in the GLM.

### line_units

enumeration
    Used to control the appearance of units in the data rows. "ALL" will force the units to be printed for every row entry, if present. "NONE" will suppress the printing of units within the rows, even if conversions are being made. "DEFAULT" will only print the units if they are defined in the GLM for that column.

### flags

enumeration
    Use DELTAMODE to enable [subsecond] operation when collecting data. Some features of recorder do not work the same when operation in DELTAMODE is enabled. These include triggers, limits, formatting, plotting, and interval.

## Examples

A recorder that will watch the voltages of a node and record all three phases every iteration in kilovolts. 
    
    
    object recorder {
       parent ThatNode;
       property voltage_A[kV],voltage_B[kV],voltage_C[kV];
       interval -1;
       limit 1000;
       file ThatNode_kV.csv;
    }
    

A recorder that records when a house's heater or air conditioning turns on or off. 
    
    
    object recorder {
       parent OurHouse;
       property hc_mode;
       interval 0;
       limit 1000;
       file house_hvac_state.csv;
    }
    

A recorder that prints the real and imaginary components of a node's voltage every five minutes. Useful for output to Microsoft Excel. 
    
    
    object recorder {
       parent ThisNode;
       property voltage_A.real,voltage_A.imag;
       interval 300;
       limit 1000;
       file ThisNode_ri.csv;
    }
    

## Caveats

The recorder attempts to read the value from the last iteration of a timestep, rather than the first iteration of a timestep. Normally the final value is more important than the initial or the intermediate values, for iterative solvers, but an interval of -1 can be used if necessary to record the value of a property with greater resolution. 

The recorder cannot currently record the final value into a file, even though that value will be present in the output or dump XML file. This is a known quirk that cannot be resolved with the current structure of GridLAB-D, but is being worked on. All values written by various recorders at the same timestamp will be consistent between each other, however. 

## See also

  * [collector]
  * [Tape (module)]
  * [group recorder]

