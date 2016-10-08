% MYSQL - Interact with a MySQL database server
%
%   If no output arguments are given on the left, then display results.
%   If arguments are given, then return requested data silently.
%
%   mysql( 'open', host, user, password )
%      Open a connection with specified parameters, or defaults if none
%            host:  default is local host. Use colon for port number
%            user:  default is Unix login name.
%        password:  default says connect without password.
%
%       Examples: mysql('open','arkiv')     %  connect on default port
%                 mysql('open','arkiv:2215')
%
%   If successful, open returns the new connection handle.
%   If unsuccessful, it throws an error and returns nothing.
%
%   mysql('close')
%      Close the current connection
%
%   mysql('use',db)  or   mysql('use db')
%      Set the current database to db   Example:  mysql('use cme')
%
%   mysql('status')
%      Display information about the connection and the server.
%      Return    0  if connection is open and functioning
%                1  if connection is closed
%                2  if should be open but we cannot ping the server
%
%   mysql( query )
%      Send the given query or command to the MySQL server
%
%      If arguments are given on the left, then each argument
%          is set to the column of the returned query.
%      Dates and times in result are converted to Matlab format:
%          dates are serial day number, and times are fraction of day.
%      String variables are returned as cell arrays.
%
%      Example:
%      [ t, p ] = mysql('select time,price,askbid from cme.sp
%                  where date="1997-04-30" and expir like "1997-06-%"');
%         (but be sure to put quoted text all on one input line)
%      Returns time and price for trades on the June 1997 contract
%      that occured on April 30, 1997.
%
%   All string comparisons are case-insensitive
%
%   Multiple connections:  The program can maintain up to 10
%   independent connections. Any command may be preceded by a
%   connection handle -- an integer from 0 to 9 -- to apply the
%   command to that connection.
%   Example:
%      mysql(5,'open','host2')  %  connection 5 to host 2
%      mysql                    %  status of all connections
%
%   If no connection handle is given then
%      an OPEN statement will find an unused handle
%          and return the value
%      any other statement will use the most recently
%          used handle. When the current connection is closed,
%          "most recent" reverts to its previous value.
%
%   This permits nested constructions of the form
%
%      mysql('open','data1',...)
%      mysql('use something')
%      mysql('do something')
%
%          perhaps inside another .m file
%            mysql('open','data2',...)
%             ...
%            mysql('close')
%
%      mysql('do something else to data1')
%      mysql('close')
%
%   The special command 'closeall' closes all open connections
%   The special case 'mysql' or 'mysql()' with no arguments
%      shows status of all open connections (returns nothing).
