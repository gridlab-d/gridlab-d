# HowTo:mysql

!!! warning

	This page contains features that are unfinished, were never implemented, or have since been deprecated. We preserve these pages for archival purposes, and also as a foundational resource for prospective developers who may wish to implement the same or similar feature. Many of these pages provide robust explanations of the theory behind a particular module or feature that we hope readers will find useful. 
  
	**This page does not reflect the current state of GridLAB-D™**

HowTo:mysql \- MySQL How To Guide 

## Contents

  * 1 Setup the default connection
  * 2 A simple mysql recorder
  * 3 A simple mysql player
  * 4 A simple mysql collector
  * 5 Using multiple mysql databases
  * 6 Converting a tape recorder to mysql
  * 7 Converting a tape player to mysql
  * 8 Converting a tape collector to mysql
  * 9 See also
The mysql module was designed to offer a high level of compatibility with the player, recorder, and collector objects in the tape module. This guide offers a HowTo for implementing mysql interfaces and converting tape I/O to mysql I/O. 

## Setup the default connection

The default database connects to the server on the **localhost** (on port `127.0.0.1` or socket name `/tmp/mysql.sock`) using the database schema **gridlabd** with the user **gridlabd** (no password). 

To use the default database you must install a MySQL server on the localhost. The installation procedure depends on the host platform and can be found at [www.mysql.com](http://www.mysql.com/). We strongly recommend also installed the MySQL WorkBench to allow administration of the server. Using the workbench you should create a user **gridlabd** with full control of the schema **gridlabd**. If you are concerned about the security of this schema, you can also set the password for this user and restrict access to **localhost** clients only. 

## A simple mysql recorder

The simple example of a mysql recorder is illustrated by the following example. The module declaration replaces a conventional tape module declaration and the specifications of the object is compatible (although not always identical): 
    
    
    // Examples:mysql_recorder_1.glm
    module mysql;
    database { 
      // uses defaults (see mysql for details)
    }
    class test {
      randomvar x; // random number
    }
    object test {
      x "type:normal(0,1); refresh:1hr"; // updated every hour
      object recorder {
        table "test";
        property x;
      };
    }
    

Omitting all specifications for the database object uses the default connection. 

If you want to record the values in a different unit than the unit to property you are recording, you must specify the unit with the property. If the property you are are recording also has a unit, the value will be converted automatically as it is sampled: 
    
    
    // Examples:mysql_recorder_2.glm
    module mysql;
    database { 
      // uses defaults (see mysql for details)
    }
    class test {
      randomvar x[h]; // random number of hours
    }
    object test {
      x type:normal(0,1); refresh:1hr"; // updated every hour
      object recorder {
        table "test";
        property x[min]; // converts hours to minutes
      };
    }
    

## A simple mysql player



## A simple mysql collector



## Using multiple mysql databases



## Converting a tape recorder to mysql



## Converting a tape player to mysql



## Converting a tape collector to mysql



## Related Concepts:

  * mysql module 
    * database class
    * recorder class
    * player class
    * collector class
    * MySQL Import/Export
  * Technical manuals 
    * Programmer's manual
    * MySQL How To Guide

