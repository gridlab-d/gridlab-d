# GridLAB-D™ Documentation Outline

The following is a conversion of the outline originally written by Trevor Hardy and serves as a proposed table of contents for the final documentation. Feel free to edit and add details as needed.

## Home
Splash or Landing Page (Not gridlabd.org)
 A few paragraphs and/or a few-minute introductory video describing what GLD is and what people use it for. Lots of pictures, easy to digest, accessible to as broad an audience as possible while answering the question “Is GLD useful for me? Should I keep reading?” 

 - What is GLD?
 - Who should use GLD?
 - How do I cite GLD?

## 0.0 GridLAB-D
- **Contact**
- **Version History**
- **Projects**
- **Publications**
- **Resources & Support**
  - Email address 
  - Github discussions
  - Chat-with-GLD-experts server (Discord, Gittr) 
  - Link to example suite
  - Link to GLD autotests 
  - Link to source code repo
- **Style Guide**

## 1.0 Prospective Users
- **Key Attributes of GLD**
- ***Technical Overview*** -
A few pages, describing what GLD can do in more detail. This section is for those that are pretty sure GLD can do something they want and they want to make sure. Still lots of graphics but meant to be browsed and read in 15 minutes or less. Probably each of the sections below has one or more links to dedicated content elsewhere if the reader wants to know more.

 *Consider this as an executive summary of our documentation work, revisit when the bulk of the docs/our narrative plan are complete*

  - How GLD models the power system and how that’s expressed in a .glm 
  - Object definition through parameters 
  - Object synchronization through time 
  - Quasi-static time series (not true dynamics)
  - Commonly Used Functionality -
Showing off the popular integrated models and the interaction between the devices that makes GLD a great platform for doing this kind of modeling.
  - Powerflow
  - Single-zone Structure Model (House)
  - Smart Grid Devices
    - Single-zone structure with HVAC (nominally residential home) 
    - Rooftop solar 
    - Battery 
    - EV 
  - Three-Phase Unbalanced Transients (Deltamode)
  - Co-Simulation with HELICS
  - libgld - Using GLD as an engine in a larger application. Make your own GUI, make a purpose-built script that runs your analysis, use other models as part of the GLD simulation.

## New Users

- **Getting Started**
- ***Installation*** - This page needs work/creation
  - installing and validating pre-built binaries, 
  - exe
  - building from source
- **Theory of Operation** - most of this page seems like dev theory, might make sense to move most of if there and the first few sections to *Prospective Users/Technical Overview*
- ***Running existing example*** - this page needs to be created. Something very simple that serves as a test of the installation and produces some plots to look at for verification.

## 3.0 Modeler Guide
- Who is a “modeler” and what does this guide intends to provide?
    - By the time folks have gotten to this section, we assume they have reviewed the installation instructions and the key attributes of GLD (Chapters 1-2 in original wiki tutorial)
  - Where does transient mode fit? Where is it first introduced, where is the bulk of the content?
    - consult with dev team about role of transient mode

### Intro to Modeling
 - 3.1.1 Basic Distribution System Modeling
 - 3.1.2 GLM Models
    - GLD model file formats
 - 3.1.3 Modules
    - Class hierarchy and property inheretance
    - Tape
    - Clock
    - Message
    - Market
    - Verifying of Debugging a Model
      - What are the red flags?
      - Common warnings that can be avoided
      - If we make validation scripts, how to use them
 - 3.1.4 Recorders and Players
 - 3.1.5 Schedules and Loadshapes
 - 3.1.6 Distributed Generation
    - Device models - One page for each model listing all properties and what they do. Often there are multiple properties that are related so we might want to create sub-sections around functionality and describe the related parameters there.
    - Non-Device models
 - 3.1.7 Advanced Distribution Models
 - 3.1.8 Built-In Distribution Models

### Modules
- Climate
- Commercial
- Connection
- Market
- mysql
- Network
- Objects (doesn't really fit)
- Powerflow
- Residential
- *Assert*
- *Module Functions*
- *Module Globals*
- *Tape*

### Metrics & Recorders

### Loads


### Modeling Reference
- Debugging and Validation
- transient mode and Timing

## 4.0 Developer 
Who is a “developer” and what this guide intends to provide 

### 4.1 Introduction
### 4.2 Building from Source
### 4.3 Software Architecture & Design
### 4.4 Developer Fundamentals
### 4.5 Deltamode Development
- GLD object synchronization process
- UML sequence diagram?
### 4.6 Debugging

### Other
- Parallelization implementation 
- Loader 
- Device Model Structure
- libgld
- Contributing
- Running and Adding tests
- Submitting a PR

#### 4.7 Style Guide

#### Doxygen API documentation (largely auto-generated) 
Does this make a class hierarchy diagram automatically or do we need to add one in separately?


## 5.0 Integrator Guide

Who is an “integrator” and what this guide intends to provide 

- Overview of libgld
- Common APIs
- Loading and running a model, pausing execution
- Setting a property value 
- Getting a value out from the model (APIs to make your own Tape module)
- Common Applications of libgld
    - Just links to things in "6.0 Examples"?
- Doxygen API reference?
    - Somewhere we need to have a comprehensive list of the APIs Integrators can use 



## 6.0 Examples

Collection of examples each with their own documentation page and supporting files (.glms, weather, etc). Ideally these are tested as part of some kind of CI/CD thing so we can know they always work. Ideally we have one dedicated example for each feature.