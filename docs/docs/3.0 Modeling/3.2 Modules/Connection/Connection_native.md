# native

## Synopsis
    
    
    module connection;
    class native {
       version 3.0; 
    }
    

## Class members

### version

Identifies the gridlabd version to use.

## Example
    
    
    module connection;
    class native 
    {
       mode SERVER; // enable server mode
       transport TCP; // use TCP for data transport
       version 3.0; // use GridLAB-D Version 3.0 format
    }
    
