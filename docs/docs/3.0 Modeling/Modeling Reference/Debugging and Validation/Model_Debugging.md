# Model Debugging

The debugger is started when the **\--debugger** command-line option is used. It can also be started by including the line 
    
    
    #set debugger=1
    

in the GLM file. The debugger supports two methods of interrupting the simulation. 

  1. **Breakpoints** halt the simulator and start the debugger whenever a situation arises that matches the breakpoint criterion. For example, a breakpoint on the bottom-up pass will stop the simulation every time an object sync is called during a bottom-up pass.


  1. **Watchpoints** are different from breakpoints in that the debugger is only stopped when the value being watched changes. For example, a watchpoint on node:12 voltage would only stop the simulation when the voltage of node:12 is changed. In contrast, a breakpoint on node:12 would stop each time node:12 is sync'd.
While the debugger is running **help** will print a list of all the available commands. 

# Getting started

To start the debugger you must include the **\--debugger** option on the command-line. Note that while the debugger is running, the system will only operate in single-threaded mode. 

Each time the debugger stops to prompt for input, it displays the current simulation time and simulator status. The status include which pass is currently running (see PASSCONFIG), which rank is being processed, which object is about to be updated, and which iteration is being run (if the time has not advanced yet). 
    
    
    DEBUG: time 2000-01-01 00:00:00 PST
    DEBUG: pass BOTTOMUP, rank 0, object link:14, iteration 1
    GLD>
    

Debugging commands may be abbreviated to the extent that they are unambiguous. For example b may be used instead of break, but wa must be used for watch to distinguish it from where. 

# Listing objects

To obtain a list of objects loaded, you may use the list command: 
    
    
    GLD> list
    A-b---    2 INIT                     node:0           ROOT
    A-b---    1 INIT                     node:1           node:0
    A-b---    1 INIT                     node:2           node:0
    A-b---    1 INIT                     node:3           node:0
    A-b---    0 INIT                     link:4           node:1
    A-b---    0 INIT                     link:5           node:2
    A-b---    0 INIT                     link:6           node:2
    A-b---    0 INIT                     link:7           node:3
    A-b---    0 INIT                     link:8           node:3	
    GLD>
    

You may limit the list to only the object of a particular class: 
    
    
    GLD> list node
    A-b---    2 INIT                     node:0           ROOT
    A-b---    1 INIT                     node:1           node:0
    A-b---    1 INIT                     node:2           node:0
    A-b---    1 INIT                     node:3           node:0
    GLD>
    

  1. The first column contains flags indicating the status of the object. In the first character: 
     * A indicates the object is active (operating)
     * P indicates the object is planned (not yet operating)
     * R indicates the object is retired (no longer operating) In the second character:
     * \- indicates that the object is not called on the PRETOPDOWN pass
     * t indicates that the object has yet to be called on the PRETOPDOWN pass
     * T indicates that the object has already been called on the PRETOPDOWN pass In the third character:
     * \- indicates that the object is not called on the BOTTOMUP pass
     * b indicates that the object has yet to be called on the BOTTOMUP pass
     * B indicates that the object has already been called on the BOTTOMUP pass In the fourth character:
     * \- indicates that the object is not called on the POSTTOPDOWN pass
     * t indicates that the object has yet to be called on the POSTTOPDOWN pass
     * T indicates that the object has already been called on the POSTTOPDOWN pass In the fifth character:
     * \- indicates the object is unlocked
     * l indicates the object is locked In the sixth character:
     * \- indicates the object's native PLC code is enabled
     * x indicates the object's native PLC code is disabled
  2. The second field is the object's rank.
  3. The third field is the object's internal clock (or INIT) if the object has not yet been sync'd.
  4. The fourth field is the name (class:id) of the object.
  5. The fifth field is the name of the object's parent (or ROOT) if is has none.
# Printing values

To inspect the properties of an object, you can use the print command. With no option, the current object is printed: 
    
    
    GLD> print
    DEBUG: object link:5 {
      parent = node:2
      rank = 0;
      clock = 0 (0);
      complex Y = +10-1j;
      complex I = +0+0j;
      double B = +0;
      object from = node:0;
      object to = node:2;
      }
    GLD>	
    

When an object name (class:id) is provided, that object is printed: 
    
    
    GLD> print node:0
    DEBUG: object node:0 {
      root object
      rank = 2;
      clock = 0 (0);
      latitude = 49N12'34.0";
      longitude = 121W15'48.3";
      complex V = +1-0d;
      complex S = +0+0j;
      double G = +0;
      double B = +0;
      double Qmax_MVAR = +0;
      double Qmin_MVAR = +0;
      enumeration type = 3;
      int16 bus_id = 0;
      char32 name = Feeder;
      int16 flow_area_num = 1;
      complex Vobs = +0+0d;
      double Vstdev = +0;
      }
    GLD>
    

# Scripting commands

You can run a script containing debug commands using the script command: 
    
    
    GLD> sys copy con: test.scr
    wa node:0
    run
    ^Z
    1 file(s) copied.
    GLD> script test.scr
    DEBUG: resuming simulation, Ctrl-C interrupts
    DEBUG: watchpoint 0 stopped on object node:0
    DEBUG: object node:0 {
      root object
      rank = 2;
      clock = 2000-01-01 00:00:00 PST (946713600);
      latitude = 49N12'34.0";
      longitude = 121W15'48.3";
      complex V = +1-0d;
      complex S = +0.522519+0.0522519j;
      double G = +0;
      double B = +0;
      double Qmax_MVAR = +0;
      double Qmin_MVAR = +0;
      enumeration type = 3;
      int16 bus_id = 0;
      char32 name = Feeder;
      int16 flow_area_num = 1;
      complex Vobs = +0+0d;
      double Vstdev = +0;
      }
    DEBUG: watchpoint 1 stopped on object node:0
    DEBUG: object node:0 {
      root object
      rank = 2;
      clock = 2000-01-01 00:00:00 PST (946713600);
      latitude = 49N12'34.0";
      longitude = 121W15'48.3";
      complex V = +1-0d;
      complex S = +0.522519+0.0522519j;
      double G = +0;
      double B = +0;
      double Qmax_MVAR = +0;
      double Qmin_MVAR = +0;
      enumeration type = 3;
      int16 bus_id = 0;
      char32 name = Feeder;
      int16 flow_area_num = 1;
      complex Vobs = +0+0d;
      double Vstdev = +0;
      }
    DEBUG: time 2000-01-01 00:00:00 PST
    DEBUG: pass BOTTOMUP, rank 2, object node:0, iteration 5
    GLD>
    


