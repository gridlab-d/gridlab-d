# xml

## Synopsis
    
    
    module [connection];
    class xml {
       version 1.0; 
       encoding UTF8|UTF16; 
       schema "_[xsd]-url_"; 
       stylesheet "_[xsl]-url_"; 
    }
    

## Class members

### encoding

    Determine the encoding of characters.

#### UTF8

    Character encoding uses 8 bits.

#### UTF16

    Character encoding uses 16 bits.

### schema

    Identifies the XML schema ([xsd]) to use.

### stylesheet

    Identifies the XML stylesheet ([xsl]) to use.

### version

    Identifies the XML version to use.

## Example
    
    
    module [connection];
    class xml 
    {
       [mode] [SERVER]; // enable server mode
       [transport] [TCP]; // use TCP for data transport
       [version] 1.0; // use XML version 1.0
       [encoding] [connection:xml#UTF8]; // use 8bit character encoding
       [schema] "<http://www.gridlabd.org/gridlabd_3.0.xsd>"; // use 3.0 schema
       [stylesheet] "<http://www.gridlabd.org/gridlabd_3.0.xsl>"; // use 3.0 stylesheet
    }
    
