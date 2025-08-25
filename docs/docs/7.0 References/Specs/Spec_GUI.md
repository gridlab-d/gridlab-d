# Spec:GUI

**Source URL:** https://GridLAB-D™.shoutwiki.com/wiki/Spec:GUI
# Spec:GUI

Approval item: 

## Contents

  * 1 GUI HTML Encoding
    * 1.1 Wait operations
  * 2 GUI Generation Principles
    * 2.1 Layout Rules
    * 2.2 Entity Linkage
  * 3 GUI Definition Syntax
    * 3.1 Common attributes
    * 3.2 Common entities
    * 3.3 Action entities
  * 4 Implementation approach
    * 4.1 load.c
    * 4.2 gui.h
    * 4.3 gui.c
    * 4.4 server.c
    * 4.5 graph.c
  * 5 Examples
  * 6 **TODO**
    * 6.1 See also
This document describes the technical requirements for a GridLAB-D™ Graphical User Interface. 

  


# 

GUI HTML Encoding

The simplest GUI can be encoded directly in HTML and loaded by the GridLAB-D™ model using the following syntax: 
    
    
    gui {
      source _file.html_ ;
    }
    

The HTML file may contain any HTML code desired. In addition, it may use Javascript script code load from the support file `rt/gridlabd.js` to perform API calls to the core. 

## Wait operations

A GUI block may halt the load process by including a wait entity, as follows: 
    
    
    gui {
      source _file.html_ ;
      wait _action_ ;
    }
    

When a wait entity is encountered, the GUI load stops to the GLM load process and turns over control to the GUI. When the GUI action specified is sent by the GUI, such as after a button is pressed, the loader continues loading the GLM file. 

# 

GUI Generation Principles

A GridLAB-D™ GUI is generated automatically by GridLAB-D™ when a GUI block is present in the GLM file. GUIs are collections of GUI entities that link the user to various parts of the GridLAB-D™ system. The entities include 

  * Text entities that can be either constant or linked to GridLAB-D™ objects, variables, or properties;
  * Input entities that are linked to GridLAB-D™ object properties or variables;
  * Action entities that control the state of GridLAB-D™; and
  * Graphical entities derived from GridLAB-D™ outputs.
## Layout Rules

GUI objects are places on the screen in columns and rows. Normally, each entity is placed the column that corresponds to its position relative to the left side of the current block. Thus the second item in the second row will be placed below the second item in the first row. The exception to this rule is the `span` group, which suppressed this behavior for its members. 

## Entity Linkage

Each entity, if suitable, may be linked to an appropriate property, variable or output in GridLAB-D™. In general the association rules are as follows: 

Entity linkage rules  Property | Text | Input | Check | Radio | Select | Custom1 | Graph   
---|---|---|---|---|---|---|---  
Boolean | R | RW | RW | RW | RW |  |   
Integer | R | RW |  |  |  | slider(RW) |   
Enumeration | R | RW |  | RW | RW |  |   
Set | R | RW | RW |  |  |  |   
Double | R4 | RW4 |  |  |  | slider(RW) |   
Complex | R4 | RW4 |  |  |  | 2d(RW) |   
Timestamp | R | RW |  |  |  | calendar(RW)  
clock(RW) |   
Object | R | RW |  |  |  | search(RW) |   
Filename | R | RW2 |  |  |  | file(RW) | R3  
  
1 Custom entities are property specific (e.g., calendar, clock, slider, 2D input).  
2 Includes the auxiliary [...] button  
3 Only CSV output files from recorders, collectors, and histograms can be graphed. 4 Includes automatic unit conversion for both read and write operations. 

# 

GUI Definition Syntax

The layout of the GUI presentment and linkage of the GUI entities to the GridLAB-D™ model objects is defined in the GLM file (see [GUI_FR#Presentment encoding]). 

The syntactical structure of the GLM GUI block shall be 
    
    
     _gui_block_ : "gui" { _gui_list_ }
     
     _gui_list_ : _entity_block_ _gui_list_
     _gui_list_ : _parameter_value_ _gui_list_
     _gui_list_ :
     
     _entity_block_ : _entity_ { _gui_list_ }
     
     _parameter_value_ : _parameter_ _value_ ;
     
     _entity_ : "tab"
     _entity_ : "group"
     _entity_ : "table"
     _entity_ : "span"
     _entity_ : "text"
     _entity_ : "input"
     _entity_ : "checkbox"
     _entity_ : "radio"
     _entity_ : "select"
     _entity_ : "datetime"
     _entity_ : "graph"
     _entity_ : "start"
     _entity_ : "stop"
     _entity_ : "pause"
     _entity_ : "reset"
     _entity_ : "quit"
     _entity_ : "open"
     _entity_ : "save"
     _entity_ : "saveas"
     _entity_ : "export"
     
     _parameter_ : "link"
     _parameter_ : "value"
     _parameter_ : "lines"
     _parameter_ : "password"
     _parameter_ : "disabled"
     _parameter_ : "multiple"
     _parameter_ : "default"
     _parameter_ : "member"
     _parameter_ : "choices"
     _parameter_ : "x"
     _parameter_ : "y"
     _parameter_ : "width"
     _parameter_ : "height"
     _parameter_ : "color"
     _parameter_ : "size"
     _parameter_ : "font"
     _parameter_ : "style"
     _parameter_ : "unit"
     
     _value_ : _object_name_ "." _property_name_
     _value_ : _global_varname_
     _value_ : _module_name_ ":" _varname_
     _value_ : _environment_varname_
     _value_ : _set_
     _value_ : _enumeration_
     _value_ : _real_
     _value_ : _integer_
     _value_ : _boolean_
     _value_ : _string_
    

## Common attributes

All entities recognize the following attributes: 

  * **x** \- specifies the horizontal position in pixels (for integers) or fraction of the block (for values between 0 and 1) as measured from the left of the block.
  * **y** \- specifies the vertical position in pixels (for integers) or fraction of the block (for values between 0 and 1) as measured from the top of the block.
  * **width** \- specifies the width in pixels (for integers) or fraction of the block (for values between 0 and 1).
  * **height** \- specifies the height in pixels (for integers) or fraction of the block (for values between 0 and 1).
  * **color** \- specifies the color of the entity text as a color name or a hexadecimal rgb code.
  * **size** \- specifies the size of the entity text as a size name or a pixel height.
  * **font** \- specifies the font of the entity text as a font family name.
  * **style** \- specifies the font style of the entity as a style set (e.g., **bold** , **italic** , **underline**)
## Common entities

**tab**
    A tab block is used to create a tabbed grouping of entities. Recognized parameters are:

  * **value** \- specifies the text to be displayed on the tab
**group**
    A group block is used to create a visible grouping of entities using a box. Recognized parameters are:

  * **value** \- specifies the text to be displayed in the upper left corner of the box.
**table**
    A table is created to display data in a file. Recognized parameters are:

  * **link** \- specifies the source of the data.
  * **columns** \- specifies the number of columns of data to display.
  * **rows** \- specifies the number of rows of data to display.
The following entities may be used in any block. 

**text**
    Creates a text entity on the screen. Text entities recognized the following attributes:

  * **lines** \- specifies how many lines of text are displayed.
**input**
    Creates a text input entity on the screen. The input entity must be linked to a string or number property. If the linked property is a number property, the input shall be constrained to match the property. Supported property shall include integers (8,16,32,64 bit), doubles and complex numbers. When the property has units associated with it, the property shall be displayed in those units by default, or use the units provided by the **unit** attribute. If the user enters a unit, the input shall be converted accordingly, if possible. Loadshapes and enduses shall be considered strings. Input entities recognized the following attributes:

  * **lines** \- specifies how many lines of text are displayed.
  * **password** \- specifies that the text should be obfuscated.
  * **disabled** \- specifies that the text should be read only.
  * **unit** \- specifies the units in which to display numbers.
**select**
    Creates a selection input. The select must be linked to a property that is a set, an enumeration, or an object reference so that the choices can be automatically presented. The select entity recognizes the following attributes:

  * **lines** \- specifies the number of select items to show.
  * **drop** \- specifies whether a drop list is used
**check**
    Creates a checkbox input. If _member_ is omitted, the link must refer to a boolean property. If _member_ is included, it must match a member of the set property that _link_ refers to. The attribute _value_ provides the text label to be placed adjacent to the checkbox. Check entities support the following attributes:

  * **member** \- specifies which member of the set the check box is linked to.
**radio**
    Creates a radio button input. The link must refer to an enumeration property. The value of _member_ must match a member of the enumeration that the link refers to. The _value_ provides the text label to be placed adjacent to the radio button. Radio entities support the following attributes

  * **member** \- specifies which member of the enumeration the radio is linked to.
**datetime**
    Create a date entry field. The link must refer to a TIMESTAMP property.

**graph**
    Creates a graph entity. The link must refer to a CSV data file, most often one generated by GridLAB-D™ during the run. Graph entities support the following attributes:

  * **package** \- specifies the plotting package to use (e.g., gnuplot)
  * **command** \- specifies the plotting command string to send to the package.
  * **refresh** \- specifies the plotting refresh delay in seconds (default is to wait until done).
**file**
    Create a filename entity. The link must refer to a string. A "..." button will placed adjacent to the field, which when clicked open the standard file dialog.

## Action entities

The following entities create action buttons. 

**start**
    Creates a button which when clicked starts the simulation. This entity shall be disabled when the simulation is already running.

**pause**
    Creates a button which when clicked pauses the simulation. This entity shall be disabled when the simulation isn't running.

**stop**
    Creates a button which when clicked stops the simulation. This entity shall be disabled when the simulation isn't running. A stop action shall require confirmation.

**reset**
    Creates a button which when clicked reset the simulation. This entity shall be disabled when the simulation hasn't started. A reset action shall require confirmation.

**quit**
    Creates a button which when clicked exit the GUI. This action shall require confirmation when the current session is not saved.

**open**
    Creates a button which when clicked open a file dialog and loads the selected GLM, XML, or GLD file. This action shall require confirmation when the current session is not saved.

**save**
    Creates a button which when clicked saves the current session as a GLD file.

**saveas**
    Creates a button which when clicked open a file dialog and saves the current session to the specified GLD file.

**export**
    Creates a button which when clicked opens a file dialog and saves the current session to the specified GLM or XML file.

# 

Implementation approach

The GUI API is an integral part of the core. The following coding elements are required 

## load.c

A new top level loader block is added to support parsing `gui` block. This includes all the subordinate GUI blocks. 

## gui.h

The `gui` entity structure is defined. It allows GUI entities to be organized in a hierarchy (parent-child) or linked lists that mirrors the representation in the GLM file. 

## gui.c

The GUI module itself contains GUI entity creation, management and generation code. 

## server.c

The server code is extended to allow linkage commands from the GUI module. The linkages include 

  * reading a variable or property;
  * writing a variable or property; and
  * initiating an action.
## graph.c

The current version of GridLAB-D™ (2.1) supports graphing using only `gnuplot`. However, for web-based GUIs, `jpgraph` will be supported as well. 

# 

Examples

[File:Spec:GUI Figure 1.png](/w/index.php?title=Special:Upload&wpDestFile=Spec:GUI_Figure_1.png "File:Spec:GUI Figure 1.png")

Figure 1 - An example GUI

. 

The following GLM file generates the page shown in Figure 1. 
    
    
    gui {
    	title link :modelname;
    	group {
    		title link House1:name;
    		row { // first row
    			text value "Floor area:";
    			input {
    				link House1:floorarea;
    				unit "sf";
    			}
    		}
    		row { // second row
    			text value "Floor area:";
    			check link House1:system_type;
    		}
    		row { // third row
    			text value "System mode: ";
    			check link House1:system_mode;
    		}
    		row { // fourth row
    			text value "Construction quality";
    			select link "House1:thermal_integrity_level";
    		}
    		row {
    			text value "File name:";
    			span {
    				input link :modelname;
    				action save;
    				action load;
    			}
    		}
    	}
    	row {
    		span {
    			action restart;
    			action run;
    			action pause;
    			action stop;
    		}
    	}
    	status link :stream;
    }
    
    object house {
    	name "House1";
    	floorarea 1200 sf;
    }
    

# 

**TODO**

  1. Special entities for datetime, schedules, loadshapes, and enduses neeed to be defined.
## See also

  * [Graphic User Interface API]
    * [Requirements]
    * Specifications
    * [Technical Manuals]
    * [Validation]
