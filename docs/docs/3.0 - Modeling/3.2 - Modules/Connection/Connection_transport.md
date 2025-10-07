# transport
TODO - Useful - Does transport need its own page? This is more of a definition.

## Synopsis
    
    
    module connection;
    class [xml]|[json] {
      transport #MEM|TCP|UDP;
    }
    

## Description

### MEM

    The transport is made using MMAP (windows) or SHMEM (*nix).

### TCP

    The transport is over a connection-based streaming socket.

### UDP

    The tranport is over a connectionless datagram socket.
