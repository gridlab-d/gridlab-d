# Mysql

## Synopsis
    
    
    [module](/wiki/Module "Module") mysql {
      hostname "localhost";
      username "gridlabd";
      password "";
      schema "gridlabd";
      port 3306;
      socketname "/tmp/mysql.sock";
      clientflags [COMPRESS](/wiki/Database#COMPRESS "Database")|[FOUND_ROWS](/wiki/Database#FOUND_ROWS "Database")|[IGNORE_SIGPIPE](/wiki/Database#IGNORE_SIGPIPE "Database")|[INTERACTIVE](/wiki/Database#INTERACTIVE "Database")|[LOCAL_FILES](/wiki/Database#LOCAL_FILES "Database")|[MULTI_RESULTS](/wiki/Database#MULTI_RESULTS "Database")|[MULTI_STATEMENTS](/wiki/Database#MULTI_STATEMENTS "Database")|[NO_SCHEMA](/wiki/Database#NO_SCHEMA "Database")|[ODBC](/wiki/Database#ODBC "Database")|[SSL](/wiki/Database#SSL "Database")|[REMEMBER_OPTIONS](/wiki/Database#REMEMBER_OPTIONS "Database");
    }
    

## Description

The mysql module implements the principal [tape](/wiki/Tape "Tape") module classes [recorder](/wiki/Recorder "Recorder"), [player](/wiki/Player "Player"), and [collector](/wiki/Collector "Collector") in a property-compatible manner. For details on the functionality of these classes, see the [tape](/wiki/Tape "Tape") module. The main difference between the [tape](/wiki/Tape "Tape") implementation and the mysql implementation is that these classes can refer to a [database](/wiki/Database "Database") connection if there is more than one connection in the GLM file. 

### hostname

The default server to use for a connection. See [database](/wiki/Database "Database") [hostname](/wiki/Database#hostname "Database") for details. 

### username

The default username to use for a connection. See [database](/wiki/Database "Database") [username](/wiki/Database#username "Database") for details. 

### password

The default password to use for a connection. See [database](/wiki/Database "Database") [password](/wiki/Database#password "Database") for details. 

### schema

The default schema to use for a connection. See [database](/wiki/Database "Database") [schema](/wiki/Database#schema "Database") for details. 

### port

The default port to use for a connection. See [database](/wiki/Database "Database") [port](/wiki/Database#port "Database") for details. 

### socketname

The default socket name to use for a connection. See [database](/wiki/Database "Database") [socketname](/wiki/Database#socketname "Database") for details. 

### clientflags

The default client flags to use for a connection. See [database](/wiki/Database "Database") [clientflags](/wiki/Database#clientflags "Database") for details. 

## 

Classes

[database](/wiki/Database "Database")
    Implements a connection to a MySQL server. There must be at least one defined in a model in order to use the recorder, player, or collector objects.
[ recorder](/wiki/Recorder_\(mysql\) "Recorder \(mysql\)")
    Implements a property recorder using the specified (or last) database connection.
[ player](/wiki/Player_\(mysql\) "Player \(mysql\)")
    Implements a property player using the specified (or last) database connection.
[ collector](/wiki/Collector_\(mysql\) "Collector \(mysql\)")
    Implements a property aggregate collector using the specified (or last) database connection.

## 

Prerequisites

If you are using your local host as the MySQL server, you must install [MySQL Server](http://www.mysql.com/downloads/mysql/). After you have set up the server, you should create the user **gridlabd** and grant that user permission to create databases if you intend to use the default connection parameters. 

Windows Systems
    You must include the MySQL client library in the **PATH** environment variable.

Linux/MacOSX Systems
    
    You must include the MySQL client library in the **(DY)LD_LIBRARY_PATH** environment variable.

Although it is not required, use of the [MySQL Workbench](http://www.mysql.com/downloads/workbench/) is highly recommended to facilitate managing and reviewing the results from GridLAB-D's mysql module. 

## 

Bugs

On Windows 7 it is a common problem that GridLAB-D™ cannot find `libmysql.dll` when the `module mysql` directive is parsed in a GLM file, regardless of the `PATH` environment variable. The only solution seems to be to include the MySQL Connector library folder in `GLPATH` or to copy the file `libmysql.dll` to the GridLAB-D™ library folder. 

## 

Version

The mysql module was introduced in [Hassayampa (Version 3.0)](/wiki/Hassayampa "Hassayampa"). 

## 

See also

  * mysql module 
    * [database](/wiki/Database "Database") class
    * [recorder](/wiki/Recorder_\(mysql\) "Recorder \(mysql\)") class
    * [player](/wiki/Player_\(mysql\) "Player \(mysql\)") class
    * [collector](/wiki/Collector_\(mysql\) "Collector \(mysql\)") class
    * [MySQL Import/Export](/wiki/MySQL_Import/Export "MySQL Import/Export")
  * Technical manuals 
    * [Programmer's manual](/wiki/Dev:mysql "Dev:mysql")
    * [MySQL How To Guide](/wiki/HowTo:mysql "HowTo:mysql")



Retrieved from "[https://GridLAB-D™.shoutwiki.com/w/index.php?title=Mysql&oldid=6200](https://GridLAB-D™.shoutwiki.com/w/index.php?title=Mysql&oldid=6200)"
  *[m]: This is a minor edit
