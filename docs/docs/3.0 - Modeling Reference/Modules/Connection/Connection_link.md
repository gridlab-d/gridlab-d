# link

The **link** pseudo-property allow one or more variables to be mapped to and from a remote system during various events in GridLAB-D™. 

## Synopsis
    
    
    module connection;
    object example {
      link "map1 :local1 direction1 remote1;
      link "map2 :local2 direction2 remote2;
      ...
      link "mapN :localN directionN remoteN;
    }
    

## Description

The _map_ may be one of the following: 

  * **allow** : Specifies that a mapping forbidden under [STANDARD] [security] should be allowed.
  * **deny** : Specifies that a mapping allowed under [STANDARD] [security] should be forbidden.
  * **init** : Map during initialization sequence
  * **precommit** : Map during precommit sequence
  * **presync** : Map during presync sequence
  * **sync** : Map during sync sequence
  * **postsync** : Map during postsync sequence
  * **commit** : Map during commit sequence
  * **plc** : Map during PLC sequence
  * **finalize** : Map during finalize sequence
  * **term** : Map during termination sequence
  * **prenotify** : Map before a change to any connection property
  * **postnotify** : Map after a change to any connection property
  * **file** : Map using the specifications provided in the specified link file file

The _local_ variable is specified in the form _name_._property_. If the _name_._property_ form is not found, the list of global variables is searched for a match. The _name_ and the _property_ may be specified using simple pattern matching as _prefix_ *, in which case all names or properties that have the same prefix will be linked. 

The _direction_ may be specified as one of the following 

  * <- : Copy to local from remote
  * -> : Copy to remote from local
  * <-> : Copy both ways

The _remote_ variable is specified as a string of arbitrary structure terminated by a semicolon or white space. The following special characters will be substituted if encountered 

  * \# : GridLAB-D™ object number
  * \@ : GridLAB-D™ object name
  * \$ : GridLAB-D™ object class
  * \\ : A single backslash
  * \! : Lookup _local_ in the link file index

### Link file

The **link** map file is used to provide a connection map with a list of **link** directives and mapping it should use. The **link** map contains a section for each map, e.g., 
    
    
    allow: 
      local1 direction1 remote1;
      local2 direction2 remote2;
      ...
      localN directionN remoteN; 
    deny:
      localN+1 directionN+1 remoteN+1;
      localN+2 directionN+2 remoteN+2;
      ...
      localM directionM remoteM;
    init:
      ...
    

In addition the map can contain an index section to translate _local_ names to _remote_ names using the \! special character in the remote name, e.g., 
    
    
    index:
      local1 remote1
      local2 remote2
      ...
      localN remoteN
    

## Example

The following example creates a native connection and maps 
    
    
    module connection;
    class test {
      double x;
      double y;  
    }
    object test {
      name my;
      x 1.23;
      y 3.45;
    }
    object native {
    	mode SERVER;
    	transport TCP;
    	link "precommit:my.x->var1";
    	link "commit:my.y<-var2";
    }
    