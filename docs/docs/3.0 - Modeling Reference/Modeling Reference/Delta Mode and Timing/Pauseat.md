# Pauseat

The **pauseat** time is a timestamp global variable that enables a synchronization event on the main loop state when the clock reaches or exceeds the **pauseat** time. The default **pauseat** time is NEVER. 

## GLM

The **pauseat** time can be set in a GLM file using the directive 
    
    
    #set pauseat=_datetime_
    

where _datetime_ is a timestamp. If _datetime_ is NEVER, then the _pauseat_ time is cleared and no synchronization event will occur on the main loop state. 

## Command line

The **pauseat** time can be set from the command line using the define command line option: 
    
    
    host% **gridlabd --define pauseat=_datetime_**
    
    -or-
    
    host% **gridlabd -D pauseat=_datetime_**
    

where _datetime_ is a timestamp. If _datetime_ is NEVER, then the _pauseat_ time is cleared and no synchronization event will occur on the main loop. 

## Server control

The server main loop synchronization event may be directly controlled using the server control commands. The _pauseat_ can be cancelled using the control query: 
    
    
    http://_hostname_ :_portnum_ /control/resume
    

and it may be set to a new value 
    
    
    http://_hostname_ :_portnum_ /control/pauseat=_datetime_
    

which will implicitly restart the simulation if the new _datetime_ is greater than the current clock value. 

# See also

  * timestamp
  * clock
  * main loop
  * server control
