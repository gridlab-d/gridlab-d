# Player (mysql)

!!! warning

	This page contains features that are unfinished, were never implemented, or have since been deprecated. We preserve these pages for archival purposes, and also as a foundational resource for prospective developers who may wish to implement the same or similar feature. Many of these pages provide robust explanations of the theory behind a particular module or feature that we hope readers will find useful. 
  
	**This page does not reflect the current state of GridLAB-D™**

The **mysql player** is designed to be compatible the the **tape player object** so that when the mysql module is used in place of the tape module, there are few changes, if any, required to the player objects. 

The source table must have a sequence field, `id` , and a time field, `t`, to function as a player data source. 

  * The **sequence** field must be an integer and determine the sequence in which records are presented to the player.
  * The **time** field must be a timestamp, datetime, or numeric value and determines the time value used to post values to objects. If the time field is a timestamp or datetime, values are posted using absolute time. If the time field is a numeric value, values are posted in relative time with the first time value being relative to the epoch (1970-01-01 00:00:00 UTC). If the time field is a floating point value or the time field includes microseconds, the player will be operate using subsecond intervals.

## Synopsis
    
    module mysql;
    object player {
      property property-name;
      table|file source-table;
      mode {"r","r+"};
      filetype {"CSV"};
      connection database-object-name;
      options 0;
      loop number-of-loops;
    }
    

## Description

Parameter | Description
-- | --
**connection** | Specifies the database object used to connect to MySQL. If none is provided, the last database defined is used.
**file** | This is a synonym for table provided for compatibility with tape player.
**filetype** | Provided for compability with tape player and has no real effect.
**loop** | The input data table will be treated as a tape loop if the field type is an integer or double. The loop property determines how many time the input data table will rewind before the player ceases to update the target object's properties.
**mode** | Specifies the read mode to use, which may be either "r" or "r+". This is provided for compatibility with tape player and has no real affect.
**options** | No options are supported at this time.
**property** | Specifies the target property (or properties) that are to be updated. The properties must match the field names in the source table. The data types are automatically converted using the following rules: <br/>  * Timestamps are parsed as `yyyy-mm-dd HH:MM:SS` in the modeltimezone. <br/> * Real numbers are parsed using unit conversion. If the target property has units and the property's field also specifies a unit, then the values are converted from field's unit to the target property's unit. <br/> * Integers are converted at 64 bit precision before being cast into the size of the target property. <br/> * Sets and enumerations may be received as integers, but string values are accepted (and probably preferable). <br/> * All other data types are extracted from the database as strings and are converted by GridLAB-D™'s built-in types conversion routines.
**table** | Specifies the source table from which data is read.


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

