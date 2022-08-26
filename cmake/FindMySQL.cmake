#
# Find the MySQL client includes and library
#

# This module defines
# MYSQL_INCLUDE_DIRECTORIES, where to find mysql.h
# MYSQL_LIBRARIES, the libraries to link against to connect to MySQL
# MYSQL_FOUND, If false, you cannot build anything that requires MySQL.

# also defined, but not for general use are
# MYSQL_LIBRARY, where to find the MySQL library.

SET( MYSQL_FOUND 0 )

FIND_PATH(MYSQL_INCLUDE_DIRECTORIES mysql.h
        /usr/include
        /usr/include/mysql
        /usr/local/include
        /usr/local/include/mysql
        /usr/local/mysql/include
        "C:/Program Files/MySQL/include"
        "C:/Program Files/MySQL/MySQL Server 5.0/include"
        "C:/MySQL/include"
        "[HKEY_LOCAL_MACHINE\\SOFTWARE\\MySQL AB\\MySQL Server 5.0;Location]/include"
        "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\MySQL AB\\MySQL Server 5.0;Location]/include"
        PATH_SUFFIXES mysql
        DOC "Specify the directory containing mysql.h."
        )

FIND_LIBRARY( MYSQL_LIBRARY
        NAMES mysql libmysql mysqlclient
        PATHS
        /usr/lib
        /usr/lib/mysql
        /usr/local/lib
        /usr/local/lib/mysql
        /usr/local/mysql/lib
        "C:/Program Files/MySQL/lib"
        "C:/Program Files/MySQL/MySQL Server 5.0/lib/opt"
        "C:/MySQL/lib/debug"
        "[HKEY_LOCAL_MACHINE\\SOFTWARE\\MySQL AB\\MySQL Server 5.0;Location]/lib/opt"
        "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\MySQL AB\\MySQL Server 5.0;Location]/lib/opt"
        DOC "Specify the mysql library here."
        )

# On Windows you typically don't need to include any extra libraries
# to build MYSQL stuff.

IF (NOT WIN32)
    FIND_LIBRARY( MYSQL_EXTRA_LIBRARIES
            NAMES z zlib
            PATHS /usr/lib /usr/local/lib
            DOC "If more libraries are necessary to link in a MySQL client (typically zlib), specify them here.")
ELSE ()
    SET( MYSQL_EXTRA_LIBRARIES "" )
ENDIF ()


IF (MYSQL_LIBRARY)
    IF (MYSQL_INCLUDE_DIRECTORIES)
        SET( MYSQL_FOUND 1 )
    ENDIF ()
ENDIF ()

MARK_AS_ADVANCED( MYSQL_FOUND MYSQL_LIBRARY MYSQL_EXTRA_LIBRARIES MYSQL_INCLUDE_DIRECTORIES )
