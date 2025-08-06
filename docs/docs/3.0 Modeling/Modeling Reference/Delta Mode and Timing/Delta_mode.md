# Subsecond

**Source URL:** https://gridlab-d.shoutwiki.com/wiki/Delta_mode

---
 
 
Approval item:  

Subsecond \- Subsecond simulation mode (_delta mode_) 

Modelers may simulate certain kinds of systems at subsecond resolution. Normally GridLAB-D runs in an event-driven mode. In _event mode_ the clock is driven by the schedule of events in the objects defined in the model. Events may only be scheduled to a precision of 1 second. 

If subsecond behavior must be modeled, the event-driven simulation must be stopped while the subsecond behavior is simulated using a fixed time-step clock. This mode is called _delta mode_. When delta mode is operating, events are ignored and the simulation advances in very small time steps. 

## Simulation Operation

The simulation enters delta mode when at least one module requests a switch from event mode to delta mode. When a module request delta mode, it also informs the simulation core of the desired timestep, which is usually a parameter either fixed by the module's programmer or determined by the GLM modeler. 

While operating in delta mode, the simulation performs the following sequence of operations until all the modules state that the simulation may return to event mode 

  **Module Preupdate** \- notify modules that event mode is being suspended and a series of delta mode updates is about to begin
  **For each timestep** \- loop for timestep from current clock time until maximum time elapse or all modules request return to event mode 

  **For each object** \- loop through list of objects that flag DELTAMODE support from lowest rank to highest rank 

  **Class update** \- send update message to objects that flag DELTAMODE support and export the update function
  **Module Interupdate** \- notify modules that all object updates are done
  **Module Postupdate** \- notify modules that delta mode is ending and event mode is resuming

## Caveats

  * The delta mode of operation is potentially a lot slower than event mode. For this reason, realtime operation and delta mode operation are currently mutually exclusive. If realtime operation is enable, the simulation will not enable delta mode, even when a module requests it.

  * When operating in delta mode, objects are updated only in bottom-up rank order.

  * Module updates are processed in the order in which modules are loaded by the GLM file. This means that modules which depend on other modules in delta mode should be loaded last. For example, for the timestamps of recordings from the tape module to coincide with the states of sampled values the tape module should be loaded after the modules which the recorder samples. This way the high-speed recorders can sample the states of the other models after the delta time update has been applied to all objects and modules.
## Version

This capability was introduced to a limited extent in Grizzly (Version 2.3) and not all modules support subsecond simulations. 

### Support

Only certain objects support subsecond processing. They are (listed by module): 

tape
    

  * player
  * recorder
powerflow
    

  * **TODO**:  node
  * **TODO**:  link
**TODO**: 
    

  * add others here
## See also

  * Subsecond
    * Development 
      * Requirements
      * Specifications
      * Programmer's Manual
    * Global variables
      * simulation_mode
      * deltamode_maximumtime
      * deltamode_timestep
      * deltamode_preferred_module_order
      * deltamode_updateorder
      * deltamode_iteration_limit
      * deltamode_forced_extra_timesteps
      * deltamode_forced_always

