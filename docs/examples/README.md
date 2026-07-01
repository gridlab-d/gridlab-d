# GridLAB-D™ Example Suite

## Existing Examples
This folder contains the existing suite of examples (gleaned from various sources) that are candidates for inclusion in a final documentation examples. These show off various features of GridLAB-D™ and, in the case of those pulled from the old tutorial on the wiki, create a progression from basics to more advanced features.

Each of these examples has its own folder which contains the .glm, any supporting files, and a "README" with metadata about the example including its purpose. It is expected that all of these examples are in a functioning state though this may or may not have been confirmed recently.

### Outline of Examples

#### Core Concepts

- **[object_based](object_based/)** — A walkthrough of a GridLAB-D™ model file intended for reading rather than running. Introduces the concept of classes and objects and how they are defined and referenced in a `.glm`.

- **[discrete_time](discrete_time/)** — Illustrates how GridLAB-D™ advances through simulation time in response to object-driven events. Uses a recorder to show how the choice of recording interval controls the granularity of time steps and affects total run time.

#### Data Input and Output

- **[players](players/)** — Demonstrates the tape module's player object for injecting time-varying data from an external CSV file into a running simulation. Covers basic player syntax and file format.

- **[recorders](recorders/)** — Demonstrates the tape module's recorder object for capturing property values from simulation objects to a CSV file. Covers basic recorder syntax and output format.

- **[schedules_direct](schedules_direct/)** — Shows how to use a `schedule` block to directly assign a time-varying value to an object property, without requiring an external player file.

- **[schedules_modify](schedules_modify/)** — Shows how to use a `schedule` block in combination with `modify` to scale or offset an object property over time, allowing relative adjustments rather than absolute assignment.

#### Basic Distribution System Modeling

- **[dist_system_basics](dist_system_basics/)** — A minimal three-phase distribution feeder model demonstrating the core powerflow objects (nodes, links, loads). Intended as a first introduction to modeling electrical networks in GridLAB-D™.

- **[dist_system_basics_alt](dist_system_basics_alt/)** — Extends the basic distribution model to illustrate alternative `.glm` syntax options: quoted strings, inline (embedded) object definitions, recording real and imaginary parts separately, and shorthand phase notation.

#### Residential Loads

- **[residential_load_basics](residential_load_basics/)** — Introduces the `house` object and demonstrates how weather data (loaded from a TMY3 file) drives HVAC system operation. Provides a starting point for all residential load modeling.

- **[residential_location](residential_location/)** — Builds on the basic house model to compare HVAC runtime across multiple geographic locations by swapping in different TMY3 weather files, highlighting the sensitivity of energy consumption to climate.

- **[residential_other_loads](residential_other_loads/)** — Demonstrates how to add implicit end-use loads (e.g., lighting, appliances) to a house object beyond the HVAC system, giving a more complete picture of residential energy consumption.

- **[residential_thermostat_settings](residential_thermostat_settings/)** — Shows the effect of heating and cooling thermostat setpoints on indoor air temperature dynamics, providing insight into occupant comfort and load control opportunities.

- **[zip_loads](zip_loads/)** — Demonstrates the ZIP (constant impedance, constant current, constant power) composite load model. Useful for understanding load-voltage sensitivity in powerflow studies.

#### Distributed Generation

- **[dist_gen_solar](dist_gen_solar/)** — Models a photovoltaic solar installation using the `solar` and `inverter` objects, first on a single house and then deployed across a group of houses. Introduces the interaction between weather-driven generation and household loads.

- **[dist_load_following](dist_load_following/)** — Demonstrates the inverter's load-following control mode with a battery storage device, where the inverter automatically adjusts output to track a load or generation target.

- **[dist_battery_solar](dist_battery_solar/)** — Combines a battery and a solar array under a shared inverter to demonstrate load-following using both generation and storage together.

#### Advanced Distribution System Modeling

- **[capacitors](capacitors/)** — Models switched shunt capacitor banks and demonstrates how they are automatically dispatched to regulate voltage on a distribution feeder.

- **[voltage_regulator_ldc](voltage_regulator_ldc/)** — Demonstrates a voltage regulator operating in line drop compensator (LDC) mode, where the regulator estimates voltage at a downstream point to drive its tap position.

- **[voltage_regulator_sense](voltage_regulator_sense/)** — Demonstrates a voltage regulator using a remote voltage sensing point, allowing the regulator to respond directly to the measured voltage at a specific location on the feeder.

#### Deltamode (Transient Simulation)

- **[deltamode_player](deltamode_player/)** — Shows how a player file can be used to trigger a transition from quasi-steady-state (QSTS) simulation into deltamode for transient analysis. Includes an induction motor model whose dynamic behavior is observed during the transient event.

#### Advanced Features

- **[meter_off](meter_off/)** — Demonstrates how to interrupt power to a house by deactivating its meter object while allowing the house's thermal model to continue evolving. Useful for studying outage impacts and restoration.

- **[integrated_helics_ev](integrated_helics_ev/)** — Shows how to run GridLAB-D™ as a HELICS federate alongside a Python co-simulation controller. A Python script manages the charging power set points for a group of EV chargers modeled in GridLAB-D™.

#### Python API

The `api/` folder contains Python scripts demonstrating the GridLAB-D™ Python API (`gridlabd` module). The examples are roughly organized from basic usage to more advanced applications.

- **[example_sim_start_stop.py](api/example_sim_start_stop.py)** — Loads a model and modifies the simulation start and stop times via the API before running, illustrating pre-run model configuration.

- **[example_sim_stepping.py](api/example_sim_stepping.py)** — Demonstrates how to step a simulation forward through time using explicit API calls rather than running it to completion in one shot.

- **[example_get_messages.py](api/example_get_messages.py)** — Shows how to retrieve informational, warning, and error messages produced by GridLAB-D™ during a simulation via the API.

- **[example_reading_data.py](api/example_reading_data.py)** — Reads property values (indoor air temperatures) from a running simulation at each time step and writes them to an HDF5 file. Also demonstrates adaptive step sizing based on observed system state.

- **[example_read_write_data.py](api/example_read_write_data.py)** — Shows bidirectional data exchange during a running simulation: reads solar power output and writes modified thermostat setpoints to implement a simple pre-cooling controller entirely in Python.

- **[example_app_gld_monitor.py](api/example_app_gld_monitor.py)** — A `tkinter`-based GUI that runs a GridLAB-D™ simulation and plots a live time-series of selected object properties as the simulation advances.

- **[example_app_sim_monitor_gui.py](api/example_app_sim_monitor_gui.py)** — A more fully featured interactive desktop GUI for loading a `.glm`, configuring start/stop times and step size, selecting objects and properties to plot on multiple axes, and controlling simulation playback (start, stop, step, restart).

- **[example_app_model_edit_gui.py](api/example_app_model_edit_gui.py)** — A `tkinter` GUI for batch-editing object properties in a loaded model. Supports class-filtered random property adjustments before running the simulation.

- **[example_app_gld_db_read_write.py](api/example_app_gld_db_read_write.py)** — Demonstrates using a PostgreSQL database as the data source and sink for a simulation: reads thermostat setpoint schedules from the database and writes indoor temperature results back to it at each time step.

- **[example_app_pp_gld_pf.py](api/example_app_pp_gld_pf.py)** — Integrates pandapower (transmission-level powerflow) with GridLAB-D™ (distribution-level) to perform a co-simulated transmission-and-distribution (T+D) powerflow with optional microstepping for convergence.

- **[example_multi_gld.py](api/example_multi_gld.py)** — Demonstrates launching and running multiple GridLAB-D™ instances in parallel from a single Python script.

- **[example_app_gld_ep/](api/example_app_gld_ep/)** — Integrates GridLAB-D™ with EnergyPlus (via its Python API) to co-simulate residential electrical loads alongside detailed whole-building energy models.

## Proposed New Examples
The following list of examples are being proposed for creation and addition to the existing example suite. It is undefined how they may or may not integrate with any proposed or existing tutorial. 

### Deltamode
There is one existing deltamode example that shows how to trigger deltamode with a player file. It would be good to broaden the suite to better show off how to use deltamode and what it can do.

- Motor operation in deltamode - This is demonstrated in the existing example but it would be good to expand it to more clearly demonstrate changes in the motor parameters and how they impact transient performance
- QSTS to deltamode and back to QSTS - Maybe triggering a transient event like opening a switch (dropping load) that triggers deltamode for a few seconds and then moves back to QSTS for the remainder of the simulated day.
- Deltamode with inverter functions - The microgrid program and stuff Wei Du has been doing uses deltamode a lot in conjunction with inverter control operations that are only realized in deltamode.

### Wind Turbines
This is likely low priority because distributed wind is not as common but we do have a distributed wind generator so maybe we should show it off.

- windturb_dg example

### Inverter Control Modes
There are a large number of control modes in the inverter (both QSTS and deltamode) that have not been well-demonstrated in an example. In the spirit of "if its not documented, its an Easter egg", we should document some of these. I (Trevor) don't know what these features are.

- Inverter control mode 1
- Inverter control mode 2
- Inverter control mode 3
- ...

### Markets
There is a lot of existing documentation on how the market module is used (transactive energy was one of the first GLD use cases) but I don't think we have as many well-formed examples showing this in action. TE is more commonly implemented via co-simulation these days so we need to make sure this functionality is going to be retained (and thus needs documenting)

 - One-way markets (passive controller) - Non-bidding but price-responsive devices
 - Two-way markets - Devices bid into a retail market that clears
 
 ### Reliability Module
 GLD's reliability module is not well-documented and I (Trevor) don't know if there's any good examples. Like the inverter controls, we need to understand what functionality this provides prior to defining the examples we want to write.
 
- Reliability example 1
- Reliability example 2
- Reliability example 3
- ...

### Voltage, Current, and Impedance Dumps
GLD has the ability to provide comprehensive representations of the powerflow model and state; it would be good to have an example showing how these are useful. These may be most useful when accessed via the API (if the API provides such affordances) as it would allow users to inspect the state of their model as part of debugging.

- Using voltage and current dumps to look for out-of-bounds values - If there's a way to programmatically access the GLD-generated warnings and errors that would be a more straight-forward way of achieving a similar goal. It would still be good, though, to show the value of these dumps, though. Show how these values can help find errors in the model. Could be related to looking at the impedance dump.
- Using impedance dumps to look for modeling errors (very high or low impedance values)

### API Use
The new API will allow for a broad new paradigm for using GridLAB-D™ and showing off the capabilities is, in my (Trevor's) opinion will revolutionize the use of GridLAB-D™. The more of these kinds of things we can show, the bigger the vision for GLD we can cast and the more likely it is to be used more broadly.

- Basic use 
    - Loading and running a model (just like using GridLAB-D™ today)
    - Model modification prior to running
    - Accessing warning and error messages when loading and running a model.
- Runtime operations
    - Controlling time in GridLAB-D
    - Reading and writing during runtime - replacement for tape module recorders and players
    - HELICS integration - We can use a presumed existing reference implementation
    - MATLAB integration - Since lots of students use MATLAB it would be good to show this integration. Maybe an Python script that [integrates both MATLAB and GLD](https://www.mathworks.com/help/matlab/matlab_external/call-matlab-functions-from-python.html) as well as a [MATLAB script that calls the GLD APIs](https://www.mathworks.com/help/matlab/call-python-libraries.html). Maybe we do a T+D simulation using MATPOWER?
    - Database data collection - SQL or other time-series database as a replacement for tape
    - Integration with popular libraries - matplotlib, pandas, tkinter (GUI)
    - Asynchronous API calls (if that takes a unique syntax) - This would allow GLD to do things like solve the model while the calling script does things (like make a graph), allowing for greater computational efficiency.
    - T+D integration using PyPOWER - This is effectively co-simulation without HELICS and is pretty cool.
    
