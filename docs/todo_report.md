# TODO Report

This report contains all TODO items found in Markdown files.

## CSV reader actors
- `3.0 - Modeling Reference\Modules\Climate\CSV_reader_(climate_class).md` - l.10 - what is a CSV reader actor?


## Check Status
- `3.0 - Modeling Reference\Metrics & Recorders\plotting-output.md` - l.69 - Future
- `3.0 - Modeling Reference\Modules\Generators\Windturb_dg.md` - l.9 - This model remains in the experimental level of development.


## Check status
- `3.0 - Modeling Reference\Other Features\Units.md` - l.224 - Although the `.` syntax is not valid now, it should be acceptable as a multiplication on the appropriate side of the `/`. For example `a.b/c.d` should be the same as `a*b/c/d`. This would be very much more user friendly and quite easy to implement.


## Clarification
- `1.0 - Prospective Users\GridLAB-D_Key_Attributes.md` - l.30 - The following is a sentence fragment. What is a general term? Is this supposed to be related to "Model?" If so, it should not be its own bullet** A general term to describe how a particular part of GridLAB-D™ functions or is represented in code. For example, "How does GridLAB-D™ model solar panels?"


## Climate Actors
- `3.0 - Modeling Reference\Modules\Climate\1.0 - Climate.md` - l.75 - what is a climate actor?


## Consistency
- `2.0 - New Users\Tutorial\2.2.1 - Overview.md` - l.3 - ...grid technologies or technology? In the next sentance, "load modeling technology" seems odd. Can we say, "load modeling software or programs?" Technology seems like an odd word here *[jk 12/1 I almost feel like "capabilities" is the more apropriate word here. Trevor?]*** It incorporates advanced modeling techniques with high-performance algorithms to deliver the latest in end use load modeling technology integrated with three-phase unbalanced power flow and retail market systems. Historically, the inability to effectively model and evaluate smart grid technologies has been a barrier to adoption; GridLAB-D™ is designed to address this problem.


## Context
- `0.0 - GridLAB-D™\0.1 - Introduction.md` - l.44 - Should people know what a "property call" is?**.


## Define
- `1.0 - Prospective Users\Technical_Overview.md` - l.67 - Is this "Building Combined Heat and Power"? If so we should define it.** BCHP, and Grid-Friendly™ appliance controls creates a number of technology opportunities and challenges. GridLAB-D™ will permit utility managers to better evaluate the cost/benefit trade-off between infrastructure expansion investments and distributed resources investments by including the other economic benefits of DER (e.g., increase wholesale purchasing elasticity, improved reliability metrics, ancillary services products to sell in wholesale markets).


## Delete?
- `1.0 - Prospective Users\Technical_Overview.md` - l.18 - This Transmission System module no longer exists in GLD?** The transmission system functionality is included to allow for the interconnection of multiple distribution feeders. If a transmission module was not included, each distribution system could only be solved independently of other systems. While distribution systems can be solved independently, as is common in current commercial software packages, GridLAB-D™ will have the ability to generate a power flow solution for multiple distributions systems interconnected via a transmission or sub-transmission network. Traditionally, the ability to examine interactions at this level has been limited by computational power. To address this limitation, GridLAB-D™ is being developed for execution on multiple processor systems. In the current version of GridLAB-D™, the AC power flow solution method used for the transmission system is the Gauss-Seidel (GS) method, chosen for its inherent ability to solve for poor initial conditions, and to remain numerically stable in multiprocessor environments.
- `2.0 - New Users\Tutorial\2.2.1 - Overview.md` - l.30 - Rest of this paragraph, while interesting to us, is irrelevant for using GLD. [jk- agree. Maybe move to our version history section for a little historical background?]** Back then, David Chassin and Ross Guttromson were commissioned under the Laboratory’s Energy Systems Transformation Initiative to look into a) whether such a software system could be built, b) whether it could model how energy systems might evolve over time, and c) how much value would this evolution bring to consumers and utilities.  In 2007, after the US Department of Energy's Office of Electricity committed to getting the results of that work more widely available, the open-source model of development and distribution was used to make sure that as many people as possible could both contribute to it and benefit from what it has to offer.  Since then, GridLAB-D™ has grown quickly, mainly because of the hard work and dedication of all the contributors, and of course the early dedication of the GridLAB-D™ team at PNNL.
- `2.0 - New Users\Tutorial\2.2.1 - Overview.md` - l.38 - What section are we talking about? The guide?If we keep this bulleted list, it should reflect the order we're utilizing (is that 2.3, 2.4, 2.5, and so forth?) [jk- agree, delete, we've rearranged so much since this was written]** In this section we will discuss the following:


## Edit
- `0.0 - GridLAB-D™\0.1 - Introduction.md` - l.10 - "shorter time periods than [what]"? Just say "increasingly shorter time periods"? Is the markets module being deleted? [JK- agree, should say something like energy trading products are traded at increasingly shorter time periods]** shorter time periods and demand response programs are moving more and more toward real-time pricing. Market-based trading activity impacts ever more directly the physical operation of the system and the boundaries of these coupled systems extend beyond the traditional boundaries of utility-centric energy system operations. To address the gaps in our simulation capabilities, the US Department of Energy is developing GridLAB-D™ at Pacific Northwest National Laboratory in collaboration with industry and academia. This is the first of a new generation of power distribution system simulation software.
- `0.0 - GridLAB-D™\0.1 - Introduction.md` - l.15 - much more accurate than what? [JK- from the official brochure: The advantages of this algorithm over traditional finite difference-based simulators are that it: 1) handles unusual situations much more accurately**]


## Empty
- `1.0 - Prospective Users\Technical_Overview.md` - l.78 - Empty section? Remove or write**


## Figure Scaling
- `0.0 - GridLAB-D™\style-guide.md` - l.97 - Have yet to figure out how to rescale an image that renders correctly in our documentation. Would like to add some auto-formatter to resize all images to be page-width in size.


## Incomplete
- `3.0 - Modeling Reference\Modules\Climate\1.0 - Climate.md` - l.82 - Fill in the rest of these property descriptions
- `3.0 - Modeling Reference\Other Features\Microgrids.md` - l.660 - Super-second implementation details will go here - AVR and Drooping


## Introduction
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.2 - Modules.md` - l.7 - What is a GridLAB-D module?
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.2 - Modules.md` - l.8 - What is a module function?
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.2 - Modules.md` - l.9 - What is a module global?


## Keep?
- `1.0 - Prospective Users\Technical_Overview.md` - l.22 - If transmission system paragraph is deleted, this does not need to be called out as "Distribution System."** To accurately represent distribution systems, individual feeders are expressed in terms of conductor types, conductor placement on poles, underground conductor orientation, phasing, and grounding. GridLAB-D™ does not simplify distribution system component models. The distribution module of GridLAB-D™ utilizes the traditional forward and backward sweep method for solving the unbalanced 3-phase AC power flow problem. This method was selected in lieu of newer methods such as current injection methods for the same reasons that the GS method was selected for the transmission module; converging in the fewest number of iterations is not the primary goal. Similar to the transmission module, the distribution modules begin with a flat start at initialization, and all subsequent solutions will be derived from the previous time step.
- `1.0 - Prospective Users\Technical_Overview.md` - l.54 - If GLD no longer has markets module, this will probably go away also.** Today's power system simulation tools do not provide the analysis capabilities needed to study the forces driving change in the energy industry. The combined influence of fast-changing information technology, novel and cost-effective distributed energy resources, multiple and overlapping energy markets, and new business strategies result in very high uncertainty about the success of these important innovations. Some concerns expressed by utility engineers, regulators, various stakeholders, and consumers can be addressed by GridLAB-D™. Some example uses include:


## Needed?
- `1.0 - Prospective Users\Technical_Overview.md` - l.24 - Does this belong in "Technical Overview"? Too deep? Too shallow?** Metering is supported for both single/split phase and three-phase customers. GridLAB-D™ also supports reclosers, islanding, distributed generation models, and overbuilt lines are anticipated in coming versions.


## Quasi
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.5 - Time Management.md` - l.44 - Static Mode Synchronization Procedure - Find the diagram that Frank talks about.


## Reference
- `2.0 - New Users\Tutorial\2.2.1 - Overview.md` - l.28 - Previous linke does not seem to work [jk-updated should work]** However, we suggest that all beginners and most intermediate users continue through this guide.


## Relevance
- `1.0 - Prospective Users\Technical_Overview.md` - l.50 - Are we still developing more detailed end use behavior as we march forward in time?** Commercial loads are simulated using an aggregate multi-zone Energy Technology Perspectives (ETP) model that will be enhanced with more detailed end use behavior in coming versions.
- `1.0 - Prospective Users\Technical_Overview.md` - l.63 - Do we provide the following functionality?** GridLAB-D™ will provide the ability to model consumer choice behavior in response to multiple rate offerings (including fixed rates, demand rates, time-of-day rates, and real-time rates) to determine whether a suite of rate offerings is likely to succeed.
- `3.0 - Modeling Reference\Modules\Powerflow\Switch_object.md` - l.52 - Review whether this level of detail of model implementation is necessary


## Relevant
- `3.0 - Modeling Reference\Modules\Residential\ETP_closed_form_solution.md` - l.527 - Should this go somewhere or is it no longer relevant?


## Relevant?
- `1.0 - Prospective Users\Technical_Overview.md` - l.71 - The following sentence - Does it do this? Will it? Should it?** GridLAB-D™ will even be able to evaluate the consumer rebound effect following one or more curtailment or load-shed events in a single day.


## Review
- `0.0 - GridLAB-D™\0.1 - Introduction.md` - l.34 - this section used to say the following, which is almost identical to a paragraph just above. SM changed this whole bullet to what is written just before this comment. "This guide to using GridLAB-D™ is intended to help those who are at least slightly familiar with distribution systems to establish a foundation that will allow them to use GridLAB-D™ in their work. It is not intended to be comprehensive as GridLAB-D™ contains many models with many parameters, but rather to address some of the more important and popular features. The guide will not only address practical issues such as how certain models function but also more general topics exploring the architecture of GridLAB-D™."**
- `2.0 - New Users\2.1 - Installation Guide.md` - l.83 - determine whether this should change or even be listed. The current default directory does not include these paths.**
- `2.0 - New Users\2.1 - Installation Guide.md` - l.99 - do these get set automatically when you select the add to path option in the executable? If not, describe what the user needs to do and what is done automatically. [JK- not sure. I don't see these set in my path, though I used the simple executable instructions. These instructions may change as well with the new release. Flag for revisit/review by Dev team]**
- `3.0 - Modeling Reference\Modules\1.0 - Introduction.md` - l.40 - Review list before next release. Add note here that says relase number it was last updated.


## Rewrite
- `2.0 - New Users\Tutorial\2.2.1 - Overview.md` - l.13 - The following is very unclear. What is the hierarchy? Based on my understanding, this is how I would write this. If this is correct, please replace the next 1.5 sentences accordingly: Within GridLAB-D™, users assign objects to different types of classes. A combination of one or many object classes make up a module. Modules are the aggregation of object classes... Nah, I don't even know what this is saying. What is nested in what? Could say, "objects such as a house or a building..." [addressed by jk 12/1]**


## Status
- `3.0 - Modeling Reference\Modules\Powerflow\1.0 - Power_Flow_User_Guide.md` - l.458 - This parameter is unused at this point. Future versions of GridLAB-D™ may implement this functionality**
- `3.0 - Modeling Reference\Modules\Powerflow\4.0 - Reliability_User_Guide.md` - l.288 - is this still true?


## Terminology
- `2.0 - New Users\Tutorial\2.2.1 - Overview.md` - l.5 - I added "of the software." Is this correct? [jk 12/1, yes]** of the software. The guide will address practical issues such as how certain models function, and will also cover more general topics within the architecture of GridLAB-D™.


## UNTAGGED
- `0.0 - GridLAB-D™\0.1 - Introduction.md` - l.26 - )** page.
- `0.0 - GridLAB-D™\0.2 - resources.md` - l.14 - **[Forum]**.
- `0.0 - GridLAB-D™\0.5 - Version History.md` - l.155 - *
- `2.0 - New Users\Tutorial\2.2.2 - Models.md` - l.268 - write your check code here
- `2.0 - New Users\Tutorial\2.2.3 - Running a Model.md` - l.11 - Test Ebony
- `2.0 - New Users\Tutorial\2.2.5 - Model Options.md` - l.35 - [Expansion variables]() for details.
- `2.0 - New Users\Tutorial\2.2.5 - Model Options.md` - l.45 - [Functional values]() for details.
- `2.0 - New Users\Tutorial\2.2.5 - Model Options.md` - l.58 - [Property calculations]() for details.
- `2.0 - New Users\Tutorial\2.2.5 - Model Options.md` - l.90 - :
- `2.0 - New Users\Tutorial\2.5.1 - Basic Distribution System Modeling.md` - l.75 - Is this true?)
- `2.0 - New Users\Tutorial\2.5.2 - GLM Models.md` - l.626 - Cite the right section.
- `2.0 - New Users\Tutorial\2.5.2 - GLM Models.md` - l.661 - add necessary properties so this actually loads ok
- `2.0 - New Users\Tutorial\2.5.2 - GLM Models.md` - l.714 - [Expansion variables]() for details.
- `2.0 - New Users\Tutorial\2.5.2 - GLM Models.md` - l.726 - [Functional values]() for details.
- `2.0 - New Users\Tutorial\2.5.2 - GLM Models.md` - l.739 - [Property calculations]() for details.
- `2.0 - New Users\Tutorial\2.5.2 - GLM Models.md` - l.770 - *
- `2.0 - New Users\Tutorial\2.5.2 - GLM Models.md` - l.826 - \
- `2.0 - New Users\Tutorial\2.5.2 - GLM Models.md` - l.829 - \
- `2.0 - New Users\Tutorial\2.5.2 - GLM Models.md` - l.832 - \
- `2.0 - New Users\Tutorial\2.5.2 - GLM Models.md` - l.835 - *
- `2.0 - New Users\Tutorial\2.5.2 - GLM Models.md` - l.890 - Why is this a mystery?
- `2.0 - New Users\Tutorial\2.5.3 - Modules.md` - l.103 - THIS IS A SPEC FILE) [waterheater](../../7.0%20References/Specs/Spec_Residential.md) model:
- `2.0 - New Users\Tutorial\2.5.3 - Modules.md` - l.315 - `.
- `2.0 - New Users\Tutorial\2.5.3 - Modules.md` - l.317 - ` add objects going all the way up to the feeder, including line, configuration, transformers, voltage regulators, fuses, switches, etc.
- `2.0 - New Users\Tutorial\2.5.3 - Modules.md` - l.321 - ` implement a simple dynamic-price demand response dispatch
- `2.0 - New Users\Tutorial\2.5.3 - Modules.md` - l.384 - ` implement a histogram
- `2.0 - New Users\Tutorial\2.5.3.1 - Clock Block.md` - l.2 - Add reference after we break 2.5.2 - GLM Models. A user can use the clock block to indicate the start and stop time of a simulation, as well as provide timezone information.
- `2.0 - New Users\Tutorial\2.5.6 - Distributed_Generation.md` - l.128 - Keep this mention of energy_storage? or update page?** The battery object is in a state of flux, containing some legacy models and some new models. In early versions of GridLAB-D™ (pre-v3.0), it was assumed that the model included both the battery and the inverter; the battery was connected directly to a meter (or triplex_meter) object. Post-v3.0, the inverter model has been separated from the battery object. In this case, the battery is connected as a child of an inverter object and the inverter is then connected to the meter object. This may cause some confusion - development of new models will focus on a full separation of the inverter and battery models, but legacy code still exists for those that are still using it.
- `2.0 - New Users\Tutorial\2.5.6 - Distributed_Generation.md` - l.136 - Can charging and discharging be controlled? It doesn't look like it based on the code**.)
- `3.0 - Modeling Reference\Modules\Climate\1.0 - Climate.md` - l.3 - - Update - Update for [Hassayampa (Version 3.0)]
- `3.0 - Modeling Reference\Modules\Climate\1.0 - Climate.md` - l.70 - - Incomplete - Climate (class) page is imcomplete
- `3.0 - Modeling Reference\Modules\Generators\Energy_storage.md` - l.157 - :
- `3.0 - Modeling Reference\Modules\Generators\Energy_storage.md` - l.161 - :
- `3.0 - Modeling Reference\Modules\Generators\Solar.md` - l.21 - Description | 69.8 [degF]
- `3.0 - Modeling Reference\Modules\Residential\ETP_closed_form_solution.md` - l.525 - :
- `3.0 - Modeling Reference\Modules\Residential\Waterheater.md` - l.165 - - Correction -  The two-node equations listed are incorrect, even though the repository code is correct. The latter should be parsed for the former. --[Mhauer] 20:11, 5 February 2009 (UTC)
- `3.0 - Modeling Reference\Modules\Residential\ZIPload.md` - l.84 - : Document cycling, demand response and aggregate modes.
- `3.0 - Modeling Reference\Modules\Residential\ZIPload.md` - l.208 - - Examples - Examples for cycling, demand response and aggregate modes.
- `3.0 - Modeling Reference\Modules\Tape\Player.md` - l.63 - The behavior of DST is not specified in subsecond mode, i.e., are timestamp in the localtime or standard time? (see ticket:563).
- `3.0 - Modeling Reference\Modules\Tape\Tape.md` - l.26 - - Empty - Define gnuplot_path
- `3.0 - Modeling Reference\Other Features\Checkpoints.md` - l.3 - - Incomplete - Add in content from related checkpoint files
- `3.0 - Modeling Reference\Other Features\Validate.md` - l.35 - - Redirect - pull in def for redirect). You can send all output to the console using --redirect none command option.
- `4.0 Developing Reference\4.7 - Release_Process.md` - l.2 - Update outline for release process section
- `4.0 Developing Reference\4.7 - Release_Process.md` - l.4 - Write content for process section
- `4.0 Developing Reference\4.2 - Building from Source\4.2.1 - Building_from_Source.md` - l.30 - - Update and verify pre-requisite installation command list for each build platform
- `4.0 Developing Reference\4.2 - Building from Source\4.2.3 - Setting-Up_WSL_for_Windows.md` - l.15 - Describe how to update the PATH variable for WSL
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.1 - gldcore.md` - l.11 - - link - link to appropriate page for more info.
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.1 - gldcore.md` - l.113 - \--[Dchassin] 00:07, 7 March 2009 (UTC)]
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.1 - gldcore.md` - l.117 - \--[Dchassin] 00:07, 7 March 2009 (UTC)]
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.1 - gldcore.md` - l.121 - \--[Dchassin] 00:07, 7 March 2009 (UTC)]
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.1 - gldcore.md` - l.133 - \--[Dchassin] 00:07, 7 March 2009 (UTC)]
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.2 - Modules.md` - l.57 - add gl_global_create() calls here (see module globals for details)
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.2 - Modules.md` - l.58 - call new for each class here (see create class for details)
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.2 - Modules.md` - l.59 - return oclass member of first new class
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.2 - Modules.md` - l.74 - perform cleanup actions if needed
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.2 - Modules.md` - l.92 - perform simulation end operations
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.2 - Modules.md` - l.107 - perform check operations and report issues
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.2 - Modules.md` - l.495 - - link - insert link for source doc on **set_callback** for details.
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.3 - Objects.md` - l.80 - Describe class members
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.5 - Time Management.md` - l.17 - Add link.
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.5 - Time Management.md` - l.99 - Add your deltamode_desired code */
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.5 - Time Management.md` - l.108 - Add preupdate code */
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.5 - Time Management.md` - l.120 - Add your interupdate code here */
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.5 - Time Management.md` - l.136 - Add your postupdate code here */
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.5 - Time Management.md` - l.162 - add your object update code here */
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
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.3 - Creating_a_class.md` - l.136 - :
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.3 - Creating_a_class.md` - l.140 - :
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.3 - Creating_a_class.md` - l.144 - :
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.3 - Creating_a_class.md` - l.148 - :
- `4.0 Developing Reference\4.4 - Development Fundamentals\4.4.3 - Creating_a_class.md` - l.152 - :
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
- `4.0 Developing Reference\4.5 - Debugging\Assert.md` - l.152 - - Empty - Double Assert section is
- `4.0 Developing Reference\4.5 - Debugging\Assert.md` - l.189 - - Empty - Add content for Enumeration Assert


## Update
- `0.0 - GridLAB-D™\0.1 - Introduction.md` - l.36 - This is basically the intro to modeling section now... This section should show the user, hey, go check out the next sections...**
- `2.0 - New Users\Tutorial\2.2.2 - Models.md` - l.553 - Talk about GLMModifier here instead**:
- `3.0 - Modeling Reference\Modules\Generators\Inverters\Spec_sync_ctrl.md` - l.127 - Review flowchart for accuracy and update
- `3.0 - Modeling Reference\Modules\Residential\Waterheater.md` - l.75 - Add multilayer model, with understanding that it is computationally expensive and not much confidence in its numerical stability.
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.2 - Modules.md` - l.152 - The `test` function is relatively unused and was intended to support module tests.
- `4.0 Developing Reference\4.3 - Software Architecture and Design\4.3.2 - Modules.md` - l.156 - The `stream` function will soon be required to support checkpoints.


## Update?
- `2.0 - New Users\Tutorial\2.2.1 - Overview.md` - l.7 - This previous link still works, but brings the user to a place that was last updated 9 years ago by Trevor *[jk 12/1--this the location of the required files to run the tutorial examples. I wouldn't expect them to be updated unless we add new ones. This doesn't preclude us from updating the language or descriptions in that section, but the .glms and inculde files should remain largely unchanged]***


## Version History
- `2.0 - New Users\Tutorial\2.2.1 - Overview.md` - l.36 - What version are we on now? V5.3.0? Suggest deleting (probably this entire paragraph). [jk-, let's move this to version history section and then add to it for full context]** With Version 1, GridLAB-D™ revealed the potential for a transformation in how complex energy systems are modeled, and garnered a great deal of interest from potential users around the world.  The availability of Version 2 has built on that interest and provides a much more appealing and flexible product with a wider potential range of users.  The open-source system works well on both proprietary and open-source operating system and is expected to perform strongly in the utility market.  GridLAB-D™ is set to transform how we model and study modern energy systems.


## What else?
- `2.0 - New Users\Tutorial\2.2.1 - Overview.md` - l.53 - Its next proposed steady what? Steady state? Or should steady be replaced with state? [jk, believe it should be steady state solution, Trevor confirm?]**


## Where are these?
- `0.0 - GridLAB-D™\0.1 - Introduction.md` - l.30 - Where will they exist?**


## tone?
- `2.0 - New Users\Tutorial\2.2.1 - Overview.md` - l.32 - JK- this next sentence feels a little outdated, I feel like we have lots of examples of this being a really great model of collaboration and shared discovery** Many vendors of established energy-related software tools still scoff at the idea that such a tool can make an impact, but the success of other large-scale open-source projects shows that this approach can and will work so long as enough support from contributors is available.


---

**Summary:** 163 TODO items found across 32 stages.
