# transport


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
