# Finding Objects

Searching for objects and defining object groups both use the same syntax.


## Search criteria
Object searches are usually expressed using a search criteria, such as
```
object class {
  group "property op value";
  // ..
}
```
where class is the class of object that uses the group property (e.g., collector, histogram), property is the object property to match against (e.g., name, class, parent), op is the comparison operator (e.g., =, <, !~), and value the value to match against.

The following properties are always recognized, in addition to all the properties defined by the class:

- **class**
    the name of class which implements the object
- **module**
    the name of the module which implements the class
- **id**
    the id number of the object
- **name**
    the name of the object
- **parent**
    the parent of the object (matches the name of the parent)
- **rank**
    the object's rank
- **latitude**
    the object's latitude
- **longitude**
    the object's longitude
- **clock**
    the object's clock
- **in_svc**
    the object's in-service date/time
- **out_svc**
    the object's out-of-service date.time
- **flags**
    the object's flags (see Object header properties )

## Compound criteria
Multiple search criteria can be indicated using and/or as appropriate. Parenthetical operators are not supported.

**Caveat**
    
See http://sourceforge.net/p/gridlab-d/tickets/668 regarding the OR functionality which is not implemented prior to Hassayampa (Version 3.0). Resolution is planned in Hassayampa (Version 3.0).

The implementation of the and and or operators is incomplete and not mathematically correct. Any logical statement joined with an and will remove all objects not identified with that operation from the working set. Any logical statement joined with an or will add all objects that match that operation to the working set. There is no sense of operator precedence, and operators are processed from left to right.

For example, to build the set 'all triplex_meters with groupid blue or groupid red', the set must be 'groupid=red OR groupid=blue AND class=triplex_meter'. First the set is populated with all objects with a red or blue groupid, then is filtered for only the triplex meters in that set.

The set 'all triplex_meters with groupid blue, plus all meters with groupid red' cannot be generated with the existing system. 'groupid=red AND class=triplex_meter OR groupid=blue AND class=meter' will only return meters with the blue groupid.


### Date/time values
Date and time values must be fully qualified absolute date/time stamps using the appropriate timezone. Relative time can also be given using s, m, h, d, or w suffixes as desired, e.g., 1800s to indicate 30 minutes.


## Operators
```
!= Not equal, e.g., property!=number
<= Less than or equal, e.g., property<=number
>= Greater than or equal, e.g., property>=number
!~ Not like, e.g., property<=string
= Equal, e.g., property=number
< Less than, e.g., property<=number
> Greater than, e.g., property>=number
~ Like, e.g., property<=string
```