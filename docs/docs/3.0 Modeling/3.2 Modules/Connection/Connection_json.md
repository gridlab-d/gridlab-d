# json

Implementation of JSON data exchange with external applications The json class implements a connection to external software using the [JSON link protocol]. 

## Synopsis
    
    
    module [connection];
    class json {
       link "allow:_object_._property_ [<-|->] _remote_ ";
       link "forbid:_object_._property_ [<-|->] _remote_ ";
       link "[init|precommit|presync|sync|postsync|prenotify|postnotify|commit|finalize|term]:_object_._property_ [<-|->]_remote_ ";
       option "connection:[client|server], udp,readcache 256, writecache 256";
       option "transport:port _number_ , header_version _digit_ , hostname [_hostname_ |_ipv4addr_], debug_level _digit_ , on_error {retry|abort|ignore}, maxretry [_number_ |none]";
    }

## Class members

### link

The link pseudo-member is used to control access using allow and forbid, and control the event mapping process. 

#### allow

The allow link specifier indicates that a normally prohibited exchange of data should be allowed. This specifier only has an effect for security modes that limit data exchanges. 

#### forbid

The forbid link specifier indicates that a normally allowed exchange of data should be forbidden. his specifier only has an effect for security modes that permit data exchanges. 

### option

The option pseudo-member is used to control connection layer and transport layer options. 

#### init

The init option indicates the data elements are to exchange during an [INIT] event. 

#### precommit

**TODO**: 

#### presync

**TODO**: 

#### sync

**TODO**: 

#### postsync

**TODO**: 

#### prenotify

**TODO**: 

#### postnotify

**TODO**: 

#### commit

**TODO**: 

#### finalize

**TODO**: 

#### term

**TODO**: 

#### connection

**TODO**: 

#### client

**TODO**: 

#### server

**TODO**: 

#### udp

**TODO**: 

#### readcache

**TODO**: 

#### writecache

**TODO**: 

#### transport

**TODO**: 

#### port

**TODO**: 

#### header_version

**TODO**: 

#### hostname

**TODO**: 

#### debug_level

**TODO**: 

#### on_error

**TODO**: 

#### retry

**TODO**: 

#### abort

**TODO**: 

#### ignore

**TODO**: 

#### maxretry

**TODO**: 

#### none

**TODO**: 

## Example
    
    
    module [connection];
    object json {
       link "init:my.x-> var1";
       link "init:my.y <- var2";
       link "sync:my.x-> var1";
       link "sync:my.y <- var2";
       option "connection:client,udp";
       option "transport:hostname localhost, timeout 1000, on_error retry, maxretry none";
    }
    