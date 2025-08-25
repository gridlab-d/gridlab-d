# Performance profiling

GridLAB-D™ has a performance profiler built into the core that allows users to generate an analysis of the simulation performance. Additional measurements on class and module performance are collected to perform module/class performance analysis. The performance analysis is based on the quantitative measurements collected while the simulation is running. These measurements are then used to estimate the overall performance metrics for both the core and the modules. 

## Core profiles

The key measurements of the core performance analysis shown in Table 1 and the performance metrics are shown in Table 2. 

Table 1 - Core performance measurements  $N_{objects}$ | The total number of objects in the model.   
---|---  
$N_{threads}$ | The maximum number of concurrent threads used by the simulation.   
$T_{total}$ | The total elapsed wall-clock time of the simulation.   
$T_{sync}$ | The total wall-clock time spent in object synchronization. This the cumulative sum of the time spent in each objects' synchronization functions   
$T_{sim}$ | The total elapsed simulation time of the model. This is the simulation stop time minus the simulation start time.   
$N_{sync}$ | The total number of sync passes completed. The sync count is incremented whenever an object synchronization is requested.   
$N_{step}$ | The total number of timesteps completed. The step count is incremented each time the clock advances.   
$N_{lock}$ | The total number of locks requested. The lock count is incremented each time a lock is requested.   
$N_{spin}$ | The total number of lock spins completed. The lock spin is incremented each time a lock is requested but denied.   

Table 2 - Core performance metrics  Total objects | $N_{objects}$  
---|---  
Parallelism | $N_{threads}$  
Total time | $T_{total}$  
Core time | $T_{total} - T_{sync}$  
Model time | $T_{sync}$  
Simulation time | $T_{sim}$  
Simulation speed | $T_{sim} \times N_{objects} / T_{total}$  
Syncs completed | $N_{sync}$  
Time steps completed | $N_{step}$  
Convergence efficiency | $N_{sync}/N_{step}$  
Memory lock contention | $N_{lock}/N_{spin}$  
Average timestep | $T_{sim} / N_{sync}$  
Simulation rate | $T_{sim} / T_{total}$  
  
## Module/class profiles

The class profiles are much simpler than core profiles because they only present the amount of time spent overall in each class, the fraction of the total time spend in the class, and the time spend in the class per instance of the class. 


