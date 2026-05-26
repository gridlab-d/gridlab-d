# xml


!!! warning

	This page contains features that are unfinished, were never implemented, or have since been deprecated. We preserve these pages for archival purposes, and also as a foundational resource for prospective developers who may wish to implement the same or similar feature. Many of these pages provide robust explanations of the theory behind a particular module or feature that we hope readers will find useful. 
	
	**This page does not reflect the current state of GridLAB-D™**
    
    
    module connection;
    class xml {
        version 1.0; 
        encoding UTF8|UTF16; 
        schema "xsd-url"; 
        stylesheet "xsl-url"; 
    }
    

## Class members

Member | Description
-- | --
**encoding** | Determine the encoding of characters.
**UTF8** | Character encoding uses 8 bits.
**UTF16** | Character encoding uses 16 bits.
**schema** | Identifies the XML schema ([xsd]) to use.
**stylesheet** | Identifies the XML stylesheet ([xsl]) to use.
**version** | Identifies the XML version to use.

## Example
    
    
    module connection;
    class xml 
    {
        mode SERVER; // enable server mode
        transport TCP; // use TCP for data transport
        version 1.0; // use XML version 1.0
        encoding connection:xml#UTF8; // use 8bit character encoding
        schema "http://www.gridlabd.org/gridlabd_3.0.xsd"; // use 3.0 schema
        stylesheet "http://www.gridlabd.org/gridlabd_3.0.xsl"; // use 3.0 stylesheet
    }
    
