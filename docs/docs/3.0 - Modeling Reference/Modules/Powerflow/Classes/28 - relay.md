## Relay

Relays are used to provide momentary breaks in the system and are implemented as reclosers. A **relay** object only functions in on a basic level and does not provide any **reliability**-module-functionality at this time. It is untested in the `NR` solver and a **recloser** object is suggested instead. A typical relay would be implemented as 
    
    
    object relay {
    	name recloser_A;
    	phases ABCN;
    	from node_1;
    	to load_5;
    	time_to_change 1.0s;
    	recloser_delays 5.0s;
    	recloser_tries 5;
    	}
    
    

### Relay Parameters

**relay** objects are derived from **link** objects, so all of those parameters should be available. 

Property Name  | Type  | Unit  | Description   
---|---|---|---  
**time_to_change**  | double  | seconds  | Time to physically change out or reset the relay/recloser after it has locked out. **Note: This feature is unimplemented at this time.**  
**recloser_delay**  | double  | seconds  | Time from a trip before the **recloser** will attempt to close the circuit again.   
**recloser_tries**  | int16  | N/A  | Number of reclosing attempts before the **recloser** object will lock in the open position.   
  
### Relay State of Development

Relay is considered a well developed and validated model in terms of `FBS`-based powerflow solutions, however, models incorporating fault analysis, reliability, and other advanced features have not been validated. 

