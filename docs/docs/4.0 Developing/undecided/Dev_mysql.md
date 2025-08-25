# Dev:mysql

**Source URL:** https://GridLAB-D™.shoutwiki.com/wiki/Dev:mysql
Dev:mysql \- MySQL Programmer's Manual 

The [mysql] module is designed to be compatible with the [tape] module such that any definition of an object created for the [tape] module will work when using the [mysql] module. 

However, a number of important assumptions are made when using the default behaviors of the [mysql] module: 

  1. The local host (127.0.0.1) has a running MySQL server (see below) and is accepting connection on port 3306 (TCP) or the socket `/tmp/mysql.sock`.
  2. The user **gridlabd** has been created on the local host's MySQL server.
  3. The user **gridlabd** has no password for client on the local host.
  4. The user **gridlabd** has permission to create and administer the schema **gridlabd**.
  5. Only one instance of the class [database] is created.
## Prerequisites

To build GridLAB-D™ with the [mysql] module, you must install the [MySQL Connector C](http://www.mysql.com/downloads/connector/c/) library. The [mysql] module was developed using Version 6.0 of the library. 

Linux and Mac OSX builds
    The MySQL libraries must be installed prior to running `./configure` and the libraries must be installed in `/usr/local/mysql-connector-c`. Because the downloaded files usually include the version number, you should create a symbolic link to the download folder:
    
    
    host% **cd /usr/local**
    host% **ln -s _mysql-connector-dir_ mysql-connector-c**
    

Windows builds
    The libraries must be installed in `;C:\Program Files\MySQL\MySQL Connector C\include`. Note that by default the MySQL Connector C installer includes the version number in the path so you should change the install folder to match the above path.

## Validation

The autotest validation procedure for the [mysql] module requires a local running [MySQL Server](http://dev.mysql.com) with the following 

  1. A schema named `gridlabd` that is empty.
  2. A user named `gridlabd` with `ALTER,CREATE,CREATE TEMPORARY TABLES,DELETE,DROP,INDEX,INSERT,LOCK TABLES,REFERENCES,SELECT,UPDATE` access to the schema `gridlabd` with no password from host `localhost` (do not use the IP address `127.0.0.1`). Do not set account limits or any administrative roles for this user.
**Note:** It is very strongly recommended that you do not use the `gridlabd` user for any other purpose than testing and validation because the test procedures often include `DROP DATABASE` commands resulting in total destruction of all data in the `gridlabd` schema. If you use the `NEWDB` option on the default schema `gridlabd` you will get a warning such as 
    
    
       WARNING  [INIT] : database:0 users NEWDB option on the default schema 'gridlabd' - this is extremely risky
    

but that's all you get before the deed is done. Obviously, autotests are expected to produce this warning because they typically use `NEWDB` on the default schema. 

## See also

  * [mysql] module 
    * [database] class
    * recorder class
    * [player] class
    * [collector] class
    * [MySQL Import/Export]
  * Technical manuals 
    * Programmer's manual
    * [MySQL How To Guide]

