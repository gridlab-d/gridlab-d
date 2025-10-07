# Key Attributes of GridLAB-D™
Depending on your background with software development, familiarity with other distribution system simulators, and the specific studies being done GridLAB-D™ may come across as cryptic, flexible, complicated, overwhelming, and/or limited. To help provide context and explain what makes GridLAB-D™ unique and the kinds of problems it can be helpful in addressing, the remainder of this chapter addresses some of the key attributes of GridLAB-D™.

## Open Source

GridLAB-D™ is open source software whose development has been largely funded through the [United States Department of Energy](http://energy.gov). Two distinct advantages come with being open source:

* Code is readily available for inspection. This is helpful for those trying to learn GridLAB-D™ as it allows access to all the inner workings of the software. There is no need for mystery about what the software is actually doing as all mechanisms are laid bare.

* Correction or expansion of the code base to meet the user’s particular need is possible. If a bug is found, any user can correct it. If additional functionality not present in the standard distribution of the software is needed, users can modify existing code or augment it with their own to provide that functionality.

## Command-Line

GridLAB-D™ is a pure command-line program; there is no GUI at the time of this writing. The input files that GridLAB-D™ uses when running a simulation are simple text files as are the most common output files. To start a simulation, users will type commands into a system terminal or command line where the on-going status of the simulation will also be displayed.

While this does increase the learning curve for many users as defining systems via text files and typing commands at a text-based command prompt increases the level of abstraction, text-based simulations have advantages.

* Portability -  With all inputs and outputs as text files, viewing and graphing results from a simulation is easily accomplished through any number of tools such as gnuplot or Microsoft Excel.

* Scalability -  Using a GUI to directly define small systems is very convenient and intuitive but defining large systems by hand is tedious and imprecise. Defining a multi-thousand node distribution system with text manipulation and scripting tools such as Matlab, Python, Perl, or even grep allows models to be created and modified in a much more efficient manner.

## Object-Based

GridLAB-D™ can be thought of as a core simulator with a collection of modules that contain model implementations for various entities that are relevant for distribution system simulations. Said differently, the GridLAB-D™ core is responsible for managing the flow of the time and interactions between a number of objects that have been assembled to simulate the effects of a distribution system as a whole.

Because of the modular nature of GridLAB-D™, there are a few terms that should be clarified as they will frequently come up during discussion of the design and use of GridLAB-D™.

* “Model” - An engineering or mathematical term (rather than programming). There are two somewhat related ways in which this term is used in the GridLAB-D™ world,

* A general term to describe how a particular part of GridLAB-D™ functions or is represented in code. For example, "How does GridLAB-D™ model solar panels?"

* A "model file" (commonly ending in ".glm") containing the description of the distribution system being studied. This file contains the specific statement used to model a distribution system as a whole. For example, "Where is the model you used for that assignment?"

* "Class" - A C++ programing term with a similar usage as "model". Much of GridLAB-D™ is written in C++ and sometimes GridLAB-D™ developers and programmers will use this term somewhat interchangeably with "model", particularly after a long day of programming and trouble-shooting. To be specific, a "class" is the collection of code that contains the equations, parameter declarations, and algorithms that define how a particular entity will behave in GridLAB-D™.

For example, in GridLAB-D™ there are classes that define the operation of capacitors, air-conditioners, voltage regulators, houses, and solar panels, among others. In the air-conditioner class, there is code that defines, for example, how much current air-conditioners draw when operating, and how much cold air they put out; these types of definitions are essential to create some kind of model of an air-conditioner that interacts with the grid and buildings realistically. Also note that in a class, these kinds of model/class parameters are generic and have no specific values associated with them (though default values may be defined). The code in the class definition describes how the current from the grid relates to the amount of cold air produced but does not necessarily define any specific amount of current drawn for all air-conditioners ever created in GridLAB-D™. The specific current draw will get defined when all the class parameters are given specific values as a part of a specific instance of the air-conditioner class being created in a model file (".glm").

* "Object" - Another C++ programming term used in a very similar way to that language. In C++, an object is a specific instance of a class. That is, an object is one particular, air-conditioner, house, or solar panel that may operate slightly differently than another one next door or down the street. The inner workings of all air-conditioners are defined by the air-conditioner class but their size, efficiency, and location can be different from one particular air-conditioner to another. All air-conditioners function in the same way as defined by code for their class (e.g. turning electrical energy into cold air) but the specific values assigned to the parameters for each object will produce unique operational patterns.

* “Module” - A collection of related classes typically defined in the same source code file. Modules will often need to be explicitly included in a model file to give GridLAB-D™ access to their functionality.

When looking at a GridLAB-D™ model file (“.glm”), almost all the text in the file is devoted to defining objects. All of these objects are specific instances of classes (models) and their parameter definitions and relationships between each other constitute the system as a whole that is being modeled.

