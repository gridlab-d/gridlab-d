# Mysql

!!! warning

	This page contains features that are unfinished, were never implemented, or have since been deprecated. We preserve these pages for archival purposes, and also as a foundational resource for prospective developers who may wish to implement the same or similar feature. Many of these pages provide robust explanations of the theory behind a particular module or feature that we hope readers will find useful. 
  
	**This page does not reflect the current state of GridLAB-D™**

mysql \- MySQL module [Template:NEW30]

## Synopsis
    
    
    module mysql {
      hostname "localhost";
      username "gridlabd";
      password "";
      schema "gridlabd";
      port 3306;
      socketname "/tmp/mysql.sock";
      clientflags [COMPRESS]|[FOUND_ROWS]|[IGNORE_SIGPIPE]|[INTERACTIVE]|[LOCAL_FILES]|[MULTI_RESULTS]|[MULTI_STATEMENTS]|[NO_SCHEMA]|[ODBC]|[SSL]|[REMEMBER_OPTIONS];
    }
    

## Description

The mysql module implements the principal [tape] module classes recorder, [player], and [collector] in a property-compatible manner. For details on the functionality of these classes, see the [tape] module. The main difference between the [tape] implementation and the mysql implementation is that these classes can refer to a [database] connection if there is more than one connection in the GLM file. 

### hostname

The default server to use for a connection. See [database] [hostname] for details. 

### username

The default username to use for a connection. See [database] [username] for details. 

### password

The default password to use for a connection. See [database] [password] for details. 

### schema

The default schema to use for a connection. See [database] [schema] for details. 

### port

The default port to use for a connection. See [database] [port] for details. 

### socketname

The default socket name to use for a connection. See [database] [socketname] for details. 

### clientflags

The default client flags to use for a connection. See [database] [clientflags] for details. 

## Classes

[database]
    Implements a connection to a MySQL server. There must be at least one defined in a model in order to use the recorder, player, or collector objects.
[ recorder]
    Implements a property recorder using the specified (or last) database connection.
[ player]
    Implements a property player using the specified (or last) database connection.
[ collector]
    Implements a property aggregate collector using the specified (or last) database connection.

## Prerequisites

If you are using your local host as the MySQL server, you must install [MySQL Server](http://www.mysql.com/downloads/mysql/). After you have set up the server, you should create the user **gridlabd** and grant that user permission to create databases if you intend to use the default connection parameters. 

Windows Systems
    You must include the MySQL client library in the **PATH** environment variable.

Linux/MacOSX Systems
    
    You must include the MySQL client library in the **(DY)LD_LIBRARY_PATH** environment variable.

Although it is not required, use of the [MySQL Workbench](http://www.mysql.com/downloads/workbench/) is highly recommended to facilitate managing and reviewing the results from GridLAB-D's mysql module. 

## Bugs

On Windows 7 it is a common problem that GridLAB-D™ cannot find `libmysql.dll` when the `module mysql` directive is parsed in a GLM file, regardless of the `PATH` environment variable. The only solution seems to be to include the MySQL Connector library folder in `GLPATH` or to copy the file `libmysql.dll` to the GridLAB-D™ library folder. 

## Version

The mysql module was introduced in [Hassayampa (Version 3.0)]. 

## Related Concepts:

  * mysql module 
    * [database] class
    * recorder class
    * [player] class
    * [collector] class
    * [MySQL Import/Export]
  * Technical manuals 
    * [Programmer's manual]
    * [MySQL How To Guide]
