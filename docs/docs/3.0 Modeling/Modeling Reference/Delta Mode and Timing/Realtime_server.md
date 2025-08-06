# Realtime server

# Real-time server

**TODO**:  Update for [Hassayampa (Version 3.0)]

Important node
    This capability is partially implemented. See the [Realtime_server] page for details. Striken text indicates the capability is not implemented at this time.

A real-time server (RTserver) module that include synchronous HTTP services has been implemented. The realtime server component has two main features. 

  1. It locks the simulator clock to the system clock. Although the simulator clock may be started before the real-time clock, the simulator will advanced as quickly as possible until the real-time clock time is encountered. From then on the simulation will run in real-time. This allow stable initialization to be performed if necessary.   
If the simulation clock is initialized to TS_INIT, the simulation will start immediately with the real-time clock. This is enabled by setting the global variable `run_realtime` to a non-zero value. ~~If the value is greater than 1, then the clock will run at a rate that many times greater than the system clock.~~
  2. Incoming HTTP traffic on port 80 is handled during the sync process of the server object. ~~As many servers can be created as desired. The name of the server is used for virtual host naming but if the server is unnamed, it takes the name of the local host.~~ Messages are handled synchronously in the order they are received and responses are always delivered in XML.
The following messages are handled: 

`http://_servername_ /_globalname_`
    This query obtains the value of a global variable. The XML result is usually as follows


     <global>
        <_variable-name_ >
           <unit>_unitname_ </unit>
           _value_
        </_variable-name_ >
     </global>
    


`http://_servername_ /_objectname_`
    This message queries the server for an object. The XML result is usually as follows
    
    
    <_objectname_ >
       <_propertyname_ >
         <unit>_unitname_ </unit>
         _value_
       </_propertyname_ >
       etc...
     </_objectname_ >
    


`http://_servername_ /_objectname_ /_propertyname_`
    This message queries the server for property of an object. The XML result is usually as follows


    
     <_objectname_ >
       <_propertyname_ >
         <unit>_unitname_ </unit>
         _value_
       </_propertyname_ >
     </_objectname_ >
    


`http://_servername_ /_objectname_ /_propertyname_ =_value_`
    This message queries the server for property of an object. The XML result is usually as follows


    
     <_objectname_ >
       <_propertyname_ >
         <unit>_unitname_ </unit>
         _value_
       </_propertyname_ >
     </_objectname_ >
    


Optionally, units may be appended, e.g., `http://_servername_ /_objectname_ /_propertyname_?='value unit __`


`http://_servername_ /_objectname_ /_functionname_?_arg1_ &_arg2_ &_..._ &_argN_`
    This message calls the function on the object. The XML result is usually as follows

    
    <_objectname_ >
       <_functionname_ >
         _int64-value_
       </_functionname_ >
     </_objectname_ >
    


  
## Realtime mode

GridLAB-D can be run as realtime system emulator by setting the global variable [run_realtime] to a non-zero value, e.g., 
    
    
     host% **gridlabd[-D] [run_realtime]=1 _modelname_.glm**
    

In realtime mode, GridLAB-D will attach the internal simulation clock to the system clock. GridLAB-D will update the model at most once per second. 

While running in realtime mode, the stop time is used to stop the simulation. If the stop time is not set or it was set to a time before the start time (in real time), then the simulation will run indefinitely until it receives a signal to stop. 

## Server mode

[Server] mode is essential to the realtime mode and allows a web-based application to access the global variable and properties of named objects. 

To start GridLAB-D in [server] mode, simply include the command line argument `--[server]`, e.g., 
    
    
     host% **gridlabd _modelname_.glm --[server]**
    

**Prior to[Keeler (Version 4.0)]:**

To get the value of a [global variable], use the following query 
    
    
     host% **wget http://_hostname_ :6267/_variable-name_ -q -O -**
    

To get the value of an [object property], use the following query 
    
    
     host% **wget http://_hostname_ :6267/_object-name_ /_property-name_ -q -O -**
    

To set the value of an [object property], use the following query 
    
    
     host% **wget http://_hostname_ :6267/_object-name_ /_property-name_ =_value_ -q -O -**
    

The value can include [units] (separate by a space) and they will be converted automatically. The value is read back after is set to confirm that it was accepted (including [unit] conversion). 

**As of[Keeler (Version 4.0)]:**

To get the value of a [global variable], use the following query 
    
    
     host% **wget http://_hostname_ :6267/_method_ /_variable-name_ -q -O -**
    

where the method can be **raw** , **json** , or **xml**. To get the value of an [object property], use the following query 
    
    
     host% **wget http://_hostname_ :6267/_format_ /_object-name_ /_property-name_ -q -O -**
    

If you use the **xml** method, you can specify formatting by appending the specifications to the property name, e.g., 
    
    
     host% **wget http://_hostname_ :6267/_format_ /_object-name_ /_property-name_[_spec_] -q -O -**
    

The format specification is structured as follows for double values 
    
    
     [_unit_ ,_precision_ _format_]
    

and for complex values 
    
    
     [_unit_ ,_precision_ _format_ _part_]
    

The _unit_ must be compatible with the unit of the value given. The _precision_ must be a single digit from 0 to 9. The format must be one of **a** , **f** , **e** , **g** , **A** , **F** , **E** , **G** , as interpreted by `printf()`. The part may 'i' or 'j' for complex rectangular, 'd' for complex polar with angle in degrees, 'r' for complex polar with angle in radians, 'M' for magnitude, 'D' angle only in degree, 'R' for angle only in radians. 

To set the value of an [object property], use the following query 
    
    
     host% **wget http://_hostname_ :6267/_format_ /_object-name_ /_property-name_ =_value_ -q -O -**
    

The value can include [units] (separate by a space) and they will be converted automatically. The value is read back after is set to confirm that it was accepted (including [unit] conversion). 

### Control

[Template:NEW30] (as of [Grizzly:2732](http://sourceforge.net/apps/trac/gridlab-d/changeset/2732)) 

To [pause] the simulation at a specified time, use the following query: 
    
    
     host% **wget 'http://_hostname_ :6267/[control]/[pauseat]=_YYYY-MM-DD HH:MM:SS ZZZ'_**__

Note that if the simulation is already [paused] and the time given is later than the current time, it will [resume] the simulation and pause at the specified time. 

To [resume] the simulation when it is paused, use the following query: 
    
    
     host% **wget http://_hostname_ :6267/[control]/[resume]**
    

In this case, the simulation will run until the stoptime or steady state is reached, whichever comes first. 

To [shutdown] the simulation, use the following query: 
    
    
    host% **wget http://_hostname_ :6267/[control]/[shutdown]**
    

### XML

**Prior to[Keeler (Version 4.0)]**

To read data entities, use the following query: 
    
    
     host% **wget http://_hostname_ :6267/[xml]/_specification_**
    

where the specification may take the forms 

  * _varname_ to read a [global variable]
  * _module::varname_ to read a module variable
  * _name:property_ to read an [object property]
To write data entities, use the following query: 
    
    
     host% **wget http://_hostname_ :6267/[xml]/_specification_ =_value_**
    

The value may includes [units] if unit conversion is needed: 
    
    
     host% **wget 'http://_hostname_ :6267/[xml]/_specification_ =_value_ _unit'_**
    

### Output

Output files may be requested using the query: 
    
    
     host% wget http://_hostname_ :6267/output/_filename_._ext_
    

Only files in the working directory or subdirectories thereof may be accessed. 

The response to [global variable] requests will be in the form 
    
    
     <globalvar>
       <name>_variable_name_ </name>
       <value>_value[ unit]_ </value>
     </globalvar>
    

The response to [object property] requests will be in the form 
    
    
     <property>
       <object>_object_name_ </object>
       <name>_property_name_ </name>
       <value>_value[ unit]_ </value>
     </property>
    

### Runtime Files

Some client support functions require runtime libraries that are provided by the server. To read a runtime library use the query 
    
    
     host% wget http://_hostname_ :6267/rt/_library_name_
    

The library content is returned. For example, to read the default javascript library, use the query 
    
    
     host% wget http://_hostname_ :6267/rt/gridlabd.js
    

**As of[Keeler (Version 4.0)]**

Any global variable name surrounded by '<<<' '>>>' will be substituted on the server-side. 

### Server-side applications

Some server-side applications may be installed for a server and called by clients by using the query 
    
    
     host% wget http://_hostname_ :6267/_appname_ /_scriptname_
    

The following applications are currently supported if they are installed on the server: 

Server applications  Application | App Name | Script Extension   
---|---|---  
Perl | perl | .pl   
GnuPlot | gnuplot | .plt   
Python | python | .r   
Java | java | .jar   
R | r | .r   
Scilab | scilab | .cse   
Octave | octave | .m   
  
In all cases the output is copied to the client. Typically, it is the HTML code needed to properly embed the result of the script. 

## See also

  * Realtime server
    * [control]
    * [open]
    * [xml]
    * [gui]
    * [output]
    * [action]
    * [rt]
    * [perl]
    * [gnuplot]
    * [java]
    * [python]
    * [r]
    * [scilab]
    * [octave]
  * [server]
  * [server_portnum]
  * [server_quit_on_close]
