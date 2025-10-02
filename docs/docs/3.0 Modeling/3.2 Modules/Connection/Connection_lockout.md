# lockout
TODO - Useful - Does lockout need its own page? This is more of a definition.

Client/server connection module security lockout time control property 

## Synopsis
    
    
    module connection {
      lockout 10 s;
    }
    

## Description

The lockout property of the connection module determines the time that a server will lock out a client when it violates an EXTREME security constraint. Lockouts only occur when security is set to PARANOID. 
