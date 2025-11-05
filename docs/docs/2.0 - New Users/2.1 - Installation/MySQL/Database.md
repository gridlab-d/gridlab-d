# Database

MySQL database connection

## Synopsis
    
    
    object database {
      hostname "localhost";
      username "gridlabd";
      password "";
      schema "gridlabd";
      port 3306;
      socketname "/tmp/mysql.sock";
      clientflags COMPRESS|FOUND_ROWS|IGNORE_SIGPIPE|INTERACTIVE|LOCAL_FILES|MULTI_RESULTS|MULTI_STATEMENTS|NO_SCHEMA|ODBC|SSL|REMEMBER_OPTIONS;
      options SHOWQUERY|NOCREATE|NEWDB|OVERWRITE;
      on_init _file-name_ ;
      on_sync _file-name_ ;
      on_term _file-name_ ;
      sync_interval _seconds_ ;
      tz_offset _seconds_ ; 
      uses_dst FALSE; 
    }
    

## Description

At least one database object must defined for recorder, player, and collector objects to function using a database. Typically, if no connection is specified by these objects, the last database defined will be used automatically. 

### hostname

    Specifies the MySQL server hostname. By default this is the default hostname, which is by default **127.0.0.1** or **localhost**.

### username

    Specifies the MySQL username. By default this is the default username, which is by default **gridlabd**.

### password

    Specifies the MySQL password. By default this is the default password, which is by default an empty string (i.e., none provided).

### schema

    Specifies the database schema to use. If none is specified, the default schema is used (which is by default _gridlabd_). If a blank schema name is specified, the modelname is used. The schema will be automatically created if it does not exist unless the NOCREATE option is specified.

### port

    Specifies the port used to access the MySQL server.

### socketname

    Specifies the socketname used to access the MySQL server (linux/Mac only)

### clientflags

    Sets client flags used when connecting to the server.

#### COMPRESS

    Enables data compression in the client/server protocol.

#### FOUND_ROWS

    Returns the number of rows found instead of the number of rows changed.

#### IGNORE_SIGPIPE

    Prevents the client library from installing a SIGPIPE signal handler. This can be used to avoid conflicts with a handler that the application has already installed.

#### INTERACTIVE

    Permit interactive_timeout seconds of inactivity (rather than wait_timeout seconds) before closing the connection. The client's session wait_timeout variable is set to the value of the session interactive_timeout variable.

#### LOCAL_FILES

    Enable `LOAD DATA LOCAL` handling.

#### MULTI_RESULTS

    Tell the server that the client can handle multiple result sets from multiple-statement executions or stored procedures. This flag is automatically enabled if MULTI_STATEMENTS is enabled. MULTI_RESULTS can be enabled when you call mysql_real_connect(), either explicitly by passing the MULTI_RESULTS flag itself, or implicitly by passing MULTI_STATEMENTS (which also enables MULTI_RESULTS). In MySQL 5.7, MULTI_RESULTS is enabled by default.

#### MULTI_STATEMENTS

    Tell the server that the client may send multiple statements in a single string (separated by ; characters). If this flag is not set, multiple-statement execution is disabled. If you enable MULTI_STATEMENTS or MULTI_RESULTS, process the result for every call to `mysql_query()` or `mysql_real_query()` by using a loop that calls `mysql_next_result()` to determine whether there are more results. For an example, see [Section 25.8.17, "C API Support for Multiple Statement Execution"](http://dev.mysql.com/doc/refman/5.7/en/c-api-multiple-queries.html).

#### NO_SCHEMA

    Do not permit db_name.tbl_name.col_name syntax. This is for ODBC. It causes the parser to generate an error if you use that syntax, which is useful for trapping bugs in some ODBC programs.

#### ODBC

    Unused.

#### SSL

    Use SSL (encrypted protocol). Do not set this option within an application program; it is set internally in the client library. Instead, use `mysql_ssl_set()` before calling `mysql_real_connect()`.

#### REMEMBER_OPTIONS

    Remember options specified by calls to mysql_options(). Without this option, if mysql_real_connect() fails, you must repeat the mysql_options() calls before trying to connect again. With this option, the mysql_options() calls need not be repeated.

### options

    Set the table handling options.

#### SHOWQUERY

    When the SHOWQUERY is specified, each query executed by the client is displayed as a verbose message.

#### NOCREATE

    Disable automatic creation of tables that don't exist.

#### NEWDB

    When the NEWDB option is specified, then the schema is destroyed before it used. CAUTION: this may cause loss of data and should be used with care, especially if the default schema _gridlabd_ is used.

#### OVERWRITE

    Enable deletion of existing data when an object initializes for output to a table. CAUTION: this may cause loss of data.

### on_init

    Specifies the MySQL script to execute when the database object is initialized. Scripts support all valid MySQL commands, as well as the DUMP and BACKUP commands.

### on_sync

    Specifies the MySQL script to execute when the database object is synchronized. Scripts support all valid MySQL commands, as well as the DUMP and BACKUP commands.

### on_term

    Specifies the MySQL script to execute when the database object is termination. Scripts support all valid MySQL commands, as well as the DUMP and BACKUP commands.

### sync_interval

    Specifies the interval between database object synchronization events.

### tz_offset

    Specifies the number of seconds offset between database times and object timestamps.

### uses_dst

    Specifies whether tz_offset should consider summer time or daylight savings time rules.

## Version

The database object was introduced in Hassayampa (Version 3.0). 

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

