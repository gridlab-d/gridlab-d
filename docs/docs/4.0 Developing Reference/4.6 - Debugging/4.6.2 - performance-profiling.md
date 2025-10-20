# Performance Profiling

GridLAB-D™ has a performance profiler built into the core that allows users to generate an analysis of the simulation performance. Additional measurements on class and module performance are collected to perform module/class performance analysis. The performance analysis is based on the quantitative measurements collected while the simulation is running. These measurements are then used to estimate the overall performance metrics for both the core and the modules.


## Core profiles
The key measurements of the core performance analysis shown in Table 1 and the performance metrics are shown in Table 2.

Table 1 - Core performance measurements
| Property      | Description |
|---------------|-------------|
| N_objects     | The total number of objects in the model. |
| N_threads     | The maximum number of concurrent threads used by the simulation. |
| T_total       | The total elapsed wall-clock time of the simulation. |
| T_sync        | The total wall-clock time spent in object synchronization. This is the cumulative sum of the time spent in each object's synchronization functions. |
| T_sim         | The total elapsed simulation time of the model. This is the simulation stop time minus the simulation start time. |
| N_sync        | The total number of sync passes completed. The sync count is incremented whenever an object synchronization is requested. |
| N_step        | The total number of timesteps completed. The step count is incremented each time the clock advances. |
| N_lock        | The total number of locks requested. The lock count is incremented each time a lock is requested. |
| N_spin        | The total number of lock spins completed. The lock spin is incremented each time a lock is requested but denied. |

Table 2 - Core performance metrics
| Metric                  | Formula / Value Description                                 |
|-------------------------|-------------------------------------------------------------|
| Total objects           | N_objects                                                   |
| Parallelism             | N_threads                                                   |
| Total time              | T_total                                                     |
| └─ Core time            | T_total - T_sync                                            |
| └─ Model time           | T_sync                                                      |
| Simulation time         | T_sim                                                       |
| Simulation speed        | (T_sim × N_objects) / T_total                               |
| Syncs completed         | N_sync                                                      |
| Time steps completed    | N_step                                                      |
| Convergence efficiency  | N_sync / N_step                                             |
| Memory lock contention  | N_lock / N_spin                                             |
| Average timestep        | T_sim / N_sync                                              |
| Simulation rate         | T_sim / T_total                                             |


## Module/class profiles

The class profiles are much simpler than core profiles because they only present the amount of time spent overall in each class, the fraction of the total time spend in the class, and the time spend in the class per instance of the class.