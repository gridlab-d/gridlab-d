# Technical Overview
**TODO - FT - Holistic look at this file to ensure relevane and accuracy**
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
  * Fuses
  * Switches
  * Shunt capacitor banks

# Thermal Modeling - Buildings

Commercial and residential buildings are implemented using the Equivalent Thermal Parameters model. These are differential models solved for both time as a function of state and state as a function of time. Currently implemented residential end uses include: 

  * Water heaters
  * Refrigerators
  * Stand-alone freezers
  * Dishwashers
  * Clothes washers and dryers
  * Electric ranges
  * Microwaves
  * Electric plugs and lights
  * Internal gains
  * House loads (including air conditioning, heat pumps, and solar loads)

**TODO - Relevance - Are we still developing more detailed end use behavior as we march forward in time?** Commercial loads are simulated using an aggregate multi-zone Energy Technology Perspectives (ETP) model that will be enhanced with more detailed end use behavior in coming versions. 


# Economic Behaviors
**TODO - Keep? - If GLD no longer has markets module, this will probably go away also.** Today's power system simulation tools do not provide the analysis capabilities needed to study the forces driving change in the energy industry. The combined influence of fast-changing information technology, novel and cost-effective distributed energy resources, multiple and overlapping energy markets, and new business strategies result in very high uncertainty about the success of these important innovations. Some concerns expressed by utility engineers, regulators, various stakeholders, and consumers can be addressed by GridLAB-D™. Some example uses include:

* Data Collection and Analysis - Rate Structures.
* Physical and Economic Boundary Conditions - Distributed Resources.
* Peak Load Management.
* Distribution Automation Design.

## Data Collection and Analysis - Rate Structures

Multiple differentiated energy products based on new rate structure offerings to consumers is very attractive to utilities because it creates the opportunity to reveal demand elasticity and gives utilities the ability to balance supplier market power in the wholesale markets. The challenge is designing rate structures that are both profitable and attractive to consumers. **TODO - Relevance - Do we provide the following functionality?** GridLAB-D™ will provide the ability to model consumer choice behavior in response to multiple rate offerings (including fixed rates, demand rates, time-of-day rates, and real-time rates) to determine whether a suite of rate offerings is likely to succeed. 

## Physical and Economic Boundary Conditions - Distributed Resources

The advent of new distributed energy resource (DER) technologies, such as on-site distributed generation, **TODO - Define - Is this "Building Combined Heat and Power"? If so we should define it.** BCHP, and Grid-Friendly™ appliance controls creates a number of technology opportunities and challenges. GridLAB-D™ will permit utility managers to better evaluate the cost/benefit trade-off between infrastructure expansion investments and distributed resources investments by including the other economic benefits of DER (e.g., increase wholesale purchasing elasticity, improved reliability metrics, ancillary services products to sell in wholesale markets). 

## Peak Load Management

Many peak-shaving programs and emergency curtailment programs have failed to deliver their expected benefits. GridLAB-D™ can be calibrated to observe consumer behavior to understand its interaction with various peak shaving strategies. **TODO - Rephrase? - Following sentence is slightly off. Maybe just "available" needs to be changed to "availability"? But this sentence still reads a little odd.** The impact of consumer satisfaction on the available of peak-shaving resources can be evaluated, and a more accurate forecast of the true available resources can be determined. **TODO - Relevant? - The following sentence - Does it do this? Will it? Should it?** GridLAB-D™ will even be able to evaluate the consumer rebound effect following one or more curtailment or load-shed events in a single day. 

## Distribution Automation Design

GridLAB-D™ can support some aspects of the design and analysis of distribution automation technology, allowing utilities to offer heterogeneous reliability within the same system but managing power closer to the point of use. 

# Integration with Other Software
**TODO - Empty - Empty section? Remove or write**