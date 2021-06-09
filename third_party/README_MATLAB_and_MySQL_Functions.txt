** This information is in the README_MATLAB_and_MySQL_Functions.txt file in the GridLAB-D install folder. **

This version of GridLAB-D was compiled against the Mathworks MATLAB R2020b 64-bit engine extensions and the MySQL Connector/C 6.1.11.

== MATLAB ==
To utilize the MATLAB connection, you will need a properly licensed version of MATLAB with the libeng.dll and libmx.dll files, which should be part of the standard MATLAB distribution.

== MySQL ==
To utilize the MySQL connector, you will need to download the MySQL C-connector x64 file (mysql-connector-c-6.1.11-winx64.zip) from https://downloads.mysql.com/archives/c-c/.  Note that you should only need the libmysql.dll from that package - copy it to your path, or place it in the "lib" folder of your GridLAB-D install.