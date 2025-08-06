# security

Client/server connection module security control property. The security property of the connection module determine the default security configuration for classes implemented by the module. 


## Synopsis
    
    
    module connection {
      security NONE|LOW|NORMAL|HIGH|EXTREME|PARANOID;
    }
    

### NONE

No security controls are applied. Incoming requests are honored to the extent possible without regard to the consequences on the stability and consistency of the simulation. This is not recommend for general use, but it can be useful for debugging security problems.

### LOW

Limited security controls are applied. Incoming data/action requests are honored unless they are specifically forbidden by a link or action statement.

### NORMAL

Average security controls are applied. Incoming read requests are honored unless they are specifically forbidden by a link or action statement. Incoming write requests return an error unless they are specifically permitted by a link or action statement.

### HIGH

Extensive security controls are applied. Incoming data/action requests return an error unless they are specifically permitted by a link or action statement.

### EXTREME

All security conditions that normally result in an error code being returned are logged locally and no response is returned to the requestor.

### PARANOID

Same as EXTREME except that any security condition that violates the access controls results in a shutdown of the offending connection. The incoming address is ignored for future incoming connection for a duration controlled by the lockout module global variable.
