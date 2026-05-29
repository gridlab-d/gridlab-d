## Sectionalizer

**sectionalizer** objects provide a means to isolate faulted portions of a system. **sectionalizer** objects work in conjuction with the **reliability** module and the **[recloser](27-recloser.md)** objects. **reliability** will automatically open a **sectionalizer** if an upstream **recloser** is present, and has "tries" available. **sectionalizer** objects should work for both solver methods, but the **reliability** functionality only works in the `NR` solver. 

A minimal **sectionalizer** implementation is: 
    
    
    object sectionalizer {
    	name Test_Section;
    	phases ABC;
    	}
    

with an equivalent representation of: 
    
    
    object sectionalizer {
    	name Test_Section;
    	phases ABC;
    	phase_A_state CLOSED;
    	phase_B_state CLOSED;
    	phase_C_state CLOSED;
    	operating_mode BANKED;
    	}
    

**sectionalizer** objects behave exactly like **[switch](26-switch.md)** objects, aside from their **reliability** coordination. **sectionalizer** objects inherit all **[switch](26-switch.md)** properties and default to a banked operation mode. No new parameters are introduced in sectionalizers. 

### Sectionalizer Parameters

### Sectionalizer State of Development

**sectionalizer** objects are based on **[switch](26-switch.md)** objects and share a common state of development. Normal operation is tested and verified. **reliability**-based actions are validated, but not fully tested at this time. 
