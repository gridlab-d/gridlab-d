/** $Id: user_manual.h 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute

@file user_manual.h

@page credits Credits

*****************************************************************
*****************************************************************
\section credits1_0 Version 1.0 (Allston)

\subsection credits1_0mgt Management
- DOE/OE Program Manager: Eric Lightner (US DOE)
- GridWise Program Manager: Rob Pratt (PNNL)
- GridLAB Project Manager: Linda Connell (PNNL)
- GridLAB System Architect: David Chassin (PNNL)

\subsection credits1_0core GridLAB Core 
- Manager: David Chassin (PNNL)
- Design development: David Chassin (PNNL), Shawn Walton (PNNL), Matt Hauer (PNNL), Brandon Carpenter (PNNL)
- Unit testing: Nate Tenney (PNNL)

\subsection credits1_0dist Distribution network modeling
- Manager: Kevin Schneider (PNNL)
- Design development: Clint Gerkensmeyer (PNNL), Brandon Carpenter (PNNL)
- Unit testing: Nate Tenney (PNNL), Bo Yang (PNNL)

\subsection credits1_0res Residential building modeling
- Manager: Krishnan Gowri (PNNL)
- Design development: Srinivas Katipamula (PNNL), Todd Taylor (PNNL), Ning Lu (PNNL)
- Unit testing: Nate Tenney (PNNL), Krishnan Gowri (PNNL)

\subsection credits1_0com Commercial building modeling
- Manager: David Chassin (PNNL)
- Design development: Alex Zheng (Horizon Energy)

\subsection credits1_0trans Transmission network modeling
- Manager: David Chassin (PNNL)
- Design development: David Chassin (PNNL)

\subsection credits1_0prod Production
- Manager: Teresa Carlon (PNNL)
- Version control: Brandon Carpenter (PNNL), Nate Tenney (PNNL)
- Bug tracking: Nate Tenney (PNNL)
- Autobuild: Brandon Carpenter (PNNL), Nate Tenney (PNNL)
- Documentation: Teresa Carlon (PNNL), David Chassin (PNNL)


*****************************************************************
*****************************************************************
*****************************************************************
@page user_manual User's Manual

*****************************************************************
*****************************************************************
\section user1_0 1.0 About GridLAB-D

\version 1.0 \e Allston, release in January 2008 includes the main GridLAB core,
a powerflow module based on Kirsten's forward-backsweep method, and a residential
module based on the equivalent thermal parameters (ETP) method.  This is a limited
release for the partners that will help produce the Version 1.1 release.

\version 1.1 \e Buckley, release April 2008, includes java module link and additional
objects in existing modules.

*****************************************************************
*****************************************************************
\section user2_0 2.0 Installation

*****************************************************************
\subsection user2_1 2.1 Windows installation
@todo Describe Windows installation method. (doxygen, high priority) (ticket #153)

*****************************************************************
\subsection user2_2 2.2 Linux installation

To build the Linux version, download the source code to a folder of your choosing 
and type \p make \p install.  This will install GridLAB-D in the \p /usr/local folder
and make it available for use by all users.

*****************************************************************
*****************************************************************
\section user3_0 3.0 Running GridLAB-D

GridLAB-D is run from the command-line using the following syntax:
	\verbatim
	gridlabd.exe options filename ...
	\endverbatim
The valid \p options are listed is the \ref cmdarg "Command-line Arguments". 
A list of parameters can also be displayed by using the \p –-help command-line parameter.

The \p filename parameter is the name of one or more model definition input files to load. 
By convention, model definition input files are given the extension .glm (see \ref user4_0 "Model Definition").

 
*****************************************************************
*****************************************************************
\section user4_0 4.0 Model Definition

This section describes the modeling language used by GridLAB-D to define power and load models.  
The model definition is stored in a file with an extension of \p .glm. There are three main declaration 
blocks that may be defined in a model definition file:
- \p clock
- \p module
- \p object

Of these, only \p module and \p object declarations are required.  Additionally a \p class block may be provided to enforce modeling consistency.

Comments within the model file are introduced using a hash (#) character. All text between a hash mark and the end of a line is ignored.

A number of sample model definition files are included with the GridLAB-D distribution.

*****************************************************************
\subsection user4_1 4.1 Clock definition

The clock is used to specify the simulation start time and the clock resolution. The clock declaration typically looks like this:
\verbatim
clock {
	tick double;
	timestamp [int64[s|m|h|d] | 'timestamp'];
	timezone std[+|-]hh[:mm[:ss]][dst];
}
\endverbatim
- \p tick - The tick property specifies the time resolution of the simulation engine in seconds.  
The clock tick is set to 1 second by default. It may be any value for which the log10 is a 
negative integer between 0 and 9, e.g., 1, 1e-1, 1e-2, ..., 1e-9.  A value of 1 second is 
recommended for early releases of GridLAB-D.
- \p timestamp - The timestamp property specifies the start of the simulation.  This number is specified in one of two formats:
	- as a 64-bit integer representing seconds, minutes, hours, or days since January 1, 1970, 00:00:00 GMT. 
	Both \p int64 and \p int64s assume seconds, \p int64m assumes minutes, \p int64h assumes hours, \p and int64d assumes days.
	- in the form of a \p timestamp, e.g., \p 'yyyy-mm-dd hh:mm:ss'. This form must be enclosed in single quotes (apostrophes).
- \p timezone - The timezone property is used to specify which timezone rules are in effect and whether daylight saving 
time rules are to be applied. The syntax of the timezone variable must be Posix compliant, as follows:
\verbatim
std[+|-]hh[:mm[:ss]][dst] 
\endverbatim
and must be a valid time-zone name listed in the \p tzinfo.txt file. The currently defined timezones are:
\include tzinfo.txt

If unspecified, the clock is initialized to zero, which starts the clock on Jan 1, 1970 GMT.

Multiple clock blocks may be specified.  The effect of each clock is to set the initial clock value of any object subsequently 
defined to the timestamp given.  This can be used to create an initially unsynchronized model and should be used with great caution.  

*****************************************************************
\subsection user4_2 4.2 Module block

The modules block loads and configures modules that will be used in the simulation and allows 
exposed properties of those modules to be set. The currently supported GridLAB-D modules include:
- powerflow - Implements the distribution network flow solver.
- residential - Implements the residential loads models.
- climate - Implements the TMY weather data.
- tape - Implements the boundary condition, load shaper, and data collection.

Additional modules may be added. The module names correspond to .dll names (Windows) or .so (Linux) that are distributed with GridLAB-D.

A module declaration typically looks like this:
\verbatim
module module_name {
	property value;
}
\endverbatim

Where \p property is a variable supported by that module and \p value is the value to which the property should be initialized at 
the start of the simulation. Property values must be one of the defined formats outlined in \ref user4_4 "Section 4.4, Properties". 

Modules can be declared without initializing any properties by omitting the bracketed section above and adding a semi-colon after the module name. 
The following example shows what a modules section might look like:
\verbatim
# modules
module gauss_method {
	acceleration_factor 1.4;
	convergence_limit 0.001;
}
module tape;
module climate;
module commercial;
\endverbatim

*****************************************************************
\subsection user4_3 4.3 Objects

A model consists of instantiations of objects that are supported by the modules listed. 
For example, the power flow model might define as a series of node, link, capbank, fuse 
and regulator objects. The residential model may be defined as a series of residences, 
refrigerators, and water heaters. This section describes the format of the object 
definitions. See Sections __ through __ for specific objects and their properties 
supported by each module.

@todo provide detailed object property sections for users. (doxygen, high priority) (ticket #154)

An object definition is required for each object created. An object declaration might look like this:
\verbatim
object <class>:<int> {
	[root | parent [<class>:<int> | <object.name>];
	clock [<int64>[s|m|h|d] | '<timestamp>'];
	latitute <latitude>;
	longitude <longitude>;
	in '<timestamp>';
	out '<timestamp>';
	name <string>;
	...
}
\endverbatim

\<class>:\<int> refers to an object by specifying a class supported by one of the modules included in the modules section of the module definition file.  
The \<int> id is a positive 32-bit integer that must be unique for the entire model, but not necessarily sequential. 
The object id can be specified as a range, such as \p 12..18 to construct multiple objects in a series.  
If the first number is left out, then the second number is used as a count of how many objects are created. 
For example, an id of \p ..19 will construct 19 separate objects.

Properties for different objects are described in subsequent sections. However, some properties are attributes of all objects. 
These attribute properties include:
- root | parent - The parent property is very important as it establishes the order in which objects are synchronized--parents are 
synchronized before their children. It also controls the order of pre- and post-synchronization steps. 
The parent is specified using an object class, followed by a semi-colon, and the named object's id number, or by refering to the object's name. 
The parent specification may reference an object not yet defined, but all parent object references must be resolved somewhere 
within the module definition file (unresolved parent references will cause an error). The root property can be specified for 
any object that has no parent object. For objects with no specified parent, root will be assumed by default.

The parent property can be specified as \<class>:*, which has the effect of using the first as-yet childless object with that name.  
This is typically used in conjunction with multiple object ids (see above). For example, the following declaration 
will create one node object, 10 office objects, and 10 recorder objects. The office objects will all have the same parent 
(node:1) but the recorder objects will each have a different parent (office:1 through office:10).  Once the office objects 
are assigned as parents to the recorder objects, the office:* nomenclature cannot be used to assign those same 
office objects as parents to other objects (i.e. they are no longer unreferenced).
\verbatim
object node:1 {
	type 3;
	name "Feeder";
}

object office:1..10 {
	parent Feeder;
}
object recorder:..10 {
	parent office:*;
}
\endverbatim

- \p latitude / \p longitude - The latitude and longitude are optional properties used to enable GIS systems to map 
the objects on a map.  In the absence of a specification for these properties, no mapping of the object will be supported.
- \p in_svc / \p out_svc - The \p in_svc and \p out_svc properties define the dates and times at which object go into and come 
out of service.  Objects will only be synchronized between these two dates.  Omitting the \p in_svc property will 
cause the object to be synchronized starting with the initial simulation clock.  Omitting the \p out_svc property 
will cause the object to be synchronized until the end of the simulation.
- \p name - The name provides a canonical name that can be used to refer to the object without using the \<class>:\<id>
specification.  This is particularly important when linking multiple models together from diverse files.  However,
a name must be unique over the entire simulation.

Additional properties that are supported by each object can be set. For the specific properties 
that are supported by an object, see Section ___ or consult the documentation for that module.

@todo provide a link to the documentation for the properties of each module (doxygen, high priority) (ticket #155)

*****************************************************************
\subsection user4_4 4.4 Properties

This section describes the properties supported by the version 1.0 GridLAB-D modules and their corresponding objects. 
Property values can be specified as a number, a string, a date and time, or a functional.  The value set must match the property type as defined
by the module: 

- Integers can be specified only as an optional sign followed by a series of digits.  If the integer is too big, 
it will be truncated to fit the memory allotted for the variable on the given system. 
Integers are defined as being one of the following:
	- \p int16 - a 16-bit integer
	- \p int32 - a 32-bit integer
	- \p int64 - a 64-bit integer

-	String values should be enclosed in double quotes. String are defined as being one of the following:
	- \p char8 - a string with at most 8 characters
	- \p char32 - a string with at most 32 characters
	- \p char256 - a string with at most 256 characters
	- \p char1024 - a string with at most 1024 characters

- \p double precision floating point numbers. They can be specified as a decimal number containing a decimal point, 
  optionally preceded by a + or - sign and optionally followed by the e or E character and a decimal number. Floating
  point values may be provided with units, provided the units given are compatible with the units specified for the
  property.  The following describe 71,200 square feet:
	\verbatim
	71200.0 ft^2
	7.12e4 sf
	6740.8 m^2
	\endverbatim

- \p complex numbers take the form of two decimal numbers with a + or - sign between them and followed by the 
imaginary flag desired, either \p i, \p j, or \p d, to indicate rectangular or polar coordinates, as appropriate. 
Valid examples include:
	\verbatim
	12.3+4.56j
	12.3-4.56i
	12.3+4.56d
	\endverbatim
Complex numbers can also have units, but the unit manager does not handle phase at this point.  Therefore the
distinction between \p kW, \p kVA, and \p kVAR is lost when units are converted.  This is a shortcoming that will be remedied in
the near future, so we recommend care be taken in specifying units correctly, e.g., \p kW for the real component of power,
\p kVA for the magnitude of power, and \p kVAR for the imaginary component of power.

- Timestamps are strings entered in the format \p 'yyyy-mm-dd hh:mm:ss \p tz'. They must be enclosed in single quotes (apostrophes). 
  A valid example is: 
	\verbatim
	'2007-01-15 00:00:00 PST'
	\endverbatim
  Timestamps can also be given in the form \<number>\<unit>, when the \<unit> can be \p s for seconds, \p m for minutes, \p h for hours
  or \p d for days, as figured from 1/1/1970 0:00:00 GMT.

- \p enumeration and \p set values are stored as integers corresponding to keyword values defined by a given module. 
  Using numbers requires prior knowledge of how the module implements the property and what values the property supports and is highly
  discouraged.  An enumeration stored alternate values (only one can be stored at a time), whereas a set stores concurrent values (several
  can be stored at a time).  Keywords sets may be combined using the | operator.  However, if all the keywords are single characters, the
  | may be omitted.

  When an enumeration or set is defined, the module that defines it must specify what the keywords are and what internal numbers they correspond to.

  An illustrative example of an enumeration is:
	\code
	typedef enum {ITEM0=0, ITEM1=1, ITEM2=2} ITEMS;
	enumeration item;
	gl_publish(oclass,
		PT_enumeration, "item", PADDR(item),
			PT_KEYWORD,"ITEM0", ITEM0,
			PT_KEYWORD,"ITEM1", ITEM1,
			PT_KEYWORD,"ITEM2", ITEM2,
		NULL);
	\endcode

  An example of a set is:
	\code
	#define NO_OPTIONS	0x00
	#define OPTION_1	0x01
	#define OPTION_2	0x02
	#define OPTION_3	0x04
	#define ALL_OPTIONS	0x07
	set options;
	gl_publish(oclass,
		PT_enumeration, "options", PADDR(options),
			// keywords should be published in order of decreasing complexity
			PT_KEYWORD,"ALL_OPTIONS", ALL_OPTIONS,
			PT_KEYWORD,"OPTION_1", OPTION_1,
			PT_KEYWORD,"OPTION_2", OPTION_2,
			PT_KEYWORD,"OPTION_3", OPTION_3,
			PT_KEYWORD,"NO_OPTIONS", NO_OPTIONS,
		NULL);
	\endcode
  
- \p object is used to refer to another object.  The name given must be either specified as \<class>:\<int> or the object's canonical name.

- \p functional values may be used for floating point properties only, and not for integer or complex numbers. 
The only functionals defined at this time are the random number functions, which supports a variety of probability distributions, including
	- \p random.uniform(a,b) - returns a uniformly distributed random number between a and b.
	- \p random.normal(m,s) - returns a normally distributed random number with mean m and standard deviation s.
	- \p random.lognormal(m,s) - returns a number whose log is normally distributed about the geometric mean m and geometric standard deviation s.
  For example, to generate a normally distributed value with a mean of 10 and a standard deviation of 3, the functional random.normal(10,3) 
 would be used.  This is particularly useful in conjunction with ranged ids, in which case the ids will be guaranteed to be unique.

See the section on \ref random "Random numbers" for more information.

*****************************************************************
*****************************************************************
\section user5_0 5.0 Distribution Network Module

This section describes the objects and properties supported by the \p powerflow module. The \p powerflow module is the basic unbalanced 
3-phase flow modeling and simulation module for GridLAB-D.  It defines two generic objects, the link and the node objects.  
Together these two object types are necessary to create power flow networks.  It also supports the following more specific objects 
types used in power systems.  Among the node objects are:
- \p capacitor - defines a capacitor
- \p meter - defines a load meter (both single and poly-phase)
Among the link objects are:
- \p _switch - defines a manual switch component
- \p relay - defines an automated relay component
- \p fuse - defines a fuse component
- \p transformer - 

A network can only be defined using only objects derived from link and node objects. A network can be defined using the 
abstract link and node objects.  However, such a network could not exhibit any of the particular behaviors that 
are intrinsic to the more specific object types.

Be aware that some object properties are defined per-unit, which means that they are in relation to a reference value for the entire network, 
or for the part of the network to which they belong.  All MVA values are in relation to the network mvabase property.  
Voltages are in relation to the root node's voltage, which varies over time.  When voltage regulars or transformers intercede 
between the node and the root node, then their voltage becomes the voltage base.


*****************************************************************
*****************************************************************
\subsection user6_0 6.0 Residential Module
This section defines the objects and properties supported by the residential module. 

*****************************************************************
\subsection user6_1 6.1 House Object Properties
Each house object supports the following properties that define its behavior.
-	size (double)

*****************************************************************
\subsection user6_2 6.2 Lights Object Properties
Each lights object supports the following properties that define its behavior.
-	type (select). The type property is initialized by setting it to the number of the desired lighting type.
	-	Incandescent
	-	Fluorescent
	-	Compact Fluorescent
	-	Solid-State Lighting (SSL)
	-	High Intensity Discharge (HID)
-	placement (select). 
	-	Indoor=0
	-	Outdoor=1
-	installed_power  (double)
-	circuit_split (double)
-	power_density (double)
-	demand (double)
-	power_demand (complex)
-	internal_heat (double)
-	external_heat (double)
-	kwh_meter (complex)

*****************************************************************
\subsection user6_3 6.3 Refrigerator Object Properties
-	size (double)

*****************************************************************
\subsection user6_4 6.4 Washer Object Properties
-	installed_power (double)

*****************************************************************
\subsection user6_5 6.5 Range Object Properties
-	installed_power (double)

@todo complete the residential module user documentation (ticket 155)

*****************************************************************
*****************************************************************
\subsection user7_0 7.0 Commercial Module
The commercial module is not supported in GridLAB-D Version 1.1 but is planned for future versions.

@todo complete the commercial building module user documentation (ticket 155)
 
*****************************************************************
*****************************************************************
\subsection user8_0 8.0 Tape Module
This section defines the objects and properties supported by the tape module. 

*****************************************************************
\subsection user8_1 8.1 Player Object Properties
<<<<<<< .mine
•	property (string32) - the published name of the property whose values are written from the tape
•	file (string1024) - a name for locating the file, global object, or database with the player data
•	filetype (string8) - defines what media to use for the player (file, memory, odbc)
•	loop (int32) - how many times to play this file
=======
-	property (string32)
-	file (string1024)
-	filetype (string8)
-	loop (int32)
>>>>>>> .r333

*****************************************************************
\subsection user8_2 8.2 Shaper Object Properties
<<<<<<< .mine
•	property (string32) - the published name of the property whose values are written from the tape
•	file (string1024) - a name for locating the file, global object, or database with the shaper data
•	filetype (string8) - defines what media to use for the shaper (file, memory, odbc)
•	group (string256) - the find statement that determines which objects to shape values on
•	magnitude (double)
•	events (double)
=======
-	property (string32)
-	file (string1024)
-	filetype (string8)
-	group (string256)
-	magnitude (double)
-	events (double)
>>>>>>> .r333

*****************************************************************
\subsection user8_3 8.3 Recorder Object Properties
<<<<<<< .mine
•	property (string1024) - the published name of the property whose values are written to the tape
•	file (string1024) - a name for locating the file, global object, or database for the recorder data
•	trigger (string32) - the condition to write values on, such as ">40" or "=TRIPPED"
•	interval (int64) - how frequently to write the value of the aggregate (-1 == on every sync call)
•	limit (int32) - the maximum number of lines to write
=======
-	property (string1024)
-	file (string1024)
-	trigger (string32)
-	interval (int64)
-	limit (int32)
>>>>>>> .r333

*****************************************************************
\subsection user8_4 8.4 Collector Object Properties

•	property (string1024) - the published name of the property whose values are written to the tape
•	file (string 1024) - a name for locating the file, global object, or database for the collector data
•	trigger (string32) - the condition to write values on, such as ">40" or "=TRIPPED"
•	interval (int64) - how frequently to write the value of the aggregate (-1 == on every sync call)
•	group (string256) - defines the aggregate group (see aggregations)
•	limit (int32) - the maximum number of lines to write

@todo complete the tape module user documentation (ticket 155)

*****************************************************************
*****************************************************************
\subsection user9_0 9.0 Climate Module 
Note: this module needs work. It will export variables eventually. You can create more than one climate object, but there is no benefit from doing so.

@todo complete the climate module user documentation (ticket 155)

*****************************************************************
*****************************************************************
\subsection user10_0 10.0 Troubleshooting

@todo write troubleshooting section (doxygen, high priority) (ticket #157)



*****************************************************************

**/
