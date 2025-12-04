# Delta Mode Document

## 1.0 Prospective Users
The electric grid regularly experiences rapidly-evolving events (e.g., faults, generator transients) that need to be modeled accurately. GridLAB-D is capable of simulating events occurring on a timescale of milliseconds in order to offer insight into such dynamic behavior. This mode of operation is called "Delta Mode".

The detailed grid modeling necessary for events such as faults is computationally intensive. Thus, the electric grid is modeled as a steady-state system to manage the computational burden, when appropriate. Events unfolding over slower timescales of minutes or hours (e.g., a gradual increase in consumer demand) are usually modeled as a succession of steady states; a method known as quasi-static time-series simulation. This is GridLAB-D's main mode of operation. 

GridLAB-D users can pick the appropriate mode of operation depending on their needs. For example, a user interested in modeling the energization of a transformer should choose "Delta Mode" simulations **TODO:re-iterate rationale**, while a user interested in observing the behavior of the system over a day of operations should remain in the main quasi-static mode of operation.

However, real power systems experience events that cannot be neatly split into one of these two categories. For example, a user may be interested in simulating the behavior of a system during an hour in which a fault happens and then is cleared. Tools focused on dynamic simulations usually are either incapable of simulating a full hour of grid operation or need large amounts of time. On the other hand, tools specializing in quasi-static time-series simulations cannot accurately capture the dynamic behavior of the system during a fault. GridLAB-D can seamlessly transition between quasi-static and dynamic simulations to take advantage of the benefits each simulation mode offers in different situations, thus offering increased flexibility to its users. **TODO: Describe whether this transition is automatic or user-enabled in some way**

- Branding for "Delta mode" and quasistatic mode (maybe QSTS as in the paper? maybe event-driven mode as in the presentation?).

## 2.0 New Users (Simulation Time -> The clock)
There are two main ways in which the GridLAB-D global clock may advance: synchronously or asynchronously. 

Asynchronous clock operation is the default; GridLAB-D simulations start in this mode, which is also known as event-driven mode. During asynchronous operation the global clock advances by varying time steps depending on the pending state changes of the model's objects until no object reports a need for a future state change. Objects may schedule requests for future state changes at a granularity of one second. An important function of the asynchronous mode of operation is to enable quasi-static time-series simulations of the electric grid. Users that wish to use fixed time steps in quasistatic domain simulations may add a recorder (and potentially also set an appropriate global minimum time step).

-	Where else do we document this? (We can use conversation with Andy Fisher from November 26.)

Synchronous clock operation is associated with sub-second time-domain "Delta Mode" simulations. Here, the global clock advances by fixed time-steps at each iteration, while ignoring any scheduled events. This helps capture fast transients described by differential equations. 

GridLAB-D monitors when one or more objects may desire to enter “Delta Mode”, as well as their preferred time step, to decide when to transition from asynchronous to synchronous operations. In this case, all objects that support synchronous operation enter “Delta Mode” using the minimum requested time step. When these objects report that they have converged to steady-state conditions again, “Delta Mode” is exited, and asynchronous, event-driven simulation is resumed.

It is important to note that state-dependent models that lack “Delta Mode” support, such as water heaters or capacitors, do not update their state during asynchronous operation. Their state is only updated when “Delta Mode” is exited. This may affect simulation results, is “Delta Mode” is used for extended periods of time.

## 3.0 Modeling
- IF we keep the current structure, go to the power flow module and add the slide from Frank's presentation there.

## TO DO
- Move the pre-commit and post-commit info to development. Done.

More comments.
-	Check how detailed delta mode simulations are. Can they capture inverters or other things that happen faster than electromechanical simulations such as electromagnetic transients? What’s the status of the “Status of Development” slide? This has implications for the introductory sections, as well.
-	Branding for Delta Mode?
-	What triggers Delta Mode?
