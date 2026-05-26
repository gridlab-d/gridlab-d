# GridLAB-D Example Suite

## Existing Examples
This folder contains the existing suite of examples (gleaned from various sources) that are candidates for inclusion in a final documentation examples. These show off various features of GridLAB-D and, in the case of those pulled from the old tutorial on the wiki, create a progression from basics to more advanced features.

Each of these examples has its own folder which contains the .glm, any supporting files, and a "README" with metadata about the example including its purpose. It is expected that all of these examples are in a functioning state though this may or may not have been confirmed recently.

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
The new API will allow for a broad new paradigm for using GridLAB-D and showing off the capabilities is, in my (Trevor's) opinion will revolutionize the use of GridLAB-D. The more of these kinds of things we can show, the bigger the vision for GLD we can cast and the more likely it is to be used more broadly.

- Basic use 
    - Loading and running a model (just like using GridLAB-D today)
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
    
