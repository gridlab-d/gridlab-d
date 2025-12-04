# json


!!! warning

	This page contains features that are unfinished, were never implemented, or have since been deprecated. We preserve these pages for archival purposes, and also as a foundational resource for prospective developers who may wish to implement the same or similar feature. Many of these pages provide robust explanations of the theory behind a particular module or feature that we hope readers will find useful. 
	
	**This page does not reflect the current state of GridLAB-D™**

Implementation of JSON data exchange with external applications The json class implements a **connection** to external software using the JSON link protocol. 

## Synopsis
    
    
    module connection;
      class json {
         link "allow:object.property [<-|->] remote";
         link "forbid:object.property [<-|->] remote";
         link "[init|precommit|presync|sync|postsync|prenotify|postnotify|commit|finalize|term]:object.property [<-|->]remote";
         option "connection:[client|server], udp,readcache 256, writecache 256";
         option "transport:port number, header_version digit, hostname [hostname|ipv4addr], debug_level digit, on_error {retry|abort|ignore}, maxretry [number|none]";
      }

## Class members
TODO - Incomplete - Class member table for json connection incomplete

Member | Description
-- | --
**link** | The link pseudo-member is used to control access using allow and forbid, and control the event mapping process. 
**allow** | The allow link specifier indicates that a normally prohibited exchange of data should be allowed. This specifier only has an effect for security modes that limit data exchanges. 
**forbid** | The forbid link specifier indicates that a normally allowed exchange of data should be forbidden. his specifier only has an effect for security modes that permit data exchanges. 
**option** | The option pseudo-member is used to control connection layer and transport layer options. 
**init** | The init option indicates the data elements are to exchange during an [INIT] event. 
**precommit** | **TODO**
**presync**| **TODO**: 
**sync**| **TODO**: 
**postsync**| **TODO**: 
**prenotify**| **TODO**: 
**postnotify**| **TODO**: 
**commit**| **TODO**: 
**finalize**| **TODO**: 
**term**| **TODO**: 
**connection**| **TODO**: 
**client**| **TODO**: 
**server**| **TODO**: 
**udp**| **TODO**: 
**readcache**| **TODO**: 
**writecache**| **TODO**: 
**transport**| **TODO**: 
**port**| **TODO**: 
**header_version**| **TODO**: 
**hostname**| **TODO**: 
**debug_level**| **TODO**: 
**on_error**| **TODO**: 
**retry**| **TODO**: 
**abort**| **TODO**: 
**ignore**| **TODO**: 
**maxretry**| **TODO**: 
**none**| **TODO**: 

## Example
    
    
    module connection;
    object json {
       link "init:my.x-> var1";
       link "init:my.y <- var2";
       link "sync:my.x-> var1";
       link "sync:my.y <- var2";
       option "connection:client,udp";
       option "transport:hostname localhost, timeout 1000, on_error retry, maxretry none";
    }
    