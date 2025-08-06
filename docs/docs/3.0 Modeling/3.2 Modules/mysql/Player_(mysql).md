# Player (mysql)

MySQL player class Template:NEW30

## Synopsis
    
    
    module mysql;
    object player {
      property _property-name_ ;
      table|file _source-table_ ;
      mode {"r","r+"};
      filetype {"CSV"};
      connection _database-object-name_ ;
      options 0;
      loop _number-of-loops_ ;
    }
    

## Description

The mysql player is designed to be compatible the the tape player object so that when the mysql module is used in place of the tape module, there are few changes, if any, required to the player objects. 

The source table must have a sequence field, _id_ , and a time field, _t_ , to function as a player data source. 

  * The sequence field must be an integer and determine the sequence in which records are presented to the player.
  * The time field must be a timestamp, datetime, or numeric value and determines the time value used to post values to objects. If the time field is a timestamp or datetime, values are posted using absolute time. If the time field is a numeric value, values are posted in relative time with the first time value being relative to the epoch (1970-01-01 00:00:00 UTC). If the time field is a floating point value or the time field includes microseconds, the player will be operate using subsecond intervals.



* **connection** -
Specifies the database object used to connect to MySQL. If none is provided, the last database defined is used.

* **file** -
This is a synonym for table provided for compatibility with tape player.

* **filetype** -
Provided for compability with tape player and has no real effect.

* **loop** -
    The input data table will be treated as a tape loop if the field type is an integer or double. The loop property determines how many time the input data table will rewind before the player ceases to update the target object's properties.

* **mode** -

    Specifies the read mode to use, which may be either "r" or "r+". This is provided for compatibility with tape player and has no real affect.

* **options** -

    No options are supported at this time.

* **property** -
    Specifies the target property (or properties) that are to be updated. The properties must match the field names in the source table. The data types are automatically converted using the following rules: 

  * Timestamps are parsed as `yyyy-mm-dd HH:MM:SS` in the modeltimezone.
  * Real numbers are parsed using unit conversion. If the target property has units and the property's field also specifies a unit, then the values are converted from field's unit to the target property's unit.
  * Integers are converted at 64 bit precision before being cast into the size of the target property.
  * Sets and enumerations may be received as integers, but string values are accepted (and probably preferable).
  * All other data types are extracted from the database as strings and are converted by GridLAB-D's built-in types conversion routines.

* **table** -
    Specifies the source table from which data is read.

* **Version** -
The mysql player was introduced in Hassayampa (Version 3.0). 

## See also

  * mysql module 
    * database class
    * recorder class
    * player class
    * collector class
    * MySQL Import/Export
  * Technical manuals 
    * Programmer's manual
    * MySQL How To Guide

