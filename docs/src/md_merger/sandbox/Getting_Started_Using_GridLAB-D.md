# Getting Started

  This section introduces the basic workings of GridLAB-D™, from file types to
  how to create and run your first model.
---
## Introduction

GridLAB-D™ is a power system simulation tool that provides valuable information to users who design and operate electric power transmission and distribution systems, and to utilities that wish to take advantage of the latest smart grid technology. It incorporates advanced modeling techniques with high-performance algorithms to deliver the latest in end use load modeling technology integrated with three-phase unbalanced power flow, and retail market systems. Historically, the inability to effectively model and evaluate smart grid technologies has been a barrier to adoption; GridLAB-D™ is designed to address this problem.

This guide to using GridLAB-D™ is intended to help those who are at least slightly familiar with distribution systems to establish a foundation that will allow them to use GridLAB-D™ in their work. It is not intended to be comprehensive as GridLAB-D™ contains many models with many parameters, but rather to address some of the more important and popular features.  The guide will not only address practical issues such as how certain models function but also more general topics exploring the architecture of GridLAB-D™.

This guide contains and references many example GridLAB-D™ models and files.  Readers are recommended to pull a local copy of the folder at [https://github.com/gridlab-d/course/tree/master/Tutorial](https://github.com/gridlab-d/course/tree/master/Tutorial) so these files are readily available during the tutorial.

## GridLAB-D™ Design

GridLAB-D™ is a flexible agent-based simulator that can model the behavior of many objects over time.  The simulator looks for changes in objects that affect other objects and keeps track of the evolution of these objects over time.  GridLAB-D™ continues advancing the clock and allows each object to update itself until all the objects report that they are at equilibrium and the clock need not be advanced further.  This is very important to understand and is often one of the least understood aspect of GridLAB-D™.

GridLAB-D™ uses modules to define classes of objects.  Each class must be defined in a module.  Modules can either be static, meaning they are implemented in a dynamic link library (e.g., `.dll` in Windows, `.so` in Linux, `.dylib` on Macs), or they can be dynamic, meaning they are compiled and linked at runtime.  Classes define which properties are allowed in objects, and how behaviors are implemented.  Objects are instances of classes, so each object can have it's own values for each property while sharing behaviors with other objects of the same class. But during simulations GridLAB-D™will keep the objects' properties synchronized with each other as time advances.

## GridLAB-D™ Input Files

GridLAB-D™ uses two principle kinds of input files.  `.glm` files are used primarily to synthesize populations of objects and encode object behavior.  `.xml` files are used to represent instances of `.glm` files and exchange data with other software systems.  In general `.xml` files will work best with static modules that can be loaded from pre-existing libraries (.so files in Linux), while `.glm` files do not require static modules.  `.glm` files can also extend classes of objects implemented by modules, while `.xml` files cannot.

At this point, there is no released tool for editing `.glm` files, although the GldEditor is under development and can be worked on by those with access to source code.  Output `.xml` files are viewable using XSL/CSS stylesheets published on the GridLAB-D™website.

Please consult the \["/wiki/Creating\_GLM\_Files">Creating GLM Files] page for details on designing GridLAB-D™ models.  For a guide to the modeling using the static module, see the \["/wiki/Modeler%27s\_Guide">Modeler's Guide].

## Running GridLAB-D™

GridLAB-D™ can be run using the simple command line `gridlabd myfile`.  Command line arguments, including options are evaluated and executed in the order in which they appear.

Output is generated to `stdout` and `stderr`.  Output redirection is controlled using the `--redirect` command line option.

## GridLAB-D™ Output Files

GridLAB-D™ can output result one of two ways.  The first and simplest is to use the `-o myfile.xml` command line option to generate an instance of the model at the end of simulation.  For more information on `.xml` output, see the \["/w/index.php?title=XML\_Data\_Files\&action=edit\&redlink=1" >XML Data Files] page.

The second method of generating output is to use the *tape* module's *recorder* and *collector* objects to generate a time-series of particular values or aggregate values over the entire model.  For more information on this see \["/wiki/Tape\_Module\_Guide" >Tape Module Guide] page.

### The Primer

If you’re already competent with GridLAB-D™ and want skip straight to the details of runtime classes or high-performance simulation, then as an advanced user, you should feel free to jump straight the list of \["/wiki/Modules">modules].  However, all beginners and most intermediate users will find there are important concepts and details that you’ll need to get start and quickly become an advanced user.

GridLAB-D™ is the first (and so far only) environment for simulating the highly integrated modern energy systems that are coming into being all over the world.  GridLAB-D™ is also an open-source system, meaning that the source code, the programming code that makes up GridLAB-D™, is freely available to anyone.  People all over the world can add to GridLAB-D™, fix bugs, make improvements, or suggest optimizations.  And they do.  GridLAB-D™ has grown a lot since the prototype implementation (called PDSS, short for Power Distribution System Simulator) created at Pacific Northwest National Laboratory (PNNL) in 2002 \["#References">\[1]].  Back then, David Chassin and Ross Guttromson were commissioned under the Laboratory’s Energy Systems Transformation Initiative to look into a) whether such a software system could be built, b) whether it could model how energy systems might evolve over time, and c) how much value would this evolution bring to consumers and utilities.  In 2007, after the US Department of Energy's Office of Electricity committed to getting the results of that work more widely available, the open-source model of development and distribution was used to make sure that as many people as possible could both contribute to it and benefit from what it has to offer.  Since then, GridLAB-D™ has grown quickly, mainly because of the hard work and dedication of all the contributors, and of course the early dedication of the GridLAB-D™ team at PNNL.

Unlike proprietary simulation tools, where the source code is written by a few people and carefully guarded, open-source projects like GridLAB-D™ exclude no one who is interested in making a contribution if they are competent enough.  Many vendors of energy established software tools still scoff at the idea that such a tool can make an impact, but the success of other large-scale open-source projects shows that this approach can work and will work so long as enough support from contributors is available.

Another important advantage of the open-source model is the transparency of the implementation, which is necessary to building confidence in the accuracy of the results that come from using GridLAB-D™.  While proprietary tools must be carefully validated using test-cases, GridLAB-D™has the advantage that validation can begin much sooner by ensuring that the implementation meets industry standards for quality and accuracy.

With Version 1, GridLAB-D™revealed the potential for a transformation in how complex energy systems are modeled, and garnered a great deal of interest from potential users around the world.  The availability of Version 2 has built on that interest and provides a much more appealing and flexible product with a wider potential range of users.  The open-source system works well on both proprietary and open-source operating system and is expected to perform strongly in the utility market.  GridLAB-D™is set to transform how we model and study modern energy systems.

In this primer we will discuss the following:

* Essential concepts and terminology

* Starting and stopping GridLAB-D

* Creating and running models

* Creating and modifying classes

* Instantiating objects

* Extracting results

* Constructing complex models

Throughout these pages certain conventions are observed to assist readers in gaining access to a maximum amount of information with a minimum of effort.

* Unavailable links are marked in red.  These indicate that further information is needed, but it has not yet been written.  As with many MediaWiki documentation efforts, these pages are always a work in progress.  Red links indicate the leading edge of that work.

*Synopsis*

* Most pages have a leading synopsis section that briefly describes the usage of the command or concept in question.  Often the synopsis will contain blue links to other portions of the page that describe that aspect of the topic in more detail.

* Sample code is usually provided with descriptions of features.  Often these examples build on previous examples. In such cases, new lines are underlined, unwanted lines are stricken, and only the new concepts and terms are marked with \["/wiki/Gridlabd">blue links].

*See Also*

* Most pages have a "See also" section at the bottom that provide you with suggested topics that are related to the page.  These are often grouped in topic areas of many related pages.

## Understanding GridLAB-D™ Basics

GridLAB-D™ is an agent-based simulation environment specifically designed to model modern energy systems.  It’s a program capable of tracking the simultaneous states of vast numbers of objects having a wide variety of properties and behaviors, and gathering information about their condition as time progresses in the simulation.

GridLAB-D™ simulates a system by computing a series of steady states, separated by state transitions.  The simulation discovers when those state transitions occur by looking at each object to determine a) what its next proposed steady is, b) whether that state consistent with the proposed steady states of all the other objects in the model, and c) how long that state is expected to last.

The details of how GridLAB-D™ works are left to a later discussion.  But GridLAB-D™ comes with everything needed to make this happen, and determine what the final result is.  All the tools needed for constructing and compiling models, debugging and running them in the simulator, and extracting results are either installed with GridLAB-D™ or available as free downloads from open-source repositories.

At this point, this is all you need to know to get right into creating a simple model and running a simulation. Jump on over to [Modeling 101](docs:intro-to-modeling) to start modeling in GridLAB-D™.

