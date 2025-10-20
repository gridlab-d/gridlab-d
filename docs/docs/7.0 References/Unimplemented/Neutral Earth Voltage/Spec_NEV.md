# Introduction

!!! warning

	This page contains features that are unfinished, were never implemented, or have since been deprecated. We preserve these pages for archival purposes, and also as a foundational resource for prospective developers who may wish to implement the same or similar feature. Many of these pages provide robust explanations of the theory behind a particular module or feature that we hope readers will find useful. 
	**This page does not reflect the current state of GridLAB-D™**

The Neutral-Earth Voltage implementation of GridLAB-D™ will require many changes to the underlying [powerflow] source code. These specifications outline the expected implementation and hope to resolve any conflicts prior to actual coding. 

# Terminology

To ensure all specifications are compatible and discussions are consistent, the following terms will be utilized for NEV-related discussions and specifications. 

Table 1 - NEV Terminology  Term | Definition   
---|---  
Bus | Connection point of link-based objects, representing a point of voltage potential. Interchangeable with node.   
Connection | A specific wire or relationship between two phases of distinct buses/nodes/terminals. e.g., the wire representing phase A between two nodes.   
Link | Connection between two distinct buses or nodes. A physical, specific connection implementation (e.g., a 3-phase line). Can be composed of multiple phases, so long as it is consistent between the from bus/node and to bus/node (i.e., link can't change form between two buses/nodes).   
Load | An electrical path between two distinct phases/terminals on the same bus that consumes/produces some form of power. Under old [powerflow] implementations, this was a current, impedance, or power shunt between a phase and an implied zero-potential reference. A line-to-ground fault could be implemented as a low-impedance `load` connection between a terminal and the ground plane.   
Node | Connection point of link-based objects, representing a point of voltage potential(s). Interchangeable with bus.   
Phase | An individual voltage potential specification on a node or bus. Phase "A" is a very specific implementation, but this will be more generalized for the NEV-related implementation. This will be numeric based for new implementations (e.g., ABC could be 1,2,3). Phase points are defined as terminal connections within the NEV solver framework.   
Terminal | A generalized, individual voltage potential specification on a node or bus. Synonymous with "Phase" under the ABC convention, this will be referenced by number instead. 64 unique phase "specifications" will be supported for flexibility. e.g., phase A could be phase 1, phase B could be phase 2, etc.   
  
# Sections

The following pages will contain specifications for individual aspects of the NEV solver implementation. Note that these pages are not stand-alone and a significant amount of overlap is expected. Please be sure to review all specifications pages. 

[NEV Data Formatting](./Spec_NEVDataFormat.md)

[NEV Link Objects](./Spec_NEVLink.md)

[NEV Node Objects](./Spec_NEVNode.md)

[NEV Solver Implementation](./Spec_NEVSolver.md)

[Other Module Interactions](./Spec_NEVModules.md)

# See also

  * [Overview Page]
  * [Requirements]
  * [Implementation]
  * [Keeler (Version 4.0)]

