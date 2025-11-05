# Spec:Subsecond

TODO - Simplify - Simplify this for integration with deltamode docs

This document provides the specifications for the subsecond simulation capability for GridLAB-D™. 

# Specifications

## S1

Simulation mode control
    A two-state global variable named [simulation_mode] shall control whether the simulation is running in quasi-steady event mode ([EVENT]) or finite time-difference mode ([DELTA]) ([R1]).

## S2

Simulation mode switching
    Modules shall be required to indicate when a switch to [DELTA] mode is desired ([R2]). The following function shall be exported by modules that need the opportunity to provide this indication before the first sync in the main exec loop:
    
    
    unsigned int deltamode_desired()
    {
       return maximum_dt; 
       /* maximum_dt=0 to indicate delta mode is not desired */
       /* maximum_dt>0 to indicate delta mode is desired */
       /* the value shall indicate the maximum timestep supported by the module */
    }
    

    The [deltamode_timestep] [global variable] shall be set to the smallest value of `maximum_dt` received form the modules.

    Note that the core shall switch to [DELTA] mode only when the model is consistent. There is no requirement for prompt mode switching when requested should the request be made when the model is inconsistent.

Delta mode switch
    The simulation shall switch from [EVENT] mode to [DELTA] mode before the first sync in the main exec loop. ([R2]).

Event mode switch
    The simulation shall switch from [DELTA] mode to [EVENT] mode after the last update in the delta update loop or when an exception in encountered ([R2]).

Delta mode time limit
    The [deltamode_timelimit] [global variable] shall indicate the maximum time in seconds that the delta mode loop shall process without allowing at least a single pass in event mode. The default value shall be 3600.

## S3

Time step control
    The time when operating in [DELTA] mode shall be stored in nanoseconds and accessible via the global variable named [deltamode_timestep] ([R3]).

## S4

Object update order
    Objects shall be updated in the bottom-up rank order (0->N) with objects of the same rank being executed in parallel, if sufficient threads are available ([R4]).

## S5

Class export function
    To enable [DELTA] mode updates a GridLAB-D™ class shall export the "C" function ([R5])
    
    
     extern "C" int _class_ _deltaupdate(OBJECT *, unsigned long dt)
     {
       /* call to class implementation */
       return 1; /* zero on failure */
     }
    

## S6

Object update enable flag
    To enable [DELTA] mode updates an object shall set the DELTAMODE flag ([R6]).

## S7

Update callbacks
    To receive pre-update, inter-update and post-update calls a module shall implement the functions ([R7])
    
    
     extern "C" int preupdate(MODULE *module, TIMESTAMP t0, unsigned int64 dt)
     {
       /* called before the first update after switch from event to delta mode */
       return 1; /* 0 on failure, 1 ok */
     }
     extern "C" int interupdate(MODULE *module, TIMESTAMP t0, unsigned int64 Dt)
     {
       /* called between update loops when clock changes in delta mode */
       return 1; /* 0 on failure, 1 remain in delta mode, 2 event mode permissible */
     }
     extern "C" int postupdate(MODULE *module, TIMESTAMP t0, unsigned int64 dt)
     {
       /* called after final update before switching from delta to event mode */
       return 1; /* 0 on failure, 1 ok */
     }
    

where _t0_ is the initial time (in seconds) at which delta mode was entered, and _dt_ elapsed time (in nanoseconds) since delta mode was entered. 

## S8

The profile data shall be presented in the following form ([R8]) 

  * Initialization time in seconds.
  * Average timestep in ms.
  * Minimum timestep in ms.
  * Maximum timestep in ms.
  * Number of updates performed.
  * Preupdate time in seconds and % of total delta time.
  * Update time in seconds and % of total delta time.
  * Interupdate time in seconds and % of total delta time.
  * Postupdate time in seconds and % of total delta time.
# Test plan

Part 1
    Default verification

  1. Check [simulation_mode]==[INIT]
  2. Check [deltamode_timestep]==10ms
  3. Check [deltamode_maximumtime]==1hr
Part 2
    Variable value ranges

  1. Check [simulation_mode]={[EVENT],[DELTA],[ERROR],[INIT]}
  2. Check [deltamode_timestep]={10000,10000000}
  3. Check [deltamode_maximumtime]={1000000000,3600000000000}
Part 3
    Simple model check
    
    
    clock {
    	timezone "PST+8PDT";
    	starttime '2001-01-01 00:00:00 PST';
    	stoptime '2001-01-03 00:00:00 PST';
    }
    module generators {
    	[enable_subsecond_models] TRUE;
    }
    object [diesel_dg]:..2 {
    	[Gen_mode] [CONSTANTP];
    	[Gen_status] ONLINE;
    	flags DELTAMODE;
    }
    object [diesel_dg] {
    	[Gen_mode] [CONSTANTP];
    	[Gen_status] ONLINE;
    	flags DELTAMODE;
    	[rank] 2;
    }
    

## See also

  * [Subsecond]
    * Development 
      * [Requirements]
      * Specifications
      * [Programmer's Manual]
    * [Global variables]
      * [simulation_mode]
      * [deltamode_maximumtime]
      * [deltamode_timestep]
      * [deltamode_preferred_module_order]
      * [deltamode_updateorder]
      * [deltamode_iteration_limit]
      * [deltamode_forced_extra_timesteps]
      * [deltamode_forced_always]

