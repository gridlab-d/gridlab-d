# Object (directive)

**Source URL:** https://gridlab-d.shoutwiki.com/wiki/Writing_Objects
# Object (directive)



object \- The object directive is used to define one or more objects in the model. 

# 

Synopsis
    
    
    object _class_ {
      name _object-name_ ;
      id _object-id_ ;
      groupid _group-id_ ;
      parent _parent-name_ ;
      rank _rank_ ;
      schedule_skew _schedule_skew_ ;
      latitude _degrees-minutes-seconds_ ;
      longitude _degrees-minutes-seconds_ ;
      in _date-time_ ;
      out _date-time_ ;
      heartbeat _seconds_ ; // [Template:NEW30]
      flags NONE|HASPLC|LOCKED|RECALC|FOREIGN|SKIPSAVE|RERANK|DELTAMODE;
    }
    object _class_ :_id_ { ... }
    object _class_ :_from-id_.._to-id_ { ... }
    object _class_ :.._count_ { ... }
    

# 

GLM

## Common properties

All objects share common properties that can be defined. 

### name

Specifies the unique name used to reference this object from other objects. Object names must only contain letters, numbers, and underscores. By default, object names cannot start with a number, but the global property `relax_naming_rules` can be set to a nonzero value to work around this. Starting an object name with a number can lead to parsing errors when linking objects at load time. 

### id

Specifies the unique id number used to reference this object when no name is given. 

### groupid

Specifies a group number to which this object belongs. This is used to help find objects that are aggregated using [collectors]. 

### parent

Specifies the parent object in the object ranks. `parent [string];` will set an object's parent as the specifically named object. If an object is nested within another object, it will automatically use the object that it is defined within as the parent object. Entering `root;` into the object's property block will set an object to explicitly not have a parent. 

### rank

The rank of the object determines its order of execution. An object's parent will always have a numerically greater rank than its children. Object ranks can be increased, but not decreased from within a model file. Increasing an object's rank will cause it to be called later in the pre-top down and post-top down passes, and earlier in the bottom-up pass. 

### schedule_skew

The number of seconds to offset any input schedule signals by, used to smooth out the otherwise lock-step behavior when changing parameters. Positive and negative integers are equally valid, telling the object to use the value later or sooner, respectively. 

### latitude

Specifies the latitude of the object's geo-coordinates. It is valid to use decimal numbers with an N, S, E, or W to indicate the hemisphere. The format `12N34'56"` is also valid. **Note:** as of [Hassayampa (Version 3.0)] only the format `12N34.56` or `12N34:56` are valid. 

### longitude

Specifies the longitude of the object's geo-coordinates. See latitude

### in

Specifies the date and time at which the object becomes active in the simulation. The default is [INIT]. Objects that are not in service for the entirety of a given run should not parent any objects. 

### out

Specifies the date and time at which the object becomes inactive in the simulation. The default is [NEVER]. See in. 

### heartbeat

[Template:NEW30] The object heartbeat determines the number of seconds that elapse between calls to the **heartbeat__classname_()** export function. If the object heartbeat is zero or if the object's class does not export the heartbeat function, the heartbeat is not called. By default the object heartbeat is zero. 

### flags

Specifies the flags for special object behavior. See below for the specific meanings of the various flags 

#### NONE

Indicates that no flags are set (default). 

#### HASPLC

Indicates that the object has active [PLC] code. Deprecated as of [Hassayampa (Version 3.0)]

#### LOCKED

Indicates that the object is current locked against concurrent memory access control. 

#### RECALC

Indicates that the object is in an inconsistent state and needs an internal recalculation to be performed at the earliest opportunity. 

#### FOREIGN

Indicates that the object was created by a DLL and its memory cannot be freed by the core. 

#### SKIPSAFE

Indicates that the object sync functions can be safely skipped. 

#### RERANK

Reserved for internal use only. 

#### DELTAMODE

Indicates that the object should be included in any [subsecond] processing . 

## Single objects

To create a single object use the syntax: 
    
    
    object _class_ {
      _property_ _value_ ;
      // ...
    } 
    

## Numbered objects

To create an object with a specific identification number use the syntax: 
    
    
    object _class_ :_id_ {
      _property_ _value_ ;
      // ...
    }
    

Note
    There is no guarantee that the object will keep the assigned id number once loaded in memory. However, the number given will be used to ensure a unique identity for that object.

## Multiple numbered objects

To define multiple objects with identification numbers in a range use the syntax: 
    
    
    object _class_ :_from_.._to_ {
      _property_ _value_ ;
      // ...
    }
    

## Multiple objects

To define multiples objects use the syntax: 
    
    
     object _class_ :.._count_ {
      _property_ _value_ ;
      // ...
    }
    

## Expansions

There are a number of intrinsic expansions available while an object is being defined: 

  * **{file}** embeds the current file (full path,name,extension)
  * **{filename}** embeds the name of the file (no path, no extension)
  * **{fileext}** embeds the extension of the file (no path, no name)
  * **{filepath}** embeds the path of the file (no name, no extension)
  * **{line}** embeds the current line number
  * **{namespace}** embeds the name of the current namespace
  * **{class}** embeds the classname of the current object
  * **{id}** embeds the id of the current object
  * **{var}** embeds the current value of the current object's variable _var_
Expansions are embedded using the syntax: 
    
    
     object _class_ {
       _property_ `_value_{_expansion_}_value_ `;
     }
    

For example, the following property assignment will embed the object id in the property _my_string_ : 
    
    
       my_string `object_{id}`;
    

# 

See also

  * [Class directive]
  * [Directives]
