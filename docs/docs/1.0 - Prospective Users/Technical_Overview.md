# Technical Overview

The GridLAB-D™ system currently implements modules to perform the following functions: 

  * Power and energy flow and control.
  * Load electric, thermal, and control behavior.
  * Economic behaviors.
  * Data collection and analysis.
  * Physical and economic boundary condition management.
  * Integration with other software.

# Power Flow

The power flow component of GridLAB-D™ is separated into a distribution module and a transmission module. While distribution systems are the primary focus of GridLAB-D™, the transmission module is included so that the interactions between two or more distribution systems can be simulated. 

## Transmission System

**TODO - Delete? - This Transmission System module no longer exists in GLD?** The transmission system functionality is included to allow for the interconnection of multiple distribution feeders. If a transmission module was not included, each distribution system could only be solved independently of other systems. While distribution systems can be solved independently, as is common in current commercial software packages, GridLAB-D™ will have the ability to generate a power flow solution for multiple distributions systems interconnected via a transmission or sub-transmission network. Traditionally, the ability to examine interactions at this level has been limited by computational power. To address this limitation, GridLAB-D™ is being developed for execution on multiple processor systems. In the current version of GridLAB-D™, the AC power flow solution method used for the transmission system is the Gauss-Seidel (GS) method, chosen for its inherent ability to solve for poor initial conditions, and to remain numerically stable in multiprocessor environments. 

## Distribution System

**TODO - Keep? - If transmission system paragraph is deleted, this does not need to be called out as "Distribution System."** To accurately represent distribution systems, individual feeders are expressed in terms of conductor types, conductor placement on poles, underground conductor orientation, phasing, and grounding. GridLAB-D™ does not simplify distribution system component models. The distribution module of GridLAB-D™ utilizes the traditional forward and backward sweep method for solving the unbalanced 3-phase AC power flow problem. This method was selected in lieu of newer methods such as current injection methods for the same reasons that the GS method was selected for the transmission module; converging in the fewest number of iterations is not the primary goal. Similar to the transmission module, the distribution modules begin with a flat start at initialization, and all subsequent solutions will be derived from the previous time step. 

**TODO - Needed? - Does this belong in "Technical Overview"? Too deep? Too shallow?** Metering is supported for both single/split phase and three-phase customers. GridLAB-D™ also supports reclosers, islanding, distributed generation models, and overbuilt lines are anticipated in coming versions. 

The following power distribution system components are implemented and available for use: 

* Overhead and underground lines
* Transformers
* Voltage regulators
* Fuses and switches
* Capacitor banks
* Metering for single/split-phase and three-phase service

This supports analysis of feeder behavior, voltage performance, protection and control interactions, and distributed resource impacts under realistic operating conditions.

## End-Use and Building Load Modeling

GridLAB-D™ includes physics-based load models for residential and commercial applications. Residential building behavior is represented using the [Equivalent Thermal Parameters (ETP)](../3.0%20-%20Modeling%20Reference/Modules/Residential/ETP_closed_form_solution.md) model, with end-use detail for appliances and household systems.

Representative residential end uses include:

* Water heaters
* Plug loads
* Occupancy loads
* Lighting
* HVAC loads, including heat pumps and air conditioning
* EV chargerrs

These capabilities allow users to study how customer behavior and building dynamics affect feeder-level power demand and system performance.

## Control, Automation, and Scenario Analysis

GridLAB-D™ supports time-series simulation of control and automation strategies, including studies involving:

* Voltage and reactive power management
* Distributed energy resource integration
* Demand response and peak-load management workflows
* Reliability and operational scenario analysis

The modular architecture allows studies to combine electrical, control, and load dynamics in a single simulation workflow.

## Data Collection and Output Workflows

GridLAB-D™ provides built-in mechanisms for recording, replaying, and post-processing simulation data. Users can capture both object-level and system-level outputs to support validation, benchmarking, and downstream analytics.

## Integration and Extensibility

GridLAB-D™ is designed to integrate with external tools and custom workflows. In current practice, integration commonly includes:

* Python-based orchestration and automation
* Co-simulation and cross-tool workflows (for example, HELICS-based workflows)
* Batch studies and scripted analysis pipelines

As capabilities evolve across releases, users should consult the version-specific Modeling Reference and release notes for implementation details.