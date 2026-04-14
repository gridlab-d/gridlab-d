# TODO Report

This report contains all TODO items found in Markdown files.

## CSV reader actors
- `3.0 - Modeling Reference\Modules\Climate\CSV_reader_(climate_class).md` - l.10 - what is a CSV reader actor?


## Check Status
- `3.0 - Modeling Reference\Modules\Generators\Windturb_dg.md` - l.9 - This model remains in the experimental level of development.


## Check status
- `3.0 - Modeling Reference\Other Features\Units.md` - l.224 - Although the `.` syntax is not valid now, it should be acceptable as a multiplication on the appropriate side of the `/`. For example `a.b/c.d` should be the same as `a*b/c/d`. This would be very much more user friendly and quite easy to implement.


## Clarification
- `1.0 - Prospective Users\GridLAB-D_Key_Attributes.md` - l.30 - The following is a sentence fragment. What is a general term? Is this supposed to be related to "Model?" If so, it should not be its own bullet** A general term to describe how a particular part of GridLAB-D™ functions or is represented in code. For example, "How does GridLAB-D™ model solar panels?"


## Context
- `0.0 - GridLAB-D™\0.1 - Introduction.md` - l.47 - Should people know what a "property call" is?**.


## Define
- `1.0 - Prospective Users\Technical_Overview.md` - l.67 - Is this "Building Combined Heat and Power"? If so we should define it.** BCHP, and Grid-Friendly™ appliance controls creates a number of technology opportunities and challenges. GridLAB-D™ will permit utility managers to better evaluate the cost/benefit trade-off between infrastructure expansion investments and distributed resources investments by including the other economic benefits of DER (e.g., increase wholesale purchasing elasticity, improved reliability metrics, ancillary services products to sell in wholesale markets).


## Definition
- `3.0 - Modeling Reference\Modules\Tape\1.0 - Tape.md` - l.26 - Define gnuplot_path**


## Delete?
- `1.0 - Prospective Users\Technical_Overview.md` - l.18 - This Transmission System module no longer exists in GLD?** The transmission system functionality is included to allow for the interconnection of multiple distribution feeders. If a transmission module was not included, each distribution system could only be solved independently of other systems. While distribution systems can be solved independently, as is common in current commercial software packages, GridLAB-D™ will have the ability to generate a power flow solution for multiple distributions systems interconnected via a transmission or sub-transmission network. Traditionally, the ability to examine interactions at this level has been limited by computational power. To address this limitation, GridLAB-D™ is being developed for execution on multiple processor systems. In the current version of GridLAB-D™, the AC power flow solution method used for the transmission system is the Gauss-Seidel (GS) method, chosen for its inherent ability to solve for poor initial conditions, and to remain numerically stable in multiprocessor environments.


## Discuss API
- `2.0 - New Users\Tutorial\2.5.2 - GLM Models.md` - l.484 - Talk about GLMModifier or API here instead**:


## EMPTY
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.1 - gldcore.md` - l.113 - Triangle distribution [Dchassin] 00:07, 7 March 2009 (UTC)
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.1 - gldcore.md` - l.117 - Weibull distribution [Dchassin] 00:07, 7 March 2009 (UTC)
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.1 - gldcore.md` - l.121 - Rayleigh distribution [Dchassin] 00:07, 7 March 2009 (UTC)
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.1 - gldcore.md` - l.133 - Gamma distribution [Dchassin] 00:07, 7 March 2009 (UTC)


## Empty
- `1.0 - Prospective Users\Technical_Overview.md` - l.78 - Empty section? Remove or write**


## Examples
- `2.0 - New Users\Tutorial\2.5.3.1 - SimulationTime.md` - l.212 - Check that this Discrete Time example satisfies the needs of a Quasi-static mode example
- `2.0 - New Users\Tutorial\2.5.3.1 - SimulationTime.md` - l.370 - Transient Mode
- `3.0 - Modeling Reference\Modules\Residential\ZIPload.md` - l.211 - Examples for cycling, demand response and aggregate modes.**


## Incomplete
- `3.0 - Modeling Reference\Other Features\Microgrids.md` - l.706 - Super-second implementation details will go here - AVR and Drooping


## Introduction
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.2 - Modules.md` - l.46 - What is a GridLAB-D module?
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.2 - Modules.md` - l.47 - What is a module function?
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.2 - Modules.md` - l.48 - What is a module global?


## Keep?
- `1.0 - Prospective Users\Technical_Overview.md` - l.22 - If transmission system paragraph is deleted, this does not need to be called out as "Distribution System."** To accurately represent distribution systems, individual feeders are expressed in terms of conductor types, conductor placement on poles, underground conductor orientation, phasing, and grounding. GridLAB-D™ does not simplify distribution system component models. The distribution module of GridLAB-D™ utilizes the traditional forward and backward sweep method for solving the unbalanced 3-phase AC power flow problem. This method was selected in lieu of newer methods such as current injection methods for the same reasons that the GS method was selected for the transmission module; converging in the fewest number of iterations is not the primary goal. Similar to the transmission module, the distribution modules begin with a flat start at initialization, and all subsequent solutions will be derived from the previous time step.
- `1.0 - Prospective Users\Technical_Overview.md` - l.54 - If GLD no longer has markets module, this will probably go away also.** Today's power system simulation tools do not provide the analysis capabilities needed to study the forces driving change in the energy industry. The combined influence of fast-changing information technology, novel and cost-effective distributed energy resources, multiple and overlapping energy markets, and new business strategies result in very high uncertainty about the success of these important innovations. Some concerns expressed by utility engineers, regulators, various stakeholders, and consumers can be addressed by GridLAB-D™. Some example uses include:


## LINK
- `4.0 Developing Reference\4.5 - Testing and Debugging\Testing_and_Validation.md` - l.68 - I need to find a way to embed excel in wiki in order to display the traceability matrix.


## More context
- `3.0 - Modeling Reference\Modules\Residential\ZIPload.md` - l.87 - Document cycling, demand response and aggregate modes.**


## Needed?
- `1.0 - Prospective Users\Technical_Overview.md` - l.24 - Does this belong in "Technical Overview"? Too deep? Too shallow?** Metering is supported for both single/split phase and three-phase customers. GridLAB-D™ also supports reclosers, islanding, distributed generation models, and overbuilt lines are anticipated in coming versions.


## Ordering/Content
- `0.0 - GridLAB-D™\0.1 - Introduction.md` - l.41 - To be written after review of section**


## Quasi
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.5 - Time Management.md` - l.45 - Static Mode Synchronization Procedure - Find the diagram that Frank talks about.


## Relevance
- `1.0 - Prospective Users\Technical_Overview.md` - l.50 - Are we still developing more detailed end use behavior as we march forward in time?** Commercial loads are simulated using an aggregate multi-zone Energy Technology Perspectives (ETP) model that will be enhanced with more detailed end use behavior in coming versions.
- `1.0 - Prospective Users\Technical_Overview.md` - l.63 - Do we provide the following functionality?** GridLAB-D™ will provide the ability to model consumer choice behavior in response to multiple rate offerings (including fixed rates, demand rates, time-of-day rates, and real-time rates) to determine whether a suite of rate offerings is likely to succeed.
- `3.0 - Modeling Reference\Modules\Powerflow\Switch_object.md` - l.52 - Review whether this level of detail of model implementation is necessary


## Relevant?
- `1.0 - Prospective Users\Technical_Overview.md` - l.71 - The following sentence - Does it do this? Will it? Should it?** GridLAB-D™ will even be able to evaluate the consumer rebound effect following one or more curtailment or load-shed events in a single day.


## Review
- `2.0 - New Users\2.1 - Installation Guide.md` - l.83 - determine whether this should change or even be listed. The current default directory does not include these paths.**
- `2.0 - New Users\2.1 - Installation Guide.md` - l.99 - do these get set automatically when you select the add to path option in the executable? If not, describe what the user needs to do and what is done automatically. [JK- not sure. I don't see these set in my path, though I used the simple executable instructions. These instructions may change as well with the new release. Flag for revisit/review by Dev team]**
- `3.0 - Modeling Reference\Modules\1.0 - Introduction.md` - l.40 - Review list before next release. Add note here that says relase number it was last updated.


## Revisit
- `0.0 - GridLAB-D™\0.1 - Introduction.md` - l.37 - When the online webinars are created, polish this preceding statement**
- `2.0 - New Users\Tutorial\2.5.6 - Distributed_Generation.md` - l.138 - Update this description when energy_storage page and status has been updated.** The battery object is in a state of flux, containing some legacy models and some new models. In early versions of GridLAB-D™ (pre-v3.0), it was assumed that the model included both the battery and the inverter; the battery was connected directly to a meter (or `triplex_meter`) object. Post-v3.0, the inverter model has been separated from the battery object. In this case, the battery is connected as a child of an inverter object and the inverter is then connected to the meter object. This may cause some confusion - development of new models will focus on a full separation of the inverter and battery models, but legacy code still exists for those that are still using it.


## Status
- `3.0 - Modeling Reference\Modules\Powerflow\1.0 - Power_Flow_User_Guide.md` - l.463 - This parameter is unused at this point. Future versions of GridLAB-D™ may implement this functionality**
- `3.0 - Modeling Reference\Modules\Powerflow\4.0 - Reliability_User_Guide.md` - l.288 - is this still true?
- `3.0 - Modeling Reference\Modules\Powerflow\Classes\09 - overhead_line_conductor.md` - l.21 - This parameter is unused at this point. Future versions of GridLAB-D™ may implement this functionality**
- `3.0 - Modeling Reference\Modules\Tape\2.0 - Player.md` - l.63 - The behavior of DST is not specified in transient mode, i.e., are timestamp in the localtime or standard time? (see ticket:563).**


## UNTAGGED
- `2.0 - New Users\Tutorial\2.5.6 - Distributed_Generation.md` - l.146 - Can charging and discharging be controlled? It doesn't look like it based on the code**.)
- `3.0 - Modeling Reference\Modules\Generators\Energy_storage.md` - l.157 - - Status - update with energy storage models status
- `3.0 - Modeling Reference\Modules\Generators\Energy_storage.md` - l.161 - - Example - add example demonstrating energy storage model use case
- `3.0 - Modeling Reference\Modules\Tape\5.0 - Collector.md` - l.12 - // other properties may not be documented
- `3.0 - Modeling Reference\Other Features\Checkpoints.md` - l.5 - - Incomplete - Add in content from related checkpoint files
- `3.0 - Modeling Reference\Other Features\CommandLineOptions.md` - l.72 - write your check code here
- `4.0 Developing Reference\4.7 - Release_Process.md` - l.2 - Update outline for release process section
- `4.0 Developing Reference\4.7 - Release_Process.md` - l.4 - Write content for process section
- `4.0 Developing Reference\4.0 - Feature Development\Checkpointing.md` - l.19 - detail what API calls
- `4.0 Developing Reference\4.0 - Feature Development\Checkpointing.md` - l.23 - Repalce this as necessary - placeholder/sample left for now
- `4.0 Developing Reference\4.0 - Feature Development\Checkpointing.md` - l.77 - :
- `4.0 Developing Reference\4.0 - Feature Development\Multithreading.md` - l.29 - Needed?
- `4.0 Developing Reference\4.0 - Feature Development\Python API.md` - l.40 - - Add additional paragraph or two about the Python class assuming that materializes as planned.
- `4.0 Developing Reference\4.0 - Feature Development\Python API.md` - l.46 - Update all examples with appropriate `.get_messages()` to make sure nothing bad is happening.
- `4.0 Developing Reference\4.0 - Feature Development\Python API.md` - l.52 - - Make sure this is correct in the final version: The data types used when interacting with the model match the types used by GridLAB-D. That is, if a GridLAB-D model specifies that a value is a complex number, that is the data type returned when using the API to ask for that parameter from the specified object and the data type expected when writing to that parameter via the API.
- `4.0 Developing Reference\4.0 - Feature Development\Python API.md` - l.53 - - Make sure this is correct in the final version:Simulation time is always represented by and ISO 8601 string which is easily converted to a Python datetime object. (Or maybe in the final version with the user-facing API it is a datetime object already).
- `4.0 Developing Reference\4.0 - Feature Development\Python API.md` - l.55 - - Make sure this is correct in the final version: Trying to write an object parameter that is read-only via the API will produce a warning.
- `4.0 Developing Reference\4.0 - Feature Development\Python API.md` - l.84 - Add best practices when using the API and link to sections that demonstrate each of them.
- `4.0 Developing Reference\4.0 - Feature Development\Python API.md` - l.256 - - Update link to master branch once the features is merged in.
- `4.0 Developing Reference\4.2 - Building from Source\4.2.1 - Building_from_Source.md` - l.30 - - Update and verify pre-requisite installation command list for each build platform
- `4.0 Developing Reference\4.2 - Building from Source\4.2.3 - Setting-Up_WSL_for_Windows.md` - l.15 - Describe how to update the PATH variable for WSL
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.1 - gldcore.md` - l.11 - - link - link to appropriate page for more info.
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.2 - Modules.md` - l.96 - add gl_global_create() calls here (see module globals for details)
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.2 - Modules.md` - l.97 - call new for each class here (see create class for details)
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.2 - Modules.md` - l.98 - return oclass member of first new class
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.2 - Modules.md` - l.113 - perform cleanup actions if needed
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.2 - Modules.md` - l.131 - perform simulation end operations
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.2 - Modules.md` - l.146 - perform check operations and report issues
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.2 - Modules.md` - l.534 - - link - insert link for source doc on **set_callback** for details.
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.3 - Objects.md` - l.120 - - Empty - gld_object: Describe class members
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.5 - Time Management.md` - l.18 - Add link.
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.5 - Time Management.md` - l.101 - Add your transient mode_desired code */
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.5 - Time Management.md` - l.110 - Add preupdate code */
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.5 - Time Management.md` - l.122 - Add your interupdate code here */
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.5 - Time Management.md` - l.138 - Add your postupdate code here */
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.5 - Time Management.md` - l.164 - add your object update code here */
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.2 - Creating_a_module.md` - l.51 - - link - link to appropriate theory of operation page.
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.2 - Creating_a_module.md` - l.112 - - link - link to appropriate publishing class variables page/section.
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.2 - Creating_a_module.md` - l.122 - - link - link to appropriate publishing class functions page/section.
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.2 - Creating_a_module.md` - l.133 - - link - link to appropriate publishing class functions page/section.
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.3 - Creating_a_class.md` - l.28 - add public typedefs
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.3 - Creating_a_class.md` - l.29 - declare published variables using GL_* macros
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.3 - Creating_a_class.md` - l.31 - add private typedefs
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.3 - Creating_a_class.md` - l.32 - add unpublished variables
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.3 - Creating_a_class.md` - l.37 - add optional class functions
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.3 - Creating_a_class.md` - l.39 - add published class functions
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.3 - Creating_a_class.md` - l.41 - add desired internal functions
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.3 - Creating_a_class.md` - l.60 - add optional functions declarations
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.3 - Creating_a_class.md` - l.63 - add declaration of class globals
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.3 - Creating_a_class.md` - l.75 - set defaults
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.3 - Creating_a_class.md` - l.80 - set defaults
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.3 - Creating_a_class.md` - l.85 - initialize object
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.3 - Creating_a_class.md` - l.88 - add implementations of optional class functions
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.3 - Creating_a_class.md` - l.100 - add new classes before this line
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.3 - Creating_a_class.md` - l.114 - add other VS project options
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.3 - Creating_a_class.md` - l.118 - :
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.3 - Creating_a_class.md` - l.162 - :
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.3 - Creating_a_class.md` - l.166 - :
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.3 - Creating_a_class.md` - l.170 - :
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.3 - Creating_a_class.md` - l.174 - :
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.3 - Creating_a_class.md` - l.178 - :
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.3 - Creating_a_class.md` - l.494 - describe how modulated shapes are generated from schedules
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.3 - Creating_a_class.md` - l.498 - describe how queued shapes are generated from schedules
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.3 - Creating_a_class.md` - l.502 - describe how pulse shapes are generated from schedules
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.4 - Creating_a_solver.md` - l.14 - structure of solver data (see Step 3)
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.4 - Creating_a_solver.md` - l.29 - handle set params (see Step 4)
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.4 - Creating_a_solver.md` - l.47 - handle get params (see Step 5)
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.4 - Creating_a_solver.md` - l.59 - implement solver (see Step 6)
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.4 - Creating_a_solver.md` - l.70 - ` comment
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.4 - Creating_a_solver.md` - l.87 - ` comment
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.4 - Creating_a_solver.md` - l.102 - ` comment
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.4 - Creating_a_solver.md` - l.145 - ` comment
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.5 - GridLAB-D Device Modeling.md` - l.3 - - Consider for PNNL report or tutorial or tutorial video or some other in-depth treatment.
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.5 - GridLAB-D Device Modeling.md` - l.132 - :
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.5 - GridLAB-D Device Modeling.md` - l.136 - :
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.5 - GridLAB-D Device Modeling.md` - l.140 - :
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.5 - GridLAB-D Device Modeling.md` - l.144 - :
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.5 - GridLAB-D Device Modeling.md` - l.148 - :
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.5 - GridLAB-D Device Modeling.md` - l.152 - :
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.5 - GridLAB-D Device Modeling.md` - l.156 - :
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.5 - GridLAB-D Device Modeling.md` - l.160 - :
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.5 - GridLAB-D Device Modeling.md` - l.206 - Add an example of a full-fledged class with runtime components.
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.5 - GridLAB-D Device Modeling.md` - l.428 - implement your function here
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.5 - GridLAB-D Device Modeling.md` - l.430 - return data pointer
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.5 - GridLAB-D Device Modeling.md` - l.440 - add arguments to call
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.7 - Issue Tracking.md` - l.23 - " column.  It may also be assigned a responsible party at this step.  if the issue is found to be unrelated to the project scope, the reviewing party will remove or reassign the project flag (the Issue remains open, but goes back into the general pool).
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.7 - Issue Tracking.md` - l.26 - " Issue, that Issue should be moved to the "In Progress" column.  Assignments will be performed by the periodic reviewer, or by self-assignment of a relevant developer.
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.7 - Issue Tracking.md` - l.39 - " - the Issue is relevant to the overall Project and should be completed in the next few weeks
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.7 - Issue Tracking.md` - l.40 - " to "In Progress" - the Issue has a developed assigned and work addressing the request/issue has begun
- `4.0 Developing Reference\4.5 - Testing and Debugging\Assert.md` - l.152 - - Empty - Double Assert section is
- `4.0 Developing Reference\4.5 - Testing and Debugging\Assert.md` - l.189 - - Empty - Add content for Enumeration Assert


## Update
- `2.0 - New Users\Tutorial\2.5.6 - Distributed_Generation.md` - l.175 - the load following control mode currently only operates at unit power factor. A similar control mode for reactive load following is slated for 3.2 release in summer of 2015.
- `3.0 - Modeling Reference\Modules\Generators\Inverters\Spec_sync_ctrl.md` - l.127 - Review flowchart for accuracy and update
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.2 - Modules.md` - l.191 - The `test` function is relatively unused and was intended to support module tests.
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.2 - Modules.md` - l.195 - The `stream` function will soon be required to support checkpoints.


## Verify
- `3.0 - Modeling Reference\Other Features\Validate.md` - l.200 - does not appear to do that.)
- `3.0 - Modeling Reference\Other Features\Validate.md` - l.202 - does not appear to do that.)
- `3.0 - Modeling Reference\Other Features\Validate.md` - l.208 - only checks syntax, does not check proper functionality).


---

**Summary:** 139 TODO items found across 28 stages.
