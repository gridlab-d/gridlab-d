# Introduction

From a power system generation perspective, GridLAB-D™ objects categorize generators by:

- physics-based models, and
- grid integration and interaction control for additional grid services.

**Physics-Based Models for Grid Stability and Dynamics.** Physics-Based Models group generators by how they interact with the grid's physical variables like frequency, voltage, and rotor angles. GridLAB-D™ provides objects for

- **Synchronous Generators:** traditional thermal, hydro, diesel, and nuclear plants. They couple directly to grid frequency, providing natural rotary inertia and high short-circuit current to stabilize voltage.
- **Inverter-Based Resources (IBRs):** solar PV, wind turbines, and battery storage. They use power electronics to convert DC or variable AC to grid frequency.
- **Grid-Following (GFL) IBRs:** most current modes for wind and solar generators. They require an existing grid voltage signal to lock onto and inject current.
- **Grid-Forming (GFM) IBRs:** advanced modern inverters. They act as a voltage source, capable of black-starting a grid and mimicking physical inertia.

**Managerial and Controlling Models for Dispatch and Economics.** Managerial models allow generators to participate in certain grid services, such as

- **Secondary frequency control** to maintain grid stability through Automatic Generation Control (AGC), and/or
- **Generator synchronization** to coordinate the connection of distributed generation units to the grid.