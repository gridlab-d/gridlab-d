# mode


    module connection;
    class [xml]|[json] {
      mode CLIENT|SERVER;
    }
    

### CLIENT

The connection is originated with GridLAB-D as client. The remote host is considered the server and is expected to accept multiple incoming connections from GridLAB-D.

### SERVER

The connection is originated with the remote host(s) as client. GridLAB-D is considered the server and is expected to accept multiple incoming connections from remote host(s).
