# Spec:Multirun mode

**Source URL:** https://GridLAB-D™.shoutwiki.com/wiki/Spec:Multirun_mode
Approval item:  \--[Dchassin] 16:29, 9 October 2011 (UTC) 

## Contents

  * 1 Theory of operation
  * 2 GLM syntax
  * 3 Command line
  * 4 General specifications
  * 5 See also
This document describes the specifications for implementation of [multirun mode]. 

## Theory of operation

[Multirun mode] operates by creating and coordinating multiple [instances] of GridLAB-D™. The originating [instance] is called the [master] and the dependent instances are called the [slaves]. 

The overall process is as follows: 

(bold indicates code changes required for [multirun mode] support)  Master | Slaves (1 or more)   
---|---  
(main thread) | (control thread) | (main thread) | (control thread)   
load model |  |  |   
**create instance** |  |  |   
**create linkages** | **start slaves** |  |   
initialize model | **wait for slaves exit** | **enter slave mode** |   
start main loop |  | load model | **connect to master**  
**send t0 to slaves** |  | initialize model | **signal master**  
**send data to slaves** |  | start main loop | **while t1!=TS_NEVER**  
**signal slaves** |  |  pause main loop |  **wait for slave signal**  
**wait for master signals** |  |  |  **receive t1 from master**  
|  |  |  **receive data from master**  
|  |  |  **resume main loop**  
|  |  sync objects |  **wait for main loop pause**  
|  |  |  **send t2 to master**  
|  |  |  **send data to master**  
|  |  |  **signal master**  
**recv t2 from slaves** |  |  |   
|  |   
sync objects |  |  |   
update global clock |  |  |   
end main loop on TS_NEVER |  |  |   
**write TS_NEVER to slaves** |  |  |   
**signal slaves** |  |  | **end loop**  
|  | end main loop on TS_NEVER | **halt main loop**  
|  | **send TS_NEVER to master** | **end thread**  
|  | **signal master** |   
**wait for master signal** |  | exit |   
**read TS_NEVER** from slave | **end thread** |  |   
exit |  |  |   
  
## GLM syntax

An instance is defined in the [master] file using the [instance] [directive] syntax: 
    
    
    **[instance]** (optional [string])[hostname]**:**(optional [int16])_portnum_ {
       (required)**[model]** ([string])_[slave]_model_filename_.glm;
       (optional)**[cacheid]** ([int64])_[cacheid]'_
       (multiple)_master_object_:_property_ **- >** _slave_object_:_property_ ;
       (multiple)_master_object_:_property_ **< -** _slave_object_:_property_ ;
    }
    

## Command line

The command line for [master] runs is unchanged. However, the --[port] [command option] can be used to change the default port number for socket-based connections. 

The command line for [slave] runs shall use the following syntax: 
    
    
    host% gridlabd --[slave] _connection_string_
    

where the _connection_string_ is platform specific and differs depending on whether the multirun is on the local host or uses multiple machines. 

## General specifications

  * No changes to [slave] model files shall be required to link a [slave] with a [master].
  * When the [hostname] is not specified, the default hostname shall be [localhost].
  * When the [portnum] is not specified, the default port number shall be 6267.
  * The [cacheid] is to be used for debugging purposes only and shall not be documented for normal usage.
  * If the [slave] model cannot be loaded, the problem shall be detected and handled by the [slave] [instance].
  * If a [master] object or property cannot be found the [master] shall notify output an error message identify the file and line on which the offending linkage occurred.
  * If a [slave] object or property cannot be found, the problem shall be detected and handled by the [slave] [instance].
  * If the [slave] cannot find a [linkage] object or property, the [slave] shall output an invalid linkage error, notify the [master] that it cannot initialize, and terminate in the normal manner.
  * Output streams shall have a prefix indicating the source of the message. The [master] message prefix shall be formatted as "M _cpu_(_pid_)" and [slave] message prefix shall be formatted as "S _cpu_(_pid_)", where _cpu_ is the CPU to which the instance is assigned (see [pstatus]) and _pid_ is its process id.
## See also

  * [Multirun mode]
    * [Directives]
    * [instance]
    * [linkage]
    * [master]
    * [slave]
  * Technical documents 
    * [Requirements]
    * Specifications
    * [Technical manuals]
    * [Validation]

